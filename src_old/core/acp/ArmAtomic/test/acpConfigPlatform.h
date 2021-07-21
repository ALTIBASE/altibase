/* ./src/core/include/acpConfigPlatform.h.  Generated from acpConfigPlatform.h.in by configure.  */
#if !defined(_O_ACP_CONFIG_PLATFORM_H_)
#define _O_ACP_CONFIG_PLATFORM_H_


/* CPU Information */

//#define ALTI_CFG_CPU "X86"
/* #undef ALTI_CFG_CPU_ALPHA */
/* #undef ALTI_CFG_CPU_IA64 */
/* #undef ALTI_CFG_CPU_PARISC */
/* #undef ALTI_CFG_CPU_POWERPC */
/* #undef ALTI_CFG_CPU_SPARC */
//#define ALTI_CFG_CPU_X86 1
#define ALTI_CFG_CPU_ARM64 1

/* COMPILE BIT */

#define ALTI_CFG_BITTYPE "64"
/* #undef ALTI_CFG_BITTYPE_32 */
#define ALTI_CFG_BITTYPE_64 1

/* OS Information */

#define ALTI_CFG_OS "LINUX"
/* #undef ALTI_CFG_OS_AIX */
/* #undef ALTI_CFG_OS_CYGWIN */
/* #undef ALTI_CFG_OS_HPUX */
/* #undef ALTI_CFG_OS_ITRON */
#define ALTI_CFG_OS_LINUX 1
/* #undef ALTI_CFG_OS_QNX */
/* #undef ALTI_CFG_OS_SOLARIS */
/* #undef ALTI_CFG_OS_TRU64 */
/* #undef ALTI_CFG_OS_WINDOWS */
/* #undef ALTI_CFG_OS_FREEBSD */
/* #undef ALTI_CFG_OS_DARWIN */

/* OS Version */

#define ALTI_CFG_OS_MAJOR 4
#define ALTI_CFG_OS_MINOR 13

/* Byte Order */

/* #undef ACP_CFG_BIG_ENDIAN */
#define ACP_CFG_LITTLE_ENDIAN 1

/* Library Extension Set */

#define ACP_DL_MOD_FULL_FORMAT "%s/%s"
#define ACP_DL_MOD_NAME_FORMAT "%s"
#define ACP_DL_LIB_FULL_FORMAT "%s/lib%s.so"
#define ACP_DL_LIB_NAME_FORMAT "lib%s.so"
#define ACP_DL_LIB_EXT "so"

/* Binary Extension Set */

#define ACP_PROC_EXE_NAME_FORMAT "%s"
#define ACP_PROC_EXE_EXT ""

/* #undef ACP_CFG_AIX_USEPOLL */

/* C Library Support */

#define HAVE_ECVT_R 1
/* #undef HAVE_ECVT_S */
/* #undef HAVE_ECVT */
#define HAVE_FCVT_R 1
/* #undef HAVE_FCVT_S */
/* #undef HAVE_FCVT */
/* #undef HAVE_GCVT_S */
#define HAVE_GCVT 1

/* Access Environment Variables */

#define ALTIBASE_ENV_PREFIX "ALTIBASE_"

#endif
