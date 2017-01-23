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

#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include "osal.h"

typedef struct {
    int                     irqnum;
    os_interrupt_handler_t *irqfunc;
    void *                  data;
} os_irq_t;

typedef struct {
    int             irqnum;
    os_event_t      event;
    os_interrupt_t  irqhandle;
    int             disabled;
} os_irqproxy_t;

static struct proc_dir_entry *procfile;

static irqreturn_t os_irq_wrapper(int irqnum, void *data)
{
    os_irq_t *irq = data;
    irq->irqfunc(irq->data);
    return IRQ_HANDLED;
}

osal_result os_register_top_half(   int irqnum,
                                    os_interrupt_t *irqhandle,
                                    os_interrupt_handler_t *irqhandler,
                                    void *data,
                                    const char *name)
{
    os_irq_t *irq;

    if(!irqhandle || !irqhandler || !name) {
        return OSAL_INVALID_PARAM;
    }

    irq = OS_ALLOC(sizeof(os_irq_t));
    if(!irq) {
        return OSAL_ERROR;
    }

    irq->irqnum = irqnum;
    irq->irqfunc = irqhandler;
    irq->data = data;

    if(request_irq(irqnum, os_irq_wrapper, IRQF_SHARED, name, irq)){
        OS_FREE(irq);
        return OSAL_ERROR;
    }
    *irqhandle = (void *)irq;
    return OSAL_SUCCESS;
}

osal_result os_unregister_top_half(os_interrupt_t *irqhandle)
{
    os_irq_t *irq = *irqhandle;

    free_irq(irq->irqnum, irq);
    OS_FREE(irq);
    *irqhandle = NULL;
    return OSAL_SUCCESS;
}

void osal_irqproxy_handler(void *data)
{
    os_irqproxy_t *irqproxy = data;
    os_event_set(&irqproxy->event);
    disable_irq_nosync(irqproxy->irqnum);
}

static int osal_irqproxy_open(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static ssize_t osal_irqproxy_read(  struct file *file,
                                    char __user *buffer,
                                    size_t size,
                                    loff_t *offset)
{
    os_irqproxy_t *irqproxy = file->private_data;

    if(!irqproxy) {
        return 0;
    }
    if(irqproxy->disabled){
        irqproxy->disabled = 0;
        enable_irq(irqproxy->irqnum);
    }
    if(os_event_wait(&irqproxy->event, EVENT_NO_TIMEOUT) != OSAL_SUCCESS){
        return -EINTR;
    }
    irqproxy->disabled = 1;
    return 0;
}

static ssize_t osal_irqproxy_write( struct file *file,
                                    const char __user *buffer,
                                    size_t size,
                                    loff_t *offset)
{
    os_irqproxy_t *irqproxy;
    unsigned char irqnum;

    // Check that we haven't been called before
    // Only a write of one byte is valid
    if(file->private_data || size != 1) {
        return -EINVAL;
    }

    //get the irq number
    if(copy_from_user(&irqnum, buffer, 1)) {
        return -EFAULT;
    }

    // zero is not valid
    if(!irqnum) {
        return -EINVAL;
    }

    irqproxy = OS_ALLOC(sizeof(os_irqproxy_t));
    if(!irqproxy) {
        return -ENOMEM;
    }

    irqproxy->irqnum = irqnum;
    irqproxy->disabled = 0;
    os_event_create(&irqproxy->event, 0);

    if(os_register_top_half(irqnum, &irqproxy->irqhandle, osal_irqproxy_handler, irqproxy, "irqproxy") != OSAL_SUCCESS){
        os_event_destroy(&irqproxy->event);
        OS_FREE(irqproxy);
        return -EINVAL;
    }

    file->private_data = irqproxy;
    return 0;
}

static int osal_irqproxy_release(struct inode *inode, struct file *file)
{
    os_irqproxy_t *irqproxy = file->private_data;

    if(file->private_data){
        os_unregister_top_half(&irqproxy->irqhandle);
        if(irqproxy->disabled || os_event_wait(&irqproxy->event, 0) == OSAL_SUCCESS){
            OS_INFO("cleaning up irq %d proxy while an IRQ was pending!\n",
                    irqproxy->irqnum);
            enable_irq(irqproxy->irqnum);
        }
        os_event_destroy(&irqproxy->event);
        OS_FREE(file->private_data);
    }
    return 0;
}


static struct file_operations irqproxy_fops = {
    .open    = osal_irqproxy_open,
    .read    = osal_irqproxy_read,
    .write   = osal_irqproxy_write,
    .release = osal_irqproxy_release,
};

osal_result osal_irqproxy_init(void)
{

  procfile = proc_create("irqproxy", S_IRUSR | S_IWUSR, NULL, &irqproxy_fops);
    if(!procfile){
        OS_INFO("Cannot create osal irqproxy access file!\n");
        return OSAL_ERROR;
    }

    return OSAL_SUCCESS;
}

osal_result osal_irqproxy_exit(void)
{
    if(procfile){
        proc_remove(procfile);
        procfile = NULL;
    }
    return OSAL_SUCCESS;
}
