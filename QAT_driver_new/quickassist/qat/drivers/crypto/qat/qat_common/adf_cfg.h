/* SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only) */
/* Copyright(c) 2014 - 2021 Intel Corporation */
#ifndef ADF_CFG_H_
#define ADF_CFG_H_

#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/debugfs.h>
#include "adf_accel_devices.h"
#include "adf_cfg_common.h"
#include "adf_cfg_strings.h"

#if defined(QAT_UIO) || defined(QAT_ESXI)
#include "adf_cfg_device.h"
struct adf_cfg_instance;
enum adf_cfg_fw_image_type;
#endif /* QAT_UIO || QAT_ESXI */

struct adf_cfg_key_val {
	char key[ADF_CFG_MAX_KEY_LEN_IN_BYTES];
	char val[ADF_CFG_MAX_VAL_LEN_IN_BYTES];
	enum adf_cfg_val_type type;
	struct list_head list;
};

struct adf_cfg_section {
	char name[ADF_CFG_MAX_SECTION_LEN_IN_BYTES];
#ifdef QAT_UIO
	bool processed;
	bool is_derived;
#endif
	struct list_head list;
	struct list_head param_head;
};

struct adf_cfg_device_data {
#ifdef QAT_UIO
	struct adf_cfg_device *dev;
#endif
	struct list_head sec_list;
	struct dentry *debug;
	struct rw_semaphore lock;
};

struct adf_cfg_depot_list {
	struct list_head sec_list;
};

int adf_cfg_dev_add(struct adf_accel_dev *accel_dev);
void adf_cfg_dev_remove(struct adf_accel_dev *accel_dev);
int adf_cfg_depot_restore_all(struct adf_accel_dev *accel_dev);
int adf_cfg_section_add(struct adf_accel_dev *accel_dev, const char *name);
void adf_cfg_del_all(struct adf_accel_dev *accel_dev);
void adf_cfg_depot_del_all(struct adf_accel_dev *accel_dev);
int adf_cfg_add_key_value_param(struct adf_accel_dev *accel_dev,
				const char *section_name,
				const char *key, const void *val,
				enum adf_cfg_val_type type);
int adf_cfg_get_param_value(struct adf_accel_dev *accel_dev,
			    const char *section, const char *name, char *value);
int adf_cfg_save_section(struct adf_accel_dev *accel_dev,
			 const char *name,
			 struct adf_cfg_section *section);
int adf_cfg_restore_section(struct adf_accel_dev *accel_dev,
			    struct adf_cfg_section *section);
int adf_cfg_depot_save_all(struct adf_accel_dev *accel_dev);
void adf_cfg_depot_init(void);

#ifdef QAT_UIO
struct adf_cfg_section *adf_cfg_sec_find(struct adf_accel_dev *accel_dev,
					 const char *sec_name);
int adf_cfg_derived_section_add(struct adf_accel_dev *accel_dev,
				const char *name);
int adf_cfg_remove_key_param(struct adf_accel_dev *accel_dev,
			     const char *section_name,
			     const char *key);
int adf_cfg_setup_irq(struct adf_accel_dev *accel_dev);
void adf_cfg_set_asym_rings_mask(struct adf_accel_dev *accel_dev);
void adf_cfg_gen_dispatch_arbiter(struct adf_accel_dev *accel_dev,
				  const u32 *thrd_to_arb_map,
				  u32 *thrd_to_arb_map_gen,
				  u32 total_engines);
void adf_cfg_keyval_del_all(struct list_head *head);
int adf_cfg_vf_capability_check(struct adf_accel_dev *accel_dev);
#endif
#if defined(QAT_UIO) || defined(QAT_ESXI)
int adf_cfg_get_fw_image_type(struct adf_accel_dev *accel_dev,
			      enum adf_cfg_fw_image_type *fw_image_type);
#endif /* QAT_UIO || QAT_ESXI */

static inline int adf_cy_inst_cross_banks(struct adf_accel_dev *accel_dev)
{
	if (accel_dev->hw_device->num_rings_per_bank == 2)
		return 1;
	else
		return 0;
}

#endif
