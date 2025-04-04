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
 * @file lac_pke_mmp.c
 *
 * @ingroup LacAsymCommonMmp
 *
 * Implementation of mmp related functions
 ******************************************************************************/

/*
********************************************************************************
* Include public/global header files
********************************************************************************
*/
#include "cpa.h"
#include "lac_common.h"

#include "lac_pke_mmp.h"

/*
********************************************************************************
* Include private header files
********************************************************************************
*/

/*
********************************************************************************
* Static Variables
********************************************************************************
*/

/*
********************************************************************************
* Define static function definitions
********************************************************************************
*/

/*
********************************************************************************
* Global Variables
********************************************************************************
*/

/*
********************************************************************************
* Define public/global function definitions
********************************************************************************
*/

Cpa32U LacPke_GetMmpId(Cpa32U sizeInBits,
                       const Cpa32U pSizeIdTable[][LAC_PKE_NUM_COLUMNS],
                       Cpa32U numTableEntries)
{
    Cpa32U id = LAC_PKE_INVALID_FUNC_ID;
    Cpa32U sizeIndex = 0;

    for (sizeIndex = 0; sizeIndex < numTableEntries; sizeIndex++)
    {
        if (pSizeIdTable[sizeIndex][LAC_PKE_SIZE_COLUMN] == sizeInBits)
        {
            id = pSizeIdTable[sizeIndex][LAC_PKE_ID_COLUMN];
            break;
        }
    }

    return id;
}

Cpa32U LacPke_GetIndex_VariableSize(
    Cpa32U sizeInBits,
    const Cpa32U pSizeIdTable[][LAC_PKE_NUM_COLUMNS],
    Cpa32U numTableEntries)
{
    Cpa32U index = LAC_PKE_INVALID_INDEX;
    Cpa32U sizeIndex = 0;

    for (sizeIndex = 0;
         (sizeIndex < numTableEntries) && (LAC_PKE_INVALID_INDEX == index);
         sizeIndex++)
    {
        if (sizeInBits <= pSizeIdTable[sizeIndex][LAC_PKE_SIZE_COLUMN])
        {
            index = sizeIndex;
            break;
        }
    }

    return index;
}
