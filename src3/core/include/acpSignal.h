/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id: acpSignal.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_SIGNAL_H_)
#define _O_ACP_SIGNAL_H_

/**
 * @file
 * @ingroup CoreThr
 * @ingroup CoreProc
 */

#include <acpCallstack.h>
#include <acpError.h>


ACP_EXTERN_C_BEGIN


#if defined (ALTI_CFG_OS_WINDOWS)

#if !defined(SIGTRAP)
/* STATUS_BREAKPOINT translated into SIGTRAP */
#define SIGTRAP   5
#endif
#if !defined(SIGKILL)
#define SIGKILL   9
#endif
#if !defined(SIGBUS)
/* STATUS_IN_PAGE_ERROR translated into SIGBUS */
#define SIGBUS    10
#endif
#if !defined(SIGSYS)
/* STATUS_INVALID_PARAMETER translated into SIGSYS */
#define SIGSYS    12
#endif
/*
 * BUG-30089
 * SIGPIPE is not available on Windows(TM)
 * This is a fake definition
 */
#if !defined(SIGPIPE)
#define SIGPIPE   13
#endif
#if !defined(SIGALRM)
/* STATUS_TIMEOUT translated into SIGALRM */
#define SIGALRM   14
#endif
#if !defined(SIGSTKFLT)
/* STATUS_FLOAT_STACK_CHECK translated into SIGSTKFLT */
#define SIGSTKFLT 16
#endif

/* defines additional windows exception codes */
#if !defined(STATUS_ABORT_EXIT)
#define STATUS_ABORT_EXIT           ((DWORD)0xC0000140L)
#endif
#if !defined(STATUS_INVALID_PARAMETER)
#define STATUS_INVALID_PARAMETER    ((DWORD)0xC000000DL)
#endif
#if !defined(STATUS_STACK_BUFFER_OVERRUN)
#define STATUS_STACK_BUFFER_OVERRUN ((DWORD)0xC000010DL)
#endif
#if !defined(STATUS_PROCESS_KILLED)
#define STATUS_PROCESS_KILLED       ((DWORD)0xC0000150L)
#endif

#endif  /* ALTI_CFG_OS_WINDOWS */


/**
 * signal handler function
 */
typedef void acp_signal_handler_t(acp_sint32_t     aSignal,
                                  acp_callstack_t *aCallstack);


/**
 * signal set for signal blocking
 * @see acpSignalBlock() acpSignalUnblock()
 */
typedef enum acp_signal_set_t
{
    ACP_SIGNAL_SET_INT,  /**< interrupt signal   */
    ACP_SIGNAL_SET_PIPE, /**< broken pipe signal */
    ACP_SIGNAL_SET_MAX
} acp_signal_set_t;


void acpSignalGetDefaultBlockSet(void);


ACP_EXPORT acp_rc_t acpSignalBlockDefault(void);
ACP_EXPORT acp_rc_t acpSignalBlockAll(void);

ACP_EXPORT acp_rc_t acpSignalBlock(acp_signal_set_t aSignalSet);
ACP_EXPORT acp_rc_t acpSignalUnblock(acp_signal_set_t aSignalSet);

ACP_EXPORT acp_rc_t acpSignalSetExceptionHandler(acp_signal_handler_t *
                                                 aHandler);


ACP_EXTERN_C_END


#endif
