/*******************************************************************************
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 *   redistributing this file, you may do so under either license.
 * 
 *   GPL LICENSE SUMMARY
 * 
 *   Copyright(c) 2007-2023 Intel Corporation. All rights reserved.
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of version 2 of the GNU General Public License as
 *   published by the Free Software Foundation.
 * 
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *   The full GNU General Public License is included in this distribution
 *   in the file called LICENSE.GPL.
 * 
 *   Contact Information:
 *   Intel Corporation
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
 * 
 *  version: QAT20.L.1.2.30-00078
 *
 * @file qae_page_table_uio.h
 *
 * This file provides user-space page tables (similar to Intel x86/x64
 * page tables) for fast virtual to physical address translation. Essentially,
 * this is an implementation of the trie data structure optimized for the x86 HW
 * constraints.
 * Memory required:
 *  - 8 Mb to cover 4 Gb address space.
 * I.e. if only 1 Gb is used it will require additional 2 Mb.
 *
 ******************************************************************************/

#ifndef QAE_PAGE_TABLE_UIO_H
#define QAE_PAGE_TABLE_UIO_H

#include "qae_page_table_defs.h"
#include "qae_page_table_common.h"

static inline void free_page_table_hpg(page_table_t *const table)
{
    /* There are 1+3 levels in 64-bit page table for 2MB hugepages. */
    free_page_level(table, 3);
    /* Reset global root table. */
    memset(table, 0, sizeof(page_table_t));
}

static inline uint64_t load_key_hpg(page_table_t *level, void *virt)
{
    page_index_t id;
    uint64_t phy_addr;

    id.addr = (uintptr_t)virt;

    level = level->next[id.hpg_entry.idxl4].pt;
    if (NULL == level)
        return 0;

    level = level->next[id.hpg_entry.idxl3].pt;
    if (NULL == level)
        return 0;

    level = level->next[id.hpg_entry.idxl2].pt;
    if (NULL == level)
        return 0;

    phy_addr = level->next[id.hpg_entry.idxl1].pa;
    /* the hash key is of 4KB long for both normal page and huge page */
    return phy_addr & ~QAE_PAGE_MASK;
}

#endif /* QAE_PAGE_TABLE_UIO_H */
