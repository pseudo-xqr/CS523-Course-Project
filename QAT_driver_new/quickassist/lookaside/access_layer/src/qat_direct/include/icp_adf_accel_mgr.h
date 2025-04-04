/*****************************************************************************
 *
 *   BSD LICENSE
 * 
 *   Copyright(c) 2007-2023 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *  version: QAT20.L.1.2.30-00078
 *
 *****************************************************************************/

/*****************************************************************************
 * @file icp_adf_accel_mgr.h
 *
 * @description
 *      This file contains the function prototype for accel
 *      instances management
 *
 *****************************************************************************/
#ifndef ICP_ADF_ACCEL_MGR_H
#define ICP_ADF_ACCEL_MGR_H

/*
 * Device reset mode type.
 * If device reset is triggered from atomic context
 * it needs to be in ICP_ADF_DEV_RESET_ASYNC mode.
 * Otherwise can be either.
 */
typedef enum icp_adf_dev_reset_mode_e
{
    ICP_ADF_DEV_RESET_ASYNC = 0,
    ICP_ADF_DEV_RESET_SYNC
} icp_adf_dev_reset_mode_t;

/*
 * icp_adf_reset_dev
 *
 * Description:
 * Function resets the given device.
 * If device reset is triggered from atomic context
 * it needs to be in ICP_ADF_DEV_RESET_ASYNC mode.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_adf_reset_dev(icp_accel_dev_t *accel_dev,
                            icp_adf_dev_reset_mode_t mode);

/*
 * icp_adf_is_dev_in_error
 * Check if device is in error state.
 *
 * Returns:
 *   CPA_TRUE   device is in error state
 *   CPA_FALSE  device is not in error state
 */
CpaBoolean icp_adf_is_dev_in_error(icp_accel_dev_t *accel_dev);

/*
 * icp_amgr_getNumInstances
 *
 * Description:
 * Returns number of accel instances in the system.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_amgr_getNumInstances(Cpa16U *pNumInstances);

/*
 * icp_amgr_getInstances
 *
 * Description:
 * Returns table of accel instances in the system.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_amgr_getInstances(Cpa16U numInstances,
                                icp_accel_dev_t **pAccel_devs);
/*
 * icp_amgr_getAccelDevByCapabilities
 *
 * Description:
 * Returns a started accel device that implements the capabilities
 * specified in capabilitiesMask.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_amgr_getAccelDevByCapabilities(Cpa32U capabilitiesMask,
                                             icp_accel_dev_t **pAccel_devs,
                                             Cpa16U *pNumInstances);
/*
 * icp_amgr_getAllAccelDevByCapabilities
 *
 * Description:
 * Returns table of accel devices that are started and implement
 * the capabilities specified in capabilitiesMask.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_amgr_getAllAccelDevByCapabilities(Cpa32U capabilitiesMask,
                                                icp_accel_dev_t **pAccel_devs,
                                                Cpa16U *pNumInstances);

/*
 * icp_amgr_getAllAccelDevByEachCapability
 *
 * Description:
 * Returns table of accel devices that are started and implement
 * each of the capability specified in capabilitiesMask.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_amgr_getAllAccelDevByEachCapability(Cpa32U capabilitiesMask,
                                                  icp_accel_dev_t **pAccel_devs,
                                                  Cpa16U *pNumInstances);

/*
 * icp_amgr_getAccelDevCapabilities
 * Returns accel devices capabilities specified in capabilitiesMask.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_amgr_getAccelDevCapabilities(icp_accel_dev_t *accel_dev,
                                           Cpa32U *pCapabilitiesMask);

/*
 * icp_qa_dev_get
 *
 * Description:
 * Function increments the device usage counter.
 *
 * Returns: void
 */
void icp_qa_dev_get(icp_accel_dev_t *pDev);

/*
 * icp_qa_dev_put
 *
 * Description:
 * Function decrements the device usage counter.
 *
 * Returns: void
 */
void icp_qa_dev_put(icp_accel_dev_t *pDev);

/*
 * icp_adf_getAccelDevByAccelId
 *
 * Description:
 * Gets the accel_dev structure based on accelId
 *
 * Returns: a pointer to the accelerator structure or NULL if not found.
 */
icp_accel_dev_t *icp_adf_getAccelDevByAccelId(Cpa32U accelId);

/*
 * icp_adf_GetNumDevices
 *
 * Description:
 * This function is used to determine the number of acceleration devices.
 *
 * Returns:
 *   CPA_STATUS_SUCCESS   on success
 *   CPA_STATUS_FAIL      on failure
 */
CpaStatus icp_adf_get_numDevices(Cpa32U *num_dev);

/*
 * icp_adf_isDeviceAvailable
 *
 * Description:
 * This function is used to check if there is at least one active qat device
 *
 * Returns:
 *   CPA_TRUE     at least one acceleration device is active
 *   CPA_FALSE    zero qat devices found or all qat devices are in-active(down)
 */
CpaBoolean icp_adf_isDeviceAvailable(void);

#endif /* ICP_ADF_ACCEL_MGR_H */
