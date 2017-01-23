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

#include "osal_type.h"
#include "osal_io.h"
#include "os/osal_sema.h"
#include "os/linux_kernel.h"

/*
 * use DEFINE_SPINLOCK() instead, will be deleted permantly */
/* spinlock_t static_sema_lock = SPIN_LOCK_UNLOCKED; */

static DEFINE_SPINLOCK(static_sema_lock);

int os_sema_init_pre_inited(os_sema_t *p_sema, int initial)
{
    osal_result ret_code = OSAL_SUCCESS;

    if(NULL == p_sema){
        ret_code = OSAL_INVALID_PARAM;
        goto error;
    }

    spin_lock(&static_sema_lock);
    if(0 == p_sema->sema_static_init_cnt) {
        sema_init(&p_sema->sema, initial);
        p_sema->sema_static_init_cnt = 1;
    }
    spin_unlock(&static_sema_lock);

error:
    return ret_code;
}


int os_sema_init(os_sema_t *p_sema, int initial)
{
    osal_result ret_code = OSAL_SUCCESS;

    if(NULL == p_sema){
        ret_code = OSAL_INVALID_PARAM;
        goto error;
    }
    sema_init(&p_sema->sema, initial); // The return type is void

error:
    return ret_code;
}

void os_sema_destroy(os_sema_t *p_sema)
{
    // Nothing to be done for now, e.g., freeing memory etc.
    return;
}

void os_sema_get(os_sema_t *p_sema)
{
    //Does not handle the case of down_interruptible
    //The process cannot be interrupted by a signal and will hang
    //if the semaphore is never got.
    down(&p_sema->sema);
}

int os_sema_tryget(os_sema_t *p_sema)
{
    // Non-blocking version. Returns zero if sem acquired, non-zero if not acquired.
    //Does not handle the case of down_interruptible
    //The process cannot be interrupted by a signal and will hang
    //if the semaphore is never got.
    return down_trylock(&p_sema->sema);
}

void os_sema_put(os_sema_t *p_sema)
{
    up(&p_sema->sema);
}
