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
 * $Id: acpCallstackPrivate.c 11386 2010-06-28 09:15:31Z djin $
 ******************************************************************************/

#include <acpCallstackPrivate.h>


#if defined(ALTI_CFG_OS_AIX)

#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext) \
    do                                          \
    {                                           \
        (void)getcontext(&(aContext));          \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return aCallstack->mPC;
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    aCallstack->mPC = aCallstack->mFP[2];
    aCallstack->mFP = aCallstack->mFP[0];

    if ((aCallstack->mFP != NULL) && (aCallstack->mPC != NULL))
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

ACP_INLINE void acpCallstackInitFromLC(acp_callstack_t          *aCallstack,
                                       ACP_CALLSTACK_LOCALCTX_T *aContext)
{
    acpCallstackInitFromSC(aCallstack, aContext);
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
    aCallstack->mFP = (void **)aContext->uc_mcontext.jmp_context.gpr[1];
    aCallstack->mPC = (void *)aContext->uc_mcontext.jmp_context.iar;
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}

#elif defined(ALTI_CFG_OS_HPUX)

#if defined(ALTI_CFG_CPU_IA64)

#define ACP_CALLSTACK_GETLOCALCONTEXT(aCallstack)                              \
    do                                                                         \
    {                                                                          \
        (aCallstack).mEnv  = uwx_init();                                    \
        (aCallstack).mInfo = uwx_self_init_info((aCallstack).mEnv);         \
                                                                               \
        (void)uwx_register_callbacks((aCallstack).mEnv,                     \
                                     (intptr_t)(aCallstack).mInfo,          \
                                     uwx_self_copyin,                          \
                                     uwx_self_lookupip);                       \
        (void)uwx_self_init_context((aCallstack).mEnv);                     \
                                                                               \
        (void)uwx_get_reg((aCallstack).mEnv, UWX_REG_IP, &((aCallstack).mIP)); \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return (void *)(acp_ulong_t)aCallstack->mIP;
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    acp_sint32_t sRet;

    sRet = uwx_step(aCallstack->mEnv);

    switch (sRet)
    {
        case UWX_ABI_FRAME:
            (void)uwx_self_do_context_frame(aCallstack->mEnv,
                                            aCallstack->mInfo);

        case UWX_OK:
            if (uwx_get_reg(aCallstack->mEnv,
                            UWX_REG_IP,
                            &aCallstack->mIP) == UWX_OK)
            {
                return ACP_TRUE;
            }
            else
            {
                return ACP_FALSE;
            }

        default:
            return ACP_FALSE;
    }
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
    aCallstack->mEnv  = uwx_init();
    aCallstack->mInfo = uwx_self_init_info(aCallstack->mEnv);

    (void)uwx_register_callbacks(aCallstack->mEnv,
                                 (intptr_t)aCallstack->mInfo,
                                 uwx_self_copyin,
                                 uwx_self_lookupip);

    (void)uwx_self_init_from_sigcontext(aCallstack->mEnv,
                                        aCallstack->mInfo,
                                        aContext);

    (void)uwx_get_reg(aCallstack->mEnv, UWX_REG_IP, &aCallstack->mIP);
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    (void)uwx_self_free_info(aCallstack->mInfo);
    (void)uwx_free(aCallstack->mEnv);
}

#elif defined(ALTI_CFG_CPU_PARISC)

#if defined(ACP_CFG_COMPILE_64BIT)

extern curr_frame_info U_get_current_frame(void);
extern int  U_get_previous_frame(curr_frame_info *, prev_frame_info *);
extern void U_copy_frame_info(curr_frame_info *, prev_frame_info *);

#define ACP_CALLSTACK_GETLOCALCONTEXT(aCallstack)               \
    do                                                          \
    {                                                           \
        (aCallstack).mCurrContext = U_get_current_frame();      \
                                                                \
        (void)acpCallstackUnwind(&(aCallstack));                \
    } while(0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return (void *) (((acp_ulong_t)aCallstack->mCurrContext.pcoffset) & (~3));
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    U_get_previous_frame(&aCallstack->mCurrContext, &aCallstack->mPrevContext);

    if (aCallstack->mPrevContext.pcoffset)
    {
        U_copy_frame_info(&aCallstack->mCurrContext, &aCallstack->mPrevContext);

        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
    void *sPC = (void *)
        ((acp_ulong_t)(aContext->uc_mcontext.ss_wide.ss_64.ss_pcoq_head) &
         (~3));

    ACP_CALLSTACK_GETLOCALCONTEXT((*aCallstack));

    while (acpCallstackGetPC(aCallstack) != sPC)
    {
        if (acpCallstackUnwind(aCallstack) != ACP_TRUE)
        {
            return;
        }
        else
        {
            /* do nothing */
        }
    }
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}

#else /* defined(ACP_CFG_COMPILE_64BIT) */

extern void U_get_frame_info(curr_frame_info *);
extern int U_get_previous_frame_x(curr_frame_info *, prev_frame_info *, int);

#define ACP_CALLSTACK_GETLOCALCONTEXT(aCallstack)       \
    do                                                  \
    {                                                   \
        U_get_frame_info(&(aCallstack).mCurrContext);   \
                                                        \
        (aCallstack).mCurrContext.sr0 = 0;              \
        (aCallstack).mCurrContext.sr4 = 0;              \
                                                        \
        (void)acpCallstackUnwind(&(aCallstack));        \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return (void *)
        (((acp_ulong_t)aCallstack->mCurrContext.pcoffset) & (~3));
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    U_get_previous_frame_x(&aCallstack->mCurrContext,
                           &aCallstack->mPrevContext,
                           sizeof(aCallstack->mCurrContext));

    aCallstack->mCurrContext.frame_size = aCallstack->mPrevContext.frame_size;
    aCallstack->mCurrContext.sp         = aCallstack->mPrevContext.sp;
    aCallstack->mCurrContext.pcspace    = aCallstack->mPrevContext.pcspace;
    aCallstack->mCurrContext.pcoffset   = aCallstack->mPrevContext.pcoffset;
    aCallstack->mCurrContext.dp         = aCallstack->mPrevContext.dp;

    if (aCallstack->mCurrContext.pcoffset)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
    void *sPC = (void *)
        (WideOrNarrowSSReg32(&aContext->uc_mcontext, ss_pcoq_head) & (~3));

    ACP_CALLSTACK_GETLOCALCONTEXT((*aCallstack));

    while (acpCallstackGetPC(aCallstack) != sPC)
    {
        if (acpCallstackUnwind(aCallstack) != ACP_TRUE)
        {
            return;
        }
        else
        {
            /* do nothing */
        }
    }
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}

#endif /* defined(ACP_CFG_COMPILE_64BIT) */

#endif

#elif defined(ALTI_CFG_OS_LINUX)

#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext) \
    do                                          \
    {                                           \
        (void)getcontext(&(aContext));          \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return aCallstack->mPC;
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
#if defined(ACP_CFG_COMPILE_64BIT) && !defined(ALINT)
    if(aCallstack->mCursor >= aCallstack->mNoFrames)
    {
        return ACP_FALSE;
    }
    else
    {
        aCallstack->mPC = aCallstack->mFrames[aCallstack->mCursor];
        aCallstack->mCursor++;
        return ACP_TRUE;
    }
#else
    aCallstack->mPC = aCallstack->mFP[1];
    aCallstack->mFP = aCallstack->mFP[0];

    if ((aCallstack->mFP != NULL) && (aCallstack->mPC != NULL))
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
#endif
}

ACP_INLINE void acpCallstackInitFromLC(acp_callstack_t          *aCallstack,
                                       ACP_CALLSTACK_LOCALCTX_T *aContext)
{
    acpCallstackInitFromSC(aCallstack, aContext);
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
#if defined(ACP_CFG_COMPILE_64BIT) && !defined(ALINT)
    /* BUGBUG
     * Must remove backtrace later
     * Remember : Only SIGABRT makes call stack mess.
     */
    ACP_UNUSED(aContext);
    aCallstack->mNoFrames = backtrace(aCallstack->mFrames,
                                      ACP_CALLSTACK_MAXDEPTH);
    if(aCallstack->mNoFrames > ACP_CALLSTACK_MAXDEPTH)
    {
        aCallstack->mNoFrames = ACP_CALLSTACK_MAXDEPTH;
    }
    else
    {
        /* Do nothing */
    }
    aCallstack->mCursor = 0;
    /*
    aCallstack->mFP = (void **)aContext->uc_mcontext.gregs[REG_RBP];
    aCallstack->mPC = (void *)aContext->uc_mcontext.gregs[REG_RIP];
    */
#else
    aCallstack->mFP = (void **)aContext->uc_mcontext.gregs[REG_EBP];
    aCallstack->mPC = (void *)aContext->uc_mcontext.gregs[REG_EIP];
#endif
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}
#elif defined(ALTI_CFG_OS_FREEBSD)
#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext) \
    do                                          \
    {                                           \
        (void)getcontext(&(aContext));          \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{       
    return aCallstack->mPC;
}    
 
ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    aCallstack->mPC = aCallstack->mFP[1];
    aCallstack->mFP = aCallstack->mFP[0];

    if ((aCallstack->mFP != NULL) && (aCallstack->mPC != NULL))
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

ACP_INLINE void acpCallstackInitFromLC(acp_callstack_t          *aCallstack,
                                       ACP_CALLSTACK_LOCALCTX_T *aContext)
{
    acpCallstackInitFromSC(aCallstack, aContext);
}


void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
    aCallstack->mFP = (void **)aContext->uc_mcontext.mc_ebp;
    aCallstack->mPC = (void *)aContext->uc_mcontext.mc_eip;
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}


#elif defined(ALTI_CFG_OS_DARWIN)
#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext) \
    do                                          \
    {                                           \
        (void)getcontext(&(aContext));          \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{       
    return aCallstack->mPC;
}    
 
ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    aCallstack->mPC = aCallstack->mFP[1];
    aCallstack->mFP = aCallstack->mFP[0];

    if ((aCallstack->mFP != NULL) && (aCallstack->mPC != NULL))
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

ACP_INLINE void acpCallstackInitFromLC(acp_callstack_t          *aCallstack,
                                       ACP_CALLSTACK_LOCALCTX_T *aContext)
{
    acpCallstackInitFromSC(aCallstack, aContext);
}


void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    aCallstack->mFP = (void **)aContext->uc_mcontext->__ss.__ebp;
    aCallstack->mPC = (void *)aContext->uc_mcontext->__ss.__eip;
#else
    aCallstack->mFP = (void **)aContext->uc_mcontext->__ss.__rbp;
    aCallstack->mPC = (void *)aContext->uc_mcontext->__ss.__rip;
#endif
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}



#elif defined(ALTI_CFG_OS_SOLARIS)

#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext) \
    do                                          \
    {                                           \
        (void)getcontext(&(aContext));          \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return aCallstack->mPC;
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    aCallstack->mPC = (void *)aCallstack->mFP->fr_savpc;

    if (aCallstack->mFP->fr_savfp)
    {
        aCallstack->mFP = (struct frame *)
            ((uintptr_t)aCallstack->mFP->fr_savfp + STACK_BIAS);
    }
    else
    {
        /* do nothing */
    }

    if ((aCallstack->mFP != NULL) && (aCallstack->mPC != NULL))
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

ACP_INLINE void acpCallstackInitFromLC(acp_callstack_t          *aCallstack,
                                       ACP_CALLSTACK_LOCALCTX_T *aContext)
{
    acpCallstackInitFromSC(aCallstack, aContext);
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
#if defined(ALTI_CFG_CPU_SPARC)
    aCallstack->mFP = (struct frame *)
        ((uintptr_t)aContext->uc_mcontext.gregs[REG_SP] + STACK_BIAS);
    aCallstack->mPC = (void *)aContext->uc_mcontext.gregs[REG_PC]; /* REG_nPC */
#elif defined(ACP_CFG_COMPILE_64BIT)
    aCallstack->mFP = (struct frame *)
        ((uintptr_t)aContext->uc_mcontext.gregs[REG_RBP] + STACK_BIAS);
    aCallstack->mPC = (void *)aContext->uc_mcontext.gregs[REG_RIP];
#else
    aCallstack->mFP = (struct frame *)
        ((uintptr_t)aContext->uc_mcontext.gregs[EBP] + STACK_BIAS);
    aCallstack->mPC = (void *)aContext->uc_mcontext.gregs[EIP];
#endif
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}

#elif defined(ALTI_CFG_OS_TRU64)

#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext) \
    do                                          \
    {                                           \
        (void)exc_capture_context(&(aContext)); \
    } while (0)

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return (void *)aCallstack->mContext->sc_pc;
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    exc_virtual_unwind(NULL, aCallstack->mContext);

    return aCallstack->mContext->sc_pc ? ACP_TRUE : ACP_FALSE;
}

ACP_INLINE void acpCallstackInitFromLC(acp_callstack_t          *aCallstack,
                                       ACP_CALLSTACK_LOCALCTX_T *aContext)
{
    aCallstack->mContext = aContext;
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
    aCallstack->mContext = &aContext->uc_mcontext;
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}

#elif defined(ALTI_CFG_OS_WINDOWS)

#if defined(_M_IX86)

#define ACP_CALLSTACK_IMAGETYPE IMAGE_FILE_MACHINE_I386
#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext)         \
    do                                                  \
    {                                                   \
        acpMemSet(&(aContext), 0, sizeof(CONTEXT));     \
        (aContext).ContextFlags = CONTEXT_FULL;         \
                                                        \
        __asm { call x }                                \
        __asm { x: pop eax }                            \
        __asm { mov (aContext).Eip, eax }               \
        __asm { mov (aContext).Ebp, ebp }               \
        __asm { mov (aContext).Esp, esp }               \
    } while (0)

#elif defined(_M_AMD64)

#define ACP_CALLSTACK_IMAGETYPE IMAGE_FILE_MACHINE_AMD64
#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext)         \
    do                                                  \
    {                                                   \
        acpMemSet(&(aContext), 0, sizeof(CONTEXT));     \
        (aContext).ContextFlags = CONTEXT_FULL;         \
        RtlCaptureContext(&(aContext));                 \
    } while (0)

#elif defined(_M_IA64)

#define ACP_CALLSTACK_IMAGETYPE IMAGE_FILE_MACHINE_IA64
#define ACP_CALLSTACK_GETLOCALCONTEXT(aContext)         \
    do                                                  \
    {                                                   \
        acpMemSet(&(aContext), 0, sizeof(CONTEXT));     \
        (aContext).ContextFlags = CONTEXT_FULL;         \
        RtlCaptureContext(&(aContext));                 \
    } while (0)

#endif

ACP_INLINE void *acpCallstackGetPC(acp_callstack_t *aCallstack)
{
    return (void *)aCallstack->mStackFrame.AddrPC.Offset;
}

ACP_INLINE acp_bool_t acpCallstackUnwind(acp_callstack_t *aCallstack)
{
    if (StackWalk64(ACP_CALLSTACK_IMAGETYPE,
                    GetCurrentProcess(),
                    GetCurrentThread(),
                    &aCallstack->mStackFrame,
                    &aCallstack->mContext,
                    NULL,
                    SymFunctionTableAccess64,
                    SymGetModuleBase64,
                    NULL))
    {
        return aCallstack->mStackFrame.AddrPC.Offset ? ACP_TRUE : ACP_FALSE;
    }
    else
    {
        return ACP_FALSE;
    }
}

ACP_INLINE void acpCallstackInitFromLC(acp_callstack_t          *aCallstack,
                                       ACP_CALLSTACK_LOCALCTX_T *aContext)
{
    acpCallstackInitFromSC(aCallstack, aContext);
}

void acpCallstackInitFromSC(acp_callstack_t           *aCallstack,
                            ACP_CALLSTACK_SIGNALCTX_T *aContext)
{
    acpMemCpy(&aCallstack->mContext, aContext, sizeof(CONTEXT));
    acpMemSet(&aCallstack->mStackFrame, 0, sizeof(STACKFRAME64));

#if defined(_M_IX86)
    aCallstack->mStackFrame.AddrPC.Offset     = aContext->Eip;
    aCallstack->mStackFrame.AddrPC.Mode       = AddrModeFlat;
    aCallstack->mStackFrame.AddrFrame.Offset  = aContext->Ebp;
    aCallstack->mStackFrame.AddrFrame.Mode    = AddrModeFlat;
    aCallstack->mStackFrame.AddrStack.Offset  = aContext->Esp;
    aCallstack->mStackFrame.AddrStack.Mode    = AddrModeFlat;
#elif defined(_M_AMD64)
    aCallstack->mStackFrame.AddrPC.Offset     = aContext->Rip;
    aCallstack->mStackFrame.AddrPC.Mode       = AddrModeFlat;
    aCallstack->mStackFrame.AddrFrame.Offset  = aContext->Rbp;
    aCallstack->mStackFrame.AddrFrame.Mode    = AddrModeFlat;
    aCallstack->mStackFrame.AddrStack.Offset  = aContext->Rsp;
    aCallstack->mStackFrame.AddrStack.Mode    = AddrModeFlat;
#elif defined(_M_IA64)
    aCallstack->mStackFrame.AddrPC.Offset     = aContext->StIIP;
    aCallstack->mStackFrame.AddrPC.Mode       = AddrModeFlat;
    aCallstack->mStackFrame.AddrFrame.Offset  = 0;
    aCallstack->mStackFrame.AddrFrame.Mode    = AddrModeFlat;
    aCallstack->mStackFrame.AddrStack.Offset  = aContext->IntSp;
    aCallstack->mStackFrame.AddrStack.Mode    = AddrModeFlat;
    aCallstack->mStackFrame.AddrBStore.Offset = aContext->RsBSP;
    aCallstack->mStackFrame.AddrBStore.Mode   = AddrModeFlat;
#endif

    (void)acpCallstackUnwind(aCallstack);
}

void acpCallstackFinal(acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);
}

#endif


acp_rc_t acpCallstackTraceInternal(acp_callstack_t          *aCallstack,
                                   acp_callstack_callback_t *aCallback,
                                   void                     *aCallbackData)
{
#if defined(ACP_CALLSTACK_LOCALCTX_T)
    ACP_CALLSTACK_LOCALCTX_T  sLocalContext;
#endif
    acp_callstack_t           sLocalCallstack;
    acp_callstack_t          *sCallstack = NULL;
    acp_callstack_frame_t     sFrame;
    acp_sym_table_t           sSymTable;
    acp_rc_t                  sRC;
    acp_bool_t                sSymbolLookup;

    /*
     * [1.1] callstack initialization
     */
    if (aCallstack == NULL)
    {
#if defined(ACP_CALLSTACK_LOCALCTX_T)
        ACP_CALLSTACK_GETLOCALCONTEXT(sLocalContext);

        acpCallstackInitFromLC(&sLocalCallstack, &sLocalContext);
#else
        ACP_CALLSTACK_GETLOCALCONTEXT(sLocalCallstack);
#endif

        sCallstack = &sLocalCallstack;
    }
    else
    {
        sCallstack = aCallstack;
    }

    /*
     * [1.2] symbol lookup initialization
     */
    acpSymInfoInit(&sFrame.mSymInfo);

    sRC = acpSymTableInit(&sSymTable);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        sSymbolLookup = ACP_TRUE;
    }
    else
    {
        sSymbolLookup = ACP_FALSE;
    }

    /*
     * [2] callstack tracing
     */
    sFrame.mIndex = 0;
    sRC           = ACP_RC_SUCCESS;

    while (1)
    {
        sFrame.mAddress = acpCallstackGetPC(sCallstack);

        if (sSymbolLookup == ACP_TRUE)
        {
            (void)acpSymFind(&sSymTable, &sFrame.mSymInfo, sFrame.mAddress);
        }
        else
        {
            /* do nothing */
        }

        if ((*aCallback)(&sFrame, aCallbackData) != 0)
        {
            sRC = ACP_RC_ECANCELED;
            break;
        }
        else
        {
            /* do nothing */
        }

        if (acpCallstackUnwind(sCallstack) != ACP_TRUE)
        {
            break;
        }
        else
        {
            /* do nothing */
        }

        sFrame.mIndex++;
    }

    /*
     * [3.1] symbol lookup finalization
     */
    if (sSymbolLookup == ACP_TRUE)
    {
        (void)acpSymTableFinal(&sSymTable);
    }
    else
    {
        /* do nothing */
    }

    acpSymInfoFinal(&sFrame.mSymInfo);

    /*
     * [3.2] callstack finalization
     */
    if (aCallstack == NULL)
    {
        acpCallstackFinal(sCallstack);
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}
