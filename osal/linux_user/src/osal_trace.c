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
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <osal_char.h>
#include <osal_trace.h>

// for backtrace
#include <execinfo.h>
#include <signal.h>
#include <sys/time.h>

#define MAXSTR 512

char *      level_string[] = { " ", " *** ERROR *** ", "WARNING ", "INFO" };
static int  priority = LOG_USER;
int         syslog_trace_level[] = {LOG_DEBUG, LOG_ERR, LOG_WARNING, LOG_INFO};

TRACE_PARMS *trace_init(char *subsys, char *name, TRACE_LEVEL level)
{
    TRACE_PARMS *parms = (TRACE_PARMS*)malloc( sizeof( TRACE_PARMS ) );

    if(parms == NULL) {
        return NULL;
    }

    parms->label = name;
    parms->subsys = subsys;
    parms->level = level;

    return (parms);
}

void trace_deinit(TRACE_PARMS *tparm)
{
    if(tparm) {
        free(tparm);
    }
}

void trace( TRACE_PARMS *subsys, TRACE_LEVEL level, TCHAR *szFormat, ... )
{
    char buffer[MAXSTR],temp_buff[MAXSTR];
    va_list list;
    va_start( list, szFormat );

    if(subsys == NULL)
    {
        printf("Invalid TRACE handle, may be not Initialized !!\n");
        //syslog("Invalid TRACE handle, may be not Initialized !!");
        return;
    }

    if( subsys->level >= level )
    {
        vsprintf( buffer, szFormat, list );
        sprintf(temp_buff,
                "%s_%s:%s:%s \n",
                subsys->subsys,
                subsys->label,
                level_string[level],
                buffer );

        priority = priority | syslog_trace_level[subsys->level];
        syslog(priority, temp_buff);
    }
    return;
}


void verify_trace( char *szFormat, ... )
{
    char buffer[MAXSTR],temp_buff[MAXSTR];
    va_list list;
    va_start( list, szFormat );

    vsprintf( buffer, szFormat, list );
    sprintf( temp_buff,"VERIFY:%s%s\n", level_string[T_ERROR], buffer );
    priority = priority | LOG_ERR;
    syslog(priority, temp_buff);
    //OutputDebugString(temp_buff);
}

int osal_debug=2; //changed so _os_debug is not enabled by default

/* Prints only when osal_debug >= 3 */
void _os_debug( char *szFormat, ... )
{
    char    buffer[MAXSTR];
    va_list list;

    if( osal_debug >= 3 ) {
        va_start( list, szFormat );
        vsnprintf( buffer, MAXSTR, szFormat, list );
        printf( "DEBUG: %s", buffer );
    }
}


/* Prints only when osal_debug >= 2 */
void _os_print( char *szFormat, ... )
{
    char    buffer[MAXSTR];
    va_list list;

    if( osal_debug >= 2 ) {
        va_start( list, szFormat );
        vsnprintf( buffer, MAXSTR, szFormat, list );
        printf( "%s", buffer );
       fflush(stdout);
    }
}

// Prints at all times, necessary for audio, do not change without consent of
// audio team
void _os_info( char *szFormat, ... )
{
    char buffer[MAXSTR];
    va_list list;

    va_start( list, szFormat );

    vsnprintf( buffer, MAXSTR, szFormat, list );
    printf( "%s", buffer );
}


/* Prints only when osal_debug >= 1 */
void _os_error( char *szFormat, ... )
{
    char buffer[MAXSTR];
    va_list list;

    if( osal_debug >= 1 ) {
        va_start( list, szFormat );
        vsnprintf( buffer, MAXSTR, szFormat, list );
        printf( "ERROR: %s", buffer );
    }
}


void os_backtrace(void)
{
    void *  stacklist[50];
    int     size;

    size = backtrace(stacklist, 50);
    fflush(stdout);
    backtrace_symbols_fd(stacklist, size, 1);
    fflush(stdout);
}
