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
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "osal.h"


typedef struct {
    unsigned char           irqnum;
    int                     fd;
    pthread_t               thread;
    os_interrupt_handler_t *irqfunc;
    void *                  data;
} os_irquser_t;


static void *os_irq_thread(void * data)
{
    os_irquser_t *irq = data;
    char tmp;

    while(read(irq->fd, &tmp, 1) >= 0){
        irq->irqfunc(irq->data);
    }
    return NULL;
}


osal_result os_start_irq_thread(os_irquser_t *irq)
{
    pthread_attr_t      attr;
    struct sched_param param;
    int ret;

    // All this code assumes that we are runnning as root and are able to
    // set the scheduling parameters
    if((ret = pthread_attr_init(&attr))) {
        OS_ERROR("pthread_attr_init failed error: %d\n", ret);
        return OSAL_ERROR;
    }

    param.sched_priority = 99; //highest priority

    if((ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED))){
        OS_ERROR("pthread_attr_setinheritsched failed error: %d\n", ret);
        return OSAL_ERROR;
    }

    if((ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO))) {
        OS_ERROR("pthread_attr_setschedpolicy failed error: %d\n", ret);
        return OSAL_ERROR;
    }

    if((ret = pthread_attr_setschedparam(&attr, &param))) {
        OS_ERROR("pthread_attr_setschedparam failed error: %d\n", ret);
        return OSAL_ERROR;
    }

    if(pthread_create(&irq->thread, &attr, &os_irq_thread, irq)){
        OS_ERROR("pthread_create failed\n");
        return OSAL_ERROR;
    }
    return OSAL_SUCCESS;
}


osal_result os_register_top_half(
            int irqnum,
            os_interrupt_t *irqhandle,
            os_interrupt_handler_t *irqhandler,
            void *data,
            const char *name)
{
    os_irquser_t *irq;
    int ret;

    if(!irqhandle || !irqhandler || !name || irqnum > 255) {
        return OSAL_INVALID_PARAM;
    }

    irq = OS_ALLOC(sizeof(os_irquser_t));

    if(!irq) {
        return OSAL_ERROR;
    }
    irq->irqnum = irqnum;
    irq->irqfunc = irqhandler;
    irq->data = data;

    irq->fd = open("/proc/irqproxy", O_RDWR);
    if(irq->fd < 0){
        OS_ERROR("cannot open irq proxy handle\n");
        goto error;
    }

    if(mlockall(MCL_FUTURE)){
        OS_ERROR("mlockall failed.  Userspace IRQs may be unsafe\n");
    }

    ret = write(irq->fd, &irq->irqnum, sizeof(irq->irqnum));
    if(ret < 0){
        OS_ERROR("error setting bonding to irq %d\n", irq->irqnum);
        goto error;
    }

    if(os_start_irq_thread(irq) != OSAL_SUCCESS){
        OS_ERROR("failed to start irq thread\n");
        goto error;
    }

    *irqhandle = (void *)irq;
    return OSAL_SUCCESS;

 error:
    if(irq->fd >= 0) {
        close(irq->fd);
    }
    OS_FREE(irq);
    return OSAL_ERROR;
}

osal_result os_unregister_top_half(os_interrupt_t *irqhandle)
{
    os_irquser_t *irq;

    if(!irqhandle) {
        return OSAL_ERROR;
    }

    irq = *irqhandle;
    if(!irq) {
        return OSAL_ERROR;
    }

    // this close will also cause the read() to fail in the irq thread
    close(irq->fd);
    pthread_join(irq->thread, NULL);

    OS_FREE(irq);
    *irqhandle = NULL;
    return OSAL_SUCCESS;
}
