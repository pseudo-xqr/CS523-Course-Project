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
 ******************************************************************************
 * @file  cpa_dc_sample_user.c
 *
 *****************************************************************************/
#include <unistd.h>

#include "cpa_sample_utils.h"
#include "icp_sal_user.h"

#define FILE_NAME_LENGTH 100
char *gFileNameIn = NULL;
char *gFileNameComp = NULL;
char *gFileNameOut = NULL;

extern CpaStatus dcStatefulSample(void);

extern CpaStatus dcResetSessionSample(void);

int gDebugParam = 1;

int main(int argc, const char **argv)
{
    CpaStatus stat = CPA_STATUS_SUCCESS;
    char fileToComp[FILE_NAME_LENGTH] = "paper4";
    char fileComp[FILE_NAME_LENGTH] = "paper4.gz";
    char fileRes[FILE_NAME_LENGTH] = "paper4_2";

    if (argc > 1)
    {
        gDebugParam = atoi(argv[1]);
    }

    PRINT_DBG("Starting Stateful Compression Sample Code App ...\n");

    stat = qaeMemInit();
    if (CPA_STATUS_SUCCESS != stat)
    {
        PRINT_ERR("Failed to initialize memory driver\n");
        return 0;
    }

    stat = icp_sal_userStartMultiProcess("SSL", CPA_FALSE);
    if (CPA_STATUS_SUCCESS != stat)
    {
        PRINT_ERR("Failed to start user process SSL\n");
        qaeMemDestroy();
        return 0;
    }
    gFileNameIn = fileToComp;
    gFileNameComp = fileComp;
    gFileNameOut = fileRes;

    stat = dcStatefulSample();

    if (CPA_STATUS_SUCCESS == stat)
    {
        PRINT_DBG("\nStateful Compression Sample Code App finished\n");
    }
    else if (CPA_STATUS_UNSUPPORTED == stat)
    {
        PRINT_DBG("\nStateful Compression Sample Code App execution skipped - "
                  "unsupported\n");
    }
    else
    {
        PRINT_ERR("\nStateful Compression Sample Code App failed\n");
    }

#ifdef RESET_SESSION_API
    PRINT_DBG("Starting Stateful Compression Reset Session App ...\n");
    stat = dcResetSessionSample();
    if (CPA_STATUS_SUCCESS == stat)
    {
        PRINT_DBG(
            "\nStateful Reset Session Compression Sample Code App finished\n");
    }
    else if (CPA_STATUS_UNSUPPORTED == stat)
    {
        PRINT_DBG("\nStateful Compression Sample Code App execution skipped - "
                  "unsupported\n");
    }
    else
    {
        PRINT_ERR(
            "\nStateful Reset Session Compression Sample Code App failed\n");
    }
#endif
    icp_sal_userStop();

    qaeMemDestroy();
    gFileNameIn = NULL;
    gFileNameComp = NULL;
    gFileNameOut = NULL;

    return 0;
}
