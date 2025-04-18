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
 * @file lac_sym_compile_check.c
 *
 * @ingroup LacSym
 *
 * This file checks at compile time that some assumptions about the layout
 * of key structures are as expected.
 *
 *
 ***************************************************************************/

#include "cpa.h"

#include "lac_common.h"
#include "icp_accel_devices.h"
#include "icp_adf_debug.h"
#include "lac_sym.h"
#include "cpa_cy_sym_dp.h"

#define COMPILE_TIME_ASSERT(pred)                                              \
    switch (0)                                                                 \
    {                                                                          \
        case 0:                                                                \
        case pred:;                                                            \
    }

void LacSym_CompileTimeAssertions(void)
{
    /* *************************************************************
     * Check sessionCtx is at the same location in bulk cookie and
     * CpaCySymDpOpData.
     * This is required for the callbacks to work as expected -
     * see LacSymCb_ProcessCallback
     * ************************************************************* */

    COMPILE_TIME_ASSERT(offsetof(lac_sym_bulk_cookie_t, sessionCtx) ==
                        offsetof(CpaCySymDpOpData, sessionCtx));
}
