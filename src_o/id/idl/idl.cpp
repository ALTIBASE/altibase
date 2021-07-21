/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idl.cpp 89585 2020-12-18 01:46:27Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <iduStack.h>
#include <iduProperty.h>
#include <idErrorCode.h>
#include <acp.h>

#ifndef BUILD_FOR_UTIL
#include <ideLogEntry.h>
#endif

#if defined(VC_WIN32)
#  if !defined(VC_WINCE)
#    include <iphlpapi.h>
#  endif /* VC_WINCE */
#    include <stdlib.h>
#    include <stdio.h>
#    include <time.h>
#  if defined(USE_OLD_GETMACADDR)
#    include <Nb30.h>


#ifdef  defined(HP_HPUX) || defined(IA64_HP_HPUX)
#include <sys/mpctl.h>
#endif


// For get mac address
typedef struct _ASTAT_
{
    ADAPTER_STATUS adapt;
    NAME_BUFFER NameBuf[30];
} ASTAT, *PASTAT;

ASTAT Adapter;
#  else
#    if !defined(VC_WINCE)
#    include <lm.h>
#    endif /* VC_WINCE */
#  endif /* USE_OLD_GETMACADDR */
#elif defined(WRS_VXWORKS)
#    include <netinet/if_ether.h>
#elif defined(ITRON)
#else
#    include <unistd.h>
#endif

#if !defined(VC_WINCE)
#include <sys/types.h>
#include <sys/stat.h>
#endif /* VC_WINCE */

#if defined(SPARC_SOLARIS) || defined(X86_SOLARIS)
#include <kstat.h>
#include <kvm.h>
#include <sys/statvfs.h>
#include <sys/times.h>
#endif // SPARC_SsOLARIS

#if defined( HP_HPUX) || defined(IA64_HP_HPUX)
#include <sys/pstat.h>
#include <sys/vfs.h>
#include <sys/times.h>
#endif // HP_HPUX

#ifdef DEC_TRU64
//#include <sys/ustat.h>
#include <sys/statvfs.h>
#include <sys/termios.h>
#include <sys/times.h>
#endif // DEC_TRU64

#if defined(INTEL_LINUX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(ALPHA_LINUX) || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) || defined(WRS_LINUX)
#include <sys/vfs.h>
#include <sys/times.h>
#endif // INTEL_LINUX || IA64_LINUX || ALPHA_LINUX

#ifdef NTO_QNX
#include <sys/statvfs.h>
#include <sys/times.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#endif // NTO_QNX

#ifdef CYGWIN32
#include <sys/statfs.h>
#endif // CYGWIN32

#ifdef IBM_AIX
#include <sys/statfs.h>
#include <sys/times.h>
#endif // IBM_AIX

#if defined(X86_64_DARWIN)
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <sys/times.h>
#endif

#if !defined(VC_WIN32)
# if !defined(IBM_AIX) && !defined(NTO_QNX)
# if ( defined(SPARC_SOLARIS) || defined(X86_SOLARIS) ) && (OS_MAJORVER == 2) && (OS_MINORVER == 5)
#    include <sys/swap.h>
# elif defined(ITRON)
/* empty */
# else /* ( defined(SPARC_SOLARIS) || defined(X86_SOLARIS) ) && (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
#   if !defined(CYGWIN32) && !defined(WRS_VXWORKS) && !defined(ANDROID) && !defined(SYMBIAN) && !defined(X86_64_DARWIN)
#     include <sys/sysinfo.h>
#     include <sys/swap.h>
#   endif
# endif /* ( defined(SPARC_SOLARIS) || defined(X86_SOLARIS) ) && (OS_MAJORVER == 2) && (OS_MINORVER == 5) */
# endif /* IBM_AIX && !defined(NTO_QNX) */
#endif /* VC_WIN32 */

#if defined(WRS_VXWORKS)
extern "C" int vsnprintf
(
    char        *buf,
    int          siz,
    const char  *fmt,
    va_list     list
);
#endif

#if defined(SYMBIAN)
#include <sys/times.h>
#endif

#if defined(VC_WINCE)
#include <IPHlpApi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

SInt idlVA::getSystemMemory(ULong *PageSize,
                            ULong *MaxMem, ULong *AvailMem, ULong *FreeMem,
                            ULong *SwapMaxMem, ULong *SwapFreeMem)
{
    /* 초기화 */
    *PageSize    = 0;
    *MaxMem      = 0;
    *AvailMem    = 0;
    *FreeMem     = 0;
    *SwapMaxMem  = 0;
    *SwapFreeMem = 0;

#if defined(SPARC_SOLARIS) || defined(X86_SOLARIS)
    /* ------------------------------
     * 메인 메모리 크기를 구한다.
     * -----------------------------*/

    /* ----------------
     * [1] 변수 선언
     * ---------------*/

    kstat_t       *ks = NULL;
    kstat_ctl_t   *kc = NULL;
    kstat_named_t *kn = NULL;

    /* ----------------
     * [2] PAGESIZE 얻기
     * ---------------*/
    *PageSize = (ULong)idlOS::sysconf(_SC_PAGESIZE);

    /* ----------------
     * [3] Max Memory
     * ---------------*/
    *MaxMem = idlOS::sysconf(_SC_PHYS_PAGES)  * (*PageSize);

    kc = kstat_open();
    ks = kstat_lookup(kc, (SChar *)"unix", 0, (SChar *)"system_pages");
    if (kstat_read(kc, ks, 0) == -1)
    {
        kstat_close(kc);
        return -1;
    }
    /* ----------------
     * [4] Available Memory
     * ---------------*/
    kn = (kstat_named_t *)kstat_data_lookup(ks, (SChar *)"availrmem");
    if (kn)
    {
        *AvailMem =  kn->value.ul * (*PageSize);
    }

    /* ----------------
     * [5] Free Memory
     * ---------------*/
    kn = (kstat_named_t *)kstat_data_lookup(ks, (SChar *)"freemem");
    if (kn)
    {
        *FreeMem = kn->value.ul * (*PageSize);
    }
    kstat_close(kc);


    /* ------------------------------
     * SWAP 크기를 구한다.
     * -----------------------------*/

    /* ----------------
     * [1] 변수 선언
     * ---------------*/
    struct anoninfo anon;
    SInt swtotal = 0;
    SInt swfr;

    idlOS::memset(&anon, 0, sizeof(anoninfo));

    /* ----------------
     * [2] SWAP 정보 얻기
     * ---------------*/
    if (swapctl(SC_AINFO, &anon) != -1)
    {
        swtotal = anon.ani_max;
        swfr = anon.ani_max - anon.ani_resv;
    }
    else
    {
        return -1;
    }
    *SwapMaxMem  = swtotal * (*PageSize);
    *SwapFreeMem = swfr    * (*PageSize);
    return 0;

#else /* Not solaris... */
#    if defined (VC_WIN32)
    SYSTEM_INFO SystemInformation;
    GetSystemInfo( &SystemInformation );    // can't fail
    *PageSize    = SystemInformation.dwPageSize;
    *MaxMem      = 1024*1024*1024;
    *AvailMem    = 1024*1024*1024;
    *FreeMem     = 1024*1024*1024;
    *SwapMaxMem  = 1024*1024*1024;
    *SwapFreeMem = 1024*1024*1024;
    return 0;
#    endif
    return -1;
#endif
}

// 시스템에 설치된 CPU 갯수를 구한다.
SInt idlVA::getProcessorCount()
{
    SInt cntCPU = 0;
#if defined(SPARC_SOLARIS) || defined (X86_SOLARIS) || defined(IBM_AIX)
    cntCPU = (SInt)idlOS::sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(DEC_TRU64)
    cntCPU = (SInt)idlOS::sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(INTEL_LINUX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(ALPHA_LINUX) || defined(NTO_QNX) || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) || defined(ELDK_LINUX) || defined(MIPS64_LINUX) || defined(WRS_LINUX) || defined(X86_64_DARWIN)
    cntCPU = (SInt)idlOS::sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(VC_WIN32)
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    cntCPU = systemInfo.dwNumberOfProcessors;
#elif defined(HP_HPUX) || defined(IA64_HP_HPUX)
    // BUGBUG : by gamestar : 나중에 적용 고려
    // source from st library : server.c
    //#include <sys/mpctl.h>
    //n = mpctl(MPC_GETNUMSPUS, 0, 0);
    struct pst_dynamic store_pst_dynamic;

    pstat_getdynamic( &store_pst_dynamic, sizeof(store_pst_dynamic), 1, 0 );
    /* BUG-21067: HPUX에서 CPU갯수를 가져올 경우에 Active Processor개수를 가져와야
     *            합니다. */
    cntCPU = (SInt)store_pst_dynamic.psd_proc_cnt;
#elif defined(ITRON)
    /* empty */
    cntCPU = 1;
#else
#  if !defined (CYGWIN32)
    //idlOS::printf("oops!! warning...did you check this?(%s:%d) \n",
    //              __FILE__, __LINE__);
#  endif
    cntCPU = 1; // default는 1로 함.
#endif
    return ( cntCPU > 1 ? cntCPU : (SInt)1 );
}

/*
 * BUG-44958  Physical core count is needed 
 *
 * 시스템에 설치된 physical core 갯수를 구한다.
 * 
 * Supported System based on "제품별 지원 OS 현황(2017.6.8)"
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * AIX   / PowerPC
 * Linux / X86
 * Linux / PowerPC
 * HP    / IA64(itanium)
 * 
 * 지원하지 않는 시스템이거나 정보를 얻지 못하게 되면 기존의 논리코어갯수를 리턴하도록 한다.
 */
SInt idlVA::getPhysicalCoreCount()
{

#if  defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(IBM_AIX)
  //AIX/PowerPC , Linux/PowerPC
  FILE  *fp;
  SChar  sBuf[1024];
  SChar *sPtr= NULL;
  SInt   sNCores = ((SInt)idlOS::sysconf(_SC_NPROCESSORS_ONLN)); //just in case 

  fp = popen("lparstat -i | grep \"Online Virtual CPUs\"", "r");
  if (fp != NULL) {
      while (fgets(sBuf, sizeof(sBuf), fp) != NULL) {
            sPtr=strstr(sBuf,":");
            if(sPtr != NULL )
            {
                    sNCores =  idlOS::atoi(sPtr + 2); //BUG-46543
            }
      }
      pclose(fp);
  }

#elif defined(INTEL_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) 
  //Linux/X86
  FILE *fp;
  SChar sBuf[1024];
  SChar*sPtr= NULL;
  SInt  sNCores = ((SInt)idlOS::sysconf(_SC_NPROCESSORS_ONLN));//just in case

  fp = popen("grep -m 1 flags  /proc/cpuinfo", "r");
  if (fp != NULL) 
  {
      while (fgets(sBuf, sizeof(sBuf), fp) != NULL) 
      {
            sPtr=strstr(sBuf," ht ");
            if(sPtr != NULL )
            {
                //intel always 2 per core
                if( sNCores > 1 ) 
                {
                    sNCores /= 2;
                }
                else
                {
                    sNCores = 1;
                }
            }
      }
      pclose(fp);
  }

#elif defined(HP_HPUX) || defined(IA64_HP_HPUX)
    //HP/IA64(itanium)
    SInt sNCores = mpctl(MPC_GETNUMCORES_SYS,0,0);
#else
    //for unsupported platforms
    SInt sNCores = idlVA::getProcessorCount()
#endif
    return ( sNCores > 1 ? sNCores : 1 );
}


#ifdef DEC_TRU64
IDL_EXTERN_C int statvfs(const char *, struct statvfs *);
#endif // DEC_TRU64

// path로 지정된 파일시스템의 가용 디스크 공간을 구한다
// RETURN : free space (byte)
SLong idlVA::getDiskFreeSpace (const SChar *path)
{
#if defined(SPARC_SOLARIS) || defined(X86_SOLARIS) || defined(DEC_TRU64) || defined(NTO_QNX)
    struct  statvfs buf;

    if (statvfs (path, &buf) == -1 )
    {
        return 0;
    }

    return (((SLong)buf.f_bavail * buf.f_frsize));
#elif defined( VC_WIN32 )
    // path가 파일인지 디렉토리인지 알아본다.
    BOOL bFileIsDirectory = FALSE;
#if defined (VC_WINCE)
    wchar_t *sPath;
    wchar_t *sDirPath;
    sPath = PDL_Ascii_To_Wide::convert(path);
    HANDLE hFile = CreateFile( sPath,
                               0,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    delete []sPath;
#else
    HANDLE hFile = CreateFile( path,
                               0,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
#endif /* VC_WINCE */

    if (hFile != INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION fi;
        if ( !GetFileInformationByHandle( hFile, &fi) ) {
            return -1;
        }
        if ( fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
            bFileIsDirectory = TRUE;
        }

        if (!CloseHandle( hFile ))
        {
            return -1;
        }
    }

    // local copy 생성
    SChar *dirPath = new SChar[ strlen(path) + 1 ];
    if ( dirPath == NULL )
    {
        return -1;
    }
    strcpy( dirPath, path );

    if ( !bFileIsDirectory )
    {
        // path를 directory로 만들어준다.
        // c:\altibase_home\logs\log0  -> c:\altibase_home\logs
        for (SInt i=strlen(dirPath); i>0; i--) {
            if ( dirPath[i-1] == IDL_FILE_SEPARATOR ) {
                dirPath[i-1] = '\0';
                break;
            }
            dirPath[i-1] = '\0';
        }
    }

    ULARGE_INTEGER lFreeBytesAvailableToCaller;
    ULARGE_INTEGER lTotalNumberOfBytes;
    ULARGE_INTEGER lTotalNumberOfFreeBytes;
#if defined (VC_WINCE)
    sDirPath = PDL_Ascii_To_Wide::convert(dirPath);
    if (!GetDiskFreeSpaceEx(sDirPath,
                            &lFreeBytesAvailableToCaller,
                            &lTotalNumberOfBytes,
                            &lTotalNumberOfFreeBytes) ) {
        delete []sDirPath;
        delete []dirPath;
        return 0;
    }
    delete []sDirPath;
    delete []dirPath;
#else
    if (!GetDiskFreeSpaceEx(dirPath,
                            &lFreeBytesAvailableToCaller,
                            &lTotalNumberOfBytes,
                            &lTotalNumberOfFreeBytes) ) {
        delete []dirPath;
        return 0;
    }

    delete []dirPath;
#endif /* VC_WINCE */
    return (SLong)lFreeBytesAvailableToCaller.QuadPart;
#elif defined(ITRON) || defined(ARM_LINUX) || defined(ELDK_LINUX) || defined(MIPS64_LINUX) || defined(SYMBIAN)
    return 0;
#else
    struct  statfs  buf;

    if (statfs ((SChar*)path, &buf) == -1)
    {
        return 0;
    }

    return (((SLong)buf.f_bavail * buf.f_bsize));
#endif
}

#if defined(DEC_TRU64) && (OS_MAJORVER <= 4)
//===================================================================
// To Fix PR-17754
// 일부 플랫폼에 없는 vsnprintf() 구현을 위한 내부 함수
//===================================================================

/*

@deftypefn Extension SInt vasprintf (SChar** @var{aResult}, const SChar* @var{aFormat}, va_list @var{aArgs})

Like @code{vsprintf}, but instead of passing a pointer to a buffer,
you pass a pointer to a pointer.  This function will compute the size
of the buffer needed, allocate memory with @code{malloc}, and store a
pointer to the allocated memory in @code{*@var{aResult}}.  The value
returned is the same as @code{vsprintf} would return.  If memory could
not be allocated, minus one is returned and @code{NULL} is stored in
@code{*@var{aResult}}.

@end deftypefn

*/

SInt
idlOS::int_vasprintf( SChar**      aResult,
                      const SChar* aFormat,
                      va_list      aArgs )
{
    const SChar* sP;
    SInt         sSIntValue;
    SInt         sTotalWidth;
    va_list      sAp;

    sP = aFormat;
    /* Add one to make sure that it is never zero, which might cause malloc
       to return NULL. */
    sTotalWidth = (SInt)strlen( aFormat ) + 1;
    memcpy( (void*)&sAp, (void*)&aArgs, ID_SIZEOF(va_list) );

    while ( *sP != '\0' )
    {
        if ( *sP++ == '%' )
        {
            while ( strchr( "-+ #0", *sP ) )
            {
                ++sP;
            }
            if ( *sP == '*' )
            {
                ++sP;
                sSIntValue = (SInt)va_arg( sAp, int );
                if ( sSIntValue < 0 )
                {
                    sSIntValue = -sSIntValue;
                }
                sTotalWidth += sSIntValue;
            }
            else
            {
                sTotalWidth += (SInt)strtoul( sP, (SChar**)&sP, 10 );
            }
            if ( *sP == '.' )
            {
                ++sP;
                if ( *sP == '*' )
                {
                    ++sP;
                    sSIntValue = (SInt)va_arg( sAp, int );
                    if ( sSIntValue < 0 )
                    {
                        sSIntValue = -sSIntValue;
                    }
                    sTotalWidth += sSIntValue;
                }
                else
                {
                    sTotalWidth += (SInt)strtoul( sP, (SChar**)&sP, 10 );
                }
            }
            while ( strchr( "hlL", *sP ) )
            {
                sP++;
            }
            /* Should be big enough for any format specifier
               except %s and floats. */
            sTotalWidth += 30;
            switch ( *sP )
            {
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                case 'c':
                    (void)va_arg( sAp, int );
                    break;
                case 'f':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                    (void)va_arg( sAp, double );
                    /* Since an ieee double can have an exponent of 307, we'll
                       make the buffer wide enough to cover the gross case. */
                    sTotalWidth += 307;
                    break;
                case 's':
                    sTotalWidth += (SInt)strlen( va_arg( sAp, char* ) );
                    break;
                case 'p':
                case 'n':
                    (void)va_arg( sAp, char* );
                    break;
            }
            sP++;
        }
    }
    *aResult = (SChar*)malloc( (UInt)sTotalWidth );
    if ( *aResult != NULL )
    {
        return vsprintf( *aResult, aFormat, aArgs );
    }
    else
    {
        return -1;
    }
}

SInt
idlOS::vasprintf( SChar**      aResult,
                  const SChar* aFormat,
                  va_list      aArgs )
{
    return int_vasprintf( aResult, aFormat, aArgs );
}

#endif /* (DEC_TRU64) && (OS_MAJORVER <= 4) */

SInt
idlOS::vsnprintf(char *buf, size_t size, const char *format, va_list ap)
{
#if defined(DEC_TRU64) && (OS_MAJORVER <= 4)
//===================================================================
// To Fix PR-17754
// 일부 플랫폼에 없는 vsnprintf()의 구현
//===================================================================

/* This implementation relies on a working vasprintf.  */

    SChar* sBuf;
    SInt   sResult;

    sBuf = NULL;
    sResult = vasprintf( &sBuf, format, ap );

    if ( sBuf == NULL )
    {
        return -1;
    }
    if ( sResult < 0 )
    {
        free( sBuf );
        return -1;
    }

    sResult = (SInt)strlen( sBuf );
    if ( size > 0 )
    {
        if ( (vSLong)size > sResult )
        {
            memcpy( buf, sBuf, (UInt)( sResult + 1 ) );
        }
        else
        {
            memcpy( buf, sBuf, size - 1 );
            buf[ size - 1 ] = '\0';

            sResult = size - 1;
        }
    }
    free( sBuf );
    return sResult;

#elif defined(VC_WIN32)
    SInt result = ::_vsnprintf (buf, size, format, ap);
    // Win32 doesn't 0-terminate the string if it overruns maxlen.
	// BUG-28671 Win32 doesn't 0-terminate the string if buffer size equal string size.
    if ( ((result == -1) || ( result >= (SInt)size ) )  && (size > 0))
    {
        buf[size - 1] = '\0';
        result = (SInt)size - 1;
    }
    return result;

#else
    /* BUG-25100: HP s[v]nprintf 종류의 함수에서 버퍼보다 작은 사이즈가 복사되는
     * 경우는 리턴 값이 -1입니다.
     *
     * snprintf, svnprintf는 format의 길이가 size보다 크거나 같은경우 format의
     * 길이를 return하고 HP는 -1을 return한다. 하지만 상위에서 format의 길이가
     * 같거나 긴경우 실제 buf에 복사된 data의 길이를 return하는 것으로 알고 있다.
     * 하여 size보다 format의 길이가 크거나 같은 경우 size - 1값을 리턴한다.
     * -1을 하는 이유는 buf의 마지막은 항상 null로 끝나기때문이다. */
    SInt result = ::vsnprintf (buf, size, format, ap);

    if( ( ( result == -1 ) || ( result >= (SInt)size ) ) && (size > 0) )
    {
        buf[size - 1] = '\0';
        result = (SInt)size - 1;
    }

    return result;
#endif
}

SInt
idlOS::snprintf(char *buf, size_t size, const char *format, ...)
{
    SInt result;
    va_list ap;

    va_start (ap, format);
    result = vsnprintf(buf, size, format, ap);
    va_end (ap);

    return result;
}


SInt
idlOS::atexit(void (*function)(void))
{
    return ::atexit(function);
}

SDouble
idlOS::strtod (const char *s, char **endptr)
{
    return ::strtod(s,endptr);
}

SChar *
idlOS::strsep(SChar **aStringp, const SChar *aDelim)
{
    register SChar *s;
    register const SChar *sPanp;
    register int c, sc;
    SChar *sTok;

    if ((s = *aStringp) != NULL)
    {

        for(sTok = s;;)
        {
            c = *s++;
            sPanp = aDelim;
            do
            {
                if ((sc = *sPanp++) == c)
                {
                    if (c == 0)
                    {
                        s = NULL;
                    }
                    else
                    {
                        *(--s)=0;
                        ++s;
                    }
                    *aStringp = s;
                    return  sTok;
                }
            } while (sc != 0);
        }
    }
    return (NULL);
};


SChar*
idlOS::strcasestr(const SChar *s, const SChar *w)
{
#define TO_LOWER(c)  ( (c>='A' && c<='Z')?(c|0x20):c )
    SChar         b, c;
    const SChar *n, *r;

    if((w == NULL) || (*w == 0))
    {
        return NULL;
    }
    if((s == NULL) || (*s == 0))
    {
        return NULL;
    }

    while(1)
    {
        /* get first match */
        for( n = w, b = TO_LOWER( (*w) ), c = TO_LOWER( (*s) )
                 ;b != c;
             ++s, c = TO_LOWER((*s)) )
        {
            if( c == '\0' )
            {
                return NULL;
            }
        }

        /* match string */
        r = s;
        do
        {

            if(*s == '\0')
            {
                return NULL;
            }

            ++n; b = TO_LOWER( (*n) );
            ++s; c = TO_LOWER( (*s) );

            if( *n == '\0' )
            {
                goto M0;
            }

        } while( c == b );

    };


  M0: return (SChar*)r;
#undef TO_LOWER
}
SInt idlOS::strToInt( UChar* aValue,
                      UInt   aLength )
{
    UInt         sTmpValue;
    SInt         sValue;
    acp_rc_t     sRC;
    acp_sint32_t sSign;
    acp_char_t  *sEnd;

    sRC = acpCStrToInt32((acp_char_t *)aValue,
                         aLength,
                         &sSign,
                         &sTmpValue,
                         10, /* only decimal */
                         &sEnd);
    assert(ACP_RC_IS_SUCCESS(sRC));

    sValue = (SInt)sTmpValue * sSign;

    return sValue;
}

SInt idlOS::uIntToStr( UInt aNumber,
                       SChar * aStr,
                       UInt   aLength )
{
    acp_rc_t     sRC;

    sRC = acpCStrUInt32ToCStr10(aNumber, (acp_char_t*)aStr, aLength);
    assert(ACP_RC_IS_SUCCESS(sRC));

    return sRC;
}

time_t
idlOS::mktime( struct tm *t )
{
    // mktime을 사용할 수 있는 범위가
    // 시스템마다 조금씩 다름.
    // id에서 공통 범위로 제약함
    // 1970-01-01 ~ 2037-12-31까지만 유효함
    if( t->tm_year < 70 || t->tm_year >= 138 )
    {
        return -1;
    }
    return PDL_OS::mktime( t );
}

SLong idlVA::fstrToLong(SChar *str, idBool *aValidity)
{
    SLong   result = 0;
    SInt    sign = 0;
    idBool  sTempValidity;
    UInt    sState = 0; /* 0 : space state 1 : digit(char) state 2 : end space state */

    if (aValidity == NULL)
    {
        aValidity = &sTempValidity;
    }

    if ( str == NULL )
    {
        *aValidity = ID_FALSE;
        return result;
    }
    *aValidity = ID_TRUE;

    for ( SInt i = 0; str[i] != 0; i++ )
    {
        // skip the white space
        if ( str[i] == ' ' || str[i] == '\t' || str[i] == '\n' )
        {
            if (sState == 1)
            {
                sState = 2; // end of input state
            }

            if (sState != 0 && sState != 2)
            {
                goto validity_error;
            }
            continue;
        }

        if (sState == 0)
        {
            sState = 1;
        }
        else
        {
            if (sState != 1)
            {
                goto validity_error;
            }
        }

        if ( sign == 0 )
        {
            if (str[i] == '-' )
            {
                sign = -1;
                continue;
            }
            else
            {
                sign = 1;
            }
        }

        if ( isdigit (str[i]) )
        {
            result = result * ID_LONG(10) + ( str[i] - '0' );
        }
        else
        {
            // Mega byte sign
            if ( str[i] == 'g' || str[i] == 'G' )
            {
                result = result * ID_LONG(1024) * ID_LONG(1024) * ID_LONG(1024);
            }
            else
            {
                if ( str[i] == 'm' || str[i] == 'M' )
                {
                    result = result * ID_LONG(1024) * ID_LONG(1024);
                }
                else
                {
                    // Kilo byte sign
                    if ( str[i] == 'k' || str[i] == 'K' )
                    {
                        result = result * ID_LONG(1024);
                    }
                    else
                    {
                        // invalid character.
                        *aValidity = ID_FALSE;
                        return 0;
                    }
                }
            }
        }
    }
    return result * sign;

  validity_error:

    // invalid character.
    *aValidity = ID_FALSE;
    return 0;

}


SInt idlVA::strnToSLong(const SChar *aStr, UInt aLen, SLong *aValue, SChar **aTail)
{
    ULong  sValue = 0;
    const  SChar *s = aStr;
    UChar  sBase, sDigit, sDigCnt;
    SChar  c,  sSign = 0;


    /* skip leading spaces */
    for(c = *s; aLen > 0 && c == ' '; s++, aLen--, c = *s)
    {
        if( c == 0)
        {
            /* empty string */
            return -1;
        }
    }

    /* at least one character mast be here */
    if(aLen < 1)
    {
        return -1;
    }

    /* fix a sign if is it */
    if( c == '+' || c == '-')
    {
        s++;
        aLen--;
        sSign  =  (c == '-') ? -1 : 1;
    }

    /* skip leading spaces */
    for( c = *s ;aLen > 0 && c == ' '; s++, aLen--, c = *s)
    {
        if(c == 0)
        {
            return -1;
        }
    }

    /* detect base */
    if( c != '0')
    {
        sBase = 10;
    }
    else
    {
        if( s[1] == 'x' || s[1] == 'X')
        {
            sBase =16;
            s    +=2;
            aLen -=2;
        }
        else
        {
            sBase =  8;
        }
    }


    /* at least single digits shold be there */
    if(aLen < 1 )
    {
        return -1;
    }

    for(sDigCnt =0,  sValue = 0, c = *s;  c != 0 && aLen > 0; s++, aLen--, c = *s)
    {
        if( isdigit(c) )
        {
            sDigit = (c - '0');
        }
        else
        {
            if( islower(c) )
            {
                sDigit = c + (10 -'a');
            }
            else
            {
                sDigit = c + (10 -'A');
            }
        }

        /* it is not reach the '0' or end of buffer */
        if(sDigit > sBase)
        {
            break;
        }

        sValue= sValue* sBase + sDigit;

        if( sValue > ID_SLONG_MAX)
        {
            break;
        }

        sDigCnt++;
    }

    if( aTail != NULL)
    {
        *aTail = (SChar*)s;
    }

    if( sSign == 0 )
    {
        *aValue = sValue;
    }
    else
    {
        *aValue = -sValue;
    }

    /* 0 - is Ok, 1 - is Warning the right truncated */
    if( c == 0 || aLen == 0 )
    {
        return 0;
    }
    else
    {
        if( sValue > ID_SLONG_MAX)
        {
            return -2;  /* Overflow the SLong */
        }

        if(sDigCnt == 0)
        {
            return -1;  /* Error - there are no significant digits      */
        }
        else
        {
            return  1; /* Wrning - there are right truncation of string */
        }
    }
}


SInt idlVA::strnToSDouble( const SChar *aStr, UInt aLen, SDouble *aValue, SChar **aTail)
{
    SDouble sValue, sDigit;
    const   SChar *s = aStr;
    SInt    sDigCnt, s10, sExp = 1;
    SChar   c, sSign = 0, sExpSign = 1;



    /* skip leading spaces */
    for(c = *s; c == ' '; )
    {
        if( aLen == 0 )
        {
            return -1;
        }
    }

    /* fix a sign if is it */
    if( c == '+' || c == '-')
    {
        s++;
        aLen--;
        sSign  =  (c == '-') ? -1 : 1;
    }

    /* skip leading spaces */
    for( c = *s ;aLen > 0 && c == ' '; c = *(++s), aLen--) ;

    if(c == 0 || aLen < 1 )
    {
        return -1;
    }

    /* mantissa */
    for(sDigCnt =0,  sValue = 0;  c != 0 && aLen > 0; c = *(++s), aLen--)
    {
        if( !isdigit(c) )
        {
            break;
        }
        sDigit = (c - '0');
        sValue= sValue*10 + sDigit;
        sDigCnt++;
    }

    /* fraction */
    if(c == '.')
    {
        c = *(++s); aLen--;
        for( s10 = 10;  isdigit(c) && aLen > 0; c = *(++s), aLen--)
        {
            sDigit = (c - '0');
            sValue += sDigit/s10;
            sDigCnt++;
            s10 *= 10;
        }
    }

    /* skip leading spaces */
    for(c = *s ;aLen > 0 && c == ' '; s++, aLen--, c = *s) ;
    if (c == 0 || aLen == 0 )
    {
        goto M1;
    }

    if( aLen != 0 && (c == 'e' || c == 'E'))
    {
        c = *(++s);  aLen--;

        for( ;aLen > 0 && c==' ' ; aLen--, c = *(++s)) ;

        if( c == '-')
        {
            sExpSign = -1;
        }
        else
        {
            sExpSign =  1;
        }

        for( ;aLen > 0 && ( c == ' ' || c == '-' || c == '+'); aLen--, c = *(++s)) ;

        for(sExp = 0 ;  isdigit(c) && aLen > 0; aLen--, c = *(++s))
        {
            sExp = sExp * 10 + (c - '0');
        }
    }


    if( sExp > 1 )
    {
        /* Exponent overflow spec */
        if(sExp > 1000000)
        {
            return -2;
        }

        /*BUGBUG~TODO mach better for perfomance use & IEEE ; sExp *= sExpSign; */
        if( sExpSign > 0)
        {
            for( ; sExp ; sExp--) sValue *= 10;
        }
        else
        {
            for( ; sExp ; sExp--) sValue /= 10;
        }
    }

  M1:
    if( aTail != NULL)
    {
        *aTail = (SChar*)s;
    }

    if( sSign == 0 )
    {
        *aValue = sValue;
    }
    else
    {
        *aValue = -sValue;
    }

    /* 0 - is Ok, 1 - is Warning the right truncated */
    if( c == 0 || aLen == 0 )
    {
        return 0;
    }
    else
    {
        if(sDigCnt == 0)
        {
            return -1;  /* Error - there are no significant digits      */
        }
        else
        {
            return  1; /* Wrning - there are right truncation of string */
        }
    }
}


SInt idlVA::slongToStr(SChar *aBuf, UInt aBufLen, SLong aVal, SInt radix)
{
    static const SChar digits[] = "0123456789ABCDEF";
    UInt    charPos = aBufLen - 1;
    idBool  sIsNeg  = (aVal < 0) ? ID_TRUE: ID_FALSE;

    aBuf[charPos--] = 0;

    if( sIsNeg == ID_FALSE)
    {
        aVal = -aVal;
    }

    while( aVal <= (-radix))
    {
        if( charPos >= aBufLen)
        {
            return -1;
        }
        aBuf[charPos--] = digits[ -(aVal % radix)];
        aVal = aVal / radix;
    }
    aBuf[charPos] = digits[(-aVal)];

    switch(radix)
    {
        case 16:
            if( (aBufLen - charPos) < 2)
            {
                return -1;
            }
            aBuf[--charPos] = 'x';
            aBuf[--charPos] = '0';
            break;

        case  8:
            if( (aBufLen - charPos) < 1)
            {
                return -1;
            }
            aBuf[--charPos] = '0';
            break;
    }

    if (sIsNeg == ID_TRUE)
    {
        if( (aBufLen - charPos) < 1)
        {
            return -1;
        }
        aBuf[--charPos] = '-';
    }

    idlOS::memmove( aBuf, (aBuf + charPos), aBufLen - charPos);
    return  (aBufLen - charPos);
}



ssize_t
idlVA::recv_nn (PDL_SOCKET handle,
                void *buf,
                size_t len)
{
    errno = 0; // to prevent error remain in case TIMEOUT
    return idlVA::recv_nn_i (handle,
                             buf,
                             len);
}

ssize_t
idlVA::recv_nn (PDL_SOCKET handle,
                void *buf,
                size_t len,
                const PDL_Time_Value *timeout)
{
    errno = 0; // to prevent error remain in case TIMEOUT
    return idlVA::recv_nn_i (handle,
                             buf,
                             len,
                             timeout);
}

// To Fix BUG-15181 [A3/A4] recv_nn_i가 timeout을 무시합니다.
// recv_nn 은 조금이라도 받은 데이터가 있으면 timeout을 무시한다.
// ( replication에서 이러한 시맨틱으로 recv_nn을 사용하고 있어서
//   recv_nn을 직접 수정할 수 없음.)
//
// recv_nn_to는 조금이라도 받은 데이터가 있어도
// timeout걸리면 에러를 리턴한다.

ssize_t
idlVA::recv_nn_to (PDL_SOCKET handle,
                void *buf,
                size_t len,
                const PDL_Time_Value *timeout)
{
    errno = 0; // to prevent error remain in case TIMEOUT
    return idlVA::recv_nn_to_i (handle,
                                buf,
                                len,
                                timeout);
}

ssize_t
idlVA::send_nn (PDL_SOCKET handle,
                void *buf,
                size_t len)
{
    errno = 0; // to prevent error remain in case TIMEOUT
    return idlVA::send_nn_i (handle,
                             buf,
                             len);
}

ssize_t
idlVA::send_nn (PDL_SOCKET handle,
                void *buf,
                size_t len,
                const PDL_Time_Value *timeout)
{
    errno = 0; // to prevent error remain in case TIMEOUT
    return idlVA::send_nn_i (handle,
                             buf,
                             len,
                             timeout);
}

SInt idlVA::system_timed_wait(SChar *path, SInt second, FILE *resultFp)
{
#if defined(VC_WIN32) || defined(WRS_VXWORKS) || defined(ITRON) || defined(ARM_LINUX) || defined(ELDK_LINUX) || defined (MIPS64_LINUX)
    /* empty */
    return -1;
#else
    SChar buffer[1024];
    FILE *fpin;
    PDL_Time_Value waittm;
#ifdef __CSURF__
    PDL_HANDLE sFd;
    ssize_t rc = 0;
#endif
    waittm.initialize(second);

    fpin = idlOS::popen(path, (SChar *)"r");
    if (fpin == NULL)
    {
        if (resultFp)
            idlOS::fprintf(resultFp, "Can't Execution Command =>[%s]\n", path);
        return -1;
    }
    else
    {
        while(1)
        {
/*BUGBUG_NT*//*fileno should not be used!! remove it when tspManager is being ported..*/
#ifdef __CSURF__
            sFd = fileno( fpin );
            if ( sFd != PDL_INVALID_HANDLE )
            {
                rc = idlVA::timed_readline( sFd, buffer, 1024, &waittm );
            }
#else
            ssize_t rc = idlVA::timed_readline(fileno(fpin), buffer, 1024, &waittm);
#endif
/*BUGBUG_NT*/

            if (rc == 0) return 0;

            if (rc < 0) return -1; // TIMEOUT!!

            if (resultFp)
                idlOS::fprintf(resultFp, "%s", buffer);
        }
    }
    (void)idlOS::pclose(fpin);
    return 0;
#endif /* !VC_WIN32 && !WRS_VXWORKS */
}

/**************************************************************
  ADDED FUNCTIONS FOR WIN 32 COMPATIBILITY
***************************************************************/

clock_t idlOS::times(struct tms *buffer) {
#if defined(VC_WIN32)
#if !defined(VC_WINCE)
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime; /* 100 ns */
    FILETIME userTime; /* 100 ns */

    if (!GetProcessTimes( GetCurrentProcess(),
                          &creationTime,
                          &exitTime,
                          &kernelTime,
                          &userTime ) ) {
        return (clock_t) -1;
    }
    LARGE_INTEGER liKernelTime; /* 100 ns */
    LARGE_INTEGER liUserTime; /* 100 ns */
    memcpy(&liKernelTime, &kernelTime, sizeof(liKernelTime));
    memcpy(&liUserTime, &userTime, sizeof(liUserTime));

    // BUG - 차일드 process를 알아내는 WIN32 API와 매핑되지 않았음.
    buffer->tms_stime = buffer->tms_cstime =
        (clock_t) ((liKernelTime.QuadPart / 10000000.0) * CLOCKS_PER_SEC);

    buffer->tms_utime = buffer->tms_cutime =
        (clock_t) ((liUserTime.QuadPart / 10000000.0) * CLOCKS_PER_SEC);

    return ::clock();
#else
    return ::GetTickCount();
#endif /* VC_WINCE */

#elif defined(ITRON)
    /* empty */
    return -1;
#else
#  if !defined(CYGWIN32) && !defined(WRS_VXWORKS) && !defined(ARM_LINUX) && !defined(ELDK_LINUX) && !defined(MIPS64_LINUX)
    return ::times(buffer);
#  else
    return times(buffer);
#  endif
#endif
}

#if defined(VC_WIN32) || defined(WRS_VXWORKS) || ( ( defined(SPARC_SOLARIS) || defined(X86_SOLARIS) ) && (OS_MAJORVER == 2) && (OS_MINORVER > 7) ) || defined(ANDROID) || defined(SYMBIAN) || defined(X86_64_DARWIN)

#define OUT_KEY_LEN       11
#define SALT_KEY_LEN       2
#define MK_VISIBLE_CHAR(a)  (gVisibleChar[ (UInt)(a) % 256])

static SChar*  idl_Crypt(SChar *aPasswd, SChar* aSalt, SChar *aResult)
{
    static SChar gVisibleChar[256] =
    {
/*   0    1    2    3    4    5    6    7    8    9    10   11  12   13   14   15   */
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
        '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '.', '1',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
        '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1',
        '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
    };

    //we will consider size of sbox 256 bytes
    //(extra byte are only to prevent any mishep just in case)
    SChar Sbox[257], Sbox2[257];
    UInt i, j, t, x;

    SChar temp , k;
    i = j = k = t =  x = 0;
    temp = 0;

    //always initialize the arrays with zero
    idlOS::memset(Sbox,  0, sizeof(Sbox));
    idlOS::memset(Sbox2, 0, sizeof(Sbox2));

    //initialize sbox i
    for(i = 0; i < 256; i++)
    {
        Sbox[i] = (SChar)i;
    }

    if (aSalt == NULL) // generate salt key
    {
        SChar sSalt1;
        SChar sSalt2;

        idlOS::srand((UInt)idlOS::time(NULL));

        sSalt1 = MK_VISIBLE_CHAR(idlOS::rand());
        sSalt2 = MK_VISIBLE_CHAR(idlOS::rand());

        for(i = 0; i < 256 ; i += 2)
        {
            Sbox2[i]     = sSalt1;
            Sbox2[i + 1] = sSalt2;
        }
    }
    else
    {
        if (idlOS::strlen(aSalt) < SALT_KEY_LEN)  // skip key & generate salt key
        {
            SChar sSalt1;
            SChar sSalt2;

            idlOS::srand((UInt)idlOS::time(NULL));

            sSalt1 = MK_VISIBLE_CHAR(idlOS::rand());
            sSalt2 = MK_VISIBLE_CHAR(idlOS::rand());

            for(i = 0; i < 256 ; i += 2)
            {
                Sbox2[i]     = sSalt1;
                Sbox2[i + 1] = sSalt2;
            }
        }
        else
        {

            //initialize the sbox2 with user key
            j = 0;
            for(i = 0; i < 256U ; i++)
            {
                if(j == SALT_KEY_LEN)
                {
                    j = 0;
                }
                Sbox2[i] = aSalt[j++];
            }
        }
    }

    j = 0 ; //Initialize j
    //scramble sbox1 with sbox2
    for(i = 0; i < 256; i++)
    {
        j = (j + (UInt) Sbox[i] + (UInt) Sbox2[i]) % 256 ;
        temp =  Sbox[i];
        Sbox[i] = Sbox[j];
        Sbox[j] =  temp;
    }

    i = j = 0;
    for(x = 0; x < OUT_KEY_LEN; x++)
    {
        //increment i
        i = (i + 1U) % 256;
        //increment j
        j = (j + (UInt) Sbox[i]) % 256U;

        //Scramble SBox #1 further so encryption routine will
        //will repeat itself at great interval
        temp = Sbox[i];
        Sbox[i] = Sbox[j] ;
        Sbox[j] = temp;

        //Get ready to create pseudo random  byte for encryption key
        t = ((UInt) Sbox[i] + (UInt) Sbox[j]) %  256;

        //get the random byte
        k = Sbox[t];

        //xor with the data and done
        aResult[x + 2] = MK_VISIBLE_CHAR(aPasswd[x] ^ k);
        // make visible character (from 33 to 126) by gamestar
    }
    aResult[x + 2] = 0;
    aResult[0] = Sbox2[0];
    aResult[1] = Sbox2[1];

    return aResult;
}
#endif


UChar *idlOS::crypt(UChar *aPasswd, UChar *aSalt, IDL_CRYPT_DATA *aData)
{
    UChar* result;

#ifdef IBM_AIX
    result = (UChar*)::crypt_r( (char*)aPasswd, (char*)aSalt, aData );
#elif defined(OS_LINUX_KERNEL) && !defined(ANDROID)
    result = (UChar*)crypt_r( (char*)aPasswd, (char*)aSalt, aData );

/*
 * BUG-35338  [MACOSX]sometimes , invalid password message occurs.
 */
#elif defined(VC_WIN32) || defined(WRS_VXWORKS) || ( ( defined(SPARC_SOLARIS) || defined(X86_SOLARIS) ) && (OS_MAJORVER == 2) && (OS_MINORVER > 7) ) || defined(ANDROID) || defined(SYMBIAN) || defined(X86_64_DARWIN)
    /*
     * windows or solaris 2.8, 2.9, 2.10... have to use our own crypt.
     * refer BUG-4716
     */
    result = (UChar *)idl_Crypt((SChar *)aPasswd,
                                (SChar *)aSalt,
                                (SChar *)(aData->mDummy));

#elif defined(ITRON)
    /* empty */
    return NULL;
#else
    result = (UChar*)::crypt( (char*)aPasswd, (char*)aSalt );
#endif /* INTEL_LINUX */
    return result;
}

// BUG-6152 by kumdory, 2003-03-25
SInt idlOS::clock_gettime( clockid_t clockid, struct timespec *ts )
{
#if defined(INTEL_LINUX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(ALPHA_LINUX) || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX)|| defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX) || defined(MIPS_WRS_LINUX)
    return ::clock_gettime( clockid, ts );
#else
    return PDL_OS::clock_gettime( clockid, ts );
#endif
}

SInt idlVA::connect_timed_wait(PDL_SOCKET sockfd,
                               struct sockaddr *addr,
                               int addrlen,
                               PDL_Time_Value  *tval)
{
    SInt  n;
    SInt  error;
    fd_set rset, wset;
#if defined(VC_WIN32)
    fd_set eset;
#endif

    if ( idlVA::setNonBlock( sockfd ) != IDE_SUCCESS )
    {
        return -1;
    }

    error = 0;

    if ( (n = idlOS::connect( sockfd, addr, addrlen)) < 0 )
    {
        if (errno != EINPROGRESS && errno != EWOULDBLOCK)
        {
            idlVA::setBlock(sockfd);
            return -1;
        }
    }

    if ( n != 0 )
    {
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        FD_SET(sockfd, &rset);
        FD_SET(sockfd, &wset);

#if !defined(VC_WIN32)
        n = idlOS::select(sockfd + 1, &rset, &wset, NULL, tval);
#else
        FD_ZERO(&eset);
        FD_SET(sockfd, &eset);
        n = idlOS::select(sockfd + 1, &rset, &wset, &eset, tval);
#endif

        idlVA::setBlock(sockfd);

        if ( n == 0)
        {
            errno = ETIMEDOUT;
            return -1;
        }
        else if ( n < 0 )
        {
            return -1;
        }

        if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
        {
#if !defined(VC_WINCE)
            SInt len;
            len = (SInt)sizeof(error);

            if (idlOS::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (SChar*)&error, &len) < 0)
            {
                return -1;
            }
#endif
        }
        else
        {
#if !defined(VC_WIN32)
            return -1;
#else
            if (FD_ISSET(sockfd, &eset))
            {
#if !defined(VC_WINCE)
                SInt len;
                len = (SInt)sizeof(error);

                if (idlOS::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (SChar*)&error, &len) == 0)
                {
                    errno = error;
                }
                return -1;
#endif
            }
#endif
        }
    }
    else
    {
        idlVA::setBlock(sockfd);
    }

    if (error)
    {
        errno = error;
        return -1;
    }

    return 0;
}

/**************************************************************
  WIN 32 WRAPPERS
***************************************************************/
#if defined(VC_WIN32) || defined(WRS_VXWORKS)

#define WIN_BADCH   (int)'?'
#define WIN_BADARG  (int)':'
#define WIN_EMSG    ""

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */
SChar    *optarg;                /* argument associated with option */
/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

/* 1003.2 says this must be 1 before any call.  */
SInt     optind = 1;             /* index into parent argv vector */

/* Callers store zero here to inhibit the error message
   for unrecognized options.  */
SInt     opterr = 1;             /* if error message should be printed */
/* Set to an option character which was unrecognized.
   This must be initialized on some systems to avoid linking in the
   system's own getopt implementation.  */

SInt     optopt = '?';           /* character checked for validity */
static SInt     optreset;               /* reset getopt */

static SInt
idl_getopt(SInt nargc, SChar * const *nargv, const SChar *ostr)
{
    static SChar *place = WIN_EMSG;              /* option letter processing */
    SChar *oli;                              /* option letter list index */

    if (optreset || !*place) {              /* update scanning pointer */
        optreset = 0;
        if (optind >= nargc || *(place = nargv[optind]) != '-') {
            place = WIN_EMSG;
            return (EOF);
        }
        if (place[1] && *++place == '-') {      /* found "--" */
            ++optind;
            place = WIN_EMSG;
            return (EOF);
        }
    }                                       /* option letter okay? */
    if ((optopt = (SInt)*place++) == (SInt)':' ||
        !(oli = (SChar*)strchr(ostr, optopt))) {
        /*
         * if the user didn't specify '-' as an option,
         * assume it means EOF.
         */
        if (optopt == (SInt)'-')
            return (EOF);
        if (!*place)
            ++optind;
        if (opterr && *ostr != ':')
            (void)fprintf(stderr,
                          "%s: illegal option -- %c\n", __FILE__, optopt);
        return (WIN_BADCH);
    }
    if (*++oli != ':') {                    /* don't need argument */
        optarg = NULL;
        if (!*place)
            ++optind;
    }
    else {                                  /* need an argument */
        if (*place)                     /* no white space */
            optarg = place;
        else if (nargc <= ++optind) {   /* no arg */
            place = WIN_EMSG;
            if (*ostr == ':')
                return (WIN_BADARG);
            if (opterr)
                (void)fprintf(stderr,
                              "%s: option requires an argument -- %c\n",
                              __FILE__, optopt);
            return (WIN_BADCH);
        }
        else                            /* white space */
            optarg = nargv[optind];
        place = WIN_EMSG;
        ++optind;
    }
    return (optopt);                        /* dump back option letter */
}
#endif

SInt idlOS::getopt (int argc,
                    SChar *const *argv,
                    const SChar *optstring)
{
#if defined(VC_WIN32) || defined(WRS_VXWORKS)
    return idl_getopt(argc, argv, optstring);
#else
    return PDL_OS::getopt(argc, argv, optstring);
#endif
}

SLong idlOS::sysconf(int name)
{
#if defined(VC_WIN32)
    switch(name) {
        case _SC_CLK_TCK :
            return (SLong)CLOCKS_PER_SEC;
        default:
            return (SLong)-1;
    }
#else
    return PDL_OS::sysconf(name);
#endif
}

#if defined(VC_WIN32)

#if defined(DEBUG)
void printWin32Error( DWORD dwMessageId )
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwMessageId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
        );
    // Process any inserts in lpMsgBuf.
    // ...
    // Display the string.
    printf( "\n---------- WIN32 LAST ERROR ---------\n");
    printf( "%s", (LPCTSTR)lpMsgBuf);
    // Free the buffer.
    LocalFree( lpMsgBuf );
}
#endif

SInt idlVA::daemonize (const char pathname[],
                       int close_all_handles)
{
#if defined(HP_HPUX) || defined(IA64_HP_HPUX)
    if (close_all_handles != 0)
    {
        SInt i;
        // close all except 2 (stderr for stack dump)
        for ( i = PDL::max_handles () - 1; i >= 3; i--)
            PDL_OS::close (i);
        PDL_OS::close (1);
        PDL_OS::close (0);
    }

    return PDL::daemonize(pathname, 0);
#else
    return PDL::daemonize(pathname, close_all_handles);
#endif
}

#endif

SInt idlVA::getSystemUptime(SChar *aBuffer, SInt aBuffSize)
{
    static SChar *sUptimeLocate[] =
    {
        (SChar *)"/usr/bin/uptime",
        (SChar *)"/usr/ucb/uptime",
        NULL
    };
#if defined(VC_WIN32)
    char DAYOFWEEK[7][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char MONTH[12][4] = {"Jan","Feb","Mar","Apr","May","Jun",
                         "Jul","Aug","Sep","Oct","Nov","Dec"};
    SYSTEMTIME systime_;
    int days, hours, minutes, remain_days, remain_hours;
    char day_tail;

    int dwStart    = GetTickCount();

    days           = dwStart/(24*60*60*1000);
    remain_days    = dwStart%(24*60*60*1000);
    hours          = remain_days/(60*60*1000);
    remain_hours   = remain_days%(60*60*1000);
    minutes        = remain_hours/(60*1000);
    day_tail = (days >= 2) ? 's' : ' ';

    GetLocalTime(&systime_);

    idlOS::snprintf(aBuffer, aBuffSize, "up %0d day%c %02d:%02d (since  %s %s %d %d:%d:%d %d)",
                    days, day_tail, hours, minutes,
                    DAYOFWEEK[systime_.wDayOfWeek],
                    MONTH[systime_.wMonth-1],
                    systime_.wDay - days,
                    systime_.wHour - hours, systime_.wMinute - minutes, systime_.wSecond,
                    systime_.wYear);
    return 0;
#elif defined(WRS_VXWORKS)
    idlOS::strncpy(aBuffer, "can't get system uptime", aBuffSize);
    return 0;
#else
    SInt  error;
    FILE *fp;
    SInt  i;

    error = 1;
    for (i = 0; sUptimeLocate[i] != NULL; i++)
    {
        fp = popen(sUptimeLocate[i], "r");
        if (fp != NULL)
        {
            if (idlOS::fgets(aBuffer, aBuffSize, fp) != NULL)
            {
                pclose(fp);
                error = 0;
                break;
            }
            else
            {
                pclose(fp);
            }
        }
    }
    if (error == 1)
    {
        // failed to get uptime
        idlOS::strncpy(aBuffer, "can't get system uptime", aBuffSize);
    }
    return 0;
#endif
}

SInt idlVA::appendFormat(char *aBuffer, size_t aBufferSize, const char *aFormat, ...)
{
    size_t  sLen;
    SInt    sResult;
    va_list ap;

    sLen = idlOS::strlen(aBuffer);
    // BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서 local Array의
    // ptr를 반환하고 있습니다.
    //
    // aBufferSize가 sLen보다 커야합니다. 그렇지 않으면 이후 vsnprintf함수 호출시
    // 음수값이 넘어가고, 그 음수값이 unsigned로 해석되면서 메모리를 긁을 수 
    // 있습니다.
    assert( aBufferSize > sLen );

    va_start(ap, aFormat);
    sResult = idlOS::vsnprintf(aBuffer + sLen, aBufferSize - sLen, aFormat, ap);
    va_end(ap);

    if (sResult < 0)
    {
        return sResult;
    }
    else
    {
        return sResult + sLen;
    }
}

SInt idlVA::appendString(char *aBuffer, size_t aBufferSize, char *aString, UInt aLen)
{
    size_t  sLen;
    char   *sPtr;
    SInt    sSize;

    sLen  = idlOS::strlen(aBuffer);
    sPtr  = aBuffer + sLen;
    sSize = IDL_MIN(aBufferSize - sLen - 1, aLen);

    idlOS::memcpy(sPtr, aString, sSize);
    sPtr[sSize] = 0;

    return sLen + aLen;
}

//===================================================================
// To Fix PR-13963
// 요구한 크기가 large heap을 요구한것인지를 판단
//===================================================================

void idlOS::checkLargeHeapUse (
#ifndef BUILD_FOR_UTIL
    size_t nbytes
#else
    size_t
#endif
    )
{
#ifndef BUILD_FOR_UTIL
    // 요구한 크기가 정해진 크기를 넘는다면 로그에 출력
    if( ( INSPECTION_LARGE_HEAP_THRESHOLD_INITIALIZED == ID_TRUE ) &&
        ( INSPECTION_LARGE_HEAP_THRESHOLD > 0 ) &&
        ( nbytes >= INSPECTION_LARGE_HEAP_THRESHOLD ) )
    {
        ideLogEntry sLog( IDE_SERVER_3 );
        sLog.appendFormat( ID_TRC_MEMORY_THRESHOLD_OVERFLOW, (ULong) nbytes );
        iduStack::dumpStack();
        sLog.write();
    }
#endif
}

void *idlOS::malloc ( size_t nbytes )
{
    // 요구한 크기가 정해진 크기를 넘는다면 로그에 출력
    checkLargeHeapUse( nbytes );

    return PDL_OS::malloc( nbytes );
}

void *idlOS::calloc ( size_t elements, size_t sizeof_elements )
{
    // 요구한 크기가 정해진 크기를 넘는다면 로그에 출력
    checkLargeHeapUse( elements * sizeof_elements );
    return PDL_OS::calloc( elements, sizeof_elements );
}

void *idlOS::realloc ( void *ptr, size_t nbytes )
{
    // 요구한 크기가 정해진 크기를 넘는다면 로그에 출력
    checkLargeHeapUse( nbytes );
    return PDL_OS::realloc( ptr, nbytes );
}

void idlOS::free ( void *ptr )
{
    PDL_OS::free ( PDL_MALLOC_T(ptr) );
}

SChar idlOS::toupper(SChar c)
{
    return (c >= 'a' && c <= 'z' ) ? c - ('a' - 'A') : c;
}

SChar idlOS::tolower(SChar c)
{
    return (c >= 'A' && c <= 'Z' ) ? c + ('a' - 'A') : c;
}

/* padding이 붙은 자료 구조를 이 함수로 초기화한다.
   이 함수는 memory check시에만 초기화한다. */
void *idlOS::memsetOnMemCheck(void *s, int c, size_t len)
{
#if defined(ALTIBASE_MEMORY_CHECK)
    return memset(s, c, len);
#else
    c = c;
    len = len;
    return s;
#endif
}

UInt idlOS::hex2dec(UChar * src, UInt length)
{
    UInt result = 0;
    UInt digit;

    while(length-- > 0) 
    {
        if(*src >= '0' && *src <= '9')
        {
            digit = *src - '0';
        }
        else if(*src >= 'A' && *src <='F')
        {
            digit = *src - 'A' + 10;
        }
        else if(*src >= 'a' && *src <='f')
        {
            digit = *src - 'a' + 10;
        }
        else
        {
            break;
        }

        ++ src;
        result = (result << 4) | (digit & 0xf);
    }

    return result;
}

#if defined(VC_WINCE) || defined(SYMBIAN)
void _printf( char * format, ... )
{
    char        buf[4096];
    va_list     ap;
    PDL_HANDLE  out;

    out = idlOS::open("stdout.txt" , O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

    idlOS::memset(buf, '\0', 4096);

    va_start(ap, format);

    idlOS::vsnprintf(buf, 4096, format, ap);

    va_end(ap);

    idlOS::write(out, buf, strlen(buf));

    idlOS::close(out);
}
#endif

SInt idlOS::fdeof(PDL_HANDLE aHandle)
{
    PDL_OFF_T sEndPos;
    PDL_OFF_T sCurPos;
    SInt      sResult;

    sCurPos = idlOS::lseek(aHandle, 0, SEEK_CUR);

    sEndPos = idlOS::filesize(aHandle);

    if(sEndPos == sCurPos)
    {
        sResult = 1;
    }
    else
    {
        sResult = 0;
    }

    return sResult;
}

SChar *idlOS::fdgets(SChar *aBuf, SInt aSize, PDL_HANDLE aHandle)
{
    SInt i;
    SInt sSize = aSize - 1;

    for(i = 0; i < sSize; i++)
    {
        if(idlOS::read(aHandle, aBuf + i, 1) <= 0)
        {
            aBuf[i] = '\0';

            break;
        }

        if(aBuf[i] == '\n')
        {
            break;
        }
    }

    if(i == sSize )
    {
        aBuf[i] = '\0';
    }
    else
    {
        aBuf[i + 1] = '\0';
    }

    if(aBuf[0] == '\0')
    {
        return NULL;
    }

    return aBuf;
}

/*
 * Adding directory function
 */

SInt idlOS::rmdir(SChar* aPath)
{
    SInt sRC;
    sRC = acpDirRemove(aPath);
    return ACP_RC_IS_SUCCESS(sRC)? 0 : -1;
}
