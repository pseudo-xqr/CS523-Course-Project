/******************************************************************************
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

/**
 *****************************************************************************
 * @file lac_mem.c  Implementation of Memory Functions
 *
 * @ingroup LacMem
 *
 *****************************************************************************/

/*
*******************************************************************************
* Include header files
*******************************************************************************
*/
#include "Osal.h"
#include "cpa.h"

#include "icp_accel_devices.h"
#include "icp_adf_init.h"
#include "icp_adf_transport.h"
#include "icp_adf_debug.h"

#include "lac_mem.h"
#include "lac_mem_pools.h"
#include "lac_common.h"
#include "lac_pke_utils.h"
#include "lac_log.h"
#include "lac_sym.h"
#include "lac_list.h"
#include "lac_sym_qat.h"
#include "icp_qat_fw_la.h"
#include "lac_sal_types_crypto.h"


/*
********************************************************************************
* Static Variables
********************************************************************************
*/

#define MAX_BUFFER_SIZE (LAC_BITS_TO_BYTES(4096))
/**< @ingroup LacMem
 * Maximum size of the buffers used in the resize function */

/*
*******************************************************************************
* Define public/global function definitions
*******************************************************************************
*/
/**
 * @ingroup LacMem
 */
Cpa8U *icp_LacBufferResize(CpaInstanceHandle instanceHandle,
                           Cpa8U *pUserBuffer,
                           Cpa32U userLen,
                           Cpa32U workingLen,
                           CpaBoolean *pInternalMemory)
{
    Cpa8U *pWorkingBuffer = NULL;
    Cpa32U padSize = 0;
    sal_crypto_service_t *pCryptoService =
        (sal_crypto_service_t *)instanceHandle;

    if ((userLen > 0) && (NULL == pUserBuffer))
    {
        LAC_LOG_ERROR("pUserBuffer parameter is NULL");
        return NULL;
    }

    /* shouldn't trim the user buffer */
    if (workingLen < userLen)
    {
        LAC_LOG_ERROR2(
            "Cannot trim input buffer from %u to %u", userLen, workingLen);
        return NULL;
    }

    padSize = workingLen - userLen;

    /* check size */
    if (padSize > 0)
    {
        do
        {
            pWorkingBuffer = (Cpa8U *)Lac_MemPoolEntryAlloc(
                pCryptoService->lac_pke_align_pool);
            if (NULL == pWorkingBuffer)
            {
                LAC_LOG_ERROR("Failed to allocate pWorkingBuffer");
                return NULL;
            }
            else if ((void *)CPA_STATUS_RETRY == pWorkingBuffer)
            {
                osalYield();
            }
        } while ((void *)CPA_STATUS_RETRY == pWorkingBuffer);

        /* Zero MSB of buffer */
        LAC_OS_BZERO(pWorkingBuffer, padSize);

        /* Copy from user buffer to internal buffer */
        if (userLen)
        {
            memcpy(pWorkingBuffer + padSize, pUserBuffer, userLen);
        }

        /* Indicate that internally allocated memory is being sent to QAT */
        *pInternalMemory = CPA_TRUE;

        return pWorkingBuffer;
    } /* if (padSize > 0) ... */

    return pUserBuffer;
}

/**
 * @ingroup LacMem
 */
CpaStatus icp_LacBufferRestore(Cpa8U *pUserBuffer,
                               Cpa32U userLen,
                               Cpa8U *pWorkingBuffer,
                               Cpa32U workingLen,
                               CpaBoolean copyBuf)
{
    Cpa32U padSize = 0;

    /* NULL is a valid value for working buffer as this function may be
     * called to clean up in an error case where all the resize operations
     * were not completed */
    if (NULL == pWorkingBuffer)
    {
        return CPA_STATUS_SUCCESS;
    }

    if (workingLen < userLen)
    {
        LAC_LOG_ERROR("Invalid buffer sizes");
        return CPA_STATUS_INVALID_PARAM;
    }

    if (pUserBuffer != pWorkingBuffer)
    {

        if (CPA_TRUE == copyBuf)
        {
            /* Copy from internal buffer to user buffer */
            padSize = workingLen - userLen;
            memcpy(pUserBuffer, pWorkingBuffer + padSize, userLen);
        }

        Lac_MemPoolEntryFree(pWorkingBuffer);
    }
    return CPA_STATUS_SUCCESS;
}

/**
 * @ingroup LacMem
 */
CpaPhysicalAddr SalMem_virt2PhysExternal(void *pVirtAddr, void *pServiceGen)
{
    sal_service_t *pService = (sal_service_t *)pServiceGen;

    if (pService->svmEnabled)
        return (CpaPhysicalAddr)pVirtAddr;

    if (NULL != pService->virt2PhysClient)
    {
        return pService->virt2PhysClient(pVirtAddr);
    }
    else
    {
        /* Use internal OSAL virt to phys */
        /* Ok for kernel space probably should not use for user */
        return LAC_OS_VIRT_TO_PHYS_INTERNAL(pServiceGen, pVirtAddr);
    }
}

#ifdef USER_SPACE
/**
 * @ingroup LacMem
 */
CpaPhysicalAddr SalMem_virt2PhysInternal(void *pVirtAddr, void *pServiceGen)
{
    sal_service_t *pService = (sal_service_t *)pServiceGen;

    if (!pService || !pService->svmEnabled)
        return (CpaPhysicalAddr)qaeVirtToPhysNUMA(pVirtAddr);

    return (CpaPhysicalAddr)pVirtAddr;
}
#endif

size_t icp_sal_iommu_get_remap_size(size_t size)
{
    return osalIOMMUgetRemappingSize(size);
}

CpaStatus icp_sal_iommu_map(Cpa64U phaddr, Cpa64U iova, size_t size)
{
    return osalIOMMUMap((UINT64)phaddr, (UINT64)iova, size) == 0
               ? CPA_STATUS_SUCCESS
               : CPA_STATUS_FAIL;
}

CpaStatus icp_sal_iommu_unmap(Cpa64U iova, size_t size)
{
    return osalIOMMUUnmap((UINT64)iova, size) == 0 ? CPA_STATUS_SUCCESS
                                                   : CPA_STATUS_FAIL;
}
