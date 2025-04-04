/***************************************************************************
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
 ***************************************************************************/
/**
 ****************************************************************************
 * @file qae_mem_lib_utils.h
 *
 * This file provides for Linux user space memory allocation. It uses
 * a driver that allocates the memory in kernel memory space (to ensure
 * physically contiguous memory) and maps it to
 * user space for use by the  quick assist sample code
 *
 ***************************************************************************/

#ifndef QAE_MEM_COMMON_H
#define QAE_MEM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#ifndef ICP_WITHOUT_THREAD
#include <pthread.h>
#endif
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include "qae_mem.h"
#include "qae_mem_utils.h"
#include "qae_mem_user_utils.h"
#include "qae_page_table_common.h"
#include "qae_mem_hugepage_utils.h"
#include "qae_mem_utils_common.h"

extern int g_strict_node;
/* Current cached memory size. */
extern size_t g_cache_size;
/* Maximum cached memory size, 8 Mb by default */
extern size_t g_max_cache;

/* User space hash for fast slab searching */
extern slab_list_t g_slab_list[PAGE_SIZE];

extern dev_mem_info_t *__qae_pUserCacheHead;
extern dev_mem_info_t *__qae_pUserCacheTail;
extern dev_mem_info_t *__qae_pUserMemListHead;
extern dev_mem_info_t *__qae_pUserMemListTail;
extern dev_mem_info_t *__qae_pUserLargeMemListHead;
extern dev_mem_info_t *__qae_pUserLargeMemListTail;

extern uint32_t numaAllocations_g;

API_LOCAL
void __qae_ResetControl(void);

API_LOCAL
dev_mem_info_t *__qae_userMemLookupBySize(size_t size,
                                          int node,
                                          void **block,
                                          const size_t align);

API_LOCAL
void __qae_free_slab(const int fd, dev_mem_info_t *slab);

API_LOCAL
dev_mem_info_t *__qae_find_slab(const int fd,
                                const size_t size,
                                const int node,
                                void **addr,
                                const size_t align);
API_LOCAL
int __qae_open(void);

API_LOCAL
void __qae_destroyList(const int fd, dev_mem_info_t *pList);

API_LOCAL
void __qae_reset_cache(const int fd);

API_LOCAL
int __qae_free_special(void);

API_LOCAL
dev_mem_info_t *__qae_alloc_slab(const int fd,
                                 const size_t size,
                                 const uint32_t alignment,
                                 const int node,
                                 enum slabType type);

static inline void add_slab_to_hash(dev_mem_info_t *slab)
{
    const size_t key = get_key(slab->phy_addr);

    ADD_ELEMENT_TO_HEAD_LIST(
        slab, g_slab_list[key].head, g_slab_list[key].tail, _user_hash);
}
static inline void del_slab_from_hash(dev_mem_info_t *slab)
{
    const size_t key = get_key(slab->phy_addr);

    REMOVE_ELEMENT_FROM_LIST(
        slab, g_slab_list[key].head, g_slab_list[key].tail, _user_hash);
}

static inline dev_mem_info_t *find_slab_in_hash(void *virt_addr)
{
    const size_t key = load_key_fptr(&g_page_table, virt_addr);
    dev_mem_info_t *slab = g_slab_list[key].head;

    while (slab)
    {
        uintptr_t offs = (uintptr_t)virt_addr - (uintptr_t)slab->virt_addr;
        if (offs < slab->size)
            return slab;
        slab = slab->pNext_user_hash;
    }

    return NULL;
}

static inline void *init_slab_and_alloc(block_ctrl_t *slab,
                                        const size_t size,
                                        const size_t phys_align_unit)
{
    const size_t last = slab->mem_info.size / CHUNK_SIZE;
    dev_mem_info_t *p_ctrl_blk = &slab->mem_info;
    const size_t reserved = div_round_up(sizeof(block_ctrl_t), UNIT_SIZE);
    void *virt_addr = NULL;

    /* initialise the bitmap to 1 for reserved blocks */
    set_bitmap(slab->bitmap, 0, reserved);
    /* make a barrier to stop search at the end of the bitmap */
    slab->bitmap[last] = QWORD_ALL_ONE;

    virt_addr = __qae_mem_alloc(slab, size, phys_align_unit);
    if (NULL != virt_addr)
    {
        ADD_ELEMENT_TO_HEAD_LIST(
            p_ctrl_blk, __qae_pUserMemListHead, __qae_pUserMemListTail, _user);
    }
    return virt_addr;
}

static inline int push_slab(dev_mem_info_t *slab)
{
    if (g_cache_size + slab->size <= g_max_cache)
    {
        g_cache_size += slab->size;
        ADD_ELEMENT_TO_HEAD_LIST(
            slab, __qae_pUserCacheHead, __qae_pUserCacheTail, _user);
        return 0;
    }
    return -ENOMEM;
}

static inline dev_mem_info_t *pop_slab(const int node)
{
    dev_mem_info_t *slab = NULL;

    for (slab = __qae_pUserCacheHead; slab != NULL; slab = slab->pNext_user)
    {
        if (node != NUMA_ANY_NODE)
            if (g_strict_node && (node != slab->nodeId))
                continue;

        g_cache_size -= slab->size;
        REMOVE_ELEMENT_FROM_LIST(
            slab, __qae_pUserCacheHead, __qae_pUserCacheTail, _user);
        return slab;
    }
    return NULL;
}

#ifndef CACHE_PID
static inline int check_pid(void)
{
    static pid_t pid = 0;

    if (pid != getpid())
    {
        pid = getpid();
        return 1;
    }
    return 0;
}
#endif

#endif /* QAE_MEM_COMMON_H */
