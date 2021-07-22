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
 * $Id: acpCallstack.h 6272 2009-07-07 08:06:27Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_CALLSTACK_H_)
#define _O_ACP_CALLSTACK_H_

/**
 * @file
 * @ingroup CoreError
 */

#include <acpStd.h>
#include <acpStr.h>
#include <acpSym.h>


ACP_EXTERN_C_BEGIN


#if defined(ALTI_CFG_OS_HPUX)
#if defined(ACP_CFG_COMPILE_64BIT)

typedef struct
{
    unsigned long unwind_descriptor_bits;
    unsigned int  region_start_address;
    unsigned int  region_end_address;
} uw_rec_def;

typedef struct
{
    unsigned long size;
    unsigned long sp;
    unsigned long pcoffset;
    unsigned long gp;
    unsigned long rp;
    unsigned long mrp;
    unsigned long r3;
    unsigned long r4;
    unsigned long reserved[4];
} curr_frame_info;

typedef struct
{
    unsigned long size;
    unsigned long sp;
    unsigned long pcoffset;
    unsigned long gp;
    uw_rec_def    uw_rec;
    long          uw_index;
    unsigned long r3;
    unsigned long r4;
    unsigned long reserved[4];
} prev_frame_info;

#else

typedef struct
{
    unsigned frame_size;
    unsigned sp;
    unsigned pcspace;
    unsigned pcoffset;
    unsigned dp;
    unsigned rp;
    unsigned mrp;
    unsigned sr0;
    unsigned sr4;
    unsigned r3;
    unsigned r19;
    int      r4;
    int      reserved;
} curr_frame_info;

typedef struct
{
    unsigned frame_size;
    unsigned sp;
    unsigned pcspace;
    unsigned pcoffset;
    unsigned dp;
    unsigned udescr0;
    unsigned udescr1;
    unsigned ustart;
    unsigned uend;
    unsigned uw_index;
    unsigned r19;
    int      r3;
    int      r4;
} prev_frame_info;

#endif
#endif

/**
 * the context of the execution stack
 */
typedef struct acp_callstack_t
{
#if defined(ALTI_CFG_OS_AIX)
    void **mFP;
    void  *mPC;
#elif defined(ALTI_CFG_OS_HPUX)
#if defined(ALTI_CFG_CPU_IA64)
    struct uwx_env       *mEnv;
    struct uwx_self_info *mInfo;
    acp_uint64_t          mIP;
#elif defined(ALTI_CFG_CPU_PARISC)
    curr_frame_info mCurrContext;
    prev_frame_info mPrevContext;
#endif
#elif defined(ALTI_CFG_OS_LINUX)
    void **mFP;
    void  *mPC;
#  if defined(ACP_CFG_COMPILE_64BIT)
#  define ACP_CALLSTACK_MAXDEPTH (128)
    /* BUGBUG
     * This must be removed with the removal of backtrace
     */
    void*        mFrames[ACP_CALLSTACK_MAXDEPTH];
    acp_uint32_t mCursor;
    acp_uint32_t mNoFrames;
#  endif
#elif defined(ALTI_CFG_OS_FREEBSD)
    void **mFP;
    void  *mPC;
#elif defined(ALTI_CFG_OS_DARWIN)
    void **mFP;
    void  *mPC;
#elif defined(ALTI_CFG_OS_SOLARIS)
    struct frame *mFP;
    void         *mPC;
#elif defined(ALTI_CFG_OS_TRU64)
    CONTEXT *mContext;
#elif defined(ALTI_CFG_OS_WINDOWS)
    CONTEXT      mContext;
    STACKFRAME64 mStackFrame;
#else
#warning Unknown OS
#endif
} acp_callstack_t;

/**
 * the frame of the execution stack
 */
typedef struct acp_callstack_frame_t
{
    acp_sint32_t    mIndex;   /**< index of the frame                      */
    void           *mAddress; /**< PC address in the current frame         */
    acp_sym_info_t  mSymInfo; /**< symbol information of the current frame */
} acp_callstack_frame_t;

/**
 * callstack trace callback
 */
typedef acp_sint32_t acp_callstack_callback_t(acp_callstack_frame_t *aFrame,
                                              void                  *aData);


ACP_EXPORT acp_rc_t acpCallstackDumpToString(acp_callstack_t *aCallstack,
                                             acp_str_t       *aStr);

ACP_EXPORT acp_rc_t acpCallstackDumpToStdFile(acp_callstack_t *aCallstack,
                                              acp_std_file_t  *aFile);

ACP_EXPORT acp_rc_t acpCallstackTrace(acp_callstack_t          *aCallstack,
                                      acp_callstack_callback_t *aCallback,
                                      void                     *aCallbackData);


ACP_EXTERN_C_END


#endif
