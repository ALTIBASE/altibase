/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/*******************************************************************************
 * $Id: smiTableCursor.cpp 90083 2021-02-26 00:58:48Z et16 $
 ******************************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>

#include <smErrorCode.h>

#include <smDef.h>
#include <sdp.h>
#include <smc.h>
#include <sdc.h>
#include <sml.h>
#include <sct.h>
#include <smm.h>
#include <smn.h>
#include <smnbModule.h>
#include <sdn.h>
#include <sdnnDef.h>
#include <sm2x.h>
#include <sma.h>
#include <smi.h>
#include <smu.h>
#include <svcRecord.h>
#include <svnModules.h>
#include <sgmManager.h>
#include <smr.h>


extern smiGlobalCallBackList gSmiGlobalCallBackList;

extern "C" SInt gCompareSmiUpdateColumnListByColId( const void *aLhs,
                                                    const void *aRhs )
{
    const smiUpdateColumnList * sLhs = (const smiUpdateColumnList *)aLhs;
    const smiUpdateColumnList * sRhs = (const smiUpdateColumnList *)aRhs;

    IDE_DASSERT(aLhs != NULL);
    IDE_DASSERT(aRhs != NULL);

    if( sLhs->column->id == sRhs->column->id )
    {
        return 0;
    }
    else
    {
        if( sLhs->column->id > sRhs->column->id )
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
}

/*
 * [BUG-26923] [5.3.3 release] CodeSonar::Dangerous Function Cast
 * AA Functions ( SEEK )
 */
static IDE_RC smiTableCursorAAFunction( void );
static IDE_RC smiTableCursorSeekAAFunction( void*, const void* );

/*
 * NA1 Functions( INSERT, UPDATE, REMOVE )
 */
static IDE_RC smiTableCursorNA1Function( void );
static IDE_RC smiTableCursorInsertNA1Function( idvSQL*, void*, void*, void*,
                                               smSCN, SChar**, scGRID*,
                                               const smiValue*, UInt );
static IDE_RC smiTableCursorUpdateNA1Function( idvSQL*, void*, smSCN, void*,
                                               void*, SChar*, scGRID, SChar**,
                                               scGRID*, const smiColumnList*,
                                               const smiValue*, const smiDMLRetryInfo*,
                                               smSCN, void*, ULong, idBool );
static IDE_RC smiTableCursorRemoveNA1Function( idvSQL*, void*, smSCN, void*,
                                               void*, SChar*, scGRID, smSCN,
                                               const smiDMLRetryInfo*,
                                               idBool, idBool );

/*
 * NA2 Functions ( SEEK )
 */
static IDE_RC smiTableCursorNA2Function( void );
static IDE_RC smiTableCursorSeekNA2Function( void*, const void* );

/*
 * NA3 Functions ( LOCK_ROW )
 */
static IDE_RC smiTableCursorNA3Function( void );
static IDE_RC smiTableCursorLockRowNA3Function( smiIterator* );

static IDE_RC smiTableCursorDefaultCallBackFunction( idBool     * aResult,
                                                     const void *,
                                                     void       *, /* aDirectKey */
                                                     UInt        , /* aDirectKeyPartialSize */
                                                     const scGRID,
                                                     void       * );

/***********************************************************/
/* For A4 : Remote Query Specific Functions(PROJ-1068)     */
/***********************************************************/
IDE_RC (*smiTableCursor::openRemoteQuery)(smiTableCursor *     aCursor,
                                          const void*          aTable,
                                          smSCN                aSCN,
                                          const smiRange*      aKeyRange,
                                          const smiRange*      aKeyFilter,
                                          const smiCallBack*   aRowFilter,
                                          smlLockNode *        aCurLockNodePtr,
                                          smlLockSlot *        aCurLockSlotPtr);

IDE_RC (*smiTableCursor::closeRemoteQuery)(smiTableCursor * aCursor);

IDE_RC (*smiTableCursor::updateRowRemoteQuery)(smiTableCursor       * aCursor,
                                               const smiValue       * aValueList,
                                               const smiDMLRetryInfo* aRetryInfo);

IDE_RC (*smiTableCursor::deleteRowRemoteQuery)(smiTableCursor        * aCursor,
                                               const smiDMLRetryInfo * aRetryInfo);

IDE_RC (*smiTableCursor::beforeFirstModifiedRemoteQuery)(smiTableCursor * aCursor,
                                                         UInt             aFlag);

IDE_RC (*smiTableCursor::readOldRowRemoteQuery)(smiTableCursor * aCursor,
                                                const void    ** aRow,
                                                scGRID         * aRowGRID );

IDE_RC (*smiTableCursor::readNewRowRemoteQuery)(smiTableCursor * aCursor,
                                                const void    ** aRow,
                                                scGRID         * aRowGRID );

smiGetRemoteTableNullRowFunc smiTableCursor::mGetRemoteTableNullRowFunc;

// Transaction Isolation Level에 따라서 Table Lock Mode결정.
static const UInt smiTableCursorIsolationLock[4][8] =
{
    {/* SMI_ISOLATION_CONSISTENT                              */
        SMI_LOCK_READ,            /* SMI_LOCK_READ            */
        SMI_LOCK_WRITE,           /* SMI_LOCK_WRITE           */
        SMI_LOCK_REPEATABLE,      /* SMI_LOCK_REPEATABLE      */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_TABLE_SAHRED    */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE  /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SMI_ISOLATION_REPEATABLE                              */
        SMI_LOCK_REPEATABLE,      /* SMI_LOCK_READ            */
        SMI_LOCK_WRITE,           /* SMI_LOCK_WRITE           */
        SMI_LOCK_REPEATABLE,      /* SMI_LOCK_REPEATABLE      */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_TABLE_SAHRED    */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE  /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SMI_ISOLATION_NO_PHANTOM                              */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_READ            */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_WRITE           */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_REPEATABLE      */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_TABLE_SAHRED    */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE  /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SMI_ISOLATION_NO_PHANTOM                              */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_READ            */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_WRITE           */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_REPEATABLE      */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_TABLE_SAHRED    */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_TABLE_EXCLUSIVE */
        SMI_LOCK_TABLE_EXCLUSIVE  /* SMI_LOCK_TABLE_EXCLUSIVE */
    }
};

// SMI Interface의 Lock Mode를 smlLockMgr의 LockMode로 변환
static const  smlLockMode smiTableCursorLockMode[5] =
{
    SML_ISLOCK, /* SMI_LOCK_READ            */
    SML_IXLOCK, /* SMI_LOCK_WRITE           */
    SML_IXLOCK, /* SMI_LOCK_REPEATABLE      */
    SML_SLOCK,  /* SMI_LOCK_TABLE_SAHRED    */
    SML_XLOCK   /* SMI_LOCK_TABLE_EXCLUSIVE */
};

// Table에 걸려 있는 Lock Mode에 따라서 Cursor의 동작을 결정.
static const UInt smiTableCursorLock[6][5] =
{
    {/* SML_NLOCK                                             */
        0,                        /* SMI_LOCK_READ            */
        0,                        /* SMI_LOCK_WRITE           */
        0,                        /* SMI_LOCK_REPEATABLE      */
        0,                        /* SMI_LOCK_TABLE_SAHRED    */
        0                         /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SML_SLOCK                                             */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_READ            */
        0,                        /* SMI_LOCK_WRITE           */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_REPEATABLE      */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_TABLE_SAHRED    */
        0                         /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SML_XLOCK                                             */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_READ            */
        SMI_LOCK_TABLE_EXCLUSIVE, /* SMI_LOCK_WRITE           */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_REPEATABLE      */
        SMI_LOCK_TABLE_SHARED,    /* SMI_LOCK_TABLE_SAHRED    */
        SMI_LOCK_TABLE_EXCLUSIVE  /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SML_ISLOCK                                            */
        SMI_LOCK_READ,            /* SMI_LOCK_READ            */
        0,                        /* SMI_LOCK_WRITE           */
        SMI_LOCK_REPEATABLE,      /* SMI_LOCK_REPEATABLE      */
        0,                        /* SMI_LOCK_TABLE_SAHRED    */
        0                         /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SML_IXLOCK                                            */
        0,                        /* SMI_LOCK_READ            */
        SMI_LOCK_WRITE,           /* SMI_LOCK_WRITE           */
        SMI_LOCK_REPEATABLE,      /* SMI_LOCK_REPEATABLE      */
        0,                        /* SMI_LOCK_TABLE_SAHRED    */
        0                         /* SMI_LOCK_TABLE_EXCLUSIVE */
    },
    {/* SML_SIXLOCK                                           */
        SMI_LOCK_READ,            /* SMI_LOCK_READ            */
        SMI_LOCK_WRITE,           /* SMI_LOCK_WRITE           */
        SMI_LOCK_REPEATABLE,      /* SMI_LOCK_REPEATABLE      */
        0,                        /* SMI_LOCK_TABLE_SAHRED    */
        0                         /* SMI_LOCK_TABLE_EXCLUSIVE */
    }
};

static const smiCallBack smiTableCursorDefaultCallBack =
{
    0, smiTableCursorDefaultCallBackFunction, NULL
};

static const smiRange smiTableCursorDefaultRange =
{
    NULL, NULL, NULL,
    { 0, smiTableCursorDefaultCallBackFunction, NULL },
    { 0, smiTableCursorDefaultCallBackFunction, NULL }
};

static const smSeekFunc smiTableCursorSeekFunctions[6] =
{
    (smSeekFunc)smiTableCursorSeekNA2Function,
    (smSeekFunc)smiTableCursorSeekNA2Function,
    (smSeekFunc)smiTableCursorSeekNA2Function,
    (smSeekFunc)smiTableCursorSeekNA2Function,
    (smSeekFunc)smiTableCursorSeekAAFunction,
    (smSeekFunc)smiTableCursorSeekAAFunction
};

static const smiTableDMLFunc smiTableDMLFunctions[SMI_TABLE_TYPE_COUNT] =
{
    /* SMI_TABLE_META     */
    {
        (smTableCursorInsertFunc)   smcRecord::insertVersion,
        (smTableCursorUpdateFunc)   smcRecord::updateVersion,
        (smTableCursorRemoveFunc)   smcRecord::removeVersion,
    },
    /* SMI_TABLE_TEMP_LEGACY */
    {
        (smTableCursorInsertFunc)  NULL,
        (smTableCursorUpdateFunc)  NULL,
        (smTableCursorRemoveFunc)  NULL,
    },
    /* SMI_TABLE_MEMORY   */
    {
         (smTableCursorInsertFunc)  smcRecord::insertVersion,
         (smTableCursorUpdateFunc)  smcRecord::updateVersion,
         (smTableCursorRemoveFunc)  smcRecord::removeVersion,
    },
    /* SMI_TABLE_DISK     */
    {
        (smTableCursorInsertFunc)  sdcRow::insert,
        (smTableCursorUpdateFunc)  sdcRow::update,
        (smTableCursorRemoveFunc)  sdcRow::remove,
    },
    /* SMI_TABLE_FIXED    */
    {
        (smTableCursorInsertFunc)  NULL,
        (smTableCursorUpdateFunc)  NULL,
        (smTableCursorRemoveFunc)  NULL,
    },
    /* SMI_TABLE_VOLATILE */
    {
        (smTableCursorInsertFunc)  svcRecord::insertVersion,
        (smTableCursorUpdateFunc)  svcRecord::updateVersion,
        (smTableCursorRemoveFunc)  svcRecord::removeVersion,
    },
    /* SMI_TABLE_REMOTE   */
    {
        (smTableCursorInsertFunc)  NULL,
        (smTableCursorUpdateFunc)  NULL,
        (smTableCursorRemoveFunc)  NULL,
    }
};

static IDE_RC smMemoryFuncAA( void* /*aModule*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC smInitFuncAA( void                * /*aIterator*/,
                            void                * /*aTrans*/,
                            void                * /*aTable*/,
                            void                * /*aIndex*/,
                            void                * /*aDumpObject*/,
                            const smiRange      * /*aRange*/,
                            const smiRange      * /*aKeyFilter*/,
                            const smiCallBack   * /*aRowFilter*/,
                            UInt                  /*aFlag*/,
                            smSCN                 /*aSCN*/,
                            smSCN                 /*aInfinite*/,
                            idBool                /*aUntouchable*/,
                            smiCursorProperties * /*aProperties*/,
                            const smSeekFunc   ** /*aSeekFunc*/,
                            smiStatement        * /*aStatement*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC smDestFuncAA( void * /*aIterator*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC smAllocIteratorFuncAA( void ** /*aIteratorMem*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC smFreeIteratorFuncAA( void * /*aIteratorMem*/ )
{
    return IDE_SUCCESS;
}

static smnIndexModule smDefaultModule = {
    0, /* Type */
    0, /* Flag */
    0, /* Max Key Size */
    (smnMemoryFunc) smMemoryFuncAA,
    (smnMemoryFunc) smMemoryFuncAA,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc)   NULL,

    (smTableCursorLockRowFunc) NULL,
    (smnDeleteFunc)            NULL,
    (smnFreeFunc)              NULL,
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             NULL,
    (smInitFunc)               smInitFuncAA,
    (smDestFunc)               smDestFuncAA,
    (smnFreeNodeListFunc)      NULL,
    (smnGetPositionFunc)       NULL,
    (smnSetPositionFunc)       NULL,

    (smnAllocIteratorFunc)     smAllocIteratorFuncAA,
    (smnFreeIteratorFunc)      smFreeIteratorFuncAA,
    (smnReInitFunc)            NULL,
    (smnInitMetaFunc)          NULL,

    (smnMakeDiskImageFunc)     NULL,

    (smnBuildIndexFunc)        NULL,
    (smnGetSmoNoFunc)          NULL,
    (smnMakeKeyFromRow)        NULL,
    (smnMakeKeyFromSmiValue)   NULL,
    (smnRebuildIndexColumn)    NULL,
    (smnSetIndexConsistency)   NULL,
    (smnGatherStat)            NULL
};

/*
 * [BUG-26923] [5.3.3 release] CodeSonar::Dangerous Function Cast
 * AA Functions ( SEEK )
 */
static IDE_RC smiTableCursorAAFunction( void )
{
    return IDE_SUCCESS;
}
static IDE_RC smiTableCursorSeekAAFunction( void*, const void* )
{
    return smiTableCursorAAFunction();
}

/*
 * NA1 Functions ( INSERT, UPDATE, REMOVE )
 */
static IDE_RC smiTableCursorNA1Function( void )
{
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiWriteNotApplicable ) );

    return IDE_FAILURE;
}
static IDE_RC smiTableCursorInsertNA1Function( idvSQL*, void*, void*, void*,
                                               smSCN, SChar**, scGRID*,
                                               const smiValue*, UInt )
{
    return smiTableCursorNA1Function();
}
static IDE_RC smiTableCursorUpdateNA1Function( idvSQL*, void*, smSCN, void*,
                                               void*, SChar*, scGRID, SChar**,
                                               scGRID*, const smiColumnList*,
                                               const smiValue*, const smiDMLRetryInfo*,
                                               smSCN, void*, ULong, idBool )
{
    return smiTableCursorNA1Function();
}
static IDE_RC smiTableCursorRemoveNA1Function( idvSQL*, void*, smSCN, void*,
                                               void*, SChar*, scGRID, smSCN,
                                               const smiDMLRetryInfo*,
                                               idBool, idBool )
{
    return smiTableCursorNA1Function();
}

/*
 * NA2 Functions ( SEEK )
 */
static IDE_RC smiTableCursorNA2Function( void )
{
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;
}
static IDE_RC smiTableCursorSeekNA2Function( void*, const void* )
{
    return smiTableCursorNA2Function();
}

/*
 * NA3 Functions ( LOCK_ROW )
 */
static IDE_RC smiTableCursorNA3Function( void )
{
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;
}
static IDE_RC smiTableCursorLockRowNA3Function( smiIterator* )
{
    return smiTableCursorNA3Function();
}

static IDE_RC smiTableCursorDefaultCallBackFunction( idBool     * aResult,
                                                     const void *,
                                                     void       *, /* aDirectKey */
                                                     UInt        , /* aDirectKeyPartialSize */
                                                     const scGRID,
                                                     void       * )
{
    *aResult = ID_TRUE;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Cursor의 Member를 초기화
 **********************************************************************/
void smiTableCursor::init( void )
{
    mUntouchable = ID_TRUE;
    mTableInfo   = NULL;
    mDumpObject  = NULL;  // PROJ-1618
    mSeek[0]     =
    mSeek[1]     = (smSeekFunc)smiTableCursorSeekNA2Function;
    mInsert      = (smTableCursorInsertFunc)smiTableCursorInsertNA1Function;
    mUpdate      = (smTableCursorUpdateFunc)smiTableCursorUpdateNA1Function;
    mRemove      = (smTableCursorRemoveFunc)smiTableCursorRemoveNA1Function;
    mLockRow     = (smTableCursorLockRowFunc)smiTableCursorLockRowNA3Function;
    mSeekFunc    = smiTableCursorSeekFunctions;
}

/***********************************************************************
 * Description : Cursor의 Member를 초기화
 **********************************************************************/
void smiTableCursor::initialize()
{
    mPrev = mNext = this;
    mAllPrev = mAllNext = this;

    mTrans          = NULL;
    mStatement      = NULL;
    mCursorType     = SMI_SELECT_CURSOR;
    mIterator       = NULL;

    // PROJ-1509
    mFstUndoRecSID    = SD_NULL_SID;
    mCurUndoRecSID    = SD_NULL_SID;
    mLstUndoRecSID    = SD_NULL_SID;

    mFstOidNode       = NULL;
    mFstOidCount      = 0;
    mCurOidNode       = NULL;
    mCurOidIndex      = 0;
    mLstOidNode       = NULL;
    mLstOidCount      = 0;

    // PROJ-2068
    mDPathSegInfo   = NULL;

    mNeedUndoRec  = ID_TRUE;

    SC_MAKE_NULL_GRID( mLstInsRowGRID );

    mLstInsRowPtr = NULL;

    // PROJ-2416
    mUpdateInplaceEscalationPoint = NULL;
    
    init();
}


/***********************************************************************
 * Description :
 *   FOR A4 :
 *
 *   1. Table의 type에따라 table specific function을 setting한다.
 *      - mOps (Table Type에 따라서 open, close, update, delete ...등을 setting)
 *
 *   2. TableCursor의 멤버 변수 초기화
 *      - Trans, Table header, Index module(Sequential Iterator)
 *
 *   3. 1번과정에서 setting된 cursor open 함수를 호출한다.
 *
 *   aStatement  - [IN] Cursor가 속해 있는 Statement
 *   aTable      - [IN] Cursor가 Open할 Table Handle(Table Header Row Pointer
 *   aIndex      - [IN] Cursor가 이용할 Index Handle(Index Header Pointer)
 *   aSCN        - [IN] Table Row의 SCN으로서 Valid할때 Table의 SCN을 기록해두었다가
 *                      open할때 Table의 Row의 SCN과 비교해서 Table에 변경연산이
 *                      발생했는지 Check.
 *   aColumns    - [IN] Cursor가 Update Cursor일 겨우 Update할 Column List를 넘김.
 *   aKeyRange   - [IN] Key Range
 *   aKeyFilter  - [IN] Key Filter : DRDB를 위해 추가 되었습니다. KeyRange를 제외한
 *                      모든 조건문은 Filter로 선언되지만 만약 Filter중에 Index의 Column에
 *                      에 해당하는 조건이 있다면 이 조건은 대한 검사는 Index Node의 Key값을
 *                      이용함으로써 참조해야하는 Block의 갯수 Disk I/O를 줄일 수 있다. 때문에
 *                      Filter중에 Index Key에 해당하는 Filter는 Key Filter분리하였다.
 *   aRowFilter  - [IN] Filter
 *   aFlag       - [IN] 1. Cursor의 가 Table에 대해서 필요로 하는 Lock Mode
 *                         ( SMI_LOCK_ ... )
 *                      2. Cursor의 동작속성
 *                         ( SMI_PREVIOUS ... , SMI_TRAVERSE ... )
 *   aProperties - [IN]
 *   aIsDequeue  - [IN] deQueue 여부
 **********************************************************************/
IDE_RC smiTableCursor::open( smiStatement*        aStatement,
                             const void*          aTable,
                             const void*          aIndex,
                             smSCN                aSCN,
                             const smiColumnList* aColumns,
                             const smiRange*      aKeyRange,
                             const smiRange*      aKeyFilter,
                             const smiCallBack  * aRowFilter,
                             UInt                 aFlag,
                             smiCursorType        aCursorType,
                             smiCursorProperties* aProperties,
                             idBool               aIsDequeue )
{
    UInt            sTableType;
    smxTableInfo  * sTableInfo;
    smlLockNode   * sCurLockNodePtr = NULL;
    smlLockSlot   * sCurLockSlotPtr = NULL;
    UInt            sState = 0;

    IDE_ERROR( aTable     != NULL );
    IDE_ERROR( aKeyRange  != NULL );
    IDE_ERROR( aKeyFilter != NULL );
    IDE_ERROR( aRowFilter != NULL );
    IDE_ERROR( aStatement != NULL );

    IDU_FIT_POINT_RAISE( "1.PROJ-1407@smiTableCursor::open", ERR_ART_ABORT );

    mTrans = (smxTrans*)(aStatement->getTrans())->mTrans;
    mTransFlag  = aStatement->getTrans()->mFlag;

    mCursorType = aCursorType;
    mIndexModule = &smDefaultModule;

    /* Table Header를 mTable에 Setting한다. */
    mTable = (void*)((UChar*)aTable + SMP_SLOT_HEADER_SIZE );


    IDU_FIT_POINT_RAISE( "smiTableCursor::open::ERROR_INCONSISTENT_TABLE", ERROR_INCONSISTENT_TABLE );

    // PROJ-1665
    // DML을 위해 Lock을 잡는 경우, Table의 Consistent 상태 검사
    IDE_TEST_RAISE( ( smcTable::isTableConsistent( mTable ) != ID_TRUE) &&
                    ( smuProperty::getCrashTolerance() == 0 ),
                    ERROR_INCONSISTENT_TABLE );

    /* Direct Path Insert 수행 중에 다른 DML의 실행을 막는다.
     * SELECT CURSOR도 허용하지 않는 이유는 commit 전까지 HWM를 갱신하지 않기
     * 때문에 자신이 INSERT한 데이터를 자신이 볼 수 없으므로, 논리적인 무결성을
     * 지키기 위해 SELECT CURSOR도 허용하지 않는다. */
    if( (aFlag & SMI_INSERT_METHOD_MASK) != SMI_INSERT_METHOD_APPEND
        && ( ((smcTableHeader*)mTable)->mSelfOID != SM_NULL_OID ) )
    {
        IDE_TEST( mTrans->getTableInfo( ((smcTableHeader*)mTable)->mSelfOID,
                                        &sTableInfo,
                                        ID_TRUE) /* aIsSearch */
                  != IDE_SUCCESS );

        if( sTableInfo == NULL )
        {
            /* table info를 찾을 수 없으면 DPath INSERT를 수행했을리 없다. */
        }
        else
        {
            IDE_TEST_RAISE( smxTableInfoMgr::isExistDPathIns(sTableInfo)
                                == ID_TRUE,
                            ERROR_DML_AFTER_INSERT_APPEND );
        }
    }
    else
    {
        /* do nothing */
    }

    /* Table Type을 얻어온다.*/
    sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader*)mTable );

    //BUG-48230: Dequeue 여부
    mIsDequeue = aIsDequeue;

    /* PROJ-2162 */
    if( ( aIndex != NULL ) &&
        ( ( sTableType == SMI_TABLE_DISK ) || 
          ( sTableType == SMI_TABLE_MEMORY ) || 
          ( sTableType == SMI_TABLE_META ) || 
          ( sTableType == SMI_TABLE_VOLATILE ) ) )
    {
        IDE_TEST_RAISE( smnManager::getIsConsistentOfIndexHeader(
                                (void*)aIndex )
                        == ID_FALSE, ERROR_INCONSISTENT_INDEX );
    }


    /* Table Type에 따라서 연산자를 결정한다. */
    switch(sTableType)
    {
        case SMI_TABLE_DISK:
            mOps.openFunc      = smiTableCursor::openDRDB;
            mOps.closeFunc     = smiTableCursor::closeDRDB;
            mOps.updateRowFunc = smiTableCursor::updateRowDRDB;
            mOps.deleteRowFunc = smiTableCursor::deleteRowDRDB;

            // PROJ-1509
            mOps.beforeFirstModifiedFunc =
                smiTableCursor::beforeFirstModifiedDRDB;
            mOps.readOldRowFunc  = smiTableCursor::readOldRowDRDB;
            mOps.readNewRowFunc  = smiTableCursor::readNewRowDRDB;

            break;

        case SMI_TABLE_FIXED:
            mOps.openFunc      = smiTableCursor::openMRVRDB;
            mOps.closeFunc     = smiTableCursor::closeMRVRDB;
            mOps.updateRowFunc = NULL;
            mOps.deleteRowFunc = NULL;                

            // PROJ-1509
            mOps.beforeFirstModifiedFunc =NULL;
            mOps.readOldRowFunc  = NULL;
            mOps.readNewRowFunc  = NULL;
            break;

        case SMI_TABLE_MEMORY:
        case SMI_TABLE_META:
            mOps.openFunc      = smiTableCursor::openMRVRDB;
            mOps.closeFunc     = smiTableCursor::closeMRVRDB;
            mOps.updateRowFunc = smiTableCursor::updateRowMRVRDB;
            mOps.deleteRowFunc = smiTableCursor::deleteRowMRVRDB;

            // PROJ-1509
            mOps.beforeFirstModifiedFunc =
                smiTableCursor::beforeFirstModifiedMRVRDB;
            mOps.readOldRowFunc  = smiTableCursor::readOldRowMRVRDB;
            mOps.readNewRowFunc  = smiTableCursor::readNewRowMRVRDB;
            break;
        /* PROJ-1594 Volatile TBS */
        case SMI_TABLE_VOLATILE:
            mOps.openFunc      = smiTableCursor::openMRVRDB;
            mOps.closeFunc     = smiTableCursor::closeMRVRDB;
            mOps.updateRowFunc = smiTableCursor::updateRowMRVRDB;
            mOps.deleteRowFunc = smiTableCursor::deleteRowMRVRDB;

            // PROJ-1509
            mOps.beforeFirstModifiedFunc =
                smiTableCursor::beforeFirstModifiedMRVRDB;
            mOps.readOldRowFunc  = smiTableCursor::readOldRowMRVRDB;
            mOps.readNewRowFunc  = smiTableCursor::readNewRowMRVRDB;
            break;
        /* PROJ-1068 Database Link */
        case SMI_TABLE_REMOTE:
            mOps.openFunc      = smiTableCursor::openRemoteQuery;
            mOps.closeFunc     = smiTableCursor::closeRemoteQuery;
            mOps.updateRowFunc = smiTableCursor::updateRowRemoteQuery;
            mOps.deleteRowFunc = smiTableCursor::deleteRowRemoteQuery;
            mIndexModule       = gSmnSequentialScan.mModule[SMN_GET_BASE_TABLE_TYPE_ID(SMI_TABLE_REMOTE)];

            // PROJ-1509
            mOps.beforeFirstModifiedFunc =
                smiTableCursor::beforeFirstModifiedRemoteQuery;
            mOps.readOldRowFunc  = smiTableCursor::readOldRowRemoteQuery;
            mOps.readNewRowFunc  = smiTableCursor::readNewRowRemoteQuery;
            break;
        case SMI_TABLE_TEMP_LEGACY:
        default:
            IDE_ASSERT(0);
            break;
    }

    if ((( aFlag & SMI_INSERT_METHOD_MASK) == SMI_INSERT_METHOD_APPEND ) &&
        ( smuProperty::getDPathInsertEnable() == ID_TRUE ))
    {
        // Direct-Path INSERT인 경우
        mOps.insertRowFunc = smiTableCursor::dpathInsertRow;
    }
    else
    {
        // 그 외의 경우
        mOps.insertRowFunc = smiTableCursor::normalInsertRow;
    }

    /*
      Table Type Independent Function Call
       - TableCursor의 멤버 변수 초기화
         - Trans, Table header, index module...
    */
    IDE_TEST( doCommonJobs4Open(aStatement,
                                aIndex,
                                aColumns,
                                aFlag,
                                aProperties,
                                &sCurLockNodePtr,
                                &sCurLockSlotPtr) != IDE_SUCCESS );
    sState = 1;

    /* Table Type Dependent Function Call */
    IDE_TEST( mOps.openFunc( this,
                             aTable,
                             aSCN,
                             aKeyRange,
                             aKeyFilter,
                             aRowFilter,
                             sCurLockNodePtr,
                             sCurLockSlotPtr) != IDE_SUCCESS );

    /* PROJ-2162 RestartRiskReductino
     * DB가 Inconsistency 한데,  META등도 아닌데 접근하려 한다면 */
    IDE_ERROR( sTableType != SMI_TABLE_TEMP_LEGACY );
    IDE_TEST_RAISE( ( smrRecoveryMgr::getConsistency() == ID_FALSE ) &&
                    ( smuProperty::getCrashTolerance() == 0 ) &&
                    ( sTableType != SMI_TABLE_META ) &&
                    ( sTableType != SMI_TABLE_FIXED ),
                    ERROR_INCONSISTENT_DB );

    /* PROJ-2462 Result Cache */
    if ( ( mCursorType == SMI_INSERT_CURSOR ) ||
         ( mCursorType == SMI_DELETE_CURSOR ) ||
         ( mCursorType == SMI_UPDATE_CURSOR ) ||
         ( mCursorType == SMI_COMPOSITE_CURSOR ) )
    {
        sTableType = SMI_GET_TABLE_TYPE( ( smcTableHeader * )mTable );

        if ( ( sTableType == SMI_TABLE_DISK ) ||
             ( sTableType == SMI_TABLE_MEMORY ) ||
             ( sTableType == SMI_TABLE_VOLATILE ) ||
             ( sTableType == SMI_TABLE_META ) )
        {
            smcTable::addModifyCount( (smcTableHeader *)mTable );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( ERR_ART_ABORT );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_ART ) );
    }
#endif
    IDE_EXCEPTION( ERROR_INCONSISTENT_DB );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INCONSISTENT_DB) );
    }
    IDE_EXCEPTION( ERROR_INCONSISTENT_TABLE );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INCONSISTENT_TABLE) );
    }
    IDE_EXCEPTION( ERROR_INCONSISTENT_INDEX );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ));
    }
    IDE_EXCEPTION( ERROR_DML_AFTER_INSERT_APPEND );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_DML_AFTER_INSERT_APPEND) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if( ideGetErrorCode() == smERR_REBUILD_smiTableModified )
    {
        SMX_INC_SESSION_STATISTIC( mTrans,
                                   IDV_STAT_INDEX_STMT_REBUILD_COUNT,
                                   1 /* Increase Value */ );
    }

    if( sState == 1 )
    {
        mStatement->closeCursor( this );
        // To Fix PR-13919
        // mOps.openFunc() 에서 error가 발생하면
        // mIterator 메모리가 해제되지 않는다.
        if(mIterator != NULL)
        {
            // 할당된 Iterator를 Free한다.
            IDE_ASSERT( ((smnIndexModule*)mIndexModule)->mFreeIterator((void*)mIterator)
                        == IDE_SUCCESS );
            mIterator = NULL;
        }
        else
        {
            // Nothing To Do
        }
    }

    init();

    IDE_POP();

    return IDE_FAILURE;
}

/*
    Hybrid Transaction 검사 실시

    => Disk, Memory테이블 접근(Read/Write)시 Transaction에 Flag설정
       => 두가지 다 접근된 Transaction이 Hybrid Transaction

    => Meta 테이블 변경시 Transaction에 Flag설정
       => 추후 Commit시 Log Flush실시
       => 이유: smiDef.h의 SMI_TABLE_META_LOG_FLUSH_MASK 부분 주석 참고
 */
IDE_RC smiTableCursor::checkHybridTrans( UInt             aTableType,
                                         smiCursorType    aCursorType )
{
    IDE_DASSERT( mTrans != NULL );

    switch ( aTableType )
    {
        case SMI_TABLE_DISK:
            mTrans->setDiskTBSAccessed();

            break;

        case SMI_TABLE_MEMORY:
            mTrans->setMemoryTBSAccessed();
            break;

        case SMI_TABLE_META:
            if (aCursorType != SMI_SELECT_CURSOR )
            {
                // Meta Table의 변경이 되었을 때 Log를 Flush할지 여부를
                // 확인.
                //
                // 이 Flag가 켜져 있는 경우에만
                // Meta Table변경한 Transaction의
                // Commit시에 로그를 Flush하도록 한다.
                if ( (((smcTableHeader*)mTable)->mFlag & SMI_TABLE_META_LOG_FLUSH_MASK )
                     == SMI_TABLE_META_LOG_FLUSH_TRUE )
                {
                    mTrans->setMetaTableModified();
                }
            }
            break;

        default:
            // Do nothing
            break;
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Table Type에 따라서 open연산이 공통되는 부분과 다른 부분이 있는데
 *               여기서는 공통되는 부분을 처리한다.
 *
 *               1. 공통 Member 초기화
 *               2. Table Lock 수행.
 *               3. Statement에 Cursor 등록
 *
 * aStatement  - [IN] Cursor가 속한 Statement
 * aIndex      - [IN] Index Handle (Index Header Pointer)
 * aColumns    - [IN] Update Cursor일 경우 Update될 Column List
 * aFlag       - [IN] 1. Cursor의 가 Table에 대해서 필요로 하는 Lock Mode
 *                      ( SMI_LOCK_ ... )
 *                    2. Cursor의 동작속성
 *                      ( SMI_PREVIOUS ... , SMI_TRAVERSE ... )
 *
 * aProperties     - [IN]
 * aCurLockNodePtr - [OUT] Table에 Lock수행시 이 Table의 Lock Item에  Lock
 *                         Node가 생긴다. 이 Lock Node에 대한 Pointer
 * aCurLockSlotPtr - [OUT] Lock Node의 Lock Slot Pointer
 *
 **********************************************************************/
IDE_RC smiTableCursor::doCommonJobs4Open( smiStatement*        aStatement,
                                          const void*          aIndex,
                                          const smiColumnList* aColumns,
                                          UInt                 aFlag,
                                          smiCursorProperties* aProperties,
                                          smlLockNode **       aCurLockNodePtr,
                                          smlLockSlot **       aCurLockSlotPtr)
{
    idBool           sLocked;
    idBool           sIsExclusive;
    SInt             sIsolationFlag;
    smcTableHeader * sTable;
    idBool           sIsMetaTable = ID_FALSE;
    UInt             sTableType;
    UInt             sTableTypeID;
    UChar            sIndexTypeID;
    smlLockMode      sLockMode;
    smnIndexModule  * sIndexModule = (smnIndexModule*)mIndexModule;

    IDE_TEST_RAISE( mIterator != NULL, ERR_ALREADY_OPENED );

    mStatement    = aStatement;
    sTable        = (smcTableHeader*)mTable;
    mIndex        = (void*)aIndex;
    mIsSoloCursor = ID_FALSE;
    sTableType    = SMI_GET_TABLE_TYPE( sTable );
    sTableTypeID  = SMI_TABLE_TYPE_TO_ID( sTableType );
    sIndexTypeID  = aProperties->mIndexTypeID;

    IDE_ERROR( sIndexTypeID < SMI_INDEXTYPE_COUNT );
    IDE_ERROR( sTableTypeID < SMI_TABLE_TYPE_COUNT );

    if( (aIndex == NULL) && (mCursorType == SMI_INSERT_CURSOR) )
    {
        /* do nothing */
    }
    else
    {
        if( aIndex != NULL )
        {
            sIndexModule = ((smnIndexHeader*)aIndex)->mModule;
        }
        else
        {
            sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];
        }

        IDE_TEST_RAISE( sIndexModule == NULL, err_disabled_index );

        /* index module이 올바로 설정 되었는지 index와 table type으로 검증 */
        IDE_ERROR( SMN_MAKE_INDEX_TYPE_ID(sIndexModule->mType) ==
                   (UInt)aProperties->mIndexTypeID );
        IDE_ERROR( SMN_MAKE_TABLE_TYPE_ID(sIndexModule->mType) ==
                   sTableTypeID );
    
        mIndexModule = sIndexModule;
    }

    /* FOR A4 :
       Iterator 메모리 할당을 Index Module로 넘김
       Insert의 경우에는 mIsertIterator를 사용 -- 나중에...

       if flag에 Insert  OP가 세팅되어 있으면
       mIterator에 mInsertIterator의 주소를 세팅
       else
       mIterator에 mIndexModule->allocIterator()의 값을 세팅
       endif
    */
    /* BUG-43408, BUG-45368 */
    mIterator = (smiIterator *)mIteratorBuffer;

    IDE_TEST(sIndexModule->mAllocIterator((void**)&mIterator)
             != IDE_SUCCESS );

    mColumns       = aColumns;
    mFlag          = aFlag;

    /* BUG-21738: smiTableCursor에서 통계 정보 인자로 mQPProp->mStatistics를
     * 잘못사용하고 있음. */
    mOrgCursorProp = *aProperties;
    mCursorProp    = mOrgCursorProp;

    /*
        FOR A4 : FirstReadPos와 ReadRecordCount를 QP에서 계산해서 내려주기로 함.
        Replication에서도 QP와 같이 일관된 값을 내려줌
        isRepl 인자가 필요없어짐.
    */
    /* Fixed Table일 경우 Lock Table 호출할 필요 없음. */
    if ( (sTableType == SMI_TABLE_META) || (sTableType == SMI_TABLE_FIXED) ||
         (sTableType == SMI_TABLE_REMOTE))
    {
        sIsMetaTable   = ID_TRUE;
        sIsolationFlag = SMI_ISOLATION_CONSISTENT;
    }
    else
    {
        if (( mFlag & SMI_TRANS_ISOLATION_IGNORE ) == SMI_TRANS_ISOLATION_IGNORE )
        {
            /* BUG-47758 Transaction의 Isolation level과 무관하게
             * QP,RP가 내려준 값으로만 Cursor의 Isolation level을 설정 */
            sIsolationFlag = SMI_ISOLATION_CONSISTENT;
        }
        else
        {
            /* Transaction Isolation level을 참고해서
             * Table cursor의 isolation level 을 갱신함*/
            sIsolationFlag = mTransFlag & SMI_ISOLATION_MASK;
        }
    }

    /*
       Meta Table은 Create된 이후에 DDL이 발생하지 않기 때문에 Table에 Lock
       을 걸지 않는다.
    */
    if ( sIsMetaTable == ID_FALSE )
    {
        mFlag = (mFlag & ~SMI_LOCK_MASK) |
            smiTableCursorIsolationLock[sIsolationFlag][mFlag & SMI_LOCK_MASK];

        sIsExclusive = smlLockMgr::isNotISorS(
                       smiTableCursorLockMode[mFlag&SMI_LOCK_MASK]);

        // 테이블과 관련된 테이블스페이스들에 대하여 INTENTION 잠금을 획득한다.
        IDE_TEST( sctTableSpaceMgr::lockAndValidateTBS(
                          (void*)mTrans,
                          smcTable::getTableSpaceID((void*)mTable),
                          SCT_VAL_DDL_DML,
                          ID_TRUE,   /* intent lock  여부 */
                          sIsExclusive, /* exclusive lock */
                          mCursorProp.mLockWaitMicroSec)
                  != IDE_SUCCESS );

         if ( (sTableType == SMI_TABLE_DISK) &&
              (mFlag & SMI_INSERT_METHOD_MASK) == SMI_INSERT_METHOD_APPEND )
         {
             sLockMode = SML_SIXLOCK;
         }
         else
         {
             sLockMode = smiTableCursorLockMode[mFlag&SMI_LOCK_MASK];
         }

        /* Table Lock 수행 */
        IDE_TEST(smlLockMgr::lockTable(
                        mTrans->mSlotN,
                        (smlLockItem *)( SMC_TABLE_LOCK( (smcTableHeader *)mTable ) ),
                        sLockMode,
                        mCursorProp.mLockWaitMicroSec,
                        &mLockMode,
                        &sLocked,
                        aCurLockNodePtr,
                        aCurLockSlotPtr) != IDE_SUCCESS );

        IDE_TEST(sLocked != ID_TRUE);

        // 테이블과 관련된 Index, Blob 테이블스페이스들에 대하여
        // INTENTION 잠금을 획득한다.
        IDE_TEST( sctTableSpaceMgr::lockAndValidateRelTBSs(
                          (void*)mTrans,
                          (void*)mTable,
                          SCT_VAL_DDL_DML,
                          ID_TRUE,   /* intent lock  여부 */
                          sIsExclusive, /* exclusive lock */
                          mCursorProp.mLockWaitMicroSec)
                  != IDE_SUCCESS );

        /*
           Table에 요청한 Lock Mode와 다르게 Lock이 결정될 수 있다.
           왜냐하면 여러개의 Transaction이 Table에 다양한 Lock을 잡고
           또한 하나의 Transaction 또한 여러 Mode로 Lock을 수행할 수 있기
           때문이다.
           때문에 mFlag에 Table에 걸려있는 Lock Mode로 변화시켜줘야 한다.

           Ex) Table T1이 있다고 하자.
               Tx2이 Lock Table T1 In S Mode 을 수행 후
               Tx1이 Lock Table T1 In IS Mode 을 하면
               이면 Tx1의 요청한 Lock Mode는 S Mode로 결정된다.
        */
        mFlag = (mFlag & (~SMI_LOCK_MASK)) |
            smiTableCursorLock[mLockMode][mFlag&SMI_LOCK_MASK];
    }
    else
    {
        mLockMode = SML_IXLOCK;
    }

    if( ((mFlag & SMI_LOCK_MASK) == SMI_LOCK_WRITE ) ||
        ((mFlag & SMI_LOCK_MASK) == SMI_LOCK_TABLE_EXCLUSIVE ) )
    {
        /* Change연산(Insert, Update, Delete)을 수행하는 Cursor */
        // PROJ-2199 SELECT func() FOR UPDATE 지원
        // SMI_STATEMENT_FORUPDATE 추가
        if( ((mStatement->mFlag & SMI_STATEMENT_MASK) == SMI_STATEMENT_NORMAL) ||
            ((mStatement->mFlag & SMI_STATEMENT_MASK) == SMI_STATEMENT_FORUPDATE) )
        {
            IDE_TEST(mTrans->getTableInfo(
                         sTable->mSelfOID,
                         &mTableInfo) != IDE_SUCCESS);
        }

        mUntouchable = ID_FALSE;

        mInsert  = (smTableCursorInsertFunc)smiTableDMLFunctions[ sTableTypeID ].mInsert;
        mUpdate  = (smTableCursorUpdateFunc)smiTableDMLFunctions[ sTableTypeID ].mUpdate;
        mRemove  = (smTableCursorRemoveFunc)smiTableDMLFunctions[ sTableTypeID ].mRemove;
        mLockRow = (smTableCursorLockRowFunc)sIndexModule->mLockRow;
    }
    else
    {
        if((mFlag & SMI_LOCK_MASK) == SMI_LOCK_REPEATABLE)
        {
            /* Lock Record And LOB Update */
            mUntouchable = ID_FALSE;
        }
        else
        {
            /* ReadOnly Cursor */
            mUntouchable = ID_TRUE;
        }

        mInsert = (smTableCursorInsertFunc)smiTableCursorInsertNA1Function;
        mUpdate = (smTableCursorUpdateFunc)smiTableCursorUpdateNA1Function;
        mRemove = (smTableCursorRemoveFunc)smiTableCursorRemoveNA1Function;
    }

    if( (mFlag & SMI_INSERT_METHOD_MASK) == SMI_INSERT_METHOD_APPEND )
    {
        IDE_ERROR( ((smcTableHeader*)mTable)->mSelfOID != SM_NULL_OID );
        IDE_ERROR( mTableInfo != NULL );

        if( smxTableInfoMgr::isExistDPathIns(mTableInfo) == ID_TRUE )
        {
            /* 이미 DPath INSERT를 수행한 table로 표시되어 있음. */
        }
        else
        {
            /* 대상 table에 대해 최초의 DPath INSERT를 수행할 때, table info에
             * DPath INSERT를 수행한 table이라고 표시하고, abort시 이 표시를
             * 되돌릴 NTA log를 남겨둔다. */
            smxTableInfoMgr::setExistDPathIns( mTableInfo,
                                               ID_TRUE ); /* aExistDPathIns */

            /* rollback 되면 mExistDPathIns 플래그를 ID_FALSE로 변경해 주기
             * 위한 NTA log를 남긴다. */
            IDE_TEST( smrLogMgr::writeNTALogRec(
                                mTrans->mStatistics,
                                mTrans,
                                smxTrans::getTransLstUndoNxtLSNPtr(mTrans),
                                smcTable::getSpaceID(mTable),
                                SMR_OP_DIRECT_PATH_INSERT,
                                sTable->mSelfOID )
                      != IDE_SUCCESS );
        }
    }

    /* Statement에 cursor를 등록하고 infinite SCN 과 undo no를 얻어온다. */
    IDE_TEST( mStatement->openCursor( this, &mIsSoloCursor )
              != IDE_SUCCESS );

    /* PROJ-2694 Fetch Across Rollback 
     * rollback시 cursor 재사용이 가능한지 여부를 판단하기 위해 
     * holdable로 cursor open시 Infinite SCN을 저장해둔다. */
    if( ( mTrans->mIsCursorHoldable ) == ID_TRUE )
    {
        SM_SET_SCN( &mTrans->mCursorOpenInfSCN, &mInfinite );

        /* 다음 cursor open시 holdable이 아닐 경우에도 holdable로 판단하지 않도록
         * mIsCursorHoldable 변수를 false로 세팅한다. */
        mTrans->mIsCursorHoldable = ID_FALSE;
    }

    return IDE_SUCCESS;

    /* 커서가 이미 오픈되어 있습니다. */
    IDE_EXCEPTION( ERR_ALREADY_OPENED );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smiCursorOpened ) );
    }
    IDE_EXCEPTION( err_disabled_index);
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_DISABLED_INDEX ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if( (mIndexModule != NULL) && (mIterator != NULL) )
    {
        IDE_ASSERT(((smnIndexModule*)mIndexModule)->mFreeIterator((void*)mIterator)
                   == IDE_SUCCESS);

        mIterator = NULL;
    }

    init();

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 현재 Open된 Cursor를 Close하지 않고 Key Range,Fileter를 바꿔서
 *               재사용하기 위해 제안되었다.실제 데이타를 읽어들이고 찾는것은 Iterator가
 *               하기 때문에 새로운 Key Range와 Filter를 이용해 Iterator를 초기화한다.
 *
 * aKeyRange   - [IN] 새로운 Key Range
 * aKeyFilter  - [IN] 새로운 Key Filter
 * aRowFilter  - [IN] Filter
 *
 **********************************************************************/
IDE_RC smiTableCursor::restart( const smiRange *    aKeyRange,
                                const smiRange *    aKeyFilter,
                                const smiCallBack * aRowFilter )
{
    UInt sTableType;

    IDE_ERROR( aKeyRange  != NULL );
    IDE_ERROR( aKeyFilter != NULL );
    IDE_ERROR( aRowFilter != NULL );

    IDE_TEST_RAISE( mIterator == NULL, ERR_CLOSED );

    mCursorProp = mOrgCursorProp;

    // BUG-41560
    sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader*)mTable );

    if ( sTableType != SMI_TABLE_FIXED )
    {
        IDE_TEST( ((smnIndexModule*)mIndexModule)->mDestIterator( mIterator ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* FOR A4 : smiCursorProperties와 Iterator관련 부분 변경 */
    IDE_TEST( ((smnIndexModule*)mIndexModule)->mInitIterator( mIterator,
                                                              mTrans,
                                                              mTable,
                                                              mIndex,
                                                              mDumpObject,
                                                              aKeyRange,
                                                              aKeyFilter,
                                                              aRowFilter,
                                                              mFlag,
                                                              mSCN,
                                                              mInfinite,
                                                              mUntouchable,
                                                              &mCursorProp,
                                                              &mSeekFunc,
                                                              mStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLOSED );
    IDE_SET( ideSetErrorCode( smERR_FATAL_smiCursorNotOpened ) );
    IDE_EXCEPTION_END;

    init();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Cursor를 Close하고 할당된 Iterator를 Free한다.
 *
 **********************************************************************/
IDE_RC smiTableCursor::close( void )
{
    SInt sState = 2;

    //BUG-27609 CodeSonar::Null Pointer Dereference (8)
    IDE_ERROR( this   != NULL );
    IDE_ERROR( mTrans != NULL );

    if(mTrans != NULL)
    {
        IDU_FIT_POINT( "smiTableCursor::close::closeFunc" );

        // Cursor를 Close한다.
        IDE_TEST( mOps.closeFunc(this) != IDE_SUCCESS );
    }

    if(mStatement != NULL)
    {
        // Statement에서 Cursor를 제거한다.
        mStatement->closeCursor( this );
    }

    sState = 1;

    if(mIterator != NULL)
    {
        // 할당된 Iterator를 Free한다.
        IDE_TEST(((smnIndexModule*)mIndexModule)->mFreeIterator((void*)mIterator)
                 != IDE_SUCCESS );
        mIterator = NULL;
    }

    sState = 0;

    // PROJ-1509
    // init();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sState)
    {
        case 2 :
        {
            if(mStatement != NULL)
            {
                mStatement->closeCursor( this );
            }
        }
        case 1 :
        {
            if(mIterator != NULL)
            {
                IDE_ASSERT(((smnIndexModule*)mIndexModule)->mFreeIterator((void*)mIterator)
                           == IDE_SUCCESS);
                mIterator = NULL;
            }
        }
    }

    init();

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Cursor Key Range, Key Filter 조건을 만족하는 첫번째 Data를 찾아서
 *               Cursor를 위치시킨다.
 *
 **********************************************************************/
IDE_RC smiTableCursor::beforeFirst( void )
{
    IDE_DASSERT( checkVisibilityConsistency() == IDE_SUCCESS );

    if ( mIsDequeue == ID_FALSE )
    {
        // Queue 아닐 때
        IDE_TEST( mSeekFunc[SMI_FIND_BEFORE]( mIterator,
                                              (const void**)&mSeekFunc )
                  != IDE_SUCCESS );  

        mSeek[0] = mSeekFunc[0];
        mSeek[1] = mSeekFunc[1];
    }
    else
    {
        //BUG-48230: Queue 일 때
        if ( ( mFlag & SMI_TRAVERSE_MASK ) == SMI_TRAVERSE_FORWARD )
        {
            //SMI_TRAVERSE_FORWARD
            IDE_TEST( smnbBTree::beforeFirstQ( mIterator )
                      != IDE_SUCCESS );
            mSeek[0] = (smSeekFunc)smnbBTree::fetchNextQ;
            mSeek[1] = (smSeekFunc)smnbBTree::NA;
        }
        else
        {
            //SMI_TRAVERSE_BACKWARD
            IDE_TEST( smnbBTree::afterLastQ( mIterator )
                      != IDE_SUCCESS );

            mSeek[0] = (smSeekFunc)smnbBTree::fetchPrev;
            mSeek[1] = (smSeekFunc)smnbBTree::NA;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    init();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Cursor Key Range, Key Filter 조건을 만족하는 마지막 Data를 찾아서
 *               Cursor를 위치시킨다.
 *
 **********************************************************************/
IDE_RC smiTableCursor::afterLast( void )
{
    IDE_DASSERT( checkVisibilityConsistency() == IDE_SUCCESS );

    IDE_TEST( mSeekFunc[SMI_FIND_AFTER]( mIterator,
                                         (const void**)&mSeekFunc )
              != IDE_SUCCESS );

    mSeek[0] = mSeekFunc[0];
    mSeek[1] = mSeekFunc[1];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    init();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Cursor Key Range, Key Filter 조건을 만족하는 다음 Row를
 *               읽어온다.MMDB일 경우에는 Row Pointer를, DRDB일 경우에는
 *               aRow에 할당된 Buffer에 Row를 복사하여 준다.
 *
 * aRow  - [OUT] if DRDB, row의 데이타를 aRow에 복사하여 준다.MMDB면
 *                  Row의 Pointer를 넣어준다.
 * aGRID  - [OUT] Row의 RID를 Setting한다.
 * aFlag - 현재 위치에서 현재, 다음, 이전데이타를 가져오는 것을 결정한다.
 *
 *          1.SMI_FIND_NEXT : 다음 Row를 Fetch
 *          2.SMI_FIND_PREV : 이전 Row를 Fetch
 *          3.SMI_FIND_CURR : 현재 Row를 Fetch
 *
 **********************************************************************/
IDE_RC smiTableCursor::readRow( const void  ** aRow,
                                scGRID       * aRowGRID,
                                UInt           aFlag )
{
    IDE_DASSERT( checkVisibilityConsistency() == IDE_SUCCESS );

    IDU_FIT_POINT( "smiTableCursor::readRow::mSeek" );

    IDE_TEST( mSeek[aFlag&SMI_FIND_MASK]( mIterator, aRow )
              != IDE_SUCCESS );

    SC_COPY_GRID( mIterator->mRowGRID, *aRowGRID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    init();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: 주어진 GRID에 해당하는 row를 fetch한다.
 *
 * Parameters:
 *  aRow        - [IN] row를 저장할 buffer
 *  aRowGRID    - [IN] fetch 대상 row의 GRID
 **********************************************************************/
IDE_RC smiTableCursor::readRowFromGRID( const void  ** aRow,
                                        scGRID         aRowGRID )
{
    idvSQL    * sStatistics = smxTrans::getStatistics( mTrans );
    UInt        sTableType  = SMI_GET_TABLE_TYPE( (smcTableHeader*)mTable );
    UChar     * sSlotPtr;
    idBool      sIsSuccess;
    idBool      sIsPageLatchReleased = ID_TRUE;
    idBool      sIsRowDeleted;

    IDE_ERROR( aRow != NULL );
    IDE_ERROR( SC_GRID_IS_NOT_NULL(aRowGRID) );
    IDE_ERROR( sTableType == SMI_TABLE_DISK );

    IDE_TEST( sdbBufferMgr::getPageByGRID( sStatistics,
                                           aRowGRID,
                                           SDB_S_LATCH,
                                           SDB_WAIT_NORMAL,
                                           NULL, /* aMtx */
                                           (UChar**)&sSlotPtr,
                                           &sIsSuccess )
              != IDE_SUCCESS );
    sIsPageLatchReleased = ID_FALSE;

    IDE_ERROR( sdpPhyPage::getPageType( sdpPhyPage::getHdr(sSlotPtr) )
               == SDP_PAGE_DATA);

    IDU_FIT_POINT( "smiTableCursor::readRowFromGRID::fetch" );

    IDE_TEST( sdcRow::fetch( sStatistics,
                             NULL, /* aMtx */
                             NULL, /* aSP */
                             mTrans,
                             SC_MAKE_SPACE(aRowGRID),
                             sSlotPtr,
                             ID_TRUE, /* aIsPersSlot */
                             SDB_SINGLE_PAGE_READ,
                             mCursorProp.mFetchColumnList,
                             SMI_FETCH_VERSION_CONSISTENT,
                             smxTrans::getTSSlotSID(mTrans),
                             &mSCN,
                             &mInfinite,
                             NULL, /* aIndexInfo4Fetch */
                             NULL, /* aLobInfo4Fetch */
                             ((smcTableHeader*)mTable)->mRowTemplate,
                             (UChar*)*aRow,
                             &sIsRowDeleted,
                             &sIsPageLatchReleased )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsRowDeleted == ID_TRUE, error_deletedrow );

    if( sIsPageLatchReleased == ID_FALSE )
    {
        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage( sStatistics,
                                             sSlotPtr )
                  != IDE_SUCCESS );
    }

    SC_COPY_GRID( aRowGRID, mIterator->mRowGRID );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_deletedrow );
    {
        ideLog::log( 
            IDE_SM_0, 
            "InternalError[%s:%d]\n"
            "GRID     : <%u,%u,%u>\n"
            "TSSRID   : %llu\n"
            "SCN      : %llu, %llu\n",
            (SChar *)idlVA::basename(__FILE__),
            __LINE__,
            SC_MAKE_SPACE( aRowGRID ),
            SC_MAKE_PID( aRowGRID ),
            SC_MAKE_OFFSET( aRowGRID ),
            smxTrans::getTSSlotSID(mTrans),
            SM_SCN_TO_LONG(mSCN),
            SM_SCN_TO_LONG(mInfinite) );
        ideLog::logCallStack( IDE_SM_0 );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL_ARG, 
                                  "Disk Fetch by RID" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( sStatistics,
                                               sSlotPtr )
                    == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: 주어진 row, GRID에 해당하는 레코드를 위치시킨다.
 *
 * Parameters:
 *  aRow     - [IN] fetch 대상 row의 pointer
 *  aRowGRID - [IN] fetch 대상 row의 GRID
 **********************************************************************/
IDE_RC smiTableCursor::setRowPosition( void   * aRow,
                                       scGRID   aRowGRID )
{
    UInt  sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader*)mTable );

    if ( sTableType == SMI_TABLE_DISK )
    {
        IDE_ERROR( SC_GRID_IS_NOT_NULL(aRowGRID) );
        
        SC_COPY_GRID( aRowGRID, mIterator->mRowGRID );
    }
    else
    {
        mIterator->curRecPtr = (SChar*) aRow;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 조건을 만족하는 Table의 Record 갯수를 구한다.
 *
 * aRow       - [IN]  if DRDB, aRow에 Buffer를 할당해서 넘겨주어야 한다.
 * aRowCount  - [OUT] Iterator에 설정된 조건을 만족하는 Record갯수를 구한다.
 **********************************************************************/
IDE_RC smiTableCursor::countRow( const void ** aRow,
                                 SLong *       aRowCount )
{
    *aRowCount = 0;

    while(1)
    {
        IDE_DASSERT( checkVisibilityConsistency() == IDE_SUCCESS );

        IDE_TEST( mSeek[SMI_FIND_NEXT]( mIterator, aRow )
                  != IDE_SUCCESS );
        if( *aRow == NULL )
        {
            break;
        }

        // To Fix PR-8113
        // 연산자 우선 순위 오류 수정함.
        (*aRowCount)++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    init();

    IDE_POP();

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : aValue에 해당하는 Row를 Table에 Insert한다.
 * Implementation :
 *    mOps.insertRowFunc에 일반 insert는 normalInsertRow() 함수가
 *    direct-path insert는 dpathInsertRow() 함수가 연결되어 있음
 *    - Input
 *      aValueList : insert 할 value
 *      aFlag      : flag 정보
 *    - Output
 *      aRow       : insert 된 record pointer
 *      aGRID      : insert 된 record GRID
 **********************************************************************/
IDE_RC smiTableCursor::insertRow( const smiValue  * aValueList,
                                  void           ** aRow,
                                  scGRID          * aGRID )
{
    IDU_FIT_POINT( "smiTableCursor::insertRow" );

    IDE_ERROR( this != NULL );
    IDE_ERROR( aValueList != NULL );

    IDE_TEST( mOps.insertRowFunc(this,
                                 aValueList,
                                 aRow,
                                 aGRID)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 일반 Insert Row 함수
 * Implementation :
 *    cursor를 통해 insert를 수행함
 *
 *    - Input
 *      aValueList : insert 할 value
 *      aFlag      : flag 정보
 *    - Output
 *      aRow       : insert 된 record pointer
 *      aGRID      : insert 된 record GRID
 **********************************************************************/
IDE_RC smiTableCursor::normalInsertRow(smiTableCursor  * aCursor,
                                       const smiValue  * aValueList,
                                       void           ** aRow,
                                       scGRID          * aGRID)
{
    UInt             sTableType;
    UInt             sFlag   = 0;
    SChar          * sRecPtr = NULL;
    scGRID           sRowGRID;
    smcTableHeader * sTable;
    SChar          * sNullPtr;
    UInt             sIndexCnt;

    
    sTable     = (smcTableHeader*)aCursor->mTable;
    sIndexCnt  = smcTable::getIndexCount(sTable);
    sTableType = SMI_GET_TABLE_TYPE( sTable );

    if( aCursor->mCursorProp.mIsUndoLogging == ID_TRUE )
    {
        sFlag = SM_FLAG_INSERT_LOB;
    }
    else
    {
        sFlag = SM_FLAG_INSERT_LOB_UNDO;
    }

    if( aCursor->mNeedUndoRec == ID_TRUE )
    {
        sFlag |= SM_INSERT_NEED_INSERT_UNDOREC_OK;
    }
    else
    {
        sFlag |= SM_INSERT_NEED_INSERT_UNDOREC_NO;
    }

    // mInfinite는 smiStatement에서 세팅된다.
    IDE_TEST( aCursor->mInsert( aCursor->mCursorProp.mStatistics,
                                aCursor->mTrans,
                                aCursor->mTableInfo,
                                aCursor->mTable,
                                aCursor->mInfinite,
                                &sRecPtr,
                                &sRowGRID,
                                aValueList,
                                sFlag )
              != IDE_SUCCESS );

    // PROJ-1705
    if( sIndexCnt > 0 )
    {
        if( sTableType == SMI_TABLE_DISK )
        {
            IDE_TEST( insertKeyIntoIndicesForInsert( aCursor,
                                                     sRowGRID,
                                                     aValueList )
                      != IDE_SUCCESS );
        }

        /* BUG-31417
         * when insert after row trigger on memory table is called,
         * can not reference new inserted values of INSERT SELECT statement.
         */
        else if( (sTableType == SMI_TABLE_MEMORY) ||
                 (sTableType == SMI_TABLE_META) )
        {
            IDE_ERROR( smmManager::getOIDPtr( sTable->mSpaceID,
                                              sTable->mNullOID, 
                                              (void**)&sNullPtr)
                       == IDE_SUCCESS );

            IDE_TEST( insertKeyIntoIndices( aCursor,
                                            sRecPtr,
                                            SC_NULL_GRID,
                                            sNullPtr,
                                            NULL )
                      != IDE_SUCCESS );

            IDE_TEST( setAgingIndexFlag( aCursor, 
                                         sRecPtr, 
                                         SM_OID_ACT_AGING_INDEX )
                      != IDE_SUCCESS );
        }
        else 
        {
            IDE_ERROR( sTableType == SMI_TABLE_VOLATILE );
            
            IDE_ERROR( smmManager::getOIDPtr( sTable->mSpaceID,
                                              sTable->mNullOID, 
                                              (void**)&sNullPtr)
                       == IDE_SUCCESS );

            IDE_TEST( insertKeyIntoIndices( aCursor,
                                            sRecPtr,
                                            SC_NULL_GRID,
                                            sNullPtr,
                                            NULL )
                      != IDE_SUCCESS );

            IDE_TEST( setAgingIndexFlag( aCursor, 
                                         sRecPtr, 
                                         SM_OID_ACT_AGING_INDEX )
                      != IDE_SUCCESS );

        }
    }

    if ( aCursor->mCursorType != SMI_COMPOSITE_CURSOR )
    {
        if( sTableType == SMI_TABLE_DISK )
        {
            aCursor->mLstInsRowGRID = sRowGRID;
        }
        else
        {
            aCursor->mLstInsRowPtr  = sRecPtr;
        }
    }
    else
    {
        // nothing to do
    }

    if( sTableType == SMI_TABLE_DISK )
    {
        SC_COPY_GRID( sRowGRID, /* Source GRID */
                      *aGRID ); /* Dest   GRID */
    }
    else
    {
        *aRow = sRecPtr;
        SC_COPY_GRID( sRowGRID, *aGRID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aCursor->init();

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Direct-Path Insert Row 함수
 *
 * Implementation :
 *      (1) DPath Insert가 동일 TX 내에서 한번도 초기화 되지 않았으면,
 *          초기화 부터 수행한다.
 *      (2) DPath Insert를 수행한다.
 *
 * Parameters
 *      aCursor     - [IN] insert 수행할 cursor
 *      aValueList  - [IN] insert 할 value
 ******************************************************************************/
IDE_RC smiTableCursor::dpathInsertRow(smiTableCursor  * aCursor,
                                      const smiValue  * aValueList,
                                      void           ** /*aRow*/,
                                      scGRID          * aGRID)
{
    idvSQL            * sStatistics;
    sdrMtxStartInfo     sStartInfo;
    smOID               sTableOID;
    scGRID              sRowGRID;

    IDE_ASSERT( SMI_TABLE_TYPE_IS_DISK( (smcTableHeader*)aCursor->mTable ) == ID_TRUE );

    IDU_FIT_POINT( "2.PROJ-1665@smiTableCursor::dpathInsertRow" );

    sStatistics = aCursor->mCursorProp.mStatistics;

    // TSS 할당
    sStartInfo.mTrans   = aCursor->mTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    IDE_TEST( smxTrans::allocTXSegEntry( sStatistics,
                                         &sStartInfo )
              != IDE_SUCCESS );

    // DPathEntry를 할당받아서 Transaction에 달아둔다.
    IDE_TEST( smxTrans::allocDPathEntry( aCursor->mTrans )
              != IDE_SUCCESS );

    // DPathSegInfo를 받아온다.
    if( aCursor->mDPathSegInfo == NULL )
    {
        sTableOID = smcTable::getTableOID( aCursor->mTable );
        IDE_TEST( sdcDPathInsertMgr::allocDPathSegInfo(
                                                sStatistics,
                                                aCursor->mTrans,
                                                sTableOID,
                                                &aCursor->mDPathSegInfo )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // insert를 수행
    //------------------------------------------
    IDE_TEST( sdcRow::insertAppend( sStatistics,
                                    aCursor->mTrans,
                                    aCursor->mDPathSegInfo,
                                    aCursor->mTable,
                                    aCursor->mInfinite,
                                    aValueList,
                                    &sRowGRID )
              != IDE_SUCCESS );

    
    //------------------------------------------
    // iterator 증가
    //------------------------------------------
    SC_COPY_GRID( sRowGRID, aCursor->mLstInsRowGRID );

    SC_COPY_GRID( sRowGRID, *aGRID );
    
    IDU_FIT_POINT( "1.PROJ-1665@smiTableCursor::dpathInsertRow");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2264
IDE_RC smiTableCursor::insertRowWithIgnoreUniqueError( smiTableCursor  * aCursor,
                                                       smcTableHeader  * aTableHeader,
                                                       smiValue        * aValueList,
                                                       smOID           * aOID,
                                                       void           ** aRow )
{
    UInt              sTableType;
    SChar           * sRowPtr = NULL;
    scGRID            sRowGRID;
    SChar           * sNullPtr;
    UInt              sIndexCnt;
    smxSavepoint    * sISavepoint = NULL;
    scPageID          sPageID;
    smOID             sOID;
    SChar           * sExistUniqueRow;
    idBool            sUniqueOccurrence = ID_FALSE;
    smLSN             sLsnNTA;
    smSCN             sSCN;
    const smiColumn * sColumn;
    UInt              sState = 0;

    IDE_ERROR( aCursor      != NULL );
    IDE_ERROR( aTableHeader != NULL );
    // BUG-36718
    IDE_ERROR( aRow != NULL );

    sIndexCnt  = smcTable::getIndexCount( aTableHeader );
    sTableType = SMI_GET_TABLE_TYPE( aTableHeader );
    sColumn    = smcTable::getColumn(aTableHeader, 0);

    // BUG-39378
    // setImpSavepoint4Retry함수를 이용해 ImpSavepoint를 설정 한다.
    IDE_TEST( smxTrans::setImpSavepoint4Retry( aCursor->mTrans,
                                               (void**)&sISavepoint ) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smlLockMgr::lockTableModeIX( aCursor->mTrans,
                                           SMC_TABLE_LOCK( aTableHeader ) )
              != IDE_SUCCESS );

    sLsnNTA = smxTrans::getTransLstUndoNxtLSN( aCursor->mTrans );

    IDE_TEST( smcRecord::insertVersion( NULL,   //aStatistics
                                        aCursor->mTrans,
                                        aCursor->mTableInfo,
                                        aTableHeader,
                                        aCursor->mInfinite,
                                        &sRowPtr,
                                        NULL,   //aRetInsertSlotGRID
                                        aValueList,
                                        (SM_FLAG_INSERT_LOB |
                                         SM_INSERT_NEED_INSERT_UNDOREC_OK) )
              != IDE_SUCCESS );

    sPageID = SMP_SLOT_GET_PID( sRowPtr );
    sOID    = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sRowPtr ) );

    if( sIndexCnt > 0 )
    {
        if( sTableType == SMI_TABLE_MEMORY )
        {
            SC_MAKE_NULL_GRID( sRowGRID );

            IDE_ERROR( smmManager::getOIDPtr( aTableHeader->mSpaceID,
                                              aTableHeader->mNullOID, 
                                              (void**)&sNullPtr)
                       == IDE_SUCCESS );

            if( insertKeyIntoIndices( aCursor,
                                      sRowPtr,
                                      sRowGRID,
                                      sNullPtr,
                                      &sExistUniqueRow ) != IDE_SUCCESS )
            {
                if( ideGetErrorCode() == smERR_ABORT_smnUniqueViolation )
                {
                    SMX_INC_SESSION_STATISTIC( aCursor->mTrans,
                                               IDV_STAT_INDEX_UPDATE_RETRY_COUNT,
                                               1 );

                    smcTable::incStatAtABort( aTableHeader,
                                              SMC_INC_UNIQUE_VIOLATION_CNT );

                    sUniqueOccurrence = ID_TRUE;

                    IDE_TEST( aCursor->mTrans->abortToImpSavepoint( sISavepoint )
                              != IDE_SUCCESS );
                }
                else
                {
                    // if others error
                    IDE_RAISE( IDE_EXCEPTION_END_LABEL );
                }
            }
            else
            {
                // if insert index success

                // PROJ-2429 Dictionary based data compress for on-disk DB
                // disk dictionary compression column인 경우 dictionary table에 
                // 삽입을 성공했으면 해당 record가 rollback되면 안된다. 
                // 따라서 rollback시 aging되지 않도록 막고 다른 Tx가 해당 record를 
                // 볼 수있게 하기 위해 record의 createSCN을 0으로 설정 한다.
                if ( (sColumn->flag & SMI_COLUMN_COMPRESSION_TARGET_MASK)
                     == SMI_COLUMN_COMPRESSION_TARGET_DISK  )
                {
                    IDE_TEST( setAgingIndexFlag( aCursor, 
                                                 sRowPtr, 
                                                 SM_OID_ACT_COMPRESSION )
                              != IDE_SUCCESS );

                    SM_INIT_SCN( &sSCN );
                    IDE_TEST(smcRecord::setSCNLogging( aCursor->mTrans,
                                                       aTableHeader,
                                                       (SChar*)sRowPtr,
                                                       sSCN)
                             != IDE_SUCCESS);

                    IDE_TEST( smrLogMgr::writeNTALogRec(NULL, /* idvSQL* */
                                                        aCursor->mTrans,
                                                        &sLsnNTA)
                            != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( setAgingIndexFlag( aCursor, 
                                                 sRowPtr, 
                                                 SM_OID_ACT_AGING_INDEX )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            // if not memory db
            IDE_ASSERT(0);
        }
    }

    sState = 0;
    IDE_TEST( aCursor->mTrans->unsetImpSavepoint( sISavepoint )
              != IDE_SUCCESS );

    if( sUniqueOccurrence == ID_TRUE )
    {
        *aOID = SM_MAKE_OID( SMP_SLOT_GET_PID(sExistUniqueRow),
                             SMP_SLOT_GET_OFFSET((smpSlotHeader*)sExistUniqueRow) );

        *aRow = sExistUniqueRow;
    }
    else
    {
        *aOID = sOID;
        *aRow = sRowPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-39378 ImpSavepoint is set twice in a smiStatment 
       when inserting a data to dictionary table */
    if ( sState == 1 )
    {
        IDE_ASSERT( aCursor->mTrans->unsetImpSavepoint( sISavepoint )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 마지막으로 Insert된 Record ID를 가져온다.
 **********************************************************************/
scGRID  smiTableCursor::getLastInsertedRID( )
{
    IDE_DASSERT( mCursorType == SMI_INSERT_CURSOR );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL( mLstInsRowGRID ) );

    return mLstInsRowGRID;
}

/***********************************************************************
 * Description : 현재 Cursor의 Iterator가 가리키는 Row를 update한다.
 *
 * aValueList - [IN] 업데이할 Column의 Value List
 * aRetryInfo - [IN] retry 정보
 **********************************************************************/
IDE_RC smiTableCursor::updateRow( const smiValue        * aValueList,
                                  const smiDMLRetryInfo * aRetryInfo )
{
    IDE_ERROR_RAISE( this      != NULL, ERR_CLOSED );
    IDE_ERROR_RAISE( mIterator != NULL, ERR_CLOSED );

    prepareRetryInfo( &aRetryInfo );

    IDU_FIT_POINT( "smiTableCursor::updateRow" );

    IDE_TEST( mOps.updateRowFunc( this,
                                  aValueList,
                                  aRetryInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLOSED );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smiCursorNotOpened ) );
    }
    IDE_EXCEPTION_END;

    if( ideGetErrorCode() != smERR_RETRY_Row_Retry )
    {
        /* Row Retry는 cursor가 close되지 않고,
         * 마지막 row만 retry하는것이다..
         * Cursor가 계속 사용되므로 init()하면 안된다. */
        init();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BUG-39399
 *              현재 Cursor의 Iterator가 가리키는 Row를
 *              내가 이미 update 했는지 여부를 확인.
 *
 * aValueList - [IN]  업데이할 Column의 Value List
 * aRetryInfo - [IN]  retry 정보
 * aRow       - [OUT] update 된 row의 pointer
 * aGRID      - [OUT] update 된 row의 GRID
 * aIsUpdatedByMe   - [OUT] ID_TRUE 이면, aRow가 이미 같은 statement에서
 *                          내가 이미 update 한 row이다.
 **********************************************************************/
IDE_RC smiTableCursor::isUpdatedRowBySameStmt( idBool   * aIsUpdatedBySameStmt )
{
    smiTableCursor    * sCursor     = NULL;
    UInt                sTableType  = 0;

    IDE_ERROR_RAISE( this      != NULL, ERR_CLOSED );
    IDE_ERROR_RAISE( mIterator != NULL, ERR_CLOSED );
    IDE_ERROR_RAISE( aIsUpdatedBySameStmt != NULL, ERR_CLOSED );

    sCursor = (smiTableCursor *)this;

    sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader *)(sCursor->mTable) );

    switch( sTableType )
    {
        case SMI_TABLE_MEMORY :
            IDE_TEST( smcRecord::isUpdatedVersionBySameStmt(
                                            sCursor->mTrans,
                                            sCursor->mSCN,
                                            (smcTableHeader *)sCursor->mTable,
                                            sCursor->mIterator->curRecPtr,
                                            sCursor->mInfinite,
                                            aIsUpdatedBySameStmt,
                                            sCursor->mStatement->isForbiddenToRetry() )
                      != IDE_SUCCESS );

            break;

        case SMI_TABLE_DISK :
            IDE_TEST( sdcRow::isUpdatedRowBySameStmt(
                                            sCursor->mIterator->properties->mStatistics,
                                            sCursor->mTrans,
                                            sCursor->mSCN,
                                            sCursor->mTable,
                                            sCursor->mIterator->mRowGRID,
                                            sCursor->mInfinite,
                                            aIsUpdatedBySameStmt )
                      != IDE_SUCCESS );

            break;

        case SMI_TABLE_VOLATILE :
            IDE_TEST( svcRecord::isUpdatedVersionBySameStmt(
                                            sCursor->mTrans,
                                            sCursor->mSCN,
                                            (smcTableHeader *)sCursor->mTable,
                                            sCursor->mIterator->curRecPtr,
                                            sCursor->mInfinite,
                                            aIsUpdatedBySameStmt,
                                            sCursor->mStatement->isForbiddenToRetry() )
                      != IDE_SUCCESS );

            break;

        default :
            break;
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLOSED );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smiCursorNotOpened ) );
    }
    IDE_EXCEPTION_END;

    if( ideGetErrorCode() != smERR_RETRY_Row_Retry )
    {
        /* Row Retry는 cursor가 close되지 않고,
         * 마지막 row만 retry하는것이다..
         * Cursor가 계속 사용되므로 init()하면 안된다. */
        init();
    }

    return IDE_FAILURE;
}

//PROJ-1362 Internal LOB
/***********************************************************************
 * Description : 현재 Cursor의 Iterator가 가리키는 Row를 update한다.
 *
 * aValueList - [IN]  업데이할 Column의 Value List
 * aRetryInfo - [IN]  retry 정보
 * aRow       - [OUT] update 된 row의 pointer
 * aGRID      - [OUT] update 된 row의 GRID
 **********************************************************************/
IDE_RC smiTableCursor::updateRow(const smiValue        * aValueList,
                                 const smiDMLRetryInfo * aRetryInfo,
                                 void                 ** aRow,
                                 scGRID                * aGRID )
{
    IDE_ERROR_RAISE( this      != NULL, ERR_CLOSED );
    IDE_ERROR_RAISE( mIterator != NULL, ERR_CLOSED );

    prepareRetryInfo( &aRetryInfo );

    IDE_TEST( mOps.updateRowFunc( this,
                                  aValueList,
                                  aRetryInfo )
              != IDE_SUCCESS );

    // mIterator의 curRecPtr, RowRID가 설정되어 있다.
    if( aRow != NULL)
    {
        *aRow = mIterator->curRecPtr;
    }

    if( aGRID != NULL)
    {
        SC_COPY_GRID( mIterator->mRowGRID, /* Source GRID */
                      *aGRID );            /* Dest   GRID */

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLOSED );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smiCursorNotOpened ) );
    }
    IDE_EXCEPTION_END;

    if( ideGetErrorCode() != smERR_RETRY_Row_Retry )
    {
        /* Row Retry는 cursor가 close되지 않고,
         * 마지막 row만 retry하는것이다..
         * Cursor가 계속 사용되므로 init()하면 안된다. */
        init();
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 현재 Cursor의 Iterator가 가리키는 Row를 삭제한다.
 *
 * aRetryInfo - [IN] retry 정보
 **********************************************************************/
IDE_RC smiTableCursor::deleteRow( const smiDMLRetryInfo * aRetryInfo )
{
    IDE_ERROR_RAISE( this      != NULL, ERR_CLOSED );
    IDE_ERROR_RAISE( mIterator != NULL, ERR_CLOSED );

    prepareRetryInfo( &aRetryInfo );

    IDU_FIT_POINT( "smiTableCursor::deleteRow" );

    IDE_TEST( mOps.deleteRowFunc( this,
                                  aRetryInfo ) != IDE_SUCCESS );

    mIterator->curRecPtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLOSED );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smiCursorNotOpened ) );
    }
    IDE_EXCEPTION_END;

    if( ideGetErrorCode() != smERR_RETRY_Row_Retry )
    {
        /* Row Retry는 cursor가 close되지 않고,
         * 마지막 row만 retry하는것이다..
         * Cursor가 계속 사용되므로 init()하면 안된다. */
        init();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 마지막으로 update한 row의 최신 버전을 가져온다.
 *
 *   aRow     - [OUT] 최신 버전의 row
 *   aRowGRID - [OUT] disk GRID
 **********************************************************************/
IDE_RC smiTableCursor::getLastRow( const void ** aRow,
                                   scGRID      * aRowGRID )
{
    UInt               sTableType;
    idBool             sTrySuccess;
    scGRID             sRowGRID;
    UChar             *sRow;
    idBool             sIsRowDeleted;
    idBool             sIsPageLatchReleased = ID_TRUE;

    IDE_ERROR( this != NULL );
    IDE_ERROR( (mCursorType == SMI_INSERT_CURSOR) ||
               (mCursorType == SMI_DELETE_CURSOR) ||
               (mCursorType == SMI_UPDATE_CURSOR) ||
               (mCursorType == SMI_COMPOSITE_CURSOR) );

    sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader*)mTable );

    IDE_ERROR( (sTableType == SMI_TABLE_DISK)   ||
               (sTableType == SMI_TABLE_MEMORY) ||
               (sTableType == SMI_TABLE_VOLATILE) );

    if( sTableType == SMI_TABLE_DISK )
    {
        if( mCursorType == SMI_INSERT_CURSOR )
        {
            SC_COPY_GRID( mLstInsRowGRID, sRowGRID );
        }
        else
        {
            IDE_ERROR( mIterator != NULL );
            SC_COPY_GRID( mIterator->mRowGRID, sRowGRID );
        }

        IDE_TEST( sdbBufferMgr::getPageByGRID( mCursorProp.mStatistics,
                                               sRowGRID,
                                               SDB_S_LATCH,
                                               SDB_WAIT_NORMAL,
                                               NULL, /* aMtx */
                                               (UChar**)&sRow,
                                               &sTrySuccess )
                  != IDE_SUCCESS );
        sIsPageLatchReleased = ID_FALSE;

        IDE_TEST( sdcRow::fetch(
                      mCursorProp.mStatistics,
                      NULL, /* aMtx */
                      NULL, /* aSP */
                      mTrans,
                      SC_MAKE_SPACE(sRowGRID),
                      (UChar*)sRow,
                      ID_TRUE, /* aIsPersSlot */
                      SDB_SINGLE_PAGE_READ,
                      mCursorProp.mFetchColumnList,
                      SMI_FETCH_VERSION_LAST,
                      SD_NULL_SID, /* aMyTSSlotSID */
                      NULL,        /* aMyStmtSCN */
                      NULL,        /* aInfiniteSCN */
                      NULL,        /* aIndexInfo4Fetch */
                      NULL,        /* aLobInfo4Fetch */
                      ((smcTableHeader*)mTable)->mRowTemplate,
                      (UChar*)*aRow,
                      &sIsRowDeleted,
                      &sIsPageLatchReleased )
                  != IDE_SUCCESS );

        /* BUG-23319
         * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
        /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
         * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
         * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
         * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
         * 상황에 따라 적절한 처리를 해야 한다. */
        if( sIsPageLatchReleased == ID_FALSE )
        {
            sIsPageLatchReleased = ID_TRUE;
            IDE_TEST( sdbBufferMgr::releasePage( mCursorProp.mStatistics,
                                                 sRow )
                      != IDE_SUCCESS );
        }
        *aRowGRID = sRowGRID;
    }
    else
    {
        /* BUG-22282: TC/Server/mm4/Project/PROJ-1518/iloader/AfterTrigger에
         * 비정상 종료합니다.
         *
         * Cursor가 Insert일경우 mIterator가 Null이고 마지막 Insert한 Row는
         * mLstInsRowPtr에 저장되어 있다.
         * */
        if( mCursorType == SMI_INSERT_CURSOR )
        {
            *aRow = mLstInsRowPtr;
        }
        else
        {
            IDE_ERROR( mIterator != NULL );
            // 마지막으로 update, insert한 row
            *aRow = mIterator->curRecPtr;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( mCursorProp.mStatistics,
                                               sRow )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 **********************************************************************/
IDE_RC smiTableCursor::getLastModifiedRow( void ** aRowBuf,
                                           UInt    aLength )
{
    UInt               sTableType;
    idBool             sTrySuccess;
    scGRID             sRowGRID;
    UChar             *sRow;
    idBool             sIsRowDeleted;
    SChar             *sLstModifiedRow;
    idBool             sIsPageLatchReleased = ID_TRUE;

    IDE_ERROR( this    != NULL );
    IDE_ERROR( aRowBuf != NULL );
    IDE_ERROR( aLength > 0 );
    IDE_ERROR( (mCursorType == SMI_INSERT_CURSOR) ||
               (mCursorType == SMI_DELETE_CURSOR) ||
               (mCursorType == SMI_UPDATE_CURSOR) ||
               (mCursorType == SMI_COMPOSITE_CURSOR) );

    sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader*)mTable );

    IDE_ERROR( (sTableType == SMI_TABLE_DISK)   ||
               (sTableType == SMI_TABLE_MEMORY) ||
               (sTableType == SMI_TABLE_VOLATILE) );

    if( sTableType == SMI_TABLE_DISK )
    {
        if( mCursorType == SMI_INSERT_CURSOR )
        {
            SC_COPY_GRID( mLstInsRowGRID, sRowGRID );
        }
        else
        {
            IDE_ERROR( mIterator != NULL );
            SC_COPY_GRID( mIterator->mRowGRID, sRowGRID );
        }

        IDE_TEST( sdbBufferMgr::getPageByGRID( mCursorProp.mStatistics,
                                               sRowGRID,
                                               SDB_S_LATCH,
                                               SDB_WAIT_NORMAL,
                                               NULL, /* aMtx */
                                               (UChar**)&sRow,
                                               &sTrySuccess )
                  != IDE_SUCCESS );
        sIsPageLatchReleased = ID_FALSE;

        IDE_TEST( sdcRow::fetch(
                      mCursorProp.mStatistics,
                      NULL, /* aMtx */
                      NULL, /* aSP */
                      mTrans,
                      SC_MAKE_SPACE(sRowGRID),
                      (UChar*)sRow,
                      ID_TRUE, /* aIsPersSlot */
                      SDB_SINGLE_PAGE_READ,
                      mCursorProp.mFetchColumnList,
                      SMI_FETCH_VERSION_LAST,
                      SD_NULL_SID, /* aMyTSSlotSID */
                      NULL,        /* aMyStmtSCN */
                      NULL,        /* aInfiniteSCN */
                      NULL,        /* aIndexInfo4Fetch */
                      NULL,        /* aLobInfo4Fetch */
                      ((smcTableHeader*)mTable)->mRowTemplate,
                      (UChar*)*aRowBuf,
                      &sIsRowDeleted,
                      &sIsPageLatchReleased )
                  != IDE_SUCCESS );

        /* BUG-23319
         * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
        /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
         * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
         * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
         * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
         * 상황에 따라 적절한 처리를 해야 한다. */
        if( sIsPageLatchReleased == ID_FALSE )
        {
            sIsPageLatchReleased = ID_TRUE;
            IDE_TEST( sdbBufferMgr::releasePage( mCursorProp.mStatistics,
                                                 sRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* BUG-22282: TC/Server/mm4/Project/PROJ-1518/iloader/AfterTrigger에
         * 비정상 종료합니다.
         *
         * Cursor가 Insert일경우 mIterator가 Null이고 마지막 Insert한 Row는
         * mLstInsRowPtr에 저장되어 있다.
         * */
        if( mCursorType == SMI_INSERT_CURSOR )
        {
            sLstModifiedRow = mLstInsRowPtr;
        }
        else
        {
            IDE_ERROR( mIterator != NULL );
            sLstModifiedRow = mIterator->curRecPtr;
        }

        if( sTableType == SMI_TABLE_MEMORY )
        {
            IDE_ERROR( aLength <= ((smcTableHeader*)mTable)->mFixed.mMRDB.mSlotSize );
        }
        else
        {
            // BUG-27869 VOLATILE 테이블 스페이스에서 트리거를 사용하면
            //           ASSERT 로 죽습니다.
            // Volatile Table에 대한 내용이 없습니다. 추가합니다.
            IDE_DASSERT( sTableType == SMI_TABLE_VOLATILE );

            IDE_ERROR( aLength <= ((smcTableHeader*)mTable)->mFixed.mVRDB.mSlotSize );
        }
        /* PROJ-2375 Global Meta
         * 버퍼에 복사하지 않고 포인터만 참조한다. 
         */ 
        *aRowBuf = sLstModifiedRow;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( mCursorProp.mStatistics,
                                               sRow )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 **********************************************************************/
IDE_RC smiTableCursor::getModifiedRow( void   ** aRowBuf,
                                       UInt      aLength,
                                       void    * aRow,
                                       scGRID    aRowGRID )
{
    UInt               sTableType;
    idBool             sTrySuccess;
    UChar             *sRow;
    idBool             sIsRowDeleted;
    idBool             sIsPageLatchReleased = ID_TRUE;

    IDE_ERROR( this    != NULL );
    IDE_ERROR( aRowBuf != NULL );
    IDE_ERROR( aLength > 0 );
    IDE_ERROR( mCursorType == SMI_COMPOSITE_CURSOR );

    sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader*)mTable );

    IDE_ERROR( (sTableType == SMI_TABLE_DISK)   ||
               (sTableType == SMI_TABLE_MEMORY) ||
               (sTableType == SMI_TABLE_VOLATILE) );

    if( sTableType == SMI_TABLE_DISK )
    {
        IDE_TEST( sdbBufferMgr::getPageByGRID( mCursorProp.mStatistics,
                                               aRowGRID,
                                               SDB_S_LATCH,
                                               SDB_WAIT_NORMAL,
                                               NULL, /* aMtx */
                                               (UChar**)&sRow,
                                               &sTrySuccess )
                  != IDE_SUCCESS );
        sIsPageLatchReleased = ID_FALSE;

        IDE_TEST( sdcRow::fetch(
                      mCursorProp.mStatistics,
                      NULL, /* aMtx */
                      NULL, /* aSP */
                      mTrans,
                      SC_MAKE_SPACE(aRowGRID),
                      (UChar*)sRow,
                      ID_TRUE, /* aIsPersSlot */
                      SDB_SINGLE_PAGE_READ,
                      mCursorProp.mFetchColumnList,
                      SMI_FETCH_VERSION_LAST,
                      SD_NULL_SID, /* aMyTSSSID */
                      NULL,        /* aMyStmtSCN */
                      NULL,        /* aInfiniteSCN */
                      NULL,        /* aIndexInfo4Fetch */
                      NULL,        /* aLobInfo4Fetch */
                      ((smcTableHeader*)mTable)->mRowTemplate,
                      (UChar*)*aRowBuf,
                      &sIsRowDeleted,
                      &sIsPageLatchReleased)
                  != IDE_SUCCESS );

        /* BUG-23319
         * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
        /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
         * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
         * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
         * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
         * 상황에 따라 적절한 처리를 해야 한다. */
        if( sIsPageLatchReleased == ID_FALSE )
        {
            sIsPageLatchReleased = ID_TRUE;
            IDE_TEST( sdbBufferMgr::releasePage( mCursorProp.mStatistics,
                                                 sRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        if( sTableType == SMI_TABLE_MEMORY )
        {
            IDE_ERROR( aLength <= ((smcTableHeader*)mTable)->mFixed.mMRDB.mSlotSize );
        }
        else
        {
            // BUG-27869 VOLATILE 테이블 스페이스에서 트리거를 사용하면
            //           ASSERT 로 죽습니다.
            // Volatile Table에 대한 내용이 없습니다. 추가합니다.
            IDE_DASSERT( sTableType == SMI_TABLE_VOLATILE );

            IDE_ERROR( aLength <= ((smcTableHeader*)mTable)->mFixed.mVRDB.mSlotSize );
        }
        
        /* PROJ-2375 Global Meta
         * 버퍼에 복사하지 않고 포인터만 참조한다. 
         */
        *aRowBuf = aRow;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( mCursorProp.mStatistics,
                                               sRow )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Update는 Column단위로 수행된다. 때문에 Update시 Update되는
 *               Column이외의 Column의 Index가 걸려 있는 경우에는 Index에
 *               대한 Update가 불필요하다. 때문에 여기서는 전체 Index중에서
 *               Update시 Update가 필요한 Index만을 찾아서 넘겨준다.
 *
 * aIndexOID     - [IN] Index Header를 가지고 있는 Variable Column OID
 * aIndexLength  - [IN] 위 Varcolumn의 길이
 * aCols         - [IN] Update되는 Column List
 * aModifyIdxBit - [IN-OUT] Update해야도는 Index를 Bit를 이용해 표시해서
 *                 넘겨준다.
 *                 ex) 110000000000 이면 첫번째 Index, 두번째 Index가
 *                     Update대상이 된다.
 **********************************************************************/
void smiTableCursor::makeUpdatedIndexList( smcTableHeader       * aTableHeader,
                                           const smiColumnList  * aCols,
                                           ULong                * aModifyIdxBit,
                                           ULong                * aCheckUniqueIdxBit )
{

    UInt                  i;
    UInt                  j;
    UInt                  sIndexCnt;
    smnIndexHeader      * sIndexCursor;
    const smiColumnList * sColumn;
    ULong               * sBytePtr            = aModifyIdxBit;
    ULong               * sCheckUniqueBytePtr = aCheckUniqueIdxBit;
    ULong                 sBitMask            = ((ULong)1 << 63);
    idBool                sDoModify;

    *aModifyIdxBit      = (ULong)0;
    *aCheckUniqueIdxBit = (ULong)0;

    sIndexCnt =  smcTable::getIndexCount(aTableHeader);

    // 영향받는 인덱스에만 key insert
    for( i = 0; i < sIndexCnt;i++ )
    {
        sIndexCursor = (smnIndexHeader*) smcTable::getTableIndex(aTableHeader,i);
        
        sDoModify = ID_FALSE;
        
        if( aCols != NULL )
        {
            for( j = 0; j < sIndexCursor->mColumnCount; j++ )
            {
                sColumn = aCols;
                while( sColumn != NULL )
                {
                    if( sIndexCursor->mColumns[j] == sColumn->column->id )
                    {
                        sDoModify = ID_TRUE;
                        break;
                    }//if sIndexCursor
                    sColumn = sColumn->next;
                }//while
                if(sColumn != NULL) // found same column
                {
                    break;
                }//if
            }//for j
        }
        else
        {
            sDoModify = ID_TRUE;
        }
        
        if( sDoModify == ID_TRUE ) // found same column
        {
            // Update해야되는 Index 추가.
            *sBytePtr |= sBitMask;
            
            if( ((sIndexCursor->mFlag & SMI_INDEX_UNIQUE_MASK) ==
                 SMI_INDEX_UNIQUE_ENABLE) ||
                ((sIndexCursor->mFlag & SMI_INDEX_LOCAL_UNIQUE_MASK) ==
                 SMI_INDEX_LOCAL_UNIQUE_ENABLE) )
            {
                *sCheckUniqueBytePtr |= sBitMask;

            }
        }//if
        
        sBitMask = sBitMask >> 1;
    }//for i

}

/***********************************************************************
 * Description :
 **********************************************************************/
IDE_RC smiTableCursor::insertKeysWithUndoSID( smiTableCursor * aCursor )
{
    UChar          * sPage;
    UChar          * sSlotDirPtr;
    idBool           sTrySuccess;
    UChar          * sURHdr;
    SInt             sSlotCount;
    SInt             i;
    scGRID           sRowGRID;
    SInt             sStartSlotSeq;
    idBool           sFixPage = ID_FALSE;
    SInt             sIndexCnt;
    ULong          * sBytePtr;
    ULong            sBitMask;
    smcTableHeader * sTable;
    sdcUndoRecType   sType;
    sdcUndoRecFlag   sFlag;
    smOID            sTableOID;
    sdpSegMgmtOp   * sSegMgmtOp;
    sdpSegInfo       sSegInfo;
    sdpExtInfo       sExtInfo;
    scPageID         sSegPID;
    sdRID            sCurExtRID;
    scPageID         sCurPageID;

    IDE_ERROR( aCursor->mFstUndoRecSID != SD_NULL_SID );
    IDE_ERROR( aCursor->mFstUndoExtRID != SD_NULL_RID );
    IDE_ERROR( aCursor->mLstUndoRecSID != SD_NULL_SID );

    IDV_SQL_OPTIME_BEGIN(
        aCursor->mCursorProp.mStatistics,
        IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER );

    sTable = (smcTableHeader*)aCursor->mTable;

    sIndexCnt = smcTable::getIndexCount(sTable);

    IDE_TEST_CONT( sIndexCnt == 0, SKIP_INSERT_KEY );

    /* 삽입할 Index가 있는지 체크 */
    sBytePtr = &aCursor->mModifyIdxBit;
    sBitMask = ((ULong)1 << 63);

    for( i = 0; i < sIndexCnt; i++ )
    {
        if( ((*sBytePtr) & sBitMask) != 0 )
        {
            break;
        }
        sBitMask = sBitMask >> 1;
    }

    IDE_TEST_CONT( i == sIndexCnt, SKIP_INSERT_KEY );

    sSegMgmtOp =
        sdpSegDescMgr::getSegMgmtOp( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    sSegPID =
        smxTrans::getUDSegPtr(aCursor->mTrans)->getSegPID();
    sCurExtRID = aCursor->mFstUndoExtRID;

    IDE_ASSERT( sSegMgmtOp->mGetSegInfo(
                    NULL,
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                    sSegPID,
                    NULL, /* aTableHeader */
                    &sSegInfo ) == IDE_SUCCESS );

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       sCurExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    sCurPageID    = SD_MAKE_PID( aCursor->mFstUndoRecSID );
    sStartSlotSeq = SD_MAKE_SLOTNUM( aCursor->mFstUndoRecSID );

    while( sCurPageID != SD_NULL_PID )
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( aCursor->mCursorProp.mStatistics,
                                              SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                              sCurPageID,
                                              (UChar**)&sPage,
                                              &sTrySuccess )
                  != IDE_SUCCESS );
        sFixPage = ID_TRUE;

        IDE_ASSERT(((sdpPhyPageHdr*)sPage)->mPageType == SDP_PAGE_UNDO );

        IDE_ASSERT( sCurPageID >= sExtInfo.mFstDataPID  );

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sPage);
        sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

        for( i = sStartSlotSeq; i < sSlotCount; i++ )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                               i,
                                                               &sURHdr )
                      != IDE_SUCCESS );

            SDC_GET_UNDOREC_HDR_1B_FIELD( sURHdr,
                                          SDC_UNDOREC_HDR_FLAG,
                                          &sFlag );
            SDC_GET_UNDOREC_HDR_FIELD( sURHdr,
                                       SDC_UNDOREC_HDR_TABLEOID,
                                       &sTableOID );

            // Undo Record의 TableOID가 SM_NULL_OID 이면 관계없는
            // Type이므로 skip 한다.
            if ( sTableOID != sTable->mSelfOID )
            {
                // 다른 커서가 작성한 Undo Record 이므로 Skip 한다
                continue;
            }

            if ( SDC_UNDOREC_FLAG_IS_UNDO_FOR_HEAD_ROWPIECE(sFlag)
                 != ID_TRUE )
            {
                continue;
            }

            if ( SDC_UNDOREC_FLAG_IS_UNDO_FOR_LOB_UPDATE(sFlag)
                 == ID_TRUE )
            {
                continue;
            }

            if ( SDC_UNDOREC_FLAG_IS_VALID(sFlag)
                 != ID_TRUE )
            {
                continue;
            }

            SDC_GET_UNDOREC_HDR_1B_FIELD( sURHdr,
                                          SDC_UNDOREC_HDR_TYPE,
                                          &sType );

            IDE_ERROR(
                (sType == SDC_UNDO_INSERT_ROW_PIECE)      ||
                (sType == SDC_UNDO_UPDATE_ROW_PIECE)      ||
                (sType == SDC_UNDO_OVERWRITE_ROW_PIECE)   ||
                (sType == SDC_UNDO_CHANGE_ROW_PIECE_LINK) ||
                (sType == SDC_UNDO_DELETE_ROW_PIECE)      ||
                (sType == SDC_UNDO_LOCK_ROW) );

            if( (sType == SDC_UNDO_INSERT_ROW_PIECE)      ||
                (sType == SDC_UNDO_CHANGE_ROW_PIECE_LINK) ||
                (sType == SDC_UNDO_DELETE_ROW_PIECE) )
            {
                continue;
            }

            if( sdcUndoRecord::isExplicitLockRec(sURHdr)
                == ID_TRUE )
            {
                continue;
            }

            IDE_ERROR( (sType == SDC_UNDO_UPDATE_ROW_PIECE)      ||
                       (sType == SDC_UNDO_OVERWRITE_ROW_PIECE)   ||
                       (sType == SDC_UNDO_LOCK_ROW) );

            sdcUndoRecord::parseRowGRID((UChar*)sURHdr, &sRowGRID);
            IDE_ERROR( SC_GRID_IS_NOT_NULL( sRowGRID ) );

            IDE_ERROR( smcTable::getTableSpaceID(sTable) 
                       == SC_MAKE_SPACE( sRowGRID ) );

            IDE_TEST( smiTableCursor::insertKeyIntoIndices( aCursor,
                                                            NULL, /* aRow */
                                                            sRowGRID,
                                                            NULL,
                                                            NULL )
                      != IDE_SUCCESS );

            IDE_TEST( iduCheckSessionEvent(
                         aCursor->mIterator->properties->mStatistics )
                     != IDE_SUCCESS );
        }

        sFixPage = ID_FALSE;
        IDE_TEST(sdbBufferMgr::unfixPage(
                          aCursor->mCursorProp.mStatistics,
                          sPage ) != IDE_SUCCESS);

        if( sCurPageID == SD_MAKE_PID( aCursor->mLstUndoRecSID ) )
        {
            break;
        }

        IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                                SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                &sSegInfo,
                                                NULL,
                                                &sCurExtRID,
                                                &sExtInfo,
                                                &sCurPageID )
                  != IDE_SUCCESS );
        sStartSlotSeq = 1;
    }

    IDE_EXCEPTION_CONT( SKIP_INSERT_KEY );

    IDV_SQL_OPTIME_END( aCursor->mCursorProp.mStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sFixPage == ID_TRUE )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::unfixPage(
                             aCursor->mCursorProp.mStatistics,
                             sPage ) == IDE_SUCCESS );
        IDE_POP();
    }

    IDV_SQL_OPTIME_END( aCursor->mCursorProp.mStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : MemInsert, Mem&Disk Update시 사용.
 **********************************************************************/
IDE_RC smiTableCursor::insertKeyIntoIndices( smiTableCursor * aCursor,
                                             SChar          * aRow,
                                             scGRID           aRowGRID,
                                             SChar          * aNullRow,
                                             SChar         ** aExistUniqueRow )
{
    idBool                sUniqueCheck;
    UInt                  i;
    UInt                  j;
    UInt                  sIndexCnt;
    smnIndexHeader      * sIndexCursor;
    ULong               * sCheckUniqueBytePtr;
    ULong                 sBitMask;
    ULong                 sKeyValueBuf[SD_PAGE_SIZE/ID_SIZEOF(ULong)];
    idBool                sIsRowDeleted;
    UChar               * sRow;
    idBool                sTrySuccess;
    sdSID                 sRowSID;
    idBool                sIsPageLatchReleased = ID_TRUE;
    idBool                sDiskIndex;
    idBool                sIsExistFreeKey = ID_FALSE;

    sIndexCnt = smcTable::getIndexCount(aCursor->mTable);

    sCheckUniqueBytePtr = &aCursor->mCheckUniqueIdxBit;
    sBitMask = ((ULong)1 << 63);

    if( SMI_TABLE_TYPE_IS_DISK((smcTableHeader*)aCursor->mTable)
        == ID_TRUE )
    {
        sDiskIndex = ID_TRUE;
    }
    else
    {
        sDiskIndex = ID_FALSE;
    }
    
    for(i =0 ; i < sIndexCnt; i++ )
    {
        if (( aCursor->mModifyIdxBit & sBitMask ) != 0 )
        {
            sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex(aCursor->mTable, i);
            if ( smnManager::isIndexEnabled( sIndexCursor ) == ID_TRUE )
            {
                if ( ( (*sCheckUniqueBytePtr) & sBitMask ) != 0 )
                {
                    sUniqueCheck = ID_TRUE;
                }
                else
                {
                    sUniqueCheck = ID_FALSE;
                }

                if ( sDiskIndex == ID_TRUE )
                {
                    IDE_TEST_RAISE( smnManager::getIsConsistentOfIndexHeader(
                                        (void*)sIndexCursor ) == ID_FALSE, 
                                    err_inconsistent_index );

                    IDE_TEST( sdbBufferMgr::getPageByGRID(
                                      aCursor->mCursorProp.mStatistics,
                                      aRowGRID,
                                      SDB_S_LATCH,
                                      SDB_WAIT_NORMAL,
                                      NULL, /* aMtx */
                                      (UChar**)&sRow,
                                      &sTrySuccess )
                              != IDE_SUCCESS );
                    sIsPageLatchReleased = ID_FALSE;

                    sRowSID = SD_MAKE_SID_FROM_GRID( aRowGRID );

                    IDE_TEST( sIndexCursor->mModule->mMakeKeyFromRow(
                                          aCursor->mCursorProp.mStatistics,
                                          NULL, /* aMtx */
                                          NULL, /* aSP */
                                          aCursor->mTrans,
                                          aCursor->mTable,
                                          sIndexCursor,
                                          (UChar*)sRow,
                                          SDB_SINGLE_PAGE_READ,
                                          SC_MAKE_SPACE(aRowGRID),
                                          SMI_FETCH_VERSION_LAST,
                                          SD_NULL_SID, /* aTssSID */
                                          NULL,        /* aSCN */
                                          NULL,        /* aInfiniteSCN */
                                          (UChar*)sKeyValueBuf,
                                          &sIsRowDeleted,
                                          &sIsPageLatchReleased)
                              != IDE_SUCCESS );
                    IDE_ASSERT( sIsRowDeleted == ID_FALSE );

                    /* BUG-23319
                     * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
                    /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
                     * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
                     * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
                     * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
                     * 상황에 따라 적절한 처리를 해야 한다. */
                    if ( sIsPageLatchReleased == ID_FALSE )
                    {
                        sIsPageLatchReleased = ID_TRUE;
                        IDE_TEST( sdbBufferMgr::releasePage(
                                      aCursor->mCursorProp.mStatistics,
                                      (UChar*)sRow )
                                  != IDE_SUCCESS );
                    }

                    IDE_TEST( sIndexCursor->mInsert(
                                  aCursor->mCursorProp.mStatistics,
                                  aCursor->mTrans,
                                  aCursor->mTable,
                                  sIndexCursor,
                                  aCursor->mInfinite,
                                  (SChar*)sKeyValueBuf,
                                  NULL,
                                  sUniqueCheck,
                                  aCursor->mSCN,
                                  &sRowSID,
                                  aExistUniqueRow,
                                  aCursor->mOrgCursorProp.mLockWaitMicroSec,
                                  aCursor->mStatement->isForbiddenToRetry() )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( sIndexCursor->mInsert(
                                  aCursor->mCursorProp.mStatistics,
                                  aCursor->mTrans,
                                  aCursor->mTable,
                                  sIndexCursor,
                                  aCursor->mInfinite,
                                  aRow,
                                  aNullRow,
                                  sUniqueCheck,
                                  aCursor->mSCN,
                                  &sRowSID,
                                  aExistUniqueRow,
                                  aCursor->mOrgCursorProp.mLockWaitMicroSec,
                                  aCursor->mStatement->isForbiddenToRetry() )
                              != IDE_SUCCESS );
                }
            }
        }
        sBitMask = sBitMask >> 1;
    }//for i

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_inconsistent_index );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ));
    }
    IDE_EXCEPTION_END;

    if( sDiskIndex == ID_FALSE )
    {
        /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure
         * of index aging. 
         * MemIndex에 대한 삽입이 실패하면, 아예 해당 Row에 대한 모든 Index
         * 삽입을 복구함 */

        /* i까지 Index 삽입을 시도했으니, i 직전까지의 Index에 대해
         * Loop를 돌면 됨 */
        sBitMask = ((ULong)1 << 63);
        for( j = 0 ; j < i ; j ++ )
        {
            /* BUG-47615 갱신한 Index key 만 제거한다 */
            if (( aCursor->mModifyIdxBit & sBitMask ) != 0 )
            {
                sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex(
                    aCursor->mTable, j );

                if( smnManager::isIndexEnabled( sIndexCursor ) == ID_TRUE )
                {
                    IDE_ASSERT( sIndexCursor->mModule->mFreeSlot( sIndexCursor,
                                                                  aRow,
                                                                  ID_FALSE,    /*aIgnoreNotFoundKey*/
                                                                  &sIsExistFreeKey )
                                == IDE_SUCCESS );

                    IDE_ASSERT( sIsExistFreeKey == ID_TRUE );
                }
                else
                {
                    /* nothing to do ... */
                }
            }
            sBitMask = sBitMask >> 1;
        }
    }

    IDE_PUSH();
    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aCursor->mCursorProp.mStatistics,
                                               (UChar*)sRow )
                    == IDE_SUCCESS );
    }

    if ( ideGetErrorCode() == smERR_ABORT_smnUniqueViolation)
    {
        SMX_INC_SESSION_STATISTIC( aCursor->mTrans,
                                   IDV_STAT_INDEX_UNIQUE_VIOLATION_COUNT,
                                   1 /* Increase Value */ );

        smcTable::incStatAtABort( (smcTableHeader *)aCursor->mTable, SMC_INC_UNIQUE_VIOLATION_CNT );
    }
    IDE_POP();

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Disk Insert용
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC smiTableCursor::insertKeyIntoIndices( smiTableCursor    *aCursor,
                                             scGRID             aRowGRID,
                                             const smiValue    *aValueList,
                                             idBool             aForce )
{
    idBool                sUniqueCheck;
    UInt                  i;
    UInt                  sIndexCnt;
    smnIndexHeader      * sIndexCursor;
    ULong               * sBytePtr;
    ULong                 sBitMask;
    ULong                 sKeyValueBuf[SD_PAGE_SIZE/ID_SIZEOF(ULong)];
    sdSID                 sRowSID;

    IDE_DASSERT( SMI_TABLE_TYPE_IS_DISK( (smcTableHeader*)aCursor->mTable ) == ID_TRUE );

    sIndexCnt = smcTable::getIndexCount(aCursor->mTable);

    sBytePtr = &aCursor->mModifyIdxBit;
    sBitMask = ((ULong)1 << 63);
    for(i =0 ; i < sIndexCnt; i++ )
    {
        sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex(aCursor->mTable, i);

        /* BUG-35254 - the local index is broken when executing insert or delete
         *             using composite cursor.
         * Insert인 경우 mModifyIdxBit를 확인할 필요없이 무조건 모든 인덱스를
         * 갱신해야 한다.
         * 하지만 Update인 경우 확인해야 하기 때문에 Insert인 경우 aForce가
         * ID_TRUE로 넘어오게되고, 이때에만 mModifyIdxBit를 확인하지 않고
         * 인덱스를 갱신한다. */
        if( ((sIndexCursor->mFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE) &&
            ( (aForce == ID_TRUE) ||
              (((*sBytePtr) & sBitMask) != 0) ) )
        {
            if( ((sIndexCursor->mFlag & SMI_INDEX_UNIQUE_MASK) ==
                 SMI_INDEX_UNIQUE_ENABLE) ||
                ((sIndexCursor->mFlag & SMI_INDEX_LOCAL_UNIQUE_MASK) ==
                 SMI_INDEX_LOCAL_UNIQUE_ENABLE) )
            {
                sUniqueCheck = ID_TRUE;
            }
            else
            {
                sUniqueCheck = ID_FALSE;
            }

            IDE_TEST_RAISE( smnManager::getIsConsistentOfIndexHeader(
                                              (void*)sIndexCursor ) == ID_FALSE, 
                            err_inconsistent_index );

            IDE_TEST( sIndexCursor->mModule->mMakeKeyFromSmiValue(
                                              sIndexCursor,
                                              aValueList,
                                              (UChar*)sKeyValueBuf )
                      != IDE_SUCCESS );

            sRowSID = SD_MAKE_SID_FROM_GRID( aRowGRID );
                
            IDE_TEST( sIndexCursor->mInsert( aCursor->mCursorProp.mStatistics,
                                             aCursor->mTrans,
                                             aCursor->mTable,
                                             sIndexCursor,
                                             aCursor->mInfinite,
                                             (SChar*)sKeyValueBuf,
                                             NULL, // aNull
                                             sUniqueCheck,
                                             aCursor->mSCN,
                                             &sRowSID,
                                             NULL,  //aExistUniqueRow
                                             aCursor->mOrgCursorProp.mLockWaitMicroSec,
                                             aCursor->mStatement->isForbiddenToRetry() )
                            
                      != IDE_SUCCESS );
        } //if
        sBitMask = sBitMask >> 1;
    } //for i

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_inconsistent_index);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ));
    }
    IDE_EXCEPTION_END;

    if ( ideGetErrorCode() == smERR_ABORT_smnUniqueViolation)
    {
        SMX_INC_SESSION_STATISTIC( aCursor->mTrans,
                                   IDV_STAT_INDEX_UNIQUE_VIOLATION_COUNT,
                                   1 /* Increase Value */ );

        smcTable::incStatAtABort( (smcTableHeader *)aCursor->mTable, SMC_INC_UNIQUE_VIOLATION_CNT );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 **********************************************************************/
IDE_RC smiTableCursor::deleteKeys( smiTableCursor * aCursor,
                                   scGRID           aRowGRID,
                                   idBool           aForce )
{
    ULong                 sKeyValueBuf[SD_PAGE_SIZE/ID_SIZEOF(ULong)];
    UInt                  i;
    UInt                  sIndexCnt;
    smnIndexHeader      * sIndexCursor;
    ULong               * sBytePtr;
    ULong                 sBitMask;
    smiIterator         * sIterator;
    sdrMtxStartInfo       sStartInfo;
    sdSID                 sRowSID;
    UChar               * sRow;
    idBool                sTrySuccess;
    idBool                sIsDeleted;
    idBool                sIsPageLatchReleased = ID_TRUE;

    IDE_ERROR( aCursor != NULL );

    sIndexCnt = smcTable::getIndexCount(aCursor->mTable);

    IDE_TEST_CONT( sIndexCnt == 0, RETURN_SUCCESS );

    sIterator = (smiIterator*)aCursor->mIterator;
    sBytePtr  = &aCursor->mModifyIdxBit;
    sBitMask  = ((ULong)1 << 63);

    sStartInfo.mTrans   = aCursor->mTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    IDE_TEST( smxTrans::allocTXSegEntry( aCursor->mCursorProp.mStatistics,
                                         &sStartInfo )
              != IDE_SUCCESS );

    sRowSID = SD_MAKE_SID_FROM_GRID( aRowGRID );

    for(i = 0 ; i < sIndexCnt ; i++ )
    {
        sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( aCursor->mTable, i );

        /* BUG-35254 - the local index is broken when executing insert or delete
         *             using composite cursor.
         * Delete인 경우 mModifyIdxBit를 확인할 필요없이 무조건 모든 인덱스를
         * 갱신해야 한다.
         * 하지만 Update인 경우 확인해야 하기 때문에 Delete인 경우 aForce가
         * ID_TRUE로 넘어오게되고, 이때에만 mModifyIdxBit를 확인하지 않고
         * 인덱스를 갱신한다. */
        if( ((sIndexCursor->mFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE) &&
            ( (aForce == ID_TRUE) ||
              (((*sBytePtr) & sBitMask) != 0) ) )
        {
            IDE_TEST_RAISE( smnManager::getIsConsistentOfIndexHeader(
                    (void*)sIndexCursor ) == ID_FALSE, 
                ERR_INCONSISTENT_INDEX );
            /*
             * Data Page에 Shard Latch를 획득한 상태에서 인덱스에 접근하면
             * Delayed CTS Stamping시 Latch Escalation실패로 인해서
             * Delayed CTS Stamping이 실패한다. 따라서 인덱스 연산전에 획득한
             * Latch를 제거해야 한다.
             */
            IDE_TEST( sdbBufferMgr::getPageByGRID( aCursor->mCursorProp.mStatistics,
                                                   aRowGRID,
                                                   SDB_S_LATCH,
                                                   SDB_WAIT_NORMAL,
                                                   NULL, /* aMtx */
                                                   (UChar**)&sRow,
                                                   &sTrySuccess )
                      != IDE_SUCCESS );
            sIsPageLatchReleased = ID_FALSE;

            /*
             * 삭제된 레코드로 부터 삭제전 Old Image를 구축하기 위해서
             * SMI_FETCH_VERSION_LASTPREV 로 설정한다. 
             */
            IDE_TEST( sIndexCursor->mModule->mMakeKeyFromRow(
                          sIterator->properties->mStatistics,
                          NULL, /* aMtx */
                          NULL, /* aSP */
                          aCursor->mTrans,
                          aCursor->mTable,
                          sIndexCursor,
                          (UChar*)sRow,
                          SDB_SINGLE_PAGE_READ,
                          SC_MAKE_SPACE(sIterator->mRowGRID),
                          SMI_FETCH_VERSION_LASTPREV,
                          smxTrans::getTSSlotSID( aCursor->mTrans ),
                          &aCursor->mSCN,
                          &aCursor->mInfinite,
                          (UChar*)sKeyValueBuf,
                          &sIsDeleted,
                          &sIsPageLatchReleased )
                      != IDE_SUCCESS );

            IDE_DASSERT( sIsDeleted == ID_FALSE );

            /* BUG-23319
             * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
            /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
             * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
             * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
             * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
             * 상황에 따라 적절한 처리를 해야 한다. */
            if( sIsPageLatchReleased == ID_FALSE )
            {
                sIsPageLatchReleased = ID_TRUE;
                IDE_TEST( sdbBufferMgr::releasePage( aCursor->mCursorProp.mStatistics,
                                                     (UChar*)sRow )
                          != IDE_SUCCESS );
            }

            IDE_TEST( sIndexCursor->mModule->mDelete( aCursor->mCursorProp.mStatistics,
                                                      aCursor->mTrans,
                                                      sIndexCursor,
                                                      (SChar*)sKeyValueBuf,
                                                      sIterator,
                                                      sRowSID )
                      != IDE_SUCCESS );
        }

        sBitMask = sBitMask >> 1;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INCONSISTENT_INDEX);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ));
    }
    IDE_EXCEPTION_END;

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( aCursor->mCursorProp.mStatistics,
                                               (UChar*)sRow )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/****************************************/
/* PROJ-1509 : Foreign Key Revisit      */
/****************************************/

IDE_RC smiTableCursor::beforeFirstModified( UInt aFlag )
{
    IDE_ERROR( this != NULL );

    IDE_TEST( mOps.beforeFirstModifiedFunc( this, aFlag )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTableCursor::readOldRow( const void ** aRow,
                                   scGRID      * aRowGRID )
{
    IDE_ERROR( this != NULL );

    IDE_TEST( mOps.readOldRowFunc( this,
                                   aRow,
                                   aRowGRID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTableCursor::readNewRow( const void ** aRow,
                                   scGRID      * aRowGRID )
{
    IDE_ERROR( this != NULL );

    IDE_TEST( mOps.readNewRowFunc( this,
                                   aRow,
                                   aRowGRID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :
 **********************************************************************/
IDE_RC smiTableCursor::openMRVRDB( smiTableCursor *    aCursor,
                                   const void*         aTable,
                                   smSCN               aSCN,
                                   const smiRange *    aKeyRange,
                                   const smiRange *    aKeyFilter,
                                   const smiCallBack * aRowFilter,
                                   smlLockNode *       aCurLockNodePtr,
                                   smlLockSlot *       aCurLockSlotPtr)
{
    smSCN        sSCN;
    smSCN        sCreateSCN;
    smTID        sDummyTID;
    smxTrans   * sTrans = aCursor->mTrans;

    /* Cursor를 열기 전에 Statement의 ViewSCN과 Table의 CreateSCN을 비교 해서
     * Table이 Modify 되었는지 여부를 확인한다.*/
    if ( ( ((smcTableHeader*)(aCursor->mTable))->mFlag & SMI_TABLE_PRIVATE_VOLATILE_MASK )
         == SMI_TABLE_PRIVATE_VOLATILE_FALSE )
    {
        if ( sTrans->mIsGCTx == ID_TRUE )
        {
            SM_SET_SCN( &sCreateSCN, &(((smpSlotHeader *)aTable)->mCreateSCN) );

            if ( SM_SCN_IS_INFINITE( sCreateSCN ) &&
                 /* BUG-48244 : 내(TX)가 2PC한 TX는 Pending 완료시까지 대기할 필요없다. */
                 ( SMP_GET_TID( sCreateSCN ) != smxTrans::getTransID( sTrans ) ) )
            {
                /* PROJ-2733 
                 * commit되지 않은 테이블에 접근할수도 있다. */
                IDE_TEST( smxTrans::waitPendingTx( sTrans,
                                                   sCreateSCN,
                                                   aCursor->mSCN )
                          != IDE_SUCCESS );
            }
        }

        IDE_DASSERT( SM_SCN_IS_FREE_ROW( ((smpSlotHeader*)aTable)->mLimitSCN ) == ID_TRUE );

        /* BUG-48244 */
        SM_SET_SCN( &sCreateSCN, &(((smpSlotHeader *)aTable)->mCreateSCN) );
        SMX_GET_SCN_AND_TID( sCreateSCN, sSCN, sDummyTID );

        IDE_TEST_RAISE( ( SM_SCN_IS_EQ(&sSCN, &aSCN) != ID_TRUE
                          && SM_SCN_IS_NOT_INIT(aSCN) ) ||
                        ( SM_SCN_IS_GT(&sSCN, &aCursor->mSCN)  &&
                          SM_SCN_IS_NOT_INFINITE(sSCN) ),
                        ERR_MODIFIED );
    }
    else
    {
        /* SMI_TABLE_PRIVATE_VOLATILE_TRUE == Temporary Table */
        /* BUG-33982, BUG-47606 Temporary Table일 경우 모두 볼 수 있어야 합니다.
         * Temporary Table 일 경우 Transaction 별로 독립적, 독점적으로 접근, 사용함.
         * 해당 Transaction 진행 도중에 Temporary Table를 생성해서 사용하고 Commit 시에 정리됨,
         * 해당 Transaction의 먼저 시작 된 Statement에서 Temporary Table에 접근 가능해야 함.
         * 그래서 Temporary Table에 한하여 CreateSCN을 확인하지 않음 */
    }

    //---------------------------------
    // PROJ-1509
    // 트랜잭션의 oid list 중에서 cursor open 당시 마지막 oid node 정보 설정
    //---------------------------------

    aCursor->mFstOidNode =
        aCursor->mTrans->getCurNodeOfNVL();

    if( aCursor->mFstOidNode != NULL )
    {
        aCursor->mFstOidCount = ((smxOIDNode*)aCursor->mFstOidNode)->mOIDCnt;
    }

    if(aCursor->mIndex != NULL)
    {
        IDE_TEST_RAISE((((smnIndexHeader*)aCursor->mIndex)->mFlag &
                        SMI_INDEX_USE_MASK)
                       != SMI_INDEX_USE_ENABLE, err_disabled_index);
    }

    IDE_TEST( ((smnIndexModule*)aCursor->mIndexModule)->mInitIterator( aCursor->mIterator,
                                                                       aCursor->mTrans,
                                                                       (void*)aCursor->mTable,
                                                                       (void*)aCursor->mIndex,
                                                                       aCursor->mDumpObject,
                                                                       aKeyRange,
                                                                       aKeyFilter,
                                                                       aRowFilter,
                                                                       aCursor->mFlag,
                                                                       aCursor->mSCN,
                                                                       aCursor->mInfinite,
                                                                       aCursor->mUntouchable,
                                                                       &aCursor->mCursorProp,
                                                                       &aCursor->mSeekFunc,
                                                                       aCursor->mStatement )
              != IDE_SUCCESS );

    if( aCursor->mUntouchable == ID_FALSE )
    {
        // 일단 모든 인덱스에 key insert하도록 해둠
        // inplace update인 경우에 다시 구함.
        makeUpdatedIndexList( ((smcTableHeader*)aCursor->mTable),
                              aCursor->mColumns,
                              &aCursor->mModifyIdxBit,
                              &aCursor->mCheckUniqueIdxBit );
        
        /* 메모리는 모든 인덱스에 키 삽입하도록 해야 한다.
         * ID_ULONG_MAX = 0xFFFFFFFFFFFFFFFF = set all bit 1 */
        aCursor->mModifyIdxBit = ID_ULONG_MAX;
    }
    aCursor->mUpdateInplaceEscalationPoint = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_disabled_index );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_DISABLED_INDEX) );
    }
    IDE_EXCEPTION( ERR_MODIFIED );
    {
        if( aCurLockSlotPtr != NULL )
        {
            (void)smlLockMgr::unlockTable( aCursor->mTrans->mSlotN,
                                           aCurLockNodePtr,
                                           aCurLockSlotPtr );
        }

        if( aCursor->mStatement->isForbiddenToRetry() )
        {
            SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];
            idlOS::snprintf( sMsgBuf,
                             SMI_MAX_ERR_MSG_LEN,
                             "[TABLE VALIDATION] "
                             "TableOID:%"ID_vULONG_FMT", "
                             "CursorSCN:%"ID_UINT64_FMT", "
                             "TableSCN:%"ID_UINT64_FMT", %"ID_UINT64_FMT,
                             ((smcTableHeader*)(aCursor->mTable))->mSelfOID,
                             SM_SCN_TO_LONG(aCursor->mSCN),
                             SM_SCN_TO_LONG(sSCN),
                             SM_SCN_TO_LONG(aSCN) );

            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) );

            IDE_ERRLOG( IDE_SD_19 );
        }
        else
        {
            IDE_SET( ideSetErrorCode(smERR_REBUILD_smiTableModified) );
        }
  
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : MRDB, VRDB용 Cursor를 Close한다. 이때 Close시 Cursor가 Change연산(
 *               insert, delete, update)을 수행했다면 Transaction의 OID List
 *               에 close시에 index에 insert해야되는 row가 있는지 보고 있으면
 *               insert한다. insert, update는 cursor close시에 index에 row를
 *               insert한다.
 *
 * aCursor - [IN] Close할 Cursor.
 **********************************************************************/
IDE_RC smiTableCursor::closeMRVRDB( smiTableCursor * aCursor )
{
    smxTrans*       sTrans;
    smcTableHeader* sTable;
    smcTableHeader* sTableOfCursor;
    smxOIDNode*     sNode;
    smxOIDInfo*     sNodeFence;
    smxOIDInfo*     sNodeCursor;
    SChar*          sNullPtr;
    smxOIDNode*     sOIDHeadNodePtr;
    UInt            sIndexCnt;
    UInt            sState       = 1;
    UInt            sFstOidIndex = 0;
    scGRID          sRowGRID;
    SChar*          sRowPtrOfCursor;

    sTrans        = aCursor->mTrans;

    /* update inplace 로 escalation 된 경우 idx bit 값이 달라진 것을 초기화. 
     * escaltion 이전 부분을 처리하기 위해서.
     * ID_ULONG_MAX = 0xFFFFFFFFFFFFFFFF = set all bit 1 */
    aCursor->mModifyIdxBit = ID_ULONG_MAX;

    /*
     * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
     * 수행할 필요가 없습니다.
     */
    if( (smiStatement::getSkipCursorClose( aCursor->mStatement ) == ID_FALSE) &&
        ( aCursor->mUntouchable == ID_FALSE) )
    {
        sOIDHeadNodePtr = &(sTrans->mOIDList->mOIDNodeListHead);

        if( aCursor->mFstOidNode == NULL )
        {
            /*
              Cursor가 Open될때 Transaction OID List에 하나도 Node가 없었다.
              때문에 시작위치부터 조사해야 된다.
            */
            aCursor->mFstOidNode  = sOIDHeadNodePtr->mNxtNode;
            aCursor->mFstOidCount = 0;
        }

        /*
          aCursor->mOidNode는 Cursor가 Open될때 Transaction OID List의 마지막
          Node를 가리키고 있다.
        */
        sTable = (smcTableHeader*)aCursor->mTable;
        sIndexCnt =  smcTable::getIndexCount(sTable);

        /* BUG-31417
         * when insert after row trigger on memory table is called,
         * can not reference new inserted values of INSERT SELECT statement.
         *
         * normalInsertRow 함수에서 Insert직후 Key를 Index에 넣도록 수정하여
         * Insert Cursor Close시에는 Key를 Index에 넣지 않는다.
         */
        if( (aCursor->mFstOidNode != sOIDHeadNodePtr) &&
            (sIndexCnt  != 0) &&
            (aCursor->mCursorType != SMI_INSERT_CURSOR) )
        {
            IDE_ERROR( smmManager::getOIDPtr( sTable->mSpaceID,
                                              sTable->mNullOID, 
                                              (void**)&sNullPtr)
                       == IDE_SUCCESS );
            sNode         = (smxOIDNode*)aCursor->mFstOidNode;
            sFstOidIndex  = aCursor->mFstOidCount;

            do
            {
                IDE_ASSERT(sNode->mOIDCnt >= sFstOidIndex);
                sNodeCursor = sNode->mArrOIDInfo + sFstOidIndex;
                sNodeFence  = sNode->mArrOIDInfo + sNode->mOIDCnt;

                for( ; sNodeCursor != sNodeFence; sNodeCursor++ )
                {
                    /*
                      현재 OID List의 Table OID와 Cursor가 open한 Table이 같고,
                      Index Insert를 해야한다면 Insert수행
                    */
                    IDE_ASSERT( smcTable::getTableHeaderFromOID( sNodeCursor->mTableOID,
                                                                 (void**)&sTableOfCursor )
                                == IDE_SUCCESS );
                    if( ( sTableOfCursor == sTable ) &&
                        ( sNodeCursor->mFlag & SM_OID_ACT_CURSOR_INDEX ) )
                    {
                        SC_MAKE_NULL_GRID( sRowGRID );
                        IDE_ASSERT( smmManager::getOIDPtr( 
                                        sNodeCursor->mSpaceID,
                                        sNodeCursor->mTargetOID, 
                                        (void**)&sRowPtrOfCursor)
                                    == IDE_SUCCESS );
                        /* 
                            PROJ-2416
                            inplace update한 row부터는  필수 인덱스만 삽입 하도록 해서 성능 개선
                            ID_ULONG_MAX로 초기화되어있던 mModifyidxBit 를 새로 세팅한다.
                        */
                        if ( aCursor->mUpdateInplaceEscalationPoint == sRowPtrOfCursor )
                        {
                            makeUpdatedIndexList( ((smcTableHeader*)aCursor->mTable),
                                                  aCursor->mColumns,
                                                  &aCursor->mModifyIdxBit,
                                                  &aCursor->mCheckUniqueIdxBit );
                        }
                        else
                        {
                            /* do nothing */
                        }

                        if ( aCursor->mModifyIdxBit != 0 )
                        {
                            IDE_TEST( smiTableCursor::insertKeyIntoIndices(
                                          aCursor,
                                          (SChar *)sRowPtrOfCursor,
                                          sRowGRID,
                                          sNullPtr,
                                          NULL )
                                      != IDE_SUCCESS );
                            sNodeCursor->mFlag |= SM_OID_ACT_AGING_INDEX;
                        }
                        else
                        {
                            /* do nothing */
                        }
                    }
                }

                sNode = sNode->mNxtNode;
                sFstOidIndex = 0;

                /*
                  중간에 Session이 끊어졌는지 Check해서 끊어졌으면
                  현재 Transaction을 Abort시킨다.
                */
                IDE_TEST(iduCheckSessionEvent( aCursor->mCursorProp.mStatistics )
                         != IDE_SUCCESS);
            }
            while( sNode != sOIDHeadNodePtr );
        }

        /*---------------------------------
         * PROJ-1509
         * Cursor close 당시 트랜잭션의 마지막 oid node 정보 설정
         *---------------------------------*/
        aCursor->mLstOidNode = sOIDHeadNodePtr->mPrvNode;

        aCursor->mLstOidCount =
            ((smxOIDNode*)(aCursor->mLstOidNode))->mOIDCnt;
    }

    sState = 0;
    IDE_TEST( ((smnIndexModule*)aCursor->mIndexModule)->mDestIterator(
                  aCursor->mIterator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        /* BUG-22442: smiTableCursor::close에서 error발생시 Iterator에 대해
         *            Destroy를 하지 않고 있습니다. */
        IDE_ASSERT( ((smnIndexModule*)aCursor->mIndexModule)->mDestIterator(
                        aCursor->mIterator )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * TASK-5030
 * Description : Full XLog를 위해 update 할 column list와 value list
 *      를 수정한다.
 *
 * aCursor              - [IN] table cursor
 * aValueList           - [IN] original value list
 * aUpdateColumnList    - [IN] sorted column list of target table
 * aUpdateColumnCount   - [IN] column count of target table
 * aOldValueBuffer      - [IN] buffer for old value
 * aNewColumnList       - [OUT] new column list
 * aNewValueList        - [OUT] new value list
 **********************************************************************/
IDE_RC smiTableCursor::makeColumnNValueListMRDB(
                            smiTableCursor      * aCursor,
                            const smiValue      * aValueList,
                            smiUpdateColumnList * aUpdateColumnList,
                            UInt                  aUpdateColumnCount,
                            SChar               * aOldValueBuffer,
                            smiColumnList       * aNewColumnList,
                            smiValue            * aNewValueList)
{
    UInt                i                   = 0;
    UInt                j                   = 0;
    UInt                sNewColumnPosition  = 0;
    UInt                sOldColumnLength    = 0;
    UInt                sRemainedReadSize   = 0;
    UInt                sReadPosition       = 0;
    UInt                sReadSize           = 0;
    UInt                sOldBufferOffset    = 0;
    idBool              sExistNewValue  = ID_FALSE;
    const smiColumn   * sColumn;
    void              * sOldColumnValue;
    SChar             * sOldFixRowPtr   = aCursor->mIterator->curRecPtr;

    /* new value가 있는경우 -> new value를 그대로 사용
     * new value없고 Lob type이 아닌경우 -> old value를 읽어와 사용
     * new value없고 Lob type인 경우 -> 무시 */
    for( i = 0, j = 0, sNewColumnPosition = 0 ;
         i < smcTable::getColumnCount(aCursor->mTable) ;
         i++ )
    {
        sColumn = smcTable::getColumn(aCursor->mTable, i);

        /* compare old column with new column */
        if( j < aUpdateColumnCount )
        {
            if ( sColumn->id == (aUpdateColumnList + j)->column->id )
            {
                sExistNewValue = ID_TRUE;
            }
            else
            {
                sExistNewValue = ID_FALSE;
            }
        }
        else
        {
            sExistNewValue = ID_FALSE;
        }

        /* set column list and value list */
        if ( sExistNewValue == ID_TRUE )
        {
            /* new value가 있으면 */
            (aNewColumnList + sNewColumnPosition)->column =
                                (aUpdateColumnList + j)->column;
            (aNewColumnList + sNewColumnPosition)->next   =
                                (aNewColumnList + (sNewColumnPosition + 1));

            (aNewValueList + sNewColumnPosition)->length  =
                (aValueList + ((aUpdateColumnList + j)->position) )->length;
            (aNewValueList + sNewColumnPosition)->value   =
                (aValueList + ((aUpdateColumnList + j)->position) )->value;

            j++;
            sNewColumnPosition++;
        }
        else
        {
            /* new value가 없으면 */
            if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
                != SMI_COLUMN_COMPRESSION_TRUE )
            {
                sOldColumnLength = smcRecord::getColumnLen( sColumn, sOldFixRowPtr );

                /* smiTableBakcup::appendRow() 참고해서 작성하였음 */
                switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                {
                    case SMI_COLUMN_TYPE_LOB:
                        /* Lob type은 제외 */
                        continue;

                    case SMI_COLUMN_TYPE_VARIABLE:
                    case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                        sRemainedReadSize = sOldColumnLength;
                        sReadPosition     = 0;

                        while( sRemainedReadSize > 0 )
                        {
                            if( sRemainedReadSize < SMP_VC_PIECE_MAX_SIZE )
                            {
                                sReadSize = sRemainedReadSize;
                            }
                            else
                            {
                                sReadSize = SMP_VC_PIECE_MAX_SIZE;
                            }

                            sOldColumnValue  = smcRecord::getVarRow( sOldFixRowPtr,
                                                                     sColumn,
                                                                     sReadPosition,  
                                                                     &sReadSize, 
                                                                     (SChar*)(aOldValueBuffer + sOldBufferOffset),
                                                                     ID_FALSE );

                            IDE_ERROR( sOldColumnValue != NULL );

                            /* value 시작지점을 저장 */
                            if( sReadPosition == 0)
                            {
                                (aNewValueList + sNewColumnPosition)->value = sOldColumnValue;
                            }
                            else
                            {
                                /* do nothging */
                            }

                            sRemainedReadSize   -= sReadSize;
                            sReadPosition       += sReadSize;
                            sOldBufferOffset    += sReadSize;
                        }

                        break;

                    case SMI_COLUMN_TYPE_FIXED:
                        sOldColumnValue  = sOldFixRowPtr + sColumn->offset;

                        (aNewValueList + sNewColumnPosition)->value   = sOldColumnValue;

                        break;

                    default:
                        /* Column은 Variable 이거나 Fixed 이어야 한다. */
                        IDE_ERROR_MSG( 0,
                                       "sColumn->id    :%"ID_UINT32_FMT"\n"
                                       "sColumn->flag  :%"ID_UINT32_FMT"\n"
                                       "sColumn->offset:%"ID_UINT32_FMT"\n"
                                       "sColumn->vcInOu:%"ID_UINT32_FMT"\n"
                                       "sColumn->size  :%"ID_UINT32_FMT"\n"
                                       "sColType       :%"ID_UINT32_FMT"\n",
                                       sColumn->id,
                                       sColumn->flag,
                                       sColumn->offset,
                                       sColumn->vcInOutBaseSize,
                                       sColumn->size,
                                       sColumn->flag & SMI_COLUMN_TYPE_MASK );
                        break;
                }
            }
            else
            {
                /* BUG-39478 supplimental log에서 update 할 때
                 * compressed column에 대한 고려를 하지 않습니다. */

                sOldColumnValue  = sOldFixRowPtr + sColumn->offset;

                (aNewValueList + sNewColumnPosition)->value = sOldColumnValue;
                sOldColumnLength  = ID_SIZEOF(smOID);
            }

            (aNewColumnList + sNewColumnPosition)->column = sColumn;
            (aNewColumnList + sNewColumnPosition)->next   = (aNewColumnList + (sNewColumnPosition + 1));

            (aNewValueList + sNewColumnPosition)->length  = sOldColumnLength;

            if( sOldColumnLength != 0 )
            {
                IDE_ASSERT( (aNewValueList + sNewColumnPosition)->value != NULL );
            }
            else
            {
                /* length가 0이면 value는 NULL로 설정 */
                (aNewValueList + sNewColumnPosition)->value   = NULL;
            }

            sNewColumnPosition++;
        }
    } /* end of for */

    (aNewColumnList + (sNewColumnPosition -1))->next = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * TASK-5030
 * Description : Full XLog를 위해 필요한 메모리를 할당받고, 새로운 
 *      column list와 value list를 만들 준비를 한다.
 *      before value 중 LOB type은 제외한다.
 *
 * aCursor          - [IN] table cursor
 * aValueList       - [IN] original value list
 * aNewColumnList   - [OUT] new column list
 * aNewValueList    - [OUT] new value list
 * aIsReadBeforeImg - [OUT] before value를 읽는지, 안 읽는지.
 *                          ID_TRUE의 경우, before를 읽었음. 
 *                          메모리 해제 필요
 **********************************************************************/
IDE_RC smiTableCursor::makeFullUpdateMRDB( smiTableCursor  * aCursor,
                                           const smiValue  * aValueList,
                                           smiColumnList  ** aNewColumnList,
                                           smiValue       ** aNewValueList,
                                           SChar          ** aOldValueBuffer,
                                           idBool          * aIsReadBeforeImg )
{
    UInt                    sState              = 0;
    UInt                    sColumnCount        = 0;
    UInt                    sUpdateColumnCount  = 0;
    UInt                    i                   = 0;
    UInt                    j                   = 0;
    UInt                    sOldValueBufferSize = 0;
    UInt                    sOldColumnLength    = 0;
    idBool                  sExistNewValue      = ID_FALSE;
    UInt                    sOldValueCnt        = 0;
    smiColumnList         * sNewColumnList      = NULL;
    smiValue              * sNewValueList       = NULL;
    SChar                 * sOldValueBuffer     = NULL;
    const smiColumn       * sColumn             = NULL;
    const smiColumnList   * sCurrColumnList     = NULL;
    smiUpdateColumnList   * sUpdateColumnList   = NULL;
    SChar                 * sOldFixRowPtr       = aCursor->mIterator->curRecPtr;


    // 테이블의 총 컬럼 수
    sColumnCount = smcTable::getColumnCount( aCursor->mTable );

    // new value의 수, 즉 update 연산이 수행 될 column의 수 계산
    sUpdateColumnCount  = 0;
    sCurrColumnList     = aCursor->mColumns;

    while( sCurrColumnList != NULL )
    {
        sUpdateColumnCount++;
        sCurrColumnList = sCurrColumnList->next;
    }

    IDE_ERROR( sUpdateColumnCount != 0 );

    IDU_FIT_POINT( "smiTableCursor::makeFullUpdateMRDB::calloc" );

    // new value column을 정렬하기 위해 메모리 할당
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMI,
                                 sUpdateColumnCount,
                                 ID_SIZEOF(smiUpdateColumnList),
                                 (void**)&sUpdateColumnList )
              != IDE_SUCCESS );

    // 정렬
    sCurrColumnList = aCursor->mColumns;
    for ( i = 0 ; i < sUpdateColumnCount ; i++ )
    {
        (sUpdateColumnList + i)->column     = sCurrColumnList->column;
        (sUpdateColumnList + i)->position   = i;

        sCurrColumnList = sCurrColumnList->next;
    }

    idlOS::qsort( sUpdateColumnList,
                  sUpdateColumnCount,
                  ID_SIZEOF(smiUpdateColumnList),
                  gCompareSmiUpdateColumnListByColId );

    // before column을 저장할 버퍼의 크기를 구함. (lob은 제외)
    for( i = 0, j = 0 ; i < sColumnCount ; i++ )
    {
        sColumn = smcTable::getColumn(aCursor->mTable, i);

        if( j < sUpdateColumnCount )
        {
            /* after image에서 해당 컬럼이 있는지 검사 */
            if( sColumn->id == (sUpdateColumnList + j)->column->id )
            {
                j++;

                sExistNewValue = ID_TRUE;
            }
            else
            {
                sExistNewValue = ID_FALSE;
            }
        }
        else
        {
            sExistNewValue = ID_FALSE;
        }

        /* old value에서 받아와야 할 경우 */
        if( sExistNewValue == ID_FALSE )
        {
            /* BUG-40650 suppliment log에서
             * old value 의 사용 유무를 확인합니다. */
            sOldValueCnt++;

            /* BUG-39478 supplimental log에서 update 할 때
             * compressed column에 대한 고려를 하지 않습니다.
             *
             * old value buffer는 variable column에 대해서만 사용됩니다. */
            if ( ( ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_VARIABLE ) ||
                   ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
                &&
                ( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
                  == SMI_COLUMN_COMPRESSION_FALSE ) )
            {
                sOldColumnLength     = smcRecord::getColumnLen( sColumn,
                                                                sOldFixRowPtr );
                sOldValueBufferSize += sOldColumnLength;
            }
        }
        else
        {
            /* do nothging */
        }

    } // end of 1st for loop

    if( sOldValueCnt != 0 )
    {
        // new column list alloc
        /* smiTableCursor_makeFullUpdateMRDB_calloc_NewColumnList.tc */
        IDU_FIT_POINT("smiTableCursor::makeFullUpdateMRDB::calloc::NewColumnList");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMI,
                                     sColumnCount,
                                     ID_SIZEOF(smiColumnList),
                                     (void**) &sNewColumnList )
                  != IDE_SUCCESS);
        sState = 1;

        // new value list alloc
        /* smiTableCursor_makeFullUpdateMRDB_calloc_NewValueList.tc */
        IDU_FIT_POINT("smiTableCursor::makeFullUpdateMRDB::calloc::NewValueList");
        IDE_TEST( iduMemMgr::calloc ( IDU_MEM_SM_SMI,
                                      sColumnCount,
                                      ID_SIZEOF(smiValue),
                                      (void**) &sNewValueList )
                  != IDE_SUCCESS );
        sState = 2;

        /* BUG-40650 임시 버퍼를 사용하는 경우에만 할당한다. */
        if( sOldValueBufferSize != 0 )
        {
            // before column value 저장용 버퍼
            /* smiTableCursor_makeFullUpdateMRDB_calloc_OldValueBuffer.tc */
            IDU_FIT_POINT("smiTableCursor::makeFullUpdateMRDB::calloc::OldValueBuffer");
            IDE_TEST( iduMemMgr::calloc ( IDU_MEM_SM_SMI,
                                          sOldValueBufferSize,
                                          ID_SIZEOF(SChar),
                                          (void**) &sOldValueBuffer )
                      != IDE_SUCCESS );
            sState = 3;
        }

        // set new columnlist and valuelist 
        IDE_TEST( makeColumnNValueListMRDB( aCursor,
                                            aValueList,
                                            sUpdateColumnList,
                                            sUpdateColumnCount,
                                            sOldValueBuffer,
                                            sNewColumnList,
                                            sNewValueList)
                  != IDE_SUCCESS );

        (*aNewColumnList)   = sNewColumnList;
        (*aNewValueList)    = sNewValueList;
        (*aOldValueBuffer)  = sOldValueBuffer;
        (*aIsReadBeforeImg) = ID_TRUE;
    }
    else
    {
        // before column value를 가져올 필요가 없는 경우
        (*aNewColumnList)   = NULL;
        (*aNewValueList)    = NULL;
        (*aOldValueBuffer)  = NULL;
        (*aIsReadBeforeImg) = ID_FALSE;
    }

    IDE_ERROR( sUpdateColumnList != NULL );

    IDE_TEST( iduMemMgr::free( sUpdateColumnList )
              != IDE_SUCCESS );
    sUpdateColumnList = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            (void) iduMemMgr::free( sOldValueBuffer );

        case 2:
            (void) iduMemMgr::free( sNewValueList );

        case 1:
            (void) iduMemMgr::free( sNewColumnList );

        default:
            break;
    }

    if( sUpdateColumnList != NULL )
    {
        (void) iduMemMgr::free( sUpdateColumnList );
    }
    
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : aCursor가 가리키는 Row를 aValueList로 갱신한다.
 *
 * aCursor     - [IN] Table Cursor
 * aValueList  - [IN] Update Value List
 * aRetryInfo  - [IN] retry 정보
 **********************************************************************/
IDE_RC smiTableCursor::updateRowMRVRDB( smiTableCursor       *  aCursor,
                                        const smiValue       *  aValueList,
                                        const smiDMLRetryInfo* aRetryInfo )
{
    UInt                    sLockEscMemSize;
    UInt                    sTableType;
    smTableCursorUpdateFunc sUpdateInpFunc;
    idvSQL                * sStatistics;
    ULong                   sUpdateMaxLogSize;
    idBool                  sIsUpdateInplace    = ID_FALSE;
    idBool                  sIsPrivateVol;

    smxTrans              * sTrans              = aCursor->mTrans;

    smiColumnList         * sNewColumnList      = NULL;
    smiValue              * sNewValueList       = NULL;
    SChar                 * sOldValueBuffer     = NULL;
    idBool                  sIsReadBeforeImg    = ID_FALSE;
    UInt                    sState              = 0;

    IDE_ERROR( aCursor->mIterator != NULL );
    IDE_TEST_RAISE( aCursor->mIterator->curRecPtr == NULL,
                    ERR_NO_SELECTED_ROW );

    /* TASK-5030 타 DBMS와 연동을 위한 ALA 기능 추가 */
    if ( smcTable::isSupplementalTable( (smcTableHeader*)(aCursor->mTable) ) == ID_TRUE )
    {
        IDE_TEST( makeFullUpdateMRDB( aCursor,
                                      aValueList,
                                      &sNewColumnList,
                                      &sNewValueList,
                                      &sOldValueBuffer,
                                      &sIsReadBeforeImg )
                  != IDE_SUCCESS );
        sState = 1;

        if( sIsReadBeforeImg == ID_FALSE )
        {
            /* before value를 읽어올 필요가 없는 경우 */
            sNewColumnList  = (smiColumnList *)aCursor->mColumns;
            sNewValueList   = (smiValue *)aValueList;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sNewColumnList  = (smiColumnList *)aCursor->mColumns;
        sNewValueList   = (smiValue *)aValueList;
    }

    sLockEscMemSize = smuProperty::getLockEscMemSize();

    sTableType = SMI_GET_TABLE_TYPE( (smcTableHeader*)(aCursor->mTable) );

    /* PROJ-1407 Temporary Table
     * User temp table일 경우 inplace로 update한다. */
    sIsPrivateVol = (( (((smcTableHeader*)(aCursor->mTable))->mFlag)
                       & SMI_TABLE_PRIVATE_VOLATILE_MASK )
                     == SMI_TABLE_PRIVATE_VOLATILE_TRUE ) ? ID_TRUE : ID_FALSE ;

    if(sTableType == SMI_TABLE_VOLATILE)
    {
        sUpdateInpFunc = (smTableCursorUpdateFunc)svcRecord::updateInplace;
    }
    else
    {
        sUpdateInpFunc = (smTableCursorUpdateFunc)smcRecord::updateInplace;
    }

    IDE_TEST_CONT( (sTrans->mFlag & SMI_TRANS_INPLACE_UPDATE_MASK)
                    == SMI_TRANS_INPLACE_UPDATE_DISABLE,
                    CONT_INPLACE_UPDATE_DISABLE );

    if(aCursor->mUpdate != sUpdateInpFunc)
    {
        if((aCursor->mIsSoloCursor == ID_TRUE) && (sTableType != SMI_TABLE_META))
        {
            // PRJ-1476.
            if(smuProperty::getTableLockEnable() == 1)
            {
                // BUG-11725
                if((sTrans->mUpdateSize > sLockEscMemSize) &&
                   (sLockEscMemSize > 0))
                {
                    sIsUpdateInplace = ID_TRUE;
                }

                if(aCursor->mLockMode == SML_XLOCK)
                {
                    sIsUpdateInplace = ID_TRUE;
                }
            }

            // PROJ-1407 Temporary Table
            if( sIsPrivateVol == ID_TRUE )
            {
                sIsUpdateInplace = ID_TRUE;
            }
        }

        if( sIsUpdateInplace == ID_TRUE )
        {
            /* inplace update 시작 시점을 저장 */
            aCursor->mUpdateInplaceEscalationPoint = aCursor->mIterator->curRecPtr;

            // To Fix BUG-14969
            if ( ( aCursor->mFlag & SMI_INPLACE_UPDATE_MASK )
                 == SMI_INPLACE_UPDATE_ENABLE )
            {
                if(aCursor->mLockMode != SML_XLOCK)
                {
                    if( smlLockMgr::lockTable(
                            sTrans->mSlotN,
                            (smlLockItem *)
                            SMC_TABLE_LOCK( (smcTableHeader *)aCursor->mTable ),
                            SML_XLOCK,
                            sTrans->getLockTimeoutByUSec(),
                            &aCursor->mLockMode ) != IDE_SUCCESS )
                    {
                        IDE_TEST( (sTrans->mFlag & SMI_TRANS_INPLACE_UPDATE_MASK)
                                    != SMI_TRANS_INPLACE_UPDATE_TRY )

                        sTrans->mFlag &= ~SMI_TRANS_INPLACE_UPDATE_MASK;
                        sTrans->mFlag |=  SMI_TRANS_INPLACE_UPDATE_DISABLE;
                        IDE_CONT( CONT_INPLACE_UPDATE_DISABLE );
                    }
                }

                makeUpdatedIndexList( ((smcTableHeader*)aCursor->mTable),
                                      aCursor->mColumns,
                                      &aCursor->mModifyIdxBit,
                                      &aCursor->mCheckUniqueIdxBit );

                aCursor->mUpdate = sUpdateInpFunc;
            }
            else
            {
                // mFlag가 SMI_INPLACE_DISABLE인 경우,
                // inplace update로 수행되면 안됨
                // < inplace update 하면 안되는 이유 >
                //   trigger나 foreign key가 있는 경우,
                //   갱신 전 후 값을 읽어야 하는데 inplace update하면
                //   갱신 전 후 값을 구할 수 없다.
                //   따라서 QP에서 trigger나 foreign key가 있는 경우,
                //   SMI_INPLACE_DISABLE flag를 설정해준다.
            }

        } // aCursor->mIsSolCursor

        sStatistics       = smxTrans::getStatistics( sTrans );
        sUpdateMaxLogSize = 
                gSmiGlobalCallBackList.getUpdateMaxLogSize( sStatistics );

        if( sUpdateMaxLogSize != 0 )
        {
            /* BUG-19080: 
             *     OLD Version의 양이 일정이상 만들면 해당 Transaction을
             *     Abort하는 기능이 필요합니다.
             *
             * TRX_UPDATE_MAX_LOGSIZE이 0이면 이 Property는 Disable된 것이므로
             * 무시한다.
             *
             * */
            IDE_TEST_RAISE( sTrans->mUpdateSize > sUpdateMaxLogSize,
                            ERR_TOO_MANY_UPDATE );
        }
    }// aCursor->mUpdate != sUpdateInpFunc

    IDE_EXCEPTION_CONT( CONT_INPLACE_UPDATE_DISABLE );

    IDE_TEST( aCursor->mUpdate( aCursor->mIterator->properties->mStatistics,
                                aCursor->mTrans,
                                aCursor->mSCN,
                                aCursor->mTableInfo,
                                aCursor->mTable,
                                aCursor->mIterator->curRecPtr,
                                aCursor->mIterator->mRowGRID,
                                &(aCursor->mIterator->curRecPtr),
                                &(aCursor->mIterator->mRowGRID),
                                sNewColumnList,
                                sNewValueList,
                                aRetryInfo,
                                aCursor->mInfinite,
                                NULL, // aLobInfo4Update
                                aCursor->mModifyIdxBit,
                                aCursor->mStatement->isForbiddenToRetry() )
              != IDE_SUCCESS );

    if( sIsReadBeforeImg == ID_TRUE )
    {
        sState = 0;
        /* memory free */
        IDE_TEST( destFullUpdateMRDB( &sNewColumnList,
                                      &sNewValueList,
                                      &sOldValueBuffer)
                  != IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SELECTED_ROW );
    IDE_SET( ideSetErrorCode( smERR_FATAL_smiNoSelectedRow ) );

    IDE_EXCEPTION( ERR_TOO_MANY_UPDATE );
    IDE_SET( ideSetErrorCode( smERR_ABORT_TOO_MANY_UPDATE_LOG,
                              sTrans->mUpdateSize,
                              sUpdateMaxLogSize ) );
    IDE_EXCEPTION_END;

    if( sIsReadBeforeImg == ID_TRUE )
    {
        if( sState == 1 )
        {
            /* memory free */
            (void)destFullUpdateMRDB( &sNewColumnList,
                                      &sNewValueList,
                                      &sOldValueBuffer );
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aCursor가 가리키는 Row를 삭제한다.
 *
 * aCursor     - [IN] Table Cursor
 * aRetryInfo  - [IN] retry 정보
 **********************************************************************/
IDE_RC smiTableCursor::deleteRowMRVRDB( smiTableCursor        * aCursor,
                                        const smiDMLRetryInfo * aRetryInfo )
{
    /* =================================================================
     * If the delete operation is performed by a replicated transaction,
     * the delete row is made one per transaction not per cursor        *
     * =================================================================*/
    IDE_TEST_RAISE( aCursor->mIterator->curRecPtr == NULL,
                    ERR_NO_SELECTED_ROW );

    IDE_TEST( aCursor->mRemove( aCursor->mIterator->properties->mStatistics,
                                aCursor->mTrans,
                                aCursor->mSCN,
                                aCursor->mTableInfo,
                                aCursor->mTable,
                                aCursor->mIterator->curRecPtr,
                                aCursor->mIterator->mRowGRID,
                                aCursor->mInfinite,
                                aRetryInfo,
                                aCursor->mIsDequeue,
                                aCursor->mStatement->isForbiddenToRetry() )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SELECTED_ROW );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smiNoSelectedRow ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 **********************************************************************/
IDE_RC smiTableCursor::openDRDB( smiTableCursor *    aCursor,
                                 const void*         aTable,
                                 smSCN               aSCN,
                                 const smiRange *    aKeyRange,
                                 const smiRange *    aKeyFilter,
                                 const smiCallBack * aRowFilter,
                                 smlLockNode *       aCurLockNodePtr,
                                 smlLockSlot *       aCurLockSlotPtr)
{
    smSCN      sCreateSCN;
    smSCN      sSCN;
    smTID      sDummyTID;
    smxTrans * sTrans = aCursor->mTrans;

    if( aCursor->mCursorType == SMI_INSERT_CURSOR )
    {
        (void)gSmiGlobalCallBackList.checkNeedUndoRecord( aCursor->mStatement, 
                                                          (void*)aTable, 
                                                          &aCursor->mNeedUndoRec );
    }

    if ( sTrans->mIsGCTx == ID_TRUE )
    {
        SM_SET_SCN( &sCreateSCN, &(((smpSlotHeader *)aTable)->mCreateSCN) );

        if ( SM_SCN_IS_INFINITE( sCreateSCN ) &&
             /* BUG-48244 : 내(TX)가 2PC한 TX는 Pending 완료시까지 대기할 필요없다. */
             ( SMP_GET_TID( sCreateSCN ) != smxTrans::getTransID( sTrans ) ) )
        {
            /* PROJ-2733
             * commit되지 않은 테이블에 접근할수도 있다. */
            IDE_TEST( smxTrans::waitPendingTx( sTrans,
                                               sCreateSCN,
                                               aCursor->mSCN )
                      != IDE_SUCCESS );
        }
    }

    IDE_DASSERT( SM_SCN_IS_FREE_ROW( ((smpSlotHeader*)aTable)->mLimitSCN ) == ID_TRUE );

    /* Table Meta가 변경되었는지 체크 */
    SMX_GET_SCN_AND_TID( ((smpSlotHeader*)aTable)->mCreateSCN, sSCN, sDummyTID );

    IDE_TEST_RAISE( ( SM_SCN_IS_EQ(&sSCN, &aSCN) != ID_TRUE
                      && SM_SCN_IS_NOT_INIT(aSCN) ) ||
                    ( SM_SCN_IS_GT(&sSCN, &aCursor->mSCN)  &&
                      SM_SCN_IS_NOT_INFINITE(sSCN) ),
                    ERR_MODIFIED );

    /* Iterator 초기화 */
    if(aCursor->mIndex != NULL)
    {
        IDE_TEST_RAISE( ( ((smnIndexHeader*)aCursor->mIndex)->mFlag &
                          SMI_INDEX_USE_MASK )
                        != SMI_INDEX_USE_ENABLE, err_disabled_index );

        IDE_TEST_RAISE( smnManager::getIsConsistentOfIndexHeader(
                                              (void*)aCursor->mIndex ) == ID_FALSE, 
                        err_inconsistent_index);
    }

    /* FOR A4 : Cursor Property 적용, aIterator --> mIterator */
    IDE_TEST( ((smnIndexModule*)aCursor->mIndexModule)->mInitIterator( aCursor->mIterator,
                                                                       aCursor->mTrans,
                                                                       (void*)aCursor->mTable,
                                                                       (void*)aCursor->mIndex,
                                                                       aCursor->mDumpObject,
                                                                       aKeyRange,
                                                                       aKeyFilter,
                                                                       aRowFilter,
                                                                       aCursor->mFlag,
                                                                       aCursor->mSCN,
                                                                       aCursor->mInfinite,
                                                                       aCursor->mUntouchable,
                                                                       &aCursor->mCursorProp,
                                                                       &aCursor->mSeekFunc,
                                                                       aCursor->mStatement )
              != IDE_SUCCESS );

    if( aCursor->mUntouchable == ID_FALSE )
    {
        if( ( aCursor->mNeedUndoRec == ID_TRUE ) ||
            ( aCursor->mCursorType  != SMI_INSERT_CURSOR ) )
        {
            // Insert와 update undo segment에서 next undo record가 될 지점의
            // RID를 미리 구해 놓는다. <- close시에 요기서 부터 처리하면 됨.
            aCursor->mTrans->getUndoCurPos( &aCursor->mFstUndoRecSID,
                                            &aCursor->mFstUndoExtRID );
        }

        // insert, 혹은 update연산으로 영향을 받는 Index 리스트를 구한다.
        makeUpdatedIndexList( ((smcTableHeader*)aCursor->mTable),
                              aCursor->mColumns,
                              &aCursor->mModifyIdxBit,
                              &aCursor->mCheckUniqueIdxBit );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_disabled_index );
    { 
        IDE_SET( ideSetErrorCode(smERR_ABORT_DISABLED_INDEX) );
    }
    IDE_EXCEPTION( ERR_MODIFIED );
    {
        if(aCurLockSlotPtr != NULL)
        {
            (void)smlLockMgr::unlockTable( aCursor->mTrans->mSlotN,
                                           aCurLockNodePtr,
                                           aCurLockSlotPtr );
        }

        if( aCursor->mStatement->isForbiddenToRetry() )
        {
            SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];
            idlOS::snprintf( sMsgBuf,
                             SMI_MAX_ERR_MSG_LEN,
                             "[TABLE VALIDATION] "
                             "TableOID:%"ID_vULONG_FMT", "
                             "CursorSCN:%"ID_UINT64_FMT", "
                             "TableSCN:%"ID_UINT64_FMT", %"ID_UINT64_FMT,
                             ((smcTableHeader*)(aCursor->mTable))->mSelfOID,
                             SM_SCN_TO_LONG(aCursor->mSCN),
                             SM_SCN_TO_LONG(sSCN),
                             SM_SCN_TO_LONG(aSCN) );

            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) );

            IDE_ERRLOG( IDE_SD_19 );
        }
        else
        {
            IDE_SET( ideSetErrorCode( smERR_REBUILD_smiTableModified ) );
        }
    }
    IDE_EXCEPTION( err_inconsistent_index);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTableCursor::closeDRDB4InsertCursor( smiTableCursor * aCursor )
{
    scPageID        sUndoPageID;
    scSlotNum       sFstSlotNum;
    sdcTXSegEntry  *sEntry;

    sEntry      = aCursor->mTrans->getTXSegEntry();
    sUndoPageID = sEntry->mFstUndoPID;

    /***************************************************************************
     * BUG-30109 - Direct-Path INSERT를 수행한 Cursor일 경우,
     * mDPathSegInfo 내에서 마지막으로 할당한 페이지에 대하여
     * setDirty를 수행해 준다.
     *
     *  mDPathSegInfo를 할당하여 매달아 주기 전에 예외가 발생하여 Transaction이
     * Abort될 경우, INSERT METHOD가 APPEND라고 해도 mDPathSegInfo가 NULL일 수
     * 있다.
     **************************************************************************/
    if( (aCursor->mFlag & SMI_INSERT_METHOD_MASK) == SMI_INSERT_METHOD_APPEND &&
        (aCursor->mDPathSegInfo != NULL) )
    {
        sdcDPathInsertMgr::setDirtyLastAllocPage( aCursor->mDPathSegInfo );
    }

    IDE_TEST_CONT( aCursor->mNeedUndoRec == ID_FALSE,
                    SKIP_SET_UNDO_POSITION );

    if( sUndoPageID != SD_NULL_PID )
    {
        if ( aCursor->mFstUndoRecSID == SD_NULL_SID )
        {
            //----------------------------
            // open 당시, transaction의 undo record가 하나도 생성되지 않아
            // 첫번째 insert undo record sid가 설정되지 않은 경우,
            // 이를 설정해줌
            //----------------------------
            sFstSlotNum = sEntry->mFstUndoSlotNum;
            IDE_ASSERT( sFstSlotNum != SC_NULL_SLOTNUM );

            aCursor->mFstUndoRecSID = SD_MAKE_SID( sUndoPageID, sFstSlotNum );
            aCursor->mFstUndoExtRID = sEntry->mFstExtRID4UDS;
        }
        else
        {
            // nothing to do
        }

        //----------------------------
        // 나의 cursor가 생성한 마지막 insert undo record rid 위치 구하기
        //----------------------------
        aCursor->mTrans->getUndoCurPos( &aCursor->mLstUndoRecSID,
                                        &aCursor->mLstUndoExtRID );
    }

    IDE_EXCEPTION_CONT( SKIP_SET_UNDO_POSITION );

    return IDE_SUCCESS;
}

IDE_RC smiTableCursor::closeDRDB4UpdateCursor( smiTableCursor * aCursor )
{
    scPageID        sUndoPageID;
    scSlotNum       sFstSlotNum;
    sdcTXSegEntry  *sEntry;

    IDE_DASSERT( (aCursor->mCursorType == SMI_UPDATE_CURSOR) ||
                 (aCursor->mCursorType == SMI_COMPOSITE_CURSOR) );

    sEntry      = aCursor->mTrans->getTXSegEntry();
    sUndoPageID = sEntry->mFstUndoPID;

    if( sUndoPageID != SD_NULL_PID )
    {
        if ( aCursor->mFstUndoRecSID == SD_NULL_SID )
        {
            //----------------------------
            // open 당시, transaction의 undo record가 하나도 생성되지 않아
            // 첫번째 update undo record sid가 설정되지 않은 경우,
            // 이를 설정해줌
            //----------------------------
            sFstSlotNum = sEntry->mFstUndoSlotNum;
            IDE_ASSERT( sFstSlotNum != SC_NULL_SLOTNUM );

            aCursor->mFstUndoRecSID = SD_MAKE_SID( sUndoPageID, sFstSlotNum );
            aCursor->mFstUndoExtRID = sEntry->mFstExtRID4UDS;
        }
        else
        {
            // nothing to do
        }

        //----------------------------
        // 나의 cursor가 생성한 마지막 update undo sid 위치 구하기
        //----------------------------
        aCursor->mTrans->getUndoCurPos( &aCursor->mLstUndoRecSID,
                                        &aCursor->mLstUndoExtRID );

        IDE_TEST(smiTableCursor::insertKeysWithUndoSID( aCursor )
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smiTableCursor::closeDRDB4DeleteCursor( smiTableCursor * aCursor )
{
    scPageID        sUndoPageID;
    scSlotNum       sFstSlotNum;
    sdcTXSegEntry  *sEntry;

    IDE_DASSERT( aCursor->mCursorType == SMI_DELETE_CURSOR );

    sEntry      = aCursor->mTrans->getTXSegEntry();
    sUndoPageID = sEntry->mFstUndoPID;

    if( sUndoPageID != SD_NULL_PID )
    {
        if ( aCursor->mFstUndoRecSID == SD_NULL_SID )
        {
            //----------------------------
            // open 당시, transaction의 undo record가 하나도 생성되지 않아
            // 첫번째 insert undo record sid가 설정되지 않은 경우,
            // 이를 설정해줌
            //----------------------------
            sFstSlotNum = sEntry->mFstUndoSlotNum;
            IDE_ASSERT( sFstSlotNum != SC_NULL_SLOTNUM );

            aCursor->mFstUndoRecSID =
                SD_MAKE_SID( sUndoPageID, sFstSlotNum );
            aCursor->mFstUndoExtRID = sEntry->mFstExtRID4UDS;
        }
        else
        {
            // nothing to do
        }

        //----------------------------
        // 나의 cursor가 생성한 마지막 undo record rid 위치 구하기
        //----------------------------
        aCursor->mTrans->getUndoCurPos(
                        &aCursor->mLstUndoRecSID,
                        &aCursor->mLstUndoExtRID );
    }

    return IDE_SUCCESS;
}

IDE_RC smiTableCursor::closeDRDB( smiTableCursor * aCursor )
{
    UInt  sState     = 1;
    sdSID sTSSlotSID = smxTrans::getTSSlotSID( aCursor->mTrans );

    /*
     * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
     * 수행할 필요가 없습니다.
     */
    if( (smiStatement::getSkipCursorClose( aCursor->mStatement ) == ID_FALSE) &&
        (aCursor->mUntouchable == ID_FALSE) &&
        (sTSSlotSID != SD_NULL_SID) )
    {
        switch( aCursor->mCursorType )
        {
            case SMI_INSERT_CURSOR :
            {
                IDE_TEST( closeDRDB4InsertCursor( aCursor )
                          != IDE_SUCCESS );
                break;
            }
            case SMI_UPDATE_CURSOR :
            {
                IDE_TEST( closeDRDB4UpdateCursor( aCursor )
                          != IDE_SUCCESS );
                break;
            }
            case SMI_DELETE_CURSOR :
            {
                IDE_TEST( closeDRDB4DeleteCursor( aCursor )
                          != IDE_SUCCESS );
                break;
            }
            case SMI_COMPOSITE_CURSOR :
            {
                IDE_TEST( closeDRDB4UpdateCursor( aCursor )
                          != IDE_SUCCESS );
                IDE_TEST( closeDRDB4InsertCursor( aCursor )
                          != IDE_SUCCESS );
                break;
            }
            default :  // SMI_LOCKROW_CURSOR
            {
                break;
            }
        }
    }
    else
    {
        // nothing to do
    }

    sState = 0;
    IDE_TEST( ((smnIndexModule*)aCursor->mIndexModule)->mDestIterator(
                                                     aCursor->mIterator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        /* BUG-22442: smiTableCursor::close에서 error발생시 Iterator에 대해
         *            Destroy를 하지 않고 있습니다. */
        IDE_ASSERT( ((smnIndexModule*)aCursor->mIndexModule)->mDestIterator(
                        aCursor->mIterator )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/******************************************************************
 * TASK-5030
 * Description :
 *  sdcRow가 넘겨준 정보를 바탕으로 smiValue형태를 다시 구축한다.
 *  sdnbBTree::makeSmiValueListInFetch()를 참고하여 작성하였음
 *  
 *  aIndexColumn        - [IN]     인덱스 칼럼 정보
 *  aCopyOffset         - [IN]     column은 여러 rowpiece에 나누어 저장될 수 있으므로,
 *                                 copy offset 정보를 인자로 넘긴다.
 *                                 aCopyOffset이 0이면 first colpiece이고
 *                                 aCopyOffset이 0이 아니면 first colpiece가 아니다.
 *  aColumnValue        - [IN]     복사할 column value
 *  aKeyInfo            - [OUT]    뽑아낸 Row에 관한 정보를 저장할 곳이다.
 *
 * ****************************************************************/
IDE_RC smiTableCursor::makeSmiValueListInFetch(
                       const smiColumn            * aIndexColumn,
                       UInt                         aCopyOffset,
                       const smiValue             * aColumnValue,
                       void                       * aIndexInfo )
{
    UShort               sColumnSeq;
    smiValue           * sValue;
    sdcIndexInfo4Fetch * sIndexInfo;

    IDE_DASSERT( aIndexColumn != NULL );
    IDE_DASSERT( aColumnValue != NULL );
    IDE_DASSERT( aIndexInfo   != NULL );

    sIndexInfo   = (sdcIndexInfo4Fetch *)aIndexInfo;
    sColumnSeq   = aIndexColumn->id & SMI_COLUMN_ID_MASK;
    sValue       = &sIndexInfo->mValueList[ sColumnSeq ];

    if( aCopyOffset == 0 ) //first col-piece
    {
        sValue->length = aColumnValue->length;
        sValue->value  = sIndexInfo->mBufferCursor;
    }
    else                   //first col-piece가 아닌 경우
    {
        sValue->length += aColumnValue->length;
    }

    if( 0 < aColumnValue->length ) //NULL일 경우 length는 0
    {
        ID_WRITE_AND_MOVE_DEST( sIndexInfo->mBufferCursor,
                                aColumnValue->value,
                                aColumnValue->length );
    }

    return IDE_SUCCESS;
}


/***********************************************************************
 * TASK-5030
 * Description : Full XLog를 위한 new value list와 
 *               new column list를 만든다.
 *      sdnbBTree::makeKeyValueFromRow()를 참고해서 작성
 *
 * aCursor                  - [IN] table cursor
 * aValueList               - [IN] original value list (after value list)
 * aIndexInfo4Fetch         - [IN] for fetching before image
 * aFetchColumnList         - [IN] for fetching before image
 * aBeforeRowBufferSource   - [IN] buffer for before value
 * aNewColumnList           - [OUT] new column List
 **********************************************************************/
IDE_RC smiTableCursor::makeColumnNValueListDRDB(
                            smiTableCursor      * aCursor,
                            const smiValue      * aValueList,
                            sdcIndexInfo4Fetch  * aIndexInfo4Fetch,
                            smiFetchColumnList  * aFetchColumnList,
                            SChar               * aBeforeRowBufferSource,
                            smiColumnList       * aNewColumnList )
{
    UInt                    i               = 0;
    UInt                    j               = 0;
    idBool                  sIsRowDeleted;
    idBool                  sIsPageLatchReleased;
    idBool                  sTrySuccess;
    idBool                  sIsFindColumn   = ID_FALSE;
    smcTableHeader        * sTableHeader;
    smiColumn             * sTableColumn;
    UChar                 * sBeforeRowBuffer;
    smiFetchColumnList    * sCurrFetchColumnList;
    smiColumnList         * sCurrColumnList;
    smiColumnList         * sCurrUpdateColumnList;
    UChar                 * sRow;

    IDE_ASSERT( aCursor->mTable != NULL );

    sTableHeader     = (smcTableHeader *)(aCursor->mTable);

    sBeforeRowBuffer = (UChar*)idlOS::align8( (ULong)aBeforeRowBufferSource );

    aIndexInfo4Fetch->mTableHeader          = sTableHeader;
    aIndexInfo4Fetch->mCallbackFunc4Index   = smiTableCursor::makeSmiValueListInFetch; 
    aIndexInfo4Fetch->mBuffer               = (UChar*)sBeforeRowBuffer;
    aIndexInfo4Fetch->mBufferCursor         = (UChar*)sBeforeRowBuffer;
    aIndexInfo4Fetch->mFetchSize            = SDN_FETCH_SIZE_UNLIMITED;

    IDE_TEST( sdbBufferMgr::getPageByGRID( aCursor->mIterator->properties->mStatistics,
                                           aCursor->mIterator->mRowGRID,
                                           SDB_S_LATCH,
                                           SDB_WAIT_NORMAL,
                                           NULL, /* aMtx */
                                           (UChar**)&sRow,
                                           &sTrySuccess )
                != IDE_SUCCESS);

    IDE_TEST( sdcRow::fetch(
                        aCursor->mIterator->properties->mStatistics,
                        NULL, /* aMtx */
                        NULL, /* aSP */
                        aCursor->mTrans,
                        smcTable::getTableSpaceID(sTableHeader),
                        sRow,
                        ID_TRUE, /* aIsPersSlot */
                        SDB_SINGLE_PAGE_READ,
                        aFetchColumnList,
                        SMI_FETCH_VERSION_CONSISTENT,
                        smxTrans::getTSSlotSID( aCursor->mTrans ),
                        &aCursor->mSCN,
                        &aCursor->mInfinite,
                        aIndexInfo4Fetch,
                        NULL, /* aLobInfo4Fetch */
                        sTableHeader->mRowTemplate,
                        (UChar*)sBeforeRowBuffer,
                        &sIsRowDeleted,
                        &sIsPageLatchReleased
                        )
              != IDE_SUCCESS );

    if( sIsPageLatchReleased == ID_FALSE )
    {
        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage( aCursor->mIterator->properties->mStatistics,
                                             sRow )
                  != IDE_SUCCESS );
    }

    /* set new column list */
    sCurrUpdateColumnList = (smiColumnList *)aCursor->mColumns; /* column list for after image */
    sCurrFetchColumnList  = aFetchColumnList; /* column list for before image */
    for( i = 0, j = 0 ;
         i < sTableHeader->mColumnCount ;
         i++ )
    {
        sTableColumn  = (smiColumn *)smcTable::getColumn( sTableHeader, i );
        sIsFindColumn = ID_FALSE;

        /* There are no more columns */
        if( (sCurrUpdateColumnList == NULL ) && (sCurrFetchColumnList == NULL ))
        {
            break;
        }

        if( sCurrUpdateColumnList == NULL )
        {
            /* There are no more after image */
        }
        else
        {
            if( sTableColumn->id == sCurrUpdateColumnList->column->id )
            {
                /* in case of after image */
                (aNewColumnList + j)->column = sCurrUpdateColumnList->column;
                (aNewColumnList + j)->next   = aNewColumnList + (j + 1);

                sCurrUpdateColumnList = sCurrUpdateColumnList->next;
                sIsFindColumn         = ID_TRUE;
            }
            else
            {
                /* do nothing */
            }
        }

        if( sCurrFetchColumnList == NULL )
        {
            /* There are no more before image */
        }
        else
        {
            if( sTableColumn->id == sCurrFetchColumnList->column->id )
            {
                /* in case of before image */
                (aNewColumnList + j)->column = sCurrFetchColumnList->column;
                (aNewColumnList + j)->next   = aNewColumnList + (j + 1);

                sCurrFetchColumnList = sCurrFetchColumnList->next;
                sIsFindColumn        = ID_TRUE;
            }
            else
            {
                /* do nothing */
            }
        }

        if( sIsFindColumn == ID_FALSE )
        {
            /* in case of lob type column on a before image, it is ture */
            IDE_ASSERT( SMI_IS_LOB_COLUMN(sTableColumn->flag) == ID_TRUE );
        }
        else
        {
            j++;
        }
    }
    (aNewColumnList + (j -1))->next = NULL;

    /* merge after image with befre image */
    sCurrUpdateColumnList = (smiColumnList *)aCursor->mColumns;  /* column list for after image */
    for( i = 0, j = 0 ; i < sTableHeader->mColumnCount ; i++ )
    {
        sTableColumn = (smiColumn *)smcTable::getColumn( sTableHeader, i );

        if( sTableColumn->id == sCurrUpdateColumnList->column->id )
        {
            /* in case of after value */
            aIndexInfo4Fetch->mValueList[i] = aValueList[j];

            if( sCurrUpdateColumnList->next == NULL )
            {
                /* There are no more after values */
                break;
            }
            else
            {
                j++;
                sCurrUpdateColumnList = sCurrUpdateColumnList->next;
            }
        }
    }

    /* compaction a value list */
    for( i = 0, j = 0, sCurrColumnList = aNewColumnList;
         i < sTableHeader->mColumnCount ;
         i++ )
    {
        if( j != 0 )
        {
            IDE_ASSERT( i >= j );

            aIndexInfo4Fetch->mValueList[ i - j ] = aIndexInfo4Fetch->mValueList[ i ];
        }

        sTableColumn = (smiColumn *)smcTable::getColumn( sTableHeader, i );

        if( sTableColumn->id == sCurrColumnList->column->id )
        {
            if( sCurrColumnList->next != NULL )
            {
                sCurrColumnList = sCurrColumnList->next;
            }
        }
        else
        {
            /* do not match, need to increase a compaction count */
            j++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * TASK-5030
 * Description : before value를 저장할 버퍼 크기를 구하고,
 *      Full XLog를 위해 Fetch Column List 를 만든다.
 *      여기서 만든 Fetch Column List를 makeColumnNValueList4DRDB()
 *      에서 사용한다.
 *      sdnManager::makeFetchColumnList() 를 참고하여 작성
 *      TASK-5030 이외에 사용하지 말것. 
 *      LOB 타입은 요구사항이 아니므로 무시
 *
 * aCursor          - [IN] table cursor
 * aFetchColumnList - [OUT] new fetch column list
 * aMaxRowSize      - [OUT] before value를 저장할 버퍼의 크기
 **********************************************************************/
IDE_RC smiTableCursor::makeFetchColumnList( smiTableCursor      * aCursor,
                                            smiFetchColumnList  * aFetchColumnList,
                                            UInt                * aMaxRowSize )
{
    UInt                  i             = 0;
    UInt                  j             = 0;
    UInt                  sColumnCount  = 0;
    UInt                  sMaxRowSize   = 0;
    smiFetchColumnList  * sFetchColumnList;
    smiColumn           * sTableColumn;
    const smiColumnList * sUpdateColumnList;
    smcTableHeader      * sTableHeader;

    IDE_ERROR( aFetchColumnList != NULL );

    sFetchColumnList = aFetchColumnList;
    sTableHeader     = (smcTableHeader *)(aCursor->mTable);
    sColumnCount     = sTableHeader->mColumnCount;

    /* calculate the buffer size for storing before image */
    sUpdateColumnList = aCursor->mColumns;
    for( i = 0, j = 0 ; i < sColumnCount ; i++ )
    {
        sTableColumn = (smiColumn*)smcTable::getColumn( sTableHeader, i );

        if( sTableColumn->id == sUpdateColumnList->column->id )
        {
            /* this column exists in after image */
            if( sUpdateColumnList->next != NULL)
            {
                sUpdateColumnList = sUpdateColumnList->next;
            }
            else
            {
                /* do nothing */
            }

            /* ignore a column of after image */
            continue;
        }
        else
        {
            /* this column exists in before image */
            if( SMI_IS_LOB_COLUMN(sTableColumn->flag) == ID_TRUE )
            {
                /* ignore a column because it is lob type */
                continue;
            }
        }

        sFetchColumnList[ j ].column = sTableColumn;
        sFetchColumnList[ j ].columnSeq = SDC_GET_COLUMN_SEQ( sTableColumn );

        IDE_TEST( gSmiGlobalCallBackList.findCopyDiskColumnValue(
                sTableColumn,
                (smiCopyDiskColumnValueFunc*)
                &sFetchColumnList[ j ].copyDiskColumn )
            != IDE_SUCCESS );

        sFetchColumnList[ j ].next = &sFetchColumnList[ j + 1 ];

        if( sMaxRowSize <
            idlOS::align8( sTableColumn->offset + sTableColumn->size ) )
        {
            sMaxRowSize =
                idlOS::align8( sTableColumn->offset + sTableColumn->size );
        }

        j++;
    }

    sFetchColumnList[ j - 1 ].next = NULL;

    (*aMaxRowSize)      = sMaxRowSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiTableCursor::makeFullUpdateDRDB( smiTableCursor    * aCursor,
                                           const smiValue    * aValueList,
                                           smiColumnList    ** aNewColumnList,
                                           smiValue         ** aNewValueList,
                                           SChar            ** aOldValueBuffer,
                                           idBool            * aIsReadBeforeImg )
{
    UInt                  sState          = 0;
    UInt                  sMaxRowSize     = 0;
    sdcIndexInfo4Fetch    sIndexInfo4Fetch;    
    smiColumnList       * sNewColumnList  = NULL;
    smiValue            * sNewValueList   = NULL;
    smcTableHeader      * sTableHeader    = NULL;
    SChar               * sOldValueBuffer = NULL;
    smiFetchColumnList  * sFetchColumnList= NULL;

    IDE_ERROR( aCursor->mTable != NULL );

    sTableHeader = (smcTableHeader *)(aCursor->mTable);

    /* smiTableCursor_makeFullUpdateDRDB_calloc_FetchColumnList.tc */
    IDU_FIT_POINT("smiTableCursor::makeFullUpdateDRDB::calloc::FetchColumnList");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMI,
                                 sTableHeader->mColumnCount,
                                 ID_SIZEOF(smiFetchColumnList),
                                 (void**) &sFetchColumnList )
              != IDE_SUCCESS );

    IDE_TEST( makeFetchColumnList( aCursor,
                                   sFetchColumnList,
                                   &sMaxRowSize )
              != IDE_SUCCESS );

    if ( sMaxRowSize == 0 )
    {
        /* do not need to read a before image */
        (*aNewColumnList)   = NULL;
        (*aNewValueList)    = NULL;
        (*aOldValueBuffer)  = NULL;
        (*aIsReadBeforeImg) = ID_FALSE;

    }
    else
    {
        /* need to read a before image and make new column and value list */
        /* smiTableCursor_makeFullUpdateDRDB_calloc_NewColumnList.tc */
        IDU_FIT_POINT("smiTableCursor::makeFullUpdateDRDB::calloc::NewColumnList");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMI,
                                     sTableHeader->mColumnCount,
                                     ID_SIZEOF(smiColumnList),
                                     (void**) &sNewColumnList )
                  != IDE_SUCCESS);
        sState = 1;

        /* smiTableCursor_makeFullUpdateDRDB_calloc_NewValueList.tc */
        IDU_FIT_POINT("smiTableCursor::makeFullUpdateDRDB::calloc::NewValueList");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMI,
                                     sTableHeader->mColumnCount,
                                     ID_SIZEOF(smiValue),
                                     (void**) &sNewValueList )
                  != IDE_SUCCESS );
        sState = 2;

        /* smiTableCursor_makeFullUpdateDRDB_calloc_OldValueBuffer.tc */
        IDU_FIT_POINT("smiTableCursor::makeFullUpdateDRDB::calloc::OldValueBuffer");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMI,
                                     1,
                                     sMaxRowSize + ID_SIZEOF(ULong), // for align
                                     (void**) &sOldValueBuffer )
                  != IDE_SUCCESS);
        sState = 3;

        IDE_TEST( makeColumnNValueListDRDB( aCursor,
                                            aValueList,
                                            &sIndexInfo4Fetch,
                                            sFetchColumnList,
                                            sOldValueBuffer,
                                            sNewColumnList )
                  != IDE_SUCCESS );

        idlOS::memcpy( sNewValueList,
                       sIndexInfo4Fetch.mValueList,
                       sTableHeader->mColumnCount * ID_SIZEOF(smiValue) );

        (*aNewColumnList)   = sNewColumnList;
        (*aNewValueList)    = sNewValueList;
        (*aOldValueBuffer)  = sOldValueBuffer;
        (*aIsReadBeforeImg) = ID_TRUE;
    }

    IDE_ERROR( sFetchColumnList != NULL );
    
    IDE_TEST( iduMemMgr::free( sFetchColumnList )
              != IDE_SUCCESS );
    sFetchColumnList = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            (void) iduMemMgr::free( sOldValueBuffer );
        case 2:
            (void) iduMemMgr::free( sNewValueList );
        case 1:
            (void) iduMemMgr::free( sNewColumnList );

        default:
            break;
    }

    if( sFetchColumnList != NULL )
    {
        (void) iduMemMgr::free( sFetchColumnList );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 현재 Cursor의 Iterator가 가리키는 Row를 update한다.
 *
 * aCursor     - [IN] Table Cursor
 * aValueList  - [IN] Update Value List
 * aRetryInfo  - [IN] retry 정보
 **********************************************************************/
IDE_RC smiTableCursor::updateRowDRDB( smiTableCursor       * aCursor,
                                      const smiValue       * aValueList,
                                      const smiDMLRetryInfo* aRetryInfo )
{
    smiColumnList     * sNewColumnList = NULL;
    smiValue          * sNewValueList = NULL;
    SChar             * sOldValueBuffer = NULL;
    idBool              sIsReadBeforeImg = ID_FALSE;
    UInt                sState = 0;

    IDE_ERROR( aCursor->mTable != NULL );
    IDE_TEST_RAISE(
             SC_GRID_IS_NULL(aCursor->mIterator->mRowGRID),
             ERR_NO_SELECTED_ROW );

    /* TASK-5030 타 DBMS와 연동을 위한 ALA 기능 추가 */
    if ( smcTable::isSupplementalTable( (smcTableHeader*)(aCursor->mTable) ) == ID_TRUE )
    {
        IDE_TEST( makeFullUpdateDRDB( aCursor,
                                      aValueList,
                                      &sNewColumnList,
                                      &sNewValueList,
                                      &sOldValueBuffer,
                                      &sIsReadBeforeImg )
                  != IDE_SUCCESS );
        sState = 1;

        if( sIsReadBeforeImg == ID_FALSE )
        {
            /* before value를 읽어올 필요가 없는 경우 */
            sNewColumnList  = (smiColumnList *)aCursor->mColumns;
            sNewValueList   = (smiValue *)aValueList;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sNewColumnList  = (smiColumnList *)aCursor->mColumns;
        sNewValueList   = (smiValue *)aValueList;
    }

    IDE_TEST( aCursor->mUpdate( aCursor->mIterator->properties->mStatistics,
                                aCursor->mTrans,
                                aCursor->mSCN,
                                aCursor->mTableInfo,
                                aCursor->mTable,
                                aCursor->mIterator->curRecPtr,
                                aCursor->mIterator->mRowGRID,
                                &(aCursor->mIterator->curRecPtr),
                                NULL, /* aRetSlotRID */ 
                                sNewColumnList,
                                sNewValueList,
                                aRetryInfo,
                                aCursor->mInfinite,
                                NULL, /* aLobInfo4Update */
                                0,    /* aModifyIdxBit   */
                                aCursor->mStatement->isForbiddenToRetry() )
              != IDE_SUCCESS );
    /*
     * 인덱스 키를 레코드보다 먼저 지워서는 안된다.
     * 반드시 lock validation을 거친 후에 인덱스 키를 삭제해야 한다.
     */
    if( SMI_TABLE_TYPE_IS_DISK( (smcTableHeader*)aCursor->mTable ) == ID_TRUE )
    {
        IDE_TEST( deleteKeysForUpdate( aCursor,
                                       aCursor->mIterator->mRowGRID )
                  != IDE_SUCCESS );
    }

    IDE_TEST(iduCheckSessionEvent(aCursor->mIterator->properties->mStatistics)
             != IDE_SUCCESS);

    if( sIsReadBeforeImg == ID_TRUE )
    {
        sState = 0;
        /* memory free */
        IDE_TEST( destFullUpdateDRDB( sNewColumnList,
                                      sNewValueList,
                                      sOldValueBuffer )
                  != IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SELECTED_ROW );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smiNoSelectedRow ) );
    }
    IDE_EXCEPTION_END;

    if( sIsReadBeforeImg == ID_TRUE )
    {
        if( sState == 1 )
        {
            (void)destFullUpdateDRDB( sNewColumnList,
                                      sNewValueList,
                                      sOldValueBuffer );
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 현재 Cursor의 Iterator가 가리키는 Row를 delete한다.
 *
 * aCursor     - [IN] Table Cursor
 * aRetryInfo  - [IN] retry 정보
 **********************************************************************/
IDE_RC smiTableCursor::deleteRowDRDB( smiTableCursor        * aCursor,
                                      const smiDMLRetryInfo * aRetryInfo )
{
    IDE_ERROR( aCursor->mTable != NULL );

    IDE_TEST_RAISE( SC_GRID_IS_NULL(aCursor->mIterator->mRowGRID),
                    ERR_NO_SELECTED_ROW );

    IDE_TEST( aCursor->mIsDequeue == ID_TRUE );

    IDE_TEST( aCursor->mRemove( aCursor->mIterator->properties->mStatistics,
                                aCursor->mTrans,
                                aCursor->mSCN,
                                aCursor->mTableInfo,
                                aCursor->mTable,
                                aCursor->mIterator->curRecPtr,
                                aCursor->mIterator->mRowGRID,
                                aCursor->mInfinite,
                                aRetryInfo,
                                aCursor->mIsDequeue,
                                aCursor->mStatement->isForbiddenToRetry() )
              != IDE_SUCCESS ); 

    /*
     * 지워진 키를 다시 삭제할수 있기 때문에
     * 인덱스 키를 레코드보다 먼저 지워서는 안된다.
     * 반드시 lock validation을 거친 후에 인덱스 키를 삭제해야 한다.
     */
    if( SMI_TABLE_TYPE_IS_DISK( (smcTableHeader*)aCursor->mTable ) == ID_TRUE )
    {
        IDE_TEST( deleteKeysForDelete( aCursor,
                                       aCursor->mIterator->mRowGRID )
                  != IDE_SUCCESS );
    }

    IDE_TEST(iduCheckSessionEvent(aCursor->mIterator->properties->mStatistics)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SELECTED_ROW );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smiNoSelectedRow ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 **********************************************************************/
const smiRange * smiTableCursor::getDefaultKeyRange( )
{
    return &smiTableCursorDefaultRange;
}

/***********************************************************************
 * Description :
 **********************************************************************/
const smiCallBack * smiTableCursor::getDefaultFilter( )
{
    return &smiTableCursorDefaultCallBack;
}


/*********************************************************
  PROJ-1509
  Function Description: beforeFirstModifiedMRVRDB
  Implementation :
       cursor가 첫번째 생성한 Oid 위치 정보 저장
***********************************************************/
IDE_RC smiTableCursor::beforeFirstModifiedMRVRDB( smiTableCursor * aCursor,
                                                  UInt             aFlag )
{
    aCursor->mFlag &= ~SMI_FIND_MODIFIED_MASK;
    aCursor->mFlag |= aFlag;

    if ( ( aCursor->mFstOidNode == aCursor->mLstOidNode ) &&
         ( aCursor->mFstOidCount == aCursor->mLstOidCount ) )
    {
        // 현재 cursor에 의해 생성된 oid record가 없는 경우
        aCursor->mCurOidNode = NULL;
        aCursor->mCurOidIndex = 0;
    }
    else
    {
        if ( aCursor->mFstOidCount >= smuProperty::getOIDListCount() )
        {
            IDE_ERROR( aCursor->mFstOidNode != NULL );

            // mOidNode의 마지막 record 일 경우,
            // 다음 mOidNode의 첫번째 record로 설정해야 함
            aCursor->mCurOidNode =
                ((smxOIDNode*)aCursor->mFstOidNode)->mNxtNode;
            aCursor->mCurOidIndex = 0;
        }
        else
        {
            aCursor->mCurOidNode = aCursor->mFstOidNode;
            aCursor->mCurOidIndex = aCursor->mFstOidCount;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: readOldRowMRVRDB
  Implementation :
      (1) beforeFirstModified() 함수가 이미 수행되었는지 검사
      (2) OldRow 를 찾음
      (3) 결과 반환
***********************************************************/
IDE_RC smiTableCursor::readOldRowMRVRDB( smiTableCursor * aCursor,
                                         const void    ** aRow,
                                         scGRID         * /* aGRID*/ )
{
    idBool       sFoundOldRow = ID_FALSE;
    smxOIDInfo * sCurOIDRec   = NULL;
    smOID        sCurRowOID   = SM_NULL_OID;
    scSpaceID    sSpaceID     = SC_NULL_SPACEID;
    UInt         sMaxOIDCnt   = smuProperty::getOIDListCount();

    //------------------------
    // (1) beforeFirstModified()를 이전에 수행하였는지 검사
    //------------------------

    IDE_ERROR( ( aCursor->mFlag & SMI_FIND_MODIFIED_MASK )
               == SMI_FIND_MODIFIED_OLD );

    IDE_ERROR( aCursor->mCursorType != SMI_INSERT_CURSOR );

    //------------------------
    // (2) old row를 찾음
    //------------------------

    while(aCursor->mCurOidNode != NULL)
    {
        if ( ( aCursor->mCurOidNode == aCursor->mLstOidNode ) &&
             ( aCursor->mCurOidIndex == aCursor->mLstOidCount ) )  // BUG-23370
        {
            // cursor가 생성한 undo record는 모두 읽어서 더이상
            // 읽어야 할 undo record가 존재하지 않음
            aCursor->mCurOidNode = NULL;
            aCursor->mCurOidIndex = 0;
            break;
        }
        else
        {
            if( aCursor->mCurOidIndex < sMaxOIDCnt )
            {
                sCurOIDRec = &(((smxOIDNode*)aCursor->mCurOidNode)->
                               mArrOIDInfo[aCursor->mCurOidIndex]);

                //------------------------
                // 나의  cursor가 갱신한 old record 인지 검사
                //------------------------

                if ( sCurOIDRec->mTableOID == aCursor->mTableInfo->mTableOID )
                {
                    sCurRowOID = sCurOIDRec->mTargetOID;

                    // old record 인지 검사
                    if (  ((sCurOIDRec->mFlag & SM_OID_OP_MASK) == SM_OID_OP_OLD_FIXED_SLOT)
                          /*delete row 연산을 수행한 row는 readOldRow에 의해 읽혀져야 한다.*/
                        ||((sCurOIDRec->mFlag & SM_OID_OP_MASK) == SM_OID_OP_DELETED_SLOT))
                    {
                        // old record를 찾음
                        sFoundOldRow = ID_TRUE;
                        sSpaceID = sCurOIDRec->mSpaceID;
                        IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                                           sCurRowOID, 
                                                           (void**)aRow )
                                    == IDE_SUCCESS );
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }

        //------------------------
        // 다음 oid record로 진행
        //------------------------

        aCursor->mCurOidIndex++;

        // To Fix BUG-14927
        if ( aCursor->mCurOidIndex > sMaxOIDCnt )
        {
            // 현재 OID Node에 OID Record를 모두 읽었으면
            // 다음 OID Node로 진행
            aCursor->mCurOidNode =
                ((smxOIDNode*)(aCursor->mCurOidNode))->mNxtNode;
            aCursor->mCurOidIndex = 0;
        }
        else
        {
            // nothing to do
        }

        //------------------------
        // row를 찾은 경우, 중단
        //------------------------

        if ( sFoundOldRow == ID_TRUE )
        {
            break;
        }
        else
        {
            // nothing to do
        }
    }

    //------------------------
    // (3) 결과 반환
    //------------------------

    if ( sFoundOldRow == ID_TRUE )
    {
        IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                           sCurRowOID, 
                                           (void**)aRow )
                    == IDE_SUCCESS );
    }
    else
    {
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: readNewRowMRVRDB
  Implementation :
      (1) beforeFirstModified() 함수가 이미 수행되었는지 검사
      (2) NewRow 를 찾음
      (3) 결과 반환
***********************************************************/
IDE_RC smiTableCursor::readNewRowMRVRDB( smiTableCursor * aCursor,
                                         const void    ** aRow,
                                         scGRID         * /* aRowGRID*/ )
{
    idBool       sFoundNewRow = ID_FALSE;
    smxOIDInfo * sCurOIDRec   = NULL;
    smOID        sCurRowOID   = SM_NULL_OID;
    scSpaceID    sSpaceID     = SC_NULL_SPACEID;
    UInt         sMaxOIDCnt   = smuProperty::getOIDListCount();

    //------------------------
    // (1) beforeFirstModified()를 이전에 수행하였는지 검사
    //------------------------

    IDE_ERROR( ( aCursor->mFlag & SMI_FIND_MODIFIED_MASK )
               == SMI_FIND_MODIFIED_NEW );

    IDE_ERROR( aCursor->mCursorType != SMI_DELETE_CURSOR );

    //------------------------
    // (2) new row를 찾음
    //------------------------

    while(aCursor->mCurOidNode != NULL)
    {
        if ( ( aCursor->mCurOidNode == aCursor->mLstOidNode ) &&
             ( aCursor->mCurOidIndex == aCursor->mLstOidCount ) )  // BUG-23370
        {
            // cursor가 생성한 undo record는 모두 읽어서 더이상
            // 읽어야 할 undo record가 존재하지 않음
            aCursor->mCurOidNode = NULL;
            aCursor->mCurOidIndex = 0;
            break;
        }
        else
        {
            if( aCursor->mCurOidIndex < sMaxOIDCnt )
            {
                sCurOIDRec = &(((smxOIDNode*)aCursor->mCurOidNode)->
                               mArrOIDInfo[aCursor->mCurOidIndex]);

                //------------------------
                // 나의  cursor가 갱신한 new record 인지 검사
                //------------------------

                if( sCurOIDRec->mTableOID == aCursor->mTableInfo->mTableOID )
                {

                    sCurRowOID = sCurOIDRec->mTargetOID;

                    // new record 인지 검사
                    if ((sCurOIDRec->mFlag & SM_OID_OP_MASK)
                        == SM_OID_OP_NEW_FIXED_SLOT)
                    {
                        // new record를 찾음
                        sFoundNewRow = ID_TRUE;
                        sSpaceID = sCurOIDRec->mSpaceID;
                        IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                                           sCurRowOID, 
                                                           (void**)aRow )
                                    == IDE_SUCCESS );
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }

        //------------------------
        // 다음 oid record로 진행
        //------------------------

        aCursor->mCurOidIndex++;

        // To Fix BUG-14927
        if ( aCursor->mCurOidIndex > sMaxOIDCnt )
        {
            // 현재 OID Node에 OID Record를 모두 읽었으면
            // 다음 OID Node로 진행
            aCursor->mCurOidNode =
                ((smxOIDNode*)(aCursor->mCurOidNode))->mNxtNode;
            aCursor->mCurOidIndex = 0;
        }
        else
        {
            // nothing to do
        }

        //------------------------
        // row를 찾은 경우, 중단
        //------------------------

        if ( sFoundNewRow == ID_TRUE )
        {
            break;
        }
        else
        {
            // nothing to do
        }
    }

    //------------------------
    // (3) 결과 반환
    //------------------------

    if ( sFoundNewRow == ID_TRUE )
    {
        IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                           sCurRowOID, 
                                           (void**)aRow )
                    == IDE_SUCCESS );
    }
    else
    {
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  Function Description: beforeFirstModifiedDRDB
  Implementation :
      cursor가 첫번째 생성된 undo record 위치 정보 저장
***********************************************************/
IDE_RC smiTableCursor::beforeFirstModifiedDRDB( smiTableCursor * aCursor,
                                                UInt             aFlag )
{
    aCursor->mFlag &= ~SMI_FIND_MODIFIED_MASK;
    aCursor->mFlag |= aFlag;

    // 갱신 연산에 대해서만 지원
    switch( aCursor->mCursorType )
    {
        case SMI_INSERT_CURSOR:
        {
            IDE_ERROR( (aFlag & SMI_FIND_MODIFIED_MASK)
                        == SMI_FIND_MODIFIED_NEW );

            aCursor->mCurUndoRecSID = aCursor->mFstUndoRecSID;
            aCursor->mCurUndoExtRID = aCursor->mFstUndoExtRID;

            // To Fix BUG-14990
            aCursor->mFlag &= ~SMI_READ_UNDO_PAGE_LIST_MASK;
            aCursor->mFlag |= SMI_READ_UNDO_PAGE_LIST_INSERT;
            break;
        }
        case SMI_DELETE_CURSOR:
        {
            IDE_ERROR( (aFlag & SMI_FIND_MODIFIED_MASK )
                        == SMI_FIND_MODIFIED_OLD );

            aCursor->mCurUndoRecSID = aCursor->mFstUndoRecSID;
            aCursor->mCurUndoExtRID = aCursor->mFstUndoExtRID;

            // To Fix BUG-14990
            aCursor->mFlag &= ~SMI_READ_UNDO_PAGE_LIST_MASK;
            aCursor->mFlag |= SMI_READ_UNDO_PAGE_LIST_UPDATE;
            break;
        }
        case SMI_UPDATE_CURSOR:
        case SMI_COMPOSITE_CURSOR:
        {
            aCursor->mCurUndoRecSID = aCursor->mFstUndoRecSID;
            aCursor->mCurUndoExtRID = aCursor->mFstUndoExtRID;

            // To Fix BUG-14990
            aCursor->mFlag &= ~SMI_READ_UNDO_PAGE_LIST_MASK;
            aCursor->mFlag |= SMI_READ_UNDO_PAGE_LIST_UPDATE;
            break;
        }
        case SMI_SELECT_CURSOR:
        {
            // To Fix BUG-14955
            // mCursorType 정보는 원래 cursor open 시에 QP가 사용 목적에
            // 따라 결정해서 SM에게 넘겨주는 것이 가장 바람직하다.
            // 그러나 현재 mCursorType 정보는 updateRow(), insertRow(),
            // deleteRow() 호출 시에 이 함수들 안에서 설정되도록 구현되어
            // 있다.
            // 따라서 QP에서 update를 위하여 cursor를 open하여도
            // 조건에 맞는 row가 없어 실제 updateRow()가 호출되지 않았을
            // 경우, cursor type이 SMI_SELECT_CURSOR일 수 있다.
            // ( SMI_SELECT_CURSOR가 default type이다. )
            // To Fix BUG-14990
            aCursor->mFlag &= ~SMI_READ_UNDO_PAGE_LIST_MASK;
            aCursor->mFlag |= SMI_READ_UNDO_PAGE_LIST_NONE;
            break;
        }
        case SMI_LOCKROW_CURSOR:
        {
            // Referential constraint check를 위한 lock row 커서에서는
            // 본 함수가 불려서는 안된다.
            IDE_ERROR(0);
            break;
        }
        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: readNewRowDRDB
  implementation :
      (1) beforeFirstModified()를 이전에 수행하였는지 검사
      (2) new row 반환

***********************************************************/
IDE_RC smiTableCursor::readNewRowDRDB( smiTableCursor * aCursor,
                                       const void    ** aRow,
                                       scGRID*          aRowGRID )
{
    UChar         * sCurUndoPage;
    UChar         * sSlotDirPtr;
    UChar         * sCurUndoRecHdr;
    idBool          sTrySuccess;
    idBool          sExistNewRow    = ID_FALSE;
    idBool          sIsFixUndoPage  = ID_FALSE;
    scGRID          sUpdatedRowGRID;
    UChar         * sUpdatedRow;
    sdcUndoRecType  sType;
    sdcUndoRecFlag  sFlag;
    smOID           sTableOID;
    idBool          sIsRowDeleted;
    scSpaceID       sTableSpaceID = smcTable::getTableSpaceID(aCursor->mTable);
    sdpSegMgmtOp  * sSegMgmtOp;
    scPageID        sSegPID;
    sdpSegInfo      sSegInfo;
    sdpExtInfo      sExtInfo;
    scPageID        sPrvUndoPID = SD_NULL_PID;
    idBool          sIsPageLatchReleased = ID_TRUE;
    smxTrans      * sTrans;

    //------------------------
    // (1) beforeFirstModified()를 이전에 수행하였는지 검사
    //------------------------

    IDE_ERROR( ( aCursor->mFlag & SMI_FIND_MODIFIED_MASK )
               == SMI_FIND_MODIFIED_NEW );

    IDE_ERROR( aCursor->mCursorType != SMI_DELETE_CURSOR );

    //------------------------
    // (2) new row 반환
    //------------------------

    /* BUG-25176 readNewRow와 readOldRow에서 Data가 변경되지 않은 경우를 고려하지 못함
     *
     * 예를들어, Foriegn Key 포함된 디스크 테이블에 INSERT INTO SELECT ..를 수행해서
     * SELECT 건수가 0건인 경우 Transaction Segment Entry가 NULL이므로 무조건
     * skip 한다. */

    sTrans = aCursor->mTrans;

    if ( sTrans->getTXSegEntry() == NULL )
    {
        *aRow = NULL;
        SC_MAKE_NULL_GRID( *aRowGRID ); 
        IDE_CONT( cont_skip_no_data );
    }

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    sSegPID    = smxTrans::getUDSegPtr(sTrans)->getSegPID();
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_ASSERT( sSegMgmtOp->mGetSegInfo(
                    NULL,
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                    sSegPID,
                    NULL, /* aTableHeader */
                    &sSegInfo ) == IDE_SUCCESS );

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       aCursor->mCurUndoExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    while ( aCursor->mCurUndoRecSID != SD_NULL_SID )
    {
        if( aCursor->mCurUndoRecSID == aCursor->mLstUndoRecSID )
        {
            // cursor가 생성한 undo record는 모두 읽어서 더이상
            // 읽어야 할 undo record가 존재하지 않음
            aCursor->mCurUndoRecSID = SD_NULL_SID;
            aCursor->mCurUndoExtRID = SD_NULL_RID;
        }
        else
        {
            //------------------------
            //현재 undo page list에 읽어야 할 undo record가 존재하는 경우
            //------------------------

            if ( sPrvUndoPID != SD_MAKE_PID( aCursor->mCurUndoRecSID ) )
            {
                // 현재 undo record가 저장된 page fix
                IDE_TEST( sdbBufferMgr::fixPageByPID( aCursor->mCursorProp.mStatistics,
                                                      SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                      SD_MAKE_PID( aCursor->mCurUndoRecSID ),
                                                      &sCurUndoPage,
                                                      &sTrySuccess )
                          != IDE_SUCCESS );

                sIsFixUndoPage = ID_TRUE;
                sPrvUndoPID    = SD_MAKE_PID( aCursor->mCurUndoRecSID );
            }
            else
            {
                IDE_ASSERT( sIsFixUndoPage == ID_TRUE );
            }

            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sCurUndoPage );

            if( SD_MAKE_SLOTNUM(aCursor->mCurUndoRecSID) ==
                sdpSlotDirectory::getCount(sSlotDirPtr) )
            {
                IDE_ASSERT( aCursor->mCurUndoRecSID ==
                            aCursor->mFstUndoRecSID );
                /* mFstUndoRecSID는 이전 cursor의
                 * 마지막 undo record 다음 위치를 가리킨다.
                 * 첫번째 undo record를 저장하려고 할때,
                 * mFstUndoRecSID가 가리키는 페이지에는 공간이 부족하여
                 * 저장할 수 없으면, 다음 페이지에 첫번째 undo record를
                 * 저장하게 된다.
                 * 그러면 mFstUndoRecSID는 실제로 존재하지 않는 위치를 가리키게 된다.
                 * 이런 경우에는 setNextUndoRecSID4NewRow() 함수를 호출하고
                 * continue를 한다. */
                IDE_TEST( setNextUndoRecSID4NewRow( &sSegInfo,
                                                    &sExtInfo,
                                                    aCursor,
                                                    sCurUndoPage,
                                                    &sIsFixUndoPage )
                          != IDE_SUCCESS );
                continue;
            }

            //-------------------------------------------------------
            // undo record가 존재하는 경우,
            // 나의 cursor에 의해 쓰여진 new row가 존재하는지 검사
            //-------------------------------------------------------
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                        sSlotDirPtr,
                                        SD_MAKE_SLOTNUM(aCursor->mCurUndoRecSID),
                                        &sCurUndoRecHdr)
                      != IDE_SUCCESS );

            SDC_GET_UNDOREC_HDR_FIELD( sCurUndoRecHdr,
                                       SDC_UNDOREC_HDR_TABLEOID,
                                       &sTableOID );

            if ( aCursor->mTableInfo->mTableOID == sTableOID )
            {
                SDC_GET_UNDOREC_HDR_1B_FIELD( sCurUndoRecHdr,
                                              SDC_UNDOREC_HDR_FLAG,
                                              &sFlag );

                if( ( SDC_UNDOREC_FLAG_IS_UNDO_FOR_HEAD_ROWPIECE(sFlag) == ID_TRUE ) &&
                    ( SDC_UNDOREC_FLAG_IS_VALID(sFlag) == ID_TRUE ) )
                {
                    SDC_GET_UNDOREC_HDR_1B_FIELD( sCurUndoRecHdr,
                                                  SDC_UNDOREC_HDR_TYPE,
                                                  &sType );

                    /* xxxx 함수 처리 */
                    switch( sType )
                    {
                        case SDC_UNDO_INSERT_ROW_PIECE :
                            sExistNewRow = ID_TRUE;
                            break;

                        case SDC_UNDO_UPDATE_ROW_PIECE    :
                        case SDC_UNDO_OVERWRITE_ROW_PIECE :
                            sExistNewRow = ID_TRUE;
                            break;

                        case SDC_UNDO_LOCK_ROW :
                            if( sdcUndoRecord::isExplicitLockRec(
                                   sCurUndoRecHdr) == ID_TRUE )
                            {
                                sExistNewRow = ID_FALSE;
                            }
                            else
                            {
                                sExistNewRow = ID_TRUE;;
                            }
                            break;

                        case SDC_UNDO_CHANGE_ROW_PIECE_LINK :
                        case SDC_UNDO_DELETE_ROW_PIECE :
                            sExistNewRow = ID_FALSE;
                            break;

                        default :
                            ideLog::log( 
                                IDE_SERVER_0,
                                "CurUndoRecSID  : %llu\n"
                                "          PID  : %u\n"
                                "          Slot : %u\n"
                                "sPrvUndoPID    : %u\n"
                                "sTableOID      : %lu\n"
                                "sFlag          : %u\n"
                                "sType          : %u\n"
                                ,
                                aCursor->mCurUndoRecSID,
                                SD_MAKE_PID( aCursor->mCurUndoRecSID ),
                                SD_MAKE_SLOTNUM( aCursor->mCurUndoRecSID ),
                                sPrvUndoPID,
                                sTableOID,
                                sFlag,
                                sType );
                            ideLog::logMem( IDE_SERVER_0,
                                            sCurUndoRecHdr,
                                            SDC_UNDOREC_HDR_MAX_SIZE );
                            ideLog::logMem( IDE_SERVER_0,
                                            sCurUndoPage,
                                            SD_PAGE_SIZE );

                            IDE_ASSERT(0);
                            break;
                    }
                }
                else
                {
                    sExistNewRow = ID_FALSE;
                }
            }
            else
            {
                // 다른 커서가 작성한 Undo Record인 경우 Skip 한다.
                sExistNewRow = ID_FALSE;
            }

            if ( sExistNewRow == ID_TRUE )
            {
                // 나의 cursor에 대한 undo record 인 경우 ,
                // undo record를 이용하여 update된 row를 fix
                sdcUndoRecord::parseRowGRID( sCurUndoRecHdr,
                                             &sUpdatedRowGRID );

                IDE_ASSERT( SC_GRID_IS_NOT_NULL( sUpdatedRowGRID ) );

                IDE_ASSERT( sTableSpaceID  ==
                            SC_MAKE_SPACE( sUpdatedRowGRID ) );

                IDE_TEST( sdbBufferMgr::getPageByGRID(
                                        aCursor->mCursorProp.mStatistics,
                                        sUpdatedRowGRID,
                                        SDB_S_LATCH,
                                        SDB_WAIT_NORMAL,
                                        NULL, /* aMtx */
                                        (UChar**)&sUpdatedRow,
                                        &sTrySuccess )
                          != IDE_SUCCESS );
                sIsPageLatchReleased = ID_FALSE;

                //-------------------------------------------------------
                // New Row가 존재하는 경우, New Row를 생성
                //-------------------------------------------------------
                IDE_TEST( sdcRow::fetch(
                              aCursor->mCursorProp.mStatistics,
                              NULL, /* aMtx */
                              NULL, /* aSP */
                              aCursor->mTrans,
                              SC_MAKE_SPACE(sUpdatedRowGRID),
                              sUpdatedRow,
                              ID_TRUE, /* aIsPersSlot */
                              SDB_SINGLE_PAGE_READ,
                              aCursor->mCursorProp.mFetchColumnList,
                              SMI_FETCH_VERSION_LAST,
                              SD_NULL_SID, /* aMyTSSRID */
                              NULL,        /* aMyStmtSCN */
                              NULL,        /* aInfiniteSCN */
                              NULL,        /* aIndexInfo4Fetch */
                              NULL,        /* aLobInfo4Fetch */
                              ((smcTableHeader*)aCursor->mTable)->mRowTemplate,
                              (UChar*)*aRow,
                              &sIsRowDeleted,
                              &sIsPageLatchReleased )
                          != IDE_SUCCESS );

                /* BUG-23319
                 * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
                /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
                 * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
                 * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
                 * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
                 * 상황에 따라 적절한 처리를 해야 한다. */
                if( sIsPageLatchReleased == ID_FALSE )
                {
                    sIsPageLatchReleased = ID_TRUE;
                    IDE_TEST( sdbBufferMgr::releasePage(
                                  aCursor->mCursorProp.mStatistics,
                                  sUpdatedRow )
                              != IDE_SUCCESS );
                }

                SC_COPY_GRID( sUpdatedRowGRID, *aRowGRID );
            }
            else
            {
                // nothing to do
            }

            //-------------------------------------------------------
            // 다음 undo record rid를 설정함
            //-------------------------------------------------------
            IDE_TEST( setNextUndoRecSID4NewRow( &sSegInfo,
                                                &sExtInfo,
                                                aCursor,
                                                sCurUndoRecHdr,
                                                &sIsFixUndoPage )
                      != IDE_SUCCESS );

            if ( sExistNewRow == ID_TRUE )
            {
                //-------------------------------------------------------
                // new row 값을 찾은 경우, new row 값 찾는 작업을 중단
                //-------------------------------------------------------
                break;
            }
            else
            {
                // 다음 undo record를 읽어야 하므로
                // 계속 진행
            }
        }
    }

    if ( sIsFixUndoPage == ID_TRUE )
    {
        sIsFixUndoPage = ID_FALSE;
        IDE_TEST(sdbBufferMgr::unfixPage( aCursor->mCursorProp.mStatistics,
                                          sCurUndoPage )
                 != IDE_SUCCESS);
    }

    if ( sExistNewRow == ID_FALSE )
    {
        // 더 이상 undo log record가 없는 경우
        *aRow = NULL;
        SC_MAKE_NULL_GRID( *aRowGRID );
    }
    else
    {
        // nothing to do
    }

    IDE_EXCEPTION_CONT( cont_skip_no_data );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if ( sIsFixUndoPage == ID_TRUE )
    {
        IDE_ASSERT(
            sdbBufferMgr::unfixPage(aCursor->mCursorProp.mStatistics,
                                    sCurUndoPage) == IDE_SUCCESS );
    }

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aCursor->mCursorProp.mStatistics,
                                               (UChar*)sUpdatedRow )
                    == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;

}

/*********************************************************
  function description: readOldRowDRDB
  implementation :
      (1) beforeFirstModified()를 이전에 수행하였는지 검사
      (2) old row 반환

***********************************************************/
IDE_RC smiTableCursor::readOldRowDRDB( smiTableCursor * aCursor,
                                       const void    ** aRow,
                                       scGRID         * aRowGRID )
{
    UChar         * sCurUndoPage;
    UChar         * sSlotDirPtr;
    UChar         * sCurUndoRecHdr = NULL;
    idBool          sTrySuccess;
    idBool          sExistOldRow   = ID_FALSE;
    idBool          sIsFixUndoPage = ID_FALSE;
    sdcRowHdrInfo   sRowHdrInfo;
    scGRID          sUpdatedRowGRID;
    UChar         * sUpdatedRow;
    sdcUndoRecFlag  sFlag;
    sdcUndoRecType  sType;
    smOID           sTableOID;
    idBool          sIsRowDeleted;
    scSpaceID       sTableSpaceID
                    = smcTable::getTableSpaceID(aCursor->mTable);
    sdpSegMgmtOp  * sSegMgmtOp;
    sdpSegInfo      sSegInfo;
    sdpExtInfo      sExtInfo;
    scPageID        sSegPID;
    scPageID        sPrvUndoPID = SD_NULL_PID;
    idBool          sIsPageLatchReleased = ID_TRUE;
    smxTrans      * sTrans;

    //------------------------
    // (1) beforeFirstModified()를 이전에 수행하였는지 검사
    //------------------------

    IDE_ERROR( ( aCursor->mFlag & SMI_FIND_MODIFIED_MASK )
               != SMI_FIND_MODIFIED_NEW );

    IDE_ERROR( aCursor->mCursorType != SMI_INSERT_CURSOR );

    //------------------------
    // (2) old row 반환
    //------------------------
    /* BUG-25176 readNewRow에서 Data가 변경되지 않은 경우를 고려하지 못함
     * Transaction Segment Entry가 NULL인 경우는 들어올일이 없지만, 안전하게
     * 확인하고 무조건 skip 한다. */
    sTrans = aCursor->mTrans;

    if ( sTrans->getTXSegEntry() == NULL )
    {
        *aRow = NULL;
        SC_MAKE_NULL_GRID( *aRowGRID ); 
        IDE_CONT( cont_skip_no_data );
    }

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    sSegPID    = smxTrans::getUDSegPtr(sTrans)->getSegPID();
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_ASSERT( sSegMgmtOp->mGetSegInfo(
                    NULL,
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                    sSegPID,
                    NULL, /* aTableHeader */
                    &sSegInfo ) == IDE_SUCCESS );

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       aCursor->mCurUndoExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    while( aCursor->mCurUndoRecSID != SD_NULL_SID )
    {
        if ( aCursor->mCurUndoRecSID == aCursor->mLstUndoRecSID )
        {
            // cursor가 생성한 undo record는 모두 읽어
            // 더이상 읽어야 할 undo record가 없음
            aCursor->mCurUndoRecSID = SD_NULL_SID;
            aCursor->mCurUndoExtRID = SD_NULL_RID;
        }
        else
        {
            if ( sPrvUndoPID != SD_MAKE_PID( aCursor->mCurUndoRecSID ) )
            {
                // 현재 undo record가 저장된 page fix
                IDE_TEST( sdbBufferMgr::fixPageByPID( aCursor->mCursorProp.mStatistics,
                                                      SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                      SD_MAKE_PID( aCursor->mCurUndoRecSID ),
                                                      &sCurUndoPage,
                                                      &sTrySuccess )
                          != IDE_SUCCESS );

                sIsFixUndoPage = ID_TRUE;
                sPrvUndoPID    = SD_MAKE_PID( aCursor->mCurUndoRecSID );
            }
            else
            {
                IDE_ASSERT( sIsFixUndoPage == ID_TRUE );
            }

            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sCurUndoPage);

            if( SD_MAKE_SLOTNUM(aCursor->mCurUndoRecSID) ==
                sdpSlotDirectory::getCount(sSlotDirPtr) )
            {
                IDE_ASSERT( aCursor->mCurUndoRecSID ==
                            aCursor->mFstUndoRecSID );
                /*
                 * mFstUndoRecSID는 이전 cursor의
                 * 마지막 undo record 다음 위치를 가리킨다.
                 * 첫번째 undo record를 저장하려고 할때,
                 * mFstUndoRecSID가 가리키는 페이지에는 공간이 부족하여
                 * 저장할 수 없으면, 다음 페이지에 첫번째 undo record를
                 * 저장하게 된다.
                 * 그러면 mFstUndoRecSID는 실제로 존재하지 않는 위치를 가리키게 된다.
                 * 이런 경우에는 setNextUndoRecSID() 함수를 호출하고 continue를 한다.
                 */
                IDE_TEST( setNextUndoRecSID( &sSegInfo,
                                             &sExtInfo,
                                             aCursor,
                                             sCurUndoPage,
                                             &sIsFixUndoPage )
                          != IDE_SUCCESS);
                continue;
            }

            //-------------------------------------------------------
            // undo record가 존재하는 경우,
            // 나의 cursor에 의해 쓰여진 old row가 존재하는지 검사
            //-------------------------------------------------------
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                        sSlotDirPtr,
                                        SD_MAKE_SLOTNUM(aCursor->mCurUndoRecSID),
                                        &sCurUndoRecHdr)
                      != IDE_SUCCESS );

            SDC_GET_UNDOREC_HDR_FIELD( sCurUndoRecHdr,
                                       SDC_UNDOREC_HDR_TABLEOID,
                                       &sTableOID);

            if ( aCursor->mTableInfo->mTableOID == sTableOID )
            {
                SDC_GET_UNDOREC_HDR_1B_FIELD( sCurUndoRecHdr,
                                              SDC_UNDOREC_HDR_FLAG,
                                              &sFlag );

                if( ( SDC_UNDOREC_FLAG_IS_UNDO_FOR_HEAD_ROWPIECE(sFlag) == ID_TRUE ) &&
                    ( SDC_UNDOREC_FLAG_IS_VALID(sFlag) == ID_TRUE ) )
                {
                    SDC_GET_UNDOREC_HDR_1B_FIELD( sCurUndoRecHdr,
                                                  SDC_UNDOREC_HDR_TYPE,
                                                  &sType );
 
                    /* xxx 함수처리 */
                    switch( sType )
                    {
                        case SDC_UNDO_DELETE_ROW_PIECE :
                            sExistOldRow = ID_TRUE;
                            break;

                        case SDC_UNDO_UPDATE_ROW_PIECE    :
                        case SDC_UNDO_OVERWRITE_ROW_PIECE :
                            sExistOldRow = ID_TRUE;
                            break;

                        case SDC_UNDO_LOCK_ROW :
                            if( sdcUndoRecord::isExplicitLockRec(
                                   sCurUndoRecHdr) == ID_TRUE )
                            {
                                sExistOldRow = ID_FALSE;
                            }
                            else
                            {
                                sExistOldRow = ID_TRUE;;
                            }
                            break;

                        case SDC_UNDO_CHANGE_ROW_PIECE_LINK :
                        case SDC_UNDO_INSERT_ROW_PIECE :
                            sExistOldRow = ID_FALSE;
                            break;

                        default :
                            ideLog::log( 
                                IDE_SERVER_0,
                                "CurUndoRecSID  : %llu\n"
                                "          PID  : %u\n"
                                "          Slot : %u\n"
                                "sPrvUndoPID    : %u\n"
                                "sTableOID      : %lu\n"
                                "sFlag          : %u\n"
                                "sType          : %u\n"
                                ,
                                aCursor->mCurUndoRecSID,
                                SD_MAKE_PID( aCursor->mCurUndoRecSID ),
                                SD_MAKE_SLOTNUM( aCursor->mCurUndoRecSID ),
                                sPrvUndoPID,
                                sTableOID,
                                sFlag,
                                sType );
                            ideLog::logMem( IDE_SERVER_0,
                                            sCurUndoRecHdr,
                                            SDC_UNDOREC_HDR_MAX_SIZE );
                            ideLog::logMem( IDE_SERVER_0,
                                            sCurUndoPage,
                                            SD_PAGE_SIZE );


                            IDE_ASSERT(0);
                            break;
                    }
                }
                else
                {
                    sExistOldRow = ID_FALSE;
                }
            }
            else
            {
                // 다른 커서가 작성한 Undo Record인 경우 Skip 한다.
                sExistOldRow = ID_FALSE;
            }

            if ( sExistOldRow == ID_TRUE )
            {
                //------------------------
                // 나의 cursor에 대한 undo record 인 경우,
                // undo record를 이용하여 update된 row를 fix
                //------------------------
                sdcUndoRecord::parseRowGRID(sCurUndoRecHdr, &sUpdatedRowGRID);

                sdcUndoRecord::parseRowHdr( sCurUndoRecHdr, &sRowHdrInfo );

                IDE_DASSERT( SC_GRID_IS_NOT_NULL( sUpdatedRowGRID ) );

                IDE_ASSERT( sTableSpaceID  ==
                            SC_MAKE_SPACE( sUpdatedRowGRID ) );

                IDE_TEST( sdbBufferMgr::getPageByGRID(
                                           aCursor->mCursorProp.mStatistics,
                                           sUpdatedRowGRID,
                                           SDB_S_LATCH,
                                           SDB_WAIT_NORMAL,
                                           NULL, /* aMtx */
                                           (UChar**)&sUpdatedRow,
                                           &sTrySuccess )
                          != IDE_SUCCESS );
                sIsPageLatchReleased = ID_FALSE;

                //-------------------------------------------------------
                // Old Row가 존재하는 경우, Old Row를 생성
                //-------------------------------------------------------
                IDE_TEST( sdcRow::fetch(
                              aCursor->mCursorProp.mStatistics,
                              NULL, /* aMtx */
                              NULL, /* aSP */
                              aCursor->mTrans,
                              SC_MAKE_SPACE(sUpdatedRowGRID),
                              sUpdatedRow,
                              ID_TRUE, /* aIsPersSlot */
                              SDB_SINGLE_PAGE_READ,
                              aCursor->mCursorProp.mFetchColumnList,
                              SMI_FETCH_VERSION_CONSISTENT,
                              smxTrans::getTSSlotSID( aCursor->mTrans ),
                              &aCursor->mSCN,
                              &aCursor->mInfinite,
                              NULL, /* aIndexInfo4Fetch */
                              NULL, /* aLobInfo4Fetch */
                              ((smcTableHeader*)aCursor->mTable)->mRowTemplate,
                              (UChar*)*aRow,
                              &sIsRowDeleted,
                              &sIsPageLatchReleased )
                          != IDE_SUCCESS );

                /* BUG-23319
                 * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
                /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
                 * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
                 * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
                 * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
                 * 상황에 따라 적절한 처리를 해야 한다. */
                if( sIsPageLatchReleased == ID_FALSE )
                {
                    sIsPageLatchReleased = ID_TRUE;
                    IDE_TEST( sdbBufferMgr::releasePage(
                                  aCursor->mCursorProp.mStatistics,
                                  (UChar*)sUpdatedRow ) != IDE_SUCCESS );
                }

                SC_COPY_GRID( sUpdatedRowGRID, *aRowGRID );
            }
            else
            {
                // nothing to do
            }

            //-------------------------------------------------------
            // 다음 undo record rid를 설정함
            //-------------------------------------------------------
            IDE_TEST( setNextUndoRecSID( &sSegInfo,
                                         &sExtInfo,
                                         aCursor,
                                         sCurUndoRecHdr,
                                         &sIsFixUndoPage )
                      != IDE_SUCCESS);

            if ( sExistOldRow == ID_TRUE )
            {
                //-------------------------------------------------------
                // old row 값을 찾은 경우,
                // old row 값 찾는 작업을 중단
                //-------------------------------------------------------
                break;
            }
            else
            {
                // 다음 undo record를 읽어야 하므로
                // 계속 진행
            }
        }
    }

    if ( sIsFixUndoPage == ID_TRUE )
    {
        sIsFixUndoPage = ID_FALSE;
        IDE_TEST(sdbBufferMgr::unfixPage(aCursor->mCursorProp.mStatistics,
                                         sCurUndoPage)
                 != IDE_SUCCESS);
    }

    if ( sExistOldRow == ID_FALSE )
    {
        *aRow = NULL;
        SC_MAKE_NULL_GRID( *aRowGRID );
    }

    IDE_EXCEPTION_CONT( cont_skip_no_data );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if ( sIsFixUndoPage == ID_TRUE )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage(
                        aCursor->mCursorProp.mStatistics,
                        sCurUndoPage ) == IDE_SUCCESS );
    }

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aCursor->mCursorProp.mStatistics,
                                               (UChar*)sUpdatedRow )
                    == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;

}

IDE_RC smiTableCursor::setNextUndoRecSID4NewRow( sdpSegInfo     * aSegInfo,
                                                 sdpExtInfo     * aExtInfo,
                                                 smiTableCursor * aCursor,
                                                 UChar          * aCurUndoRecHdr,
                                                 idBool         * aIsFixPage )
{
    IDE_TEST( setNextUndoRecSID( aSegInfo,
                                 aExtInfo,
                                 aCursor,
                                 aCurUndoRecHdr,
                                 aIsFixPage ) != IDE_SUCCESS );

    if ( aCursor->mCursorType == SMI_COMPOSITE_CURSOR )
    {
        if ( aCursor->mCurUndoRecSID == SD_NULL_SID )
        {
            if ( ( aCursor->mFlag & SMI_READ_UNDO_PAGE_LIST_MASK )
                 == SMI_READ_UNDO_PAGE_LIST_UPDATE )
            {
                aCursor->mFlag &= ~SMI_READ_UNDO_PAGE_LIST_MASK;
                aCursor->mFlag |= SMI_READ_UNDO_PAGE_LIST_INSERT;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smiTableCursor::setNextUndoRecSID( sdpSegInfo     * aSegInfo,
                                          sdpExtInfo     * aExtInfo,
                                          smiTableCursor * aCursor,
                                          UChar          * aCurUndoRec,
                                          idBool         * aIsFixPage )
{
    UChar         * sCurUndoRec;
    UChar         * sSlotDirPtr;
    scPageID        sCurUndoPageID;
    SInt            sUndoRecCnt;
    SInt            sCurRecSlotNum;
    UChar         * sNxtUndoPage;
    idBool          sTrySuccess;
    UInt            sState = 0;
    sdpSegMgmtOp  * sSegMgmtOp;

    IDE_ASSERT( *aIsFixPage == ID_TRUE );

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    sCurUndoRec    = aCurUndoRec;
    sCurUndoPageID = SD_MAKE_PID( aCursor->mCurUndoRecSID );
    sSlotDirPtr    = sdpPhyPage::getSlotDirStartPtr( sCurUndoRec );
    sUndoRecCnt    = sdpSlotDirectory::getCount( sSlotDirPtr );
    sCurRecSlotNum = SD_MAKE_SLOTNUM( aCursor->mCurUndoRecSID );

    sCurRecSlotNum++;
    if( sCurRecSlotNum < sUndoRecCnt )
    {
        //---------------------------------
        // 현재 undo page내의 다음 undo record로 진행
        //---------------------------------

        // 다음 undo record의 SID를 mCurUndoRecSID에 설정
        aCursor->mCurUndoRecSID =
            SD_MAKE_SID( sCurUndoPageID, sCurRecSlotNum );
    }
    else
    {
        /*
         * ### sCurRecSlotNum == sUndoRecCnt + 1 인 경우 ###
         *
         *  첫번째 undo record를 저장하려고 할때,
         * mFstUndoRecSID가 가리키는 페이지에 공간이 부족하여
         * 저장할 수 없으면, 다음 페이지에 첫번째 undo record를
         * 저장하게 된다.
         * 그러면 mFstUndoRecSID는 실제로 존재하지 않는 위치를 가리키게 된다.
         */
        IDE_ASSERT( (sCurRecSlotNum == sUndoRecCnt) ||
                    (sCurRecSlotNum == (sUndoRecCnt+1)) );

        //---------------------------------
        // 현재 undo page내에 존재하는 undo record 개수를 넘어서는 경우,
        // 다음 undo page로 이동
        //---------------------------------

        if ( sCurUndoPageID == SD_MAKE_PID( aCursor->mLstUndoRecSID ) )
        {
            // To Fix BUG-14798
            //    last undo record rid는 cursor close당시,
            //    앞으로 생성될 다음 undo record의 rid 이다.
            //    현재 undo page내에 존재하는 undo record 개수를 넘어서고,
            //    현재 undo page가 마지막 undo page 이라는 것은
            //    cursor close 이후 undo record가 생성되지 않았으며
            //    나의 cursor가 생성한 undo record를 모두 읽었음을 의미한다.

            aCursor->mCurUndoRecSID = SD_NULL_SID;
            aCursor->mCurUndoExtRID = SD_NULL_RID;
        }
        else
        {
            IDE_TEST( sSegMgmtOp->mGetNxtAllocPage(
                                  NULL, /* idvSQL */
                                  SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                  aSegInfo,
                                  NULL,
                                  &aCursor->mCurUndoExtRID,
                                  aExtInfo,
                                  &sCurUndoPageID )
                      != IDE_SUCCESS );

            *aIsFixPage = ID_FALSE;
            IDE_TEST( sdbBufferMgr::unfixPage(
                                  aCursor->mCursorProp.mStatistics,
                                  sCurUndoRec ) != IDE_SUCCESS );

            // 다음 undo page fix
             IDE_TEST( sdbBufferMgr::fixPageByPID( aCursor->mCursorProp.mStatistics,
                                                   SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                   sCurUndoPageID,
                                                   (UChar**)&sNxtUndoPage,
                                                   &sTrySuccess )
                       != IDE_SUCCESS );
            sState = 1;

            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sNxtUndoPage);
            sUndoRecCnt = sdpSlotDirectory::getCount(sSlotDirPtr);
            IDE_ASSERT(sUndoRecCnt > 0);

            // 다음 undo record의 RID를 mCurUndoRecRID에 설정
            aCursor->mCurUndoRecSID = SD_MAKE_SID( sCurUndoPageID, 1 );

            sState = 0;
            IDE_TEST( sdbBufferMgr::unfixPage(
                                    aCursor->mCursorProp.mStatistics,
                                    sNxtUndoPage ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT(
            sdbBufferMgr::unfixPage( aCursor->mCursorProp.mStatistics,
                                     sNxtUndoPage ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}



/*********************************************************************************/
/* For A4 : interface what to setRemote Query Specific Functions(PROJ-1068)     */
/*********************************************************************************/
IDE_RC smiTableCursor::setRemoteQueryCallback( smiTableSpecificFuncs *aFuncs,
                                               smiGetRemoteTableNullRowFunc aGetNullRowFunc )
{
    openRemoteQuery = aFuncs->openFunc;
    closeRemoteQuery = aFuncs->closeFunc;
    updateRowRemoteQuery = aFuncs->updateRowFunc;
    deleteRowRemoteQuery = aFuncs->deleteRowFunc;
    beforeFirstModifiedRemoteQuery = aFuncs->beforeFirstModifiedFunc;
    readOldRowRemoteQuery = aFuncs->readOldRowFunc;
    readNewRowRemoteQuery = aFuncs->readNewRowFunc;

    mGetRemoteTableNullRowFunc = aGetNullRowFunc;
    
    return IDE_SUCCESS;
}


/**********************************************************************
 * Description: aIterator가 현재 가리키고 있는 Row에 대해서 XLock을
 *              획득합니다.
 *
 * aIterator - [IN] Cursor의 Iterator
 *
 * Related Issue:
 *   BUG-19068: smiTableCursor가 현재가리키고 있는 Row에 대해서
 *              Lock을 잡을수 잇는 Interface가 필요합니다.
 *
 *********************************************************************/
IDE_RC smiTableCursor::lockRow()
{
    return mLockRow( mIterator );
}


/**********************************************************************
 * Description: ViewSCN이 Statement/TableCursor/Iterator간에 동일한지 
 *              평가한다.
 *
 * this - aIterator - [IN] Cursor의 Iterator
 *
 * Related Issue:
 * BUG-31993 [sm_interface] The server does not reset Iterator ViewSCN 
 * after building index for Temp Table 
 *
 *********************************************************************/
IDE_RC smiTableCursor::checkVisibilityConsistency()
{
    idBool sConsistency = ID_TRUE;
    smSCN  sStatementSCN;
    smSCN  sCursorSCN;
    smSCN  sIteratorSCN;

    if( ( mStatement != NULL ) &&
        ( mIterator != NULL ) )
    {
        SM_SET_SCN( &sStatementSCN, &mStatement->mSCN );
        SM_SET_SCN( &sCursorSCN,    &mSCN             );
        SM_SET_SCN( &sIteratorSCN,  &mIterator->SCN   );

        SM_CLEAR_SCN_VIEW_BIT( &sStatementSCN );
        SM_CLEAR_SCN_VIEW_BIT( &sCursorSCN    );
        SM_CLEAR_SCN_VIEW_BIT( &sIteratorSCN  );

        if( ( !SM_SCN_IS_EQ( &sStatementSCN, &sCursorSCN ) ) || 
            ( !SM_SCN_IS_EQ( &sStatementSCN, &sIteratorSCN ) ) )
        {
            sConsistency = ID_FALSE;
        }
    }

    if( sConsistency == ID_FALSE )
    {
        ideLog::logCallStack( IDE_SM_0 );

        ideLog::log( IDE_SM_0,
                     "VisibilityConsistency - Tablecursor\n"
                     "StatmentSCN : 0x%llx\n"
                     "CursorSCN   : 0x%llx\n"
                     "IteratorSCN : 0x%llx\n",
                     SM_SCN_TO_LONG( mStatement->mSCN ),
                     SM_SCN_TO_LONG( mSCN ),
                     SM_SCN_TO_LONG( mIterator->SCN ) );

        ideLog::logMem( IDE_SM_0,
                        ( (UChar*) mStatement ) - 512 ,
                        ID_SIZEOF( smiStatement ) + 1024 );
        ideLog::logMem( IDE_SM_0,
                        ( (UChar*) this ) - 512,
                        ID_SIZEOF( smiTableCursor ) + 1024 );
        ideLog::logMem( IDE_SM_0,
                        ( (UChar*) mIterator ) - 512,
                        ID_SIZEOF( smiIterator ) + 1024 );

        IDE_ASSERT( 0 );
    }

    return IDE_SUCCESS;
}


IDE_RC smiTableCursor::destFullUpdateMRDB( smiColumnList  ** aNewColumnList,
                                           smiValue       ** aNewValueList,
                                           SChar          ** aOldValueBuffer )
{
    IDE_TEST( iduMemMgr::free( *aNewColumnList )  != IDE_SUCCESS );
    *aNewColumnList = NULL;

    IDE_TEST( iduMemMgr::free( *aNewValueList )   != IDE_SUCCESS );
    *aNewValueList = NULL;

    if( *aOldValueBuffer != NULL )
    {
        /* BUG-40650 임시 버퍼를 사용하는 경우에만 반환한다. */
        IDE_TEST( iduMemMgr::free( *aOldValueBuffer ) != IDE_SUCCESS );
        *aOldValueBuffer = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smiTableCursor::destFullUpdateDRDB( smiColumnList  * aNewColumnList,
                                           smiValue       * aNewValueList,
                                           SChar          * aOldValueBuffer )
{
    IDE_TEST( iduMemMgr::free( aNewColumnList )  != IDE_SUCCESS );
    IDE_TEST( iduMemMgr::free( aNewValueList )   != IDE_SUCCESS );
    IDE_TEST( iduMemMgr::free( aOldValueBuffer ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2264
IDE_RC smiTableCursor::setAgingIndexFlag( smiTableCursor * aCursor,
                                          SChar          * aRecPtr,
                                          UInt             aFlag )
{
    smxOIDNode     * sHead;
    smxOIDNode     * sNode;
    smxOIDInfo     * sNodeCursor;
    smOID            sOID;
    UInt             sOIDPos;
    smcTableHeader * sTable;

    sTable = (smcTableHeader*)aCursor->mTable;

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore 
     * the failure of index aging. */
    sHead = &( aCursor->mTrans->mOIDList->mOIDNodeListHead );
    sOID = SMP_SLOT_GET_OID( aRecPtr );

    /* BackTraverse */
    sNode   = sHead;
    sOIDPos = 0;
    do
    {
        if( sOIDPos > 0 ) /* remain oid */
        {
            sOIDPos --;
        }
        else
        {
            sNode   = sNode->mPrvNode;
            if( ( sNode == sHead ) ||
                ( sNode->mOIDCnt == 0 ) )
            {
                /* CircularList이기 때문에, 같다는 말은 비었다는 말.
                 * 비어 있다면, Rollback/Stamping/Aging 연산이 필요 없다는 것
                 * 아직 그런 경우 못봤음.*/
                /* 마지막 Node를 가져옴 */
                ideLog::log( IDE_DUMP_0, 
                             "mSpaceID : %u\n"
                             "mOID     : %lu",
                             sTable->mSpaceID,
                             sOID );
                IDE_ERROR( 0 );
            }
            else
            {
                sOIDPos = sNode->mOIDCnt - 1;
            }
        }
        sNodeCursor = &sNode->mArrOIDInfo[ sOIDPos ];
    }
    while( sNodeCursor->mTargetOID != sOID ) ;

    IDE_ASSERT( ( sNodeCursor->mFlag & SM_OID_ACT_AGING_INDEX )
                == 0 );

    /* PROJ-2429 Dictionary based data compress for on-disk DB 
     * Flag에 따라 rollback수행 시 aging 을 수행할 수도(SM_OID_ACT_AGING_INDEX) 있고 
     * 하지 않을 수도(SM_OID_ACT_AGING_ROLLBACK, dictionary table인 경우) 있다.*/
    sNodeCursor->mFlag |= aFlag;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC smiTableCursor::getTableNullRow( void ** aRow, scGRID * aRid )
{
#ifdef  DEBUG
    UInt sTableType;

    sTableType = ((smcTableHeader *)mTable)->mFlag & SMI_TABLE_TYPE_MASK;

    IDE_DASSERT( sTableType == SMI_TABLE_REMOTE );
#endif
    IDE_TEST_RAISE( mGetRemoteTableNullRowFunc == NULL, NO_CALLBACK_FUNC );

    IDE_TEST( mGetRemoteTableNullRowFunc( this, aRow, aRid ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( NO_CALLBACK_FUNC )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NO_CALLBACK ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
