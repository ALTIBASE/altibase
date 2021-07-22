/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idl.h 82936 2018-04-30 04:22:11Z emlee $
 **********************************************************************/

/***********************************************************************
 *
 * NAME
 *   idl.h
 *
 * DESCRIPTION
 *   InDependent Layer for PDL WRAPPER
 *   PDL 라이브러리를 소스수준에서 idlVA 매크로로 대치함
 *   주의 : PDL.h 대신에 이 화일을 include 해야 함.
 *         (명시적으로 PDL.h를 하는 것은 이 화일의 의도와 어긋남)
 *
 * MODIFIED    (MM/DD/YYYY)
 *    sjkim     01/07/2000 - Created
 *
 **********************************************************************/

# include <OS.h>

#ifndef O_IDL_H
# define O_IDL_H 1

#ifdef __cplusplus
#define IDL_EXTERN_C       extern "C"
#define IDL_EXTERN_C_BEGIN extern "C" {
#define IDL_EXTERN_C_END   }
#else
#define IDL_EXTERN_C       extern
#define IDL_EXTERN_C_BEGIN
#define IDL_EXTERN_C_END
#endif

#include <idConfig.h>
#include <idTypes.h>
#include <idf.h>

# include <PDL2.h>
# include <Handle_Set2.h>

#if defined (VC_WINCE)
#include <WinCE_Assert.h>
#endif /* VC_WINCE*/

#if defined(DEC_TRU64) || defined(HP_HPUX) || defined(IA64_HP_HPUX)
#include <sys/termios.h>
#endif // DEC_TRU64

#if defined(VC_WIN32) || defined(WRS_VXWORKS)
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;
#else
#include <math.h>
#endif /*VC_WIN32*/

#if defined(VC_WIN32) && _MSC_VER < 1400
extern "C" __int64 _ftelli64(FILE *);
#endif

#ifdef SPARC_SOLARIS
#include <ieeefp.h> /* for use of finite() math function */
#endif /* SPARC_SOLARIS */

#if !defined(ANDROID) && !defined(SYMBIAN)

#if !defined (VC_WIN32) && !defined(ITRON)

#if !defined(WRS_VXWORKS) && !defined(X86_64_DARWIN)
#include <crypt.h>
#endif

#include <aio.h>
#endif



#endif

#if defined(IBM_AIX)
typedef  CRYPTD IDL_CRYPT_DATA;
#elif defined(OS_LINUX_KERNEL) && !defined(ANDROID)
typedef struct crypt_data  IDL_CRYPT_DATA;
#else
typedef struct IDL_DummyStruct_IDL
{
    UChar mDummy[256];
}idlDummyStruct;
typedef IDL_DummyStruct_IDL IDL_CRYPT_DATA;
#endif

# if defined (PDL_HAS_WINCE) || defined (PDL_WIN32)

#define PTHREAD_ONCE_INIT       { FALSE, -1 }
typedef struct pthread_once_t_
{
  int done;                 /* indicates if user function executed  */
  long started;             /* First thread to increment this value */
                            /* to zero executes the user function   */
}pthread_once_t;
#elif defined(WRS_VXWORKS) 
#define PTHREAD_ONCE_INIT  0 
typedef int pthread_once_t; 
#endif

//fix for ATAF
#if defined (DEC_TRU64)

#undef toupper
#undef tolower

#endif /* DEC_TRU64 */

/*
 * Some platforms, e.g. HP-UX, do not provide va_copy(), but the
 * following definition may work. (It is known to work at least on
 * HP-UX.)
 */
#if !defined(va_copy)
# define va_copy(aDest, aSource) ((aDest) = (aSource))
#endif

IDL_EXTERN_C_BEGIN
typedef void (*pthread_once_init_function_t)(void);
IDL_EXTERN_C_END

class idlOS
    : public PDL_OS
{
private:
//===================================================================
// To Fix PR-13963
// 대용량 힙 요구를 검사하고, 이를 로그에 출력하기 위한 내부 함수
//===================================================================
    static inline void checkLargeHeapUse(size_t nbytes);

#if defined(DEC_TRU64) && (OS_MAJORVER <= 4)
//===================================================================
// To Fix PR-17754
// 일부 플랫폼에 없는 vsnprintf() 구현을 위한 내부 함수
//===================================================================
    static SInt int_vasprintf( SChar**      aResult,
                               const SChar* aFormat,
                               va_list      aArgs );
    static inline SInt vasprintf( SChar**      aResult,
                                  const SChar* aFormat,
                                  va_list      aArgs );
#endif /* (DEC_TRU64) && (OS_MAJORVER <= 4) */

public:
//===================================================================
// To Fix PR-13963
// PDL_OS의 힙 할당 함수 사용을 idlOS로 처리하기 위한 함수
//===================================================================
    static void *malloc (size_t nbytes);
    static void *calloc (size_t elements, size_t sizeof_elements);
    static void *realloc (void *ptr, size_t nbytes);
    static void free (void *ptr);

    static SInt getMaxOpen ();
    static SInt setMaxOpen(SInt MaxOpen);
    static void *thr_getspecific2 (PDL_thread_key_t key);
    static SInt chmod(SChar *filename, mode_t t);
    static SChar *strerror(SInt errnum);

//  registers exit handler
    static  SInt atexit(void (*function)(void));

    static SInt pthread_cond_init(pthread_cond_t *cond,
                                  pthread_condattr_t *cond_attr);

    static SInt pthread_once( pthread_once_t * once_control,
                              pthread_once_init_function_t init_routine);

    static FILE* popen(SChar *command, SChar *mode);
    static SInt pclose(FILE *);

    static PDL_HANDLE idlOS_fileno(FILE *);

    static SInt   pthread_getconcurrency(void);
    static SInt   pthread_setconcurrency(SInt new_level);


    static SInt    idlOS_feof(FILE *);

    static SInt   vsnprintf( char *buf, size_t size, const char *format, va_list ap );
    static SInt   snprintf( char *buf, size_t size, const char *format, ... );

    static SDouble strtod(const char *, char **endptr);

    static UInt   strToUInt( UChar* aValue,
                             UInt   aLength );

    static ULong  strToULong( UChar* aValue,
                              UInt   aLength );

    static void   strUpper( void*  aValue,
                            size_t aLength );

    static SInt   strCaselessMatch( const void* aValue1,
                                    const void* aValue2 );

    static SInt   strCaselessMatch( const void* aValue1,
                                    size_t      aLength1,
                                    const void* aValue2,
                                    size_t      aLength2 );

    static SInt   strMatch( const void* aValue1,
                            size_t      aLength1,
                            const void* aValue2,
                            size_t      aLength2 );

    static SInt   strCompare( const void* aValue1,
                              size_t      aLength1,
                              const void* aValue2,
                              size_t      aLength2 );

    static SChar* strsep(SChar **aStringp,
                         const SChar *aDelim);

    static SChar* strcasestr( const SChar *aHayStack,
                              const SChar *aNeedle );

    static int    idlOS_tolower( int c );
    static int    idlOS_toupper( int c );

    static int    idlOS_isspace( int c );

    static SInt   align(SInt Size);
    static UInt   align(UInt Size);
    static SLong  align(SLong Size);
    static ULong  align(ULong Size);
    static SInt   align4(SInt Size);
    static UInt   align4(UInt Size);
    static SLong  align4(SLong Size);
    static ULong  align4(ULong Size);
    static SInt   align8(SInt Size);
    static UInt   align8(UInt Size);
    static SLong  align8(SLong Size);
    static ULong  align8(ULong Size);

    static UInt   align( UInt aOffset, UInt aAlign );
    static ULong  alignLong( ULong aOffset, ULong aAlign );
    static void*  align(void* a_pMem, UInt aAlign);

    static void*  alignDown(void* aMemoryPtr, UInt aAlignSize );

#if (defined( IBM_AIX ) && !defined (__GNUG__) && defined(COMPILE_64BIT)) || (defined(VC_WIN32) && !defined(COMPILE_64BIT)) || defined(X86_64_DARWIN)
    /*
     * AIX 컴파일러의 경우 vULong, vSLong을 별도의 타입으로 인식한다.
     * Windows 32비트 컴파일러도 마찬가지.
     * 따라서, 해당 타입에 따른 align을 별도로 구현함.
     */
    static vSLong  align(vSLong Size);
    static vULong  align(vULong Size);
    static vSLong  align4(vSLong Size);
    static vULong  align4(vULong Size);
    static vSLong  align8(vSLong Size);
    static vULong  align8(vULong Size);
#endif
    static SInt	tcgetattr ( SInt filedes, struct termios *termios_p );
    static SInt	tcsetattr( SInt filedes, SInt optional_actions, struct termios *termios_p );
    static time_t mktime( struct tm *t );
    // network -> host
    static UShort  ntoh ( UShort Value );
    static SShort  ntoh ( SShort Value );
    static UInt    ntoh ( UInt   Value );
    static SInt    ntoh ( SInt   Value );
    static ULong   ntoh ( ULong  Value );
    static SLong   ntoh ( SLong  Value );

    // host -> network
    static UShort  hton ( UShort Value );
    static SShort  hton ( SShort Value );
    static UInt    hton ( UInt   Value );
    static SInt    hton ( SInt   Value );
    static ULong   hton ( ULong  Value );
    static SLong   hton ( SLong  Value );

    static ULong   getThreadID();
    static ULong   getThreadID(PDL_thread_t aID);
    static inline SInt getParallelIndex();

    static SInt getopt (int argc,
					 char *const *argv,
					 const char *optstring);
    static SLong sysconf(int name);

    static SDouble logb(SDouble x);
/*
    static SDouble ceil(SDouble x);
*/

// math functions in <math.h> -- start
    static SDouble sin(SDouble x);
    static SDouble sinh(SDouble x);
    static SDouble asin(SDouble x);
    static SDouble asinh(SDouble x);

    static SDouble cos(SDouble x);
    static SDouble cosh(SDouble x);
    static SDouble acos(SDouble x);
    static SDouble acosh(SDouble x);

    static SDouble tan(SDouble x);
    static SDouble tanh(SDouble x);
    static SDouble atan(SDouble x);
    static SDouble atanh(SDouble x);
    static SDouble atan2(SDouble y, SDouble x);

    static SDouble sqrt(SDouble x);
    static SDouble cbrt(SDouble x);
    static SDouble floor(SDouble x);
    static SDouble ceil(SDouble x);
    static SDouble copysign(SDouble x, SDouble y);
    static SDouble erf(SDouble x);
    static SDouble erfc(SDouble x);
    static SDouble fabs(SDouble x);
    static SDouble lgamma(SDouble x);
    static SDouble lgamma_r(SDouble x, SInt *signgamp);
    static SDouble hypot(SDouble x, SDouble y);
    static SInt ilogb(SDouble x);
/*    static SInt isnan(SDouble dsrc); */

    static SDouble log(SDouble x);
    static SDouble log10(SDouble x);
    static SDouble log1p(SDouble x);
    static SDouble nextafter(SDouble x, SDouble y);
    static SDouble pow(SDouble x, SDouble y);
    static SDouble remainder(SDouble x, SDouble y);
    static SDouble fmod(SDouble x, SDouble y);
    static SDouble rint(SDouble x);
    static SDouble scalbn(SDouble x, SInt n);
/*    static SDouble significand(SDouble x); */
    static SDouble scalb(SDouble x, SDouble n);
    static SDouble exp(SDouble x);
    static SDouble expm1(SDouble x);
// math functions in <math.h> -- end

    static clock_t times(struct tms *buffer);

    static SInt directio (PDL_HANDLE fd, SInt flag);
    static UChar *crypt(UChar *aPasswd, UChar *aSalt, IDL_CRYPT_DATA *aData);

    // BUG-6152 by kumdory, 2004-03-25
    static SInt clock_gettime( clockid_t clockid, struct timespec *ts );

    // TASK-1733
    static PDL_OFF_T ftell(FILE *aFile);
    static size_t strnlen(const SChar *aString, size_t aMaxLength);
    static SChar toupper(SChar c);
    static SChar tolower(SChar c);

    /* memory check시에만 메모리를 초기화하는 함수 */
    static void *memsetOnMemCheck(void *s, int c, size_t len);

    static UInt hex2dec(UChar * src, UInt length);

    static SChar *fdgets(SChar*, SInt, PDL_HANDLE);
    static SInt   fdeof(PDL_HANDLE);

    /*
     * Directory Function 
     * return 0 on success, -1 on failure 
     */
    static int rmdir(char*);
};

class idlVA : public PDL
{
 public:
    static SInt setNonBlock(PDL_SOCKET fd);
    static SInt setBlock(PDL_SOCKET fd);
    static SInt setSockReuseraddr(PDL_SOCKET fd);

    static SInt system_timed_wait(SChar *path, SInt second, FILE *resultFp = stdin);

    static ssize_t readline(PDL_HANDLE fd, void *vptr, size_t maxlen);
    static ssize_t timed_readline(PDL_HANDLE, void *, ssize_t, PDL_Time_Value *);

    static ssize_t writeline(PDL_HANDLE fd, void *vptr);
    static ssize_t timed_writeline(PDL_HANDLE, void *, PDL_Time_Value *);

/*BUGBUG_NT*/

	static ssize_t recvline(PDL_SOCKET fd, const SChar *vptr, size_t maxlen);
	static ssize_t timed_recvline(PDL_SOCKET fp,
									  const SChar *message,
									  ssize_t maxlen,
									  PDL_Time_Value *tm);
	static ssize_t sendline(PDL_SOCKET fd, const SChar *vptr);
	static ssize_t timed_sendline(PDL_SOCKET fp,
                                   const SChar *message,
                                   PDL_Time_Value *tm);
/*BUGBUG_NT ADD*/

    static SInt getSystemMemory(ULong *PageSize,
                                ULong *MaxMem, ULong *AvailMem, ULong *FreeMem,
                                ULong *SwapMaxMem, ULong *SwapFreeMem);
    static SInt getProcessorCount();

    /* BUG-44958 */
    static SInt getPhysicalCoreCount();

    static SLong getDiskFreeSpace(const SChar *path);

    static SInt getMacAddress(ID_HOST_ID *mac, UInt *aNoValues);

    // 다양한 형태의 스트링 숫자형 지원함수 : 12M, 12K, -1238 등
    static SLong fstrToLong(SChar *str, idBool *aValidity = NULL);

    // Network API : PDL의 API는 fcntl의 과도한 호출로 인해
    // 성능이 많이 느림. 이 부분을 삭제한 API임
    // 주의!!! : 반드시 fd를 non-blocking 한 후 수행해야 함.!!

    static ssize_t recv_nn (PDL_SOCKET handle,
                           void *buf,
                            size_t len);
    static ssize_t recv_nn (PDL_SOCKET handle,
                           void *buf,
                           size_t len,
                           const PDL_Time_Value *timeout);

    // To Fix BUG-15181 [A3/A4] recv_nn_i가 timeout을 무시합니다.
    // recv_nn 은 조금이라도 받은 데이터가 있으면 timeout을 무시한다.
    // ( replication에서 이러한 스키마로 recv_nn을 사용하고 있어서
    //   recv_nn을 직접 수정할 수 없음.)
    //
    // recv_nn_to는 조금이라도 받은 데이터가 있어도
    // timeout걸리면 에러를 리턴한다.
    static ssize_t recv_nn_to (PDL_SOCKET handle,
                               void *buf,
                               size_t len,
                               const PDL_Time_Value *timeout);

    static ssize_t recv_nn_i (PDL_SOCKET handle,
                             void *buf,
                             size_t len);
    static ssize_t recv_nn_i (PDL_SOCKET handle,
                             void *buf,
                             size_t len,
                             const PDL_Time_Value *timeout);

    // recv_nn_to 가 호출하는 구현함수
    static ssize_t recv_nn_to_i (PDL_SOCKET handle,
                                 void *buf,
                                 size_t len,
                                 const PDL_Time_Value *timeout);

    static ssize_t recv_i (PDL_SOCKET handle, void *buf, size_t len);

    static ssize_t send_nn (PDL_SOCKET handle,
                           void *buf,
                            size_t len);
    static ssize_t send_nn (PDL_SOCKET handle,
                           void *buf,
                           size_t len,
                           const PDL_Time_Value *timeout);

    static ssize_t send_nn_i (PDL_SOCKET handle,
                             void *buf,
                             size_t len);
    static ssize_t send_nn_i (PDL_SOCKET handle,
                             void *buf,
                             size_t len,
                             const PDL_Time_Value *timeout);
    static ssize_t send_i (PDL_SOCKET handle, void *buf, size_t len);

#if defined(VC_WIN32)
    static SInt daemonize (const char pathname[], int close_all_handles);
#endif /*VC_WIN32*/

    static SInt connect_timed_wait(PDL_SOCKET sockfd,
                                   struct sockaddr *addr,
                                   int addrlen,
                                   PDL_Time_Value  *tval);
    static SInt getSystemUptime(SChar *aBuffer, SInt aBuffSize);

    /*
     * appendFormat 함수의 인자 및 리턴값의 spec은 snprintf와 같다.
     *   aBuffer:      출력되는 버퍼
     *   aBufferSize:  aBuffer에 할당된 사이즈
     *   aFormat:      snprintf 포맷
     *
     *   Return Value: aFormat, ... 으로 출력된 문자열 길이 (sprintf와 같다)
     *                 만약 이 값이 aBufferSize와 같거나 크면 버퍼 초과로 짤린 것임
     *                 최대 문자열 길이는 aBufferSize - 1 이며 null-terminated 된다.
     *
     * 차이점은 출력결과가 aBuffer에 concatenate된다는 점이다.
     */
    static SInt appendFormat(char *aBuffer, size_t aBufferSize, const char *aFormat, ...);
    static SInt appendString(char *aBuffer, size_t aBufferSize, char *aString, UInt aLen);

    /**
     *  @fn strToSLong()/strToSDouble
     *  @brief Function are covert string buffer to the
     *         SLong/SDouble with null terminated string
     *  or fixed buffer size.
     *  @param[in] string buffer could be null terminated.
     *  @param[in] length of buffer fixed size.
     *  @param[out] value pointer to return.
     *  @param[out] string pointer if it not signifficant.
     *  @return value is:
     *  -2 Overflow internall convertion
     *  -1 Invalid string format
     *   0 Success.
     *   1 Warning, conversion is success and tail has none digits parameters.
     */
    static SInt strnToSLong  (const SChar*, UInt, SLong*  , SChar**);
    static SInt strnToSDouble(const SChar*, UInt, SDouble*, SChar**);

    /**@fn slongToStr()
     * @brief Function convert given SLong t the given string buffer,
     * @param[in] string buffer.
     * @param[in] length of string buffer.
     * @param[in] Slong value.
     * @param[in] Radix could be 16, 10, 8, 2. 16 - write down prefix 0x,
     *                  8 write prefix 0.., others does not write else.
     * @return value 0 - is success, and -1 is failure.
     */
    static SInt slongToStr  (SChar *aBuf, UInt aBufLen, SLong aVal, SInt radix);
};


#define IDL_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#define IDL_MIN(a,b)  ( (a) < (b) ? (a) : (b) )

#ifndef SHUT_RD // for some dirty linux (power linux 6.0) by gamestar 00/8/3
#define SHUT_RD   0
#define SHUT_WR   1
#define SHUT_RDWR 2
#endif

#ifndef MSG_WAITALL
#define MSG_WAITALL     0x40            /* wait for full request or error */
#endif

#if defined(VC_WIN32)
#    define IDL_INVALID_HANDLE (PDL_INVALID_HANDLE)
#    define _SC_CLK_TCK 1

struct tms {
     clock_t    tms_utime;
     clock_t    tms_stime;
     clock_t    tms_cutime;
     clock_t    tms_cstime;
};

#    define IDL_FILE_SEPARATOR '\\'
#    define IDL_FILE_SEPARATORS "\\"
// FOR DEBUG ONLY
#if defined(DEBUG)
	extern void printWin32Error( DWORD dwMessageId );
#else
#define printWin32Error(x) {}
#endif

#else  /* !VC_WIN32 */

#    define IDL_INVALID_HANDLE (-1)
#    define IDL_FILE_SEPARATOR '/'
#    define IDL_FILE_SEPARATORS "/"
#endif /* VC_WIN32 */

#if defined VC_WIN32
#   include <math.h>
#   include <float.h>
#   define idlOS_isNotFinite(x) ( ( _fpclass( (x) ) &     \
                           (                 \
							  _FPCLASS_NN |  \
							  _FPCLASS_ND |  \
							  _FPCLASS_NZ |  \
							  _FPCLASS_PZ |  \
							  _FPCLASS_PD |  \
							  _FPCLASS_PN    \
						   )                 \
					 ) == 0 )
#else
#if defined(HP_HPUX) || defined(IA64_HP_HPUX)
#    define idlOS_isNotFinite(x) ( isfinite(x) == 0 )
#else
#if defined(DEC_TRU64)
#    define idlOS_isNotFinite(x) ( errno == EDOM || errno == ERANGE )
#else
#    define idlOS_isNotFinite(x) ( finite(x) == 0 )
#endif /* DEC_TRU64 */
#endif /* HP_HPUX */
#endif /* VC_WIN32 */

#if defined (DEC_TRU64)
#include <c_asm.h>
#endif /* DEC_TRU64 */

#if  defined(IBM_AIX) && (__xlC__  >= 0x0a01)  // >=  xlc 10.1
#include <builtins.h>
#endif

#  define IDL_MEM_BARRIER  asm("dmb ish")


# include <idl.i>


//PROJ-2182
#define IDL_CACHE_LINE   128    // just for now

#if 0         //XXX FIX IT
#if defined(VC_WIN32) || defined(CYGWIN32)
#define IDL_CACHE_LINE 32
#endif

#ifdef SPARC_SOLARIS
#define IDL_CACHE_LINE 64
#endif

#ifndef IDL_CACHE_LINE
// We don't know what the architecture is,
// so go for the gusto.
#define IDL_CACHE_LINE 64
#endif

#endif

#ifdef __CSURF__

#define ID_SERIAL_BEGIN(serial_code) { serial_code; };
#define ID_SERIAL_EXEC(serial_code,num) { serial_code; };
#define ID_SERIAL_END(serial_code) { serial_code; };   

#else

#define ID_SERIAL_BEGIN(serial_code) {\
volatile UInt vstSeed = 0;\
 if ( vstSeed++ == 0)\
 {\
     serial_code;\
 }

#define ID_SERIAL_EXEC(serial_code,num) \
  if (vstSeed++ == num) \
  {\
    serial_code;\
  }\
  else \
  { \
     IDE_ASSERT(0);\
  } 

#define ID_SERIAL_END(serial_code)\
  if (vstSeed++ != 0 )\
  {\
      serial_code;\
  }\
}
#endif

#if defined(VC_WINCE) || (_MSC_VER < 1300)
#define ID_ULTODB(aData) (SDouble)(SLong)(aData)
#else
#define ID_ULTODB(aData) (SDouble)(aData)
#endif /* VC_WINCE */

// Direct I/O 최대 Page크기 = 8K
// idpDescResource.cpp의 IDP_MAX_DIO_PAGE_SIZE 와 동일한 값이어야 한다.
#define ID_MAX_DIO_PAGE_SIZE ( 8 * 1024 )

/* fix BUG-17606, BUG-17724 */
#if defined(HP_HPUX) 
#define IDL_ODBCINST_LIBRARY_PATH "libodbcinst.sl"
#else
#define IDL_ODBCINST_LIBRARY_PATH "libodbcinst.so"
#endif /* HP_HPUX */

/* fix BUG-17606 ,BUG-17724*/
#if defined(HP_HPUX) ||  defined(IA64_HP_HPUX)
/* fix BUG-22936 64bit SQLLEN이 32bit , 64bit 모두 제공해야 합니다. */
#if defined(COMPILE_64BIT)
#if defined(BUILD_REAL_64_BIT_MODE)
#define IDL_ALTIBASE_ODBC_NAME "lib"PRODUCT_PREFIX"altibase_odbc-64bit-ul64.sl"
#else
#define IDL_ALTIBASE_ODBC_NAME "lib"PRODUCT_PREFIX"altibase_odbc-64bit-ul32.sl"
#endif /* BUILD_REAL_64_BIT_MODE */
#else
#define IDL_ALTIBASE_ODBC_NAME "lib"PRODUCT_PREFIX"altibase_odbc.sl"
#endif /* COMPILE_64BIT */
#else /* HP_HPUX  */
#if defined(PDL_WIN32)
#define IDL_ALTIBASE_ODBC_NAME PRODUCT_PREFIX"altiodbc.dll"
#else /* PDL_WIN32 */
/* fix BUG-22936 64bit SQLLEN이 32bit , 64bit 모두 제공해야 합니다. */
#if defined(COMPILE_64BIT)
#if defined(BUILD_REAL_64_BIT_MODE)
#define IDL_ALTIBASE_ODBC_NAME "lib"PRODUCT_PREFIX"altibase_odbc-64bit-ul64.so"
#else
#define IDL_ALTIBASE_ODBC_NAME "lib"PRODUCT_PREFIX"altibase_odbc-64bit-ul32.so"
#endif /* BUILD_REAL_64_BIT_MODE */
#else
#define IDL_ALTIBASE_ODBC_NAME "lib"PRODUCT_PREFIX"altibase_odbc.so"
#endif /* COMPILE_64BIT */
#endif /* PDL_WIN32 */
#endif /* HP_HPUX  */

/* BUG-22358: [ID] CPU갯수를 이용하는 코드에서 상한치를 줄수 있는 프로퍼티
 * 가 필요 합니다.
 *
 * ID_SCALABILITY_SYS를 사용하도록 수정
 * */

/* BUG-22235: CPU갯수가 다량일때 Memory를 많이 사용하는 문제가 있습니다.
 *
 * 다중화 갯수가 MAX_SCALABILITY 값을 넘어가면 이 값으로 보정한다.
 * */
#define ID_SCALABILITY_CLIENT_CPU    1
#define ID_SCALABILITY_CLIENT_MAX   32
#define ID_SCALABILITY_CPU ( idlVA::getProcessorCount() * iduProperty::getScalability4CPU() )
#define ID_SCALABILITY_SYS ( ( ID_SCALABILITY_CPU > iduProperty::getMaxScalability() ) ? \
                             iduProperty::getMaxScalability() : ID_SCALABILITY_CPU )


/* 
 * IP address and port number max length definition
 */
#define IDL_IP_ADDR_MAX_LEN 64
#define IDL_IP_PORT_MAX_LEN 8


/*
 * Values of NET_CONN_IP_STACK conf param
 */
#define IDL_NET_CONN_IP_STACK_V4    0
#define IDL_NET_CONN_IP_STACK_DUAL  1
#define IDL_NET_CONN_IP_STACK_V6    2

//  right now, all linux platforms use gcc as a compiler.
/*
 * BUG-35758 [id] add branch prediction to AIX for the performance.
 */
#if defined(OS_LINUX_KERNEL) || ( defined(IBM_AIX) && ( __xlC__  >= 0x0a01) )
#define IDL_LIKELY_TRUE(x)       __builtin_expect((long int)(x), 1)
#define IDL_LIKELY_FALSE(x)      __builtin_expect((long int)(x), 0)
#else
#define IDL_LIKELY_TRUE(x)       (x)
#define IDL_LIKELY_FALSE(x)      (x)
#endif

#endif /* IDL_H */

