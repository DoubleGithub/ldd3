/*
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
 */

/*
 *  This file contains Linux PCI API for the OAL abstractions.
 */

#include <osal_pci.h>

#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>

static os_pci_dev_t os_pci_find_device( unsigned int vendor_id,
                                        unsigned int device_id,
                                        os_pci_dev_t pci_dev
                                        )
{
    struct pci_dev *pdev = (struct pci_dev *)pci_dev;

    pdev = pci_get_device(vendor_id, device_id, pdev);
    return (os_pci_dev_t)pdev;
}


osal_result os_pci_get_interrupt(os_pci_dev_t pci_dev, unsigned *irq)
{
    struct pci_dev *pdev = (struct pci_dev *)pci_dev;

    if(!pdev || !irq) {
        return OSAL_INVALID_PARAM;
    }
    *irq = pdev->irq;
    return OSAL_SUCCESS;
}


osal_result os_pci_find_first_device(   unsigned int vendor_id,
                                        unsigned int device_id,
                                        os_pci_dev_t* pci_dev)
{
    *pci_dev = os_pci_find_device(vendor_id, device_id, NULL);
    if(*pci_dev == NULL) {
        return OSAL_NOT_FOUND;
    }
    return OSAL_SUCCESS;
}

osal_result os_pci_find_next_device(    os_pci_dev_t cur_pci_dev,
                                        os_pci_dev_t *next_pci_dev)
{
    struct pci_dev *pdev = (struct pci_dev *)cur_pci_dev;

    if(!pdev) {
        return OSAL_INVALID_PARAM;
    }

    *next_pci_dev = os_pci_find_device(pdev->vendor, pdev->device, cur_pci_dev);
    if(*next_pci_dev == NULL) {
        return OSAL_NOT_FOUND;
    }
    return OSAL_SUCCESS;
}

osal_result os_pci_device_from_slot(os_pci_dev_t *pci_dev, unsigned int slot)
{
    struct pci_bus *pbus;
    unsigned        bus   = slot >> 16;
    unsigned        devfn = (slot >> 8) & 0xff;

    pbus = pci_find_bus(0, bus);
    if(!pbus) {
        return OSAL_NOT_FOUND;
    }

    *pci_dev = pci_get_slot(pbus, devfn);
    if(!*pci_dev) {
        return OSAL_NOT_FOUND;
    }
    return OSAL_SUCCESS;
}


osal_result os_pci_find_first_device_by_class(  unsigned char subclass,
                                                unsigned char baseclass,
                                                unsigned char pi,
                                                os_pci_dev_t* pci_dev)
{
    unsigned class = (baseclass << 16) | (subclass << 8) | pi;

    *pci_dev = pci_get_class(class, NULL);
    if(!*pci_dev) {
        return OSAL_NOT_FOUND;
    }
    return OSAL_SUCCESS;
}


osal_result os_pci_find_next_device_by_class(   os_pci_dev_t cur_pci_dev,
                                                os_pci_dev_t *next_pci_dev)
{
    struct pci_dev *pdev = (struct pci_dev *)cur_pci_dev;

    *next_pci_dev = pci_get_class(pdev->class, cur_pci_dev);
    if(!*next_pci_dev) {
        return OSAL_NOT_FOUND;
    }
    return OSAL_SUCCESS;
}


osal_result os_pci_device_from_address( os_pci_dev_t* pci_dev,
                                        unsigned char bus,
                                        unsigned char dev,
                                        unsigned char func)
{
    unsigned slot = bus << 16;

    slot |= (dev & 0x1F) << 11;
    slot |= (func & 0x7) << 8;
    return os_pci_device_from_slot(pci_dev, slot);
}

osal_result os_pci_get_slot_address(os_pci_dev_t pci_dev, unsigned int *slot)
{
    struct pci_dev *pdev = (struct pci_dev *) pci_dev;

    *slot = (pdev->bus->number << 16) | (pdev->devfn << 8);
    return OSAL_SUCCESS;    
}

osal_result os_pci_get_device_address(  os_pci_dev_t pci_dev,
                                        unsigned int *bus,
                                        unsigned int *dev,
                                        unsigned int *func)
{
    struct pci_dev *pdev = (struct pci_dev *) pci_dev;

    if(bus) {
        *bus = pdev->bus->number;
    }
    if(dev) {
        *dev = PCI_SLOT(pdev->devfn);
    }
    if(func) {
        *func = PCI_FUNC(pdev->devfn);
    }
    return OSAL_SUCCESS;
}

int os_pci_read_config_8(   os_pci_dev_t pci_dev,
                            unsigned int offset,
                            unsigned char* val
                            )
{
    return pci_read_config_byte((struct pci_dev*) pci_dev, offset, val);
}

int os_pci_read_config_16( os_pci_dev_t pci_dev,
                            unsigned int offset,
                            unsigned short* val
                            )
{
    return pci_read_config_word((struct pci_dev*) pci_dev, offset, val);
}

int os_pci_read_config_32(  os_pci_dev_t pci_dev,
                            unsigned int offset,
                            unsigned int* val
                            )
{
    return pci_read_config_dword((struct pci_dev*) pci_dev, offset, val);
}

int os_pci_write_config_8(  os_pci_dev_t pci_dev,
                            unsigned int offset,
                            unsigned char val
                            )
{
    return pci_write_config_byte((struct pci_dev*) pci_dev, offset, val);
}

int os_pci_write_config_16( os_pci_dev_t pci_dev,
                            unsigned int offset,
                            unsigned short val
                            )
{
    return pci_write_config_word((struct pci_dev*) pci_dev, offset, val);
}

int os_pci_write_config_32( os_pci_dev_t pci_dev,
                            unsigned int offset,
                            unsigned int val
                            )
{
    return pci_write_config_dword((struct pci_dev*) pci_dev, offset, val);
}

osal_result os_pci_free_device( os_pci_dev_t pci_dev )
{
    return OSAL_SUCCESS;
}
