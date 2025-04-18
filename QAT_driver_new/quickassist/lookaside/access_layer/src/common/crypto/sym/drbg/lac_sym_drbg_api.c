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
 * @file lac_sym_drbg_api.c
 *
 * @ingroup LacSym_Drbg
 *
 * @description
 *     Implementation of the Deterministic Random Bit Generation API
 *
 *****************************************************************************/

/*
*******************************************************************************
* Include public/global header files
*******************************************************************************
*/
#include "cpa.h"
#include "cpa_cy_drbg.h"

/**
 * @ingroup cpaCyDrbg
 */
CpaStatus cpaCyDrbgSessionGetSize(const CpaInstanceHandle instanceHandle_in,
                                  const CpaCyDrbgSessionSetupData *pSetupData,
                                  Cpa32U *pSize)
{
    return CPA_STATUS_UNSUPPORTED;
}

/**
 * @ingroup cpaCyDrbg
 */
CpaStatus cpaCyDrbgInitSession(const CpaInstanceHandle instanceHandle_in,
                               const CpaCyGenFlatBufCbFunc pGenCb,
                               const CpaCyGenericCbFunc pReseedCb,
                               const CpaCyDrbgSessionSetupData *pSetupData,
                               CpaCyDrbgSessionHandle sessionHandle,
                               Cpa32U *pSeedLen)
{
    return CPA_STATUS_UNSUPPORTED;
}

/**
 * @ingroup cpaCyDrbg
 */
CpaStatus cpaCyDrbgGen(const CpaInstanceHandle instanceHandle_in,
                       void *pCallbackTag,
                       CpaCyDrbgGenOpData *pOpData,
                       CpaFlatBuffer *pPseudoRandomBits)
{
    return CPA_STATUS_UNSUPPORTED;
}

/**
 * @ingroup cpaCyDrbg
 */
CpaStatus cpaCyDrbgReseed(const CpaInstanceHandle instanceHandle_in,
                          void *pCallbackTag,
                          CpaCyDrbgReseedOpData *pOpData)
{
    return CPA_STATUS_UNSUPPORTED;
}

/**
 * @ingroup cpaCyDrbg
 */
CpaStatus cpaCyDrbgRemoveSession(const CpaInstanceHandle instanceHandle_in,
                                 CpaCyDrbgSessionHandle sessionHandle)
{
    return CPA_STATUS_UNSUPPORTED;
}

/**
 * @ingroup cpaCyDrbg
 */
CpaStatus cpaCyDrbgQueryStats64(const CpaInstanceHandle instanceHandle_in,
                                CpaCyDrbgStats64 *pStats)
{
    return CPA_STATUS_UNSUPPORTED;
}
