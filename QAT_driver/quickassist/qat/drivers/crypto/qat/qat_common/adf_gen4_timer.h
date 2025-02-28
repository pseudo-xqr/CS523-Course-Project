/* SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only) */
/* Copyright(c) 2020 - 2022 Intel Corporation */

#ifndef ADF_GEN4_TIMER_H_
#define ADF_GEN4_TIMER_H_

struct adf_accel_dev;

struct adf_hb_timer_data {
	struct adf_accel_dev *accel_dev;
	struct work_struct hb_int_timer_work;
};

int adf_int_timer_init(struct adf_accel_dev *accel_dev);
void adf_int_timer_exit(struct adf_accel_dev *accel_dev);

#ifdef QAT_HB_FAIL_SIM
int adf_gen4_set_max_hb_timer(struct adf_accel_dev *accel_dev);
#endif
#endif /* ADF_GEN4_TIMER_H_ */
