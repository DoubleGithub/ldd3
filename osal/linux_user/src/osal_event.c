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
//event support

#include <sys/time.h>
#include <sys/errno.h>
#include "osal.h"

void
osal_event_construct(
	os_event_t*  p_event )
{
	OS_ASSERT( p_event );

	p_event->lock = os_create_lock();
	p_event->state = OSAL_UNINITIALIZED;
}


osal_result
os_event_create(
	os_event_t* 	p_event,
	int manual_reset )
{
	OS_ASSERT( p_event );

	osal_event_construct( p_event );

	pthread_cond_init( &p_event->condvar, NULL );
	p_event->signaled = false;
	p_event->manual_reset = manual_reset;
	p_event->state = OSAL_INITIALIZED;	

	return( OSAL_SUCCESS );
}


osal_result
os_event_destroy(
	os_event_t* p_event )
{
	OS_ASSERT( osal_is_state_valid( p_event->state ) );

	/* Destroy only if the event was constructed */
	if( p_event->state == OSAL_INITIALIZED )
	{
		pthread_cond_broadcast( &p_event->condvar );
 		pthread_cond_destroy( &p_event->condvar );
	}

	os_destroy_lock( p_event->lock );
	p_event->state = OSAL_UNINITIALIZED;
	return OSAL_SUCCESS;
}


osal_result
os_event_set(
	os_event_t* p_event )
{
	/* Make sure that the event was started */
	OS_ASSERT( p_event->state == OSAL_INITIALIZED );
	
 	os_lock( p_event->lock );
	p_event->signaled = true;

	/* Wake up one */
	pthread_cond_signal( &p_event->condvar );

	os_unlock( p_event->lock );

	return( OSAL_SUCCESS );
}


osal_result
os_event_reset(
		os_event_t* p_event )
{
	/* Make sure that the event was started */
	OS_ASSERT( p_event->state == OSAL_INITIALIZED );

	os_lock( p_event->lock );
	p_event->signaled = false;
	os_unlock( p_event->lock );

	return( OSAL_SUCCESS );
}

osal_result
os_event_wait(
		os_event_t* p_event,
		unsigned long	wait_ms) //in milliseconds
{
	osal_result	    status;
	int		        wait_ret;
	struct timespec	timeout;
	struct timeval	curtime;

	/* Make sure that the event was Started */
	OS_ASSERT( p_event->state == OSAL_INITIALIZED );

	os_lock( p_event->lock );

	/* Return immediately if the event is signalled. */
	if( p_event->signaled ) {
		if( !p_event->manual_reset ) {
			p_event->signaled = false;
        }

		os_unlock( p_event->lock );
		return( OSAL_SUCCESS );
	}

	/* If just testing the state, return OSAL_TIMEOUT. */
	if( wait_ms == 0 ) {
		os_unlock( p_event->lock );
		return( OSAL_TIMEOUT );
	}

	if( wait_ms == EVENT_NO_TIMEOUT ) {
		/* Wait for condition variable to be signaled or broadcast. */
                /* WARNING! Layering violation.  Assumes os_lock_t is a
                   (pthread_mutex_t *) */
		if( (wait_ret = pthread_cond_wait( &p_event->condvar, p_event->lock ))){
			status = OSAL_NOT_DONE;
        } else {
			status = OSAL_SUCCESS;
        }
	} else {
		/* Get the current time */
		if( gettimeofday( &curtime, NULL ) != 0 ) {
			status = OSAL_ERROR;
		} else {
			timeout.tv_sec = curtime.tv_sec + (wait_ms / 1000);
			timeout.tv_nsec = (curtime.tv_usec + (wait_ms % 1000)*1000) * 1000;
			// check that tv_nsec is less than a second.  Don't
			// let it overflow or you'll be sorry
			if(timeout.tv_nsec >= 1000000000){
				timeout.tv_sec++;
				timeout.tv_nsec -= 1000000000;
			}
			/* WARNING! Layering violation.  Assumes os_lock_t is a
                           (pthread_mutex_t *) */
			wait_ret = pthread_cond_timedwait( &p_event->condvar,
				p_event->lock, &timeout );
			if( wait_ret == 0 ) {
				status = OSAL_SUCCESS;
            } else if( wait_ret == ETIMEDOUT ) {
				status = OSAL_TIMEOUT;
            } else {
				status = OSAL_NOT_DONE;
            }
		}
	}

	if( !p_event->manual_reset ) {
		p_event->signaled = false;
    }

	os_unlock( p_event->lock );
	return( status );
}

// same in userspace as a normal wait
osal_result os_event_hardwait(os_event_t* p_event,
                              unsigned long   wait_ms) //in milliseconds
{
    return os_event_wait(p_event, wait_ms);
}
