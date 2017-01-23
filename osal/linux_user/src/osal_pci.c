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

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "osal_io.h"
#include "osal_memory.h"
#include "osal_pci.h"
#include "osal_type.h"

typedef struct _pci_dev {
    unsigned long slot_address;
    unsigned irq;
} pci_dev_t;

pci_dev_t *pci_find_device(unsigned int vid, unsigned int did, pci_dev_t *pdev);
pci_dev_t *pci_find_device_by_class(unsigned char subclass, unsigned char baseclass, unsigned char pi, pci_dev_t *pci_dev);

#define PCI_BUS(a)  ((a & 0x7FFF0000) >> 16)
#define PCI_DEV(a)  ((a & 0x0000F800) >> 11)
#define PCI_FUNC(a) ((a & 0x00000700) >> 8)

osal_result os_pci_get_interrupt(
        os_pci_dev_t pci_device,
        unsigned *irq)
{
    pci_dev_t *pdev = (pci_dev_t *)pci_device;

    if(!pdev) {
        return OSAL_INVALID_PARAM;
    }

    *irq = pdev->irq;
    return OSAL_SUCCESS;
}

osal_result os_pci_find_first_device(
        unsigned int vendor_id,
        unsigned int device_id,
        os_pci_dev_t* pci_device)
{
    pci_dev_t *pdev;

    pdev = pci_find_device(vendor_id, device_id, NULL);

    if(pdev == NULL)
    {
        *pci_device = NULL;
        return OSAL_NOT_FOUND;
    }

    *pci_device = (os_pci_dev_t*) pdev;
    return OSAL_SUCCESS;
}

osal_result os_pci_find_next_device(
        os_pci_dev_t cur_pci_dev,
        os_pci_dev_t* next_pci_dev)
{
    pci_dev_t *pdev;
    unsigned int did_vid;

    *next_pci_dev = NULL;

    if(cur_pci_dev == NULL) {
        return OSAL_INVALID_PARAM;
    }

    if(OSAL_SUCCESS != os_pci_read_config_32((pci_dev_t*) cur_pci_dev, 0, &did_vid)) {
        return OSAL_ERROR;
    }

    pdev = pci_find_device( (unsigned short) did_vid,
                            (unsigned short)(did_vid >> 16),
                            (pci_dev_t*)cur_pci_dev);

    if(pdev == NULL) {
        return OSAL_NOT_FOUND;
    }

    *next_pci_dev = (os_pci_dev_t*) pdev;
    return OSAL_SUCCESS;
}

osal_result os_pci_find_first_device_by_class(
        unsigned char subclass,
        unsigned char baseclass,
        unsigned char pi,
        os_pci_dev_t* pci_device)
{
    pci_dev_t *pdev;

    pdev = pci_find_device_by_class(subclass, baseclass, pi, NULL);

    if(pdev == NULL) {
        *pci_device = NULL;
        return OSAL_NOT_FOUND;
    }

    *pci_device = (os_pci_dev_t*) pdev;
    return OSAL_SUCCESS;
}

osal_result os_pci_find_next_device_by_class(
        os_pci_dev_t cur_pci_dev,
        os_pci_dev_t* next_pci_dev)
{
    pci_dev_t *     pdev;
    unsigned int    tempData;
    unsigned char   subclass, baseclass, pi;

    *next_pci_dev = NULL;

    if(cur_pci_dev == NULL) {
        return OSAL_INVALID_PARAM;
    }

    if(OSAL_SUCCESS != os_pci_read_config_32((pci_dev_t*) cur_pci_dev, 0x8, &tempData)) {
        return OSAL_ERROR;
    }

    pi        = (tempData & 0xFF00) >> 8;
    subclass  = (tempData & 0xFF0000) >> 16;
    baseclass = (tempData & 0xFF000000) >> 24;

    pdev = pci_find_device_by_class(subclass, baseclass, pi, (pci_dev_t*)cur_pci_dev);

    if(pdev == NULL) {
        return OSAL_NOT_FOUND;
    }

    *next_pci_dev = (os_pci_dev_t*) pdev;
    return OSAL_SUCCESS;
}

#define BUF_SIZE 512

pci_dev_t *pci_find_device(
        unsigned int vid,
        unsigned int did,
        pci_dev_t *pci_dev)
{
    pci_dev_t *     device;
    FILE*           fDevices;
    char            buf[BUF_SIZE];
    unsigned int    nDevAddr, nVenDevId, irqnum;

    if(NULL == (fDevices = fopen("/proc/bus/pci/devices", "r"))) {
        return NULL;
    }

    while(NULL != fgets(buf, BUF_SIZE, fDevices))
    {
        if(3 != sscanf(buf, "%x %x %x", &nDevAddr, &nVenDevId, &irqnum))
        {
            fclose(fDevices);
            return NULL;
        }

        //if looking for the next device, go past current dev
        if((pci_dev != NULL) && ((nDevAddr << 8) <= pci_dev->slot_address)) {
            continue;
        }

        OS_DEBUG("OSAL_PCI cur id: 0x%X, seeking id: 0x%X\n", nVenDevId, (did | (vid<<16)));

        if(nVenDevId == (did | (vid<<16))) {
            OS_DEBUG("Device Found!\n");
            device = (pci_dev_t *) OS_ALLOC(sizeof(pci_dev_t));
            device->slot_address = nDevAddr << 8; //for Windows compatibility
            device->irq = irqnum;
            OS_DEBUG("slot address: 0x%08X\n", nDevAddr << 8);
            fclose(fDevices);
            return(device);
        }
    }

    OS_DEBUG("Device NOT found!\n");
    fclose(fDevices);
    return(NULL);
}

pci_dev_t *pci_find_device_by_class(
        unsigned char subclass,
        unsigned char baseclass,
        unsigned char pi,
        pci_dev_t *pci_dev)
{
    pci_dev_t *     device;
    pci_dev_t       temp_dev;
    FILE*           fDevices;
    char            buf[BUF_SIZE];
    unsigned int    nDevAddr;
    unsigned int    tempData;
    unsigned char   curSubclass, curBaseclass, curPi;

    if(NULL == (fDevices = fopen("/proc/bus/pci/devices", "r"))) {
        return NULL;
    }

    while(NULL != fgets(buf, BUF_SIZE, fDevices)) {
        if(1 != sscanf(buf, "%x", &nDevAddr)) {
            fclose(fDevices);
            return NULL;
        }

        //if looking for the next device, go past current dev
        if((pci_dev != NULL) && ((nDevAddr << 8) <= pci_dev->slot_address)) {
            continue;
        }

        temp_dev.slot_address = nDevAddr << 8; //for Windows compatibility

        if(OSAL_SUCCESS != os_pci_read_config_32(&temp_dev, 0x8, &tempData)) {
            return NULL;
        }

        curPi = (tempData & 0xFF00) >> 8;
        curSubclass = (tempData & 0xFF0000) >> 16;
        curBaseclass = (tempData & 0xFF000000) >> 24;

        OS_DEBUG("OSAL_PCI (BaseClass, SubClass, PI) Current: (0x%X, 0x%X, 0x%X), seeking: (0x%X, 0x%X, 0x%X)\n", curBaseclass, curSubclass, curPi, baseclass, subclass, pi);

        if( (curBaseclass == baseclass)
        &&  (curSubclass == subclass)
        &&  (curPi == pi)) {
            OS_DEBUG("Device Found!\n");
            device = (pci_dev_t *) OS_ALLOC(sizeof(pci_dev_t));
            device->slot_address = nDevAddr << 8; //for Windows compatibility
            OS_DEBUG("slot address: 0x%08X\n", nDevAddr << 8);
            fclose(fDevices);
            return(device);
        }
    }

    OS_DEBUG("Device NOT found!\n");
    fclose(fDevices);
    return(NULL);
}

osal_result os_pci_device_from_slot(os_pci_dev_t *pci_dev, unsigned int slot)
{
    pci_dev_t *device;
    FILE* fDev;
    char szDevAddr[64];

    *pci_dev = NULL;

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                        "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                        (unsigned int)PCI_BUS(slot),
                        (unsigned int)PCI_DEV(slot),
                        (unsigned int)PCI_FUNC(slot))) {
        return OSAL_ERROR;
    }

    if(NULL == (fDev = fopen(szDevAddr, "r"))) {
        return OSAL_NOT_FOUND;
    }

    device = (pci_dev_t*) OS_ALLOC(sizeof(pci_dev_t));

    if(device == NULL) {
        return OSAL_INSUFFICIENT_MEMORY;
    }

    device->slot_address = slot;
    *pci_dev = ((os_pci_dev_t*) device);

    fclose(fDev);

    OS_DEBUG("OSAL_PCI Found Dev: %s\n", szDevAddr);

    return OSAL_SUCCESS;
}

osal_result os_pci_device_from_address(
        os_pci_dev_t* pci_dev,
        unsigned char bus,
        unsigned char dev,
        unsigned char func)
{
    return os_pci_device_from_slot(pci_dev, (bus<<16) | (dev<<11) | (func<<8));
}

osal_result os_pci_get_device_address(
        os_pci_dev_t pci_dev,
        unsigned int *bus,
        unsigned int *dev,
        unsigned int *func)
{
    pci_dev_t *pdev = (pci_dev_t *) pci_dev;

    if(bus) {
        *bus = PCI_BUS(pdev->slot_address);
    }

    if(dev) {
        *dev = PCI_DEV(pdev->slot_address);
    }

    if(func) {
        *func = PCI_FUNC(pdev->slot_address);
    }
    return OSAL_SUCCESS;
}

osal_result os_pci_get_slot_address(os_pci_dev_t pci_dev, unsigned int *slot)
{
    if(pci_dev == NULL) {
        return OSAL_INVALID_HANDLE;
    }

    if(slot == NULL) {
        return OSAL_INVALID_PARAM;
    }

    *slot = ((pci_dev_t*)pci_dev)->slot_address;

    return OSAL_SUCCESS;
}

osal_result os_pci_read_config_8(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned char* val)
{
    unsigned char   hosemonky;
    int             fDev;
    char            szDevAddr[64];

    if(NULL == pci_dev) {
        return OSAL_INVALID_HANDLE;
    }

    if(NULL == val) {
        return OSAL_INVALID_PARAM;
    }

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                (unsigned int)PCI_BUS(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_DEV(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_FUNC(((pci_dev_t*)pci_dev)->slot_address))) {
        return OSAL_ERROR;
    }

    if(-1 == (fDev = open(szDevAddr, O_RDONLY))) {
        return OSAL_NOT_FOUND;
    }

    if(-1 == lseek(fDev, offset, SEEK_SET)) {
        return OSAL_ERROR;
    }

    if(1 != read(fDev, &hosemonky, 1)) {
        return OSAL_ERROR;
    }

    close(fDev);

    OS_DEBUG("OSAL_PCI Read: 0x%X\n", hosemonky);

    *val= hosemonky;
    return OSAL_SUCCESS;
}

osal_result os_pci_read_config_16(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned short* val)
{
    unsigned long   hosemonky;
    int             fDev;
    char            szDevAddr[64];

    if(NULL == pci_dev) {
        return OSAL_INVALID_HANDLE;
    }

    if(NULL == val) {
        return OSAL_INVALID_PARAM;
    }

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                (unsigned int)PCI_BUS(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_DEV(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_FUNC(((pci_dev_t*)pci_dev)->slot_address))) {
        return OSAL_ERROR;
    }

    if(-1 == (fDev = open(szDevAddr, O_RDONLY))) {
        return OSAL_NOT_FOUND;
    }

    if(-1 == lseek(fDev, offset, SEEK_SET)) {
        return OSAL_ERROR;
    }

    if(2 != read(fDev, &hosemonky, 2)) {
        return OSAL_ERROR;
    }

    close(fDev);

    *val= (unsigned short) hosemonky;
    return OSAL_SUCCESS;
}

osal_result os_pci_read_config_32(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned int* val)
{
    unsigned long   hosemonky;
    int             fDev;
    char            szDevAddr[64];

    if(NULL == pci_dev) {
        return OSAL_INVALID_HANDLE;
    }

    if(NULL == val) {
        return OSAL_INVALID_PARAM;
    }

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                (unsigned int)PCI_BUS(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_DEV(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_FUNC(((pci_dev_t*)pci_dev)->slot_address))) {
        return OSAL_ERROR;
    }

    if(-1 == (fDev = open(szDevAddr, O_RDONLY))) {
        return OSAL_NOT_FOUND;
    }

    if(-1 == lseek(fDev, offset, SEEK_SET)) {
        return OSAL_ERROR;
    }

    if(4 != read(fDev, &hosemonky, 4)) {
        return OSAL_ERROR;
    }

    close(fDev);

    OS_DEBUG("OSAL_PCI ReadConfig32 dev: %s, offset 0x%x, data 0x%X\n",
            szDevAddr, offset, hosemonky);
    *val= (unsigned int) hosemonky;
    return OSAL_SUCCESS;
}

osal_result os_pci_write_config_8(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned char val)
{
    int     fDev;
    char    szDevAddr[64];

    if(NULL == pci_dev) {
        return OSAL_INVALID_HANDLE;
    }

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                (unsigned int)PCI_BUS(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_DEV(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_FUNC(((pci_dev_t*)pci_dev)->slot_address))) {
        return OSAL_ERROR;
    }

    if(-1 == (fDev = open(szDevAddr, O_WRONLY))) {
        return OSAL_NOT_FOUND;
    }

    if(-1 == lseek(fDev, offset, SEEK_SET)) {
        return OSAL_ERROR;
    }

    if(1 != write(fDev, &val, 1)) {
        return OSAL_ERROR;
    }

    close(fDev);
    return OSAL_SUCCESS;
}

osal_result os_pci_write_config_16(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned short val)
{
    int     fDev;
    char    szDevAddr[64];

    if(NULL == pci_dev) {
        return OSAL_INVALID_HANDLE;
    }

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                (unsigned int)PCI_BUS(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_DEV(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_FUNC(((pci_dev_t*)pci_dev)->slot_address))) {
        return OSAL_ERROR;
    }

    if(-1 == (fDev = open(szDevAddr, O_WRONLY))) {
        return OSAL_NOT_FOUND;
    }

    if(-1 == lseek(fDev, offset, SEEK_SET)) {
        return OSAL_ERROR;
    }

    if(2 != write(fDev, &val, 2)) {
        return OSAL_ERROR;
    }

    close(fDev);
    return OSAL_SUCCESS;
}

osal_result os_pci_write_config_32(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned int val)
{
    int     fDev;
    char    szDevAddr[64];

    if(NULL == pci_dev) {
        return OSAL_INVALID_HANDLE;
    }

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                (unsigned int)PCI_BUS(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_DEV(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_FUNC(((pci_dev_t*)pci_dev)->slot_address))) {
        return OSAL_ERROR;
    }

    if(-1 == (fDev = open(szDevAddr, O_WRONLY))) {
        return OSAL_NOT_FOUND;
    }

    if(-1 == lseek(fDev, offset, SEEK_SET)) {
        return OSAL_ERROR;
    }

    if(4 != write(fDev, &val, 4)) {
        return OSAL_ERROR;
    }

    close(fDev);
    return OSAL_SUCCESS;
}

osal_result os_pci_read_config_header(
        os_pci_dev_t pci_dev,
        p_os_pci_dev_header_t pci_header)
{
    int     fDev;
    char    szDevAddr[64];

    if(NULL == pci_dev || NULL == pci_header) {
        return OSAL_INVALID_HANDLE;
    }

    if(21 != snprintf(szDevAddr, sizeof(szDevAddr),
                "/proc/bus/pci/%2.2x/%2.2x.%1.1x",
                (unsigned int)PCI_BUS(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_DEV(((pci_dev_t*)pci_dev)->slot_address),
                (unsigned int)PCI_FUNC(((pci_dev_t*)pci_dev)->slot_address))) {
        return OSAL_ERROR;
    }

    if(-1 == (fDev = open(szDevAddr, O_RDONLY))) {
        return OSAL_NOT_FOUND;
    }

    if(256 != read(fDev, pci_header, 256)) {
        return OSAL_ERROR;
    }

    close(fDev);
    return OSAL_SUCCESS;
}

osal_result os_pci_free_device(os_pci_dev_t pci_dev)
{
    if (pci_dev) {
        OS_FREE((pci_dev_t *) pci_dev);
    }
    return OSAL_SUCCESS;
}


/*
#ifdef __X86__

void read_pci_config_x86(unsigned int pci_offset, unsigned long *read_data)
{
    unsigned long tmp;

    __asm {
        mov dx, PCI_CONFIG_ADDR_REG
        mov eax, pci_offset
        out dx, eax
        mov dx, PCI_CONFIG_DATA_REG
        in eax, dx
        mov tmp, eax
    }
    *read_data=tmp;
}

void write_pci_config(unsigned int pci_offset, unsigned long write_data)
{
    __asm {
        mov dx, PCI_CONFIG_ADDR_REG
        mov eax, pci_offset
        out dx, eax
        mov dx, PCI_CONFIG_DATA_REG
        mov eax, write_data
        out dx, eax
    }
}

#endif //__X86__
*/
