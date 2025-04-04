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

#ifndef DC_CRC32_H_
#define DC_CRC32_H_

#include <cpa_types.h>
#include <cpa_dc.h>
#include "lac_common.h"
#include "dc_session.h"

/**
 * @description
 *     Calculates CRC-32 checksum for given buffer, adhering to RFC 1952 CRC
 *
 * @param[in]  initial_crc    Initial CRC-32 value (used for multi-segment calc)
 * @param[in]  buffer         Pointer to data byte array to calculate CRC on
 * @param[in]  buffer_length  Length of data array
 *
 * @retval Cpa32U             32bit long CRC checksum for given buffer
 */
extern Cpa32U crc32_gzip_refl_by8(Cpa32U initial_crc,
                                  const Cpa8U *buffer,
                                  Cpa64U buffer_length);

/**
 * @description
 *     Helper function to calculate CRC32 checksum on a buffer list.
 *
 *     For a given SGL, the CRC32 checksum is calculated based on the
 *     size of the buffer list.
 *
 * @param[out] checksum       New CRC32 checksum value
 * @param[in]  pBufferList    virtual address of src SGL to calculate CRC on.
 * @param[in]  consumedBytes  total number of bytes inside of pUserSrcBuff
 *                            to calculate CRC checksum for.
 * @param[in]  seedChecksum   Input checksum from where the calculation
 *                            will start from.
 * @note
 *     currently only CRC-32 (0x4C11DB7) algorithm is supported for calculating
 *     CRCs on input and output buffers and CRC value is expected to be
 *     32bit long.
 */
Cpa32U dcCalculateCrc32(CpaBufferList *pBufferList,
                        Cpa32U consumedBytes,
                        const Cpa32U seedChecksum);

#endif /* end of DC_CRC32_H_ */
