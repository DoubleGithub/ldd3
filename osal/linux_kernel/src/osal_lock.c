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
 * This file contains OS abstracted interfaces to lock operations.
 */

#include "osal_lock.h"
#include "osal_sema.h"
#include "osal_io.h"
#include "osal_memory.h"
#include "os/linux_kernel.h"
#include "osal_type.h"

typedef struct
{
    os_sema_t *p_sema;
}os_lock_pvt_t;

os_lock_t os_create_lock(void)
{
    os_lock_pvt_t *p_lock = NULL;

    if(NULL == (p_lock = OS_ALLOC(sizeof(os_lock_pvt_t)))) {
        OS_ERROR("OS_ALLOC failed\n");
        goto error;
    }

    if(NULL == (p_lock->p_sema = OS_ALLOC(sizeof(os_sema_t)))) {
        OS_ERROR("OS_ALLOC failed\n");
        goto error;
    }

    os_sema_init(p_lock->p_sema, 1);
    return ((os_lock_t)p_lock);

error:
    if(p_lock != NULL) {
        OS_FREE(p_lock);
    }
    return (NULL);
}

int os_lock(os_lock_t lock)
{
    os_lock_pvt_t * p_lock = (os_lock_pvt_t *)lock;
    int             ret_val = 1;

    if(NULL != lock) {
        os_sema_get(p_lock->p_sema);
    }
    return (ret_val);
}

int os_try_lock(os_lock_t lock)
{
    os_lock_pvt_t * p_lock = (os_lock_pvt_t *)lock;
    int ret_val=1;  // 1-Not acquired, 0-acquired

    if(NULL != lock) {
        ret_val = os_sema_tryget(p_lock->p_sema);
    }
    return (ret_val);
}


int os_unlock(os_lock_t lock)
{
    int             ret_val = 1;
    os_lock_pvt_t * p_lock = (os_lock_pvt_t *)lock;

    if(NULL != lock) {
        os_sema_put(p_lock->p_sema);
    }
    return (ret_val);
}

int os_destroy_lock(os_lock_t lock)
{
    int             ret_val = 1;
    os_lock_pvt_t * p_lock = (os_lock_pvt_t *)lock;

    if(NULL != p_lock) {
        OS_FREE(p_lock->p_sema);
        OS_FREE(p_lock);
    }
    return (ret_val);
}
