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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "osal_event.h"
#include "osal_interrupt.h"
#include "osal_lock.h"
#include "osal_sema.h"
#include "osal_thread.h"
#include "osal_trace.h"
#include "osal_irqlock.h"
#include "osal_pci.h"

char *version_string = "os_linux.ko Yajun Fu";

MODULE_AUTHOR("Intel Corporation, (C) 2005-2008 - All Rights Reserved");
MODULE_DESCRIPTION("Operating System Abstraction Layer (OSAL)");
MODULE_SUPPORTED_DEVICE("Intel Media Processors");

MODULE_LICENSE("Dual BSD/GPL");

osal_result osal_irqproxy_init(void);
osal_result osal_irqproxy_exit(void);

//! Insmod parameter to enable/disable debug output, debug - 1 (off)
int osal_debug = 0;
module_param(osal_debug, int, 0);


#if 1
static struct proc_dir_entry *pde;
#endif

#if 1
static int pde_open(struct inode *inode, struct file *file)
{
  return 0;
}

static int pde_release(struct inode *inode, struct file *file)
{
  return 0;
}

static ssize_t pde_read(  struct file *file,
                          char __user *buffer,
                          size_t size,
                          loff_t *offset)
{
  char buf[512];
  int ret;
  int error_count;
  memset(buf, 0, 512);
  ret = sprintf(buf, "osal_debug = %d\n", osal_debug);
  error_count = copy_to_user(buffer, buf, ret);
  if (error_count == 0)
    printk("read successfully\n");
  else {
    printk("failed to read!\n");
    return -EFAULT;
  }
  return 0;
}

static ssize_t pde_write( struct file *file,
                                    const char __user *buffer,
                                    size_t size,
                                    loff_t *offset)
{
  const int BUF_SIZE=10;
  char mybuf[BUF_SIZE + 1];

  if(!buffer) {
    return -EINVAL;
  }

  if(size > BUF_SIZE) {
    size = BUF_SIZE;
  }

  if(copy_from_user(mybuf, buffer, size)) {
    return (-EFAULT);
  }

  osal_debug = simple_strtol(mybuf, NULL, 0);
  return size;
}

static const struct file_operations pde_fops = {
  .open = pde_open,
  .release = pde_release,
  .read = pde_read,
  .write = pde_write,
};
#endif


/* This function is called at module initialization time */
  static int __init osal_init_module(void)
  {
    printk(" ESS OSAL Init \n");

#if 1
    /* only root user can change its value */
    pde = proc_create("osal_debug", 0644, NULL, &pde_fops);
    if(!pde) {
      printk("create proc_entry failed\n");
      return -EIO;
    }
#endif

    osal_irqproxy_init();
    return 0;
  }

/* This function is called at module unload */
  static void __exit osal_cleanup_module(void)
  {
    printk("osal relase\n");
    osal_irqproxy_exit();
#if 1
    if(pde) {
      proc_remove(pde);
    }
#endif
  }

module_init(osal_init_module);
module_exit(osal_cleanup_module);

#if 1
EXPORT_SYMBOL(osal_debug);
#endif
EXPORT_SYMBOL(_os_print);
EXPORT_SYMBOL(_os_debug);
EXPORT_SYMBOL(_os_error);
EXPORT_SYMBOL(os_backtrace);

EXPORT_SYMBOL(os_event_create);
EXPORT_SYMBOL(os_event_destroy);
EXPORT_SYMBOL(os_event_set);
EXPORT_SYMBOL(os_event_reset);
EXPORT_SYMBOL(os_event_wait);
EXPORT_SYMBOL(os_event_hardwait);

EXPORT_SYMBOL(os_sema_init);
EXPORT_SYMBOL(os_sema_destroy);
EXPORT_SYMBOL(os_sema_get);
EXPORT_SYMBOL(os_sema_put);
EXPORT_SYMBOL(os_sema_tryget);

EXPORT_SYMBOL(os_create_lock);
EXPORT_SYMBOL(os_destroy_lock);
EXPORT_SYMBOL(os_lock);
EXPORT_SYMBOL(os_try_lock);
EXPORT_SYMBOL(os_unlock);

EXPORT_SYMBOL(os_thread_create);
EXPORT_SYMBOL(os_thread_wait);
EXPORT_SYMBOL(os_thread_destroy);
EXPORT_SYMBOL(os_thread_yield);
EXPORT_SYMBOL(os_sleep);

EXPORT_SYMBOL(os_irqlock_acquire);
EXPORT_SYMBOL(os_irqlock_release);
EXPORT_SYMBOL(os_irqlock_init);
EXPORT_SYMBOL(os_irqlock_destroy);

EXPORT_SYMBOL(os_register_top_half);
EXPORT_SYMBOL(os_unregister_top_half);

EXPORT_SYMBOL(os_map_io_to_mem_nocache);

EXPORT_SYMBOL(os_pci_find_first_device);
EXPORT_SYMBOL(os_pci_find_next_device);
EXPORT_SYMBOL(os_pci_find_first_device_by_class);
EXPORT_SYMBOL(os_pci_find_next_device_by_class);
EXPORT_SYMBOL(os_pci_device_from_slot);
EXPORT_SYMBOL(os_pci_device_from_address);
EXPORT_SYMBOL(os_pci_get_device_address);
EXPORT_SYMBOL(os_pci_get_interrupt);
EXPORT_SYMBOL(os_pci_get_slot_address);
EXPORT_SYMBOL(os_pci_read_config_8);
EXPORT_SYMBOL(os_pci_read_config_16);
EXPORT_SYMBOL(os_pci_read_config_32);
EXPORT_SYMBOL(os_pci_write_config_8);
EXPORT_SYMBOL(os_pci_write_config_16);
EXPORT_SYMBOL(os_pci_write_config_32);
EXPORT_SYMBOL(os_pci_free_device);
