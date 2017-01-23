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

#include "osal.h"

static unsigned int thread_cnt = 0;

static
int thread_wrapper(void *arg)
{
    int             ret_val = 0;
    os_thread_t *   thread = (os_thread_t *)arg;

    if (thread->priority != 0) {
        // REALTIME THREAD
        struct sched_param    prio;

        prio.sched_priority = thread->priority;
        if(sched_setscheduler(current, SCHED_RR, &prio)){
            OS_PRINT("Couldn't set scheduler priority to %d\n",thread->priority);
        }
    }

    // TODO: Better job of figuring out the return value.
    thread->pfn_callback(thread->context);
    os_event_set(&thread->kill_event);
    return ret_val;
}


static
osal_result validate_priority(int policy, int priority)
{
    osal_result ret_code = OSAL_SUCCESS;

    if (policy == SCHED_NORMAL) {
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


typedef void (*callback_t)(void *);

osal_result
os_thread_create(   os_thread_t *   thread,
                    void *          (*func)(void*),
                    void *          arg,
                    int             priority,
                    unsigned        flags,
                    char *          name
                    )
{
#define FORMAT "OSAL thread %d"
    char        buf[sizeof(FORMAT)+20];
    int         policy;
    int         invalid_flags;
    osal_result ret_code = OSAL_SUCCESS;

    OS_ASSERT(thread);

    invalid_flags = flags & ~(OS_THREAD_CREATE_SUSPENDED | OS_THREAD_REALTIME);
    if ( invalid_flags ) {
        OS_ERROR("Invalid flag(s) passsed: 0x%08x\n", invalid_flags);
        return OSAL_INVALID_PARAM;
    }

    policy = (flags & OS_THREAD_REALTIME) ? SCHED_RR : SCHED_NORMAL;
    ret_code = validate_priority(policy, priority);
    if (ret_code != OSAL_SUCCESS) {
        return ret_code;
    }

    thread_cnt++;
    if (name == NULL) {
        sprintf(buf, FORMAT, thread_cnt);
        name = buf;
    }

    os_event_create(&thread->kill_event, 0);
    thread->pfn_callback= (callback_t) func;
    thread->context     = arg;
    thread->priority    = priority;
    thread->state       = OSAL_INITIALIZED;
    thread->task        = kthread_run(thread_wrapper, (void*)thread, "%s",name);
    
    return OSAL_SUCCESS;
}


osal_result os_thread_wait(os_thread_t* const k_thread,  int count)
{
    unsigned int i;

    for(i = 0; i < count; i++) {
        if (k_thread[i].state == OSAL_INITIALIZED) {
            /* Wait without time out */
            os_event_hardwait(&k_thread[i].kill_event, EVENT_NO_TIMEOUT);
        }
    }
    return OSAL_SUCCESS;
}

void os_thread_exit(os_thread_t* k_thread, unsigned long retval)
{
    if (k_thread->state == OSAL_INITIALIZED) {
        /* set the exit event to unblock any waiters */
        os_event_set(&k_thread->kill_event);
    }
}

osal_result os_thread_destroy(os_thread_t* k_thread)
{
    if (k_thread->state == OSAL_INITIALIZED) {
        k_thread->pfn_callback = NULL;
        k_thread->context = NULL;
    }
    os_event_destroy(&k_thread->kill_event);
    return OSAL_SUCCESS;
}

// Not implemented for Linux kernel mode
//     osal_result os_thread_terminate(os_thread_t* const  k_thread )
//     osal_result os_thread_suspend(os_thread_t* k_thread)
//     osal_result os_thread_resume(os_thread_t * k_thread)

void os_thread_yield()
{
    int jiffie = 1*HZ/1000;

    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(jiffie);
}

void os_sleep(unsigned long delay_in_ms)
{
    msleep(delay_in_ms);
}

osal_result os_thread_get_state(os_thread_t* k_thread, int * state)
{
    *state = k_thread->state;
    return OSAL_SUCCESS;
}
