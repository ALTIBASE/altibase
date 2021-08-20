/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpDescResource.cpp 91426 2021-08-04 04:40:23Z jake.jang $
 *
 * Description:
 *
 * A3에서 A4로 오면서, 수정된 것.
 * 1. 단위 변경 : LOCK_ESCALATION_MEMORY_SIZE (M에서 그냥 byte로 )
 *                그냥 크기로 사용.  1024 * 1024 만큼 곱하기 제거.
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>

#include <idpUInt.h>
#include <idpSInt.h>
#include <idpULong.h>
#include <idpSLong.h>
#include <idpString.h>

#include <idp.h>
#include <idvTime.h>
#include <idvAuditDef.h>

// To Fix PR-12035
// Page Size = 8K
// 해당 값은 smDef.h 의 SD_PAGE_SIZE와 동일한 값이어야 한다.
#if defined(SMALL_FOOTPRINT)
#define IDP_SD_PAGE_SIZE  ( 4 * 1024 )
#else
#define IDP_SD_PAGE_SIZE  ( 8 * 1024 )
#endif
#define IDP_DRDB_DATAFILE_MAX_SIZE  ID_ULONG( 32 * 1024 * 1024 * 1024 ) // 32G

// Direct I/O 최대 Page크기 = 8K
// smDef.h의 SM_MAX_DIO_PAGE_SIZE 와 동일한 값이어야 한다.
#define IDP_MAX_DIO_PAGE_SIZE ( 8 * 1024 )

// Memory table의 page 크기
// SM_PAGE_SIZE와 동일한 값이어야 한다.
#if defined(SMALL_FOOTPRINT)
#define IDP_SM_PAGE_SIZE  ( 4 * 1024 )
#else
#define IDP_SM_PAGE_SIZE  ( 32 * 1024 )
#endif

// LFG 최대값 = 1
// smDef.h의 SM_LFG_COUNT와 동일한 값이어야 한다.
#if defined(SMALL_FOOTPRINT)
#define IDP_MAX_LFG_COUNT 1
#else
/* Task 6153 */
#define IDP_MAX_LFG_COUNT 1
#endif

/* Task 6153 */
// PAGE_LIST 최대값 = 32
// smDef.h의 SM_MAX_PAGELIST_COUNT와 동일한 값이어야 한다.
#if defined(SMALL_FOOTPRINT)
#define IDP_MAX_PAGE_LIST_COUNT 1
#else
#define IDP_MAX_PAGE_LIST_COUNT 32
#endif



// Direct I/O 최대 Page크기 = 8K
// smDef.h의 SM_MAX_DIO_PAGE_SIZE 와 동일한 값이어야 한다.
#define IDP_MAX_DIO_PAGE_SIZE ( 8 * 1024 )

// EXPAND_CHUNK_PAGE_COUNT 프로퍼티의 기본값
// 이 값을 기준으로 SHM_PAGE_COUNT_PER_KEY 프로퍼티의
// 기본값도 함께 변경된다.
#if defined(SMALL_FOOTPRINT)
#define IDP_DEFAULT_EXPAND_CHUNK_PAGE_COUNT (32)
#else
#define IDP_DEFAULT_EXPAND_CHUNK_PAGE_COUNT (128)
#endif

// 최대 트랜잭션 갯수
#define IDP_MAX_TRANSACTION_COUNT (16384) // 2^14

// 최소 트랜잭션 갯수
// BUG-28565 Prepared Tx의 Undo 이후 free trans list rebuild 중 비정상 종료
// 최소 transaction table size를 16으로 정함(기존 0)
#define IDP_MIN_TRANSACTION_COUNT (16)

// LOB In Mode Max Size (BUG-30101)
// smDef.h의 SM_LOB_MAX_IN_ROW_SIZE와 동일한 값이어야 한다.
#define IDP_MAX_LOB_IN_ROW_SIZE (4000)

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
// This macro returns the number of processor within the value of MIN/MAX.
#define IDP_LIMITED_CORE_COUNT( MIN, MAX ) ( (((idlVA::getProcessorCount() < MIN)? MIN : idlVA::getProcessorCount()) < MAX) ? \
            ((idlVA::getProcessorCount() < MIN)? MIN : idlVA::getProcessorCount()) : MAX )

/* ------------------------------------------------
 *  Defined Default Mutex/Latch Type
 *  (Posix, Native)
 *
 *    IDU_MUTEX_KIND_POSIX = 0,
 *    IDU_MUTEX_KIND_NATIVE,
 *
 *    IDU_LATCH_TYPE_POSIX = 0,
 *    IDU_LATCH_TYPE_NATIVE,
 *
 * ----------------------------------------------*/

/*
  for MM

  alter system set IDU_SOURCE_INFO = n ; // callback
  alter system set AUTO_COMMIT = n ;
  alter system set __SHOW_ERROR_STACK = n ;
 */
static void *idpGetObjMem(UInt aSize)
{
    void    *sMem;
    sMem = (void *)iduMemMgr::mallocRaw(aSize);

    IDE_ASSERT(sMem != NULL);
    return sMem;
}

#define IDP_DEF(type, name, attr, min, max, default) \
{\
    idpBase *sType;\
    void    *sMem;\
    sMem = idpGetObjMem(sizeof(idp##type));\
    sType = new (sMem) idp##type(name, attr | IDP_ATTR_TP_##type, (min), (max), (default));     \
    IDE_TEST(idp::regist(sType) != IDE_SUCCESS);\
}

/*
 * Properties about database link are added to here.
 */ 
static IDE_RC registDatabaseLinkProperties( void )
{

    IDP_DEF(UInt, "DK_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(String, "DK_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_dk.log");

    IDP_DEF(UInt, "DK_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "DK_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "DBLINK_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "DBLINK_DATA_BUFFER_BLOCK_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 128);

    IDP_DEF(UInt, "DBLINK_DATA_BUFFER_BLOCK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 2);

    IDP_DEF(UInt, "DBLINK_DATA_BUFFER_ALLOC_RATIO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100, 50);

    IDP_DEF(UInt, "DBLINK_GLOBAL_TRANSACTION_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1);

    IDP_DEF(UInt, "GLOBAL_TRANSACTION_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 1);

    IDP_DEF(UInt, "DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "DBLINK_ALTILINKER_CONNECT_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 100);

    IDP_DEF(UInt, "DBLINK_REMOTE_TABLE_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 50);

    IDP_DEF(UInt, "DBLINK_RECOVERY_MAX_LOGFILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

     return IDE_SUCCESS;
     
     IDE_EXCEPTION_END;
     
     return IDE_FAILURE;    
}

/* ------------------------------------------------------------------
 *   아래의 함수에 등록할 프로퍼티의 데이타 형 및 범위를 등록하면 됨.
 *   IDP_DEF(타입, 이름, 속성, 최소,최대,기본값) 형태임.
 *   현재 지원되는 데이타 타입
 *   UInt, SInt, ULong, SLong, String
 *   속성 : 외부/내부 , 읽기전용/쓰기, 단일값/다수값,
 *          값범위 검사허용/검사거부 , 데이타 타입
 *
 *
 *   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!경고!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11

 *   idpString()을 정의할 때 NULL을 넘기지 마시오. 대신 ""을 넘기시오.

 *   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!경고!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11

 * ----------------------------------------------------------------*/


IDE_RC registProperties()
{
    UInt sLogFileAlignSize ;
    UInt sDefaultMutexSpinCnt = 0;      //BUG-46911

    /* !!!!!!!!!! REGISTRATION AREA BEGIN !!!!!!!!!! */

    // ==================================================================
    // ID Properties
    // ==================================================================
    IDP_DEF(UInt, "PORT_NO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024,   /* min */
            65535,  /* max */
            20300); /* default */

    IDP_DEF(UInt, "IPC_CHANNEL_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65535, 0);

#if defined(ALTI_CFG_OS_LINUX)
    IDP_DEF(UInt, "IPCDA_CHANNEL_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65535, 0);
#else
    IDP_DEF(UInt, "IPCDA_CHANNEL_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 0, 0);
#endif

    /*PROJ-2616*/
    IDP_DEF(UInt, "IPCDA_SERVER_SLEEP_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1000, 400);

    /*PROJ-2616*/
    IDP_DEF(UInt, "IPCDA_MESSAGEQ_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /*PROJ-2616*/
    IDP_DEF(UInt, "IPCDA_SERVER_MESSAGEQ_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65535, 100);

    /*PROJ-2616*/
    IDP_DEF(UInt, "IPCDA_DATABLOCK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32, 102400, 20480);

    IDP_DEF(UInt, "SOURCE_INFO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 0);

    IDP_DEF(String, "UNIXDOMAIN_FILEPATH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL | /* BUG-43142 */
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc"IDL_FILE_SEPARATORS"cm-unix");

    /* BUG-35332 The socket files can be moved */
    IDP_DEF(String, "IPC_FILEPATH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL | /* BUG-43142 */
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc"IDL_FILE_SEPARATORS"cm-ipc");

    IDP_DEF(String, "IPCDA_FILEPATH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc"IDL_FILE_SEPARATORS"cm-ipcda");

    IDP_DEF(UInt, "NET_CONN_IP_STACK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    /*
     * PROJ-2473 SNMP 지원
     *
     * LINUX만 SNMP를 지원하자.
     */
#if defined(ALTI_CFG_OS_LINUX)
    IDP_DEF(UInt, "SNMP_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
#else
    IDP_DEF(UInt, "SNMP_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 0, 0);
#endif

    IDP_DEF(UInt, "SNMP_PORT_NO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, 65535, 20400);

    IDP_DEF(UInt, "SNMP_TRAP_PORT_NO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, 65535, 20500);

    /* milliseconds */
    IDP_DEF(UInt, "SNMP_RECV_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 1000);

    /* milliseconds */
    IDP_DEF(UInt, "SNMP_SEND_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 100);

    IDP_DEF(UInt, "SNMP_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 3);

    IDP_DEF(String, "SNMP_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_snmp.log");

    IDP_DEF(UInt, "SNMP_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "SNMP_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    /* 
     * SNMP ALARM Setting - SNMP SET으로만 설정할 수 있다. 그래서 READONLY이다.
     *
     * 0: no send alarm,
     * 1: send alarm
     */
    IDP_DEF(UInt, "SNMP_ALARM_QUERY_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "SNMP_ALARM_UTRANS_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "SNMP_ALARM_FETCH_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* 
     * 0: no send alarm,
     * x: x회 연속으로 실패가 일어난 경우 
     */
    IDP_DEF(UInt, "SNMP_ALARM_SESSION_FAILURE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, UINT_MAX, 3);

    IDP_DEF(UInt, "SHM_POLICY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

#ifdef COMPILE_64BIT
    IDP_DEF(ULong, "SHM_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            64 * 1024 * 1024 /* 64 MB */, ID_ULONG_MAX, ID_ULONG(8 * 1024 * 1024 * 1024) /* 8G */);
#else
    IDP_DEF(ULong, "SHM_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            64 * 1024 * 1024 /* 64 MB */,
            ID_ULONG(4 * 1024 * 1024 * 1024) /* 4G */,
            ID_ULONG(4 * 1024 * 1024 * 1024) /* 4G */);
#endif

    IDP_DEF(UInt, "SHM_LOGGING",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,  /* min */
            1,  /* max */
            1); /* default */

    IDP_DEF(UInt, "__USE_DUMP_CALLSTACKS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);    //min,max,default

    IDP_DEF(UInt, "SHM_LOCK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,  /* min */
            1,  /* max */
            0); /* default */

    IDP_DEF(UInt, "SHM_LATCH_SPIN_LOCK_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,  /* min */
            ID_UINT_MAX,  /* max */
            1 );

    IDP_DEF(UInt, "SHM_LATCH_YIELD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,  /* min */
            ID_UINT_MAX,  /* max */
            1 );

    IDP_DEF(UInt, "SHM_LATCH_MAX_YIELD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,  /* min */
            ID_UINT_MAX,  /* max */
            1000 );

    IDP_DEF(UInt, "SHM_LATCH_SLEEP_DURATION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,  /* min = 0usec */
            1000000,  /* max=1000000usec=1sec */
            0 );

    IDP_DEF(UInt, "USER_PROCESS_CPU_AFFINITY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,  /* min */
            1,  /* max */
            0); /* default */

    //----------------------------------
    // NLS Properties
    //----------------------------------

    // fix BUG-13838
    // hangul collation ( ksc5601, ms949 )
    IDP_DEF(UInt, "NLS_COMP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // PROJ-1579 NCHAR
    IDP_DEF(UInt, "NLS_NCHAR_CONV_EXCP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // PROJ-1579 NCHAR
    IDP_DEF(UInt, "NLS_NCHAR_LITERAL_REPLACE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

#if defined(ALTIBASE_PRODUCT_HDB)
    // fix BUG-21266, BUG-29501
    IDP_DEF(UInt, "DEFAULT_THREAD_STACK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1048576,     /* min */
            1048576 * 128,  /* max */
            10 * 1048576);  /* default 10M */
#else
    // fix BUG-21266, BUG-29501
    IDP_DEF(UInt, "DEFAULT_THREAD_STACK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            65536,     /* min */
            10485760,  /* max */
            10485760);  /* default */
#endif

    // fix BUG-21547
    IDP_DEF(UInt, "USE_MEMORY_POOL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,   /* min */
            1,  /* max */
            1); /* default */

    //----------------------------------
    // For QP
    //----------------------------------

    IDP_DEF(UInt, "TRCLOG_DML_SENTENCE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "TRCLOG_DETAIL_PREDICATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "TRCLOG_DETAIL_MTRNODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "TRCLOG_EXPLAIN_GRAPH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // PROJ-2179
    IDP_DEF(UInt, "TRCLOG_RESULT_DESC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-38192
    // PROJ-2402 에서 이름 변경
    IDP_DEF(UInt, "TRCLOG_DISPLAY_CHILDREN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // PROJ-1358, BUG-12544
    // Min : 8, Max : 65536, Default : 128
    IDP_DEF(UInt, "QUERY_STACK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            8, 65536, 1024);

    // PROJ-1446 Host variable을 포함한 질의 최적화
    IDP_DEF(UInt, "HOST_OPTIMIZE_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-13068 session당 open할 수 있는 filehandle개수 제한
    // Min : 0, Max : 128, Default : 16
    IDP_DEF(UInt, "PSM_FILE_OPEN_LIMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 128, 16);

    // BUG-40854
    // Min : 0, Max : 128, Default : 16
    IDP_DEF(UInt, "CONNECT_TYPE_OPEN_LIMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 128, 16);

    /* BUG-41307 User Lock 지원 */
    // Min : 128, Max : 10000, Default : 128
    IDP_DEF(UInt, "USER_LOCK_POOL_INIT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            128, 100000, 128);

    /* BUG-41307 User Lock 지원 */
    // Min : 0, Max : (2^32)-1, Default : 10
    IDP_DEF(UInt, "USER_LOCK_REQUEST_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    /* BUG-41307 User Lock 지원 */
    // Min : 10, Max : 999999, Default : 10000
    IDP_DEF(UInt, "USER_LOCK_REQUEST_CHECK_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10, 999999, 10000);

    /* BUG-41307 User Lock 지원 */
    // Min : 0, Max : 10000, Default : 10
    IDP_DEF(UInt, "USER_LOCK_REQUEST_LIMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 10000, 10);
    
    // BUG-35713
    // sql로 부터 invoke되는 function에서 발생하는 no_data_found 에러무시
    IDP_DEF(UInt, "PSM_IGNORE_NO_DATA_FOUND_ERROR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // PROJ-1557 varchar 32k 지원
    // Min : 0, Max : 4000, Default : 32
    IDP_DEF(UInt, "MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 4000, 32);

    // PROJ-1362 LOB
    // Min : 0, Max : 4000, Default : 64
    IDP_DEF(UInt, "MEMORY_LOB_COLUMN_IN_ROW_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_LOB_IN_ROW_SIZE, 64);

    // PROJ-1862 Disk In Mode LOB In Row Size
    IDP_DEF(UInt, "DISK_LOB_COLUMN_IN_ROW_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_LOB_IN_ROW_SIZE, 3900);

    /* PROJ-1530 PSM, Trigger에서 LOB 데이타 타입 지원 */
    IDP_DEF(UInt, "LOB_OBJECT_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32000, 104857600, 32000);

    /* PROJ-1530 PSM, Trigger에서 LOB 데이타 타입 지원 */
    IDP_DEF(UInt, "__INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            33554432, 1073741824, 33554432);

    // BUG-18851 disable transitive predicate generation
    IDP_DEF(UInt, "__OPTIMIZER_TRANSITIVITY_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    IDP_DEF(UInt, "__OPTIMIZER_TRANSITIVITY_OLD_RULE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // PROJ-1473
    IDP_DEF(UInt, "__OPTIMIZER_PUSH_PROJECTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    IDP_DEF(UInt, "__OPTIMIZER_DEFAULT_TEMP_TBS_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0 );

    // BUG-38132 group by의 temp table 을 메모리로 고정하는 프로퍼티
    IDP_DEF(UInt, "__OPTIMIZER_FIXED_GROUP_MEMORY_TEMP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // BUG-38339 Outer Join Elimination
    IDP_DEF(UInt, "__OPTIMIZER_OUTERJOIN_ELIMINATION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    // PROJ-1413 Simple View Merging
    IDP_DEF(UInt, "__OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // BUG-34234
    // target절에 사용된 호스트 변수를 varchar type으로 강제 고정한다.
    IDP_DEF(UInt, "COERCE_HOST_VAR_IN_SELECT_LIST_TO_VARCHAR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 32000, 0 );

    // BUG-19089 FK가 있는 상태에서 CREATE REPLICATION 구문이 가능하도록 한다.
    // Min : 0, Max : 1, Default : 0
    IDP_DEF(UInt, "CHECK_FK_IN_CREATE_REPLICATION_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // BUG-29209
    // natc test를 위하여 Plan Display에서
    // 특정 정보를 보여주지 않게 하는 프로퍼티
    IDP_DEF(UInt, "__DISPLAY_PLAN_FOR_NATC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-34342
    // 산술연산 모드선택
    // precision mode : 0
    // C type과 numeric type의 연산시 numeric type으로 계산한다.
    // performance mode : 1
    // C type과 numeric type의 연산시 C type으로 계산한다.
    IDP_DEF(UInt, "ARITHMETIC_OPERATION_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 1 ); /* BUG-46195 */

    /* PROJ-1090 Function-based Index */
    IDP_DEF(UInt, "QUERY_REWRITE_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* BUG-32101
     * Disk table에서 index scan의 cost는 property를 통해 조절할 수 있다.
     */
    IDP_DEF(SInt, "OPTIMIZER_DISK_INDEX_COST_ADJ",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 10000, 100);

    // BUG-43736
    IDP_DEF(SInt, "OPTIMIZER_MEMORY_INDEX_COST_ADJ",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 10000, 100);

    /*
     * BUG-34441: Hash Join Cost를 조정할 수 있는 프로퍼티
     */
    IDP_DEF(UInt, "OPTIMIZER_HASH_JOIN_COST_ADJ",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 10000, 100);

    /* BUG-34295
     * ANSI style join 중 inner join 을 join ordering 대상으로
     * 처리하여 보다 나은 plan 을 생성한다. */
    IDP_DEF(SInt, "OPTIMIZER_ANSI_JOIN_ORDERING",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-38402
    // ansi 스타일 inner 조인만 있을경우에는 일반 타입으로 변환한다.
    IDP_DEF(SInt, "__OPTIMIZER_ANSI_INNER_JOIN_CONVERT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-40022
    // 1: NL, 2: HASH, 4: SORT, 8: MERGE
    // HASH+MERGE : 10
    IDP_DEF(UInt, "__OPTIMIZER_JOIN_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 14, 8);

    // BUG-40492,BUG-45447
    // iduMemPool의 Slot 개수의 최소값을 설정한다.
    IDP_DEF( UInt, "__MEMPOOL_MINIMUM_SLOT_COUNT",
             IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             2, 5000, 5 );

    /* BUG-34350
     * stepAfterOptimize 단계에서 tuple 의 사용되지 않는 메모리를
     * 해제하고 재할당하는 과정을 생략하여 prepare 시간을 줄인다. */
    IDP_DEF(SInt, "OPTIMIZER_REFINE_PREPARE_MEMORY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /*   
     * BUG-34235: in subquery keyrange tip을 가능한 경우 항상 적용
     *
     * 0: AUTO
     * 1: InSubqueryKeyRange
     * 2: TransformNJ
     */
    IDP_DEF(UInt, "OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1);

    /*
     * PROJ-2242 : eliminate common subexpression 이 가능한 경우 항상 적용
     *     
     * 0 : CSE OFF mode
     * 1 : CSE ON mode
     * 2 : PATIAL CSE ON mode ( BUG-48348 )
     *     - where절이 CNF로 만드는 경우 CSE를 진행하지 않음
     */
    IDP_DEF(UInt, "__OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1);

    /*
     * PROJ-2242 : constant filter subsumption 이 가능한 경우 항상 적용
     *
     * 0 : FALSE
     * 1 : TRUE
     */
    IDP_DEF(UInt, "__OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // PROJ-2242
    // 버젼을 입력받아서 미리 정의된 프로퍼티를 일괄 변경
    IDP_DEF(String, "OPTIMIZER_FEATURE_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)IDU_ALTIBASE_VERSION_STRING );  // max feature version

    // BUG-38434
    IDP_DEF(UInt, "__OPTIMIZER_DNF_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // BUG-38416 count(column) to count(*)
    IDP_DEF(UInt, "OPTIMIZER_COUNT_COLUMN_TO_COUNT_ASTAR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-36438 LIST transformation
    IDP_DEF(UInt, "__OPTIMIZER_LIST_TRANSFORMATION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

#if defined(ALTIBASE_PRODUCT_XDB)
    // BUG-41795
    IDP_DEF(UInt, "__DA_DDL_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
#endif

    // fix BUG-42752    
    IDP_DEF(UInt, "__OPTIMIZER_ESTIMATE_KEY_FILTER_SELECTIVITY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-43059 Target subquery unnest/removal disable
    IDP_DEF(UInt, "__OPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "__OPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "OPTIMIZER_DELAYED_EXECUTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    //---------------------------------------------
    // PR-13395 TPC-H ??? ?? ??
    //---------------------------------------------
    // TPC-H SCALE FACTOR? ?? ?? ?? ?? ??
    // 0 : ???? ??.
    IDP_DEF(UInt, "__QP_FAKE_STAT_TPCH_SCALE_FACTOR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65536, 0);

    // ?? BUFFER SIZE
    // 0 : ???? ??.
    IDP_DEF(UInt, "__QP_FAKE_STAT_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    //---------------------------------------------
    // PROJ-2002 Column Security
    //---------------------------------------------

    IDP_DEF(String, "SECURITY_MODULE_NAME",
            IDP_ATTR_SL_ALL       |
            IDP_ATTR_IU_IDENTICAL |
            IDP_ATTR_MS_ANY       |
            IDP_ATTR_SK_ASCII     |
            IDP_ATTR_LC_EXTERNAL  |
            IDP_ATTR_RD_WRITABLE  |
            IDP_ATTR_ML_JUSTONE   |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(String, "SECURITY_MODULE_LIBRARY",
            IDP_ATTR_SL_ALL       |
            IDP_ATTR_IU_IDENTICAL |
            IDP_ATTR_MS_ANY       |
            IDP_ATTR_SK_PATH      |
            IDP_ATTR_LC_EXTERNAL  |
            IDP_ATTR_RD_WRITABLE  |
            IDP_ATTR_ML_JUSTONE   |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(String, "SECURITY_ECC_POLICY_NAME",
            IDP_ATTR_SL_ALL       |
            IDP_ATTR_IU_IDENTICAL |
            IDP_ATTR_MS_ANY       |
            IDP_ATTR_SK_ASCII     |
            IDP_ATTR_LC_EXTERNAL  |
            IDP_ATTR_RD_WRITABLE  |
            IDP_ATTR_ML_JUSTONE   |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(UInt, "__KEY_PRESERVED_TABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
    

    //----------------------------------
    // For SM
    //----------------------------------
    IDP_DEF(UInt, "TRCLOG_SET_LOCK_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // fix BUG-33589
    IDP_DEF(UInt, "PLAN_REBUILD_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "DB_LOGGING_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* PROJ-2361 */
    IDP_DEF(UInt, "__OPTIMIZER_AVG_TRANSFORM_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /*
     *-----------------------------------------------------
     * PROJ-1071 Parallel Query
     *-----------------------------------------------------
     */

    IDP_DEF(UInt, "PARALLEL_QUERY_THREAD_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024, IDL_MIN(idlVA::getProcessorCount(), 1024));

    /* queue 대기 시간: 단위는 micro-sec */
    IDP_DEF(UInt, "PARALLEL_QUERY_QUEUE_SLEEP_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            200, 1000000, 500000);

    /* PRLQ queue size */
    IDP_DEF(UInt, "PARALLEL_QUERY_QUEUE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            4, 1048576, 1024);

    /* PROJ-2462 Reuslt Cache */
    IDP_DEF(UInt, "RESULT_CACHE_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* PROJ-2462 Reuslt Cache */
    IDP_DEF(UInt, "TOP_RESULT_CACHE_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 0 );

    /* PROJ-2462 Reuslt Cache */
    IDP_DEF(ULong, "RESULT_CACHE_MEMORY_MAXIMUM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (4096),            // min : 4K
            ID_ULONG_MAX,      // max
            (1024*1024*10));   // default : 10M

    /* PROJ-2462 Reuslt Cache */
    IDP_DEF(UInt, "TRCLOG_DETAIL_RESULTCACHE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /*
     *  Force changes parallel degree when table created.
     *  HIDDEN PROPERTY
     */
    IDP_DEF(UInt, "__FORCE_PARALLEL_DEGREE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65535, 1);

    /* PROJ-2452 Caching for DETERMINISTIC Function */

    // Cache object max count for caching
    IDP_DEF(UInt, "__QUERY_EXECUTION_CACHE_MAX_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65536, 65536 );
 
    // Cache memory max size for caching
    IDP_DEF(UInt, "__QUERY_EXECUTION_CACHE_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 10 * 1024 * 1024, 65536 );

    // Hash bucket count for caching
    IDP_DEF(UInt, "__QUERY_EXECUTION_CACHE_BUCKET_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 512, 61 );

    /* 
     * HIDDEN PROPERTY (only natc test)
     *  1 : Force cache of basic function
     *  2 : Force DETERMINISTIC option
     */
    IDP_DEF(UInt, "__FORCE_FUNCTION_CACHE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    /* PROJ-2448 Subquery caching */
    // Force subquery cache disable
    IDP_DEF(UInt, "__FORCE_SUBQUERY_CACHE_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* ORDER BY Elimination 
     * value| 의미
     *   0  | OBYE off
     *   1  | BUG-41183 target절에 count(*)일 때 인라인뷰에 ORDER BY 제거
     *   2  | BUG-48941 where 절의 서브쿼리 프리디킷의 인라인뷰에 ORDER BY 제거
     *   3  | 1+2
     */
    IDP_DEF(UInt, "__OPTIMIZER_ORDER_BY_ELIMINATION_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 1);

    // BUG-41249 DISTINCT Elimination
    IDP_DEF(UInt, "__OPTIMIZER_DISTINCT_ELIMINATION_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // PROJ-2582 recursive with
    IDP_DEF(UInt, "RECURSION_LEVEL_MAXIMUM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,             // min
            ID_UINT_MAX,   // max
            1000 );        // default

    // PROJ-2750 LEFT OUTER SKIP RIGHT
    IDP_DEF(UInt, "__LEFT_OUTER_SKIP_RIGHT_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    //----------------------------------
    // For ST
    //----------------------------------
    
    // PROJ-1583 large geometry
    // Min : 32000 Max : 100M, Default : 32000
    IDP_DEF(UInt, "ST_OBJECT_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32000, 104857600, 32000);

    // BUG-22924 allow "ring has cross lines"
    // min : 0, max : 2, default : 0
    // 0 : disallow invalid object
    // 1 : allow invalid object level 1
    // 2 : allow invalid object levle 2
    IDP_DEF(UInt, "ST_ALLOW_INVALID_OBJECT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    // PROJ-2166 Spatial Validation
    // 0 : use GPC(General Polygon Clipper) Library
    // 1 : use Altibase Polygon Clipper Library
    IDP_DEF(UInt, "ST_USE_CLIPPER_LIBRARY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-33904 new clipping library tolerance level in st-function
    // 0 : 1e-4
    // 1 : 1e-5
    // 2 : 1e-6
    // 3 : 1e-7 (default)
    // 4 : 1e-8
    // 5 : 1e-9
    // 6 : 1e-10
    // 7 : 1e-11
    // 8 : 1e-12
    IDP_DEF(UInt, "ST_CLIP_TOLERANCE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 8, 3);
    
    // BUG-33576
    // spatial validation for insert geometry data
    // 0 : disable
    // 1 : enable
    IDP_DEF(UInt, "ST_GEOMETRY_VALIDATION_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
    
    //--------------------------------------------------------
    // Trace Log End
    //--------------------------------------------------------
    /* bug-37320 max of max_listen increased */
    IDP_DEF(UInt, "MAX_LISTEN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 16384, 128);

    IDP_DEF(ULong, "QP_MEMORY_CHUNK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, ID_ULONG_MAX, 64 * 1024);

    IDP_DEF(SLong, "ALLOCATION_RETRY_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, IDU_MEM_DEFAULT_RETRY_TIME);

    IDP_DEF(UInt, "MEMMGR_LOG_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    IDP_DEF(ULong, "MEMMGR_LOG_LOWERSIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 0);

    IDP_DEF(ULong, "MEMMGR_LOG_UPPERSIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 0);

    IDP_DEF( UInt, "DA_FETCH_BUFFER_SIZE",
             IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             ( 256 * 1024 ), ID_UINT_MAX, ( 256 * 1024 ) );

    //===================================================================
    // To Fix PR-13963
    // Min: 0 (대용량 힙 사용 내역을 로깅하지 않음)
    // Max: ID_UNIT_MAX
    // default : 0
    //===================================================================
    IDP_DEF(UInt, "INSPECTION_LARGE_HEAP_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // ==================================================================
    // SM Properties
    // ==================================================================

    // RECPOINT 복구테스트 활성화 프로퍼티 PRJ-1552
    // 비활성화 : 0, 활성화 : 1
    IDP_DEF(UInt, "ENABLE_RECOVERY_TEST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "ART_DECREASE_VAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // 데이타베이스 화일 이름
    IDP_DEF(String, "DB_NAME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_ASCII     |
            IDP_ATTR_LC_EXTERNAL  |
            IDP_ATTR_RD_READONLY  |
            IDP_ATTR_ML_JUSTONE   |
            IDP_ATTR_DF_DROP_DEFAULT | /* 반드시 입력해야 함. */
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_DBNAME_LEN, (SChar *)"");

    // 데이타 파일이 존재하는 경로
    // 반드시 3개 지정되어야 하고 반드시 입력 되어야 함.
    // 2개의 메모리 DB를 위한 경로와 1개의 디스크 DB를 위한 경로를
    // 지정해야 한다.
    IDP_DEF(String, "MEM_DB_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_DF_DROP_DEFAULT | /* 반드시 입력해야 함. */
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    // DISK의 데이타 파일이 존재하는 경로 :
    // 별도로 디렉토리를 지정하지 않을 경우 Default로 여기에 생긴다.
    IDP_DEF(String, "DEFAULT_DISK_DB_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"dbs");

    // 데이타베이스를 가상 메모리 공간에 사용할 경우 0으로,
    // 공유메모리 공간에 사용할 경우 shared memory key 값을 지정해야 한다.
    IDP_DEF(UInt, "SHM_DB_KEY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // PROJ-1548 Memory Tablespace
    // 사용자가 공유메모리 Chunk의 크기를 지정할 수 있도록 한다
    /*
      문제점 : 현재는 공유메모리 Chunk의 크기가 DB확장의 기본 단위인
                EXPAND_CHUNK_PAGE_COUNT로 되어 있다.
                이는 불필요하게 많은 공유메모리 Chunk를
                만들어 낼 가능성이 있다.

      해결책 : 공유메모리 Chunk 1개의 크기를 사용자가 지정할 수 있도록
                별도의 SHM_PAGE_COUNT_PER_KEY 프로퍼티로 뺀다.
    */
    IDP_DEF(UInt, "SHM_PAGE_COUNT_PER_KEY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            320  /* 10 Mbyte */,
            ID_UINT_MAX,
            3200 /* 100 Mbyte */ );

    // FreePageList에 유지할 Page의 최소 갯수
    IDP_DEF(UInt, "MIN_PAGES_ON_TABLE_FREE_LIST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 1);

    // DB에서 Table로 할당받아올 Page 갯수
    // 값이 0이면
    // FreePageList에 유지할 Page의 최소 갯수(MIN_PAGES_ON_TABLE_FREE_LIST)로
    // DB에서 가져온다.
    IDP_DEF(UInt, "TABLE_ALLOC_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    /*
     * BUG-25327 : [MDB] Free Page Size Class 개수를 Property화 해야 합니다.
     */
    IDP_DEF(UInt, "MEM_SIZE_CLASS_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 4, 4);

    IDP_DEF(UInt, "DB_LOCK_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "MAX_CID_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1000, ID_UINT_MAX, 10000);

    // FreePageList에 유지할 Page의 최소 갯수
    IDP_DEF(UInt, "TX_PRIVATE_BUCKET_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2, 64, 2);

    // 로그화일의 경로
    IDP_DEF(String, "LOG_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_DF_DROP_DEFAULT |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    // 로그 앵커 화일이 저장되는 경로
    IDP_DEF(String, "LOGANCHOR_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_EXACT_3 |
            IDP_ATTR_DF_DROP_DEFAULT |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    // archive 로그 저장 디렉토리
    IDP_DEF(String, "ARCHIVE_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_DF_DROP_DEFAULT |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    // archive 로그 디스크가 full발생한 경우 처리
    // 대처방법정의
    IDP_DEF(UInt, "ARCHIVE_FULL_ACTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // startup시 archive thread 자동 start
    // 여부.
    IDP_DEF(UInt, "ARCHIVE_THREAD_AUTOSTART",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // lock 이 획득되지 않았을 경우,
    // 지정된 usec 만큼 sleep 하고 다시 retry한다.
    // 내부속성
    IDP_DEF(ULong, "LOCK_TIME_OUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 50);
#if 0
    IDP_DEF(UInt, "LOCK_SELECT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
#endif
    IDP_DEF(UInt, "TABLE_LOCK_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-42928 No Partition Lock */
    IDP_DEF(UInt, "TABLE_LOCK_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "TABLESPACE_LOCK_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // PROJ-1784 DML without retry
    IDP_DEF(UInt, "__DML_WITHOUT_RETRY_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // 로그파일의 크기
    // PR-14475 Group Commit
    // Direct I/O시에 로그파일을 Disk로 내리는 기본 단위는
    // Direct I/O Page 크기이다.
    // 사용자가 프로퍼티 변경하여 Direct I/O Page크기를
    // 언제든지 변경가능하므로  Direct I/O Page의 최대 크기인
    // 8K 단위혹은 mmap시 Align단위가 되는 Virtual Page의 크기인
    // idlOS::getpagesize()값중 큰값으로 로그파일 크기가 Align되도록 한다
    sLogFileAlignSize= (UInt)( (idlOS::getpagesize() > IDP_MAX_DIO_PAGE_SIZE) ?
                               idlOS::getpagesize():
                               IDP_MAX_DIO_PAGE_SIZE ) ;

    IDP_DEF(ULong, "LOG_FILE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_AL_SET_VALUE( sLogFileAlignSize ) |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            64 * 1024, ID_ULONG_MAX, 10 * 1024 * 1024);

    /* BUG-42930 LogFile Prepare Thread가 동작 도중 Server Kill 이 일어날 경우
     * LogFile Size가 0인 파일이 있을수 있습니다. Server Kill Test를 대비해
     * Size 0인 로그파일을 알아서 지우고 Server를 띄울수 있는 Property를 제공합니다. */
    IDP_DEF(UInt, "__ZERO_SIZE_LOG_FILE_AUTO_DELETE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);


    // 체크 포인트 주기 (초단위)
    IDP_DEF(UInt, "CHECKPOINT_INTERVAL_IN_SEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            3, IDV_MAX_TIME_INTERVAL_SEC, 6000);

    // 체크 포인트 주기( 로그 화일이 생성되는 회수)
    // 정해진 회수만큼 로그화일이 교체되면 체크포인트를 수행한다.
    IDP_DEF(UInt, "CHECKPOINT_INTERVAL_IN_LOG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 100);

    /* BUG-36764, BUG-40137
     * checkpoint가 발생했을때 checkpoint-flush job을 수행할 대상.
     * 0: flusher
     * 1: checkpoint thread */
    IDP_DEF(UInt, "__CHECKPOINT_FLUSH_JOB_RESPONSIBILITY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // PROJ-1566: Direct Path Buffer Flush Interval
    IDP_DEF(UInt, "__DIRECT_BUFFER_FLUSH_THREAD_SYNC_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, IDV_MAX_TIME_INTERVAL_USEC, 100000);

    // 동적으로 늘어날 수 있는 DB의 크기를 명시
#ifdef COMPILE_64BIT
    IDP_DEF(ULong, "MEM_MAX_DB_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2*IDP_MAX_PAGE_LIST_COUNT*IDP_SM_PAGE_SIZE, // min : 최소 EXPAND_CHUNK_PAGE_COUNT*32K보단 커야 한다.
            ID_ULONG_MAX,                           // max
            ID_ULONG(2 * 1024 * 1024 * 1024));      // default, 2G

    // BUG-17216
    IDP_DEF(ULong, "VOLATILE_MAX_DB_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2*IDP_MAX_PAGE_LIST_COUNT*IDP_SM_PAGE_SIZE, // min : 최소 EXPAND_CHUNK_PAGE_COUNT*32K보단 커야 한다.
            ID_ULONG_MAX,                         // max
            (ULong)ID_UINT_MAX  + 1);             // default

#else
    IDP_DEF(ULong, "MEM_MAX_DB_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2*IDP_MAX_PAGE_LIST_COUNT*IDP_SM_PAGE_SIZE,  // min : 최소 EXPAND_CHUNK_PAGE_COUNT*32K보단 커야 한다.
            (ULong)ID_UINT_MAX + 1,                // max
            ID_ULONG(2 * 1024 * 1024 * 1024));     // default, 2G

    // BUG-17216
    IDP_DEF(ULong, "VOLATILE_MAX_DB_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2*IDP_MAX_PAGE_LIST_COUNT*IDP_SM_PAGE_SIZE,  // min : 최소 EXPAND_CHUNK_PAGE_COUNT*32K보단 커야 한다.
            (ULong)ID_UINT_MAX + 1,                // max
            (ULong)ID_UINT_MAX + 1);               // default

#endif

    /* TASK-6327 New property for disk size limit */
    IDP_DEF(ULong, "DISK_MAX_DB_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2*IDP_MAX_LFG_COUNT*IDP_SM_PAGE_SIZE, // min : 최소 EXPAND_CHUNK_PAGE_COUNT*32K보단 커야 한다.
            ID_ULONG_MAX,                         // max
            ID_ULONG_MAX);                        // default
    /* TASK-6327 New property for license update */
    IDP_DEF(SInt, "__UPDATE_LICENSE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // Memory Tablespace의 기본 DB File (Checkpoint Image) 크기
    IDP_DEF(ULong, "DEFAULT_MEM_DB_FILE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_DEFAULT_EXPAND_CHUNK_PAGE_COUNT * IDP_SM_PAGE_SIZE, // min : chunk size
            ID_ULONG_MAX,                        // max
            IDP_DEFAULT_EXPAND_CHUNK_PAGE_COUNT * IDP_SM_PAGE_SIZE * 256);//default:1G

    // 임시 데이타 페이지를 한번에 할당하는 개수
    IDP_DEF(UInt, "TEMP_PAGE_CHUNK_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 128);

    // dirty page 리스트 유지를 위한 pre allocated pool 개수
    // 내부 속성
    IDP_DEF(UInt, "DIRTY_PAGE_POOL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 1024);

    /*BUG-32386  [sm_recovery] If the ager remove the MMDB slot and the
     *checkpoint thread flush the page containing the slot at same time, the
     *server can misunderstand that the freed slot is the allocated slot.
     *Ager가 MMDB Slot을 해제할때 동시에 Checkpoint Thread가 해당 page를
     *Flush할 경우, 서버가 Free된 Slot을 할당된 Slot이라고 잘못파악할 수
     *있습니다.
     *Checkpoint Flush 시점에 관한 재현을 하기 위해 Hidden Property 삽입.
     *DirtyPageFlush(DPFlush) 중 특정 page의 경우 특정 Offset까지만 기록,
     *Wait 후 다시 기록하여 TornPage를 의도적으로 생성하도록 함.
     *__MEM_DPFLUSH_WAIT_TIME    = 대기시간(초). 0일 경우, 이 기능 동작 안함.
     *__MEM_DPFLUSH_WAIT_SPACEID,PAGEID = TablespaceID 및 PageID. Page를 지정.
     *__MEM_DPFLUSH_WAIT_OFFSET  = 지정된 Page에 이 Offset까지 기록한 후
     *                             SLEEP만큼 대기 후 뒤쪽 offset을 복사함*/
    IDP_DEF(UInt, "__MEM_DPFLUSH_WAIT_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

    IDP_DEF(UInt, "__MEM_DPFLUSH_WAIT_SPACEID",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

    IDP_DEF(UInt, "__MEM_DPFLUSH_WAIT_PAGEID",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

    IDP_DEF(UInt, "__MEM_DPFLUSH_WAIT_OFFSET",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

    // 서버 재 실행시 index rebuild 과정에서 생성되는
    // index build thread의 개수
    IDP_DEF(UInt, "PARALLEL_LOAD_FACTOR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            //fix BUG-19787
            1, 512, IDL_MIN(idlVA::getProcessorCount() * 2, 512));

    // sdnn iterator mempool의 할당시 memlist 개수
    IDP_DEF(UInt, "ITERATOR_MEMORY_PARALLEL_FACTOR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,
            ID_UINT_MAX,
            2);

    // 동시에 생성될 수 있는 최대 트랜잭션 개수
    // BUG-28565 Prepared Tx의 Undo과정에서 free trans list rebuild 오류 발생
    // 최소 transaction table size를 16으로 정함(기존 0)
    IDP_DEF(UInt, "TRANSACTION_TABLE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_MIN_TRANSACTION_COUNT, IDP_MAX_TRANSACTION_COUNT, 1024);

    /* BUG-35019 TRANSACTION_TABLE_SIZE에 도달했었는지 trc 로그에 남긴다 */
    /* BUG-47655 0 == UInt Max , 0 추가함*/
    IDP_DEF(UInt, "__TRANSACTION_TABLE_FULL_TRCLOG_CYCLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 100000 );

    // 로그 압축 리소스 풀에 유지할 최소한의 리소스 갯수
    IDP_DEF(UInt, "MIN_COMPRESSION_RESOURCE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, IDP_MAX_TRANSACTION_COUNT, 16);

    // 로그 압축 리소스 풀에서 리소스가 몇 초 이상
    // 사용되지 않을 경우 Garbage Collection할지?
    //
    // 기본값 : 한시간동안 사용되지 않으면 가비지 콜렉션 실시.
    // 최대값 : ULong변수로 표현할 수 있는 Micro초의 최대값을
    //           초단위로 환산한값
    IDP_DEF(ULong, "COMPRESSION_RESOURCE_GC_SECOND",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_ULONG_MAX/1000000, 3600);


    // SM에서 사용되는 SCN의 disk write 주기를 결정
    // 이 값이 적을 경우 disk I/O가 많이 발생하며,
    // 이 값이 클 경우 그만큼 I/O가 적게 발생하는 대신
    // 사용가능한 SCN의 용량이 줄어든다.
    // 내부 속성
#if defined(ALTIBASE_PRODUCT_HDB)
    IDP_DEF(ULong, "SCN_SYNC_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            40, 8000000, 80000);
#else
    IDP_DEF(ULong, "SCN_SYNC_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            // PROJ-2446 bugbug
            10, 2000000, 20000);
#endif

    // isolation level을 지정한다.
    // 0 : read committed
    // 1 : repetable read
    // 2 : serialzable
    // 내부속성
    IDP_DEF(UInt, "ISOLATION_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    // BUG-15396
    // Commit Write Wait Mode를 지정한다.
    // 0 ( commit write no wait )
    //   : commit 시, log를 disk에 기록할때까지 기다리지 않음
    // 1 ( commit write wait )
    //   : commit 시, log를 disk에 기록할때까지 기다림
    IDP_DEF(UInt, "COMMIT_WRITE_WAIT_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // SHM_DB_KEY 값이 지정된 상태에서 알티베이스 시작시 생성되는
    // 공유 메모리 청크의 최대 크기 지정
    IDP_DEF(ULong, "STARTUP_SHM_CHUNK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, ID_ULONG_MAX, ID_ULONG(1 * 1024 * 1024 * 1024));

    // SHM_DB_KEY 값이 지정된 상태에서 알티베이스 시작시 생성되는
    // 공유 메모리 청크의 최대 크기 지정
    IDP_DEF(UInt, "SHM_STARTUP_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            ID_ULONG(32 * 1024 * 1024),       // 32M
            ID_ULONG(2 * 1024 * 1024 * 1024), // 2G
            ID_ULONG(512 * 1024 * 1024));     // 512M


    // 공유 메모리 청크의 크기
    IDP_DEF(UInt, "SHM_CHUNK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            ID_ULONG(32 * 1024 * 1024),       // 32M
            ID_ULONG(2 * 1024 * 1024 * 1024), // 2G
            ID_ULONG(512 * 1024 * 1024));     // 512M

    // SHM_DB_KEY 값이 지정된 상태에서 알티베이스 시작시 생성되는
    // 공유 메모리 청크의 Align Size
    IDP_DEF(UInt, "SHM_CHUNK_ALIGN_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, ID_ULONG(1 * 1024 * 1024 * 1024), ID_ULONG(1 * 1024 * 1024));

    // 미리 생성하는 로그화일의 개수
    IDP_DEF(UInt, "PREPARE_LOG_FILE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 5);
 
    /* BUG-48409 로그 파일을 미리 생성 할 때 temp파일을 활용할지 여부 
     * 0 : temp 파일을 생성하지 않고 logfile을 바로 만든다.
     * 1 : temp 파일을 생성해서 온전한 logfile이 완성 되면
     *     rename 으로 logfile123 으로 변경해서 등록 한다. (default)
     *     prepare 도중에 비정상 종료 할 경우 발생하는
     *     미완선 logfile을 구분하기 위해서 추가함 */
    IDP_DEF(UInt, "__USE_TEMP_FOR_PREPARE_LOGFILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-15396 : log buffer type
    // mmap(= 0) 또는 memory(= 1)
    // => LFG별로 지정하여야 한다.
    IDP_DEF(UInt, "LOG_BUFFER_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // 메모리 DB로그중 INSERT/UPDATE/DELETE LOG에 대해서
    // 로그를 기록하기 전에 compress하기 시작하는 로그 크기의 임계치
    //
    // 이 값이 0이면 로그 압축기능을 사용하지 않는다.
    //
    // 로그 레코드의 크기가 이 이상이면 압축하여 기록한다.
    IDP_DEF(UInt, "MIN_LOG_RECORD_SIZE_FOR_COMPRESS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 512 );

    // TASK-2398 Log Compress
    IDP_DEF(ULong, "DISK_REDO_LOG_DECOMPRESS_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 128 * 1024 * 1024 );


    // 로그화일의 sync 주기(초)
    // 내부 속성
    IDP_DEF(UInt, "SYNC_INTERVAL_SEC_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 3);

    // 로그화일의 sync 주기( milliseconds )
    // 내부속성
    IDP_DEF(UInt, "SYNC_INTERVAL_MSEC_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 999999, 200);

    /* BUG-35392 */
    // Uncompleted LSN을 갱신하는 Thread에 Interval을 세팅.
    IDP_DEF(UInt, "UNCOMPLETED_LSN_CHECK_THREAD_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, UINT_MAX, 1000 );

    /* BUG-35392 */
    // 로그화일의 sync 최소 주기( milliseconds )
    // 내부속성
    IDP_DEF(UInt, "LFTHREAD_SYNC_WAIT_MIN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 999999, 1);

    /* BUG-35392 */
    // 로그화일의 sync 최대 주기( milliseconds )
    // 내부속성
    IDP_DEF(UInt, "LFTHREAD_SYNC_WAIT_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (1000));

    /* BUG-35392 */
    // 로그화일의 sync 최소 주기( milliseconds )
    // 내부속성
    IDP_DEF(UInt, "LFG_MANAGER_SYNC_WAIT_MIN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 999999, 1);

    /* BUG-35392 */
    // 로그화일의 sync 최대 주기( milliseconds )
    // 내부속성
    IDP_DEF(UInt, "LFG_MANAGER_SYNC_WAIT_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 999999, 1000);

    // 로그화일 생성시 화일의 생성 방법 
    // 0 : write
    // 1 : fallocate 
    IDP_DEF(UInt, "LOG_CREATE_METHOD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // 로그화일 생성시 화일의 초기화  방법
    // 내부속성
    // 0 : 맨 끝 한바이트만 초기화  
    // 1 : 0 으로 로그 전체 초기화
    // 2 : random값으로 로그 전체 초기화
    IDP_DEF(UInt, "SYNC_CREATE_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1);

    // BUG-23637 최소 디스크 ViewSCN을 트랜잭션레벨에서 Statement 레벨로 구해야함.
    // SysMinViewSCN을 갱신하는 주기(mili-sec.)
    /* BUG-32944 [sm_transaction] REBUILD_MIN_VIEWSCN_INTERVAL_ - invalid
     * flag */
    IDP_DEF(UInt, "REBUILD_MIN_VIEWSCN_INTERVAL_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_MSEC, 100 );

    // 하나의 OID LIST 구조체에서 저장할 수 있는 OID의 개수
    // 내부 속성
    IDP_DEF(UInt, "OID_COUNT_IN_LIST_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 8);

    // 하나의 TOUCH LIST 구조체에서 저장할 수 있는
    // 갱신된 페이지의 개수
    // 내부 속성
    IDP_DEF(UInt, "TRANSACTION_TOUCH_PAGE_COUNT_BY_NODE_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 14 ); /* TASK-6950 : WRITABLE로 바꾸면 안됨. */

    // 버퍼크기 대비 트랜잭션당 캐싱할 페이지 비율
    // 내부 속성
    IDP_DEF(UInt, "TRANSACTION_TOUCH_PAGE_CACHE_RATIO_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100, 10 );

    // To Fix BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부하 및
    //                         Aging이 밀리는 현상이 더 심화됨.
    //
    // => Logical Thread 여러개를 병렬로 수행하여 문제해결
    //
    // Logical Ager Thread의 최소갯수와 최대갯수를 프로퍼티로 지정
    IDP_DEF(UInt, "MAX_LOGICAL_AGER_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 99, 99);

    IDP_DEF(UInt, "MIN_LOGICAL_AGER_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 99, 1);

#if defined(WRS_VXWORKS)
    IDP_DEF(UInt, "LOGICAL_AGER_COUNT_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 99, 1);
#else
    IDP_DEF(UInt, "LOGICAL_AGER_COUNT_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 99, 3);
#endif

#if defined(WRS_VXWORKS)
    IDP_DEF(UInt, "DELETE_AGER_COUNT_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 99, 1);
#else
    IDP_DEF(UInt, "DELETE_AGER_COUNT_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 99, 3);
#endif

    /* Ager List의 갯수를 지정하는 Property 
     * default: 7
     * min    : 1
     * max    : 99
     * 해당 Property는 소수값으로 해야 List 분배가 적절히 이루어진다. */
    IDP_DEF(UInt, "__AGER_LIST_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 99, 7);

    /* Delete Thread의 Parallel 동작을 ON/OFF하는 프로퍼티
     * Delete Thread의 갯수는 DELETE_AGER_COUNT_ 프로퍼티로 조절함.
     * 0 : PARALLEL 동작하지 않음.
     *     __AGER_LIST_COUNT 가 1일 경우 0으로 설정시 Parallel 동작 안함.
     *     __AGER_LIST_COUNT 가 1보다 클 경우 List 단위 Parallel로 동작 가능.
     * 1 : PARALLEL 동작함. 
     */
    IDP_DEF( UInt, "__PARALLEL_DELETE_THREAD",
             IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1);

    // 0: serial
    // 1: parallel
    // 내부속성
    IDP_DEF(UInt, "RESTORE_METHOD_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "RESTORE_THREAD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            //fix BUG-19787
            1, 512, IDL_MIN(idlVA::getProcessorCount() * 2, 512));

    IDP_DEF(UInt, "RESTORE_AIO_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    /* 데이터베이스 파일 로드시,
       한번에 파일에서 메모리로 읽어들일 페이지 수 */
    IDP_DEF(UInt, "RESTORE_BUFFER_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 1024);

    IDP_DEF(UInt, "CHECKPOINT_AIO_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // To Fix BUG-9366
    IDP_DEF(UInt, "CHECKPOINT_BULK_WRITE_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // To Fix BUG-9366
    IDP_DEF(UInt, "CHECKPOINT_BULK_WRITE_SLEEP_SEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 0);

    // To Fix BUG-9366
    IDP_DEF(UInt, "CHECKPOINT_BULK_WRITE_SLEEP_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_USEC, 0);

    /* checkpoint시 flush dirty pages 과정에서
       데이타베이스 파일 sync하기 위해 write해야하는
       페이지 개수 정의 기본값 100MB(3200 pages)
       값이 0일 경우에는 page write 수를 세지 않고
       모든 page를 모두 write하고 마지막에 한번만 sync한다.*/
    IDP_DEF(UInt, "CHECKPOINT_BULK_SYNC_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 3200 );


    IDP_DEF(UInt, "CHECK_STARTUP_VERSION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "CHECK_STARTUP_BITMODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "CHECK_STARTUP_ENDIAN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "CHECK_STARTUP_LOGSIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // 서버 구동시 이중화를 위해서 대상 호스트와 연결 시
    // 이 시간 이상 응답이 없을 경우 연결을 더 이상 시도하지 않고
    // 진행한다. (초)
    // 내부속성
    IDP_DEF(UInt, "REPLICATION_LOCK_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |  // BUG-18142
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3600, 5 /*sec*/);

    /* BUG-27709 receiver에 의한 반영중, 트랜잭션 alloc이
       이 시간 이상을 대기하면, 실패시키고 해당 receiver를 종료한다. */
    IDP_DEF(UInt, "__REPLICATION_TX_VICTIM_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3600, 10 /*sec*/);

    IDP_DEF(UInt, "LOGFILE_PRECREATE_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 10);

    IDP_DEF(UInt, "FILE_INIT_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10 * 1024 * 1024);
#if 0
    IDP_DEF(UInt, "LOG_BUFFER_LIST_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1);

    IDP_DEF(UInt, "LOG_BUFFER_ITEM_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 5);
#endif
    // 0 : buffered IO
    // 1 : direct IO
    // 디버그 속성
    IDP_DEF(UInt, "DATABASE_IO_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // Log 기록시 IO타입
    // 0 : buffered IO
    // 1 : direct IO
    IDP_DEF(UInt, "LOG_IO_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-15961: DirectIO를 쓰지 않는 System Property가 필요함*/
    // 0 : disable
    // 1 : enable
    IDP_DEF(UInt, "DIRECT_IO_ENABLED",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-18646: Direct Path Insert시 연속된 Page에 대한 Insert 연산시 IO를
       Page단위가 아닌 여러개의 페이지를 묶어서 한번의 IO로 수행하여야 합니다. */

    // SD_PAGE_SIZE가 8K일때 기준으로
    // Default:   1M
    // Min    :   8K
    // Max    : 100M
    IDP_DEF(UInt, "BULKIO_PAGE_COUNT_FOR_DIRECT_PATH_INSERT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            128, 12800, 128);

    /* Direct Path Buffer Page갯수를 지정 */
    // Default: 1024
    // Min    : 1024
    // Max    : 2^32 - 1
    IDP_DEF(UInt, "DIRECT_PATH_BUFFER_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, ID_UINT_MAX, 1024);

// AIX의 경우 Direct IO Page를 512 byte로
// - iduFile.h 에 그렇게 정의되어 있었음
#if defined(IBM_AIX)
    // Direct I/O시 file offset및 data size를 Align할 Page크기
    IDP_DEF(UInt, "DIRECT_IO_PAGE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            512, IDP_MAX_DIO_PAGE_SIZE, 512);
#else // 그 외의 경우는 기본값을 4096으로
    IDP_DEF(UInt, "DIRECT_IO_PAGE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            512, IDP_MAX_DIO_PAGE_SIZE, 4096);
#endif

    IDP_DEF(UInt, "DELETE_AGER_COMMIT_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 100000 );

    // 0 : CPU count
    // other : 지정된 개수
    // 디버그 속성
    IDP_DEF(UInt, "INDEX_BUILD_THREAD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            //fix BUG-19787
            1, 512, IDL_MIN(idlVA::getPhysicalCoreCount(), 512));

    // selective index building method (minimum record count)
    // min : 0, max : ID_UINT_MAX, default : 0
    IDP_DEF(UInt, "__DISK_INDEX_BOTTOM_UP_BUILD_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // min : 2, max : ID_UINT_MAX, default : 128
    IDP_DEF(UInt, "DISK_INDEX_BUILD_MERGE_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2, ID_UINT_MAX, 128);

    IDP_DEF(UInt, "INDEX_BUILD_MIN_RECORD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10000);

    /* PROJ-1628
       2개의 Node가 Key redistribution을 수행한 이후 확보되어야 할
       Free 영역 비율(%). Key redistribution 후에 각 노드에서
       이 프로퍼티에 지정된 크기의 Free 영역이 확보되지 않으면
       split으로 진행한다.(Hidden)
     */
    IDP_DEF(UInt, "__DISK_INDEX_KEY_REDISTRIBUTION_LOW_LIMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 40, 1);

    /* BUG-43046 
       디스크 인덱스가 깨져서 sdnbBTree::traverse에서 무한 루프를 돌 경우
       최대 traverse하는 length (length는 node 하나 내려갈 때 1씩 증가한다.)
       -1이면 traverse length를 check하지 않는다.
     */
    IDP_DEF(SLong, "__DISK_INDEX_MAX_TRAVERSE_LENGTH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            -1, ID_SLONG_MAX, 1000000);

    /* PROJ-1628
       Unbalanced split을 수행할 때, 새로 생성되는 Node가 가져야 하는
       Free 영역의 비율(%). 예를 들어 90으로 지정하면, A : B의 Key 비율이
       90:10이 된다는 것을 의미한다.
     */
    IDP_DEF(UInt, "DISK_INDEX_UNBALANCED_SPLIT_RATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            50, 99, 90);

    /* PROJ-1591
       Disk RTree의 Split을 수행하는 방식을 결정한다.
       0: 기본적인 R-Tree 알고리즘의 Split 방식으로 Split을 수행한다.
       1: R*-Tree 방식으로 Split을 수행한다.
     */
    IDP_DEF(UInt, "__DISK_INDEX_RTREE_SPLIT_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* PROJ-1591:
       R*-Tree 방식의 Split 모드일 경우에만 사용되는 값이다.
       Split의 분할 축을 결정하기 위해서 평가될 최대 키 갯수의 비율을
       결정한다.
     */
    IDP_DEF(UInt, "__DISK_INDEX_RTREE_SPLIT_RATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            20, 45, 40);

    /* PROJ-1591:
     */
    IDP_DEF(UInt, "__DISK_INDEX_RTREE_MAX_KEY_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            3, 500, 500);

    // refine parallel factor
    // 디버그 속성
    IDP_DEF(UInt, "REFINE_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 50);

    // 0 : disable
    // 1 : enable
    IDP_DEF(UInt, "TABLE_COMPACT_AT_SHUTDOWN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-33008 [sm-disk-collection] [DRDB] The migration row logic
     * generates invalid undo record.
     * 버그에 대한 사후 처리 수행 여부 */
    IDP_DEF(UInt, "__AFTER_CARE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

    // 0 : disable
    // 1 : enable
    // 디버그 속성
    IDP_DEF(UInt, "CHECKPOINT_ENABLED",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // sender가 디스크 상에 저장된 로그만을 보내도록 할 때 활성화 시킨다.
    IDP_DEF(UInt, "REPLICATION_SYNC_LOG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "TABLE_BACKUP_FILE_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024 * 1024, 1024);

    IDP_DEF(UInt, "DEFAULT_LPCH_ALLOC_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1);

    IDP_DEF(UInt, "AGER_WAIT_MINIMUM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (200000));

    IDP_DEF(UInt, "AGER_WAIT_MAXIMUM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (1000000));

    /* PROJ-2268 Reuse Catalog Table Slot
     * 0 : Catalog Table Slot을 재사용하지 않는다.
     * 1 : Catalog Table Slot을 재사용한다. (default) */
    IDP_DEF( UInt, "__CATALOG_SLOT_REUSABLE",
             IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );

    /* TASK-6006: Index 단위 Parallel Index Rebuilding
     * 이 값이 1일 경우에는 Index 단위 Parallel Index Rebuilding을 수행하고
     * 이 값이 0일 경우 Table 단위 Parallel Index Rebuilding을 수행한다. */
    IDP_DEF(SInt, "REBUILD_INDEX_PARALLEL_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-40509 Change Memory Index Node Split Rate
     * Memory Index에서 Unbalanced split을 수행할 때, 
     * 새로 생성되는 Node가 가져야 하는 Free 영역의 비율(%). 
     * 예를 들어 90으로 지정하면, A : B의 Key 비율이 90:10이 된다는 것을 의미한다. */
    IDP_DEF(UInt, "MEMORY_INDEX_UNBALANCED_SPLIT_RATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            50, 99, 90); 

// A3 -> A4로 바뀐 것

    // 단위 변경 : LOCK_ESCALATION_MEMORY_SIZE (M에서 그냥 byte로 )
    //             그냥 크기로 사용.  1024 * 1024 만큼 곱하기 제거.
    // BUG-18863 : LOCK_ESCALATION_MEMORY_SIZE이 Readonly로 되어있습니다.
    //             또한 Default값을 100M으로 바꾸어야 합니다.
    IDP_DEF(UInt, "LOCK_ESCALATION_MEMORY_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1000 * 1024 * 1024, 100 * 1024 * 1024 );

    /* BUG-19080: Old Version의 양이 일정이상 만들면 Transaction을 Abort하는 기능이
     *            필요합니다.
     *
     *       # 기본값: 10M
     *       # 최소값: 0 < 0이면 이 Property는 동작하지 않습니다. >
     *       # 최대값: ID_ULONG_MAX
     *       # 속  성: Session Property입니다.
     **/
    IDP_DEF( ULong, "TRX_UPDATE_MAX_LOGSIZE",
             IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_ULONG_MAX, (10 * 1024 * 1024) );

    /* PROJ-2162 RestartRiskReduction
     * Startup시 MMDB Index Build시 동시에 몇개의 Index를
     * Build할 것인가.*/
    /* BUG-32996 [sm-mem-index] The default value of
     * INDEX_REBUILD_PARALLEL_FACTOR_AT_STARTUP need to adjust */
    /* BUG-37977 기본 값을 코어 수에 따라 다르게 수정
     * 코어수가 4보다 클 경우에도 4로 제한한다.
     * 이 수정은 차후 smuWorkerThreadMgr가 개선되면 제거할 예정이다. */
    IDP_DEF(UInt, "INDEX_REBUILD_PARALLEL_FACTOR_AT_STARTUP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            //fix BUG-19787, BUG-37977, BUG-40870
            1, 512, IDP_LIMITED_CORE_COUNT( 1, 4 ) );

    /* - BUG-14053
       System 메모리 부족시 Server Start시 Meta Table의 Index만을
       Create하고 Start하는 프로퍼티 추가.
       INDEX_REBUILD_AT_STARTUP = 1: Start시 모든 Index Rebuild
       INDEX_REBUILD_AT_STARTUP = 0: Start시 Meta Table의 Index만
       Rebuild
    */
    IDP_DEF(UInt, "INDEX_REBUILD_AT_STARTUP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-47540: 통계 정보 자동 수집 기능에서 봐야 하는 앞뒤 값이 다른
     * 노드에 걸쳐있을 때 동시성을 고려하지 않고 수행되기 때문에 종종
     * 이미 변경된 또는 변경중인 row를 건드릴 수 있다. 이 프로퍼티를 켜면
     * 통계 정보 자동 수집 중 이와 같은 경우 해당 데이타에 대한 접근을
     * Skip 한다.
     */
    IDP_DEF(UInt, "__SKIP_IDX_STAT_NODE_BOUND",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

// smi
    /* TASK-4990 changing the method of collecting index statistics
     * 0 (Manual)       - 통계정보 재구축시에만 갱신됨
     * 1 (Accumulation) - Row가 변경될때마다 누적시킴(5.5.1 이하 버전)*/
    /* BUG-33203 [sm_transaction] change the default value of 
     * __DBMS_STAT_METHOD, MUTEX_TYPE, and LATCH_TYPE properties */
    IDP_DEF(UInt, "__DBMS_STAT_METHOD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* 
     * BUG-35163 - [sm_index] [SM] add some exception properties for __DBMS_STAT_METHOD
     * 아래 3 프로퍼티는 __DBMS_STAT_METHOD 설정에 예외를 두기 위함이다.
     *
     * 만약 아래 프로퍼티가 설정되어 있으면 __DBMS_STAT_METHOD 설정을 무시하고
     * 해당 프로퍼티의 설정을 따르게 된다.
     * 예를 들어 __DBMS_STAT_METHOD=0 일때,
     * __DBMS_STAT_METHOD_FOR_VRDB=1 로 설정하면 Volatile TBS에 대해서는
     * 통계 갱신을 MANUAL이 아닌 AUTO로 처리하게 된다.
     */
    IDP_DEF(UInt, "__DBMS_STAT_METHOD_FOR_MRDB",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "__DBMS_STAT_METHOD_FOR_VRDB",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "__DBMS_STAT_METHOD_FOR_DRDB",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* TASK-4990 changing the method of collecting index statistics
     * 통계정보 획득시 Sampling 자동 계산용 기준 페이지 개수
     * 기본은 131072개. 따라서 Page 131072 이하의 Page는 100% 다 가져오고,
     * 그 이상은 적게 가져와서 Sampling 한다.
     * 기본값 131072, PAGE SIZE :8K 기준으로 Sampling크기는 다음과 같다.
     * 1GB    = 100% 
     * 2GB    = 70.71%  
     * 10GB   = 31.62%
     * 100GB  = 10%
     *
     * BUG-42832 프로퍼티의 기본값을 4096으로 변경합니다.
     */
    IDP_DEF(UInt, "__DBMS_STAT_SAMPLING_BASE_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 4096 );


    /* TASK-4990 changing the method of collecting index statistics
     * 통계정보 획득시 Parallel 기본값 */
    IDP_DEF(UInt, "__DBMS_STAT_PARALLEL_DEGREE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 1 );

    /* TASK-4990 changing the method of collecting index statistics
     * Column통계정보 등 새로 보강된 통계정보를 수집할지 여부
     * 0 (No) - 1 (Yes) */
    IDP_DEF(UInt, "__DBMS_STAT_GATHER_INTERNALSTAT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* TASK-4990 changing the method of collecting index statistics
     * Row 개수가 이전 통계정보 수집시에 N%를 넘어가면, 통계정보를 재수집한다.
     * 이 Property는 N%를 의미한다. */
    IDP_DEF(UInt, "__DBMS_STAT_AUTO_PERCENTAGE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100,  10 );

    /* TASK-4990 changing the method of collecting index statistics
     * 다음 시간(SEC)만큼 흐른 후 통계정보 자동 수집 여부를 판단한다.
     * 0이면 동작하지 않는다. */
    IDP_DEF(UInt, "__DBMS_STAT_AUTO_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 0);

    // BUG-20789
    // SM 소스에서 contention이 발생할 수 있는 부분에
    // 다중화를 할 경우 CPU하나당 몇 클라이언트를 예상할 지를 나타내는 상수
    // SM_SCALABILITY는 CPU개수 곱하기 이값이다.
    IDP_DEF(UInt, "SCALABILITY_PER_CPU",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 128, 2);

    /* BUG-22235: CPU갯수가 다량일때 Memory를 많이 사용하는 문제가 있습니다.
     *
     * 다중화 갯수가 이 갯수를 넘어가면 이 값으로 보정한다.
     * */
#if defined(ALTIBASE_PRODUCT_HDB)
    IDP_DEF(UInt, "MAX_SCALABILITY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 16);
#else
    IDP_DEF(UInt, "MAX_SCALABILITY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 32);
#endif

// for sdnhTempHash
/* --------------------------------------------------------------------
 * temp hash index에서 만들어 질수있는 최대 hash table  개수이다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "MAX_TEMP_HASHTABLE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 128, 128);
#if 0
// sda
    // BUFFER POOL의 페이지 개수에 비례한 TSS의 개수의 %값
    // TSS개수를 BUFFER의 몇%로 제한함으로써 undo 페이지의 무한 확장을 막는다.
    // 0 일 경우에는 TSS의 개수가 무한으로 사용가능하다.
    IDP_DEF(UInt, "TSS_CNT_PCT_TO_BUFFER_POOL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100,(0));
#endif
//smx
    // transaction table이 full상황에서 transaction을 alloc못받을 경우에
    // 대기 하는  시간 micro sec.
    IDP_DEF(UInt, "TRANS_ALLOC_WAIT_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, (60000000) /* 60초 */ , (80));

    // PROJ-1566 : Private Buffer Page
    IDP_DEF(UInt, "PRIVATE_BUFFER_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // BUG-47525 GROUP COMMIT COUNT
    // Group Commit이 동작할때 최대 몇개까지 Tx을 Group화 하는지 결정하는 Property
    // 최대 값이기 때문에 실제로 1개만 Group 화 되는 경우도 발생한다.
    // 0으로 설정되어 있으면 Group Commit을 Off 한다.
    // Min : 0, Max : 50, Default : 10
    IDP_DEF(UInt, "__GROUP_COMMIT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 50, 10);

    // BUG-47525 GROUP COMMIT LIST COUNT
    // Group Commit을 위해 Tx을 모으는 List의 갯수
    // Write 연산이 느리기 때문에 Write에 들어간 동안
    // Tx을 Group화 할 List는 여러개 여야 한다. 
    // Min : 2, Max : 50, Default : 8
    IDP_DEF(UInt, "__GROUP_COMMIT_LIST_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2, 50, 8);

    // PROJ-1704 Disk MVCC 리뉴얼
    // 트랜잭션 세그먼트 엔트리 개수 지정
    // 트랜잭션 세그먼트 최대 개수는 첫번째
    // 데이타파일의 File Space Header에 저장될 수 있는
    // SegHdr PID의 개수이다. SegHdr PID 가 4 바이트이고
    // TSSEG 512개 UDSEG 512개까지 생성하고 저장할수 있음.
    IDP_DEF(UInt, "TRANSACTION_SEGMENT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512, (256));

    // 트랜잭션 세그먼트 테이블이 모두 ONLINE인 상황에서
    // 트랜잭션 세그먼트 엔트리를 할당 받지 못할 경우에
    // 대기 하는 시간 micro sec.
    IDP_DEF(UInt, "TRANSACTION_SEGMENT_ALLOC_WAIT_TIME_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX , (80));

    // BUG-47365 로그 압축 리소스의 재 사용 여부
    // 0 : 재사용하지 않고 트랜잭션 종료시 반환
    // 1 : 트랜잭션 종료시 반환하지 않고 재사용
    IDP_DEF(UInt, "LOG_COMP_RESOURCE_REUSE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-47365 로그 압축 리소스 재사용시
    // 반납하지 않고 가지고 있을 Size
    IDP_DEF(UInt, "COMP_RES_TUNE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            512, ID_UINT_MAX, (32 * 1024));

#if 0
    IDP_DEF(UInt, "TRANS_KEEP_TABLE_INFO_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 5);
#endif

#if defined(ALTIBASE_PRODUCT_XDB)
    // BUG-40960 : [sm-transation] [HDB DA] make transaction suspend sleep time tunable
    // transaction이 suspend될 때, 한 번에 sleep할 시간
    IDP_DEF(UInt, "TRANS_SUSPEND_SLEEP_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, (1000000) /* 1초 */ , (20));
#endif

// sdd
    // altibase 서버가 최대 열수 있는 datafile의 개수
    // 이 개수는 항상 system의 최대 datafile의 개수보다
    // 작아야 한다. 그렇지 않으면 system의 모든 open 화일을
    // altibase가 전부 점령하여 다른 process가 화일을 open
    // 할 수 없는 상황이 발생할 지 모른다.
    IDP_DEF(UInt, "MAX_OPEN_DATAFILE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 200);

    // 최대 열 수 있는 화일이 이미 열려 있어
    // 화일을 열 수 없는 경우 화일을 열기 위해 대기하는 시간
    // 이 시간이 지나면 더이상 기다리지 않고 에러를 리턴한다.
    // 초로 지정
    IDP_DEF(UInt, "OPEN_DATAFILE_WAIT_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 10);

    // 데이타 파일 backup이 종료되기까지 대기 시간(초) 또는
    // 데이타 파일 backup이 종료되고, page image log를
    // 데이타 파일에 적용하는 것을 대기하는 시간(초)

    IDP_DEF(UInt, "BACKUP_DATAFILE_END_WAIT_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 8);

    // 데이터 파일을 생성할 때 데이터 초기화하는 단위
    // 이 값은 페이지 개수이다.
    // min은 1개 즉 8192바이트이고
    // max는 1024 페이지 즉 8M 이며
    // 기본값은 1024 페이지 즉 8M이다.
    IDP_DEF(ULong, "DATAFILE_WRITE_UNIT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 1024);

    /* To Fix TASK-2688 DRDB의 Datafile에 여러개의 FD( File Descriptor )
     * 열어서 관리해야 함
     *
     * 하나의 DRDB의 Data File당 Open할 수 있는 FD의 최대 갯수
     * */
    IDP_DEF(UInt, "DRDB_FD_MAX_COUNT_PER_DATAFILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 8);

#if defined(ALTIBASE_PRODUCT_HDB)
// For sdb
/* --------------------------------------------------------------------
 * buffer pool의 크기. 페이지 개수이다.
 * 실제 크기는 페이지 개수 * 한 페이지 크기가 된다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "DOUBLE_WRITE_DIRECTORY_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 16, 2);

    IDP_DEF(String, "DOUBLE_WRITE_DIRECTORY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_DF_DROP_DEFAULT |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");
#endif

// For sdb

#if defined(ALTIBASE_PRODUCT_HDB)
#if 0
/* --------------------------------------------------------------------
 * DEBUG 모드에서 validate 함수 수행 여부
 * 이 함수가 수행될때 성능이 저하될 수 있다.
 * 0일때 수행, 1일때 수행안함
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "VALIDATE_BUFFER",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );
#endif
    /* PROJ-2669 begin */
/* --------------------------------------------------------------------
 * HOT_FLUSH_LIST_PCT
 *  0    : DO NOT DELAYED FLUSH
 *         IGNORE DELAYED_FLUSH_PROTECTION_TIME_MSEC
 *  1~100: DO DELAYED FLUSH, Use Delayed Flush list 
 * ----------------------------------------------------------------- */
    IDP_DEF( UInt, "DELAYED_FLUSH_LIST_PCT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, 100, 30 );

    IDP_DEF( UInt, "DELAYED_FLUSH_PROTECTION_TIME_MSEC",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, 100000, 100 );
    /* PROJ-2669 end */

    // PROJ-1568 begin
    IDP_DEF( UInt, "HOT_TOUCH_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE,
             1, ID_UINT_MAX, 2 );

    IDP_DEF(UInt, "BUFFER_VICTIM_SEARCH_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_MSEC, 3000); // millisecond 단위임

    IDP_DEF(UInt, "BUFFER_VICTIM_SEARCH_PCT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100, 40);

    IDP_DEF( UInt, "HOT_LIST_PCT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, 100, 0 );

    IDP_DEF( UInt, "BUFFER_HASH_BUCKET_DENSITY",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 100, 1 );

    IDP_DEF( UInt, "BUFFER_HASH_CHAIN_LATCH_DENSITY",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 100, 1 );

    /* BUG-32750 [sm-disk-resource] The hash bucket numbers of BCB(Buffer
     * Control Block) are concentrated in low number. 
     * 기존 ( FID + SPACEID + FPID ) % HASHSIZE 는 각 DataFile의 크기가 작을
     * 경우 앞쪽으로 HashValue가 몰리는 단점이 있다. 이에 따라 다음과 같이
     * 수정한다
     * ( ( SPACEID * PERMUTE1 +  FID ) * PERMUTE2 + PID ) % HASHSIZE
     * PERMUTE1은 Tablespace가 다를때 값이 어느정도 간격이 되는가,
     * PERMUTE2는 Datafile FID가 다를때 값이 어느정도 간격이 되는가,
     * 입니다.
     * PERMUTE1은 Tablespace당 Datafile 평균 개수보다 조금 작은 값이 적당하며
     * PERMUTE2은 Datafile당 Page 평균 개수보다 조금 작은 값이 적당합니다. */
    IDP_DEF( UInt, "__BUFFER_HASH_PERMUTE1",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, ID_UINT_MAX, 8 );
    IDP_DEF( UInt, "__BUFFER_HASH_PERMUTE2",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, ID_UINT_MAX, 65536 );

    /* BUG-21964: BUFFER_FLUSHER_CNT속성이 Internal입니다. External로
     *            바꿔야 합니다.
     *
     *  BUFFER_FLUSH_LIST_CNT
     *  BUFFER_PREPARE_LIST_CNT
     *  BUFFER_CHECKPOINT_LIST_CNT
     *  BUFFER_LRU_LIST_CNT
     *  BUFFER_FLUSHER_CNT 를 External로 변경.
     *  */
    IDP_DEF( UInt, "BUFFER_LRU_LIST_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 64, 7 );

    /* BUG-22070: Buffer Manager에서 분산처리 작업이
     * 제대로 이루어지지 않습니다.
     * XXX
     * 현재 리소스 문제로 일단 리스트 갯수를 1로 설정
     * 하여 문제를 해결합니다. */

    IDP_DEF( UInt, "BUFFER_FLUSH_LIST_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 64, 1 );

    IDP_DEF( UInt, "BUFFER_PREPARE_LIST_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 64, 7 );

    IDP_DEF( UInt, "BUFFER_CHECKPOINT_LIST_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 64, 4 );

    IDP_DEF( UInt, "BUFFER_FLUSHER_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 16, 2 );
#endif

#if defined(ALTIBASE_PRODUCT_HDB)
    IDP_DEF( UInt, "BUFFER_IO_BUFFER_SIZE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 256, 64 );

    /* BUG-36092 [sm] If ALTIBASE_BUFFER_AREA_SIZE= "8K", server can't clean
     * BUFFER_AREA_SIZE 가 너무 작으면 victim을 선정하지 못해 server start를 하지 못합니다. */
    IDP_DEF( ULong, "BUFFER_AREA_SIZE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE,
             8*1024*10, ID_ULONG_MAX, 128*1024*1024 );

    IDP_DEF( ULong, "BUFFER_AREA_CHUNK_SIZE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             8*1024, ID_ULONG_MAX, 32*1024*1024 );

    // BUG-48042: BUFFER AREA Parallel 생성 프로퍼티
    // default 값은 프로세서 수 / 4   
    IDP_DEF( UInt, "__BUFFAREA_CREATE_PARALLEL_DEGREE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             1, 512, 
             IDL_MAX((idlVA::getProcessorCount()/4), 1));   

#if 0
    IDP_DEF( UInt, "BUFFER_PINNING_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             0, 256, 5 );

    IDP_DEF( UInt, "BUFFER_PINNING_HISTORY_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE,
             0, 256, 5 );
#endif
    IDP_DEF( UInt, "DEFAULT_FLUSHER_WAIT_SEC",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             1, IDV_MAX_TIME_INTERVAL_SEC, 1 );

    IDP_DEF( UInt, "MAX_FLUSHER_WAIT_SEC",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             1, ID_UINT_MAX, 10 );
#endif

    IDP_DEF( ULong, "CHECKPOINT_FLUSH_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             1, ID_ULONG_MAX, 64 );

    // BUG-22857 restart recovery시 읽을 LogFile수
    // 반대로 말하면 남겨둘 LogFile수
    IDP_DEF( UInt, "FAST_START_LOGFILE_TARGET",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, ID_UINT_MAX, 100 );

    // BUG-22857 restart recovery시 읽을 redo page수
    // 반대로 말하면 남겨둘 dirty page수
    IDP_DEF( ULong, "FAST_START_IO_TARGET",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, ID_ULONG_MAX, 10000 );

    IDP_DEF( UInt, "LOW_PREPARE_PCT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, 100, 1 );

    IDP_DEF( UInt, "HIGH_FLUSH_PCT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, 100, 5 );

    IDP_DEF( UInt, "LOW_FLUSH_PCT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, 100, 1 );

    IDP_DEF( UInt, "TOUCH_TIME_INTERVAL",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, 100, 3 );

    IDP_DEF( UInt, "CHECKPOINT_FLUSH_MAX_WAIT_SEC",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, ID_UINT_MAX, 10 );

    IDP_DEF( UInt, "CHECKPOINT_FLUSH_MAX_GAP",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK ,
             0, ID_UINT_MAX, 10 );

    IDP_DEF( UInt, "BLOCK_ALL_TX_TIME_OUT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE,
             0, UINT_MAX, 3);

    IDP_DEF( UInt, "DB_FILE_MULTIPAGE_READ_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE,
             1, 128, 8);

    IDP_DEF( UInt, "SMALL_TABLE_THRESHOLD",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE,
             1, UINT_MAX, 128);

    //proj-1568 end

    // PROJ-2068 Direct-Path INSERT 성능 개선
    IDP_DEF( SLong, "__DPATH_BUFF_PAGE_ALLOC_RETRY_USEC",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE,
             0, IDV_MAX_TIME_INTERVAL_USEC, 100000);

    IDP_DEF( UInt, "__DPATH_INSERT_ENABLE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_CK_CHECK |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE,
             0, 1, 0);

// sdp
#if 0 
    // TTS 할당시 다른 트랜잭션이 완료되기를 기다리는 시간 (MSec.)
    IDP_DEF(ULong, "TRANS_WAIT_TIME_FOR_TTS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10000000, ID_ULONG_MAX, 10000000);
#endif

    /* BUG-47223 : 트랜잭션이 완료되기를 기다리는 시간 (uSec.) */
    IDP_DEF( ULong, "TRANS_WAIT_TIME",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_ULONG_MAX, ID_ULONG_MAX );

/* --------------------------------------------------------------------
 * page의 체크섬의 종류
 * 0 : 페이지 양쪽의 LSN을 page가 valid 한지 확인하기 위해 사용한다.
 * 1 : 페이지 가장 앞의 체크섬 값을 valid 확인을 위해 사용한다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "CHECKSUM_METHOD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE | 
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

/* --------------------------------------------------------------------
 * FLM( Free List Management )의 TableSpace의 Test를 위해서 추가됨.
 * 하나의 ExteDirPage내에 들어가는 ExtDesc의 갯수를 지정.
 * ----------------------------------------------------------------- */

    IDP_DEF(UInt, "__FMS_EXTDESC_CNT_IN_EXTDIRPAGE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

/* --------------------------------------------------------------------
 * PCTFREE reserves space in the data block for updates to existing rows.
 * It represents a percentage of the block size. Before reaching PCTFREE,
 * the free space is used both for insertion of new rows
 * and by the growth of the data block header.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "PCTFREE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 99, 10 );

/* --------------------------------------------------------------------
 * PCTUSED determines when a block is available for inserting new rows
 * after it has become full(reached PCTFREE).
 * Until the space used is less then PCTUSED,
 * the block is not considered for insertion.
----------------------------------------------------------------- */
    IDP_DEF(UInt, "PCTUSED",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 99, 40 );

    IDP_DEF(UInt, "TABLE_INITRANS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 120, 2 );

    IDP_DEF(UInt, "TABLE_MAXTRANS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 120, 120 );

    IDP_DEF(UInt, "INDEX_INITRANS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 50, 8 );

    IDP_DEF(UInt, "INDEX_MAXTRANS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 50, 50 ); /* BUG-48064 : CTS MAX 갯수를 30 -> 50개로 증가시킴 */

    /*
     * PROJ-1671 Bitmap Tablespace And Segment Space Management
     *
     *   기본 세그먼트 초기 Extent 개수 지정
     */
    IDP_DEF( UInt, "DEFAULT_SEGMENT_STORAGE_INITEXTENTS",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1, ID_UINT_MAX, 1);

    /*
     *   기본 세그먼트 확장 Extent 개수 지정
     */
    IDP_DEF(UInt, "DEFAULT_SEGMENT_STORAGE_NEXTEXTENTS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 1 );

    /*
     *   기본 세그먼트 최소 Extent 개수 지정
     */
    IDP_DEF(UInt, "DEFAULT_SEGMENT_STORAGE_MINEXTENTS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 1);

    /*
     *   기본 세그먼트 최대 Extent 개수 지정
     */
    IDP_DEF(UInt, "DEFAULT_SEGMENT_STORAGE_MAXEXTENTS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, ID_UINT_MAX);

    IDP_DEF(UInt, "__DISK_TMS_IGNORE_HINT_PID",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(SInt, "__DISK_TMS_MANUAL_SLOT_NO_IN_ITBMP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            -1, 1023, -1);

    IDP_DEF(SInt, "__DISK_TMS_MANUAL_SLOT_NO_IN_LFBMP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            -1, 1023, -1);

    /*
     *   TMS의 It-BMP에서 선택할 최대 Lf-BMP 후보수
     */
    IDP_DEF(UInt, "__DISK_TMS_CANDIDATE_LFBMP_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 10);

    /*
     *   TMS의 Lf-BMP에서 선택할 최대 Page 후보수
     */
    IDP_DEF(UInt, "__DISK_TMS_CANDIDATE_PAGE_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 5);


    /*
     *   TMS에서 하나의 Rt-BMP가 수용할 수 있는 최대 Slot 수
     */
    IDP_DEF(UInt, "__DISK_TMS_MAX_SLOT_CNT_PER_RTBMP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65536, 960 );    /* BUG-37695 */

    /*
     *   TMS에서 하나의 It-BMP가 수용할 수 있는 최대 Slot 수
     */
    IDP_DEF(UInt, "__DISK_TMS_MAX_SLOT_CNT_PER_ITBMP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65536, 1000);

    /*
     *   TMS에서 하나의 ExtDir가 수용할 수 있는 최대 Slot 수
     */
    IDP_DEF(UInt, "__DISK_TMS_MAX_SLOT_CNT_PER_EXTDIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65536, 480 );    /* BUG-37695 */

    /* BUG-28935 hint insertable page id array의 크기를
     * 사용자가 지정 할 수 있도록 property를 추가*/
    IDP_DEF(UInt, "__DISK_TMS_HINT_PAGE_ARRAY_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            4, 4096, 1024);

    /* BUG-28935 hint insertable page id array를 alloc하는 시기 결정
     *  0 - segment cache생성시 같이 alloc한다.
     *  1 - segment cache생성시에는 할당하지 않고,
     *      이 후 처음 insertable page를 찾을 때 alloc한다. (default)
     * */
    IDP_DEF(UInt, "__DISK_TMS_DELAYED_ALLOC_HINT_PAGE_ARRAY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
    /*
     * PROJ-1704 Disk MVCC Renewal
     *
     * TSS 세그먼트의 ExtDir 페이지당 ExtDesc 개수 설정
     * 4 * 256K = 1M
     */
    IDP_DEF(UInt, "TSSEG_EXTDESC_COUNT_PER_EXTDIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 128, 4 );

    /*
     * Undo 세그먼트의 ExtDir 페이지당 ExtDesc 개수 설정
     * 8 * 256K = 2M
     */
    IDP_DEF(UInt, "UDSEG_EXTDESC_COUNT_PER_EXTDIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 128, 8 );

    /*
     * TSSEG 세그먼트의 크기가 다음 프로퍼티보다 많은 경우
     * Shrink 연산 수행 기본값 6M
     */
    IDP_DEF(UInt, "TSSEG_SIZE_SHRINK_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (6*1024*1024) );

    /*
     * UDSEG 세그먼트의 크기가 다음 프로퍼티보다 많은 경우
     * Shrink 연산 수행 6M
     */
    IDP_DEF(UInt, "UDSEG_SIZE_SHRINK_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (6*1024*1024) );

    /*
     * UDSEG 세그먼트의 Steal Operation의 Retry Count
     */
    IDP_DEF(UInt, "RETRY_STEAL_COUNT_",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 512, 512 );

/* --------------------------------------------------------------------
 * DW buffer를 사용할 것인가 결정한다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "USE_DW_BUFFER",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-27776 [sm-disk-recovery] the server startup can be fail since the
     * dw file is removed after DW recovery. 
     * Server Startup중에는 ARTTest가 되지 않습니다. 따라서 Startup 도중
     * 여러 테스트가 가능하도록, Hidden Property를 추가합니다. */
    IDP_DEF(UInt, "__SM_STARTUP_TEST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

/* --------------------------------------------------------------------
 * create DB를 위한 property
 * ----------------------------------------------------------------- */

    /*
     * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
     *
     * 기본 세그먼트 공간관리 방식 지정
     * 0 : MANUAL ( FMS )
     * 1 : AUTO   ( TMS ) ( 기본값 )
     */
    IDP_DEF(UInt, "DEFAULT_SEGMENT_MANAGEMENT_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

/* --------------------------------------------------------------------
 * 하나의 extent group 에서 관리할 extent의 갯수를 위한 property
 * ----------------------------------------------------------------- */

    /*
     * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
     *
     * 이 property는 테스트를 위하여만 사용한다.
     */
    IDP_DEF(UInt, "DEFAULT_EXTENT_CNT_FOR_EXTENT_GROUP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX , 0); //default 가 0이면 가용한 최대갯수가 사용됨

/* --------------------------------------------------------------------
 * for system data tablespace
 * system을 위한 tablespace(data, temp, undo)는 최초에
 * 하나의 datafile로 구성되며, max size을 넘어서면 datafile이 추가된다.
 * 아래의 tablespace 관련 property 중 크기와 관련된 것은 최초에 생성되는
 * datafile의 default 속성을 정의한다.
 * ----------------------------------------------------------------- */

    // 두번째 extent의 크기
    // To Fix PR-12035
    // 최소값 조건
    // >= 2 Pages

    IDP_DEF(ULong, "SYS_DATA_TBS_EXTENT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (5 * IDP_SD_PAGE_SIZE), ID_ULONG_MAX, (512 * 1024));

    // 두번째 datafile의 초기 크기
    // To Fix PR-12035
    IDP_DEF(ULong, "SYS_DATA_FILE_INIT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (100 * 1024 * 1024));

    // 두번째 data file의 최대 크기
    // To Fix PR-12035
    // 최소값 조건
    // >= SYS_DATA_FILE_INIT_SIZE
    // To Fix BUG-14662. set default to maximum of SInt
    IDP_DEF(ULong, "SYS_DATA_FILE_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            ID_ULONG(ID_UINT_MAX / 2));

    IDP_DEF(ULong, "SYS_DATA_FILE_NEXT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            ( 8 * IDP_SD_PAGE_SIZE ),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            ( 1 * 1024 * 1024 ));

/* --------------------------------------------------------------------
 * for system undo tablespace
 * ----------------------------------------------------------------- */

    // To Fix PR-12035
    // 최소값 조건
    // >= 2 Pages
    IDP_DEF(ULong, "SYS_UNDO_TBS_EXTENT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (5 * IDP_SD_PAGE_SIZE), ID_ULONG_MAX, (256 * 1024));

    // To Fix PR-12035
    IDP_DEF(ULong, "SYS_UNDO_FILE_INIT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (32 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (100 * 1024 * 1024));

    // To Fix PR-12035
    // 최소값 조건
    // >= SYS_UNDO_FILE_INIT_SIZE
    // To Fix BUG-14662. set default to maximum of SInt
    IDP_DEF(ULong, "SYS_UNDO_FILE_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (32 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            ID_ULONG(ID_UINT_MAX / 2));

    IDP_DEF(ULong, "SYS_UNDO_FILE_NEXT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (1 * 1024 * 1024));

 /* --------------------------------------------------------------------
 * for system temp tablespace
 * ----------------------------------------------------------------- */

    // To Fix PR-12035
    // 최소값 조건
    // >= 2 Pages
    IDP_DEF(ULong, "SYS_TEMP_TBS_EXTENT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (5 * IDP_SD_PAGE_SIZE), ID_ULONG_MAX, (512 * 1024));

    // To Fix PR-12035
    IDP_DEF(ULong, "SYS_TEMP_FILE_INIT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (100 * 1024 * 1024));

    // To Fix PR-12035
    // 최소값 조건
    // >= SYS_TEMP_FILE_INIT_SIZE
    // To Fix BUG-14662. set default to maximum of SInt
    IDP_DEF(ULong, "SYS_TEMP_FILE_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            ID_ULONG(ID_UINT_MAX / 2));

    IDP_DEF(ULong, "SYS_TEMP_FILE_NEXT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (1 * 1024 * 1024));

 /* --------------------------------------------------------------------
 * PROJ-1490 페이지리스트 다중화 및 메모리 반납
 * ----------------------------------------------------------------- */

    // 데이터베이스 확장의 단위인 Expand Chunk가 가지는 Page의 수.
    IDP_DEF(UInt, "EXPAND_CHUNK_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2*IDP_MAX_PAGE_LIST_COUNT, /* min */
            /* (병렬화된 DB FREE LIST에 최소 1개 이상 나눠주어야 하기 때문) */
            ID_UINT_MAX,         /* max */
            IDP_DEFAULT_EXPAND_CHUNK_PAGE_COUNT );          /* 128 Pages */

    // 다음과 같은 Page List들을 각각 몇개의 List로 다중화 할 지 결정한다.
    //
    // 데이터베이스 Free Page List
    // 테이블의 Allocated Page List
    // 테이블의 Free Page List
    //
    IDP_DEF(UInt, "PAGE_LIST_GROUP_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
           1, IDP_MAX_PAGE_LIST_COUNT, 1);


    // Expand Chunk확장시에 Free Page들이 여러번에 걸쳐서
    // 다중화된 Free Page List로 분배된다.
    //
    // 이 때, 한번에 몇개의 Page를 Free Page List로 분배할지를 설정한다.
    //
    // default(0) : 최소로 분배되게 자동 계산
    IDP_DEF(UInt, "PER_LIST_DIST_PAGE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    // Free Page List가 List 분할후 가져야 하는 최소한의 Page수
    // Free Page List 분할시에 최소한 이 만큼의 Page가
    // List에 남아 있을 수 있는 경우에만 Free Page List를 분할한다.
    IDP_DEF(UInt, "MIN_PAGES_ON_DB_FREE_LIST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 16);


/* --------------------------------------------------------------------
 * user가 create하는 data tablespace에 대한 속성
 * ----------------------------------------------------------------- */

    // To Fix PR-12035
    // 최소값 조건
    // >= 2 Pages
    IDP_DEF(ULong, "USER_DATA_TBS_EXTENT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (5 * IDP_SD_PAGE_SIZE), ID_ULONG_MAX, (512 * 1024));

    // To Fix PR-12035
    IDP_DEF(ULong, "USER_DATA_FILE_INIT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (100 * 1024 * 1024));

    // To Fix PR-12035
    // 최소값 조건
    // >= USER_DATA_FILE_INIT_SIZE
    // To Fix BUG-14662. set default to maximum of SInt
    IDP_DEF(ULong, "USER_DATA_FILE_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            ID_ULONG(ID_UINT_MAX / 2));

    IDP_DEF(ULong, "USER_DATA_FILE_NEXT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (1 * 1024 * 1024));

/* --------------------------------------------------------------------
 * user가 create하는 temp tablespace에 대한 속성
 * ----------------------------------------------------------------- */

    // To Fix PR-12035
    // 최소값 조건
    // >= 2 Pages
    IDP_DEF(ULong, "USER_TEMP_TBS_EXTENT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (5 * IDP_SD_PAGE_SIZE), ID_ULONG_MAX, (512 * 1024));

    // To Fix PR-12035
    IDP_DEF(ULong, "USER_TEMP_FILE_INIT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (100 * 1024 * 1024));

    // To Fix PR-12035
    // 최소값 조건
    // >= USER_TEMP_FILE_INIT_SIZE
    // To Fix BUG-14662. set default to maximum of SInt
    IDP_DEF(ULong, "USER_TEMP_FILE_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            ID_ULONG(ID_UINT_MAX / 2));

    IDP_DEF(ULong, "USER_TEMP_FILE_NEXT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (8 * IDP_SD_PAGE_SIZE),
            IDP_DRDB_DATAFILE_MAX_SIZE,
            (1 * 1024 * 1024));

 // sdc
/* --------------------------------------------------------------------
 * PROJ-1595
 * Disk Index 구축시 In-Memory Sorting 영역
 * ----------------------------------------------------------------- */

    IDP_DEF(ULong, "SORT_AREA_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (512*1024), ID_ULONG_MAX, (1 * 1024 * 1024));

    /**************************************************************
     * PROJ-2201 Innovation in sorting and hashing(temp) 
     **************************************************************/
    IDP_DEF(ULong, "HASH_AREA_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (3 * 1024 * 1024), ID_ULONG_MAX, (4 * 1024 * 1024));
    
    /* WA(WorkArea)의 초기화 한 총 크기 */
    IDP_DEF(ULong, "INIT_TOTAL_WA_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, ID_ULONG_MAX);

    /* WA(WorkArea)의 최대 총 크기 */
    IDP_DEF(ULong, "TOTAL_WA_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, (128 * 1024 * 1024));

    IDP_DEF(UInt,  "__TEMP_CHECK_UNIQUE_FOR_UPDATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // 8개의 Extent, 4MB단위로 할당.
    IDP_DEF(UInt, "__TEMP_MIN_INIT_WAEXTENT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 8 );

    // create temp table 시점에, 만약 TOTAL_WA_EXTENT 가 부족하면
    // 몇개를 초과해서 초기화 할당 하여 시작 할 것인지 수를 나타낸다.
    // 만약 0이면, 초기화 하지 않고 대기한다.
    IDP_DEF(UInt, "__TEMP_OVER_INIT_WAEXTENT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 4 );

    IDP_DEF(UInt, "__TEMP_ALLOC_WAEXTENT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 4 );

    IDP_DEF(UInt, "__TEMP_INIT_WASEGMENT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 8 );

    /* TempTable용 Sort시,  QuickSort로 부분부분을 정렬 후, 이를 MergeSort로
     * 합치는 식으로 동작한다. 이때 QuickSort가 정렬할 부분의 크기를 아래
     * Property로 조절한다. */
    IDP_DEF(UInt,  "TEMP_SORT_PARTITION_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX,  4096);

    /* TempTable용 Sort시, Sort영역과 Sort결과 저장 Group으로 나뉘어진다.
     * 이중 Sort영역의 크기를 결정한다. (이에따라 저장 Group은 나머지를
     * 차지한다. */
    IDP_DEF(UInt,  "TEMP_SORT_GROUP_RATIO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            5, 90, 80 );

    /* Temp의 Hash는, Hash Slot과 Sub Hash로 나뉜다.
     * 이때 Sub Hash 에 해당하는 영역이다.
     * 전체 HASH_AREA_SIZE에 대한 비율로 설정하지만
     * 실제 크기는 512KB단위로 설정된다. */
    IDP_DEF(UInt,  "__TEMP_SUBHASH_GROUP_RATIO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 30, 4);

    /* Temp의 Hash는, Hash Slot과 Sub Hash로 나뉜다.
     * 이때 Sub Hash 에 해당하는 영역이다.
     * 전체 HASH_AREA_SIZE에 대한 비율로 설정하지만
     * 실제 크기는 1MB단위로 설정된다. */
    IDP_DEF(UInt,  "TEMP_HASH_GROUP_RATIO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 90, 6 );

    /* WA 가 부족할 경우, 몇번 재시도 할지를 결정한다 */
    IDP_DEF(UInt,  "TEMP_ALLOC_TRY_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX,  10000 );

    /* 페이지에 공간이 부족하여 Row를 기록할 수 없는 경우, 해당 Row가 아래
     * 크기보다 크면 쪼개서 기록하고 작으면 페이지에 빈공간을 남긴체 다음 Row
     * 에 저장한다. */
    IDP_DEF(UInt,  "TEMP_ROW_SPLIT_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 8192, 1024);

    /* TempTable 연산 중 다른 작업을 기다려야 할때, 쉬는 시간이다. */
    IDP_DEF(UInt, "TEMP_SLEEP_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1000 );

    /* Temp연산시, RangeScan을 위한 Index에서 Key 하나의 최대 크기이다. Key가
     * 이보다 크면, 나머지 부분을 ExtraRow로 나누어 저장한다. */
    IDP_DEF(UInt, "TEMP_MAX_KEY_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            64, 4096, 512 );

    /* Temp연산 중 각 Temp연산들의 진행여부를 감시하기 위한 StatsWatchArray
     * 의 크기이다. 이것이 작으면 재활용이 빨라져, 이전 Temp통계가 빨리
     * 사라진다. */
    IDP_DEF(UInt, "TEMP_STATS_WATCH_ARRAY_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10, ID_UINT_MAX, 1000 );

    /* StatsWatchArray에 작업을 등록하기 위한 기준 시간이다. 이 시간보다 오래
     * 걸리는 Temp연산은 StatsWatchArray에 등록된다. */
    IDP_DEF(UInt, "TEMP_STATS_WATCH_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10 );

    /* 문제발생시의 Dump를 저장할 DIRECTORY. dumptd로 분석 가능하다. */
    IDP_DEF(String, "TEMPDUMP_DIRECTORY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc");

    /* BUG-45403 문제발생시의 Dump 를 선택 할수 있다. */
    IDP_DEF(UInt , "__TEMPDUMP_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* 예외처리 Test를 위하여, 연산을 N회 수행하면 Abort시킨다. */
    IDP_DEF(UInt, "__SM_TEMP_OPER_ABORT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0 );

    IDP_DEF(UInt, "__WCB_CLEAN_MEMSET",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF( UInt, "TEMP_HASH_BUCKET_DENSITY",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE |
             IDP_ATTR_CK_CHECK,
             1, 100, 1 );

    IDP_DEF(UInt, "__TEMP_HASH_FETCH_SUBHASH_MAX_RATIO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10, 99, 90 );

/* --------------------------------------------------------------------
 * PROJ-1629
 * Memory Index 구축시 In-Memory Sorting 영역
 * Min = 1204, Default = 32768
 * ----------------------------------------------------------------- */

    IDP_DEF(ULong, "MEMORY_INDEX_BUILD_RUN_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (1024), ID_ULONG_MAX, ( 128 * 1024));

/* --------------------------------------------------------------------
 * PROJ-1629
 * Memory Index 구축시 Key Value를 사용하기 위한 Threshold
 * Min = 0, Default = 64
 * ----------------------------------------------------------------- */

    IDP_DEF(ULong, "MEMORY_INDEX_BUILD_VALUE_LENGTH_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, ( 64 ));

/* --------------------------------------------------------------------
 * PROJ-2162 RestartRiskReduction Begin
 * ----------------------------------------------------------------- */

/* --------------------------------------------------------------------
 * 구동 실패시, 이 Property를 올려서 서버를 구동시킬 수 있습니다.
 *
 * RECOVERY_NORMAL(0)    - Default
 * RECOVERY_EMERGENCY(1) - Check and Ignore inconsistent object.
 * RECOVERY_SKIP(2)      - Skip recovery.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__EMERGENCY_STARTUP_POLICY",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

/* --------------------------------------------------------------------
 * 구동 실패로 DB가 비정상적일 경우, DB는 상황이 악화되는 것을 막기 위해
 * 비정상 객체에 대한 접근을 막습니다. 이 Property는 그러한 접근을 허용
 * 하게 하고, 상황 악화를 막기 위해 다소 느리더라도 안전한 접근을 하게
 * 합니다.
 * 0 - Default
 * 1 - Inconsistent Page등은 제외하며 오류를 무시하며 탐색
 * 2 - 오류를 무시하며 탐색
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__CRASH_TOLERANCE",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

/* --------------------------------------------------------------------
 * 아래 세 Property가 하나의 그룹으로, 해당 LSN의 Log를 무시합니다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__SM_IGNORE_LFGID_IN_STARTUP",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "__SM_IGNORE_FILENO_IN_STARTUP",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "__SM_IGNORE_OFFSET_IN_STARTUP",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

/* --------------------------------------------------------------------
 * SpaceID * 2^32 + PageID의 값을 설정하면, 해당 페이지를 무시합니다.
 * ----------------------------------------------------------------- */
    IDP_DEF(ULong, "__SM_IGNORE_PAGE_IN_STARTUP",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 0);

/* --------------------------------------------------------------------
 * TableOID를 입력하면, 해당 Table을 구동시 무시합니다.
 * ----------------------------------------------------------------- */
    IDP_DEF(ULong, "__SM_IGNORE_TABLE_IN_STARTUP",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 0);

/* --------------------------------------------------------------------
 * IndexID를 입력하면, 해당 Index를 구동시 무시합니다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__SM_IGNORE_INDEX_IN_STARTUP",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

/* --------------------------------------------------------------------
 * RedoLogic과 ServiceLog간의 일치성 비교를 통해 Bug를 찾습니다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__SM_ENABLE_STARTUP_BUG_DETECTOR",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

/* --------------------------------------------------------------------
 * Minitransaction Rollback에 관한 테스트를 한다.
 * 설정된 값 만큼 MtxCommit이 이루어진 후 Rollback을 시도한다.
 * Debug모드에서만 동작한다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__SM_MTX_ROLLBACK_TEST",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

/* --------------------------------------------------------------------
 * PROJ-2162 RestartRiskReduction End
 * ----------------------------------------------------------------- */

/* --------------------------------------------------------------------
 * BUG-38515 서버 시작시 SCN을 체크하여 SCN이 INFINITE 값 일 경우 서버를 멈춘다.  
 * 이 프로퍼티는 그 상황에서 서버를 죽이지 않고 정보를 출력하기 위한 역할을 한다. 
 * 이 값이 0일 경우 SCN이 INFINITE일 경우 서버를 정지한다.
 * 이 값이 1일 경우 SCN이 INFINITE일 경우에도 서버를 죽이지 않고
 * 관련 정보를 출력한 후 그대로 진행한다.
 * BUG-41600 값이 2 일 경우를 추가한다. SCN이 INFINITE인 경우 테이블을 보정한다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__SM_SKIP_CHECKSCN_IN_STARTUP",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

/* --------------------------------------------------------------------
 * BUG-38515 __SM_SKIP_CHECKSCN_IN_STARTUP 히든 프로퍼티를 사용시 분석을 돕기 위해
 * Legacy Tx에 관련하여 addLegacyTrans와 removeLegacyTrans 함수에서 legacy Tx가 
 * 추가/제거 될 때마다 trc 로그를 남긴다. 
 * 단, legacy Tx가 많을 경우 성능에 영향을 줄 수 있으므로 
 * 히든 프로퍼티로 trc 로그를 남길지 여부를 선택할 수 있도록 한다.
 * 이 값이 0일 경우 add/remove LegacyTrans와 관련된 trc 로그를 남기지 않는다.
 * 이 값이 1일 경우 add/remove LegacyTrans와 관련된 trc 로그를 남긴다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__TRCLOG_LEGACY_TX_INFO",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

/* --------------------------------------------------------------------
 * When Recovery Fails, Turn On This Property and Skip REDO & UNDO
 * CAUTION : DATABASE WILL NOT BE CONSISTENT!!
 * Min = 0, Default = 0
 * BUG-32632 User Memory Tablesapce에서 Max size를 무시하는 비상용 Property추가
 *           User Memory Tablespace의 Max Size를 무시하고 무조건 확장
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__EMERGENCY_IGNORE_MEM_TBS_MAXSIZE",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // For Calculation

    // BUG-49062: 시퀀스 캐싱 중 업데이트가 발생 할 때, 따로 트랜잭션을 사용한다.
    // 0: 따로 트랜잭션 사용하지 않음
    // 1: 트랜잭션 사용
    IDP_DEF(UInt, "__SEQ_CACHE_UPT_TX_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // ==================================================================
    // MM Multiplexing Properties
    // ==================================================================

    IDP_DEF(UInt, "MULTIPLEXING_THREAD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, IDL_MIN(idlVA::getProcessorCount(), 1024));

    IDP_DEF(UInt, "MULTIPLEXING_MAX_THREAD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 1024);

#if defined(WRS_VXWORKS) || defined(VC_WINCE)
    IDP_DEF(UInt, "MULTIPLEXING_POLL_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            100000, 300000, 100000);
#else
    IDP_DEF(UInt, "MULTIPLEXING_POLL_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1000, 1000000, 10000);
#endif

    IDP_DEF(UInt, "MULTIPLEXING_CHECK_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            100000, 10000000, 200000);

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       service thread의 초기 lifespan 값.
       service thread가 생성되자마자 종료하는것을 방지하기 위해
       이 property를 두었다.
     */
    IDP_DEF(UInt, "SERVICE_THREAD_INITIAL_LIFESPAN",
            IDP_ATTR_SL_ALL      |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            30,ID_UINT_MAX , 6000);
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       idle 한 thread가 계속 live하기 위한 최소 assigned 된 task갯수.
     */
    IDP_DEF(UInt, "MIN_TASK_COUNT_FOR_THREAD_LIVE",
            IDP_ATTR_SL_ALL      |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,1024 , 1);
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       a service thread의 busy degree를 정의할때 사용되며,
       a service thread가 busy일때 penalty로 사용된다.
     */
    IDP_DEF(UInt, "BUSY_SERVICE_THREAD_PENALTY",
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,ID_UINT_MAX ,128);
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       max a idle thread 에서 min a idle thread으로 migration할때
       task를 수신하는 idle thread의 task  변화폭이
       이 property보다 커야지 task migration을 허용한다.
     */
    IDP_DEF(UInt, "MIN_MIGRATION_TASK_RATE",
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10,100000 ,50);

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       a idle thread에 할당된 평균 average task갯수가
       a service thread당 평균 Task갯수보다  크고,
       그 비율이 NEW_SERVICE_CREATE_RATE 이상 벌어졌고,
       그 편차가 NEW_SERVICE_CREATE_RATE_GAP 이상 벌어졌을때
       추가적으로 service thread를  생성하도록 결정한다.
     */
    IDP_DEF(UInt, "NEW_SERVICE_CREATE_RATE",
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            100,100000 , 200);

    IDP_DEF(UInt, "NEW_SERVICE_CREATE_RATE_GAP",
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,100000 ,2);

    IDP_DEF(UInt, "SERVICE_THREAD_EXIT_RATE",
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            100,100000 , 150);

    //fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
    //QueryTime Out이 아니라,Property를 제공해야 함.
    IDP_DEF(UInt, "XA_ROLLBACK_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, UINT_MAX, 120);


    // ==================================================================
    // MM Queue Hash Table Sizes
    // ==================================================================

    IDP_DEF(UInt, "QUEUE_GLOBAL_HASHTABLE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            50, ID_UINT_MAX, 50);

    IDP_DEF(UInt, "QUEUE_SESSION_HASHTABLE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10, ID_UINT_MAX, 10);

    //fix BUG-30949 A waiting time for enqueue event in transformed dedicated thread should not be infinite.
    IDP_DEF(ULong, "QUEUE_MAX_ENQ_WAIT_TIME",
                IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
                IDP_ATTR_IU_ANY |
                IDP_ATTR_MS_ANY |
                IDP_ATTR_LC_INTERNAL |
                IDP_ATTR_RD_WRITABLE |
                IDP_ATTR_ML_JUSTONE  |
                IDP_ATTR_CK_CHECK,
                10,ID_ULONG_MAX ,3000000);
    // ==================================================================
    // MM  PROJ-1436 SQL-Plan Cache
    // ==================================================================
    // default 64M
    IDP_DEF(ULong, "SQL_PLAN_CACHE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX,67108864);

    IDP_DEF(UInt, "SQL_PLAN_CACHE_BUCKET_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            5, 4096,127);

    IDP_DEF(UInt, "SQL_PLAN_CACHE_INIT_PCB_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1024,32);

    IDP_DEF(UInt, "SQL_PLAN_CACHE_INIT_PARENT_PCO_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1024,32);

    IDP_DEF(UInt, "SQL_PLAN_CACHE_INIT_CHILD_PCO_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1024,32);

   // fix BUG-29384,The upper bound of SQL_PLAN_CACHE_HOT_REGION_LRU_RATIO should be raised up to 100%
    IDP_DEF(UInt, "SQL_PLAN_CACHE_HOT_REGION_LRU_RATIO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10, 100,50);

    IDP_DEF(UInt, "SQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024, 1);
    //fix BUG-31150, It needs to add  the property for frequency of  hot region LRU  list.
    IDP_DEF(UInt, "SQL_PLAN_CACHE_HOT_REGION_FREQUENCY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2,ID_UINT_MAX,3);

    /* BUG-35521 Add TryLatch in PlanCache. */
    /* BUG-35630 Change max value to UINT_MAX */
    IDP_DEF(UInt, "SQL_PLAN_CACHE_PARENT_PCO_XLATCH_TRY_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 100);

    /* BUG-36205 Plan Cache On/Off property for PSM */
    IDP_DEF(UInt, "SQL_PLAN_CACHE_USE_IN_PSM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* This property indicates the initially allocated ratio of number of StatementPageTables
       to MAX_CLIENT in mmcStatementManager. */
    // default = 10%
    IDP_DEF(UInt, "STMTPAGETABLE_PREALLOC_RATIO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100, 10);

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* maxinum count of free-list elements of the session mutexpool */
    IDP_DEF(UInt, "SESSION_MUTEXPOOL_FREE_LIST_MAXCNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 64);

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* the number of initially initialized mutex in the free-list of the session mutexpool */
    IDP_DEF(UInt, "SESSION_MUTEXPOOL_FREE_LIST_INITCNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024, 2);

    /* These properties decide the last parameter of iduMemPool::initilize */
    IDP_DEF(UInt, "MMT_SESSION_LIST_MEMPOOL_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512,
            IDP_LIMITED_CORE_COUNT(16,128));

    IDP_DEF(UInt, "MMC_MUTEXPOOL_MEMPOOL_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512,
            IDP_LIMITED_CORE_COUNT(8,64));

    IDP_DEF(UInt, "MMC_STMTPAGETABLE_MEMPOOL_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512,
            IDP_LIMITED_CORE_COUNT(4,12));

    /* PROJ-1438 Job Scheduler */
    IDP_DEF(UInt, "JOB_THREAD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 128, 0 );

    /* PROJ-1438 Job Scheduler */
    IDP_DEF(UInt, "JOB_THREAD_QUEUE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            4, 1024, 64 );

    /* PROJ-1438 Job Scheduler */
    IDP_DEF(UInt, "JOB_SCHEDULER_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // ==================================================================
    // QP Properties
    // ==================================================================

    IDP_DEF(UInt, "MAX_CLIENT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            // BUG-33092 should change of MAX_CLIENT property's min and max value
            1, 65535, 1000);

    IDP_DEF(UInt, "CM_DISCONN_DETECT_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 3);

    IDP_DEF(SInt, "DDL_LOCK_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            -1, 65535, 0);

    IDP_DEF(UInt, "AUTO_COMMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-45295 non-autocommit session에서 tx를 미리 생성하지 않는다. */
    IDP_DEF(UInt, "TRANSACTION_START_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-19465 : CM_Buffer의 pending list를 제한
    IDP_DEF(UInt, "CM_BUFFER_MAX_PENDING_LIST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512, 512 );

    // ==================================================================
    // RP Properties
    // ==================================================================

    // BUG-14325
    // PK UPDATE in Replicated table.
    IDP_DEF(UInt, "REPLICATION_UPDATE_PK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "REPLICATION_PORT_NO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65535, 0);

    IDP_DEF(UInt, "REPLICATION_IB_PORT_NO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65535, 0);

    IDP_DEF(UInt, "REPLICATION_IB_LATENCY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(SInt, "REPLICATION_MAX_LOGFILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65535, 0);

    IDP_DEF(UInt, "REPLICATION_UPDATE_REPLACE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "REPLICATION_INSERT_REPLACE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "REPLICATION_CONNECT_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "REPLICATION_RECEIVE_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 7200);

    IDP_DEF(UInt, "REPLICATION_SENDER_SLEEP_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 60);

    IDP_DEF(UInt, "REPLICATION_RECEIVER_XLOG_QUEUE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 100);

    IDP_DEF(UInt, "REPLICATION_ACK_XLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 100);

    IDP_DEF(UInt, "REPLICATION_PROPAGATION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 0, 0);

    IDP_DEF(UInt, "REPLICATION_HBT_DETECT_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 6);

    IDP_DEF(UInt, "REPLICATION_HBT_DETECT_HIGHWATER_MARK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 5);

    IDP_DEF(ULong, "REPLICATION_SYNC_TUPLE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 500000 );

    IDP_DEF(UInt, "REPLICATION_SYNC_LOCK_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 30);

    IDP_DEF(UInt, "REPLICATION_TIMESTAMP_RESOLUTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    //fix BUG-9894
    IDP_DEF(UInt, "REPLICATION_CONNECT_RECEIVE_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 60);
    //fix BUG-9343
    IDP_DEF(UInt, "REPLICATION_SENDER_AUTO_START",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF( UInt, "REPLICATION_SENDER_START_AFTER_GIVING_UP",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );

    //TASK-2359
    IDP_DEF(UInt, "REPLICATION_PERFORMANCE_TEST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0); // 0: ENABLE ALL, 1: RECEIVER, 2:NETWORK

    IDP_DEF(UInt, "REPLICATION_PREFETCH_LOGFILE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024, 3);

    IDP_DEF(UInt, "REPLICATION_SENDER_SLEEP_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10000);

    IDP_DEF(UInt, "REPLICATION_KEEP_ALIVE_CNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 600);

    // deprecated
    IDP_DEF(UInt, "REPLICATION_SERVICE_WAIT_MAX_LIMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 50000);

    /*PROJ-1670 replication Log Buffer*/
    IDP_DEF(UInt, "REPLICATION_LOG_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 4095, 0);
    /*PROJ-1608 Recovery From Replication*/
    IDP_DEF(UInt, "REPLICATION_RECOVERY_MAX_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, ID_UINT_MAX);
    IDP_DEF(UInt, "REPLICATION_RECOVERY_MAX_LOGFILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "__REPLICATION_RECOVERY_REQUEST_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 10, 5);

    // PROJ-1442 Replication Online 중 DDL 허용
    IDP_DEF(UInt, "REPLICATION_DDL_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF( UInt, "REPLICATION_DDL_ENABLE_LEVEL",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 2, 0 );

    // PROJ-1705 RP
    IDP_DEF(UInt, "REPLICATION_POOL_ELEMENT_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            128, 65536, 256);    // DEFAULT : 128 Byte, MAX : 64K

    // PROJ-1705 RP
    IDP_DEF(UInt, "REPLICATION_POOL_ELEMENT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 10);       // DEFAULT : 10, MAX : 1024

    IDP_DEF(UInt, "REPLICATION_EAGER_PARALLEL_FACTOR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, IDP_REPLICATION_MAX_EAGER_PARALLEL_FACTOR, 
            IDL_MIN(IDL_MAX((idlVA::getProcessorCount()/2),1),
                    IDP_REPLICATION_MAX_EAGER_PARALLEL_FACTOR));  // DEFAULT : CPU 개수, MAX : 512

    IDP_DEF(UInt, "REPLICATION_COMMIT_WRITE_WAIT_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "REPLICATION_SERVER_FAILBACK_MAX_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, ID_UINT_MAX);

    IDP_DEF(UInt, "__REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 30);

    IDP_DEF(UInt, "REPLICATION_FAILBACK_INCREMENTAL_SYNC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "REPLICATION_MAX_LISTEN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512, 32);

    IDP_DEF(UInt, "REPLICATION_TRANSACTION_POOL_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 2 );

    IDP_DEF(UInt, "REPLICATION_STRICT_EAGER_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 ); /*1 is strict eager mode, 0 is loose eager mode*/

    IDP_DEF(SInt, "REPLICATION_EAGER_MAX_YIELD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_SINT_MAX, 20000 );

    /* BUG-36555 */
    IDP_DEF(UInt, "REPLICATION_BEFORE_IMAGE_LOG_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 ); /* 0: disable, 1: enable */

    IDP_DEF(UInt, "REPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 5 );

    IDP_DEF(SInt, "REPLICATION_SENDER_COMPRESS_XLOG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(SInt, "REPLICATION_SENDER_COMPRESS_XLOG_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1000, 1 );

    /* BUG-37482 */
    IDP_DEF(UInt, "REPLICATION_MAX_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 10240, 32);

    /* PROJ-2261 */
    IDP_DEF( UInt, "REPLICATION_THREAD_CPU_AFFINITY",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 3, 3 );

    /* PROJ-2336 */
    IDP_DEF(SInt, "REPLICATION_ALLOW_DUPLICATE_HOSTS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-38102 */
    IDP_DEF(SInt, "REPLICATION_SENDER_ENCRYPT_XLOG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-38716 */
    IDP_DEF( UInt, "REPLICATION_SENDER_SEND_TIMEOUT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 7200 );

    IDP_DEF( ULong, "REPLICATION_GAPLESS_MAX_WAIT_TIME",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_ULONG_MAX, 2000 );

    IDP_DEF( ULong, "REPLICATION_GAPLESS_ALLOW_TIME",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_ULONG_MAX, 10000000 );

    IDP_DEF( UInt, "REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             2, ID_UINT_MAX, 20 );

    IDP_DEF( SInt, "REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( UInt, "REPLICATION_FORCE_RECEIVER_PARALLEL_APPLY_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 0 );

    IDP_DEF( UInt, "REPLICATION_GROUPING_TRANSACTION_MAX_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1, 1000, 5 );

    IDP_DEF( UInt, "REPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1, ID_UINT_MAX, 2 );

    /* BUG-41246 */
    IDP_DEF( UInt, "REPLICATION_RECONNECT_MAX_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 0 );

    IDP_DEF( UInt, "REPLICATION_SYNC_APPLY_METHOD",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( UInt, "REPLICATION_EAGER_KEEP_LOGFILE_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 10, 3 );
   
    IDP_DEF( UInt, "REPLICATION_FORCE_SQL_APPLY_ENABLE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( UInt, "REPLICATION_SQL_APPLY_ENABLE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( UInt, "__REPLICATION_SET_RESTARTSN",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( UInt, "REPLICATION_SENDER_RETRY_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 5 );

    IDP_DEF( UInt, "REPLICATION_ALLOW_QUEUE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( UInt, "REPLICATION_RECEIVER_APPLIER_YIELD_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, UINT_MAX, 20000 );

    // PROJ-1723
    IDP_DEF(UInt, "DDL_SUPPLEMENTAL_LOG_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* PROJ-2677 DDL 복제 허용*/
    IDP_DEF( UInt, "REPLICATION_DDL_SYNC",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( UInt, "REPLICATION_DDL_SYNC_TIMEOUT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 7200 );

    IDP_DEF(ULong, "REPLICATION_GAP_UNIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_ULONG_MAX, 1024 * 1024 );

    IDP_DEF(UInt, "REPLICATION_CHECK_SRID_IN_GEOMETRY_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    IDP_DEF(UInt, "XA_HEURISTIC_COMPLETE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    IDP_DEF(UInt, "XA_INDOUBT_TX_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 60);

    IDP_DEF(UInt, "OPTIMIZER_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0     /* 0: cost, 1: rule*/);

    IDP_DEF(UInt, "PROTOCOL_DUMP", // 0 : inactive, 1 : active
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* PROJ-2725 Consistent Replication For Shard Cluster System */
    /* XLOGFILE이 저장될 위치 */
    IDP_DEF(String, "XLOGFILE_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_DF_DROP_DEFAULT |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* 미리 생성할 XLOGFILE의 수 */
    IDP_DEF(UInt, "XLOGFILE_PREPARE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 5);

    IDP_DEF(ULong, "XLOGFILE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_AL_SET_VALUE( sLogFileAlignSize ) |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            64 * 1024, ID_ULONG_MAX, 10 * 1024 * 1024);

    IDP_DEF(UInt, "XLOGFILE_REMOVE_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            3, IDV_MAX_TIME_INTERVAL_SEC, 600);

    IDP_DEF(UInt, "XLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 10);

    // BUG-44742
    // NORMALFORM_MAXIMUM의 기본값을 128에서 2048로 변경합니다.
    IDP_DEF(UInt, "NORMALFORM_MAXIMUM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 2048);

    IDP_DEF(UInt, "EXEC_DDL_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0     /* 0: DDL can be executed.*/ );

    IDP_DEF(UInt, "SELECT_HEADER_DISPLAY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1     );

    IDP_DEF(UInt, "LOGIN_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "IDLE_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0     /* default 0 : forever running */);

    IDP_DEF(UInt, "QUERY_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 600);

    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    IDP_DEF(UInt, "DDL_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "FETCH_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 60);

    IDP_DEF(UInt, "UTRANS_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 3600);
    
    // PROJ-1665 : session property로서 parallel_dml_mode
    IDP_DEF(UInt, "PARALLEL_DML_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "UPDATE_IN_PLACE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0     /* default 0 : forever running */);

    IDP_DEF(UInt, "MEMORY_COMPACT_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 60    /* default :check for every 1 min */);

    IDP_DEF(UInt, "ADMIN_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* PROJ-2563 */
    IDP_DEF( UInt, "__REPLICATION_USE_V6_PROTOCOL",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF(UInt, "_STORED_PROC_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "__SHOW_ERROR_STACK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(UInt, "TIMER_RUNNING_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 3, IDV_TIMER_DEFAULT_LEVEL );

    IDP_DEF(UInt, "TIMER_THREAD_RESOLUTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            50, 10000000, 1000 );

    /*
     * TASK-2356 [제품문제분석] DRDB의 DML 문제 파악
     * Altibase Wait Interface 통계정보 수집 여부 설정
     */
    IDP_DEF(UInt, "TIMED_STATISTICS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-38946 display name */
    IDP_DEF(UInt, "COMPATIBLE_DISPLAY_NAME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* ------------------------------------------------------------------
     *   SERVER
     * --------------------------------------------------------------*/
    // MSG LOG Property

    IDP_DEF(UInt, "ALL_MSGLOG_FLUSH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);


    IDP_DEF(String, "SERVER_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, IDE_BOOT_LOG_FILE);

    IDP_DEF(String, "SERVER_MSGLOG_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc");

    IDP_DEF(UInt, "SERVER_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "SERVER_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "SERVER_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 7);

    /* ------------------------------------------------------------------
     *   QP
     * --------------------------------------------------------------*/
    IDP_DEF(String, "QP_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_qp.log");

    IDP_DEF(UInt, "QP_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "QP_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    // BUG-24354  qp_msglog_flag=2, PREPARE_STMT_MEMORY_MAXIMUM = 200M 로 Property 의 Default 값 변경
    IDP_DEF(UInt, "QP_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 2);

    /* ------------------------------------------------------------------
     *  JOB BUG-49108
     * --------------------------------------------------------------*/
    IDP_DEF(String, "JOB_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_job.log");

    IDP_DEF(UInt, "JOB_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "JOB_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "JOB_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1);

    /* ------------------------------------------------------------------
     *  SD  BUG-46138 
     * --------------------------------------------------------------*/
    IDP_DEF(String, "SD_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_sd.log");

    IDP_DEF(UInt, "SD_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "SD_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "SD_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 65537);

    /* ------------------------------------------------------------------
     *   SM
     * --------------------------------------------------------------*/
    IDP_DEF(String, "SM_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_sm.log");

    IDP_DEF(UInt, "SM_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "SM_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "SM_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 2147483647);

    IDP_DEF(UInt, "SM_XLATCH_USE_SIGNAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* ------------------------------------------------
     *  MUTEX/LATCH Choolse
     * ----------------------------------------------*/

    /* BUG-33203 [sm_transaction] change the default value of 
     * __DBMS_STAT_METHOD, MUTEX_TYPE, and LATCH_TYPE properties */
    IDP_DEF(UInt, "LATCH_TYPE", // 0 : posix, 1 : native
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "LATCH_MINSLEEP", // in microseconds
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1000, 50);

    IDP_DEF(UInt, "LATCH_MAXSLEEP", // in microseconds
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 99999, 1000);

    IDP_DEF(UInt, "MUTEX_SLEEPTYPE", // 0 : sleep, 1 : thryield
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDU_MUTEX_SLEEP,
            IDU_MUTEX_YIELD,
            IDU_MUTEX_SLEEP);

    IDP_DEF(UInt, "MUTEX_TYPE", // 0 : posix, 1 : native
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    //BUG-17371
    // 0 = check disable, 1= check enable
    IDP_DEF(UInt, "CHECK_MUTEX_DURATION_TIME_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

/* BUG-46911
 * - Linux, 24 코어보다 클 경우 스핀 카운트 값이 크면 
 * select 성능 저하 발생. 테스트 결과에 따라 10,000으로 변경  
 */  
#if defined(ALTI_CFG_OS_LINUX)
    sDefaultMutexSpinCnt = ( (idlVA::getProcessorCount() > 24) ?
                             10000 : (idlVA::getProcessorCount() - 1)*10000 );
#else
    sDefaultMutexSpinCnt = (idlVA::getProcessorCount() - 1)*10000;
#endif

    IDP_DEF(UInt, "LATCH_SPIN_COUNT", // 0 : posix, 1 : native
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 
            (idlVA::getProcessorCount() > 1) ? sDefaultMutexSpinCnt : 1);

    IDP_DEF(UInt, "MUTEX_SPIN_COUNT", // 0 : posix, 1 : native
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 
            (idlVA::getProcessorCount() > 1) ? sDefaultMutexSpinCnt : 1);

    IDP_DEF(UInt, "NATIVE_MUTEX_SPIN_COUNT", // for IDU_MUTEX_KIND_NATIVE2
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,
            ID_UINT_MAX,
            (idlVA::getProcessorCount() > 1) ? sDefaultMutexSpinCnt : 1);

    // BUG-28856 Logging 병목제거 로 Native3추가
    // 높은 spin count를 가진 native mutex에서 Logging성능 향상
    // BUGBUG Default값은 조금 더 테스트가 필요함
    // for IDU_MUTEX_KIND_NATIVE_FOR_LOGGING
    IDP_DEF(UInt, "NATIVE_MUTEX_SPIN_COUNT_FOR_LOGGING",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,
            ID_UINT_MAX,
            (idlVA::getProcessorCount() > 1) ?
            (idlVA::getProcessorCount() - 1)*100000 : 1);

    /* BUG-35392 */
    // BUG-28856 logging 병목제거
    // Logging 시 사용하는 log alloc Mutex의 종류를 결정
    IDP_DEF(UInt, "LOG_ALLOC_MUTEX_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 3);

    /* BUG-35392 */
    // BUG-28856 logging 병목제거
    // Logging 시 사용하는 log alloc Mutex의 종류를 결정
    IDP_DEF(UInt, "FAST_UNLOCK_LOG_ALLOC_MUTEX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "__LOG_READ_METHOD_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "__LOG_COMPRESSION_ACCELERATION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1000, 1);

    IDP_DEF(String, "DEFAULT_DATE_FORMAT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_ASCII    |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"DD-MON-RRRR");

    IDP_DEF ( UInt, "__MUTEX_POOL_MAX_SIZE",
              IDP_ATTR_SL_ALL |
              IDP_ATTR_SH_ALL |
              IDP_ATTR_IU_ANY |
              IDP_ATTR_MS_ANY |
              IDP_ATTR_LC_INTERNAL |
              IDP_ATTR_RD_WRITABLE |
              IDP_ATTR_ML_JUSTONE  |
              IDP_ATTR_CK_CHECK,
              0, ID_UINT_MAX, (256*1024) );

#if defined(ALTIBASE_PRODUCT_XDB)
    /* TASK-4690
     * 메모리 인덱스 INode의 최대 slot 갯수.
     * 이 값의 *2 한 값이 LNode의 최대 slot 갯수가 된다. */
    IDP_DEF(UInt, "__MEM_BTREE_MAX_SLOT_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2, 1024, 128);
#endif

#if defined(ALTIBASE_PRODUCT_HDB)
    /* PROJ-2433
     * MEMORY BTREE INDEX NODE SIZE */
    IDP_DEF(UInt, "__MEM_BTREE_NODE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024,
            IDP_SM_PAGE_SIZE,
            4096);

    /* PROJ-2433
     * MEMORY BTREE DIRECT KEY INDEX 사용시, default max key size */
    IDP_DEF(UInt, "__MEM_BTREE_DEFAULT_MAX_KEY_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            8,
            IDP_SM_PAGE_SIZE / 3,
            8);

    /* PROJ-2433
     *  Force makes all memory btree index to direct key index when column created. (if possible) */
    IDP_DEF(SInt, "__FORCE_INDEX_DIRECTKEY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,
            1,
            0);
#endif

    /* BUG-41121
     * Force to make Persistent Index, When new memory index is created.
     *
     * BUG-41541 Disable Memory Persistent Index and Change Hidden Property
     * 0일 경우 : persistent index 미사용(기본)
     * 1일 경우 : persistent로 세팅 된 index만 persistent로 사용
     * 2일 경우 : 모든 index를 persistent로 사용 */
    
    IDP_DEF(SInt, "__FORCE_INDEX_PERSISTENCE_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,
            2,
            0);
#if 0
    /* TASK-4690
     * 인덱스 cardinality 통계 다중화 */
    IDP_DEF(UInt, "__INDEX_STAT_PARALLEL_FACTOR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512, IDL_MIN(idlVA::getProcessorCount() * 2, 512));
#endif
    /* TASK-4690
     * 0이면 iduOIDMemory 를 사용하고,
     * 1이면 iduMemPool 을 사용한다. */
    IDP_DEF(UInt, "__TX_OIDLIST_MEMPOOL_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* ------------------------------------------------------------------
     *   RP
     * --------------------------------------------------------------*/
    IDP_DEF(String, "RP_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_rp.log");

    IDP_DEF(String, "RP_CONFLICT_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_rp_conflict.log");

    IDP_DEF(UInt, "RP_CONFLICT_MSGLOG_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(String, "RP_CONFLICT_MSGLOG_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc");

    // Update Transaction이 몇 개 이상일때 Group Commit을 동작시킬 것인지
    IDP_DEF(UInt, "LFG_GROUP_COMMIT_UPDATE_TX_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 80);

    // 기본적으로 1ms마다 한번씩 Disk I/O를 수행
#if defined(WRS_VXWORKS) || defined(VC_WINCE)
    IDP_DEF(UInt, "LFG_GROUP_COMMIT_INTERVAL_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 100000);
#else
    IDP_DEF(UInt, "LFG_GROUP_COMMIT_INTERVAL_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1000);
#endif

    // 기본적으로 100us만에 한번씩 깨어나서 로그가 이미 Sync되었는지 체크
#if defined(WRS_VXWORKS) || defined(VC_WINCE)
    IDP_DEF(UInt, "LFG_GROUP_COMMIT_RETRY_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_USEC, 100000);
#else
    IDP_DEF(UInt, "LFG_GROUP_COMMIT_RETRY_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_USEC, 100);
#endif

    // Update Transaction이 몇 개 이상일때 Group Commit을 동작시킬 것인지
    IDP_DEF(UInt, "GROUP_COMMIT_UPDATE_TX_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 80);

    // 기본적으로 1ms마다 한번씩 Disk I/O를 수행
#if defined(WRS_VXWORKS) || defined(VC_WINCE)
    IDP_DEF(UInt, "GROUP_COMMIT_INTERVAL_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 100000);
#else
    IDP_DEF(UInt, "GROUP_COMMIT_INTERVAL_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1000);
#endif

    // 기본적으로 100us만에 한번씩 깨어나서 로그가 이미 Sync되었는지 체크
#if defined(WRS_VXWORKS) || defined(VC_WINCE)
    IDP_DEF(UInt, "GROUP_COMMIT_RETRY_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_USEC, 100000);
#else
    IDP_DEF(UInt, "GROUP_COMMIT_RETRY_USEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_USEC, 100);
#endif

    IDP_DEF(UInt, "RP_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "RP_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "RP_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "RP_CONFLICT_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));
    
    IDP_DEF(UInt, "RP_CONFLICT_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);
    
    IDP_DEF(UInt, "RP_CONFLICT_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 6);
                
    IDP_DEF( UInt, "REPLICATION_META_ITEM_COUNT_DIFF_ENABLE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    // ==================================================================
    // iduMemory
    // ==================================================================

    // BUG-24354  qp_msglog_flag=2, PREPARE_STMT_MEMORY_MAXIMUM = 200M 로 Property 의 Default 값 변경
    IDP_DEF(ULong, "PREPARE_STMT_MEMORY_MAXIMUM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (1024*1024),      // min : 1M
            ID_ULONG_MAX,     // max
            (200*1024*1024)); // default : 200M

    IDP_DEF(ULong, "EXECUTE_STMT_MEMORY_MAXIMUM",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            (1024*1024),       // min : 1M
            ID_ULONG_MAX,      // max
            ID_ULONG(2 * 1024 * 1024 * 1024));      // default, 2G

    // ==================================================================
    // Preallocate Memory
    // ==================================================================
    IDP_DEF(ULong, "PREALLOC_MEMORY",
            IDP_ATTR_SL_ALL | IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 0);

    /* !!!!!!!!!! REGISTRATION AREA END !!!!!!!!!! */

    // ==================================================================
    // PR-14783 SYSTEM THREAD CONTROL
    // 0 : OFF, 1 : ON
    // ==================================================================

    IDP_DEF(UInt, "MEM_DELETE_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "MEM_GC_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "BUFFER_FLUSH_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "ARCHIVE_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "CHECKPOINT_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "LOG_FLUSH_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "LOG_PREPARE_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "LOG_PREREAD_THREAD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

#ifdef ALTIBASE_ENABLE_SMARTSSD
    IDP_DEF(UInt, "SMART_SSD_LOG_RUN_GC_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(String, "SMART_SSD_LOG_DEVICE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(UInt, "SMART_SSD_GC_TIME_MILLISEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 268435455, 10);
#endif /* ALTIBASE_ENABLE_SMARTSSD */

    // To verify CASE-6829
    IDP_DEF(UInt, "__SM_CHECKSUM_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // To verify CASE-6829
    IDP_DEF(UInt, "__SM_AGER_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // To verify CASE-6829
    IDP_DEF(UInt, "__SM_CHECK_DISK_INDEX_INTEGRITY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 0);

    // To BUG-27122
    IDP_DEF(UInt, "__SM_VERIFY_DISK_INDEX_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 16, 0);

    // Index Name String MaxSize
    IDP_DEF(String, "__SM_VERIFY_DISK_INDEX_NAME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, 47, (SChar *)"");

    /* ------------------------------------------------
     *  PROJ-2059: DB Upgrade Function
     * ----------------------------------------------*/

    // DataPort File을 저장할 DIRECTORY
    IDP_DEF(String, "DATAPORT_FILE_DIRECTORY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"dbs");

    // DataPort File의 Block Size
    IDP_DEF(UInt, "__DATAPORT_FILE_BLOCK_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            8192, 1024*1024*1024, 2*1024*1024 );

    // DataPort기능 사용 시 Direct IO 사용 여부
    IDP_DEF(UInt, "__DATAPORT_DIRECT_IO_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1,0 );

    //Export시 Column를 Chain(Block간 걸침)여부를 판단하는 기준값
    IDP_DEF(UInt, "__EXPORT_COLUMN_CHAINING_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            128, 8192, 128 );

    //Import시 Commit단위
    IDP_DEF(UInt, "DATAPORT_IMPORT_COMMIT_UNIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,ID_UINT_MAX,10 );

    //Import시 Statement단위
    IDP_DEF(UInt, "DATAPORT_IMPORT_STATEMENT_UNIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1,ID_UINT_MAX,50000 );


    //Import시 Direct-path Insert 동작여부
    IDP_DEF(UInt, "__IMPORT_DIRECT_PATH_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1,1 );

    //Import시 Validation 수행 여부
    IDP_DEF(UInt, "__IMPORT_VALIDATION_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1,1 );

    // Import시 SourceTable의 Partition정보와 동일할때 Filtering을 무시
    // 해주는 기능 동작 여부
    IDP_DEF(UInt, "__IMPORT_PARTITION_MATCH_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1,1 );

    /* ------------------------------------------------
     *  PROJ-2469: Optimize View Materialization
     * ----------------------------------------------*/
    IDP_DEF(UInt, "__OPTIMIZER_VIEW_TARGET_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,1,1 );

    /* ------------------------------------------------
     *  PROJ-1598: Query Profile
     * ----------------------------------------------*/
    IDP_DEF(UInt, "QUERY_PROF_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(UInt, "QUERY_PROF_BUF_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32 * 1024, ID_UINT_MAX, 1024 * 1024);

    IDP_DEF(UInt, "QUERY_PROF_BUF_FLUSH_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            512, ID_UINT_MAX, 32 * 1024);

    IDP_DEF(UInt, "QUERY_PROF_BUF_FULL_SKIP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1); // default skip!

    IDP_DEF(UInt, "QUERY_PROF_FILE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX , 100 * 1024 * 1024); // default unlimited(0) : or each file limit up to 4G

    /* BUG-36806 */
    IDP_DEF(String, "QUERY_PROF_LOG_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc");

    IDP_DEF(String, "ACCESS_LIST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_ASCII    |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* ---------------------------------------------------------------
     * PROJ-2223 Altibase Auditing 
     * -------------------------------------------------------------- */
    /* BUG-36807 */
    IDP_DEF(String, "AUDIT_LOG_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |    
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE|
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc");

    IDP_DEF(UInt, "AUDIT_BUFFER_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32 * 1024, ID_UINT_MAX, 1024 * 1024);

    IDP_DEF(UInt, "AUDIT_BUFFER_FLUSH_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            512, ID_UINT_MAX, 32 * 1024);

    IDP_DEF(UInt, "AUDIT_BUFFER_FULL_SKIP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1); // default skip!

    IDP_DEF(UInt, "AUDIT_FILE_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX , 100 * 1024 * 1024); 

    /* BUG-39760 Enable AltiAudit to write log into syslog */
    /*
       0: use private log file. as it was before ...
       1: use syslog 
       2: use syslog & facility:local0   
       3: use syslog & facility:local1   
       4: use syslog & facility:local2   
       5: use syslog & facility:local3   
       6: use syslog & facility:local4   
       7: use syslog & facility:local5   
       8: use syslog & facility:local6   
       9: use syslog & facility:local7   
    */
    IDP_DEF(UInt, "AUDIT_OUTPUT_METHOD",
            IDP_ATTR_SL_ALL      |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, (IDV_AUDIT_OUTPUT_MAX - 1), 0);
 
    IDP_DEF(String, "AUDIT_TAG_NAME_IN_SYSLOG",
            IDP_ATTR_SL_ALL      |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_SK_ASCII    |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"AUDIT");

    //BUG-21122 :
    IDP_DEF(UInt, "AUTO_REMOTE_EXEC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0); // 0: DISABLE 1:ENABLE

    // BUG-20129
    IDP_DEF(ULong, "__HEAP_MEM_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,                       // min
            ID_ULONG_MAX,            // max
            ID_ULONG_MAX);           // default

    // To fix BUG-21134
    // min : 0, max : 3, default : 0
    // 0 : no logging
    // 1 : select logging
    // 2 : select + dml logging
    // 3:  all query logging
    IDP_DEF(UInt, "__QUERY_LOGGING_LEVEL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 0);

    /* BUG-35155 Partial CNF
     * Normalize predicate partialy. (Normal form + NNF filter)
     * It may decrease output records in SCAN node
     *  and decrease intermediate tuple count. (vs NNF) */
    IDP_DEF(SInt, "OPTIMIZER_PARTIAL_NORMALIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // fix BUG-36522
    IDP_DEF(UInt, "__PSM_SHOW_ERROR_STACK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    // fix BUG-36793
    IDP_DEF(UInt, "__BIND_PARAM_DEFAULT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 32000, 32000);

    /* PROJ-2264 Dictionary table
     *  Force makes all columns to compression column when column created. (if possible)
     *  HIDDEN PROPERTY */
    IDP_DEF(SInt, "__FORCE_COMPRESSION_COLUMN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /*
     * BUG-21487     Mutex Leak List출력을 property화 해야합니다.
     */

    IDP_DEF(UInt, "SHOW_MUTEX_LEAK_LIST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1); //default는 1로서 출력하는것임.

    /* PROJ-1864  Partial Write Problem에 대한 문제 해결.
     * Recovery에서 Corrupt page 처리 정책
     * 
     * BUG-45598: 운영중 Corrupt page 처리 정책도 포함한다. 
     *
     * BUG-46182: default 값을 3에서 2로 변경  
     *
     * 0 - corrupt page를 발견하면 무시한다.
     *     Group Hdr page가 corrupt 된 경우에는 서버 종료 한다.
     * 1 - corrupt page를 발견하면 서버를 종료한다.
     * 2 - corrupt page를 발견하면 ImgLog가 있을 경우 OverWrite 한다.
     *     단 Group Hdr page가 corrupt 된 경우를 제외하고
     *     Corrupt Page를 Overwrite하지 못해도 서버 종료 하지 않는다.
     * 3 - corrupt page를 발견하면 ImgLog로 OverWrite 시도
     *     보정되지 못한 Corrupt Page가 존재한다면 서버 종료.
     * 
     * * 운영중: 0, 2 ABORT / 1, 3 FATAL 
     */ 
    IDP_DEF(UInt, "CORRUPT_PAGE_ERR_POLICY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3, 2); 

    // bug-19279 remote sysdba enable + sys can kill session
    // default: 1 (on)
    IDP_DEF(UInt, "REMOTE_SYSDBA_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    // BUG-24993 네트워크 에러 메시지 log 여부
    // default: 1 (on)
    IDP_DEF(UInt, "NETWORK_ERROR_LOG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    /* ------------------------------------------------------------------
     *   Shard Related
     * --------------------------------------------------------------*/

    IDP_DEF(UInt, "SHARD_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(UInt, "__SHARD_TEST_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-48247 */
    IDP_DEF(UInt, "__SHARD_ALLOW_AUTO_COMMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "__SHARD_LOCAL_FORCE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // BUG-47108
    IDP_DEF(UInt, "SHARD_AGGREGATION_TRANSFORM_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    IDP_DEF(UInt, "SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT_MIN,
            IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT_MAX,
            IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT_DEFAULT);

    IDP_DEF(UInt, "SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY_MIN,
            IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY_MAX,
            IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY_DEFAULT);

    /* BUG-45967 Rebuild Data 완료 대기 */
    IDP_DEF(SInt, "SHARD_REBUILD_DATA_STEP",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            -1, 1000, 0);

    IDP_DEF(UInt, "SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT_MIN,
            IDP_SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT_MAX,
            IDP_SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT_DEFAULT);

    IDP_DEF(UInt, "SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT_MIN,
            IDP_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT_MAX,
            IDP_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT_DEFAULT);

    /* BUG-45899 */
    IDP_DEF(UInt, "TRCLOG_DETAIL_SHARD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* PROJ-2701 Sharding online data rebuild */
    IDP_DEF(UInt, "SHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(UInt, "SHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(ULong, "SHARD_META_PROPAGATION_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,         /* min = 0usec */
            60000000,  /* max = 60000000usec = 60sec */
            3000000 ); /* default 3000000usec = 3sec */

    IDP_DEF(UInt, "SHARD_TRANSFORM_STRING_LENGTH_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32,    /* min = 32 bytes */
            65535, /* max */
            512 ); /* default 512 bytes */

    // BUG-47817
    IDP_DEF(UInt, "SHARD_ADMIN_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "ZOOKEEPER_LOCK_WAIT_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10,     /* min = 10sec */
            36000,  /* max = 36000sec = 10hour */
            3600 ); /* default 3600sec = 1hour */

    IDP_DEF(UInt, "SHARD_DDL_LOCK_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65535, 3);

    IDP_DEF(UInt,  "SHARD_DDL_LOCK_TRY_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX,  200 ); /* shard_ddl_lock_timeout default 3 sec * 200 => 600 sec, 10 min */

    /* 다양한 Node Crash 상황에서의 Failback TC 작성을 위해
     * FailoverForWatcher의 동작을 ON/OFF 하는 프로퍼티 
     * 0 이면 FailoverForWatcher가 동작하고 1이면 동작하지 않도록 한다.
     * MIN     : 0 
     * MAX     : 1
     * Default : 0 */
    IDP_DEF(UInt,  "__DISABLE_FAILOVER_FOR_WATCHER",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(UInt,  "SHARD_INTERNAL_REPLICATION_PARALLEL_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 512, 1 );
    
    /* ------------------------------------------------------------------
     *   Cluster Related
     * --------------------------------------------------------------*/

#ifdef ALTIBASE_PRODUCT_HDB
# define ALTIBASE_SID "altibase"
#else /* ALTIBASE_PRODUCT_HDB */
# define ALTIBASE_SID "xdbaltibase"
#endif /* ALTIBASE_PRODUCT_HDB */

    IDP_DEF(String, "SID",
            IDP_ATTR_SL_PFILE    | IDP_ATTR_SL_ENV |
            IDP_ATTR_IU_UNIQUE   |
            IDP_ATTR_MS_ANY      |
            IDP_ATTR_SK_ALNUM    |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 12, ALTIBASE_SID);

#undef ALTIBASE_SID

    IDP_DEF(String, "SPFILE",
            IDP_ATTR_SL_PFILE       | IDP_ATTR_SL_ENV |
            IDP_ATTR_IU_IDENTICAL   |
            IDP_ATTR_MS_ANY         |
            IDP_ATTR_SK_PATH        |
            IDP_ATTR_LC_INTERNAL    |       /*향후 External로 변경해야함 XXX*/
            IDP_ATTR_RD_READONLY    |
            IDP_ATTR_ML_JUSTONE     |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(UInt, "CLUSTER_DATABASE",
            IDP_ATTR_SL_ALL         |
            IDP_ATTR_IU_IDENTICAL   |       
            IDP_ATTR_MS_ANY         |
            IDP_ATTR_LC_INTERNAL    |       /*향후 External로 변경해야함 XXX*/
            IDP_ATTR_RD_READONLY    |
            IDP_ATTR_ML_JUSTONE     |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(UInt, "INSTANCE_NUMBER",
            IDP_ATTR_SL_SPFILE      |
            IDP_ATTR_IU_UNIQUE      |
            IDP_ATTR_MS_SHARE       |
            IDP_ATTR_LC_INTERNAL    |       /*향후 External로 변경해야함 XXX*/
            IDP_ATTR_RD_READONLY    |
            IDP_ATTR_ML_JUSTONE     |
            IDP_ATTR_CK_CHECK,
            1, 100, 1 );

    IDP_DEF(String, "CLUSTER_INTERCONNECTS",
            IDP_ATTR_SL_SPFILE   |
            IDP_ATTR_IU_UNIQUE   |
            IDP_ATTR_MS_SHARE    |
            IDP_ATTR_LC_INTERNAL |          /*향후 External로 변경해야함 XXX*/
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* ------------------------------------------------------------------
     *   XA: bug-24840 divide xa log
     * --------------------------------------------------------------*/
    IDP_DEF(String, "XA_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_xa.log");

    IDP_DEF(UInt, "XA_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "XA_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    // default: 0 = IDE_XA_1(normal print) + IDE_XA_2(xid print)
    IDP_DEF(UInt, "XA_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    /* ------------------------------------------------------------------
     *   MM: BUG-28866
     * --------------------------------------------------------------*/

    /* BUG-45369 */
    IDP_DEF(UInt, "MM_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    IDP_DEF(String, "MM_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_mm.log");

    IDP_DEF(UInt, "MM_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "MM_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "MM_SESSION_LOGGING",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

/* BUG-45274 */
    IDP_DEF(UInt, "LB_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1);

    IDP_DEF(String, "LB_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_lb.log");

    IDP_DEF(UInt, "LB_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "LB_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    /* ------------------------------------------------------------------
     *   CM: BUG-41909 Add dump CM block when a packet error occurs
     * --------------------------------------------------------------*/

    IDP_DEF(UInt, "CM_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 1);

    IDP_DEF(String, "CM_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_cm.log");

    IDP_DEF(UInt, "CM_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "CM_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    /* ------------------------------------------------------------------
     *   DUMP: PRJ-2118
     * --------------------------------------------------------------*/

    IDP_DEF(String, "DUMP_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_dump.log");

    IDP_DEF(UInt, "DUMP_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "DUMP_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    /* ------------------------------------------------------------------
     *   ERROR: PRJ-2118
     * --------------------------------------------------------------*/

    IDP_DEF(String, "ERROR_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_error.log");

    IDP_DEF(UInt, "ERROR_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "ERROR_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    // BUG-29506 TBT가 TBK로 전환시 변경된 offset을 CTS에 반영하지 않습니다.
    // 재현하기 위해 CTS 할당 여부를 임의로 제어하기 위한 PROPERTY를 추가
    // 0 : CTS할당 가능하면 할당(default), 1 : CTS할당 가능하더라도 할당하지 않음
    IDP_DEF(UInt, "__DISABLE_TRANSACTION_BOUND_IN_CTS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
    // 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
    // 512 : (maximum transaction segment count) automatic
    //   1 : 특정 entry id의 segment에 transaction binding
    IDP_DEF(UInt, "__MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 512, 512 );

    //fix BUG-30566
    //shutdown immediate시 기다릴 TIMEOUT
    IDP_DEF(UInt, "SHUTDOWN_IMMEDIATE_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 60);

    // fix BUG-30731
    // V$STATEMENT, V$SQLTEXT, V$PLANTEXT 조회시
    // 한번에 검색할 Statement 수
    IDP_DEF(UInt, "STATEMENT_LIST_PARTIAL_SCAN_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    /* BUG-31144 
     * Limit max number of statements per a session 
     */
    IDP_DEF(UInt, "MAX_STATEMENTS_PER_SESSION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            1, 65536, 1024);

    /*
     * BUG-31040 set 연산으로 인해 초래되는
     *           qmvQuerySet::validate() 함수 recursion depth 를 제한합니다.
     */
    IDP_DEF(SInt, "__MAX_SET_OP_RECURSION_DEPTH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            128, 16384, 1024);

    /*
     * PROJ-2118 Bug Reporting
     *
     * release mode에서만 작동, release 에서 IDE_ERROR를
     *   0. 예외로 처리 할 것인지         ( Release default )
     *   1. Assert로 처리 할 것인지를 결정 ( Debug default )
     */
#if defined(DEBUG)
    IDP_DEF( UInt, "__ERROR_VALIDATION_LEVEL",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );
#else
    IDP_DEF( UInt, "__ERROR_VALIDATION_LEVEL",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );
#endif

    /*
     * PROJ-1718 Subquery Unnesting
     *
     * 0: Disable
     * 1: Enable
     */
    IDP_DEF(UInt, "OPTIMIZER_UNNEST_SUBQUERY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "OPTIMIZER_UNNEST_COMPLEX_SUBQUERY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /*
        PROJ-2492 Dynamic sample selection
        0 : disable
        1 ; 32 page
        2 : 64 page
        3 : 128 page
        4 : 256 page
        5 : 512 page
        6 : 1024 page
        7 : 4096 page
        8 : 16384 page
        9 : 65536 page
        10 : All page
    */
    IDP_DEF(UInt, "OPTIMIZER_AUTO_STATS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 10, 0);

    // BUG-43039 inner join push down
    IDP_DEF(UInt, "__OPTIMIZER_INNER_JOIN_PUSH_DOWN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-43068 Indexable order by 개선
    IDP_DEF(UInt, "__OPTIMIZER_ORDER_PUSH_DOWN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // PROJ-2551 simple query 최적화
    IDP_DEF(UInt, "EXECUTOR_FAST_SIMPLE_QUERY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 2);

    /*
     * BUG-32177  The server might hang when disk is full during checkpoint.
     *
     * checkpoint때문에 디스크공간 부족으로 로그파일을 만들지 못하여
     * hang이 걸리는 현상을 "완화"하기 위한것임.(internal only!)
     */
    IDP_DEF(ULong, "__RESERVED_DISK_SIZE_FOR_LOGFILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0,(1024*1024*1024) /*1G*/, 0);
  
    /*
     * PROJ-2118 Bug Reporting
     *
     * Error Trace 기록으로 인한 성능 하락을 해결하기 위한 Property
     *   0. Error Trace 를 기록하지 않고 예외 처리만 한다.
     *   1. Error Trace 를 기록하고 예외 처리 한다. ( default )
     */
    IDP_DEF( UInt, "__WRITE_ERROR_TRACE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );
    
    /*
     * PROJ-2118 Bug Reporting
     *
     * Pstack 기록으로 인한 성능 하락을 해결하기 위한 Property
     *   0. Pstack 를 기록하지 않고 예외 처리만 한다. ( default )
     *   1. Pstack 를 기록하고 예외 처리 한다.
     */
    IDP_DEF( UInt, "__WRITE_PSTACK",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    /*
     * PROJ-2118 Bug Reporting
     *
     * sigaltstack()을 위한 버퍼 사용시 스레드 별로 메모리가 소요 된다.
     *   0. sigaltstack()을 사용하지 않는다.
     *   1. sigaltstack()을 사용한다. (default)
     */
    IDP_DEF( UInt, "__USE_SIGALTSTACK",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );
    
    /*
     * PROJ-2118 Bug Reporting
     *
     * A parameter of dump_collector.sh
     */
    IDP_DEF( UInt, "__LOGFILE_COLLECT_COUNT_IN_DUMP",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 10 );

    /*
     * PROJ-2118 Bug Reporting
     *
     * Log File, Log Anchor, Trace File등을 수집하여
     * 압축파일로 저장 할지의 여부,
     *
     *   0. Dump Info를 수집하지 않는다. ( debug mode default )
     *   1. Dump Info를 수집한다.        ( release mode default )
     */
#if defined(DEBUG)
    IDP_DEF( UInt, "COLLECT_DUMP_INFO",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );
#else /* release */
    IDP_DEF( UInt, "COLLECT_DUMP_INFO",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );
#endif

    /*
     * PROJ-2118 Bug Reporting
     *
     * Windows Mini Dump 생성 여부 결정 프로퍼티
     *   0. Mini Dump 미생성 ( default )
     *   1. Mini Dump 생성
     */
    IDP_DEF( UInt, "__WRITE_WINDOWS_MINIDUMP",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );

    /* BUG-36203 PSM Optimize */
    IDP_DEF(UInt, "PSM_TEMPLATE_CACHE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 64, 0);

    /*
     * TASK-4949
     * BUG-32751
     * Memory allocator selection
     */
    IDP_DEF(UInt, "MEMORY_ALLOCATOR_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
    IDP_DEF(UInt, "MEMORY_ALLOCATOR_USE_PRIVATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
    IDP_DEF(ULong, "MEMORY_ALLOCATOR_POOLSIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            256*1024, 1024*1024*1024, 16*1024*1024);
    IDP_DEF(ULong, "MEMORY_ALLOCATOR_POOLSIZE_PRIVATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            256*1024, 1024*1024, 256*1024);
    IDP_DEF(SInt, "MEMORY_ALLOCATOR_DEFAULT_SPINLOCK_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            -1, ID_SINT_MAX, 1024);
    IDP_DEF(UInt, "MEMORY_ALLOCATOR_AUTO_SHRINK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
    /*
     * BUG-37722. Limit count of TLSF instances.
     * 0 - Default, follow system setting.
     */
    IDP_DEF(UInt, "MEMORY_ALLOCATOR_MAX_INSTANCES",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 512, 0);
    /*
     * Project 2408 Maximum size of global allocator.
     * 0 - Default, unlimited.
     */
    IDP_DEF(ULong, "MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_vULONG_MAX, 0);
    /*
     * Project 2379
     * Limit the number of resources
     */
    IDP_DEF(UInt, "MAX_THREAD_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 65536, 0);

    IDP_DEF( UInt, "THREAD_REUSE_ENABLE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1);

/* --------------------------------------------------------------------
 * PROJ-2133 Incremental Backup
 * PROJ-2488 Incremental Backup in XDB
 * incremental backup된 backup파일들의 정보를 유지할 기간을 정한다.
 * 값은 일(day)단위이다. 0으로 값이 설정되면 level0 백업이 수생되면 바로
 * 이전에 수행된 모든 backup들은 obsolete한 backupinfo가 된다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "INCREMENTAL_BACKUP_INFO_RETENTION_PERIOD",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

/* --------------------------------------------------------------------
 * PROJ-2133 Incremental Backup
 * PROJ-2488 Incremental Backup in XDB
 * 테스트 목적으로 incremental backup된 backup 파일들의 정보를 유지할 기간을 정한다.
 * 값은 초(second)단위이다. 0으로 값이 설정되면 level0 백업이 수생되면 바로
 * 이전에 수행된 모든 backup들은 obsolete한 backupinfo가 된다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__INCREMENTAL_BACKUP_INFO_RETENTION_PERIOD_FOR_TEST",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

/* --------------------------------------------------------------------
 * PROJ-2133 Incremental Backup
 * PROJ-2488 Incremental Backup in XDB
 * Incremental chunk change traking을 수행할때 몇개의
 * 페이지를 묶어서 추적할지 지정한다. 단위는 page 단위이다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "INCREMENTAL_BACKUP_CHUNK_SIZE",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 4);

/* --------------------------------------------------------------------
 * PROJ-2133 Incremental Backup
 * PROJ-2488 Incremental Backup in XDB
 * Incremental chunk change traking 수행시 변경된 페이지정보를 남길
 * 한 bitamp block의 bitmap 공간의 크기를 정한다.
 * 작을수록 changeTracking파일의 확장이 자주 발생한다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__INCREMENTAL_BACKUP_BMP_BLOCK_BITMAP_SIZE",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 488, 488);

/* --------------------------------------------------------------------
 * PROJ-2133 Incremental Backup
 * PROJ-2488 Incremental Backup in XDB
 * Incremental chunk change traking body에 속한 extent의 갯수를 지정한다.
 * 작을수록 changeTracking파일의 확장이 자주 발생한다.
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__INCREMENTAL_BACKUP_CTBODY_EXTENT_CNT",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32, 320, 320);

/* --------------------------------------------------------------------
 * PROJ-2133 Incremental Backup
 * PROJ-2488 Incremental Backup in XDB
 * ALTER DATABASE CHANGE BACKUP DIRECTORY ‘directory_path’; 구문은
 * 절대경로만 입력받게 되어있다. testcase 작성을 위해 상대경로를
 * 입력가능하게 한다.
 *
 * 0: 절대경로 입력가능
 * 1: 상대경로 입력가능 (smuProperty::getDefaultDiskDBDir() 밑에 경로생성)
 * ----------------------------------------------------------------- */
    IDP_DEF(UInt, "__INCREMENTAL_BACKUP_PATH_MAKE_ABS_PATH",
            IDP_ATTR_SL_PFILE |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
			
    /* PROJ-2207 Password policy support */
    IDP_DEF(String, "__SYSDATE_FOR_NATC",
            IDP_ATTR_SL_ALL       |
            IDP_ATTR_IU_ANY       |
            IDP_ATTR_MS_ANY       |
            IDP_ATTR_SK_ASCII     |
            IDP_ATTR_LC_INTERNAL  |
            IDP_ATTR_RD_WRITABLE  |
            IDP_ATTR_ML_JUSTONE   |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    // FAILED_LOGIN_ATTEMPTS
    IDP_DEF(UInt, "FAILED_LOGIN_ATTEMPTS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1000, 0 );
    
    /* 
     * PROJ-2047 Strengthening LOB - LOBCACHE
     */
    IDP_DEF(UInt, "LOB_CACHE_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 524288, 8192);  /* BUG-46411 */

    // PASSWORD_LOCK_TIME
    IDP_DEF(UInt, "PASSWORD_LOCK_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3650, 0 );

    // PASSWORD_LIFE_TIME
    IDP_DEF(UInt, "PASSWORD_LIFE_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3650, 0 );

    // PASSWORD_GRACE_TIME
    IDP_DEF(UInt, "PASSWORD_GRACE_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3650, 0 );

    // PASSWORD_REUSE_MAX
    IDP_DEF(UInt, "PASSWORD_REUSE_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1000, 0 );

    // PASSWORD_REUSE_TIME
    IDP_DEF(UInt, "PASSWORD_REUSE_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 3650, 0 );

    // PASSWORD_VERIFY_FUNCTION
    IDP_DEF(String, "PASSWORD_VERIFY_FUNCTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 40, (SChar *)"");

     IDP_DEF(UInt, "__SOCK_WRITE_TIMEOUT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 1800);

    /* PROJ-2102 Fast Secondary Buffer */
    // 0 : disable
    // 1 : enable
    IDP_DEF(UInt, "SECONDARY_BUFFER_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
 
    // Min : 1, Max : 16, Default : 2
    IDP_DEF( UInt, "SECONDARY_BUFFER_FLUSHER_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1, 16, 2 );

    // Min : 0, Max : 16, Default : 0
    IDP_DEF( UInt, "__MAX_SECONDARY_CHECKPOINT_FLUSHER_CNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 16, 0 );

    // Min : 0, Max : 2, Default : 2
    // 0 : ALL  1 : Dirty  2 : Clean 
    IDP_DEF( UInt, "SECONDARY_BUFFER_TYPE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 2, 2 );

    // Min : 0, Max : 32G, Default : 0
    IDP_DEF( ULong, "SECONDARY_BUFFER_SIZE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0,
             IDP_DRDB_DATAFILE_MAX_SIZE,
             0);
  
     //  
     IDP_DEF( String, "SECONDARY_BUFFER_FILE_DIRECTORY",
              IDP_ATTR_SL_ALL |
              IDP_ATTR_SH_ALL |
              IDP_ATTR_IU_ANY |
              IDP_ATTR_MS_ANY |
              IDP_ATTR_SK_PATH     |
              IDP_ATTR_LC_EXTERNAL |
              IDP_ATTR_RD_READONLY |
              IDP_ATTR_ML_JUSTONE  |
              IDP_ATTR_CK_CHECK,
              0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* PROJ-2208 */
    IDP_DEF(String, "NLS_TERRITORY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_ASCII    |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"KOREA");

    /* PROJ-2208 */
    IDP_DEF(String, "NLS_ISO_CURRENCY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_ASCII    |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* PROJ-2208 */
    IDP_DEF(String, "NLS_CURRENCY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_MULTI_BYTE |
            IDP_ATTR_LC_EXTERNAL   |
            IDP_ATTR_RD_WRITABLE   |
            IDP_ATTR_ML_JUSTONE    |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* PROJ-2208 */
    IDP_DEF(String, "NLS_NUMERIC_CHARACTERS",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_ASCII    |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* PROJ-2209 DBTIMEZONE */
    IDP_DEF(String, "TIME_ZONE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 40, (SChar *)"OS_TZ");

    /* PROJ-2232 archivelog 다중화 저장 디렉토리 */
    IDP_DEF(String, "ARCHIVE_MULTIPLEX_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_DF_USE_DEFAULT |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* PROJ-2232 archivelog 다중화 수*/
    IDP_DEF(UInt, "ARCHIVE_MULTIPLEX_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 10 , 0);

    /* PROJ-2232 log 다중화 저장 디렉토리 */
    IDP_DEF(String, "LOG_MULTIPLEX_DIR",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_MULTIPLE |
            IDP_ATTR_DF_USE_DEFAULT |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* PROJ-2232 log 다중화 수*/
    IDP_DEF(UInt, "LOG_MULTIPLEX_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 10 , 0);

    /*
     * PROJ_2232 log multiplex
     * log다중화 append Thread의 sleep조건을 지정한다.
     * 값이 작으면 자주 sleep하고 값이 크면 드물게 sleep한다.
     * ID_UINT_MAX값이면 sleep하지 않는다.
     */
    IDP_DEF(UInt, "__LOG_MULTIPLEX_THREAD_SPIN_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 
            ID_UINT_MAX,
            (idlVA::getProcessorCount() > 1) ?
            (idlVA::getProcessorCount() - 1)*100000 : 1);

    /* ==============================================================
     * PROJ-2108 Dedicated thread mode which uses less CPU
     * ============================================================== */

     IDP_DEF(UInt, "THREAD_CPU_AFFINITY",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0);

     IDP_DEF(UInt, "DEDICATED_THREAD_MODE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0);

     IDP_DEF(UInt, "DEDICATED_THREAD_INIT_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1, 65535, 4);

     IDP_DEF(UInt, "DEDICATED_THREAD_MAX_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1, 65535, 1000);
     
     IDP_DEF(UInt, "DEDICATED_THREAD_CHECK_INTERVAL",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_UINT_MAX, 3600);

     /* 
      * BUG-35179 Add property for parallel logical ager 
      * 동작하고있는 logical ager의 수가 많으면 index관련 작업을 하는 service
      * thread들의 성능이 저하된다. 따라서 병렬로 동작하는 logical ager의 수를
      * 조절해야한다.
      * LOGICAL_AGER_COUNT_ 프로퍼티를 이용해 조절가능.
      *
      * main trunk는 parallel logical ager 검증을 위해 기본값을 1로 설정한다.
      * 
      */
#if defined(ALTIBASE_PRODUCT_HDB)
     IDP_DEF(UInt, "__PARALLEL_LOGICAL_AGER",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1);
#else
     IDP_DEF(UInt, "__PARALLEL_LOGICAL_AGER",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0);
#endif
     
     /* PROJ-1753 */
     IDP_DEF(UInt, "__LIKE_OP_USE_OLD_MODULE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

     // bug-35371
     IDP_DEF(UInt, "XA_HASH_SIZE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             10, 4096, 21);

     // bug-35381
     IDP_DEF(UInt, "XID_MEMPOOL_ELEMENT_COUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             4, 4096, 4);

     // bug-35382
     IDP_DEF(UInt, "XID_MUTEX_POOL_SIZE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1, 1024, 10);

    // PROJ-1685
    IDP_DEF(UInt, "EXTPROC_AGENT_CONNECT_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            5, ID_UINT_MAX, 60);

    IDP_DEF(UInt, "EXTPROC_AGENT_IDLE_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            5, ID_UINT_MAX, 300);

    IDP_DEF(UInt, "EXTPROC_AGENT_CALL_RETRY_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 10, 1);

    // BUG-44652 Socket file path of EXTPROC AGENT could be set by property.
    IDP_DEF(String, "EXTPROC_AGENT_SOCKET_FILEPATH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"?"IDL_FILE_SEPARATORS"trc"IDL_FILE_SEPARATORS);

     IDE_TEST( registDatabaseLinkProperties() != IDE_SUCCESS );

    /* BUG-36662 Add property for archive thread to kill server when doesn't
     * exist source logfile
     * 로그파일 arching시 archive thread가 abort되면 archiving할
     * 로그파일이 존재하는지 검사하고 존재하지 않다면 비정상종료 시킨다.
     */
     IDP_DEF( UInt, "__CHECK_LOGFILE_WHEN_ARCH_THR_ABORT",
              IDP_ATTR_SL_ALL |
              IDP_ATTR_SH_ALL |
              IDP_ATTR_IU_ANY |
              IDP_ATTR_MS_ANY |
              IDP_ATTR_LC_INTERNAL |
              IDP_ATTR_RD_READONLY |
              IDP_ATTR_ML_JUSTONE  |
              IDP_ATTR_CK_CHECK,
              0, 1, 0 );

    /* 
     * BUG-35443 Add Property for Excepting SYS_TBS_MEM_DIC size from
     * MEM_MAX_DB_SIZE
     * SYS_TBS_MEM_DIC size를 MEM_MAX_DB_SIZE와는 별도로 분리한다.
     * 따라서 SYS_TBS_MEM_DIC size가 최대 MEM_MAX_DB_SIZE만큼 증가할 수 있게한다.
     */
    IDP_DEF(UInt, "__SEPARATE_DIC_TBS_SIZE_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-36203
    IDP_DEF(UInt, "__QUERY_HASH_STRING_LENGTH_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, ID_UINT_MAX);
    
    // BUG-37247
    IDP_DEF(UInt, "SYS_CONNECT_BY_PATH_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 32000, 4000);
    
    // BUG-37247
    IDP_DEF(UInt, "GROUP_CONCAT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 32000, 4000);

    // BUG-38842
    IDP_DEF(UInt, "CLOB_TO_VARCHAR_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, ID_UINT_MAX);

    // BUG-38952
    IDP_DEF(UInt, "TYPE_NULL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-41194
    IDP_DEF(UInt, "IEEE754_DOUBLE_TO_NUMERIC_FAST_CONVERSION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-41555 DBMS PIPE
    // rw-rw-rw(0666) : 0, rw-r-r(0644) : 1 Default : 0 (0666)
    IDP_DEF(UInt, "MSG_QUEUE_PERMISSION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-37252
    IDP_DEF(UInt, "EXECUTION_PLAN_MEMORY_CHECK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-37302
    IDP_DEF(UInt, "SQL_ERROR_INFO_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, 1024*1024*10, 10240);

    // PROJ-2362 memory temp 저장 효율성 개선
    IDP_DEF(UInt, "REDUCE_TEMP_MEMORY_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* BUG-38254  alter table xxx 에서 hang이 걸릴수 있습니다 */
    IDP_DEF(UInt, "__TABLE_BACKUP_TIMEOUT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDV_MAX_TIME_INTERVAL_SEC, 600 /*sec*/);

    /* BUG-38621  log 기록시 상대경로로 저장 (Disaster Recovery 테스트 용도) */
    IDP_DEF(UInt, "__RELATIVE_PATH_IN_LOG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-38101
    IDP_DEF(UInt, "CASE_SENSITIVE_PASSWORD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* 
     * BUG-38951 Support to choice a type of CM dispatcher on run-time
     *
     * 0 : Reserved (for the future)
     * 1 : Select
     * 2 : Poll     (If supported by OS)
     * 3 : Epoll    (If supported by OS) - BUG-45240
     */
    #define IDP_CM_DISPATCHER_SOCK_POLL_TYPE_MIN       (1)
#if defined(PDL_HAS_EPOLL)
    #define IDP_CM_DISPATCHER_SOCK_POLL_TYPE_MAX       (3)
    #define IDP_CM_DISPATCHER_SOCK_POLL_TYPE_DEFAULT   (3)
#elif defined(PDL_HAS_POLL)
    #define IDP_CM_DISPATCHER_SOCK_POLL_TYPE_MAX       (2)
    #define IDP_CM_DISPATCHER_SOCK_POLL_TYPE_DEFAULT   (1)
#else
    #define IDP_CM_DISPATCHER_SOCK_POLL_TYPE_MAX       (1)
    #define IDP_CM_DISPATCHER_SOCK_POLL_TYPE_DEFAULT   (1)
#endif
    IDP_DEF(UInt, "CM_DISPATCHER_SOCK_POLL_TYPE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_CM_DISPATCHER_SOCK_POLL_TYPE_MIN,
            IDP_CM_DISPATCHER_SOCK_POLL_TYPE_MAX,
            IDP_CM_DISPATCHER_SOCK_POLL_TYPE_DEFAULT);

    // fix BUG-39754
    IDP_DEF(UInt, "__FOREIGN_KEY_LOCK_ROW",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1 );

    // BUG-39679 Ericsson POC bug
    IDP_DEF(UInt, "__ENABLE_ROW_TEMPLATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-40042 oracle outer join property
    IDP_DEF(UInt, "OUTER_JOIN_OPERATOR_TRANSFORM_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* PROJ-2451 Concurrent Execute Package */
    IDP_DEF(UInt, "CONCURRENT_EXEC_DEGREE_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024, IDL_MIN(idlVA::getProcessorCount(), 1024));

    /* PROJ-2451 Concurrent Execute Package */
    IDP_DEF(UInt, "CONCURRENT_EXEC_DEGREE_DEFAULT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            2, 1024, 4);

    /* PROJ-2451 Concurrent Execute Package */
    IDP_DEF(UInt, "CONCURRENT_EXEC_WAIT_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10, 1000000, 100);

    /* BUG-40138 
     * Flusher에의한 checkpoint flush는 replacement flush나
     * checkpoint thread에의한 checkpoint flush 보다 우선 운위가 낮아야 한다.
     * 따라서, flusher에의한 checkpoint flush 수행 도중 다른 flush 작업에 대한
     * 요청/조건이 있는지 확인하도록 한다.
     * 예) 프로퍼티의 값이 10이면, flusher에 의한 checkpoint flush에 의해
           10개의 페이지가 flush될때마다 조건 체크를 한다. */
    IDP_DEF(UInt, "__FLUSHER_BUSY_CONDITION_CHECK_INTERVAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, ID_UINT_MAX, 64);

    /* BUG-41168 SSL extension */
    IDP_DEF(UInt, "TCP_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* PROJ-2474 SSL/TLS */
    IDP_DEF(UInt, "SSL_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "SSL_CLIENT_AUTHENTICATION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "__SSL_VERIFY_PEER_CERTIFICATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "SSL_PORT_NO",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024,   /* min */
            65535,  /* max */
            20443); /* default */

    IDP_DEF(UInt, "SSL_MAX_LISTEN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 16384, 128);

    IDP_DEF(String, "SSL_CIPHER_LIST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE|
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(String, "SSL_CA",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE|
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(String, "SSL_CAPATH",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE|
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(String, "SSL_CERT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE|
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(String, "SSL_KEY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE|
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    IDP_DEF(UInt, "DCI_RETRY_WAIT_TIME",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, (60000000) /* sec */ , (1000));

    /* PROJ-2441 flashback  size byte */
    IDP_DEF(ULong, "RECYCLEBIN_MEM_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, ID_ULONG( 4 * 1024 * 1024 * 1024 ) );
    
    IDP_DEF(ULong, "RECYCLEBIN_DISK_MAX_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, ID_ULONG_MAX);
    
    IDP_DEF(UInt, "RECYCLEBIN_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
    
    IDP_DEF(UInt, "__RECYCLEBIN_FOR_NATC",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 9, 0);

    /* BUG-40790 */
    IDP_DEF(UInt, "__LOB_CURSOR_HASH_BUCKET_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65535, 5);

    /* BUG-42639 Monitoring query */
    IDP_DEF(UInt, "OPTIMIZER_PERFORMANCE_VIEW",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-48160 fixed table에서 table lock 걸린 table을 skip하고 출력 
     * 0 : lock wait (default)
     * 1 : skip lock table
     * 2 : small info for lock table (not support, next job)*/
    IDP_DEF(UInt, "__SKIP_LOCKED_TABLE_AT_FIXED_TABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /************************************************/
    /* PROJ-2553 Cache-aware Memory Hash Temp Table */
    /************************************************/

    // 0 : Array-Partitioned Memory Hash Temp Table를 사용
    // 1 : Bucket-based Memory Hash Temp Table을 사용
    IDP_DEF(UInt, "HASH_JOIN_MEM_TEMP_PARTITIONING_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // 0 : 실제 삽입된 Record 개수에 맞춰 Bucket 개수 (=Partition 개수) 계산
    // 1 : Bucket 개수를 계산하지 않고, 다음의 값을 그대로 사용
    //     - /*+HASH BUCKET COUNT*/ 힌트로 설정된 Bucket 개수
    //     - (힌트가 없는 경우) QP Optimizer가 예측한 Bucket 개수 
    IDP_DEF(UInt, "HASH_JOIN_MEM_TEMP_AUTO_BUCKET_COUNT_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // Partition 개수가 TLB에 한 번에 들어갈 수 없는 경우, 두 번에 걸쳐 Fanout 해야 한다.
    // TLB Entry가 클 수록, 두 번에 걸쳐 Fanout 하는 경우가 줄어든다.
    // 구동 환경의 TLB Entry 개수를 입력하면, 최적의 방법으로 Fanout 하게 된다.
    IDP_DEF(UInt, "__HASH_JOIN_MEM_TEMP_TLB_ENTRY_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 64);

    // 0 : Single/Double Fanout을 TLB_ENTRY_COUNT에 맞춰 결정한다.
    // 1 : Single Fanout만 선택한다.
    // 2 : Double Fanout만 선택한다.
    IDP_DEF(UInt, "__FORCE_HASH_JOIN_MEM_TEMP_FANOUT_MODE",
            IDP_ATTR_SL_ALL | IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 0);

    /* PROJ-2554 */
    IDP_DEF( UInt, "ALLOC_SLOT_IN_CURRENT_PAGE",
             IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    // 0 return err
    // 1 skip free slot (default)
    // 2 force free slot
    IDP_DEF(UInt, "__REFINE_INVALID_SLOT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1);

    // TASK-6445 use old sort (for Memory Sort Temp Table)
    // 0 : 현재 정렬 알고리즘 (Timsort)
    // 1 : 기존 정렬 알고리즘 (Quicksort)
    IDP_DEF(UInt, "__USE_OLD_SORT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // BUG-43258
    IDP_DEF(UInt, "__OPTIMIZER_INDEX_CLUSTERING_FACTOR_ADJ",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 10000, 100);

    // BUG-41248 DBMS_SQL package
    IDP_DEF(UInt, "PSM_CURSOR_OPEN_LIMIT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 1024, 32);

    /*
     * Project 2595
     * Miscellanous trace log
     */
    IDP_DEF(String, "MISC_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_misc.log");

    IDP_DEF(UInt, "MISC_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "MISC_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

    IDP_DEF(UInt, "MISC_MSGLOG_FLAG",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 2);

    IDP_DEF(UInt, "TRC_ACCESS_PERMISSION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 777, 644); /*BUG-45405*/

/* PROJ-2581 */
#if defined(ALTIBASE_FIT_CHECK)

    IDP_DEF(String, "FIT_MSGLOG_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)PRODUCT_PREFIX"altibase_fit.log");

    IDP_DEF(UInt, "FIT_MSGLOG_SIZE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, (10 * 1024 * 1024));

    IDP_DEF(UInt, "FIT_MSGLOG_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 10);

#endif

#if defined(DEBUG)
    /*
     * BUG-42770
     * 1 - Enable CORE Dump on debug mode
     * 0 - Disable CORE Dump on debug mode
     * This property is only valid in debug mode
     */
    IDP_DEF(UInt, "__CORE_DUMP_ON_SIGNAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
#else
    IDP_DEF(UInt, "__CORE_DUMP_ON_SIGNAL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
#endif

    /* PROJ-2465 Tablespace Alteration for Table */
    IDP_DEF( UInt, "DDL_MEM_USAGE_THRESHOLD",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             50, 100, 100 );

    /* PROJ-2465 Tablespace Alteration for Table */
    IDP_DEF( UInt, "DDL_TBS_USAGE_THRESHOLD",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             50, 100, 100);

    /* PROJ-2465 Tablespace Alteration for Table */
    IDP_DEF( UInt, "ANALYZE_USAGE_MIN_ROWCOUNT",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             1000, ID_UINT_MAX, 1000 );
    
    /* PROJ-2613: Key Redistribution in MRDB Index
     * 이 값이 1일 경우에는 MRDB Index에서 Index의 설정에 따라 키 재분배 기능을 사용한다.
     * 이 값이 0일 경우에는 MRDB Index에서 Index의 설정과 관계 없이 키 재분배 기능을 사용하지 않는다. */
    IDP_DEF(SInt, "MEM_INDEX_KEY_REDISTRIBUTION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* PROJ-2613: Key Redistribution in MRDB Index
     * 키 재분배가 수행되기 위한 이웃 노드의 최소 빈공간 비율
     * 예)이 프로퍼티가 30일 경우 이웃 노드가 30%이상의 빈 공간이 있을 경우에만 키 재분배를 수행한다. */

    IDP_DEF(SInt, "MEM_INDEX_KEY_REDISTRIBUTION_STANDARD_RATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            10, 90, 50);

    /* PROJ-2586 PSM Parameters and return without precision */
    /*
     * PSM의 parameter와 return의 precision을 PROJ-2586 이전 방식으로 사용 지원
     * 0 : PROJ-2586 이전 방식
     * 1 : PROJ-2586이 적용된 방식( default )
     * 2 : 테스트용, PROJ-2586 + precision을 명시하여도 무시하고 default precision 적용하여 실행
     */
    IDP_DEF(UInt, "PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1);

    /*
     * PSM에서 char type parameter와 return의 최대 precision를 결정한다.
     * BUG-46573 PSM default precision을 조정합니다.
     * default : 32000
     * maximum : 65534 
     * minimum : 1
     */
    IDP_DEF(UInt, "PSM_CHAR_DEFAULT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65534, 32000);

    /*
     * PSM에서 varchar type parameter와 return의 최대 precision를 결정한다.
     * BUG-46573 PSM default precision을 조정합니다.
     * default : 32000
     * maximum : 65534
     * minimum : 1
     */
    IDP_DEF(UInt, "PSM_VARCHAR_DEFAULT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 65534, 32000);

    /*
     * PSM에서 nchar(UTF16) type parameter와 return의 최대 precision를 결정한다.
     * BUG-46573 PSM default precision을 조정합니다.
     * default : 16000
     *           = ( 32766[ manual에 표기된 nchar 최대 precision ] *
     *               32767 [ char type의 parameter 및 return의 default precision ] ) /
     *             65534 [ manual에 표기된 char 최대 precision ]
     * maximum : 32766
     * minimum : 1
     */
    IDP_DEF(UInt, "PSM_NCHAR_UTF16_DEFAULT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 32766, 16000);

    /*
     * PSM에서 nvarchar(UTF16) type parameter와 return의 최대 precision를 결정한다.
     * BUG-46573 PSM default precision을 조정합니다.
     * default : 16000
     *           = ( 32766[ manual에 표기된 nchar 최대 precision ] *
     *               32767 [ char type의 parameter 및 return의 default precision ] ) /
     *             65534 [ manual에 표기된 char 최대 precision ]
     * maximum : 32766
     * minimum : 1
     */
    IDP_DEF(UInt, "PSM_NVARCHAR_UTF16_DEFAULT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 32766, 16000);

    /*
     * PSM에서 nchar(UTF8) type parameter와 return의 최대 precision를 결정한다.
     * BUG-46573 PSM default precision을 조정합니다.
     * default : 10666
     *           = ( 21843[ manual에 표기된 nchar 최대 precision ] *
     *               32767 [ char type의 parameter 및 return의 default precision ] ) /
     *             65534 [ manual에 표기된 char 최대 precision ]
     * maximum : 21843 
     * minimum : 1
     */
    IDP_DEF(UInt, "PSM_NCHAR_UTF8_DEFAULT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 21843, 10666);

    /*
     * PSM에서 nvarchar(UTF8) type parameter와 return의 최대 precision를 결정한다.
     * BUG-46573 PSM default precision을 조정합니다.
     * default : 10666
     *           = ( 21843[ manual에 표기된 nchar 최대 precision ] *
     *               32767 [ char type의 parameter 및 return의 default precision ] ) /
     *             65534 [ manual에 표기된 char 최대 precision ]
     * maximum : 21843 
     * minimum : 1
     */
    IDP_DEF(UInt, "PSM_NVARCHAR_UTF8_DEFAULT_PRECISION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 21843, 10666);

    // BUG-42322
    IDP_DEF(ULong, "__PSM_FORMAT_CALL_STACK_OID",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 9999, 0);
    
    /* PROJ-2617 */
    IDP_DEF(UInt, "__FAULT_TOLERANCE_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    IDP_DEF(UInt, "__FAULT_TOLERANCE_TRACE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    /* BUG-34112 */
    IDP_DEF(UInt, "__FORCE_AUTONOMOUS_TRANSACTION_PRAGMA",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "__AUTONOMOUS_TRANSACTION_PRAGMA_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    IDP_DEF(UInt, "__PSM_STATEMENT_LIST_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 4096, 128);

    IDP_DEF(UInt, "__PSM_STATEMENT_POOL_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            32, 4096, 128);

    // BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
    IDP_DEF(UInt, "__DISK_TEMP_SIZE_ESTIMATE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    // BUG-43421
    IDP_DEF(UInt, "__OPTIMIZER_SEMI_JOIN_TRANSITIVITY_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* BUG-43495 */
    IDP_DEF(UInt, "__OPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    // lock node cache enable
    // 0 : disable
    // 1 : list method with count
    // 2 : bitmap method with fixed count - 64
    IDP_DEF(UInt, "LOCK_MGR_CACHE_NODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 1);
    /* BUG-43408 LockNodeCache Count  */
    IDP_DEF(UInt, "LOCK_NODE_CACHE_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024, 2);
    /* PROJ-2734 */
    IDP_DEF(UInt, "DISTRIBUTION_DEADLOCK_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
    IDP_DEF(ULong, "DISTRIBUTION_DEADLOCK_RISK_LOW_WAIT_TIME", /* microseconds */
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 1000000);
    IDP_DEF(ULong, "DISTRIBUTION_DEADLOCK_RISK_MID_WAIT_TIME", /* microseconds */
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 1000000);
    IDP_DEF(ULong, "DISTRIBUTION_DEADLOCK_RISK_HIGH_WAIT_TIME", /* microseconds */
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_ULONG_MAX, 0);
    /* PROJ-2733 분산 트랜잭션 정합성 */
    IDP_DEF(ULong, "VERSIONING_MIN_TIME", /* milliseconds */
            IDP_ATTR_SL_ALL  |
            IDP_ATTR_SH_USER | 
            IDP_ATTR_IU_ANY  |
            IDP_ATTR_MS_ANY  |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100000, 0);

    /* 0     : 무한대기
       INDOUBT_FETCH_TIMEOUT 을 무시하려면 ?
          => GLOBAL_TRANSACTION_LEVEL을 3이 아닌 값으로 설정한다. */
    IDP_DEF(UInt, "INDOUBT_FETCH_TIMEOUT", /* seconds */
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL | 
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_MIN, 
            IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_MAX, 
            IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_DEFAULT );

    /* INDOUBT_FETCH_TIMEOUT 이 발생한 이후의 동작 
        0 : skip  
        1 : 예외발생                    */
    IDP_DEF(UInt, "INDOUBT_FETCH_METHOD", 
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL | 
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_MIN, 
            IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_MAX, 
            IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_DEFAULT );

    /* BUG-43737
     * 1 : SYS_TBS_MEM_DATA
     * 2 : SYS_TBS_DISK_DATA */
    IDP_DEF(UInt, "__FORCE_TABLESPACE_DEFAULT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1, 2, 1);

    /* PROJ-2626 Snapshot Export */
    IDP_DEF(UInt, "SNAPSHOT_MEM_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100, 80 );

    /* PROJ-2626 Snapshot Export */
    IDP_DEF(UInt, "SNAPSHOT_DISK_UNDO_THRESHOLD",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 100, 80 );

    /* PROJ-2624 ACCESS_LIST */
    IDP_DEF(String, "ACCESS_LIST_FILE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_SK_PATH     |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, IDP_MAX_PROP_STRING_LEN, (SChar *)"");

    /* TASK-6744 */
    IDP_DEF(UInt, "__PLAN_RANDOM_SEED",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, ID_UINT_MAX, 0);

    /* BUG-44499 */
    IDP_DEF(UInt, "__OPTIMIZER_BUCKET_COUNT_MIN",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, 10240000, 1024);

    /* BUG-48161 */
    IDP_DEF(UInt, "__OPTIMIZER_BUCKET_COUNT_MAX",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024, 102400000, 102400000);

    /* BUG-43463 scanlist 의 lock을 잡는지 유무,
     * Memory FullScan 시에 moveNext,Prev에서 lock을 잡지 않는다.
     * link, unlink는 상관없음(Lock을 잡음)*/
    IDP_DEF(UInt, "__SCANLIST_MOVE_NONBLOCK",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);

    /* PROJ-2641 Hierarchy Query Index */
    IDP_DEF(UInt, "__OPTIMIZER_HIERARCHY_TRANSFORMATION",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    // BUG-44692
    IDP_DEF(UInt, "BUG_44692",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    /* BUG-44794 인덱스 빌드시 인덱스 통계 정보를 수집하지 않는 히든 프로퍼티 추가 */
    IDP_DEF(UInt, "__GATHER_INDEX_STAT_ON_DDL",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 /* 651: 1(AS-IS), trunk: 0 */ );

    // BUG-44795
    IDP_DEF(UInt, "__OPTIMIZER_DBMS_STAT_POLICY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택.
         0 : primary key 우선적으로 선택 (BUG-44850) +
             Inverse index NL 조인일 때 index cost가 작은 인덱스 선택 (BUG-45169)
         1 : 기존 플랜 방식과 동일
         2 : primary key 우선적으로 선택 (BUG-44850) +
             Inverse index NL 조인일 때 index cost가 작은 인덱스 선택 (BUG-45169) +
             index NL 조인 일때 index cost가 작은 인덱스 선택 + ( BUG-48327 )
             Anti 조인 일때 index cost가 작은 인덱스 선택
    */
    IDP_DEF(UInt, "__OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2, 2 );

    /* BUG-45172 
         0 : 동작하지 않음 ( 기존과 동일 )
         1 : 동작함 */
    IDP_DEF(UInt, "__OPTIMIZER_SEMI_JOIN_REMOVE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    // BUG-46154
    IDP_DEF(UInt, "__PRINT_OUT_ENABLE",
            IDP_ATTR_SL_ALL | 
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1);
    
    /* PROJ-2681 */
#if defined(ALTI_CFG_OS_LINUX)
    IDP_DEF(UInt, "IB_ENABLE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);
#else /* ALTI_CFG_OS_LINUX */
    IDP_DEF(UInt, "IB_ENABLE",
            IDP_ATTR_SL_ALL | 
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 0, 0);
#endif /* ALTI_CFG_OS_LINUX */

    IDP_DEF(UInt, "IB_PORT_NO",
            IDP_ATTR_SL_ALL | 
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            1024,   /* min */
            65535,  /* max */
            20300); /* default */

    IDP_DEF(UInt, "IB_MAX_LISTEN",
            IDP_ATTR_SL_ALL | 
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1024, 128);

    IDP_DEF(UInt, "IB_LISTENER_DISABLE",
            IDP_ATTR_SL_ALL | 
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "IB_LATENCY",
            IDP_ATTR_SL_ALL | 
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0);

    IDP_DEF(UInt, "IB_CONCHKSPIN",
            IDP_ATTR_SL_ALL | 
IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 2147483, 0);

    /* BUG-46267 */
    IDP_DEF( UInt, "NUMBER_CONVERSION_MODE",
             IDP_ATTR_SL_ALL | 
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_READONLY |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );

    /* Subquery Unnest에 관한 하위버전 호환 프로퍼티
        0   : 새로 추가 또는 변경된 SU에 관한 기능이 모두 수행된다.
              OPTIMIZER_UNNEST_SUBQUERY = 0 일 때 UNNEST hint 우선(BUG-46544)
               + 부모블록에 left outer join이 있고 
                 서브쿼리에 테이블이 1개만 사용된 경우 no unnest (BUG-48336)
        1   : BUG-46544 적용 전과 동일동작
              UNNEST hint보다 "OPTIMIZER_UNNEST_SUBQUERY" Property 우선 적용
        2   : BUG-48336 적용 전과 같이 unnest
        
       ex) 3 = 1+2 ( BUG-46544, BUG-48336 원복 )
    */
    IDP_DEF( UInt, "__OPTIMIZER_UNNEST_COMPATIBILITY",
             IDP_ATTR_SL_ALL | 
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 3, 2 );

    /* PROJ-2632 */
    IDP_DEF( UInt, "SERIAL_EXECUTE_MODE",
             IDP_ATTR_SL_ALL | 
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );

    /* PROJ-2632 */
    IDP_DEF( UInt, "TRCLOG_DETAIL_INFORMATION",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 0 );

    IDP_DEF( ULong, "MATHEMATICS_TEMP_MEMORY_MAXIMUM",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, ID_ULONG_MAX, 0 );

    /* BUG-46932 */
    IDP_DEF( UInt, "__OPTIMIZER_INVERSE_JOIN_ENABLE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );

    /* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
    IDP_DEF( UInt, "__REDUCE_PARTITION_PREPARE_MEMORY",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 3, 1 );

    IDP_DEF(UInt, "SHARED_TRANS_HASH_BUCKET_COUNT",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_USER|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_PROPERTY_SHARE_TRANS_HASH_BUCKET_COUNT_MIN,
            IDP_SHARD_PROPERTY_SHARE_TRANS_HASH_BUCKET_COUNT_MAX,
            IDP_SHARD_PROPERTY_SHARE_TRANS_HASH_BUCKET_COUNT_DEFAULT);

    /* TASK-7219 */
    IDP_DEF( UInt, "SHARD_TRANSFORM_MODE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_EXTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 15, 15 );

    /* BUG-47986
     * Enable : 1
     * Disable : 0
     */
    IDP_DEF( UInt, "__OPTIMIZER_OR_VALUE_INDEX",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1 );

    IDP_DEF(UInt, "SHARD_STATEMENT_RETRY",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL|
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_EXTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_MIN,
            IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_MAX,
            IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_DEFAULT);
    
    IDP_DEF(UInt, "__SHARD_ZOOKEEPER_TEST",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    /* BUG-48132
     * grouping 플랜에서 SORT 또는 HASH 메소드 선택하는 프로퍼티
     *   GROUP_HASH  : 1
     *   GROUP_SORT  : 2
     *   default     : 0  GROUP_HASH + GROUP_SORT 모두 사용,
     *                    기존과 동일(optimizer에 의해 결정) */
    IDP_DEF( UInt, "__OPTIMIZER_PLAN_HASH_OR_SORT_METHOD",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 2, 0);

    /* BUG-48135 NL Join Penalty 값을 조절할수 있는 property 추가 */
    IDP_DEF( UInt, "__OPTIMIZER_INDEX_NL_JOIN_PENALTY",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 20, 6);

    // BUG-48120 
    // scan cost 결정할 때 사용 모드
    // 0 : 버그 수정 전과 동일
    // 1 : BUG-48120 적용 ( default )
    //      ALL EQUAL INDEX인 경우 cost 계산에 predicate NDV를 사용
    //      selection graph의 output record count를 계산할 때는 index NDV 를 사용
    //      ACCESS METHOD를 선택할 때
    //      cost가 동일한 인덱스가 있는 경우 ALL EQUAL INDEX를 선택하도록 한다.
    IDP_DEF( UInt, "__OPTIMIZER_INDEX_COST_MODE",
             IDP_ATTR_SL_ALL |
             IDP_ATTR_SH_ALL |
             IDP_ATTR_IU_ANY |
             IDP_ATTR_MS_ANY |
             IDP_ATTR_LC_INTERNAL |
             IDP_ATTR_RD_WRITABLE |
             IDP_ATTR_ML_JUSTONE  |
             IDP_ATTR_CK_CHECK,
             0, 1, 1);

    /* BUG-48594 */
    IDP_DEF(UInt, "__SQL_PLAN_CACHE_VALID_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_READONLY |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 0 );

    /* BUG-48776 */
    IDP_DEF(UInt, "__SUBQUERY_MODE",
            IDP_ATTR_SL_ALL |
            IDP_ATTR_SH_ALL |
            IDP_ATTR_IU_ANY |
            IDP_ATTR_MS_ANY |
            IDP_ATTR_LC_INTERNAL |
            IDP_ATTR_RD_WRITABLE |
            IDP_ATTR_ML_JUSTONE  |
            IDP_ATTR_CK_CHECK,
            0, 1, 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

