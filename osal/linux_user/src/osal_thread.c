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

#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>  /// Needed for ENOTSUP

#include "osal.h"

/*
 * Internal function to run a new user mode thread.  This will be the new
 * thread's main function, and will invoke the callers main function after
 * wrapping it with synchronization.
 */
static
void *__osal_thread_wrapper(void *arg)
{
    os_thread_t *t = (os_thread_t *) arg;

    // Cancel type set to ASYNC. Default is DEFERRED, this means the thread
    // can be cancelled at anytime, not at a cancellation point.
    //
    // TODO: abort if this call returns an error
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    // 2 cases in the code
    // - This thread runs before the creating thread gets to osal_thread_resume.
    //   In that case, this thread waits here, and osal_thread_resume wakes this
    //   thread up (the state is THREAD_ACTIVE)
    // - This thread runs after osal_thread_resume is called in which case the
    //   state has been set to THREAD_ACTIVE and the thread does not wait for
    //   the condition variable to be set.
    pthread_mutex_lock(&t->mtx);
    if (t->state == OS_THREAD_CREATE_SUSPENDED) {
        pthread_cond_wait(&t->cvr,&t->mtx);
    }
    pthread_mutex_unlock(&t->mtx);

    // Invoke caller's thread main function
    t->pfn_callback(t->context);

    t->state = THREAD_INACTIVE;
    return NULL;
}

static
osal_result validate_priority(int policy, int priority)
{
    osal_result ret_code = OSAL_SUCCESS;

    if (policy == SCHED_OTHER) {
        // Non-realtime thread
        if (priority != 0) {
            OS_ERROR("Non-0 priority for non-realtime thread: %d\n", priority);
            ret_code = OSAL_INVALID_PARAM;
        }
    } else {
        // Realtime thread
        if ((priority < 1) || (priority > 99)) {
            OS_ERROR("Invalid priority for realtime thread: %d\n", priority);
            ret_code = OSAL_INVALID_PARAM;
        }
    }

    return ret_code;
}


osal_result
os_thread_create(   os_thread_t *   p_thread,
                    void *          (*func)(void*),
                    void *          arg,
                    int             priority,
                    unsigned        flags,
                    char *          name        // IGNORED IN USER SPACE
                    )
{
    osal_result         ret_code = OSAL_SUCCESS;
    int                 ret;
    pthread_attr_t      attr;
    struct sched_param  sparam;
    int                 invalid_flags;
    int                 policy;

    (void) (name);  // Suppress gcc warning

    invalid_flags = flags & ~(OS_THREAD_CREATE_SUSPENDED | OS_THREAD_REALTIME);
    if ( invalid_flags ) {
        OS_ERROR("Invalid flag(s) passsed: 0x%08x\n", invalid_flags);
        return OSAL_INVALID_PARAM;
    }

    OS_ASSERT(p_thread);

    policy = (flags & OS_THREAD_REALTIME) ? SCHED_RR : SCHED_OTHER;
    ret_code = validate_priority(policy, priority);
    if (ret_code != OSAL_SUCCESS) {
        return ret_code;
    }

    pthread_attr_init(&attr);

    // NPTL (2.6 kernel thread library) requires us to set this.
    if (pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED) == ENOTSUP) {
        OS_ERROR("pthread_attr_setinheritsch: PTHREAD_EXPLICIT_SCHED failed\n");
        return OSAL_ERROR;
    }

    sparam.sched_priority = priority;
    pthread_attr_setschedpolicy(&attr, policy);
    pthread_attr_setschedparam(&attr, &sparam);
    
    pthread_mutex_init(&p_thread->mtx, NULL);
    pthread_cond_init (&p_thread->cvr, NULL);
    p_thread->osd.state = OSAL_UNINITIALIZED;

    // Initialize our thread structure, suspending the thread until its priority
    // gets set. The state gets set correctly by the thread wrapper when the
    // thread resumes in  __osal_thread_wrapper
    p_thread->pfn_callback = func;
    p_thread->context      = arg;
    p_thread->state        = OS_THREAD_CREATE_SUSPENDED;

    ret = pthread_create(&p_thread->osd.id,
                         &attr,
                         __osal_thread_wrapper,
                         (void*)p_thread);
    if (ret != 0) {
        OS_ERROR("pthread_create returned error %d\n", ret);
        return( OSAL_ERROR );
    }

    if (ret_code == OSAL_SUCCESS) {
        p_thread->osd.state = OSAL_INITIALIZED;
        if (!(flags & OS_THREAD_CREATE_SUSPENDED)) {
            ret_code = os_thread_resume(p_thread);
        }
    }
    return ret_code;
}


osal_result
os_thread_set_priority(os_thread_t *p_thread, int priority)
{
    osal_result         ret_code  = OSAL_SUCCESS;
    int                 policy;
    struct sched_param  param;
    int                 ret;

    if (!p_thread) {
        return OSAL_INVALID_PARAM;
    }

    pthread_mutex_lock(&p_thread->mtx);

    ret = pthread_getschedparam(p_thread->osd.id, &policy, &param);
    if (ret != 0) {
        OS_ERROR("Failure in pthread_getschedparam %d\n", ret);
        ret_code = OSAL_ERROR;
        goto unlock;
    }
 
    if (OSAL_SUCCESS != (ret_code = validate_priority(policy, priority))) {
        goto unlock;
    }

    param.sched_priority = priority;
    ret = pthread_setschedparam(p_thread->osd.id, policy, &param);
    if (ret != 0) {
        OS_ERROR("Failure in pthread_setschedparam %d\n", ret);
        ret_code = OSAL_ERROR;
        goto unlock;
    }

unlock:
    pthread_mutex_unlock(&p_thread->mtx);
    return ret_code;
}


osal_result
os_thread_get_priority(os_thread_t *p_thread, int *priority, int *flags)
{
    osal_result         ret_code  = OSAL_SUCCESS;
    int                 policy;
    struct sched_param  param;
    int                 ret;

    if (!p_thread) {
        return OSAL_INVALID_PARAM;
    }

    pthread_mutex_lock(&p_thread->mtx);

    ret = pthread_getschedparam(p_thread->osd.id, &policy, &param);
    if (ret != 0) {
        OS_ERROR("Failure in pthread_getschedparam: %d\n", ret);
        ret_code = OSAL_ERROR;
    } else {
        *priority = param.sched_priority;
        *flags    = (policy == SCHED_OTHER) ? 0 : OS_THREAD_REALTIME;
    }
 
    pthread_mutex_unlock(&p_thread->mtx);
    return ret_code;
}


osal_result
os_thread_wait(os_thread_t *p_thread, int num_threads)
{
    int i = 0;
    osal_result ret_code = OSAL_SUCCESS;

    for(i = 0; i < num_threads; i++){
        pthread_join(p_thread[i].osd.id, NULL);
        p_thread[i].osd.state = OSAL_UNINITIALIZED;
    }
    return ret_code;
}


osal_result
os_thread_destroy(os_thread_t* const p_thread )
{
    int ret = 0;
    OS_ASSERT(p_thread);
    OS_ASSERT(osal_is_state_valid( p_thread->osd.state ));

    if(OSAL_INITIALIZED == p_thread->osd.state){
        pthread_cancel(p_thread->osd.id);
        ret = pthread_join(p_thread->osd.id, NULL);
    }

    if(ret){
        OS_ERROR("pthread_join returned error %d\n", ret);
    }
    p_thread->osd.state = OSAL_UNINITIALIZED;
    return OSAL_SUCCESS;
}


void
os_thread_exit(os_thread_t* p_thread, unsigned long retval)
{
    p_thread->state = THREAD_INACTIVE;
    pthread_exit(&retval);
    return;
}

osal_result
os_thread_terminate(os_thread_t* const  p_thread )
{
    osal_result ret_val   = OSAL_SUCCESS;
    int         old_state = p_thread->state;
    int         status;
    void *      result;

    p_thread->state = THREAD_INACTIVE;
    status          = pthread_cancel(p_thread->osd.id);

    if(0 != status) {
        p_thread->state = old_state;
        ret_val         = OSAL_ERROR;
    } else {
        status = pthread_join(p_thread->osd.id, &result);
        if(PTHREAD_CANCELED != result) {
            ret_val = OSAL_ERROR;
        }
    }

    return ret_val;
}


void os_thread_yield()
{
   sched_yield();
}


osal_result
os_thread_suspend(os_thread_t* p_thread)
{
    if((p_thread->state == OS_THREAD_CREATE_SUSPENDED)
    || (p_thread->state == THREAD_SUSPENDED)) { //already suspended
        return OSAL_SUCCESS;
    }
    return OSAL_NOT_IMPLEMENTED;
}


osal_result
os_thread_resume(os_thread_t * p_thread)
{
    if(p_thread->state == OS_THREAD_CREATE_SUSPENDED)
    {
        //the thread was created suspended
        pthread_mutex_lock(&p_thread->mtx);
        p_thread->state = THREAD_ACTIVE;
        pthread_cond_signal(&p_thread->cvr);
        pthread_mutex_unlock(&p_thread->mtx);
        return OSAL_SUCCESS;
    }

    return OSAL_NOT_IMPLEMENTED;
}

osal_result os_thread_get_state(os_thread_t* p_thread, int * state)
{
    *state = p_thread->state;
    return OSAL_SUCCESS;
}


void
os_sleep(unsigned long delay_in_ms)
{
    usleep(delay_in_ms * 1000);
    return ;
}
