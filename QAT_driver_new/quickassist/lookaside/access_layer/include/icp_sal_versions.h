/***************************************************************************
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
 ***************************************************************************/

/**
 ***************************************************************************
 * @file icp_sal_versions.h
 *
 * @defgroup SalVersions
 *
 * @ingroup SalVersions
 *
 * API and structures definition for obtaining software and hardware versions
 *
 ***************************************************************************/

#ifndef _ICP_SAL_VERSIONS_H_
#define _ICP_SAL_VERSIONS_H_

#define ICP_SAL_VERSIONS_FW_VERSION_SIZE 16
/**< Max length of firmware version string */
#define ICP_SAL_VERSIONS_SW_VERSION_SIZE 16
/**< Max length of software version string */
#define ICP_SAL_VERSIONS_MMP_VERSION_SIZE 16
/**< Max length of MMP binary version string */
#define ICP_SAL_VERSIONS_HW_VERSION_SIZE 4
/**< Max length of hardware version string */

/* Part name and number of the accelerator device  */
#define SAL_INFO2_DRIVER_SW_VERSION_MAJ_NUMBER 1
#define SAL_INFO2_DRIVER_SW_VERSION_MIN_NUMBER 2
#define SAL_INFO2_DRIVER_SW_VERSION_PATCH_NUMBER 30

/**
*******************************************************************************
* @ingroup SalVersions
*      Structure holding versions information
*
* @description
*      This structure stores information about versions of software
*      and hardware being run on a particular device.
*****************************************************************************/
typedef struct icp_sal_dev_version_info_s
{
    Cpa32U devId;
    /**< Number of acceleration device for which this structure holds version
     * information */
    Cpa8U firmwareVersion[ICP_SAL_VERSIONS_FW_VERSION_SIZE];
    /**< String identifying the version of the firmware associated with
     * the device. */
    Cpa8U mmpVersion[ICP_SAL_VERSIONS_MMP_VERSION_SIZE];
    /**< String identifying the version of the MMP binary associated with
     * the device. */
    Cpa8U softwareVersion[ICP_SAL_VERSIONS_SW_VERSION_SIZE];
    /**< String identifying the version of the software associated with
     * the device. */
    Cpa8U hardwareVersion[ICP_SAL_VERSIONS_HW_VERSION_SIZE];
    /**< String identifying the version of the hardware (stepping and
     * revision ID) associated with the device. */
} icp_sal_dev_version_info_t;

/**
*******************************************************************************
* @ingroup SalVersions
*      Obtains the version information for a given device
* @description
*      This function obtains hardware and software version information
*      associated with a given device.
*
* @param[in]   accelId     ID of the acceleration device for which version
*                          information is to be obtained.
* @param[out]  pVerInfo    Pointer to a structure that will hold version
*                          information
*
* @context
*      This function might sleep. It cannot be executed in a context that
*      does not permit sleeping.
* @assumptions
*      The system has been started
* @sideEffects
*      None
* @blocking
*      No
* @reentrant
*      No
* @threadSafe
*      Yes
*
* @return CPA_STATUS_SUCCESS       Operation finished successfully
* @return CPA_STATUS_INVALID_PARAM Invalid parameter passed to the function
* @return CPA_STATUS_RESOURCE      System resources problem
* @return CPA_STATUS_FAIL          Operation failed
*
*****************************************************************************/
CpaStatus icp_sal_getDevVersionInfo(Cpa32U accelId,
                                    icp_sal_dev_version_info_t *pVerInfo);

#endif
