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
 
#ifndef _O_IDU_WIN_CALLSTACK_H_
#define _O_IDU_WIN_CALLSTACK_H_ 1 

# include <idConfig.h>

# if defined(VC_WIN32)

typedef void (*iduWin32CallStackCallback)(void *);
typedef void (*iduWin32ErrorMsgCallback)(void *);

extern int iduInitializeWINDOWSCallStack(iduWin32CallStackCallback aFuncArg,
                                       iduWin32ErrorMsgCallback  aFuncErr);
extern int iduDestroyWINDOWSCallStack();

extern int iduCallStackWINDOWS(const CONTEXT *context = NULL);

extern  void iduWriteMiniDump( void*       aExceptionPtr,
                               HANDLE      aDumpFile );

# endif

#endif
