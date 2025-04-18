###############################################################################
# Top-level Look-Aside Crypto makefile, to kick-off a build for all sub-dirs
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

ifndef ICP_ROOT
$(error ICP_ROOT is undefined. Please set the path \
         "-> setenv ICP_ROOT <path to root of Source Code>")
endif

# Ensure The ICP_ENV_DIR environmental var is defined.
ifndef ICP_ENV_DIR
$(error ICP_ENV_DIR is undefined. Please set the path to your environment makefile \
        "-> setenv ICP_ENV_DIR <path>")
endif

ifeq ($(ICP_OS), freebsd)
ifeq ($(ICP_OS_LEVEL), kernel_space)
export MAKE := MAKEFLAGS= make
endif
endif

#Add your project environment Makefile
include $(ICP_ENV_DIR)/environment.mk

##directories
ICP_BUILD_OUTPUT_DIR=build/$(ICP_OS)/$(ICP_OS_LEVEL)/
#the folder where the output will be created.
FINAL_OUTPUT_DIR=$(ICP_BUILD_OUTPUT_DIR)

OUTPUT_NAME?=osalModule
LIB_STATIC=$(OUTPUT_NAME).a

#include your $(ICP_OS)_$(ICP_OS_LEVEL).mk file
include $(ICP_ENV_DIR)/$(ICP_OS)_$(ICP_OS_LEVEL).mk

ifdef ICP_WITHOUT_THREAD
EXTRA_CFLAGS+=-DICP_WITHOUT_THREAD
endif

ifdef USE_OPENSSL
EXTRA_CFLAGS+=-DUSE_OPENSSL
endif

ifeq ($(ICP_OS_LEVEL), kernel_space)
MODULE_SOURCES=$(OS_LEVEL)/$(ICP_OS_LEVEL)/module/OsalSymbols.c
endif

ifndef CROSS_COMPILE
MACHINE?=$(shell uname -m)
else
ifndef MACHINE
$(error MACHINE not defined)
endif
endif

# add the path and list of source libraries,
ADDITIONAL_KERNEL_LIBS=$(OS_LEVEL)/$(ICP_OS_LEVEL)/$(ICP_BUILD_OUTPUT_DIR)/libosal.a
SUBDIRS=$(patsubst %/$(ICP_BUILD_OUTPUT_DIR)/,%,$(dir $(ADDITIONAL_KERNEL_LIBS)))

kernel_module:lib_kernel
	@echo 'Creating OSAL kernel module'; \
	$(MAKE) -C $(KERNEL_SOURCE_ROOT)/ M=$(PWD) 2>&1
	echo "Copying outputs to $(ICP_BUILD_OUTPUT_DIR)";\
	test -d $(ICP_BUILD_OUTPUT_DIR) || mkdir -p $(ICP_BUILD_OUTPUT_DIR);\
	test -f lib.a  &&  mv lib.a $(ICP_BUILD_OUTPUT_DIR)/$(LIB_STATIC);\
	test -f $(OUTPUT_NAME).ko  &&  mv -f $(OUTPUT_NAME).ko $(ICP_BUILD_OUTPUT_DIR);\
	$(RM) -rf *.mod.* .*.cmd;

obj-m := $(OUTPUT_NAME).o
$(OUTPUT_NAME)-objs := $(ADDITIONAL_KERNEL_LIBS) $(QATAL_LIBS)\
		$(patsubst %.c,%.o, $(MODULE_SOURCES))

lib_kernel::ICP_OS_LEVEL=kernel_space
lib_kernel::osal_clean
	@for dir in  $(SUBDIRS); do \
	(echo ; echo $$dir :; cd $$dir; \
		($(MAKE) OS_LEVEL=$(OS_LEVEL) ICP_OS_LEVEL=$(ICP_OS_LEVEL) clean && $(MAKE) OS_LEVEL=$(OS_LEVEL) ICP_OS_LEVEL=$(ICP_OS_LEVEL)) || return 1) \
	done

lib::ICP_OS_LEVEL=user_space
lib::osal_clean
	@for dir in  $(SUBDIRS); do \
	(echo ; echo $$dir :; cd $$dir; \
		$(MAKE) || return 1) \
	done; \
	test -d $(ICP_BUILD_OUTPUT_DIR) || mkdir -p $(ICP_BUILD_OUTPUT_DIR);\
	mv $(OS_LEVEL)/$(ICP_OS_LEVEL)/$(ICP_BUILD_OUTPUT_DIR)/libosal.a $(ICP_BUILD_OUTPUT_DIR)
	mv $(OS_LEVEL)/$(ICP_OS_LEVEL)/$(ICP_BUILD_OUTPUT_DIR)/libosal_s.so $(ICP_BUILD_OUTPUT_DIR)


ifeq ($(ICP_OS_LEVEL),kernel_space)
lib_static: $(LIB_STATIC)
dirs:
	@echo 'Creating output directory' ;\
	test -d $(FINAL_OUTPUT_DIR) || mkdir -p $(FINAL_OUTPUT_DIR);

$(LIB_STATIC): lib_kernel dirs
	@echo 'Creating static library ${LIB_STATIC}'; \
	$(MAKE) -C $(KERNEL_SOURCE_ROOT)/ M=$(PWD) obj-m=""; \
	echo 'Copying outputs';\
	test -f lib.a  &&  mv lib.a $(FINAL_OUTPUT_DIR)/$(LIB_STATIC);\
	$(RM) -f *.o *.mod.* .*.cmd;
endif


ifneq ($(ICP_OS_LEVEL),kernel_space)
include $(ICP_BUILDSYSTEM_PATH)/build_files/rules.mk
endif

include $(ICP_BUILDSYSTEM_PATH)/build_files/Core/$(ICP_CORE).mk
include $(ICP_BUILDSYSTEM_PATH)/build_files/OS/$(ICP_OS).mk
#####################################################################################

osal_clean:
	@echo 'Removing derived objects...'; \
	$(RM) -rf *.o *.a *.mod.* *.ko .*.cmd; \
	$(RM) -rf .tmp_versions; \
	$(RM) -f $(OS_LEVEL)/$(ICP_OS_LEVEL)/OsalAtomicAddSub.o
	$(RM) -rf $(ICP_BUILD_OUTPUT_DIR);
	@for dir in  $(SUBDIRS); do \
		(test -e $$dir) && (echo ; echo $$dir :; cd $$dir; \
		($(MAKE) clean) || return 1) \
	done

