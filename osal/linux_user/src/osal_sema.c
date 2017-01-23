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
#include <assert.h>
#include <pthread.h>
#include "osal.h"

static pthread_mutex_t fast_mutex = PTHREAD_MUTEX_INITIALIZER;

osal_result os_sema_init_pre_inited(os_sema_t *s, int initial)
{
    pthread_mutex_lock(&fast_mutex);
    if(0 == s->sema_static_init_cnt) {
        pthread_mutex_init(&s->lock, NULL);
        pthread_cond_init(&s->non_zero, NULL);
        s->count = initial;
        s->sema_static_init_cnt = 1;
    }
    pthread_mutex_unlock(&fast_mutex);

    return (OSAL_SUCCESS);
}

osal_result os_sema_init(os_sema_t *s, int initial)
{
    //! Initialize the mutex associated with the condition variable
    pthread_mutex_init(&s->lock, NULL);
    //! initialize the condition variable
    pthread_cond_init(&s->non_zero, NULL);
    //! Set the count on the semaphore
    s->count = initial;
    return (OSAL_SUCCESS);
}

void os_sema_destroy(os_sema_t *s)
{
    //! Undo operations in create.
    pthread_cond_destroy(&s->non_zero);
    pthread_mutex_destroy(&s->lock);
}

void os_sema_get(os_sema_t *s)
{
    pthread_mutex_lock(&s->lock);

    //! Wait for the condition after grabbing the mutex
    while (s->count == 0) {
        //! the mutex is held when we come out of the wait so we can modify
        //! the count.
        pthread_cond_wait(&s->non_zero, &s->lock);
    }
    s->count--;

   pthread_mutex_unlock(&s->lock);
}


void os_sema_put(os_sema_t *s)
{
    pthread_mutex_lock(&s->lock);
    s->count++;
    pthread_mutex_unlock (&s->lock);

    //! Signal the waiter
    pthread_cond_signal(&s->non_zero);
}
