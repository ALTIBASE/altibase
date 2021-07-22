/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idp.h 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#ifndef _O_IDP_H_
# define _O_IDP_H_ 1

# include <idl.h>
# include <iduList.h>
# include <idpBase.h>

class ideLogEntry;
class iduFixedTableMemory;
struct idvSQL;

/*
 * 속성 Type Descriptor 등록 Phase :  each Instance : regist()
 *
 * 프로퍼티 conf 정보 등록 :  build()->insert() : string type의 데이타임
 *
 * 프로퍼티 읽기/변경 : read(), update()
 */

#define IDP_MAX_PROP_COUNT      (1024)
#define IDP_MAX_PROP_LINE_SIZE  (1024)
#define IDP_MAX_PROP_STRING_LEN (1024)
#define IDP_MAX_PROP_DBNAME_LEN (128 - 1) // (SM_MAX_DB_NAME - 1)과 같아야 함

// 최대 트랜잭션 갯수
#define IDP_MAX_TRANSACTION_COUNT (16384) // 2^14

#define IDP_PROPERTY_PREFIX      ALTIBASE_ENV_PREFIX /* from environment */
#define IDP_HOME_ENV             (SChar *)ALTIBASE_ENV_PREFIX"HOME"
#define IDP_SYSUSER_NAME         (SChar *)"SYS"
#define IDP_SYSPASSWORD_FILE     (SChar *)"conf"IDL_FILE_SEPARATORS"syspassword"
#define IDP_DEFAULT_CONFFILE     (SChar *)"conf"IDL_FILE_SEPARATORS PRODUCT_PREFIX"altibase.properties"
/* BUG-45135 */
#define IDP_LOCKFILE             (SChar *)"conf"IDL_FILE_SEPARATORS".forLock"
#define IDP_ERROR_BUF_SIZE   (1024)
#define IDP_MAX_SID_COUNT    (100)    /*System Identifier Max (= max instance count)*/

#define IDP_MAX_VERIFY_INDEX_COUNT   (16)

#define IDP_REPLICATION_MAX_EAGER_PARALLEL_FACTOR (512)

#define IDP_ATTR_SHARD_ALL   (0)
#define IDP_ATTR_SHARD_USER  (1)
#define IDP_ATTR_SHARD_COORD (2)
#define IDP_ATTR_SHARD_LIB   (3)

typedef enum
{
    IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT_DEFAULT = 1,    /* = ULN_CONN_RETRY_COUNT_DEFAULT */
    IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT_MIN     = 0,    /* = ULN_CONN_RETRY_COUNT_MIN */
    IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT_MAX     = 1024, /* = ULN_CONN_RETRY_COUNT_MAX */

    IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY_DEFAULT = 1,    /* = ULN_CONN_RETRY_DELAY_DEFAULT */
    IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY_MIN     = 0,    /* = ULN_CONN_RETRY_DELAY_MIN */
    IDP_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY_MAX     = 3600, /* = ULN_CONN_RETRY_DELAY_MAX */

    IDP_SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT_DEFAULT = 0,            /* = ULN_CONN_TMOUT_DEFAULT */
    IDP_SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT_MIN     = 0,            /* = ULN_TMOUT_MIN */
    IDP_SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT_MAX     = ID_UINT_MAX,  /* = ULN_TMOUT_MAX */

    IDP_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT_DEFAULT      = 60,           /* = ULN_LOGIN_TMOUT_DEFAULT */
    IDP_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT_MIN          = 0,            /* = ULN_TMOUT_MIN */
    IDP_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT_MAX          = ID_UINT_MAX,  /* = ULN_TMOUT_MAX */
} idpShardInternalConnAttrConst;

typedef enum
{
    IDP_SHARD_PROPERTY_SHARE_TRANS_HASH_BUCKET_COUNT_DEFAULT    = 1024,     /* = Default value of TRANSACTION_TABLE_SIZE */
    IDP_SHARD_PROPERTY_SHARE_TRANS_HASH_BUCKET_COUNT_MIN        = 16,       /* = IDP_MIN_TRANSACTION_COUNT */
    IDP_SHARD_PROPERTY_SHARE_TRANS_HASH_BUCKET_COUNT_MAX        = 16384,    /* = IDP_MAX_TRANSACTION_COUNT */

    IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_DEFAULT            = 1,        /* = ULN_SHARD_STATEMENT_RETRY_DEFAULT */
    IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_MIN                = 0,        /* = ULN_SHARD_STATEMENT_RETRY_MIN */
    IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_MAX                = 65535,    /* = ULN_SHARD_STATEMENT_RETRY_MAX */

    IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_DEFAULT            = 10,       /* FETCH_TIMEOUT 보다 작은 어떤값 */
    IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_MIN                = 0,        /* 무한대기 */
    IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_MAX                = 86400,    /* 60*60*24 = 1일 */

    IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_DEFAULT             = 1,
    IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_MIN                 = 0,        /* skip */ 
    IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_MAX                 = 1,        /* 예외발생 */
} idpShardPropertyConst;

class idp
{
    static SChar                mErrorBuf[IDP_ERROR_BUF_SIZE];
    static SChar               *mHomeDir;
    static SChar               *mConfName;
    static SChar               *mSID;
    static PDL_thread_mutex_t   mMutex;
    static UInt                 mCount; // 등록 모듈의 최대 갯수
    static iduList              mArrBaseList[IDP_MAX_PROP_COUNT]; // Property 리스트의 배열 : (id/mt/qp/mm)

    static IDE_RC insertBySrc(const SChar     *aName,
                              SChar           *aValue,
                              idpValueSource   aSrc,
                              idBool          *aFindFlag);

    static IDE_RC insertAll(iduList        *aBaseList, 
                            SChar          *aValue, 
                            idpValueSource  aSrc);
    // Parsing Functions
    static IDE_RC parseBuffer(SChar *aLineBuf, SChar **aName, SChar **aValue);
    static IDE_RC parseSID(SChar  *aBuf,
                           SChar **aSID,
                           SChar **aName);
    static IDE_RC parseSPFileLine(SChar  *aLineBuf,
                                  SChar **aSID,
                                  SChar **aName,
                                  SChar **aValue);
    static void   eraseCharacter(SChar *aBuf);
    
    static IDE_RC readPFile(); /* parse & build prop. info        */
    static IDE_RC readSPFile();
    
    static IDE_RC verifyInsertedValues();
    static IDE_RC verifyMemoryValue4Normal();
    static IDE_RC verifyMemoryValue4Cluster();
    static IDE_RC verifyMemoryValues();
    static IDE_RC verifyUniqueAttr(iduList* aBaseList);
    static IDE_RC verifyIdenticalAttr(iduList* aBaseList);
    
    static idpBase *findBase(const SChar *aName);
    static iduList *findBaseList(const SChar *aName);    
    static idpBase* findBaseBySID(iduList* aBaseList, const SChar* aSID);
    static IDE_RC   setLocalSID();
    static IDE_RC   insertMemoryValueByPriority();
    
public:
    static IDE_RC initialize(SChar *aHomeDir = NULL,
                             SChar *aConfName = NULL);
    static IDE_RC destroy();

    static IDE_RC regist(idpBase *aBaseType); /* regist Descriptor of Each Prop. */

    static void   eraseWhiteSpace(SChar *aLineBuf);

    static IDE_RC read (const SChar *aName, void *aOutParam, UInt aNum = 0);

    static IDE_RC readFromEnv (const SChar *aName, void *aOutParam, UInt aNum = 0);
    
    static IDE_RC readPtr (const SChar *aName,
                           void       **aOutParam,
                           UInt         aNum,
                           idBool      *aIsFound);

    static IDE_RC readPtr (const SChar *aName,
                           void **aOutParam,
                           UInt aNum = 0);

    static void readClonedPtrBySrc(const SChar   *aName,
                                   UInt           aNum, 
                                   idpValueSource aSrc,
                                   void         **aOutParam,
                                   idBool        *aIsFound);

    static void readPtrBySrc(const SChar   *aName,
                             UInt           aNum,
                             idpValueSource aSrc,
                             void         **aOutParam,
                             idBool        *aIsFound);

    static IDE_RC readBySID(const SChar *aSID,
                            const SChar *aName,
                            UInt         aNum,
                            void        *aOutParam);

    static IDE_RC readPtrBySID(const SChar *aSID, 
                               const SChar *aName, 
                               UInt         aNum,                               
                               void       **aOutParam);

    static IDE_RC readPtrBySID(const SChar *aSID,
                               const SChar *aName,
                               UInt         aNum,
                               void       **aOutParam,
                               idBool      *aIsFound);

    static IDE_RC getStoredCountBySID(const SChar *aSID,
                                      const SChar *aName,
                                      UInt        *aPropertyCount);
    
    static void getAllSIDByName(const SChar *aName, 
                                SChar      **aSIDsArray, 
                                UInt        *aCount);

    // BUG-43533 OPTIMIZER_FEATURE_ENABLE
    static IDE_RC update4Startup(idvSQL *aStatistics,
                                 const SChar *aName,
                                 SChar *aInParam,
                                 UInt aNum = 0,
                                 void *aArg = NULL);

    static IDE_RC update(idvSQL *aStatistics,
                         const SChar *aName,
                         SChar *aInParam,
                         UInt aNum = 0,
                         void *aArg = NULL);

    static IDE_RC update(idvSQL *aStatistics,
                         const SChar *aName,
                         UInt  aInParam,
                         UInt aNum = 0,
                         void *aArg = NULL);

    static IDE_RC update(idvSQL *aStatistics,
                         const SChar *aName,
                         ULong  aInParam,
                         UInt aNum = 0,
                         void *aArg = NULL);

    static IDE_RC updateForce(const SChar *aName,
                              SChar *aInParam,
                              UInt aNum = 0,
                              void *aArg = NULL);

    static IDE_RC updateForce(const SChar *aName,
                              UInt  aInParam,
                              UInt aNum = 0,
                              void *aArg = NULL);

    static IDE_RC updateForce(const SChar *aName,
                              ULong  aInParam,
                              UInt aNum = 0,
                              void *aArg = NULL);

    static IDE_RC setupBeforeUpdateCallback(const SChar *aName,
                                            idpChangeCallback mCallback);
    static IDE_RC setupAfterUpdateCallback(const SChar *aName,
                                           idpChangeCallback mCallback);

    static SChar *getErrorBuf() { return mErrorBuf; }

    static SChar *getHomeDir()  { return mHomeDir;  }

    static void  dump(FILE *aFP);

    static IDE_RC buildRecordCallback(idvSQL              * /* aStatistics */,
                                      void                *aHeader,
                                      void                *aDumpObj,
                                      iduFixedTableMemory *aMemory);

    static IDE_RC getMemValueCount (const SChar *aName,
                                    UInt        *aPropertyCount);

    static IDE_RC validate( const SChar * aName,
                            SChar       * aInParam,
                            idBool        aIsSytem );

    // PROJ-2727
    static IDE_RC getPropertyAttribute( const SChar * aName,
                                        UInt        * aOutParam );

    static void dumpProperty( ideLogEntry &aLog );
};

// for compatible

#define __SHOW_ERROR_STACK   (qcuProperty::__show_error_stack_)


#endif /* _O_IDP_H_ */
