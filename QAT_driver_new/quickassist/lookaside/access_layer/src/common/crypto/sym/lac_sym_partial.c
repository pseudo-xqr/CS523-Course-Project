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
 ***************************************************************************
 * @file lac_sym_partial.c   common partial packet functions
 *
 * @ingroup LacSym
 *
 ***************************************************************************/

/*
*******************************************************************************
* Include public/global header files
*******************************************************************************
*/
#include "cpa.h"

#include "icp_accel_devices.h"
#include "icp_adf_debug.h"

#include "lac_sym.h"
#include "cpa_cy_sym.h"
#include "lac_common.h"

#include "lac_sym_partial.h"


CpaStatus LacSym_PartialPacketStateCheck(CpaCySymPacketType packetType,
                                         CpaCySymPacketType partialState)
{
    CpaStatus status = CPA_STATUS_SUCCESS;

    /* ASSUMPTION - partial requests on a given session must be issued
     * sequentially to guarantee ordering
     * (i.e. issuing partials on concurrent threads for a particular session
     * just wouldn't work)
     */

    /* state is no partial - only a partial is allowed */
    if (((CPA_CY_SYM_PACKET_TYPE_FULL == partialState) &&
         (CPA_CY_SYM_PACKET_TYPE_PARTIAL == packetType)) ||

        /* state is partial - only a partial or final partial is allowed */
        ((CPA_CY_SYM_PACKET_TYPE_PARTIAL == partialState) &&
         ((CPA_CY_SYM_PACKET_TYPE_PARTIAL == packetType) ||
          (CPA_CY_SYM_PACKET_TYPE_LAST_PARTIAL == packetType))))
    {
        status = CPA_STATUS_SUCCESS;
    }
    else /* invalid sequence */
    {
        LAC_INVALID_PARAM_LOG("invalid partial packet sequence");
        status = CPA_STATUS_INVALID_PARAM;
    }

    return status;
}

void LacSym_PartialPacketStateUpdate(CpaCySymPacketType packetType,
                                     CpaCySymPacketType *pPartialState)
{
    /* if previous packet was either a full or ended a partial stream, update
     * state to partial to indicate a new partial stream was created */
    if (CPA_CY_SYM_PACKET_TYPE_FULL == *pPartialState)
    {
        *pPartialState = CPA_CY_SYM_PACKET_TYPE_PARTIAL;
    }
    else
    {
        /* if packet type is final - reset the partial state */
        if (CPA_CY_SYM_PACKET_TYPE_LAST_PARTIAL == packetType)
        {
            *pPartialState = CPA_CY_SYM_PACKET_TYPE_FULL;
        }
    }
}
