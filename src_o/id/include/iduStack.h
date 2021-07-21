/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduStack.h 89766 2021-01-14 06:45:10Z kclee $
 ****************************************************************************/

#ifndef _O_IDE_STACK_H_
#define _O_IDE_STACK_H_ 1

#include <idl.h>
#include <ide.h>

#define IDU_DUMPSTACKS_PREFIX           "[DUMPSTACKS]"
#define IDU_DUMPSTACKS_PREFIX_LEN       idlOS::strlen(IDU_DUMPSTACKS_PREFIX)


#define IDU_MAX_CALL_DEPTH (1024)

#define IDU_SET_BUCKET      iduStack::setBucketList
#define IDU_CLEAR_BUCKET    ideStack::clearBucketList

typedef void iduHandler(int, struct siginfo*, void*);

#if defined(ALTI_CFG_OS_WINDOWS)
typedef BOOL (__stdcall* iduSymInit)(HANDLE, PSTR, BOOL);

typedef BOOL (__stdcall *iduStackWalker)(
        DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
        PREAD_PROCESS_MEMORY_ROUTINE64,
        PFUNCTION_TABLE_ACCESS_ROUTINE64,
        PGET_MODULE_BASE_ROUTINE64,
        PTRANSLATE_ADDRESS_ROUTINE64);
#else
typedef struct iduSignalDef
{
    SInt            mNo;        /* Signal number */
    SChar           mName[64];  /* Signal name */
    SChar           mDesc[128]; /* Signal description */
    UInt            mFlags;     /* Signal flags */
    iduHandler*     mFunc;      /* Signal handler function */
} iduSignalDef;
#endif

#define IDU_SIGNUM_DUMP_CALLSTACKS   SIGRTMIN  /*34 in linux*/

class iduStack
{
public:
    static IDE_RC   initializeStatic(void);
    static void     setBucketList(ideLogModule, const SChar*, ...);
    static void     clearBucketList(ideLogModule);

    static IDE_RC   setSigStack(void);

#if defined(ALTI_CFG_OS_WINDOWS)
    static void __cdecl dumpStack( const CONTEXT* = NULL,
                                   SChar*         = NULL );
#else
    static void     dumpStack( const iduSignalDef*   = NULL,
                               siginfo_t*            = NULL,
                               ucontext_t*           = NULL,
                               idBool                = ID_FALSE, /* PROJ-2617 */
                               SChar*                = NULL,
                               idBool                = ID_FALSE); /*it's only for Linux.(BUG-45982) */
#endif

    static IDE_RC dumpAllCallstack();

private:
    volatile static SInt    mCallStackCriticalSection;
    static const SChar      mDigitChars[17];

    static IDTHREAD SInt    mCallDepth;
    static IDTHREAD void*   mCallStack[IDU_MAX_CALL_DEPTH];
    static PDL_HANDLE       mFD;

    static void convertHex64(ULong, SChar*, SChar**);
    static void convertDec32(UInt , SChar*, SChar**);
    static void writeBucket(SChar*,UInt*);
    static void writeDec32(SChar*,UInt*,UInt);
    static void writeHex32(SChar*,UInt*,UInt);
    static void writeHex64(SChar*,UInt*,ULong);
    static void writeHexPtr(SChar*,UInt*,void*);
    static void writeString(SChar*,UInt*,const void*, size_t);
    static void copyToBuff(SChar*,UInt*,const void*, size_t);/*BUG-48433*/

#if defined(ALTI_CFG_OS_WINDOWS)
    static iduSymInit                           mSymInitFunc;
    static iduStackWalker                       mWalkerFunc;

    static HMODULE                              mHModule;
    static PFUNCTION_TABLE_ACCESS_ROUTINE64     mAccessFunc;
    static PGET_MODULE_BASE_ROUTINE64           mGetModuleFunc;
#else

    static IDTHREAD SChar                       mSigAltStack[SIGSTKSZ];

# if defined(ALTI_CFG_OS_SOLARIS)
    static int walker(uintptr_t, int, void*);
# elif defined(ALTI_CFG_OS_LINUX) && !defined(ALTI_CFG_CPU_POWERPC)
    static SInt     tracestack(void**, SInt);
# else
# endif
#endif
};

#endif
