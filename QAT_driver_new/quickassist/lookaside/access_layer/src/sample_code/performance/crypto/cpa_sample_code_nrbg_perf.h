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
 * @file cpa_sample_code_nrbg_perf.h
 *
 * @defgroup sampleNrbgFunctional
 *
 * @ingroup sampleCode
 *
 * @description
 *     Non-Deterministic Random Bit Generation Performance Sample Code
 *functions.
 *
 ***************************************************************************/
#ifndef CPA_SAMPLE_CODE_NRBG_PERF_H
#define CPA_SAMPLE_CODE_NRBG_PERF_H
#include "cpa_sample_code_crypto_utils.h"
#include "cpa_cy_nrbg.h"
#include "icp_sal_nrbg_ht.h"
#include "cpa.h"

/*************************************************************************
 * @ingroup sampleNrbgFunctional
 *
 * @description
 *    This function starts the crypto instance and registers NRBG
 *    functions.
 *
 * @param[in] syncMode					Sync Mode of the test: async and sync
 * @param[in] nLenInBytes				Data length of Non-Deterministic Random
 Bit
 *                                      Generation, unit is byte, must be more
 than 0.
 * @param[in] numBuffers				The number of buffers List,a minimum
 * @param[in] numLoops					The number of Loops
 * @context
 *      This functions is called from the user process context
 *
 * @assumptions
 *      None
 * @sideEffects
 *      None
 * @reentrant
 *      No
 * @threadSafe
 *      Yes

 * @retval CPA_STATUS_SUCCESS       Function executed successfully.
 * @retval CPA_STATUS_FAIL          Function failed.
 *
 *************************************************************************/
CpaStatus setupNrbgTest(Cpa32U nLenInBytes,
                        sync_mode_t syncMode,
                        Cpa32U numBuffers,
                        Cpa32U numLoops);

#endif
