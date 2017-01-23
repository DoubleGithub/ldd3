

/*==========================================================================
  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.

  Contact Information:
   Intel Corporation

   2200 Mission College Blvd.
   Santa Clara, CA  97052


  BSD LICENSE 

  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions 
  are met:

    * Redistributions of source code must retain the above copyright 
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in 
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products derived 
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 =========================================================================*/
 /*
 * File Description:
 *  This file contains the XFree86 implementations of the OSAL
 *  memory.h abstractions.
 *
 *----------------------------------------------------------------------------
 * Authors:
 *  Alan Previn <alan.previn.teres.alexis@intel.com>
 *
 *----------------------------------------------------------------------------
 */

#ifndef _OSAL_LINUX_USER_IO_MEMMAP_H
#define _OSAL_LINUX_USER_IO_MEMMAP_H


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "osal.h"
os_pci_dev_t *pci_find_device(unsigned int vid, unsigned int did, os_pci_dev_t *pdev);
void * os_map_io_to_mem_cache(
        unsigned long base_address,
        unsigned long size
        )
{
    int fd;
    void *mmio;
    int pg_size = getpagesize();
    int pg_aligned_base = (base_address/pg_size)*pg_size;
    int errsv = 0;
    // pg_aligned_base always <= base_address
    int offset = base_address - pg_aligned_base;
    size += offset;
    fd = open("/dev/mem",O_RDWR);
    if(fd == -1) {
        printf("Could not open /dev/mem.\n");
        return NULL;
    }
    mmio = mmap(NULL,
                size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                pg_aligned_base);
    errsv = errno;
    close(fd);
    if(mmio == MAP_FAILED) {
        printf("Unable to mmap: %s (%d)\n", strerror(errsv), errsv);
        return NULL;
    }
    return (mmio + offset);
}
void * os_map_io_to_mem_nocache(
        unsigned long base_address,
        unsigned long size
        )
{
    int fd;
    void *mmio;
    unsigned pg_size = getpagesize();
    unsigned pg_aligned_base = (base_address/pg_size)*pg_size;
    // pg_aligned_base always <= base_address
    unsigned offset = base_address - pg_aligned_base;
    int errsv = 0;
    size += offset;
    fd = open("/dev/mem",O_RDWR | O_SYNC);
    if(!fd) {
        printf("Could not open /dev/mem.\n");
        return NULL;
    }
    mmio = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pg_aligned_base);
    errsv = errno;
    close(fd);
    if(mmio == MAP_FAILED) {
        printf("Unable to mmap: %s (%d)\n", strerror(errsv), errsv);
        return NULL;
    }
    return (mmio + offset);
}
void os_unmap_io_from_mem(
    void * virt_addr,
    unsigned long size
    )
{
    unsigned long base_address = (unsigned long)virt_addr;
    int pg_size = getpagesize();
    int pg_aligned_base = (base_address/pg_size)*pg_size;
    // pg_aligned_base always <= base_address
    int offset = base_address - pg_aligned_base;
    size += offset;
    munmap((void *)pg_aligned_base, size);
}
#endif // _OSAL_LINUX_USER_IO_MEMMAP_H
