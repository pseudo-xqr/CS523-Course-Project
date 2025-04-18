// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
/* Copyright(c) 2014 - 2021 Intel Corporation */
#ifdef QAT_PKE_OLD_SUPPORTED
#ifdef QAT_RSA_SUPPORT
#include <linux/version.h>
#if KERNEL_VERSION(4, 4, 0) <= LINUX_VERSION_CODE
#define QAT_HAVE_RSA_SUPPORT
#endif
#endif
#ifdef QAT_HAVE_RSA_SUPPORT

#include <linux/module.h>
#include <crypto/internal/rsa.h>
#include <crypto/internal/akcipher.h>
#include <crypto/akcipher.h>
#include <linux/dma-mapping.h>
#include <linux/fips.h>
#include <crypto/scatterwalk.h>
#include "qat_rsapubkey-asn1.h"
#include "qat_rsaprivkey-asn1.h"
#include "icp_qat_fw_pke.h"
#include "adf_accel_devices.h"
#include "adf_transport.h"
#include "adf_common_drv.h"
#include "qat_crypto.h"

static DEFINE_MUTEX(algs_lock);
static unsigned int active_devs;

struct qat_rsa_input_params {
	union {
		struct {
			u64 m;
			u64 e;
			u64 n;
		} enc;
		struct {
			u64 c;
			u64 d;
			u64 n;
		} dec;
		u64 in_tab[8];
	};
} __packed __aligned(64);

struct qat_rsa_output_params {
	union {
		struct {
			u64 c;
		} enc;
		struct {
			u64 m;
		} dec;
		u64 out_tab[8];
	};
} __packed __aligned(64);

struct qat_rsa_ctx {
	char *n;
	char *e;
	char *d;
	dma_addr_t dma_n;
	dma_addr_t dma_e;
	dma_addr_t dma_d;
	unsigned int key_sz;
	struct qat_crypto_instance *inst;
} __packed __aligned(64);

struct qat_rsa_request {
	struct qat_rsa_input_params in;
	struct qat_rsa_output_params out;
	dma_addr_t phy_in;
	dma_addr_t phy_out;
	char *src_align;
	char *dst_align;
	struct icp_qat_fw_pke_request req;
	struct qat_rsa_ctx *ctx;
	int err;
} __aligned(64);

static void qat_rsa_cb(struct icp_qat_fw_pke_resp *resp)
{
	struct akcipher_request *areq = (void *)(__force long)resp->opaque;
	struct qat_rsa_request *req = PTR_ALIGN(akcipher_request_ctx(areq), 64);
	struct device *dev = &GET_DEV(req->ctx->inst->accel_dev);
	int err = ICP_QAT_FW_PKE_RESP_PKE_STAT_GET(
				resp->pke_resp_hdr.comn_resp_flags);

	err = (err == ICP_QAT_FW_COMN_STATUS_FLAG_OK) ? 0 : -EINVAL;

	dma_unmap_single(dev, req->in.enc.m, req->ctx->key_sz,
			 DMA_TO_DEVICE);

	kfree_sensitive(req->src_align);

	areq->dst_len = req->ctx->key_sz;
	if (req->dst_align) {
		char *ptr = req->dst_align;

		while (!(*ptr) && areq->dst_len) {
			areq->dst_len--;
			ptr++;
		}

		if (areq->dst_len != req->ctx->key_sz)
			memmove(req->dst_align, ptr, areq->dst_len);

		scatterwalk_map_and_copy(req->dst_align, areq->dst, 0,
					 areq->dst_len, 1);

		kfree_sensitive(req->dst_align);
	} else {
		char *ptr = sg_virt(areq->dst);

		while (!(*ptr) && areq->dst_len) {
			areq->dst_len--;
			ptr++;
		}

		if (sg_virt(areq->dst) != ptr && areq->dst_len)
			memmove(sg_virt(areq->dst), ptr, areq->dst_len);
	}
	dma_unmap_single(dev, req->out.enc.c, req->ctx->key_sz,
			 DMA_FROM_DEVICE);

	dma_unmap_single(dev, req->phy_in, sizeof(struct qat_rsa_input_params),
			 DMA_TO_DEVICE);
	dma_unmap_single(dev, req->phy_out,
			 sizeof(struct qat_rsa_output_params),
			 DMA_TO_DEVICE);

	akcipher_request_complete(areq, err);
}

void qat_alg_asym_callback(void *_resp)
{
	struct icp_qat_fw_pke_resp *resp = _resp;

	qat_rsa_cb(resp);
}

#define PKE_RSA_EP_512 0x1c161b21
#define PKE_RSA_EP_1024 0x35111bf7
#define PKE_RSA_EP_1536 0x4d111cdc
#define PKE_RSA_EP_2048 0x6e111dba
#define PKE_RSA_EP_3072 0x7d111ea3
#define PKE_RSA_EP_4096 0xa5101f7e

static unsigned long qat_rsa_enc_fn_id(unsigned int len)
{
	unsigned int bitslen = len << 3;

	switch (bitslen) {
	case 512:
		return PKE_RSA_EP_512;
	case 1024:
		return PKE_RSA_EP_1024;
	case 1536:
		return PKE_RSA_EP_1536;
	case 2048:
		return PKE_RSA_EP_2048;
	case 3072:
		return PKE_RSA_EP_3072;
	case 4096:
		return PKE_RSA_EP_4096;
	default:
		return 0;
	};
}

#define PKE_RSA_DP1_512 0x1c161b3c
#define PKE_RSA_DP1_1024 0x35111c12
#define PKE_RSA_DP1_1536 0x4d111cf7
#define PKE_RSA_DP1_2048 0x6e111dda
#define PKE_RSA_DP1_3072 0x7d111ebe
#define PKE_RSA_DP1_4096 0xa5101f98

static unsigned long qat_rsa_dec_fn_id(unsigned int len)
{
	unsigned int bitslen = len << 3;

	switch (bitslen) {
	case 512:
		return PKE_RSA_DP1_512;
	case 1024:
		return PKE_RSA_DP1_1024;
	case 1536:
		return PKE_RSA_DP1_1536;
	case 2048:
		return PKE_RSA_DP1_2048;
	case 3072:
		return PKE_RSA_DP1_3072;
	case 4096:
		return PKE_RSA_DP1_4096;
	default:
		return 0;
	};
}

static int qat_rsa_enc(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct qat_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct qat_crypto_instance *inst = ctx->inst;
	struct device *dev = &GET_DEV(inst->accel_dev);
	struct qat_rsa_request *qat_req =
			PTR_ALIGN(akcipher_request_ctx(req), 64);
	struct icp_qat_fw_pke_request *msg = &qat_req->req;
	u8 *vaddr;
	int ret, ctr = 0;

	if (unlikely(!ctx->n || !ctx->e))
		return -EINVAL;

	if (req->dst_len < ctx->key_sz) {
		req->dst_len = ctx->key_sz;
		return -EOVERFLOW;
	}

	if (req->src_len > ctx->key_sz)
		return -EINVAL;

	memset(msg, '\0', sizeof(*msg));
	ICP_QAT_FW_PKE_HDR_VALID_FLAG_SET(msg->pke_hdr,
					  ICP_QAT_FW_COMN_REQ_FLAG_SET);
	msg->pke_hdr.cd_pars.func_id = qat_rsa_enc_fn_id(ctx->key_sz);
	if (unlikely(!msg->pke_hdr.cd_pars.func_id))
		return -EINVAL;

	qat_req->ctx = ctx;
	msg->pke_hdr.service_type = ICP_QAT_FW_COMN_REQ_CPM_FW_PKE;
	msg->pke_hdr.comn_req_flags =
		ICP_QAT_FW_COMN_FLAGS_BUILD(QAT_COMN_PTR_TYPE_FLAT,
					    QAT_COMN_CD_FLD_TYPE_64BIT_ADR);

	qat_req->in.enc.e = ctx->dma_e;
	qat_req->in.enc.n = ctx->dma_n;
	ret = -ENOMEM;

	/*
	 * src can be of any size in valid range, but HW expects it to be the
	 * same as modulo n so in case it is different we need to allocate a
	 * new buf and copy src data.
	 * In other case we just need to map the user provided buffer.
	 * Also need to make sure that it is in contiguous buffer.
	 */
	if (sg_is_last(req->src) && req->src_len == ctx->key_sz) {
		qat_req->src_align = NULL;

		vaddr = sg_virt(req->src);
	} else {
		int shift = ctx->key_sz - req->src_len;

		qat_req->src_align = kzalloc(ctx->key_sz, GFP_KERNEL);
		if (unlikely(!qat_req->src_align))
			return ret;

		scatterwalk_map_and_copy(qat_req->src_align + shift, req->src,
					 0, req->src_len, 0);
		vaddr = qat_req->src_align;
	}
	qat_req->in.enc.m = dma_map_single(dev, vaddr, ctx->key_sz,
					   DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->in.enc.m)))
		goto unmap_src;

	if (sg_is_last(req->dst) && req->dst_len == ctx->key_sz) {
		qat_req->dst_align = NULL;
	vaddr = sg_virt(req->dst);

	} else {
		qat_req->dst_align = kzalloc(ctx->key_sz, GFP_KERNEL);
		if (unlikely(!qat_req->dst_align))
			goto unmap_src;
		vaddr = qat_req->dst_align;
	}
	qat_req->out.enc.c = dma_map_single(dev, vaddr, ctx->key_sz,
					    DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->out.enc.c)))
		goto unmap_dst;
	qat_req->in.in_tab[3] = 0;
	qat_req->out.out_tab[1] = 0;
	qat_req->phy_in = dma_map_single(dev, &qat_req->in.enc.m,
					 sizeof(struct qat_rsa_input_params),
					 DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->phy_in)))
		goto unmap_dst;

	qat_req->phy_out = dma_map_single(dev, &qat_req->out.enc.c,
					  sizeof(struct qat_rsa_output_params),
					  DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->phy_out)))
		goto unmap_in_params;

	msg->pke_mid.src_data_addr = qat_req->phy_in;
	msg->pke_mid.dest_data_addr = qat_req->phy_out;
	msg->pke_mid.opaque = (uint64_t)(__force long)req;
	msg->input_param_count = 3;
	msg->output_param_count = 1;
	do {
		ret = adf_send_message(ctx->inst->pke_tx, (uint32_t *)msg);
	} while (ret == -EBUSY && ctr++ < 100);

	if (!ret)
		return -EINPROGRESS;

	if (!dma_mapping_error(dev, qat_req->phy_out))
		dma_unmap_single(dev, qat_req->phy_out,
				 sizeof(struct qat_rsa_output_params),
				 DMA_TO_DEVICE);
unmap_in_params:
	if (!dma_mapping_error(dev, qat_req->phy_in))
		dma_unmap_single(dev, qat_req->phy_in,
				 sizeof(struct qat_rsa_input_params),
				 DMA_TO_DEVICE);
unmap_dst:
	if (!dma_mapping_error(dev, qat_req->out.enc.c))
		dma_unmap_single(dev, qat_req->out.enc.c, ctx->key_sz,
				 DMA_FROM_DEVICE);
	kfree_sensitive(qat_req->dst_align);
unmap_src:
	if (!dma_mapping_error(dev, qat_req->in.enc.m))
		dma_unmap_single(dev, qat_req->in.enc.m, ctx->key_sz,
				 DMA_TO_DEVICE);
	kfree_sensitive(qat_req->src_align);
	return ret;
}

static int qat_rsa_dec(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct qat_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct qat_crypto_instance *inst = ctx->inst;
	struct device *dev = &GET_DEV(inst->accel_dev);
	struct qat_rsa_request *qat_req =
			PTR_ALIGN(akcipher_request_ctx(req), 64);
	struct icp_qat_fw_pke_request *msg = &qat_req->req;
	u8 *vaddr;
	int ret, ctr = 0;

	if (unlikely(!ctx->n || !ctx->d))
		return -EINVAL;

	if (req->dst_len < ctx->key_sz) {
		req->dst_len = ctx->key_sz;
		return -EOVERFLOW;
	}

	if (req->src_len > ctx->key_sz)
		return -EINVAL;

	memset(msg, '\0', sizeof(*msg));
	ICP_QAT_FW_PKE_HDR_VALID_FLAG_SET(msg->pke_hdr,
					  ICP_QAT_FW_COMN_REQ_FLAG_SET);
	msg->pke_hdr.cd_pars.func_id = qat_rsa_dec_fn_id(ctx->key_sz);
	if (unlikely(!msg->pke_hdr.cd_pars.func_id))
		return -EINVAL;

	qat_req->ctx = ctx;
	msg->pke_hdr.service_type = ICP_QAT_FW_COMN_REQ_CPM_FW_PKE;
	msg->pke_hdr.comn_req_flags =
		ICP_QAT_FW_COMN_FLAGS_BUILD(QAT_COMN_PTR_TYPE_FLAT,
					    QAT_COMN_CD_FLD_TYPE_64BIT_ADR);

	qat_req->in.dec.d = ctx->dma_d;
	qat_req->in.dec.n = ctx->dma_n;
	ret = -ENOMEM;

	/*
	 * src can be of any size in valid range, but HW expects it to be the
	 * same as modulo n so in case it is different we need to allocate a
	 * new buf and copy src data.
	 * In other case we just need to map the user provided buffer.
	 * Also need to make sure that it is in contiguous buffer.
	 */
	if (sg_is_last(req->src) && req->src_len == ctx->key_sz) {
		qat_req->src_align = NULL;
		vaddr = sg_virt(req->src);
	} else {
		int shift = ctx->key_sz - req->src_len;

		qat_req->src_align = kzalloc(ctx->key_sz, GFP_KERNEL);
		if (unlikely(!qat_req->src_align))
			return ret;

		scatterwalk_map_and_copy(qat_req->src_align + shift, req->src,
					 0, req->src_len, 0);
		vaddr = qat_req->src_align;
	}
	qat_req->in.dec.c = dma_map_single(dev, vaddr, ctx->key_sz,
					   DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->in.dec.c)))
		goto unmap_src;

	if (sg_is_last(req->dst) && req->dst_len == ctx->key_sz) {
		qat_req->dst_align = NULL;
		vaddr = sg_virt(req->dst);

	} else {
		qat_req->dst_align = kzalloc(ctx->key_sz, GFP_KERNEL);
		if (unlikely(!qat_req->dst_align))
			goto unmap_src;
		vaddr = qat_req->dst_align;
	}
	qat_req->out.dec.m = dma_map_single(dev, vaddr, ctx->key_sz,
					    DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->out.dec.m)))
		goto unmap_dst;
	qat_req->in.in_tab[3] = 0;
	qat_req->out.out_tab[1] = 0;
	qat_req->phy_in = dma_map_single(dev, &qat_req->in.dec.c,
					 sizeof(struct qat_rsa_input_params),
					 DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->phy_in)))
		goto unmap_dst;

	qat_req->phy_out = dma_map_single(dev, &qat_req->out.dec.m,
					  sizeof(struct qat_rsa_output_params),
					  DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, qat_req->phy_out)))
		goto unmap_in_params;

	msg->pke_mid.src_data_addr = qat_req->phy_in;
	msg->pke_mid.dest_data_addr = qat_req->phy_out;
	msg->pke_mid.opaque = (uint64_t)(__force long)req;
	msg->input_param_count = 3;
	msg->output_param_count = 1;
	do {
		ret = adf_send_message(ctx->inst->pke_tx, (uint32_t *)msg);
	} while (ret == -EBUSY && ctr++ < 100);

	if (!ret)
		return -EINPROGRESS;

	if (!dma_mapping_error(dev, qat_req->phy_out))
		dma_unmap_single(dev, qat_req->phy_out,
				 sizeof(struct qat_rsa_output_params),
				 DMA_TO_DEVICE);
unmap_in_params:
	if (!dma_mapping_error(dev, qat_req->phy_in))
		dma_unmap_single(dev, qat_req->phy_in,
				 sizeof(struct qat_rsa_input_params),
				 DMA_TO_DEVICE);
unmap_dst:
	if (!dma_mapping_error(dev, qat_req->out.dec.m))
		dma_unmap_single(dev, qat_req->out.dec.m, ctx->key_sz,
				 DMA_FROM_DEVICE);
	kfree_sensitive(qat_req->dst_align);
unmap_src:
	if (!dma_mapping_error(dev, qat_req->in.dec.c))
		dma_unmap_single(dev, qat_req->in.dec.c, ctx->key_sz,
				 DMA_TO_DEVICE);
	kfree_sensitive(qat_req->src_align);
	return ret;
}

int qat_rsa_get_n(void *context, size_t hdrlen, unsigned char tag,
		  const void *value, size_t vlen)
{
	struct qat_rsa_ctx *ctx = context;
	struct qat_crypto_instance *inst = ctx->inst;
	struct device *dev = &GET_DEV(inst->accel_dev);
	const char *ptr = value;
	int ret;

	while (!*ptr && vlen) {
		ptr++;
		vlen--;
	}

	ctx->key_sz = vlen;
	ret = -EINVAL;
	/* In FIPS mode only allow key size 2K & 3K */
	if (fips_enabled && (ctx->key_sz != 256 && ctx->key_sz != 384)) {
		pr_err("QAT: RSA: key size not allowed in FIPS mode\n");
		goto err;
	}
	/* invalid key size provided */
	if (!qat_rsa_enc_fn_id(ctx->key_sz))
		goto err;

	ret = -ENOMEM;
	ctx->n = dma_alloc_coherent(dev, ctx->key_sz, &ctx->dma_n, GFP_KERNEL);
	if (!ctx->n)
		goto err;

	memcpy(ctx->n, ptr, ctx->key_sz);
	return 0;
err:
	ctx->key_sz = 0;
	ctx->n = NULL;
	return ret;
}

int qat_rsa_get_e(void *context, size_t hdrlen, unsigned char tag,
		  const void *value, size_t vlen)
{
	struct qat_rsa_ctx *ctx = context;
	struct qat_crypto_instance *inst = ctx->inst;
	struct device *dev = &GET_DEV(inst->accel_dev);
	const char *ptr = value;

	while (!*ptr && vlen) {
		ptr++;
		vlen--;
	}

	if (!ctx->key_sz || !vlen || vlen > ctx->key_sz) {
		ctx->e = NULL;
		return -EINVAL;
	}

	ctx->e = dma_alloc_coherent(dev, ctx->key_sz, &ctx->dma_e, GFP_KERNEL);
	if (!ctx->e)
		return -ENOMEM;

	memcpy(ctx->e + (ctx->key_sz - vlen), ptr, vlen);
	return 0;
}

int qat_rsa_get_d(void *context, size_t hdrlen, unsigned char tag,
		  const void *value, size_t vlen)
{
	struct qat_rsa_ctx *ctx = context;
	struct qat_crypto_instance *inst = ctx->inst;
	struct device *dev = &GET_DEV(inst->accel_dev);
	const char *ptr = value;
	int ret;

	while (!*ptr && vlen) {
		ptr++;
		vlen--;
	}

	ret = -EINVAL;
	if (!ctx->key_sz || !vlen || vlen > ctx->key_sz)
		goto err;

	/* In FIPS mode only allow key size 2K & 3K */
	if (fips_enabled && (vlen != 256 && vlen != 384)) {
		pr_err("QAT: RSA: key size not allowed in FIPS mode\n");
		goto err;
	}

	ret = -ENOMEM;
	ctx->d = dma_alloc_coherent(dev, ctx->key_sz, &ctx->dma_d, GFP_KERNEL);
	if (!ctx->d)
		goto err;

	memcpy(ctx->d + (ctx->key_sz - vlen), ptr, vlen);
	return 0;
err:
	ctx->d = NULL;
	return ret;
}

static int qat_rsa_setkey(struct crypto_akcipher *tfm, const void *key,
			  unsigned int keylen, bool private)
{
	struct qat_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct device *dev = &GET_DEV(ctx->inst->accel_dev);
	int ret;

	/* Free the old key if any */
	if (ctx->n)
		dma_free_coherent(dev, ctx->key_sz, ctx->n, ctx->dma_n);
	if (ctx->e)
		dma_free_coherent(dev, ctx->key_sz, ctx->e, ctx->dma_e);
	if (ctx->d) {
		memset(ctx->d, '\0', ctx->key_sz);
		dma_free_coherent(dev, ctx->key_sz, ctx->d, ctx->dma_d);
	}

	ctx->n = NULL;
	ctx->e = NULL;
	ctx->d = NULL;

	if (private)
		ret = asn1_ber_decoder(&qat_rsaprivkey_decoder, ctx, key,
				       keylen);
	else
		ret = asn1_ber_decoder(&qat_rsapubkey_decoder, ctx, key,
				       keylen);
	if (ret < 0)
		goto free;

	if (!ctx->n || !ctx->e) {
		/* invalid key provided */
		ret = -EINVAL;
		goto free;
	}
	if (private && !ctx->d) {
		/* invalid private key provided */
		ret = -EINVAL;
		goto free;
	}

	return 0;
free:
	if (ctx->d) {
		memset(ctx->d, '\0', ctx->key_sz);
		dma_free_coherent(dev, ctx->key_sz, ctx->d, ctx->dma_d);
		ctx->d = NULL;
	}
	if (ctx->e) {
		dma_free_coherent(dev, ctx->key_sz, ctx->e, ctx->dma_e);
		ctx->e = NULL;
	}
	if (ctx->n) {
		dma_free_coherent(dev, ctx->key_sz, ctx->n, ctx->dma_n);
		ctx->n = NULL;
		ctx->key_sz = 0;
	}
	return ret;
}

static int qat_rsa_setpubkey(struct crypto_akcipher *tfm, const void *key,
			     unsigned int keylen)
{
	return qat_rsa_setkey(tfm, key, keylen, false);
}

static int qat_rsa_setprivkey(struct crypto_akcipher *tfm, const void *key,
			      unsigned int keylen)
{
	return qat_rsa_setkey(tfm, key, keylen, true);
}

static QAT_PKE_MAX_SZ_SIGN int qat_rsa_max_size(struct crypto_akcipher *tfm)
{
	struct qat_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);

	return (ctx->n) ? ctx->key_sz : -EINVAL;
}

static int qat_rsa_init_tfm(struct crypto_akcipher *tfm)
{
	struct qat_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct qat_crypto_instance *inst =
			qat_crypto_get_instance_node(get_current_node(), ASYM);

	if (!inst)
		return -EINVAL;

	ctx->key_sz = 0;
	ctx->inst = inst;
	return 0;
}

static void qat_rsa_exit_tfm(struct crypto_akcipher *tfm)
{
	struct qat_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct device *dev = &GET_DEV(ctx->inst->accel_dev);

	if (ctx->n)
		dma_free_coherent(dev, ctx->key_sz, ctx->n, ctx->dma_n);
	if (ctx->e)
		dma_free_coherent(dev, ctx->key_sz, ctx->e, ctx->dma_e);
	if (ctx->d) {
		memset(ctx->d, '\0', ctx->key_sz);
		dma_free_coherent(dev, ctx->key_sz, ctx->d, ctx->dma_d);
	}
	qat_crypto_put_instance(ctx->inst);
	ctx->n = NULL;
	ctx->e = NULL;
	ctx->d = NULL;
}

static struct akcipher_alg rsa = {
	.encrypt = qat_rsa_enc,
	.decrypt = qat_rsa_dec,
	.sign = qat_rsa_dec,
	.verify = qat_rsa_enc,
	.set_pub_key = qat_rsa_setpubkey,
	.set_priv_key = qat_rsa_setprivkey,
	.max_size = qat_rsa_max_size,
	.init = qat_rsa_init_tfm,
	.exit = qat_rsa_exit_tfm,
	.reqsize = sizeof(struct qat_rsa_request) + 64,
	.base = {
		.cra_name = "rsa",
		.cra_driver_name = "qat-rsa",
		.cra_priority = 1000,
		.cra_module = THIS_MODULE,
		.cra_ctxsize = sizeof(struct qat_rsa_ctx),
	},
};

int qat_asym_algs_register(void)
{
	int ret = 0;

	mutex_lock(&algs_lock);
	if (++active_devs == 1) {
		rsa.base.cra_flags = 0;
		ret = crypto_register_akcipher(&rsa);
	}
	mutex_unlock(&algs_lock);
	return ret;
}

void qat_asym_algs_unregister(void)
{
	mutex_lock(&algs_lock);
	if (--active_devs == 0)
		crypto_unregister_akcipher(&rsa);
	mutex_unlock(&algs_lock);
}
#else
#include <linux/types.h>

int qat_asym_algs_register(void)
{
	return 0;
}

void qat_asym_algs_unregister(void)
{
}

void qat_alg_asym_callback(void *_resp)
{
}

int qat_rsa_get_n(void *p1, size_t p2, unsigned char p3, const void *p4,
		  size_t p5)
{
	return 0;
}

int qat_rsa_get_e(void *p1, size_t p2, unsigned char p3, const void *p4,
		  size_t p5)
{
	return 0;
}

int qat_rsa_get_d(void *p1, size_t p2, unsigned char p3, const void *p4,
		  size_t p5)
{
	return 0;
}
#endif
#endif // QAT_PKE_OLD_SUPPORTED
