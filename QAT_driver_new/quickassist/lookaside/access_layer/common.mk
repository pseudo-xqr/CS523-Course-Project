###############################################################################
# Makefile variables used by all Look-Aside Crypto builds
#
# @par
#   BSD LICENSE
# 
#   Copyright(c) 2007-2023 Intel Corporation. All rights reserved.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
# 
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
# 
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
#  version: QAT20.L.1.2.30-00078
###############################################################################

# include directories for external APIs

INCLUDES += -I$(API_DIR)/lac \
        -I$(API_DIR)/dc \
        -I$(QAT_FW_API_DIR) \
        -I$(COMMON_FW_API_DIR) \
        -I$(ADF_API_DIR) \
        -I$(ME_ACCESS_LAYER_CMN_API_DIR) \
        -I$(CMN_MEM_PATH) \
        -I$(ICP_ROOT)/quickassist/lookaside/access_layer/src/qat_direct/include \
        -I$(ICP_ROOT)/quickassist/qat/drivers/crypto/qat/qat_common \

# include directories for internal LAC APIs
INCLUDES += -I$(LAC_DIR)/src/common/include \
	-I$(LAC_DIR)/src/common/crypto/sym/include \
	-I$(LAC_DIR)/src/common/crypto/asym/include \
	-I$(LAC_DIR)/src/common/crypto/kpt/include \
	-I$(LAC_DIR)/src/common/qat_ctrl/include \
	-I$(LAC_DIR)/include

EXTRA_CFLAGS += -DLAC_BYTE_ORDER=__LITTLE_ENDIAN -Wformat -Werror

ifeq ($(ICP_OS_LEVEL),kernel_space)
EXTRA_CFLAGS += -DENABLE_SPINLOCK
endif

EXTRA_LDFLAGS +=-whole-archive

ifeq ($(DISABLE_STATS), 1)
EXTRA_CFLAGS += -DDISABLE_STATS
endif

# Disable NUMA allocation thorough OSAL
EXTRA_CFLAGS += -DDISABLE_NUMA_ALLOCATION

#Max number of MR rounds can be decided at compile time to reduce memory footprint
# 0<max_mr<=50 - this is checked at compile time in lac_prime_interface_check.c
ifndef max_mr
EXTRA_CFLAGS += -D MAX_MR_ROUND=50
else
EXTRA_CFLAGS += -D MAX_MR_ROUND=$(max_mr)
endif



MACHINE ?= $(shell uname -m)
ARCH ?= $(shell uname -p)

ifeq ($(ARCH), unknown)
	CPU_LONG_MODE = $(shell cat /proc/cpuinfo | grep -cE " lm ")
	ifeq ($(shell expr $(CPU_LONG_MODE) \> 0), 1)
		ARCH = x86_64
	else
		ARCH = i386
	endif
endif

ifeq ($(ICP_OS_LEVEL), user_space)
	ifeq ($(MACHINE), x86_64)
		ifeq ($(ARCH), i386)
			EXTRA_CFLAGS+=-DICP_KERNEL64_USER32
		endif
	endif
endif

ifeq ($(ARCH), x86_64)
	EXTRA_CFLAGS += -mcx16
endif
