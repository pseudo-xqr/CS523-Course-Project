#########################################################################
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
############################################################################

####################Common variables and definitions########################

PWD=$(shell pwd)



# Ensure The ICP_ENV_DIR environmental var is defined.
ifndef ICP_ENV_DIR
$(error ICP_ENV_DIR is undefined. Please set the path to your environment makefile \
        "-> setenv ICP_ENV_DIR <path>")
endif

#Add your project environment Makefile
include $(ICP_ENV_DIR)/environment.mk
include $(ICP_LAC_DIR)/common.mk

BACKDOOR_API_INCLUDE_DIR=$(LAC_DIR)/include

ICP_OS?=linux
ICP_OS_LEVEL:=user_space
ICP_BUILD_OUTPUT?=$(ICP_ROOT)/build/

CMN_ROOT?=$(ICP_ROOT)/quickassist/utilities/libusdm_drv/
CMN_MODULE_NAME?=libusdm_drv

CFLAGS+= -std=gnu99 -Wall -DDEBUG

EXTRA_LDFLAGS += -L/usr/Lib -L$(ICP_BUILD_OUTPUT)

ADDITIONAL_OBJECTS += $(ICP_BUILD_OUTPUT)/libqat_s.so


#include the makefile with all the default and common Make variable definitions
include $(ICP_BUILDSYSTEM_PATH)/build_files/common.mk

#Add the name for the executable, Library or Module output definitions
OUTPUT_NAME=fips_sample

# List of Source Files to be compiled (to be in a single line or on different lines separated by a "\" and tab.

# List of Source Files to be compiled as a module (to be in a single line or on different lines separated by a "\" and tab.
SOURCES=cpa_fips_sample_utils.c \
        cpa_fips_sample_rsa.c \
        cpa_fips_sample_dsa.c \
        cpa_fips_sample_ecdsa.c \
        cpa_fips_sample_aes_gcm.c

#common includes between all supported OSes
INCLUDES+=-I. -I$(CMN_ROOT) -I$(BACKDOOR_API_INCLUDE_DIR)

include $(ICP_ENV_DIR)/$(ICP_OS)_$(ICP_OS_LEVEL).mk


# On the line directly below list the outputs you wish to build for
install: lib_shared lib_static

###################Include rules makefiles########################
include $(ICP_BUILDSYSTEM_PATH)/build_files/rules.mk
###################End of Rules inclusion#########################

