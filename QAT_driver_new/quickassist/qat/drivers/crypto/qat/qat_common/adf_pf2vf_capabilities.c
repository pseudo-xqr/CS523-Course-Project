// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
/* Copyright(c) 2014 - 2021 Intel Corporation */
#include <linux/device.h>

#ifdef CONFIG_PCI_IOV
#include "adf_accel_devices.h"
#include "adf_common_drv.h"
#include "adf_pf2vf_msg.h"
#include "adf_cfg.h"
#include "icp_qat_hw.h"

static int adf_pf_capabilities_msg_provider(struct adf_accel_dev *accel_dev,
					    u8 **buffer, u8 *length,
					    u8 *block_version, u8 compatibility,
					    u8 byte_num, u32 vf_nr)
{
	static u8 data[ADF_VF2PF_CAPABILITIES_V3_LENGTH] = {0};
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	u32 ext_dc_caps = hw_data->extended_dc_capabilities;
	u32 capabilities = hw_data->accel_capabilities_mask;
	u32 frequency = hw_data->clock_frequency;
	u16 byte = 0;
	u16 index = 0;

	for (byte = 0; byte < sizeof(ext_dc_caps); byte++) {
		data[byte] =
			(ext_dc_caps >> (byte * ADF_PFVF_DATA_SHIFT))
				& ADF_PFVF_DATA_MASK;
	}

	for (byte = 0, index = ADF_VF2PF_CAPABILITIES_CAP_OFFSET;
	     byte < sizeof(capabilities);
	     byte++, index++) {
		data[index] =
			(capabilities >> (byte * ADF_PFVF_DATA_SHIFT))
			& ADF_PFVF_DATA_MASK;
	}

	if (frequency) {
		for (byte = 0, index = ADF_VF2PF_CAPABILITIES_FREQ_OFFSET;
		     byte < sizeof(frequency);
		     byte++, index++) {
			data[index] =
				(frequency >> (byte * ADF_PFVF_DATA_SHIFT))
				& ADF_PFVF_DATA_MASK;
		}
		*length = ADF_VF2PF_CAPABILITIES_V3_LENGTH;
		*block_version = ADF_VF2PF_CAPABILITIES_V3_VERSION;
	} else {
		*length = ADF_VF2PF_CAPABILITIES_V2_LENGTH;
		*block_version = ADF_VF2PF_CAPABILITIES_V2_VERSION;
	}

	*buffer = data;
	return 0;
}

int adf_pf_vf_capabilities_init(struct adf_accel_dev *accel_dev)
{
	u8 data[ADF_VF2PF_CAPABILITIES_V3_LENGTH] = {0};
	u8 len = ADF_VF2PF_CAPABILITIES_V3_LENGTH;
	u8 version = ADF_VF2PF_CAPABILITIES_V2_VERSION;
	u32 extended_dc_caps = 0;
	u32 capabilities  = 0;
	u32 frequency  = 0;
	u16 byte = 0;
	u16 index = 0;

	if (!accel_dev->is_vf) {
		/* on the pf */
		if (!adf_iov_is_block_provider_registered
		    (ADF_VF2PF_BLOCK_MSG_CAP_SUMMARY))
			adf_iov_block_provider_register
			(ADF_VF2PF_BLOCK_MSG_CAP_SUMMARY,
			adf_pf_capabilities_msg_provider);
	} else if (accel_dev->vf.pf_version >=
			ADF_PFVF_COMPATIBILITY_CAPABILITIES) {
		/* on the vf */
		if (adf_iov_block_get
			(accel_dev, ADF_VF2PF_BLOCK_MSG_CAP_SUMMARY,
			 &version, data, &len)) {
			dev_err(&GET_DEV(accel_dev),
				"QAT: Failed adf_iov_block_get\n");
			return -EFAULT;
		}

		if (len < ADF_VF2PF_CAPABILITIES_V1_LENGTH) {
			dev_err(&GET_DEV(accel_dev),
				"Capabilities message truncated to %d bytes\n",
				len);
			return -EFAULT;
		}

		for (byte = 0;
		     byte < sizeof(extended_dc_caps);
		     byte++) {
			extended_dc_caps |=
				data[byte] << (byte * ADF_PFVF_DATA_SHIFT);
		}
		accel_dev->hw_device->extended_dc_capabilities =
			extended_dc_caps;

		/* Check if extended DC capabilities enabled */
		if (accel_dev->hw_device->extended_dc_capabilities &
			ICP_ACCEL_CAPABILITIES_DC_CHAINING) {
			accel_dev->chaining_enabled = true;
		} else {
			accel_dev->chaining_enabled = false;
		}

		/* Get capabilities if provided by PF */
		if (len >= ADF_VF2PF_CAPABILITIES_V2_LENGTH) {
			for (byte = 0,
			     index = ADF_VF2PF_CAPABILITIES_CAP_OFFSET;
			     byte < sizeof(capabilities);
			     byte++, index++) {
				capabilities |=
				data[index] << (byte * ADF_PFVF_DATA_SHIFT);
			}
			accel_dev->hw_device->accel_capabilities_mask =
				capabilities;
		} else {
			dev_info(&GET_DEV(accel_dev),
				 "PF did not communicate capabilities\n");
		}

		/* Get frequency if provided by the PF */
		if (len >= ADF_VF2PF_CAPABILITIES_V3_LENGTH) {
			for (byte = 0,
			     index = ADF_VF2PF_CAPABILITIES_FREQ_OFFSET;
			     byte < sizeof(frequency);
			     byte++, index++) {
				frequency |=
				    data[index] << (byte * ADF_PFVF_DATA_SHIFT);
			}
			accel_dev->hw_device->clock_frequency = frequency;
		} else {
			dev_info(&GET_DEV(accel_dev),
				 "PF did not communicate frequency\n");
		}

	} else {
		/* The PF is too old to support the extended capabilities */
		accel_dev->hw_device->extended_dc_capabilities = 0;
		accel_dev->chaining_enabled = false;
	}

	return 0;
}
#endif /* CONFIG_PCI_IOV */
