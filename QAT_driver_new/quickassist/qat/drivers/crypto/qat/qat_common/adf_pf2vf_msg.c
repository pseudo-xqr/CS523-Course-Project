// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
/* Copyright(c) 2014 - 2021 Intel Corporation */

#include <linux/delay.h>
#include <linux/spinlock.h>
#include "adf_accel_devices.h"
#include "adf_common_drv.h"
#include "adf_pf2vf_msg.h"

adf_iov_block_provider pf2vf_message_providers
				[ADF_VF2PF_MAX_LARGE_MESSAGE_TYPE + 1];
unsigned char pfvf_crc8_table[] = {
0x00, 0x97, 0xB9, 0x2E, 0xE5, 0x72, 0x5C, 0xCB,
0x5D, 0xCA, 0xE4, 0x73, 0xB8, 0x2F, 0x01, 0x96,
0xBA, 0x2D, 0x03, 0x94, 0x5F, 0xC8, 0xE6, 0x71,
0xE7, 0x70, 0x5E, 0xC9, 0x02, 0x95, 0xBB, 0x2C,
0xE3, 0x74, 0x5A, 0xCD, 0x06, 0x91, 0xBF, 0x28,
0xBE, 0x29, 0x07, 0x90, 0x5B, 0xCC, 0xE2, 0x75,
0x59, 0xCE, 0xE0, 0x77, 0xBC, 0x2B, 0x05, 0x92,
0x04, 0x93, 0xBD, 0x2A, 0xE1, 0x76, 0x58, 0xCF,
0x51, 0xC6, 0xE8, 0x7F, 0xB4, 0x23, 0x0D, 0x9A,
0x0C, 0x9B, 0xB5, 0x22, 0xE9, 0x7E, 0x50, 0xC7,
0xEB, 0x7C, 0x52, 0xC5, 0x0E, 0x99, 0xB7, 0x20,
0xB6, 0x21, 0x0F, 0x98, 0x53, 0xC4, 0xEA, 0x7D,
0xB2, 0x25, 0x0B, 0x9C, 0x57, 0xC0, 0xEE, 0x79,
0xEF, 0x78, 0x56, 0xC1, 0x0A, 0x9D, 0xB3, 0x24,
0x08, 0x9F, 0xB1, 0x26, 0xED, 0x7A, 0x54, 0xC3,
0x55, 0xC2, 0xEC, 0x7B, 0xB0, 0x27, 0x09, 0x9E,
0xA2, 0x35, 0x1B, 0x8C, 0x47, 0xD0, 0xFE, 0x69,
0xFF, 0x68, 0x46, 0xD1, 0x1A, 0x8D, 0xA3, 0x34,
0x18, 0x8F, 0xA1, 0x36, 0xFD, 0x6A, 0x44, 0xD3,
0x45, 0xD2, 0xFC, 0x6B, 0xA0, 0x37, 0x19, 0x8E,
0x41, 0xD6, 0xF8, 0x6F, 0xA4, 0x33, 0x1D, 0x8A,
0x1C, 0x8B, 0xA5, 0x32, 0xF9, 0x6E, 0x40, 0xD7,
0xFB, 0x6C, 0x42, 0xD5, 0x1E, 0x89, 0xA7, 0x30,
0xA6, 0x31, 0x1F, 0x88, 0x43, 0xD4, 0xFA, 0x6D,
0xF3, 0x64, 0x4A, 0xDD, 0x16, 0x81, 0xAF, 0x38,
0xAE, 0x39, 0x17, 0x80, 0x4B, 0xDC, 0xF2, 0x65,
0x49, 0xDE, 0xF0, 0x67, 0xAC, 0x3B, 0x15, 0x82,
0x14, 0x83, 0xAD, 0x3A, 0xF1, 0x66, 0x48, 0xDF,
0x10, 0x87, 0xA9, 0x3E, 0xF5, 0x62, 0x4C, 0xDB,
0x4D, 0xDA, 0xF4, 0x63, 0xA8, 0x3F, 0x11, 0x86,
0xAA, 0x3D, 0x13, 0x84, 0x4F, 0xD8, 0xF6, 0x61,
0xF7, 0x60, 0x4E, 0xD9, 0x12, 0x85, 0xAB, 0x3C
};

static int __adf_iov_putmsg(struct adf_accel_dev *accel_dev,
			    u16 msg_type, u32 msg_data, u8 vf_nr,
			    bool is_notification)
{
	struct adf_accel_pci *pci_info = &accel_dev->accel_pci_dev;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	void __iomem *pmisc_bar_addr =
		pci_info->pci_bars[hw_data->get_misc_bar_id(hw_data)].virt_addr;
	u32 val, local_csr_offset, remote_csr_offset;
	u32 total_delay = 0, mdelay = ADF_IOV_MSG_ACK_DELAY_MS,
		udelay = ADF_IOV_MSG_ACK_DELAY_US;
	struct mutex *lock;	/* lock preventing concurrent acces of CSR */
	int ret = 0;
	struct  pfvf_stats *pfvf_counters = NULL;
	u32 msg;
	int type_shift, data_shift;
	u32 type_mask, data_mask;
	int local_csr_shift = 0, remote_csr_shift = 0;

	if (accel_dev->is_vf) {
		local_csr_offset = hw_data->get_vf2pf_offset(0);
		remote_csr_offset = hw_data->get_pf2vf_offset(0);
		/*
		 * If this a single CSR for both directions
		 * the data from the VF is shifted
		 */
		if (local_csr_offset == remote_csr_offset)
			local_csr_shift = ADF_PFVF_VF_MSG_SHIFT;

		lock = &accel_dev->vf.vf2pf_lock;
		pfvf_counters = &accel_dev->vf.pfvf_counters;
	} else {
		local_csr_offset = hw_data->get_pf2vf_offset(vf_nr);
		remote_csr_offset = hw_data->get_vf2pf_offset(vf_nr);
		/* If this a single CSR for both directions
		 * the data from the VF is shifted
		 */
		if (local_csr_offset == remote_csr_offset)
			remote_csr_shift = ADF_PFVF_VF_MSG_SHIFT;

		lock = &accel_dev->pf.vf_info[vf_nr].pf2vf_lock;
		pfvf_counters = &accel_dev->pf.vf_info[vf_nr].pfvf_counters;
	}
	type_shift = hw_data->pfvf_type_shift;
	type_mask = hw_data->pfvf_type_mask;
	data_shift = hw_data->pfvf_data_shift;
	data_mask = hw_data->pfvf_data_mask;

	if ((msg_type & type_mask) != msg_type) {
		dev_err(&GET_DEV(accel_dev),
			"PF2VF message type 0x%X out of range\n", msg_type);
		return -EINVAL;
	}
	if ((msg_data & data_mask) != msg_data) {
		dev_err(&GET_DEV(accel_dev),
			"PF2VF message data 0x%X out of range\n", msg_data);
		return -EINVAL;
	}
	msg = (msg_type << type_shift) | (msg_data << data_shift);
	msg |= ADF_PFVF_INT | ADF_PFVF_MSGORIGIN_SYSTEM;
	msg <<= local_csr_shift;

	mutex_lock(lock);

	/*
	 * If the device has a single CSR for both directions then
	 * the in-use pattern is used to detect and avoid collisions.
	 */
	if (local_csr_offset == remote_csr_offset) {
		/* Check if PF2VF CSR is in use by remote function */
		val = ADF_CSR_RD(pmisc_bar_addr, local_csr_offset);
		if ((val >> local_csr_shift & ADF_PFVF_IN_USE_MASK) ==
			ADF_PFVF_IN_USE) {
			dev_dbg(&GET_DEV(accel_dev),
				"PF2VF CSR in use by remote function\n");
			ret = -EAGAIN;
			pfvf_counters->busy++;
			goto out;
		}

		/* Set the in-use in the other half of the CSR */
		msg |= ADF_PFVF_IN_USE << remote_csr_shift;
	}
	ADF_CSR_WR(pmisc_bar_addr, local_csr_offset, msg);
	pfvf_counters->tx++;

	/* Wait for confirmation from remote that it received the message */
	do {
		if (udelay < ADF_IOV_MSG_ACK_EXP_MAX_DELAY_US) {
			usleep_range(udelay, udelay * 2);
			udelay = udelay * 2;
			total_delay = total_delay + udelay;
		} else {
			msleep(mdelay);
			total_delay = total_delay + (mdelay * 1000);
		}
		val = ADF_CSR_RD(pmisc_bar_addr, local_csr_offset);
	} while ((val >> local_csr_shift & ADF_PFVF_INT) &&
		 (total_delay < ADF_IOV_MSG_ACK_LIN_MAX_DELAY_US));

	if (val >> local_csr_shift & ADF_PFVF_INT) {
		dev_dbg(&GET_DEV(accel_dev), "ACK not received from remote\n");
		pfvf_counters->no_ack++;
		val &= ~(ADF_PFVF_INT << local_csr_shift);
		ret = -EIO;
	}

	if (local_csr_offset == remote_csr_offset) {
		/*
		 * For fire-and-forget notifications, the remote does not
		 * clear the in-use pattern. This is used to detect collisions.
		 * The message in the CSR should be intact except for the int.
		 */
		msg &= ~(ADF_PFVF_INT << local_csr_shift);
		if (is_notification && val != msg) {
			/* Collision must have overwritten the message */
			dev_err(&GET_DEV(accel_dev),
				"Collision on notification\n");
			pfvf_counters->collision++;
			ret =  -EAGAIN;
			goto out;
		}

		/*
		 * If the far side did not clear the in-use pattern it is either
		 * 1) Notification - message left intact to detect collision
		 * 2) Older protocol (compatibility version < 3) on the far side
		 *    where the sender is responsible for clearing the in-use
		 *    pattern after the receiver has acknowledged receipt.
		 * In either case, clear the in-use pattern now.
		 */
		if ((val >> remote_csr_shift & ADF_PFVF_IN_USE_MASK) ==
				ADF_PFVF_IN_USE) {
			val &= ~(ADF_PFVF_IN_USE_MASK << remote_csr_shift);
			ADF_CSR_WR(pmisc_bar_addr, local_csr_offset, val);
		}
	}
out:
	mutex_unlock(lock);
	return ret;
}

static int adf_iov_put(struct adf_accel_dev *accel_dev,
		       u32 msg_type, u32 msg_data, u8 vf_nr,
		       bool is_notification)
{
	u32 count = 0, delay = ADF_IOV_MSG_RETRY_DELAY;
	int ret;
	struct  pfvf_stats *pfvf_counters = NULL;

	if (accel_dev->is_vf)
		pfvf_counters = &accel_dev->vf.pfvf_counters;
	else
		pfvf_counters = &accel_dev->pf.vf_info[vf_nr].pfvf_counters;

	do {
		ret = __adf_iov_putmsg(accel_dev, msg_type, msg_data, vf_nr,
				       is_notification);
		if (ret == -EAGAIN)
			msleep(delay);
		delay = delay * 2;
	} while (ret == -EAGAIN && ++count < ADF_IOV_MSG_MAX_RETRIES);
	if (ret == -EAGAIN) {
		if (is_notification)
			pfvf_counters->event_timeout++;
		else
			pfvf_counters->tx_timeout++;
	}

	return ret;
}

/**
 * adf_iov_putmsg() - send PF2VF message
 * @accel_dev:  Pointer to acceleration device.
 * @msg_type:	Message type
 * @msg_data:	Message data
 * @vf_nr:	VF number to which the message will be sent
 *
 * Function sends a messge from the PF to a VF
 *
 * Return: 0 on success, error code otherwise.
 */
int adf_iov_putmsg(struct adf_accel_dev *accel_dev, u32 msg_type, u32 msg_data,
		   u8 vf_nr)
{
	return adf_iov_put(accel_dev, msg_type, msg_data, vf_nr, false);
}
EXPORT_SYMBOL_GPL(adf_iov_putmsg);

/**
 * adf_iov_notify() - send PF2VF notification message
 * @accel_dev:  Pointer to acceleration device.
 * @msg_type:	Type of message to send
 * @msg_data:	Data associated with the message
 * @vf_nr:	VF number to which the message will be sent
 *
 * Function sends a notification messge from the PF to a VF
 *
 * Return: 0 on success, error code otherwise.
 */
int adf_iov_notify(struct adf_accel_dev *accel_dev, u32 msg_type, u32 msg_data,
		   u8 vf_nr)
{
	return adf_iov_put(accel_dev, msg_type, msg_data, vf_nr, true);
}
EXPORT_SYMBOL_GPL(adf_iov_notify);

u8 adf_pfvf_crc(u8 start_crc, u8 *buf, u8 len)
{
	u8 crc = start_crc;

	while (len-- > 0)
		crc = pfvf_crc8_table[(crc ^ *buf++) & 0xff];

	return crc;
}

int adf_iov_block_provider_register(u8 msg_type,
				    const adf_iov_block_provider provider)
{
	if (msg_type >= ARRAY_SIZE(pf2vf_message_providers)) {
		pr_err("QAT: invalid message type %d for PF2VF provider\n",
		       msg_type);
		return -EINVAL;
	}
	if (pf2vf_message_providers[msg_type]) {
		pr_err("QAT: Provider %ps already registered for message %d\n",
		       pf2vf_message_providers[msg_type], msg_type);
		return -EINVAL;
	}

	pf2vf_message_providers[msg_type] = provider;
	return 0;
}

u8 adf_iov_is_block_provider_registered(u8 msg_type)
{
	if (pf2vf_message_providers[msg_type])
		return 1;
	else
		return 0;
}

adf_iov_block_provider adf_iov_get_block_provider(u8 msg_type)
{
	return pf2vf_message_providers[msg_type];
}

int adf_iov_block_provider_unregister(u8 msg_type,
				      const adf_iov_block_provider provider)
{
	if (msg_type >= ARRAY_SIZE(pf2vf_message_providers)) {
		pr_err("QAT: invalid message type %d for PF2VF provider\n",
		       msg_type);
		return -EINVAL;
	}
	if (pf2vf_message_providers[msg_type] != provider) {
		pr_err("QAT: Provider %ps not registered for message %d\n",
		       provider, msg_type);
		return -EINVAL;
	}

	pf2vf_message_providers[msg_type] = NULL;
	return 0;
}

static int adf_iov_block_get_data(struct adf_accel_dev *accel_dev, u8 msg_type,
				  u8 byte_num, u8 *data, u8 compatibility,
				  u32 vf_nr, bool crc)
{
	u8 *buffer;
	u8 size;
	u8 msg_ver;
	u8 crc8;

	if (msg_type >= ARRAY_SIZE(pf2vf_message_providers)) {
		pr_err("QAT: invalid message type %d for PF2VF provider\n",
		       msg_type);
		*data = ADF_PF2VF_INVALID_BLOCK_TYPE;
		return -EINVAL;
	}

	if (!pf2vf_message_providers[msg_type]) {
		pr_err("QAT: No registered provider for message %d\n",
		       msg_type);
		*data = ADF_PF2VF_INVALID_BLOCK_TYPE;
		return -EINVAL;
	}

	if ((*pf2vf_message_providers[msg_type])(accel_dev, &buffer, &size,
						 &msg_ver, compatibility,
						 byte_num, vf_nr)) {
		pr_err("QAT: unknown error from provider for message %d\n",
		       msg_type);
		*data = ADF_PF2VF_UNSPECIFIED_ERROR;
		return -EINVAL;
	}

	if ((msg_type <= ADF_VF2PF_MAX_SMALL_MESSAGE_TYPE &&
	     size > ADF_VF2PF_SMALL_PAYLOAD_SIZE) ||
	    (msg_type <= ADF_VF2PF_MAX_MEDIUM_MESSAGE_TYPE &&
	     size > ADF_VF2PF_MEDIUM_PAYLOAD_SIZE) ||
	    size > ADF_VF2PF_LARGE_PAYLOAD_SIZE) {
		pr_err("QAT: Invalid size %d provided for message type %d\n",
		       size, msg_type);
		*data = ADF_PF2VF_PAYLOAD_TRUNCATED;
		return -EINVAL;
	}

	if ((!byte_num && crc) || byte_num >= size + ADF_VF2PF_BLOCK_DATA) {
		pr_err("QAT: Invalid byte number %d for message %d\n",
		       byte_num, msg_type);
		*data = ADF_PF2VF_INVALID_BYTE_NUM_REQ;
		return -EINVAL;
	}

	if (crc) {
		crc8 = adf_pfvf_crc(ADF_CRC8_INIT_VALUE, &msg_ver, 1);
		crc8 = adf_pfvf_crc(crc8, &size, 1);
		*data = adf_pfvf_crc(crc8, buffer, byte_num - 1);
	} else {
		if (byte_num == 0)
			*data = msg_ver;
		else if (byte_num == 1)
			*data = size;
		else
			*data = buffer[byte_num - 2];
	}

	return 0;
}

int adf_pf2vf_block_get_byte(struct adf_accel_dev *accel_dev, u8 msg_type,
			     u8 byte_num, u8 *data, u8 compatibility, u32 vf_nr)
{
	return adf_iov_block_get_data(accel_dev, msg_type, byte_num, data,
				      compatibility, vf_nr, false);
}

int adf_pf2vf_block_get_crc(struct adf_accel_dev *accel_dev, u8 msg_type,
			    u8 byte_num, u8 *data, u8 compatibility, u32 vf_nr)
{
	return adf_iov_block_get_data(accel_dev, msg_type, byte_num, data,
				      compatibility, vf_nr, true);
}

int adf_iov_compatibility_check(struct adf_accel_dev *accel_dev,
				struct adf_accel_compat_manager *cm,
				u8 compat_ver)
{
	int compatible = ADF_PF2VF_VF_COMPATIBLE;
	int i = 0;

	if (!cm) {
		dev_err(&GET_DEV(accel_dev),
			"QAT: compatibility manager not initialized\n");
		return ADF_PF2VF_VF_INCOMPATIBLE;
	}
	for (i = 0; i < cm->num_chker; i++) {
		compatible = cm->iov_compat_checkers[i](accel_dev,
							compat_ver);
		if (compatible == ADF_PF2VF_VF_INCOMPATIBLE) {
			dev_err(&GET_DEV(accel_dev),
				"QAT: PF and VF are incompatible [checker%d]\n",
				i);
			break;
		}
	}
	return compatible;
}

void adf_vf2pf_req_hndl(struct adf_accel_vf_info *vf_info)
{
	struct adf_accel_dev *accel_dev = vf_info->accel_dev;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	int bar_id = hw_data->get_misc_bar_id(hw_data);
	struct adf_bar *pmisc = &GET_BARS(accel_dev)[bar_id];
	void __iomem *pmisc_addr = pmisc->virt_addr;
	u32 msg, vf_nr = vf_info->vf_nr;
	u32 vf_msg, msg_type, msg_data;
	u32 resp_type = 0, resp_data = 0;
	u8 blk_byte_num = 0;
	u8 blk_msg_type = ARRAY_SIZE(pf2vf_message_providers);
	u8 blk_resp_type;
	int res;
	u8 data;
	u8 compat = 0;
	int vf_compat_ver = 0;
	bool is_notification = false;
	unsigned long flags;
	u32 remote_csr_offset = hw_data->get_vf2pf_offset(vf_nr);
	u32 local_csr_offset = hw_data->get_pf2vf_offset(vf_nr);
	int type_shift = hw_data->pfvf_type_shift;
	u32 type_mask = hw_data->pfvf_type_mask;
	int data_shift = hw_data->pfvf_data_shift;
	u32 data_mask = hw_data->pfvf_data_mask;
	u32 ring_num, value = 0;

	/* Read message from the VF */
	msg = ADF_CSR_RD(pmisc_addr, remote_csr_offset);
	if (remote_csr_offset == local_csr_offset)
		vf_msg = msg >> ADF_PFVF_VF_MSG_SHIFT;
	else
		vf_msg = msg;
	msg_type = vf_msg >> type_shift & type_mask;
	msg_data = vf_msg >> data_shift & data_mask;

	if (!(vf_msg & ADF_PFVF_INT)) {
		dev_err(&GET_DEV(accel_dev),
			"Spurious VF2PF interrupt. msg %X. Ignored\n", msg);
		vf_info->pfvf_counters.spurious++;
		goto out;
	}
	vf_info->pfvf_counters.rx++;

	if (!(vf_msg & ADF_PFVF_MSGORIGIN_SYSTEM)) {
		/* Ignore legacy non-system (non-kernel) VF2PF messages */
		dev_dbg(&GET_DEV(accel_dev),
			"Ignored non-system message from VF%d (0x%x);\n",
			vf_nr, msg);
		/*
		 * To ack, clear the VF2PFINT bit.
		 * Because this must be a legacy message, the far side
		 * must clear the in-use pattern.
		 */
		if (remote_csr_offset == local_csr_offset)
			msg &= ~(ADF_PFVF_INT << ADF_PFVF_VF_MSG_SHIFT);
		else
			msg &= ~ADF_PFVF_INT;
		ADF_CSR_WR(pmisc_addr, remote_csr_offset, msg);

		goto out;
	}

	switch (msg_type) {
	case ADF_VF2PF_MSGTYPE_COMPAT_VER_REQ:
		{
		is_notification = false;
		vf_compat_ver = msg_data >> ADF_VF2PF_COMPAT_VER_SHIFT &
			ADF_VF2PF_COMPAT_VER_MASK;
		vf_info->compat_ver = vf_compat_ver;

		resp_type = ADF_PF2VF_MSGTYPE_VERSION_RESP;
		resp_data = ADF_PFVF_COMPATIBILITY_VERSION <<
			ADF_PF2VF_VERSION_RESP_VERS_SHIFT;

		dev_dbg(&GET_DEV(accel_dev),
			"Compatibility Version Request from VF%d vers=%u\n",
			vf_nr, vf_info->compat_ver);

		if (vf_compat_ver > 0 &&
		    vf_compat_ver < ADF_PFVF_COMPATIBILITY_VERSION)
			compat = adf_iov_compatibility_check(accel_dev,
							     accel_dev->cm,
							     vf_compat_ver);
		else if (vf_compat_ver == ADF_PFVF_COMPATIBILITY_VERSION)
			compat = ADF_PF2VF_VF_COMPATIBLE;
		else if (vf_compat_ver == 0)
			compat = ADF_PF2VF_VF_INCOMPATIBLE;
		else
			compat = ADF_PF2VF_VF_COMPAT_UNKNOWN;

		resp_data |= compat << ADF_PF2VF_VERSION_RESP_RESULT_SHIFT;

		if (compat == ADF_PF2VF_VF_INCOMPATIBLE)
			dev_err(&GET_DEV(accel_dev),
				"VF%d and PF are incompatible.\n",
				vf_nr);
		}
		break;
	case ADF_VF2PF_MSGTYPE_VERSION_REQ:
		dev_dbg(&GET_DEV(accel_dev),
			"Legacy VersionRequest received from VF%d 0x%x\n",
			vf_nr, msg);
		is_notification = false;

		/* legacy driver, VF compat_ver is 0 */
		vf_info->compat_ver = 0;

		resp_type = ADF_PF2VF_MSGTYPE_VERSION_RESP;
		/* Set legacy major and minor version num */
		resp_data = 1 << ADF_PF2VF_MAJORVERSION_SHIFT |
			1 << ADF_PF2VF_MINORVERSION_SHIFT;

		/* PF always newer than legacy VF */
		compat = adf_iov_compatibility_check(accel_dev,
						     accel_dev->cm,
						     vf_info->compat_ver);
		resp_data |= compat << ADF_PF2VF_VERSION_RESP_RESULT_SHIFT;

		if (compat == ADF_PF2VF_VF_INCOMPATIBLE)
			dev_err(&GET_DEV(accel_dev),
				"VF%d and PF are incompatible.\n",
				vf_nr);
		break;
	case ADF_VF2PF_MSGTYPE_INIT:
		{
		dev_dbg(&GET_DEV(accel_dev),
			"Init message received from VF%d 0x%x\n",
			vf_nr, msg);
		is_notification = true;
		vf_info->init = true;
		}
		break;
	case ADF_VF2PF_MSGTYPE_CIR_REQ:
	case ADF_VF2PF_MSGTYPE_PIR_REQ:
	{
		ring_num = (vf_nr * hw_data->num_banks_per_vf) + msg_data;
		dev_info(&GET_DEV(accel_dev),
			 "RL info message received from VF%d for ring %d\n",
			 vf_nr, ring_num);

		if (!hw_data->pf_sla_val_provider)
			break;

		hw_data->pf_sla_val_provider(accel_dev,
					     ring_num, &value,
					     msg_type);
		/* 2 bits to store the ring num */
		resp_data = value << ADF_PFVF_RL_MSGDATA_SHIFT;
		resp_data |= ring_num % hw_data->num_banks_per_vf;

		if (msg_type == ADF_VF2PF_MSGTYPE_CIR_REQ)
			resp_type = ADF_PF2VF_MSGTYPE_CIR_RESP;
		else
			resp_type = ADF_PF2VF_MSGTYPE_PIR_RESP;
	}
	break;
	case ADF_VF2PF_MSGTYPE_SHUTDOWN:
		{
		dev_dbg(&GET_DEV(accel_dev),
			"Shutdown message received from VF%d 0x%x\n",
			vf_nr, msg);
		is_notification = true;
		vf_info->init = false;
		}
		break;
	case ADF_VF2PF_MSGTYPE_GET_LARGE_BLOCK_REQ:
	case ADF_VF2PF_MSGTYPE_GET_MEDIUM_BLOCK_REQ:
	case ADF_VF2PF_MSGTYPE_GET_SMALL_BLOCK_REQ:
		{
		is_notification = false;
		switch (msg_type) {
		case ADF_VF2PF_MSGTYPE_GET_LARGE_BLOCK_REQ:
			blk_byte_num = msg_data >>
				ADF_VF2PF_LARGE_BLOCK_BYTE_NUM_SHIFT &
				ADF_VF2PF_LARGE_BLOCK_BYTE_NUM_MASK;
			blk_msg_type = msg_data >>
				ADF_VF2PF_BLOCK_REQ_TYPE_SHIFT &
				ADF_VF2PF_LARGE_BLOCK_REQ_TYPE_MASK;
			blk_msg_type += ADF_VF2PF_MIN_LARGE_MESSAGE_TYPE;
			break;
		case ADF_VF2PF_MSGTYPE_GET_MEDIUM_BLOCK_REQ:
			blk_byte_num = msg_data >>
				ADF_VF2PF_MEDIUM_BLOCK_BYTE_NUM_SHIFT &
				ADF_VF2PF_MEDIUM_BLOCK_BYTE_NUM_MASK;
			blk_msg_type = msg_data >>
				ADF_VF2PF_BLOCK_REQ_TYPE_SHIFT &
				ADF_VF2PF_MEDIUM_BLOCK_REQ_TYPE_MASK;
			blk_msg_type += ADF_VF2PF_MIN_MEDIUM_MESSAGE_TYPE;
			break;
		case ADF_VF2PF_MSGTYPE_GET_SMALL_BLOCK_REQ:
			blk_byte_num = msg_data >>
				ADF_VF2PF_SMALL_BLOCK_BYTE_NUM_SHIFT &
				ADF_VF2PF_SMALL_BLOCK_BYTE_NUM_MASK;
			blk_msg_type = msg_data >>
				ADF_VF2PF_BLOCK_REQ_TYPE_SHIFT &
				ADF_VF2PF_SMALL_BLOCK_REQ_TYPE_MASK;
			blk_msg_type += ADF_VF2PF_MIN_SMALL_MESSAGE_TYPE;
			break;
		}

		if (msg_data >> ADF_VF2PF_BLOCK_REQ_CRC_SHIFT) {
			res = adf_pf2vf_block_get_crc(accel_dev, blk_msg_type,
						      blk_byte_num, &data,
						      vf_info->compat_ver,
						      vf_nr);
			if (res)
				blk_resp_type = ADF_PF2VF_BLOCK_RESP_TYPE_ERROR;
			else
				blk_resp_type = ADF_PF2VF_BLOCK_RESP_TYPE_CRC;
		} else  {
			if (!blk_byte_num)
				vf_info->pfvf_counters.blk_tx++;

			res = adf_pf2vf_block_get_byte(accel_dev, blk_msg_type,
						       blk_byte_num, &data,
						       vf_info->compat_ver,
						       vf_nr);
			if (res)
				blk_resp_type = ADF_PF2VF_BLOCK_RESP_TYPE_ERROR;
			else
				blk_resp_type = ADF_PF2VF_BLOCK_RESP_TYPE_DATA;
		}
		resp_type = ADF_PF2VF_MSGTYPE_BLOCK_RESP;
		resp_data = ((blk_resp_type <<
					ADF_PF2VF_BLOCK_RESP_TYPE_SHIFT) |
			    (data << ADF_PF2VF_BLOCK_RESP_DATA_SHIFT));
		}
		break;
	case ADF_VF2PF_MSGTYPE_RP_RESET:
		{
			is_notification = false;
			dev_dbg(&GET_DEV(accel_dev),
				"Ring pair reset msg received from VF%d 0x%x\n",
				vf_nr, msg);
			resp_type = ADF_PF2VF_MSGTYPE_RP_RESET_RESP;
			resp_data = RPRESET_SUCCESS;
			if (!hw_data->ring_pair_reset) {
				dev_dbg(&GET_DEV(accel_dev),
					"Ring pair reset for VF is not supported\n");
				resp_data = RPRESET_NOT_SUPPORTED;
				break;
			}
			/*Check vf msg_data, convert to PF bank number*/
			if (msg_data >= hw_data->num_banks_per_vf) {
				dev_err(&GET_DEV(accel_dev),
					"Invalid VF bank number from VF:%d\n",
					vf_nr);
				resp_data = RPRESET_INVAL_BANK;
				break;
			}
			msg_data = vf_nr * hw_data->num_banks_per_vf + msg_data;
			if (hw_data->ring_pair_reset(accel_dev, msg_data)) {
				dev_dbg(&GET_DEV(accel_dev),
					"Ring pair reset for VF%d failure\n",
					vf_nr);
				resp_data = RPRESET_TIMEOUT;
				break;
			}
			dev_dbg(&GET_DEV(accel_dev),
				"Ring pair reset for VF%d successfully\n",
				vf_nr);
		}
		break;
	case ADF_VF2PF_MSGTYPE_NOTIFY:
		{
		switch (msg_data) {
		case ADF_VF2PF_MSGGENC_RESTARTING_COMPLETE:
			{
			dev_dbg(&GET_DEV(accel_dev),
				"Restarting Complete received from VF%d 0x%x\n",
				vf_nr, msg);
			is_notification = false;
			vf_info->init = false;
			}
			break;
		default:
			dev_dbg(&GET_DEV(accel_dev),
				"Unknown notify data from VF%d (0x%x);\n",
				vf_nr, msg);
		}
		}
		break;
	default:
		dev_dbg(&GET_DEV(accel_dev), "Unknown message from VF%d (0x%x);\n",
			vf_nr, msg);
	}

	if (remote_csr_offset == local_csr_offset) {
		/* To ack, clear the interrupt bit */
		msg &= ~(ADF_PFVF_INT << ADF_PFVF_VF_MSG_SHIFT);
		/*
		 * Clear the in-use pattern if the sender won't do it.
		 * Because the compatibility version must be the first message
		 * exchanged between the VF and PF, the vf_info->compat_ver
		 * must be set at this time.
		 * The in-use pattern is not cleared for notifications so that
		 * it can be used for collision detection.
		 */
		if (vf_info->compat_ver >= ADF_PFVF_COMPATIBILITY_FAST_ACK &&
		    !is_notification)
			msg &= ~ADF_PFVF_IN_USE_MASK;
	} else {
		/* To ack, clear the interrupt bit */
		msg &= ~ADF_PFVF_INT;
	}
	ADF_CSR_WR(pmisc_addr, remote_csr_offset, msg);

	if (resp_type && adf_iov_putmsg(accel_dev, resp_type, resp_data, vf_nr))
		dev_err(&GET_DEV(accel_dev), "Failed to send response to VF\n");

out:
	/* re-enable interrupt on PF from this VF */
	spin_lock_irqsave(&accel_dev->vf2pf_csr_lock, flags);
	hw_data->enable_vf2pf_interrupts(pmisc_addr,
					 ADF_VF2PF_VFNR_TO_MASK(vf_nr),
					 ADF_VF2PF_VFNR_TO_SET(vf_nr));
	spin_unlock_irqrestore(&accel_dev->vf2pf_csr_lock, flags);
}

void adf_pf2vf_notify_restarting(struct adf_accel_dev *accel_dev)
{
	struct adf_accel_vf_info *vf;
	u32 msg_type = ADF_PF2VF_MSGTYPE_RESTARTING;
	u32 msg_data = 0;
	int i, num_vfs = pci_num_vf(accel_to_pci_dev(accel_dev));

	for (i = 0, vf = accel_dev->pf.vf_info; i < num_vfs; i++, vf++) {
		vf->restarting = true;
		if (vf->init &&
		    adf_iov_notify(accel_dev, msg_type, msg_data, i)) {
			vf->restarting = false;
			dev_err(&GET_DEV(accel_dev),
					"Failed to send restarting msg to VF%d\n",
					i);
		}
	}
}

void adf_vf2pf_wait_for_restarting_complete(struct adf_accel_dev *accel_dev)
{
	struct adf_accel_vf_info *vf;
	int i, times, num_vfs = pci_num_vf(accel_to_pci_dev(accel_dev));
	int vf_init_mask = 0;

	for (times = 0; times < ADF_VF_SHUTDOWN_RETRY; times++) {
		vf_init_mask = 0;
		for (i = 0, vf = accel_dev->pf.vf_info; i < num_vfs;
		     i++, vf++) {
			if (vf->compat_ver < ADF_PFVF_COMPATIBILITY_FALLBACK ||
			    !vf->restarting)
				continue;
			dev_dbg(&GET_DEV(accel_dev), "vf%d, vf->init %d\n",
				i, vf->init);
			vf_init_mask |= vf->init;
		}
		if (!vf_init_mask)
			break;
		msleep(ADF_PF_WAIT_RESTARTING_COMPLETE_DELAY);
	}
	if (vf_init_mask) {
		dev_warn(&GET_DEV(accel_dev),
			 "Some VFs are still running\n");
	}
}

void adf_pf2vf_notify_fatal_error(struct adf_accel_dev *accel_dev)
{
	struct adf_accel_vf_info *vf;
	int i, num_vfs = pci_num_vf(accel_to_pci_dev(accel_dev));
	u32 msg_type = ADF_PF2VF_MSGTYPE_FATAL_ERROR;
	u32 msg_data = 0;

	for (i = 0, vf = accel_dev->pf.vf_info; i < num_vfs; i++, vf++) {
		if (vf->init &&
		    adf_iov_notify(accel_dev, msg_type, msg_data, i))
			dev_err(&GET_DEV(accel_dev),
				"Failed to send fatal error msg to VF%d\n", i);
	}
}

int adf_iov_register_compat_checker(struct adf_accel_dev *accel_dev,
				    struct adf_accel_compat_manager *cm,
				    const adf_iov_compat_checker_t cc)
{
	int num = 0;

	if (!cm) {
		dev_err(&GET_DEV(accel_dev),
			"QAT: compatibility manager not initialized\n");
		return -ENOMEM;
	}

	for (num = 0; num < ADF_COMPAT_CHECKER_MAX; num++) {
		if (cm->iov_compat_checkers[num]) {
			if (cc == cm->iov_compat_checkers[num]) {
				dev_err(&GET_DEV(accel_dev),
					"QAT: already registered\n");
				return -EFAULT;
			}
		} else {
			/* registering the new checker */
			cm->iov_compat_checkers[num] = cc;
			break;
		}
	}

	if (num >= ADF_COMPAT_CHECKER_MAX) {
		dev_err(&GET_DEV(accel_dev),
			"QAT: compatibility checkers are overflow.\n");
		return -EFAULT;
	}

	cm->num_chker++;
	return 0;
}

int adf_iov_unregister_compat_checker(struct adf_accel_dev *accel_dev,
				      struct adf_accel_compat_manager *cm,
				      const adf_iov_compat_checker_t cc)
{
	int num = 0;

	if (!cm) {
		dev_err(&GET_DEV(accel_dev),
			"QAT: compatibility manager not initialized\n");
		return -ENOMEM;
	}
	num = cm->num_chker - 1;

	if (num < 0) {
		dev_err(&GET_DEV(accel_dev),
			"QAT: Array 'iov_compat_checkers' may use index value(s) -1\n");
		return -EFAULT;
	}
	if (cc == cm->iov_compat_checkers[num]) {
		/* unregistering the given checker */
		cm->iov_compat_checkers[num] = NULL;
	} else {
		dev_err(&GET_DEV(accel_dev),
			"QAT: unregistering not in the registered order\n");
		return -EFAULT;
	}

	cm->num_chker--;
	return 0;
}

int adf_iov_init_compat_manager(struct adf_accel_dev *accel_dev,
				struct adf_accel_compat_manager **cm)
{
	if (!(*cm)) {
		*cm = kzalloc(sizeof(**cm), GFP_KERNEL);
		if (!(*cm)) {
			dev_err(&GET_DEV(accel_dev),
				"QAT: compatibility manager allocation fail\n");
			return -ENOMEM;
		}
	} else {
		/* zero the struct */
		memzero_explicit(*cm, sizeof(**cm));
	}

	return 0;
}

int adf_iov_shutdown_compat_manager(struct adf_accel_dev *accel_dev,
				    struct adf_accel_compat_manager **cm)
{
	if (*cm) {
		kfree(*cm);
		*cm = NULL;
	}
	return 0;
}

static int adf_vf2pf_request_version(struct adf_accel_dev *accel_dev)
{
	unsigned long timeout = msecs_to_jiffies(ADF_IOV_MSG_RESP_TIMEOUT);
	u32 msg_type = 0, msg_data = 0;
	int ret = 0;
	int compat = 0;
	int response_received = 0;
	int retry_count = 0;
	struct  pfvf_stats *pfvf_counters = NULL;

	pfvf_counters = &accel_dev->vf.pfvf_counters;

	msg_type = ADF_VF2PF_MSGTYPE_COMPAT_VER_REQ;
	msg_data = ADF_PFVF_COMPATIBILITY_VERSION;
	BUILD_BUG_ON(ADF_PFVF_COMPATIBILITY_VERSION > 255);

	do {
		/* Send request from VF to PF */
		if (retry_count)
			pfvf_counters->retry++;
		if (adf_iov_putmsg(accel_dev, msg_type, msg_data, 0)) {
			dev_err(&GET_DEV(accel_dev),
				"Failed to send Compat Version Request.\n");
			return -EIO;
		}

		/* Wait for response */
		if (!wait_for_completion_timeout
				(&accel_dev->vf.iov_msg_completion, timeout))
			dev_err(&GET_DEV(accel_dev),
				"IOV response message timeout\n");
		else
			response_received = 1;
	} while (!response_received &&
		 ++retry_count < ADF_IOV_MSG_RESP_RETRIES);
	reinit_completion(&accel_dev->vf.iov_msg_completion);

	if (!response_received)
		pfvf_counters->rx_timeout++;
	else
		pfvf_counters->rx_rsp++;
	if (!response_received)
		return -EIO;

	if (accel_dev->vf.compatible == ADF_PF2VF_VF_COMPAT_UNKNOWN)
		/* Response from PF received, check compatibility */
		compat = adf_iov_compatibility_check(accel_dev,
						     accel_dev->cm,
						     accel_dev->vf.pf_version);
	else
		compat = accel_dev->vf.compatible;

	ret = (compat == ADF_PF2VF_VF_COMPATIBLE) ? 0 : -EFAULT;
	if (ret)
		dev_err(&GET_DEV(accel_dev),
			"VF is not compatible with PF, due to the reason %d\n",
			compat);

	return ret;
}

/**
 * adf_enable_vf2pf_comms() - Function enables communication from vf to pf
 *
 * @accel_dev: Pointer to acceleration device virtual function.
 *
 * Return: 0 on success, error code otherwise.
 */
int adf_enable_vf2pf_comms(struct adf_accel_dev *accel_dev)
{
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	int ret = 0;

	/* init workqueue for VF */
	ret = adf_init_vf_wq();
	if (ret)
		return ret;

	hw_data->enable_pf2vf_interrupt(accel_dev);
	adf_iov_init_compat_manager(accel_dev, &accel_dev->cm);
	return adf_vf2pf_request_version(accel_dev);
}
EXPORT_SYMBOL_GPL(adf_enable_vf2pf_comms);

/**
 * adf_disable_vf2pf_comms() - Function disables communication from vf to pf
 *
 * @accel_dev: Pointer to acceleration device virtual function.
 *
 * Return: 0 on success, error code otherwise.
 */
int adf_disable_vf2pf_comms(struct adf_accel_dev *accel_dev)
{
	return adf_iov_shutdown_compat_manager(accel_dev,
					       &accel_dev->cm);
}
EXPORT_SYMBOL_GPL(adf_disable_vf2pf_comms);

/**
 * adf_pf_enable_vf2pf_comms() - Function enables communication from pf
 *
 * @accel_dev: Pointer to acceleration device physical function.
 *
 * Return: 0 on success, error code otherwise.
 */
int adf_pf_enable_vf2pf_comms(struct adf_accel_dev *accel_dev)
{
	return adf_iov_init_compat_manager(accel_dev, &accel_dev->cm);
}
EXPORT_SYMBOL_GPL(adf_pf_enable_vf2pf_comms);

/**
 * adf_pf_disable_vf2pf_comms() - Function disables communication from pf
 *
 * @accel_dev: Pointer to acceleration device physical function.
 *
 * Return: 0 on success, error code otherwise.
 */
int adf_pf_disable_vf2pf_comms(struct adf_accel_dev *accel_dev)
{
	return adf_iov_shutdown_compat_manager(accel_dev, &accel_dev->cm);
}
EXPORT_SYMBOL_GPL(adf_pf_disable_vf2pf_comms);
