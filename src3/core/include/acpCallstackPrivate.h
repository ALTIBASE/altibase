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
 * $Id: acpCallstackPrivate.h 6272 2009-07-07 08:06:27Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_CALLSTACK_PRIVATE_H_)
#define _O_ACP_CALLSTACK_PRIVATE_H_

#include <acpCallstack.h>


ACP_EXTERN_C_BEGIN


#if defined(ALTI_CFG_OS_AIX)
#define ACP_CALLSTACK_LOCALCTX_T  ucontext_t
#define ACP_CALLSTACK_SIGNALCTX_T ucontext_t
#elif defined(ALTI_CFG_OS_HPUX)
#define ACP_CALLSTACK_SIGNALCTX_T ucontext_t
#elif defined(ALTI_CFG_OS_LINUX)
#define ACP_CALLSTACK_LOCALCTX_T  ucontext_t
#define ACP_CALLSTACK_SIGNALCTX_T ucontext_t
#elif defined(ALTI_CFG_OS_FREEBSD)
#define ACP_CALLSTACK_LOCALCTX_T  ucontext_t
#define ACP_CALLSTACK_SIGNALCTX_T ucontext_t
#elif defined(ALTI_CFG_OS_DARWIN)
#define ACP_CALLSTACK_LOCALCTX_T  ucontext_t
#define ACP_CALLSTACK_SIGNALCTX_T ucontext_t
#elif defined(ALTI_CFG_OS_SOLARIS)
#define ACP_CALLSTACK_LOCALCTX_T  ucontext_t
#define ACP_CALLSTACK_SIGNALCTX_T ucontext_t
#elif defined(ALTI_CFG_OS_TRU64)
#define ACP_CALLSTACK_LOCALCTX_T  CONTEXT
#define ACP_CALLSTACK_SIGNALCTX_T ucontext_t
#elif defined(ALTI_CFG_OS_WINDOWS)
#define ACP_CALLSTACK_LOCALCTX_T  CONTEXT
#define ACP_CALLSTACK_SIGNALCTX_T CONTEXT
#endif


void     acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                                ACP_CALLSTACK_SIGNALCTX_T *aContext);
void     acpCallstackFinal(acp_callstack_t *aCallstack);

acp_rc_t acpCallstackTraceInternal(acp_callstack_t          *aCallstack,
                                   acp_callstack_callback_t *aCallback,
                                   void                     *aCallbackData);


ACP_EXTERN_C_END


#endif
