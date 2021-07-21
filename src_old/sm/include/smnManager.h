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
 

/***********************************************************************
 * $Id: smnManager.h 89963 2021-02-09 05:22:10Z justin.kwon $
 **********************************************************************/

#ifndef _O_SMN_MANAGER_H_
# define _O_SMN_MANAGER_H_ 1

#include <smc.h>
#include <smnDef.h>
#include <smnbDef.h>
#include <smu.h>
#include <smx.h>


class smnManager
{
 public:
    static IDE_RC initialize( void );
    
    static IDE_RC destroy( void );

    static IDE_RC appendIndexType( smnIndexType * aIndexType );

    static IDE_RC updateIndexModule(smnIndexModule *aIndexModule);
    
    static IDE_RC allocFreeNodeList(smnFreeNodeFunc   aFreeNodeFunc,
                                    void**            aFreeNodeList);

    static void destroyFreeNodeList(void*  aFreeNodeList);
    
    static IDE_RC prepareIdxFreePages( );

    static IDE_RC releaseIdxFreePages( );

    static IDE_RC initializeJobInfo(smnpJobItem * aJobItem);

    static IDE_RC destroyJobInfo(smnpJobItem    * aJobItem);

    static IDE_RC removeAllIndexFile();

    static IDE_RC createIndex( idvSQL             * aStatistics,
                               void               * aTrans,
                               smcTableHeader     * aTable,
                               smnIndexHeader     * aIndex,
                               idBool               aIsRestartBuild,
                               idBool               aIsNeedValidation,
                               idBool               aIsPersistent,
                               smuJobThreadMgr    * aThreadMgr,
                               smiSegAttr         * aSegAttr,
                               smiSegStorageAttr  * aSegStoAttr, 
                               smnIndexHeader    ** aRebuildIndexHeader = NULL );
    
    static IDE_RC initIndex(idvSQL             * aStatistics,
                            smcTableHeader     * aTable,
                            void               * aIndex,
                            smiSegAttr         * aSegAttr,
                            smiSegStorageAttr  * aSegStoAttr,
                            void              ** aRebuildIndexHeader,
                            ULong                aSmoNo );
    
    static IDE_RC reInitIndex(idvSQL* aStatistics, void*    aIndex);
    
    static void setInitIndexPtr( void* aIndexHeader );

    /* BUG-17456 Disk Tablespace online이후 update 발생시 index 무한루프 */
    static void setIndexSmoNo( void* aIndexHeader, ULong aSmoNo );

    // BUG-24403
    static void getMaxSmoNoOfAllIndexes( smcTableHeader * aTable,
                                         ULong          * aMaxSmoNo );

    static void getIndexSmoNo( smnIndexHeader * aIndexHeader,
                               ULong          * aSmoNo );

    static void releasePageLatch( smcTableHeader   * aTable,
                                  scPageID           aPageID );

    static IDE_RC getNextPageForMemTable( smcTableHeader  * aTable,
                                          scPageID        * aPageID,
                                          idBool          * aLocked );

    static IDE_RC getNextPageForVolTable( smcTableHeader  * aTable,
                                          scPageID        * aPageID,
                                          idBool          * aLocked );

    static IDE_RC getNextRowForMemTable( smcTableHeader   * aTable,
                                         scPageID           aPageID,
                                         SChar           ** aFence,
                                         SChar           ** aRow,
                                         idBool             aIsNeedValidation );

    static IDE_RC getNextRowForVolTable( smcTableHeader   * aTable,
                                         scPageID           aPageID,
                                         SChar           ** aFence,
                                         SChar           ** aRow,
                                         idBool             aIsNeedValidation );

    // BUG-25279 Btree For Spatial과 Disk Btree의 자료구조 및 로깅 분리
    // Bug 수행 과정에서 Index 구조 개선을 위해 Build Index를 하나로 통합합니다.
    static IDE_RC buildIndex( idvSQL              * aStatistics,
                              void                * aTrans,
                              smcTableHeader      * aTable,
                              smnIndexHeader      * aIndex,
                              idBool                aIsNeedValidation,
                              idBool                aIsPersistent,
                              UInt                  aParallelDegree,
                              UInt                  aBuildFlag,
                              UInt                  aTotalRecCnt );

    static void setIndexCreated( void* aIndexHeader, idBool aIsCreated );

    static idBool getUniqueCheck(const void* aIndexHeader);

    static IDE_RC dropIndex( smcTableHeader * aTable,
                             void           * aIndexHeader );

    static IDE_RC rebuildIndexes( );
    static void   createIndexesParallel( void * aParam );

    static void   buildIndexParallel( void  * aParam );

    static IDE_RC destroyIndexes( void );
    
    static IDE_RC createIndexes( idvSQL            * aStatistics,
                                 void              * aTrans,
                                 smcTableHeader    * aTable,
                                 idBool              aIsRestartBuild,
                                 idBool              aIsNeedValidation,
                                 idBool              aIsPersistent,
                                 smiSegAttr        * aSegAttr,
                                 smiSegStorageAttr * aSegStoAttr,
                                 smuJobThreadMgr   * aThreadMgr );

    static IDE_RC createIndexes( idvSQL            * aStatistics,
                                 void              * aTrans,
                                 smcTableHeader    * aTable,
                                 idBool              aIsRestartBuild,
                                 idBool              aIsNeedValidation,
                                 smiSegAttr        * aSegAttr,
                                 smiSegStorageAttr * aSegStoAttr );

    static IDE_RC dropIndexes( smcTableHeader * aTable );

    static IDE_RC disableAllIndex( idvSQL          * aStatistics,
                                   void            * aTrans,
                                   smcTableHeader  * aTableHeader );

    static IDE_RC enableAllIndex( idvSQL          * aStatistics,
                                  void            * aTrans,
                                  smcTableHeader  * aTableHeader );

    static inline IDE_RC dropIndexRuntimeByAbort( smOID         aTableOID,
                                                  smOID         aIndexOID );

    static IDE_RC indexOperation( idvSQL*, void*, void*, void*, smSCN,
                                  SChar*, SChar*, idBool, smSCN, void*, SChar**, ULong, idBool );
    
    /*============================================================*/

    /* index module에서 smp, svp에 상관없이 사용하는 함수 */
    static inline IDE_RC checkSCN( smiIterator *aIterator,
                                   const void  *aRow,
                                   idBool      *aCanReusableRollback,
                                   idBool      *aIsVisible,
                                   idBool       aIsFullScan = ID_FALSE );

    static inline idBool checkCanReusableRollback( smiIterator * aIterator,
                                                   const void  * aRow );

    static IDE_RC lockRow( smiIterator * aIterator);
    
    static void updatedRow( smiIterator * aIterator );
    
    /* smp를 사용하는 함수 */
    static IDE_RC lockMemRow(smiIterator *aIterator);

    static void updatedMemRow(smiIterator *aIterator);

    /* svp를 사용하는 함수 */
    static IDE_RC lockVolRow( smiIterator * aIterator);

    static void updatedVolRow( smiIterator * aIterator );

    /*============================================================*/

    static IDE_RC prepareRebuildIndexs(smcTableHeader  * aTable,
                                       smnIndexHeader ** aIndexHeaderList);

    static idBool isIndexToVerifyIntegrity( const void * aIndexHeader );

    static IDE_RC verifyIndexIntegrityAtStartUp();

    static SInt  getIndexModuleNo(UInt aTableType, UInt aIndexType);

    static void * getIndexModule(UInt aTableType, UInt aIndexType);

    static UInt * getFlagPtrOfIndexHeader( void   * a_pIndexHeader );

    static smOID  getTableOIDOfIndexHeader( void * a_pIndexHeader );

    static UInt  getIndexIDOfIndexHeader( void * a_pIndexHeader );

    static inline UInt getSizeOfIndexHeader( void );

    static IDE_RC indexInsertWithoutUniqueCheck( void   * a_pTrans,
                                                 void   * aTable,
                                                 void   * a_pIndexHeader,
                                                 SChar  * a_pRow,
                                                 SChar  * a_pNull );
    
    static IDE_RC indexDeleteFunc( void   * aIndexHeader,
                                   SChar  * aRow,
                                   idBool   aIgnoreNotFoundKey,
                                   idBool * aExistKey );
    
    static void   initIndexHeader( void                 * aIndexHeader,
                                   smOID                  aTableSelfOID, 
                                   SChar                * aName,
                                   UInt                   aID,
                                   UInt                   aType,
                                   UInt                   aFlag,
                                   const  smiColumnList * aColumns,
                                   smiSegAttr           * aSegAttr,
                                   smiSegStorageAttr    * aSegStoAttr,
                                   ULong                  aDirectKeyMaxSize );
    
    static IDE_RC initIndexMetaPage ( UChar            * aPagePtr,
                                      UInt               aType,
                                      UInt               aBuildFlag,
                                      sdrMtx           * aMtx );

    static UInt   getColumnCountOfIndexHeader ( void *a_pIndexHeader );
    
    static UInt*  getColumnIDPtrOfIndexHeader ( void *a_pIndexHeader, 
                                                UInt a_nIndex );
    
    static UInt  getFlagOfIndexHeader ( void *a_pIndexHeader );
    
    static void  setFlagOfIndexHeader ( void *a_pIndexHeader, UInt a_Flag );
    
    static idBool getIsConsistentOfIndexHeader ( void   * aIndexHeader );
    
    static void   setIsConsistentOfIndexHeader ( void   * aIndexHeader, 
                                                 idBool   aFlag );
    

    static IDE_RC deleteRowFromIndexForSVC( SChar          * aRow,
                                            smcTableHeader * aHeader,
                                            ULong            aModifyIdxBit );

    static IDE_RC deleteRowFromIndex( SChar          * aRow, 
                                      smcTableHeader * aHeader,
                                      ULong            aModifyIdxBit );
    
    static idBool isNullModuleOfIndexHeader(void *a_pIndexHeader);

    static scSpaceID getIndexSpaceID( void * aIndexHeader );

    static inline scGRID * getIndexSegGRIDPtr( void * aIndexHeader );
    static UInt * getIndexFlagPtr( void * aIndexHeader );
    static UShort getIndexDropFlag( void * aIndexHeader );
    static smiSegStorageAttr * getIndexSegStoAttrPtr( void * aIndexHeader );
    static smiSegAttr * getIndexSegAttrPtr( void * aIndexHeader );
    static void  setIndexSegStoAttrPtr( void * aIndexHeader, smiSegStorageAttr aSegStoAttr );
    static void  setIndexSegAttrPtr( void * aIndexHeader, smiSegAttr aSegAttr );
    

    static SChar*  getNameOfIndexHeader(void *aIndexHeader);
    static void    setNameOfIndexHeader(void *aIndexHeader, SChar *aName);
    static UInt    getIndexNameOffset();

    static idBool checkIdxIntersectCols( void* aIndexHeader,
                                         UInt   aColumnCount,
                                         UInt * aColumns );
    static  UInt  getMaxIndexCnt();
    
    
    static idBool isPrimaryIndex(void* aIndexHeader);

    static inline idBool isIndexEnabled( void* aIndexHeader );

    static void * getSegDescByIdxPtr( void * aIndex );

    /* BUG-31845 [sm-disk-index] Debugging information is needed for 
     * PBT when fail to check visibility using DRDB Index.
     * 검증용 Dump 코드 추가 */
    static IDE_RC dumpCommonHeader( smnIndexHeader * aHeader,
                                    SChar          * aOutBuf,
                                    UInt             aOutSize );

    static void   logCommonHeader( smnIndexHeader * aHeader );

    static IDE_RC initIndexStatistics( smnIndexHeader   * aIndexHeader,
                                       smnRuntimeHeader * aRunHeader,
                                       UInt               aIndexID );
    static IDE_RC destIndexStatistics( smnIndexHeader   * aIndexHeader,
                                       smnRuntimeHeader * aRunHeader );
public:
    
    // added for A4. free list of memory index nodes(B,R Tree) 
    static smnFreeNodeList  mBaseFreeNodeList; 

    static iduMemPool       mDiskTempPagePool;

    static IDE_RC doKeyReorganization( smnIndexHeader * aIndexHeader );

private:
    static idBool mIsInvalidUniqueness;  // BUG-47736
};

inline idBool smnManager::isIndexEnabled( void* aIndexHeader )
{
    smnIndexHeader *sIndexHeader = (smnIndexHeader*)aIndexHeader;

    return ( (sIndexHeader->mFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE ) ?
        ID_TRUE : ID_FALSE;
}

/*******************************************************************************
 * Description: Enable index 도중 abort가 발생했을 때, 이미 생성된 index와
 *      runtime header 등을 파괴해 주는 함수이다.
 *
 *      Disk index의 index segment는 본 함수에서 파괴하지 않아도 NTA logical
 *      rollback 으로 파괴된다.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 * 
 * aStatistics      - [IN] idvSQL
 * aTableOID        - [IN] 대상 index가 속한 table의 OID
 * aIndexOID        - [IN] 대상 index의 OID
 ******************************************************************************/
inline IDE_RC smnManager::dropIndexRuntimeByAbort( smOID        aTableOID,
                                                   smOID        aIndexOID )
{
    smcTableHeader  * sTableHeader;
    smnIndexHeader  * sIndexHeader;

    IDE_ASSERT( smcTable::getTableHeaderFromOID( aTableOID,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );

    IDE_ASSERT( smcTable::getIndexHeaderFromOID( aIndexOID,
                                                 (void**)&sIndexHeader )
                == IDE_SUCCESS );

    IDE_TEST( dropIndex( sTableHeader,
                         sIndexHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1594 Volatile TBS
 * SCAN 모듈에서 scn을 체크할 때, memory, volatile table에 대해
 * 공통으로 사용할 수 있는 함수이다.
 *
 * PROJ-1381 Fetch Across Commits
 * Row SCN     : sCreateSCN, sLimitSCN
 * SCN from TX : sRowSCN,    sNxtSCN
 *
 *          Row SCN  | SCN from TX | TransID 
 * case 1   INFINITE | INFINITE    | VALID TID
 * case 2   INFINITE | COMMITED    | VALID TID
 * case 3   COMMITED | COMMITED    | NULL  TID
 *
 * case 2는 TX를 commit 했으나 아직 commit SCN을
 * Row에 설정하지 않은 상태이다.
 */
inline IDE_RC smnManager::checkSCN( smiIterator *aIterator,
                                    const void  *aRow,
                                    idBool      *aCanReusableRollback,
                                    idBool      *aIsVisible,
                                    idBool       aIsFullScan ) /* Default : ID_FALSE */
{
    smSCN     sCreateSCN;
    smSCN     sLimitSCN;
    smSCN     sCreateSCNTmp;
    smSCN     sLimitSCNTmp;
    smSCN     sRowSCN;
    smSCN     sNxtSCN;
    smTID     sRowTID = SM_NULL_TID;
    smTID     sNxtTID = SM_NULL_TID;
#ifdef DEBUG
    smSCN     sRowSCNAfterCheck;
#endif
    smxTrans               * sTrans = (smxTrans*)aIterator->trans;
    volatile smpSlotHeader * sSlotHeader = NULL;

    sSlotHeader = (smpSlotHeader*)aRow;

    if( aCanReusableRollback != NULL )
    {
        *aCanReusableRollback = ID_TRUE;
    }
    *aIsVisible = ID_FALSE;

    if ( aIsFullScan == ID_FALSE )
    {
        SM_SET_SCN( &sCreateSCN, &(sSlotHeader->mCreateSCN) );
        SM_SET_SCN( &sLimitSCN, &(sSlotHeader->mLimitSCN) );
    }
    else
    {
        /* BUG-48353
           FULL SCAN 인경우 PAGE LATCH 밖에서 checkSCN() 호출한다.
           CreateSCN과 LimitSCN이 동일한 Record로부터 받아온 값인지를 보장해야 한다. */
        do
        {
            ID_SERIAL_BEGIN( SM_SET_SCN( &sCreateSCN, &(sSlotHeader->mCreateSCN) ) );
            ID_SERIAL_EXEC(  SM_SET_SCN( &sLimitSCN, &(sSlotHeader->mLimitSCN) ), 1 );
            ID_SERIAL_EXEC(  SM_SET_SCN( &sCreateSCNTmp, &(sSlotHeader->mCreateSCN) ), 2 );
            ID_SERIAL_END(   SM_SET_SCN( &sLimitSCNTmp, &(sSlotHeader->mLimitSCN) ) );

        } while ( !SM_SCN_IS_EQ( &sCreateSCN, &sCreateSCNTmp ) ||
                  !SM_SCN_IS_EQ( &sLimitSCN, &sLimitSCNTmp ) );
    }

    if ( sTrans->mIsGCTx == ID_TRUE )
    {
        if ( SM_SCN_IS_INFINITE( sCreateSCN ) &&
             /* BUG-48244 : 내(TX)가 2PC한 TX는 Pending 완료시까지 대기할 필요없다. */
             ( SMP_GET_TID( sCreateSCN ) != aIterator->tid ) )
        {
            IDE_TEST( smxTrans::waitPendingTx( sTrans,
                                               sCreateSCN,
                                               aIterator->SCN )
                      != IDE_SUCCESS );
        }

        if ( SM_SCN_IS_INFINITE( sLimitSCN ) &&
             /* BUG-48244 : 내(TX)가 2PC한 TX는 Pending 완료시까지 대기할 필요없다. */
             ( SMP_GET_TID( sLimitSCN ) != aIterator->tid ) )
        {
            IDE_TEST( smxTrans::waitPendingTx( sTrans,
                                               sLimitSCN,
                                               aIterator->SCN )
                      != IDE_SUCCESS );
        }

        /* BUG-48244 : waitPendingTX()내부에서 COMMIT한 이후에 RECORD SCN을 다시 받아온다.
                       GCTX에서 높은 view로 보는경우, RECORD가 안보일수도 있기 때문이다. */ 
        if ( aIsFullScan == ID_FALSE )
        {
            SM_SET_SCN( &sCreateSCN, &(sSlotHeader->mCreateSCN) );
            SM_SET_SCN( &sLimitSCN, &(sSlotHeader->mLimitSCN) );
        }
        else
        {
            /* BUG-48353
               FULL SCAN 인경우 PAGE LATCH 밖에서 checkSCN() 호출한다.
               CreateSCN과 LimitSCN이 동일한 Record로부터 받아온 값인지를 보장해야 한다. */
            do
            {
                ID_SERIAL_BEGIN( SM_SET_SCN( &sCreateSCN, &(sSlotHeader->mCreateSCN) ) );
                ID_SERIAL_EXEC(  SM_SET_SCN( &sLimitSCN, &(sSlotHeader->mLimitSCN) ), 1 );
                ID_SERIAL_EXEC(  SM_SET_SCN( &sCreateSCNTmp, &(sSlotHeader->mCreateSCN) ), 2 );
                ID_SERIAL_END(   SM_SET_SCN( &sLimitSCNTmp, &(sSlotHeader->mLimitSCN) ) );

            } while ( !SM_SCN_IS_EQ( &sCreateSCN, &sCreateSCNTmp ) ||
                      !SM_SCN_IS_EQ( &sLimitSCN, &sLimitSCNTmp ) );
        }
    }

    SMX_GET_SCN_AND_TID( sCreateSCN, sRowSCN, sRowTID );
    SMX_GET_SCN_AND_TID( sLimitSCN, sNxtSCN, sNxtTID );

    while ( 1 )
    {
        /* delete bit를 설정한 경우 해당 ROW를 보지 않는다. */
        IDE_TEST_CONT( SM_SCN_IS_DELETED( sRowSCN ), skip_check_visibility );

        /* 읽을 수 있는 대상 SCN의 경우 */
        if ( /* row의 SCN이 cursor의 view SCN보다 작거나 */
             ( SM_SCN_IS_LE( &sRowSCN, &(aIterator->SCN) ) ) ||
             /* 같은 Tx에서 cursor가 열리기 전에 생성한 row이면 */
             ( ( sRowTID == aIterator->tid ) &&
               ( SM_SCN_IS_LT( &sCreateSCN, &(aIterator->infinite) ) ) ) )
        {
            /* row에 lock을 걸었거나 next version이 없으면 볼 수 있다. */
            if ( SM_SCN_IS_FREE_ROW( sNxtSCN ) ||
                 SM_SCN_IS_LOCK_ROW( sNxtSCN ) )
            {
                /* PROJ-2694 Fetch Across Rollback
                 * holdable cursor open 이전 자신이 생성한 row에 접근했을 경우
                 * 해당 Tx를 rollback시 cursor를 재활용할 수 없다. */
                if ( ( sRowTID == aIterator->tid ) && 
                     ( sTrans->mCursorOpenInfSCN != SM_SCN_INIT ) &&
                     ( SM_SCN_IS_LT( &sCreateSCN, &( sTrans->mCursorOpenInfSCN ) ) ) &&
                     ( aCanReusableRollback != NULL ) )
                {
                    *aCanReusableRollback = ID_FALSE;
                }
                else
                {
                    /* nothing to do */
                }

                *aIsVisible = ID_TRUE;
                break;
            }
            else
            {
                /* nothing to do */
            }

            if ( sNxtTID == aIterator->tid )
            {
                /* Next Version을 내가 만들었지만 cursor를 열기전에 만들었음 */
                if ( SM_SCN_IS_GE( &(sLimitSCN), &(aIterator->infinite) ) )
                {

                    if( ( sTrans->mCursorOpenInfSCN != SM_SCN_INIT ) && 
                        ( SM_SCN_IS_GE( &sLimitSCN, &sTrans->mCursorOpenInfSCN ) ) &&
                        ( aCanReusableRollback != NULL ) )
                    {
                        /* PROJ-2694 Fetch Across Rollback 
                         * holdable cursor open 이전 자신이 생성한 row에 접근했을 경우
                         * 해당 Tx를 rollback시 cursor를 재활용할 수 없다. */
                        *aCanReusableRollback = ID_FALSE;
                    }

                    *aIsVisible = ID_TRUE;
                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* Next Version을 내가 만들지 않은 경우
                 * 아래 3가지 경우에 대해 모두 검사가 가능하다.
                 * 1. Commit 하지 않았고, Next Version의 SCN이 infinite인 경우
                 * 2. Commit 했지만 아직 Commit SCN을 설정하지 않은 경우
                 * 3. Commit 했고, Commit SCN도 설정한 경우
                 *    (1   : sNxtSCN이 INFINITE SCN이므로 볼 수 있다. )
                 *    (2,3 : sNxtSCN이 COMMITED SCN이므로 대소 비교 가능 ) */
                if ( SM_SCN_IS_GT( &(sNxtSCN), &(aIterator->SCN) ) )
                {
                    *aIsVisible = ID_TRUE;
                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }
        }
        else
        {
            /* nothing to do */
        }

        break;
    }

#ifdef DEBUG
    if ( *aIsVisible == ID_TRUE )
    {
        /* Next Row를 Free하면 Next Row의 SCN이 0x7FFFFFFFFFFFFFFD가 된다.
         * 따라서 현재 Row를 볼 수 있다고 판단한다.
         * Next Row가 삭제되면 현재 Row도 삭제될 Row일 수 있다.
         * 그런데 이런 경우는 버그에 의한 것이다.
         * 왜냐하면 Check하고 있는 Row에 대해 Update를 수행했기 때문이다. */
        SMX_GET_SCN_AND_TID( ((smpSlotHeader*)sSlotHeader)->mCreateSCN,
                             sRowSCNAfterCheck,
                             sRowTID );
        if ( !SM_SCN_IS_EQ( &sRowSCN, &sRowSCNAfterCheck ) )
        {
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        "[##WARNING ##WARNING!!] Invalid row detected\n");

            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        "TID:[%"ID_UINT32_FMT"] View SCN:[%"ID_UINT64_FMT"] Inf SCN:[%"ID_UINT64_FMT"]\n"
                        "Row SCN:[%"ID_UINT64_FMT"]  RowSCNAfterCheck:[%"ID_UINT64_FMT"]\n",
                        (UInt)( aIterator->tid ),
                        SM_SCN_TO_LONG( aIterator->SCN ),
                        SM_SCN_TO_LONG( aIterator->infinite ),
                        SM_SCN_TO_LONG( sRowSCN ),
                        SM_SCN_TO_LONG( sRowSCNAfterCheck ) );

            smcRecord::logSlotInfo((smpSlotHeader*)sSlotHeader);

            IDE_ASSERT(0);
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
#endif

    /* BUG-40385 *aIsVisible 값에 따라 Failure 리턴일 수 있으므로,
     * 위에 IDE_TEST_RAISE -> IDE_TEST_CONT 로 변환하지 않는다. */
    IDE_EXCEPTION_CONT( skip_check_visibility );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline UInt smnManager::getSizeOfIndexHeader( void )
{
    return ID_SIZEOF( smnIndexHeader );
}

inline scGRID * smnManager::getIndexSegGRIDPtr( void * aIndexHeader )
{
    return ( &((smnIndexHeader *)aIndexHeader)->mIndexSegDesc );
}

inline idBool smnManager::checkCanReusableRollback( smiIterator * aIterator,
                                                    const void  * aRow )
{
    smSCN     sCreateSCN;
    smTID     sRowTID = SM_NULL_TID;
    smSCN     sRowSCN;
    idBool    sIsReusableRollback = ID_TRUE;
    smxTrans *sTrans = (smxTrans*)aIterator->trans;

    const   smpSlotHeader   * sSlotHeader = NULL;

    IDE_DASSERT( aIterator != NULL );
    IDE_DASSERT( aRow != NULL );

    sSlotHeader = (smpSlotHeader*)aRow;

    SM_SET_SCN( &sCreateSCN, &(sSlotHeader->mCreateSCN) );

    SMX_GET_SCN_AND_TID( sCreateSCN, sRowSCN, sRowTID );
    if( SM_SCN_IS_DELETED( sRowSCN ) == ID_TRUE )
    {
        /* 해당 row가 이미 삭제되었다면 fetch across rollback과 관계가 없다. */
        sIsReusableRollback = ID_TRUE;
    }
    else
    {
        if( sRowTID != aIterator->tid )
        {
            /* 다른 Tx의 내용에 접근할 경우 fetch across rollback과 관계가 없다. */
            sIsReusableRollback = ID_TRUE;
        }
        else
        {
            if( ( SM_SCN_IS_LT( &sCreateSCN, &( sTrans->mCursorOpenInfSCN ) ) == ID_TRUE ) && 
                ( sTrans->mCursorOpenInfSCN != SM_SCN_INIT ) )
            {
                /* 접근한 row가 holdable cursor가 열리기 전 생성한 row일 경우 fetch across rollback이 불가능하다.     */
                sIsReusableRollback = ID_FALSE;
            }
            else
            {
                /* non holdable cursor이거나 holdable cursor가 열린 후 생성한 row일 경우 fetch across rollback과 관계가 없다. */
                sIsReusableRollback = ID_TRUE;
            }
        }
    }

    return sIsReusableRollback;
}


#endif /* _O_SMN_MANAGER_H_ */
