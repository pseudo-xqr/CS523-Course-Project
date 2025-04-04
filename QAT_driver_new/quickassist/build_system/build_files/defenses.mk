####################
#  DEFENSES
#
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
####################

#Compile Code with Defenses enabled
#
$(PROG_ACY)_DEFENSES_ENABLED?=n
GCC_VER_49 := 4.9
STACK_PROTECTOR_STRONG_NOT_SUPPORTED := 1
ifneq ($(CC), icc)
STACK_PROTECTOR_STRONG_NOT_SUPPORTED := $(shell expr `$(CC) -dumpversion | cut -f1-2 -d.` \< $(GCC_VER_49))
endif
# Look for debug build in CFLAGS, EXTRA_CFLAGS or ICP_DEBUG
$(PROG_ACY)_DEFENSES_DEBUG = $(findstring -O0,$(CFLAGS))
$(PROG_ACY)_DEFENSES_DEBUG += $(findstring -O0,$(EXTRA_CFLAGS))
$(PROG_ACY)_DEFENSES_DEBUG := $(strip $($(PROG_ACY)_DEFENSES_DEBUG))

ifeq ($($(PROG_ACY)_DEFENSES_ENABLED),y)
ifeq ($($(PROG_ACY)_DEFENSES_DEBUG),)

# Stack execution protection
LIB_SHARED_FLAGS += -z noexecstack
EXE_FLAGS += -Wl,-z,noexecstack

# Data relocation and protection (RELRO)
LIB_SHARED_FLAGS += -z relro -z now
EXE_FLAGS += -Wl,-z,relro -Wl,-z,now

# Stack-based buffer overrun detection
$(PROG_ACY)_DEFENSES_STACK_PROTECTION?=y
ifeq ($($(PROG_ACY)_DEFENSES_STACK_PROTECTION),y)
ifeq "$(STACK_PROTECTOR_STRONG_NOT_SUPPORTED)" "1"
EXTRA_CFLAGS += -fstack-protector
else
EXTRA_CFLAGS += -fstack-protector-strong
endif
endif

# Position Independent Execution
EXE_FLAGS += -pie

ifeq ($($(PROG_ACY)_OS_LEVEL),user_space)
EXTRA_CFLAGS += -fPIC
endif

# Fortify source. -D_FORTIFY_SOURCE=2 needs optimization level 2 (-02)
# to work properly. It does not work with debug builds (-O0)

# Undefine in ubuntu to avoid redefinition error
EXTRA_CFLAGS += -U_FORTIFY_SOURCE

EXTRA_CFLAGS += -O2 -D_FORTIFY_SOURCE=2

# Format string vulnerabilities
EXTRA_CFLAGS += -Wformat -Wformat-security

# Security vulnerabilities
ifneq ($(OS),freebsd)
EXTRA_CFLAGS += -fno-strict-overflow -fno-delete-null-pointer-checks
ifeq ($(NO_UNUSED_CMDLINE_ARG_CFLAGS),-Wno-unused-command-line-argument)
EXTRA_CFLAGS += -Wno-unused-command-line-argument
endif
endif
EXTRA_CFLAGS += -fwrapv

else # -O0 used
$(error $(PROG_ACY)_DEFENSES_ENABLED and -O0 are incompatible)

endif
endif
