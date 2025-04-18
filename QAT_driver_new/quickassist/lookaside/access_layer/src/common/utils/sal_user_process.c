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
 *****************************************************************************
 * @file sal_user_process.c
 *
 * @ingroup SalUserProcess
 *
 * @description
 *    This file contains implementation of functions to set/get user process
 *    name
 *
 *****************************************************************************/

#include "lac_common.h"
#include "icp_adf_user_proxy.h"


static char lacProcessName[LAC_USER_PROCESS_NAME_MAX_LEN] =
    LAC_KERNEL_PROCESS_NAME;

/**< Process name used to obtain values from correct section of config file. */

/*
 * @ingroup LacCommon
 * @description
 *      This function sets the process name
 *
 * @context
 *      This functions is called from module_init or from user space process
 *      initialisation function
 *
 * @assumptions
 *      None
 * @sideEffects
 *      None
 * @reentrant
 *      No
 * @threadSafe
 *      No
 *
 * param[in]  processName    Process name to be set
 */
CpaStatus icpSetProcessName(const char *processName)
{
#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(processName);

    if (strnlen(processName, LAC_USER_PROCESS_NAME_MAX_LEN) ==
        LAC_USER_PROCESS_NAME_MAX_LEN)
    {
        LAC_LOG_ERROR1("Process name too long, maximum process name is %d",
                       LAC_USER_PROCESS_NAME_MAX_LEN - 1);
        return CPA_STATUS_FAIL;
    }
#endif

    snprintf(lacProcessName, LAC_USER_PROCESS_NAME_MAX_LEN, "%s", processName);
    return CPA_STATUS_SUCCESS;
}

/*
 * @ingroup LacCommon
 * @description
 *      This function gets the process name
 *
 * @context
 *      This functions is called from LAC context
 *
 * @assumptions
 *      None
 * @sideEffects
 *      None
 * @reentrant
 *      No
 * @threadSafe
 *      Yes
 *
 */
char *icpGetProcessName(void)
{
    return lacProcessName;
}
