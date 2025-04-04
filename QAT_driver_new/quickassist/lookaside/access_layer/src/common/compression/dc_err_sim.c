/****************************************************************************
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
 * @file dc_err_sim.c
 *
 * @ingroup Dc_DataCompression
 *
 * @description
 *      Implementation of the Data Compression error inject operations.
 *
 *****************************************************************************/

#include "dc_err_sim.h"
#include "lac_log.h"


static Cpa8U num_dc_errors;
static CpaDcReqStatus dc_error;

CpaStatus dcSetNumError(Cpa8U numErrors, CpaDcReqStatus dcError)
{
    if ((dcError < CPA_DC_EMPTY_DYM_BLK) || (dcError >= CPA_DC_OK) ||
        (CPA_DC_INCOMPLETE_FILE_ERR == dcError))
    {
        LAC_LOG_ERROR1("Unsupported ErrorType %d\n", dcError);
        return CPA_STATUS_FAIL;
    }
    num_dc_errors = numErrors;
    dc_error = dcError;

    return CPA_STATUS_SUCCESS;
}

CpaBoolean dcErrorSimEnabled(void)
{
    if (num_dc_errors > 0)
    {
        return CPA_TRUE;
    }
    else
    {
        return CPA_FALSE;
    }
}

CpaDcReqStatus dcGetErrors(void)
{
    CpaDcReqStatus error = 0;

    if (DC_ERROR_SIM == num_dc_errors)
    {
        error = dc_error;
    }
    else if (num_dc_errors > 0 && num_dc_errors <= DC_ERROR_SIM_MAX)
    {
        num_dc_errors--;
        error = dc_error;
    }
    else
    {
        error = 0;
    }
    return error;
}
