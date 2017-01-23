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

#include "os/linux_kernel.h"
#include "osal.h"

typedef struct _timeout {
    struct timer_list   timer;
    bool                timedout;
    wait_queue_head_t * queue;
}timeout_t;

static void timeout_handler(unsigned long arg)
{
    timeout_t *t = (timeout_t *) arg;

    OS_DEBUG("Event timeout\n");
    t->timedout = true;
    wake_up(t->queue);
}

static void set_timeout(timeout_t * t,
                        unsigned int milliseconds,
                        wait_queue_head_t * q)
{
    t->timedout = false;
    t->queue = q;
    init_timer(&t->timer);

    /* HZ is the number of ticks per second for Intel architectures. */
    t->timer.expires = jiffies + (milliseconds * HZ) / 1000;
    t->timer.data = (unsigned long) t;
    t->timer.function = timeout_handler;
    add_timer(&t->timer);
}

static void clear_timeout(timeout_t *t)
{
    del_timer(&t->timer);
}

static bool timedout(timeout_t * t)
{
    return (t->timedout);
}

osal_result os_event_create(os_event_t* p_event, int manual_reset )
{
    spin_lock_init(&p_event->spinlock);
    init_waitqueue_head(&p_event->wait_queue);
    p_event->signaled = false;
    p_event->manual_reset = manual_reset;
    p_event->state = OSAL_INITIALIZED;

    return (OSAL_SUCCESS);
}

osal_result os_event_destroy(os_event_t* p_event)
{
    osal_result ret_val = OSAL_SUCCESS;
    p_event->state = OSAL_UNINITIALIZED;

    return (ret_val);
}

osal_result os_event_set(os_event_t* p_event)
{
    unsigned long flags;

    // Get the spin lock first
    spin_lock_irqsave(&p_event->spinlock, flags); //Start CS 1

    p_event->signaled = true;

    if(waitqueue_active(&p_event->wait_queue)) {
        if(!p_event->manual_reset) {
            p_event->signaled = false;      // reset the event
        }
        wake_up(&p_event->wait_queue);  // Wake up one waiter
    }

    spin_unlock_irqrestore(&p_event->spinlock, flags); // End CS 1
    return (OSAL_SUCCESS);
}

osal_result os_event_reset(os_event_t* p_event)
{
    unsigned long flags;

    // Get the spinlock first
    spin_lock_irqsave(&p_event->spinlock, flags);
    p_event->signaled = false;
    spin_unlock_irqrestore(&p_event->spinlock, flags);

    return (OSAL_SUCCESS);
}

static
osal_result do_os_event_wait(   os_event_t* p_event,
                                unsigned long wait_ms,
                                int interruptible)
{
    unsigned long flags;
    osal_result ret_val = OSAL_SUCCESS;
    timeout_t timeout;

    DEFINE_WAIT(wait);

    //We put this process on the waitqueue,
    prepare_to_wait(&p_event->wait_queue,
                    &wait,
                    interruptible ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE);

    // Start critical section (CS) 1
    spin_lock_irqsave(&p_event->spinlock, flags);

    // Return immediately if the event is signaled
    if(p_event->signaled) {
        OS_DEBUG("Event signaled already\n");

        if(0 == p_event->manual_reset) {
            //reset the event if manual reset is zero
            p_event->signaled = false;
        }

        spin_unlock_irqrestore(&p_event->spinlock, flags);
        finish_wait(&p_event->wait_queue, &wait);
        return (ret_val);
    }

    if(0 == wait_ms) {
        spin_unlock_irqrestore(&p_event->spinlock, flags);
        finish_wait(&p_event->wait_queue, &wait);
        return (OSAL_TIMEOUT);
    }

    spin_unlock_irqrestore(&p_event->spinlock, flags); // End CS1

    // If a timeout was specified, set a timer to wake us up.
    if (wait_ms != EVENT_NO_TIMEOUT) {
        set_timeout(&timeout, wait_ms, &p_event->wait_queue);
    }

    schedule(); //Yield the CPU

    // This process is now woken up.  Remove it from the waitqueue
    finish_wait(&p_event->wait_queue, &wait);

    if(interruptible && signal_pending(current)) {
        // Check if we  were woken up because of a signal, and there is a timer
        // running reset the timer
        if(EVENT_NO_TIMEOUT != wait_ms) {
            clear_timeout(&timeout);
        }
        ret_val = OSAL_NOT_DONE;
    } else if(wait_ms != EVENT_NO_TIMEOUT) {
        OS_DEBUG("Event received\n");
        clear_timeout(&timeout);    // Reset the timeout
        // Check for timeout
        if(timedout(&timeout)) {
            OS_DEBUG("Event timed out\n");
            ret_val = OSAL_TIMEOUT;
        }
    } else {
        //Event received (No timeout)
        ret_val = OSAL_SUCCESS;
    }

    if (!p_event->manual_reset) {
        if(os_event_reset(p_event) == OSAL_ERROR) {
            ret_val = OSAL_ERROR;
        }
    }

    return (ret_val);
}


osal_result os_event_wait(os_event_t* p_event,
                          unsigned long wait_ms)
{
    return do_os_event_wait(p_event, wait_ms, 1);
}

osal_result os_event_hardwait(os_event_t* p_event,
                                unsigned long wait_ms)
{
    return do_os_event_wait(p_event, wait_ms, 0);
}
