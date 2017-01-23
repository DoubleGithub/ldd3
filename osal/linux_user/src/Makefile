#  This file is provided under a dual BSD/GPLv2 license.  When using or
#  redistributing this file, you may do so under either license.
#
#  GPL LICENSE SUMMARY
#
#  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#  The full GNU General Public License is included in this distribution
#  in the file called LICENSE.GPL.
#
#  Contact Information:
#   Intel Corporation
#
#   2200 Mission College Blvd.
#   Santa Clara, CA  97052
#
#
#  BSD LICENSE 
#
#  Copyright(c) 2007-2009 Intel Corporation. All rights reserved.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without 
#  modification, are permitted provided that the following conditions 
#  are met:
#
#    * Redistributions of source code must retain the above copyright 
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright 
#      notice, this list of conditions and the following disclaimer in 
#      the documentation and/or other materials provided with the 
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its 
#      contributors may be used to endorse or promote products derived 
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

BUILD_DEST ?= ../../../

vpath %.c \
	memmap

LIB_NAME = osal

LIB_OBJ = \
	memmap.o \
	osal_irq.o \
	osal_irqlock.o \
	lock.o \
	osal_sema.o \
	osal_pci.o \
	osal_thread.o \
	osal_event.o \
	osal_trace.o \
	osal_clock.o  \
	osal_version.o

LIB_DEPS=pthread

STATIC_LIB=1

CFLAGS= -DLINUX 

CFLAGS_PP = -E -C -P

INCLUDE_PATH += $(BASE_DIR)/osal/include
INCLUDE_PATH += $(BASE_DIR)/osal/linux_user/include
INCLUDE_PATH += $(BASE_DIR)/osal/linux_kernel/tophalf/include

ifeq ($(PRE_SI),true)
    CFLAGS_PP += -DPRE_SILICON
endif

include $(BUILD_DEST)/internal/SMD_Common/CommonRules.mak

TARGET_FILES= \
	lib/libosal.so

./raw/memmap.c:

./memmap.c: ./raw/memmap.c
	if [ -e ./raw/memmap.c ]; \
	then \
		rm -f ./memmap.c; \
		echo Creating: memmap.c from raw/memmap.c; \
		$(CC) $(CFLAGS_PP) ./raw/memmap.c | sed 's/__#/#/' > ./memmap.c; \
	fi;


clean: local_clean

.PHONY: local_clean
local_clean :
	echo TARG $(TARGETS)
	-rm -rf $(CLEAN_FILES)
	-rm -f $(TARGETS)
	$(RECURSE_INTO_SUBDIRS_ELF_KERNEL)
	rm -rf $(OUT)
	if [ -e ./raw/memmap.c ]; \
	then \
		rm -f ./memmap.c; \
	fi;


install: install_dev install_target

install_dev:

install_target:
	msg="FSROOT must be set to a non-empty value for $@" ;	\
	if [ -z "$(FSROOT)" ] ;									\
	then													\
		echo $$msg ;										\
		exit 1 ;											\
	fi ;													\
	for dest in $(TARGET_FILES) ;							\
	do														\
		name=`basename $$dest` ;							\
		dir=`dirname $$dest` ;								\
		[ -d $(FSROOT)/$$dest ] &&						\
			echo "$(FSROOT)/$$dest is a directory and in the way; please remove it." &&	\
			exit 1 ;										\
		if [ ! -e $(FSROOT)/$$dir ] ;						\
		then												\
			mkdir -p $(FSROOT)/$$dir || exit 1 ;			\
		fi ;												\
		cp $(BUILD_DEST)/$$dest $(FSROOT)/$$dest ||			\
			exit 1 ;										\
	done

