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
 * $Id: sdpTableSpace.h 15787 2006-05-19 02:26:17Z bskim $
 *
 * Description :
 *
 * 테이블스페이스 관리자
 *
 *
 **********************************************************************/

#ifndef _O_SCT_TABLE_SPACE_MGR_H_
#define _O_SCT_TABLE_SPACE_MGR_H_ 1

#include <sctDef.h>
// BUGBUG-1548 SDD는 SCT보다 윗 LAyer이므로 이와같이 Include해서는 안됨
#include <sdd.h>

struct smmTBSNode;
struct svmTBSNode;
class smxTrans;

class sctTableSpaceMgr
{
public:

    static IDE_RC initialize();

    static IDE_RC destroy();

    static inline idBool isBackupingTBS( scSpaceID  aSpaceID );

    // smiTBSLockValidType을 sctTBSLockValidOpt로 변경하여 반환한다.
    static sctTBSLockValidOpt
              getTBSLvType2Opt( smiTBSLockValidType aTBSLvType )
    {
        IDE_ASSERT( aTBSLvType < SMI_TBSLV_OPER_MAXMAX );

        return mTBSLockValidOpt[ aTBSLvType ];
    }

    // ( Disk/Memory 공통 ) Tablespace Node를 초기화한다.
    static IDE_RC initializeTBSNode(sctTableSpaceNode * aSpaceNode,
                                    smiTableSpaceAttr * aSpaceAttr );

    // ( Disk/Memory 공통 ) Tablespace Node를 파괴한다.
    static IDE_RC destroyTBSNode(sctTableSpaceNode * aSpaceNode );


    // Tablespace의 Sync Mutex를 획득한다.
    static IDE_RC latchSyncMutex( sctTableSpaceNode * aSpaceNode );

    // Tablespace의 Sync Mutex를 풀어준다.
    static IDE_RC unlatchSyncMutex( sctTableSpaceNode * aSpaceNode );

    // NEW 테이블스페이스 ID를 할당한다.
    static IDE_RC allocNewTableSpaceID( scSpaceID*   aNewID );

    // 다음 NEW 테이블스페이스 ID를 설정한다.
    static void   setNewTableSpaceID( scSpaceID aNewID )
                  { mNewTableSpaceID = aNewID; }

    // 다음 NEW 테이블스페이스 ID를 반환한다.
    static scSpaceID getNewTableSpaceID() { return mNewTableSpaceID; }

    // 첫 테이블스페이스 노드를 반환한다.
    static inline sctTableSpaceNode * getFirstSpaceNode();

    // 현재 테이블스페이스를 기준으로 다음 테이블스페이스 노드를 반환한다.
    static inline sctTableSpaceNode * getNextSpaceNode( scSpaceID aSpaceID );

    static inline sctTableSpaceNode *  getNextSpaceNodeIncludingDropped( scSpaceID aSpaceID );

    /* TASK-4007 [SM] PBT를 위한 기능 추가 *
     * Drop중이거나 Drop할 것이거나 생성중인 테이블 스페이스 외
     * 온전한 테이블 스페이스만 가져온다*/
    static inline sctTableSpaceNode * getNextSpaceNodeWithoutDropped( scSpaceID aSpaceID );

    // 메모리 테이블스페이스의 여부를 반환한다.
    static inline idBool isMemTableSpace( scSpaceID aSpaceID );
    static inline idBool isMemTableSpace( void * aSpaceNode );
    static inline idBool isMemTableSpaceType( smiTableSpaceType aSpaceType );

    // Volatile 테이블스페이스의 여부를 반환한다.
    static inline idBool isVolatileTableSpace( scSpaceID aSpaceID );
    static inline idBool isVolatileTableSpace( void * aSpaceNode );
    static inline idBool isVolatileTableSpaceType( smiTableSpaceType aSpaceType );

    // 디스크 테이블스페이스의 여부를 반환한다.
    static inline idBool isDiskTableSpace( scSpaceID aSpaceID );
    static inline idBool isDiskTableSpace( void * aSpaceNode );
    static inline idBool isDiskTableSpaceType( smiTableSpaceType aSpaceType );

    // 언두 테이블스페이스의 여부를 반환한다.
    static inline idBool isUndoTableSpace( scSpaceID aSpaceID );

    // 임시 테이블스페이스의 여부를 반환한다.
    static inline idBool isTempTableSpace( scSpaceID aSpaceID );
    static inline idBool isTempTableSpace( void * aSpaceNode );
    static inline idBool isTempTableSpaceType( smiTableSpaceType aSpaceType );

    static inline idBool isDataTableSpaceType( smiTableSpaceType  aType );

    // 시스템 메모리 테이블스페이스의 여부를 반환한다.
    static inline idBool isSystemMemTableSpace( scSpaceID aSpaceID );

    // TBS의 관리 영역을 반환한다. (Disk, Memory, Volatile)
    static inline smiTBSLocation getTBSLocation( scSpaceID aSpaceID );
    static inline smiTBSLocation getTBSLocation( sctTableSpaceNode * aSpaceNode );

    // PRJ-1548 User Memory Tablespace
    // 시스템 테이블스페이스의 여부를 반환한다.
    static inline idBool isSystemTableSpace( scSpaceID aSpaceID );

    static IDE_RC dumpTableSpaceList();

    // TBSID를 입력받아 테이블스페이스의 속성을 반환한다.
    static IDE_RC getTBSAttrByID( idvSQL   * aStatistics,
                                  scSpaceID          aID,
                                  smiTableSpaceAttr* aSpaceAttr );

    // Tablespace Attribute Flag의 Pointer를 반환한다.
    static IDE_RC getTBSAttrFlagByID(scSpaceID       aID,
                                     UInt          * aAttrFlagPtr);
    // Tablespace Attribute Flag를 반환한다.
    static UInt getTBSAttrFlag( sctTableSpaceNode * aSpaceNode );
    // Tablespace Attribute Flag를 설정한다.
    static void setTBSAttrFlag( sctTableSpaceNode * aSpaceNode,
                                UInt                aAttrFlag );
    // Tablespace의 Attribute Flag로부터 로그 압축여부를 얻어온다
    static IDE_RC getSpaceLogCompFlag( scSpaceID aSpaceID, idBool *aDoComp );


    // TBS명을 입력받아 테이블스페이스의 속성을 반환한다.
    static IDE_RC getTBSAttrByName(SChar*              aName,
                                   smiTableSpaceAttr*  aSpaceAttr);


    // TBS명을 입력받아 테이블스페이스의 속성을 반환한다.
    static IDE_RC findSpaceNodeByName(SChar* aName,
                                      void** aSpaceNode ,
                                      idBool aLockSpace );


    // TBS명을 입력받아 테이블스페이스가 존재하는지 확인한다.
    static idBool checkExistSpaceNodeByName( SChar* aTableSpaceName );

    // Tablespace ID로 SpaceNode를 찾는다.
    // 해당 Tablespace가 DROP된 경우 에러를 발생시킨다.
    static IDE_RC findSpaceNodeBySpaceID( scSpaceID  aSpaceID,
                                          void**     aSpaceNode )
    {
        return findSpaceNodeBySpaceID( NULL,
                                       aSpaceID,
                                       aSpaceNode,
                                       ID_FALSE );
    };

    static IDE_RC findAndLockSpaceNodeBySpaceID( idvSQL   * aStatistics,
                                                 scSpaceID  aSpaceID,
                                                 void**     aSpaceNode )
    {
        return findSpaceNodeBySpaceID( aStatistics,
                                       aSpaceID,
                                       aSpaceNode,
                                       ID_TRUE );
    };

private:
    static IDE_RC findSpaceNodeBySpaceID( idvSQL   * aStatistics,
                                          scSpaceID  aSpaceID,
                                          void**     aSpaceNode,
                                          idBool     aLockSpace );
public:
    // Tablespace ID로 SpaceNode를 찾는다.
    // 무조건 TBS Node 포인터를 준다.
    // BUG-18743: 메모리 페이지가 깨졌을 경우 PBT할 수 있는
    // 로직이 필요합니다.
    //   smmManager::isValidPageID에서 사용하기 위해서 추가.
    // 다른 로직에서는 이 함수 사용보다는 findSpaceNodeBySpaceID 을
    // 권장합니다. 왜냐하면 이함수는 Validation작업이 전혀 없습니다.
    static inline sctTableSpaceNode * getSpaceNodeBySpaceID( scSpaceID  aSpaceID )
    {
        return mSpaceNodeArray[aSpaceID];
    }

    // Tablespace ID로 SpaceNode를 찾는다.
    // 해당 Tablespace가 DROP된 경우 SpaceNode에 NULL을 리턴한다.
    static sctTableSpaceNode* findSpaceNodeWithoutException( scSpaceID  aSpaceID,
                                                             idBool     aUsingTBSAttr = ID_FALSE );


    // Tablespace ID로 SpaceNode를 찾는다.
    // 해당 Tablespace가 DROP된 경우에도 aSpaceNode에 해당 TBS를 리턴.
    static sctTableSpaceNode* findSpaceNodeIncludingDropped( scSpaceID  aSpaceID )
    {  return mSpaceNodeArray[aSpaceID]; };

    // Tablespace가 존재하는지 체크한다.
    static idBool isExistingTBS( scSpaceID aSpaceID );

    // Tablespace가 ONLINE상태인지 체크한다.
    static idBool isOnlineTBS( scSpaceID aSpaceID );

    // Tablespace가 여러 State중 하나의 State를 지니는지 체크한다.
    static idBool hasState( scSpaceID   aSpaceID,
                            sctStateSet aStateSet,
                            idBool      aUsingTBSAttr = ID_FALSE );

    static idBool hasState( sctTableSpaceNode * aSpaceNode,
                            sctStateSet         aStateSet );

    // Tablespace의 State에 aStateSet안의 State가 하나라도 있는지 체크한다.
    static idBool isStateInSet( UInt         aTBSState,
                                sctStateSet  aStateSet );

    // Tablespace안의 Table/Index를 Open하기 전에 Tablespace가
    // 사용 가능한지 체크한다.
    static IDE_RC validateTBSNode( sctTableSpaceNode * aSpaceNode,
                                   sctTBSLockValidOpt  aTBSLvOpt );

    // 테이블스페이스 생성시 데이타파일 생성에 대한 동시성 제어
    static IDE_RC lockForCrtTBS( ) { return mMtxCrtTBS.lock( NULL ); }
    static IDE_RC unlockForCrtTBS( ) { return mMtxCrtTBS.unlock(); }

    // PRJ-1548 User Memory Tablespace
    // 잠금없이 테이블스페이스와 관련된 Sync연산과 Drop 연산간의 동시성
    // 제어하기 위해 Sync Mutex를 사용한다.

    static void addTableSpaceNode( sctTableSpaceNode *aSpaceNode );
    static void removeTableSpaceNode( sctTableSpaceNode *aSpaceNode );

    // 서버구동시에 모든 임시 테이블스페이스를 Reset 한다.
    static IDE_RC resetAllTempTBS( void *aTrans );

    //for PRJ-1149 added functions
    static void getDataFileNodeByName(SChar*            aFileName,
                                      sddDataFileNode** aFileNode,
                                      scSpaceID*        aTbsID,
                                      scPageID*         aFstPageID = NULL,
                                      scPageID*         aLstPageID = NULL,
                                      SChar**           aSpaceName = NULL);


    // 트랜잭션 커밋 직전에 수행하기 위한 연산을 등록
    static IDE_RC addPendingOperation( void               *aTrans,
                                       scSpaceID           aSpaceID,
                                       idBool              aIsCommit,
                                       sctPendingOpType    aPendingOpType,
                                       sctPendingOp      **aPendingOp = NULL );


    // 테이블스페이스 관련 트랜잭션 Commit-Pending 연산을 수행한다.
    static IDE_RC executePendingOperation( idvSQL *aStatistics,
                                           void   *aPendingOp,
                                           idBool  aIsCommit);

    // PRJ-1548 User Memory Tablespace
    //
    // # 잠금계층구조의 조정
    //
    // 기존에는 table과 tablespace 간에 별개의 lock hierachy를 가지고 잠금관리를 하고 있다.
    // 이러한 관리구조는 case by case로 세는 부분을 처리해야하는 경우가 많이 발생할 것이다.
    // 예를들어 offline 연산을 할 때, 관련 table에 대한 X 잠금을 획득하기로 하였을 경우에
    // 생성중인 table이나 drop 중인 table에 대해서는 잠금처리를 어떻게 하느냐라는 문제가 생긴다.
    // 아마 catalog page에 대해서 잠금을 수행해야 할지도 모른다.
    // 이러한 경우를 정석대로 상위 개념의 tablespace 노드의 잠금을 획득하도록 수정한다면
    // 좀 더 명백하게 해결할 수 있을 것이고,
    // 보다 안전한 대책이 아닐런지..
    //
    // A. TBS NODE -> CATALOG TABLE -> TABLE -> DBF NODE 순으로
    //    lock hierachy를 정의한다.
    //
    // B. DML 수행시 TBS Node에 대해서도 INTENTION LOCK을 요청하도록 처리한다.
    // STATEMENT의 CURSOR 오픈시 처리하고, SAVEPOINT도 처리해야한다.
    // 테이블스페이스 관련 DDL과의 동시성 제어가 가능하다.
    //
    // C. SYSTEM TABLESPACE는 drop이 불가능하므로 잠금을 획득하지 않아도 된다.
    //
    // D. DDL 수행시 TBS Node 또는 DBF Node에 대해서 잠금을 요청하도록 처리한다.
    // TABLE/INDEX등에 대한 DDL에 대해서도 관련 TBS에 잠금을 획득하도록 한다.
    // 테이블스페이스 관련 DDL과의 동시성 제어가 가능하다.
    // 단, PSM/VIEW/SEQUENCE등은 SYSTEM_DICTIONARY_TABLESPACE에 생성되므로 잠금을
    // 획득할 필요는 없다.
    //
    // E. LOCK 요청 비용을 줄이기 위해서 TABLE_LOCK_ENABLE과 비슷한 맥락으로
    // TABLESPACE_LOCK_ENABLE 기능을 지원하여, TABLESPACE DDL을 허용하는 않는 범위내에서
    // TBS LIST, TBS NODE, DBF NODE 에 대한 LOCK 요청을 하지 못하도록 처리한다.


    // 테이블의 테이블스페이스들에 대하여 INTENTION 잠금을 획득한다.
    // smiValidateAndLockTable(), smiTable::lockTable, 커서 open시 호출
    static IDE_RC lockAndValidateTBS(
                      void                * aTrans,      /* [IN]  */
                      scSpaceID             aSpaceID,    /* [IN]  */
                      sctTBSLockValidOpt    aTBSLvOpt,   /* [IN]  */
                      idBool                aIsIntent,   /* [IN]  */
                      idBool                aIsExclusive,/* [IN]  */
                      ULong                 aLockWaitMicroSec ); /* [IN] */

    // 테이블과 관련된 테이블스페이스들에 대하여 INTENTION 잠금을 획득한다.
    // smiValidateAndLockTable(), smiTable::lockTable, 커서 open시 호출
    static IDE_RC lockAndValidateRelTBSs(
                    void                 * aTrans,           /* [IN] */
                    void                 * aTable,           /* [IN] */
                    sctTBSLockValidOpt     aTBSLvOpt,        /* [IN] */
                    idBool                 aIsIntent,        /* [IN] */
                    idBool                 aIsExclusive,     /* [IN] */
                    ULong                  aLockWaitMicroSec ); /* [IN] */

    // Tablespace Node에 잠금 획득 ( 인자 : Tablespace ID )
    static IDE_RC lockTBSNodeByID( void              * aTrans,
                                   scSpaceID           aSpaceID,
                                   idBool              aIsIntent,
                                   idBool              aIsExclusive,
                                   ULong               aLockWaitMicroSec,
                                   sctTBSLockValidOpt  aTBSLvOpt,
                                   idBool            * aLocked,
                                   sctLockHier       * aLockHier );

    // Tablespace Node에 잠금 획득 ( 인자 : Tablespace Node )
    static IDE_RC lockTBSNode( void              * aTrans,
                               sctTableSpaceNode * aSpaceNode,
                               idBool              aIsIntent,
                               idBool              aIsExclusive,
                               ULong               aLockWaitMicroSec,
                               sctTBSLockValidOpt  aTBSLvOpt,
                               idBool           *  aLocked,
                               sctLockHier      *  aLockHier );



    // Tablespace Node에 잠금 획득 ( 인자 : Tablespace ID )
    static IDE_RC lockTBSNodeByID( void              * aTrans,
                                   scSpaceID           aSpaceID,
                                   idBool              aIsIntent,
                                   idBool              aIsExclusive,
                                   sctTBSLockValidOpt  aTBSLvOpt );

    // Tablespace Node에 잠금 획득 ( 인자 : Tablespace Node )
    static IDE_RC lockTBSNode( void              * aTrans,
                               sctTableSpaceNode * aSpaceNode,
                               idBool              aIsIntent,
                               idBool              aIsExclusive,
                               sctTBSLockValidOpt  aTBSLvOpt );

    // 각각의 Tablespace에 대해 특정 Action을 수행한다.
    static IDE_RC doAction4EachTBS( idvSQL           * aStatistics,
                                    sctAction4TBS      aAction,
                                    void             * aActionArg,
                                    sctActionExecMode  aActionExecMode );

    // DDL_LOCK_TIMEOUT 프로퍼티에 따라 대기시간을 반환한다.
    static ULong getDDLLockTimeOut(smxTrans * aTrans);

    /* BUG-34187 윈도우 환경에서 슬러시와 역슬러시를 혼용해서 사용 불가능 합니다. */
#if defined(VC_WIN32)
    static void adjustFileSeparator( SChar * aPath );
#endif

    /* 경로의 유효성 검사 및 상대경로를 절대경로 반환 */
    static IDE_RC makeValidABSPath( idBool         aCheckPerm,
                                    SChar*         aValidName,
                                    UInt*          aNameLength,
                                    smiTBSLocation aTBSLocation );

    /* BUG-38621 */
    static IDE_RC makeRELPath( SChar        * aName,
                               UInt         * aNameLength,
                               smiTBSLocation aTBSLocation );

    // PRJ-1548 User Memory TableSpace 개념도입

    // 디스크/메모리 테이블스페이스의 백업상태를 설정한다.
    static IDE_RC startTableSpaceBackup( idvSQL            * aStatistics,
                                         sctTableSpaceNode * aSpaceNode );

    // 디스크/메모리 테이블스페이스의 백업상태를 해제한다.
    static IDE_RC endTableSpaceBackup( idvSQL            * aStatistics,
                                       sctTableSpaceNode * aSpaceNode );

    // PRJ-1149 checkpoint 정보를 DBF 노드에 전달하기 위해
    // 설정하는 함수.
    static void setRedoLSN4DBFileMetaHdr( smLSN* aDiskRedoLSN,
                                          smLSN* aMemRedoLSN );

    // 최근 체크포인트에서 결정된 디스크 Redo LSN을 반환한다.
    static smLSN* getDiskRedoLSN()  { return &mDiskRedoLSN; };
    // 최근 체크포인트에서 결정된 메모리 Redo LSN 배열을 반환한다.
    static smLSN* getMemRedoLSN() { return &mMemRedoLSN; };

    ////////////////////////////////////////////////////////////////////
    // Alter Tablespace Online/Offline 이 공통적으로 사용하는 함수
    ////////////////////////////////////////////////////////////////////

    // Alter Tablespace Online/Offline에 대한 에러처리를 수행한다.
    static IDE_RC checkError4AlterStatus( sctTableSpaceNode    * aTBSNode,
                                          smiTableSpaceState     aNewTBSState );


    // Tablespace에 대해 진행중인 Backup이 완료되기를 기다린 후,
    // Tablespace를 백업 불가능한 상태로 변경한다.
    static IDE_RC wait4BackupAndBlockBackup( idvSQL           * aStatistics,
                                             sctTableSpaceNode* aTBSNode,
                                             smiTableSpaceState aTBSSwitchingState );

    static inline void wait4Backup( void * aSpaceNode );
    static inline void wakeup4Backup( void * aSpaceNode );

    // Tablespace를 DISCARDED상태로 바꾸고, Loganchor에 Flush한다.
    static IDE_RC alterTBSdiscard( sctTableSpaceNode  * aTBSNode );

    // Tablespace의 mMaxTblDDLCommitSCN을 aCommitSCN으로 변경한다.
    static void updateTblDDLCommitSCN( scSpaceID aSpaceID,
                                       smSCN     aCommitSCN);

    // Tablespace에 대해서 aViewSCN으로 Drop Tablespace를 할수있는지를 검사한다.
    static IDE_RC canDropByViewSCN( scSpaceID aSpaceID,
                                    smSCN     aViewSCN);

    static IDE_RC setMaxFDCntAllDFileOfAllDiskTBS( UInt aMaxFDCnt4File );

    static void lockSpaceNode( idvSQL   * aStatistics,
                               void     * aSpaceNode )
    { (void)((sctTableSpaceNode*)aSpaceNode)->mMutex.lock( aStatistics ); };

    static void unlockSpaceNode( void   * aSpaceNode )
    { (void)((sctTableSpaceNode*)aSpaceNode)->mMutex.unlock( ); };

private:
    // BUG-23953
    static inline smiTableSpaceType getTableSpaceType( scSpaceID           aSpaceID );

    static inline sctTableSpaceNode * findNextSpaceNode( scSpaceID aSpaceID,
                                                         UInt      aSkipStateSet );

    static sctTableSpaceNode * mSpaceNodeArray[SC_MAX_SPACE_ARRAY_SIZE]; // BUG-48513
    static scSpaceID           mNewTableSpaceID;


    // 테이블스페이스 생성시 데이타파일 생성에 대한 동시성 제어
    static iduMutex            mMtxCrtTBS;

    // Drop DBF와 AllocPageCount 연산간의 동시성 제어
    static iduMutex            mGlobalPageCountCheckMutex;

    // PRJ-1548 디스크/메모리 파일헤더에 설정할 가장 최근에
    // 발생한 체크포인트 정보
    static smLSN               mDiskRedoLSN;
    static smLSN               mMemRedoLSN;

    // BUG-17285 Disk Tablespace 를 OFFLINE/DISCARD 후 DROP시 에러발생
    static sctTBSLockValidOpt mTBSLockValidOpt[ SMI_TBSLV_OPER_MAXMAX ];
};

/***********************************************************************
 *
 * Description : 테이블스페이스가 Backup 상태인지 체크한다.
 *
 * aSpaceID  - [IN] 테이블스페이스 ID
 *
 **********************************************************************/
inline idBool sctTableSpaceMgr::isBackupingTBS( scSpaceID  aSpaceID )
{
    return (((mSpaceNodeArray[aSpaceID]->mState & SMI_TBS_BACKUP)
             == SMI_TBS_BACKUP) ? ID_TRUE : ID_FALSE );
}

/***********************************************************************
 * Description : 테이블스페이스 type에 따른 3가지 특성을 반환한다.
 * [IN]  aSpaceID : 분석하려는 테이블 스페이스의 ID
 * [OUT] aTableSpaceType : aSpaceID에 해당하는 테이블스페이스가
 *              1) 시스템 테이블스페이스 인가?
 *              2) 템프 테이블스페이스 인가?
 *              3) 저장되는 위치(MEM, DISK, VOL)
 *                          에 대한 정보를 반환한다.
 **********************************************************************/
inline smiTableSpaceType sctTableSpaceMgr::getTableSpaceType( scSpaceID   aSpaceID )
{
    sctTableSpaceNode * sSpaceNode      = NULL;

    IDU_FIT_POINT("BUG-46450@sctTableSpaceMgr::getTableSpaceType::TablespaceType");

    if ( aSpaceID >= mNewTableSpaceID )
    {
        return SMI_TABLESPACE_TYPE_MAX;
    }
    
    sSpaceNode = mSpaceNodeArray[ aSpaceID ];

    if ( sSpaceNode == NULL )
    {
        return SMI_TABLESPACE_TYPE_MAX;
    }

    return sSpaceNode->mType ;

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;
    return SMI_TABLESPACE_TYPE_MAX;
#endif
}

/**********************************************************************
 * Description : TBS의 저장 영역(메모리, 디스크, 임시)을 반환한다.
 *
 *   aSpaceID - [IN] 관리 영역을 확인할 TBS의 ID
 **********************************************************************/
smiTBSLocation sctTableSpaceMgr::getTBSLocation( sctTableSpaceNode * aSpaceNode )
{
    switch ( aSpaceNode->mType )
    {
        case SMI_MEMORY_SYSTEM_DICTIONARY:
        case SMI_MEMORY_SYSTEM_DATA:
        case SMI_MEMORY_USER_DATA:
            return SMI_TBS_MEMORY;
            break;
        case SMI_DISK_SYSTEM_DATA:
        case SMI_DISK_USER_DATA:
        case SMI_DISK_SYSTEM_TEMP:
        case SMI_DISK_USER_TEMP:
        case SMI_DISK_SYSTEM_UNDO:
            return SMI_TBS_DISK;
            break;
        case SMI_VOLATILE_USER_DATA:
            return SMI_TBS_VOLATILE;
            break;

        default:
            break;
    }
    return SMI_TBS_NONE;
}

smiTBSLocation sctTableSpaceMgr::getTBSLocation( scSpaceID aSpaceID )
{
    if ( aSpaceID >= mNewTableSpaceID )
    {
        return SMI_TBS_NONE;
    }

    if ( NULL == mSpaceNodeArray[ aSpaceID ] )
    {
        return SMI_TBS_NONE;
    }
   
    return getTBSLocation( mSpaceNodeArray[ aSpaceID ] );
}


/**********************************************************************
 * Description : 시스템 테이블스페이스 여부 반환
 **********************************************************************/
idBool sctTableSpaceMgr::isSystemTableSpace( scSpaceID aSpaceID )
{
    if (( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC  ) ||
        ( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA ) ||
        ( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_DATA   ) ||
        ( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO   ) ||
        ( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_TEMP   ))
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}


/**********************************************************************
 * Description :
 **********************************************************************/
idBool sctTableSpaceMgr::isSystemMemTableSpace( scSpaceID aSpaceID )
{
    if (( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC ) ||
        ( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA ))
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

inline idBool sctTableSpaceMgr::isMemTableSpace( scSpaceID aSpaceID )
{
    smiTableSpaceType sType = getTableSpaceType( aSpaceID );

    return isMemTableSpaceType( sType ) ;
}

inline idBool sctTableSpaceMgr::isMemTableSpace( void * aSpaceNode )
{
    return isMemTableSpaceType( ((sctTableSpaceNode*)aSpaceNode)->mType ) ;
}

inline idBool sctTableSpaceMgr::isMemTableSpaceType( smiTableSpaceType aSpaceType )
{
    if (( aSpaceType == SMI_MEMORY_SYSTEM_DATA       ) ||
        ( aSpaceType == SMI_MEMORY_SYSTEM_DICTIONARY ) ||
        ( aSpaceType == SMI_MEMORY_USER_DATA         ))
    {
        return ID_TRUE;
    }
    return ID_FALSE ;
}

inline idBool sctTableSpaceMgr::isVolatileTableSpace( scSpaceID aSpaceID )
{
    smiTableSpaceType  sType = getTableSpaceType( aSpaceID );
    
    return isVolatileTableSpaceType( sType ) ;
}

inline idBool sctTableSpaceMgr::isVolatileTableSpace( void * aSpaceNode )
{   
    return isVolatileTableSpaceType( ((sctTableSpaceNode*)aSpaceNode)->mType ) ;
}

inline idBool sctTableSpaceMgr::isVolatileTableSpaceType( smiTableSpaceType aSpaceType )
{
    return ( SMI_VOLATILE_USER_DATA == aSpaceType ) ? ID_TRUE : ID_FALSE ;
}

inline idBool sctTableSpaceMgr::isDiskTableSpace( scSpaceID aSpaceID )
{
    smiTableSpaceType sType = getTableSpaceType( aSpaceID );

    return isDiskTableSpaceType( sType ) ;
}

inline idBool sctTableSpaceMgr::isDiskTableSpace( void * aSpaceNode )
{
    return isDiskTableSpaceType( ((sctTableSpaceNode*)aSpaceNode)->mType ) ;
}

inline idBool sctTableSpaceMgr::isDiskTableSpaceType( smiTableSpaceType aSpaceType )
{
    if (( aSpaceType == SMI_DISK_SYSTEM_DATA ) ||
        ( aSpaceType == SMI_DISK_USER_DATA   ) ||
        ( aSpaceType == SMI_DISK_SYSTEM_TEMP ) ||
        ( aSpaceType == SMI_DISK_USER_TEMP   ) ||
        ( aSpaceType == SMI_DISK_SYSTEM_UNDO ))
    {
        return ID_TRUE;
    }
    return ID_FALSE ;
}

inline idBool sctTableSpaceMgr::isTempTableSpace( scSpaceID aSpaceID )
{
    smiTableSpaceType  sType = getTableSpaceType( aSpaceID );

    return isTempTableSpaceType( sType ) ;
}

inline idBool sctTableSpaceMgr::isTempTableSpace( void * aSpaceNode )
{
    return isTempTableSpaceType( ((sctTableSpaceNode*)aSpaceNode)->mType ) ;
}

inline idBool sctTableSpaceMgr::isTempTableSpaceType( smiTableSpaceType aSpaceType )
{
    if (( aSpaceType == SMI_DISK_SYSTEM_TEMP ) ||
        ( aSpaceType == SMI_DISK_USER_TEMP   ))
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

inline idBool sctTableSpaceMgr::isDataTableSpaceType( smiTableSpaceType  aSpaceType )
{
    if (( aSpaceType == SMI_MEMORY_SYSTEM_DATA ) ||
        ( aSpaceType == SMI_MEMORY_USER_DATA   ) ||
        ( aSpaceType == SMI_VOLATILE_USER_DATA ) ||
        ( aSpaceType == SMI_DISK_SYSTEM_DATA   ) ||
        ( aSpaceType == SMI_DISK_USER_DATA     ))
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

inline idBool sctTableSpaceMgr::isUndoTableSpace( scSpaceID aSpaceID )
{
   return ( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO ) ? ID_TRUE : ID_FALSE ;
}

inline sctTableSpaceNode * sctTableSpaceMgr::getFirstSpaceNode()
{
    return mSpaceNodeArray[0];
}

inline sctTableSpaceNode * sctTableSpaceMgr::getNextSpaceNode( scSpaceID aSpaceID )
{
    return findNextSpaceNode( aSpaceID,
                              SMI_TBS_DROPPED );
}

inline sctTableSpaceNode * sctTableSpaceMgr::getNextSpaceNodeIncludingDropped( scSpaceID aSpaceID )
{
    return findNextSpaceNode( aSpaceID,
                              0/* Do Not Skip Any TBS
                                  (Including Dropped) */ );
}

inline sctTableSpaceNode * sctTableSpaceMgr::getNextSpaceNodeWithoutDropped( scSpaceID  aSpaceID )
{
    return findNextSpaceNode( aSpaceID,
                              ( SMI_TBS_DROPPED | 
                                SMI_TBS_CREATING |
                                SMI_TBS_DROP_PENDING )  );
}

/**********************************************************************
 * Description : 입력받은 TableSpace ID 의 다음 사용가능한 TableSpace Node를 반환한다.
 *
 *   aSpaceID - [IN] 이전 TBS의 ID
 **********************************************************************/
inline sctTableSpaceNode * sctTableSpaceMgr::findNextSpaceNode( scSpaceID aSpaceID,
                                                                UInt      aSkipStateSet )
{
    sctTableSpaceNode* sSpaceNode ;

    while( ++aSpaceID < mNewTableSpaceID )
    {
        sSpaceNode = mSpaceNodeArray[aSpaceID];

        if ( sSpaceNode != NULL )
        {
            if ( (sSpaceNode->mState & aSkipStateSet) == 0 )
            {
                IDE_DASSERT( aSpaceID == sSpaceNode->mID );
                return sSpaceNode;
            }
        }
    }

    return NULL;
}

/***********************************************************************
 * Description : TableSpace단위로 waiting과 wakeup 호출 함수
 *
 * FATAL 에러는 이자리에서 바로 종료되므로 예외로 처리하지 않아도 된다.
 **********************************************************************/
inline void sctTableSpaceMgr::wait4Backup( void * aSpaceNode )
{
    PDL_Time_Value sTimeValue;

    sTimeValue.set( idlOS::time(NULL) +
                    smuProperty::getDataFileBackupEndWaitInterval() );

    if ( ((sctTableSpaceNode*)aSpaceNode)->mBackupCV.timedwait(
             &(((sctTableSpaceNode*)aSpaceNode)->mMutex),
             &sTimeValue,
             IDU_IGNORE_TIMEDOUT ) != IDE_SUCCESS )
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondWait ));
    }
    return;
}

inline void sctTableSpaceMgr::wakeup4Backup( void * aSpaceNode )
{
    if ( ((sctTableSpaceNode*)aSpaceNode)->mBackupCV.broadcast() != IDE_SUCCESS )
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondSignal ));
    }
    return;
}

#endif // _O_SCT_TABLE_SPACE_H_


