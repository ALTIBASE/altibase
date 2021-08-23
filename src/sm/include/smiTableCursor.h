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
 * $Id: smiTableCursor.h 91512 2021-08-21 07:50:50Z emlee $
 **********************************************************************/

#ifndef _O_SMI_TABLE_CURSOR_H_
# define _O_SMI_TABLE_CURSOR_H_ 1

# include <smiDef.h>
# include <smDef.h>
# include <smnDef.h>
# include <smlDef.h>
# include <sdcDef.h>
# include <smxTableInfoMgr.h>

/* FOR A4 : smiTableSpecificFuncs
 *    본 구조체는 Disk Table과 Disk Temp Table,Memory Table에서
 *    서로 다른 동작을 하는
 *    커서 함수들을 구별하여 미리 함수포인터를 세팅해 놓고 사용함으로써
 *    런타입시에 Table 타입에 대한 비교를 막고자 함이다.
 *    여기에 없는 restart, insertRow, deleteRow 함수는
 *    Memory Table과 Disk Table에서 동일하게 동작한다.
 */
typedef struct tagTableSpecificFuncs
{
    IDE_RC (*openFunc)( smiTableCursor *     aCursor,
                        const void*          aTable,
                        smSCN                aSCN,
                        const smiRange*      aKeyRange,
                        const smiRange*      aKeyFilter,
                        const smiCallBack*   aRowFilter,
                        smlLockNode *        aCurLockNodePtr,
                        smlLockSlot *        aCurLockSlotPtr );

    IDE_RC (*closeFunc)( smiTableCursor * aCursor );

    IDE_RC (*insertRowFunc)( smiTableCursor  * aCursor,
                             const smiValue  * aValueList,
                             void           ** aRow,
                             scGRID          * aGRID );

    IDE_RC (*updateRowFunc)( smiTableCursor       * aCursor,
                             const smiValue       * aValueList,
                             const smiDMLRetryInfo* aRetryInfo );

    IDE_RC (*deleteRowFunc)( smiTableCursor       * aCursor,
                             const smiDMLRetryInfo* aRetryInfo );

    // PROJ-1509
    IDE_RC (*beforeFirstModifiedFunc )( smiTableCursor * aCursor,
                                        UInt             aFlag );

    IDE_RC (*readOldRowFunc )( smiTableCursor * aCursor,
                               const void    ** aRow,
                               scGRID         * aRowGRID );

    IDE_RC (*readNewRowFunc )( smiTableCursor * aCursor,
                               const void    ** aRow,
                               scGRID         * aRowGRID );

}smiTableSpecificFuncs;

typedef struct smiTableDMLFunc
{
    smTableCursorInsertFunc  mInsert;
    smTableCursorUpdateFunc  mUpdate;
    smTableCursorRemoveFunc  mRemove;
} smiTableDMLFunc;

typedef IDE_RC (*smiGetRemoteTableNullRowFunc)( smiTableCursor  * aCursor,
                                                void           ** aRow,
                                                scGRID          * aGRID );

/* Disk, Memory, Temp Table에 대한 공용 Interface인 관계로
   각각의 Table에 대한 연산자를 Function Pointer로 Cursor Open시
   결정해준다.*/

class smiTableCursor
{
    friend class smiTable;
    friend class smiStatement;

 public:
    /* For Statement */
    // 이 Cursor가 속해 있는 Statement의 Cursor의 Linked List
    smiTableCursor*          mPrev;
    smiTableCursor*          mNext;

    // Cursor가 읽어야 할 View를 결정하는 SCN
    smSCN                    mSCN;
    // Infinite
    smSCN                    mInfinite;
    smSCN                    mInfinite4DiskLob;
 
    //----------------------------------------
    // PROJ-1509
    // mFstUndoRecSID : cursor open 시, next undo record의 SID 저장
    //
    // mCurUndoRecSID    : readOldRow() 또는 readNewRow() 수행 시,
    //                     현재 undo record의 SID 저장
    //
    // mLstUndoPID : cursor close 시, 마지막 undo record의 SID 저장
    //----------------------------------------
    sdSID                       mFstUndoRecSID;
    sdSID                       mCurUndoRecSID;
    sdSID                       mLstUndoRecSID;

    sdRID                       mFstUndoExtRID;
    sdRID                       mCurUndoExtRID;
    sdRID                       mLstUndoExtRID;

    // Transaction의 All Cursor의 Double Linked List를 구성.
    smiTableCursor*          mAllPrev;
    smiTableCursor*          mAllNext;

 private:
    // Transaction Pointer & Flag Info
    smxTrans*                mTrans;
    UInt                     mTransFlag;

 public:
    // Cursor가 속해 있는 Statement
    smiStatement           * mStatement;
    // Cursor가 Open한 Table Header
    void*                    mTable;
    // PROJ-1618 Online Dump: DUMP TABLE의 Object
    void*                    mDumpObject;
    // Cursor가 Index를 이용할 경우 Index Header Pointer를 가리킴 아니면 NULL
    void*                    mIndex;
    // Interator관련 Memory 할당 및 초기화 Routine이 각각의 Index Module에 있음.
    void*                    mIndexModule;
    // Iterator Pointer
    smiIterator*             mIterator;

    /* BUG-43408, BUG-45368 */
    ULong                    mIteratorBuffer[ SMI_ITERATOR_SIZE / ID_SIZEOF(ULong) ];

    // Update시 Update되는 Column Pointer
    const smiColumnList*     mColumns;

    // SMI_LOCK_ ... + SMI_TRAVERSE ... + SMI_PREVIOUS_ ...
    UInt                     mFlag;
    // Table에 걸려있는 Lock Mode
    smlLockMode              mLockMode;
    // if mUntouchable == TRUE, ReadOnly, else Write.
    idBool                   mUntouchable;
    // if mUntouchable == FALSE and 현재 Statement의 Cursor중에 같은
    // Table에 대해 open한 Cursor가 있을 경우, mIsSoloCursor = ID_FALSE,
    // else mIsSoloCursor = ID_TRUE
    idBool                   mIsSoloCursor;
    // SMI_.._CURSOR (SELECT, INSERT, UPDATE, DELETE)
    UInt                     mCursorType;

    /* BUG-21866: Disk Table에 Insert시 Insert Undo Reco를 기록하지 말자.
     *
     * ID_FALSE이면 Insert시 Undo Rec을 기록하지 않는다. Triger나 Foreign Key
     * 가 없는 경우는 Insert Undo Rec이 불필요하다.
     * */
    idBool                   mNeedUndoRec;

    /* FOR A4 */
    smiCursorProperties      mOrgCursorProp;
    smiCursorProperties      mCursorProp;
    smiTableSpecificFuncs    mOps;

    idBool                   mIsDequeue;

    //----------------------------------------
    // PROJ-1509
    // mFstOidNode  : 트랜잭션의 oid list 중에서 cursor open 당시 마지막
    //                oid node
    // mFstOidCount : 트랜잭션의 oid list 중에서 cursor open 당시 마지막
    //                oid node의 마지막 oid record 위치
    // mCurOidNode  : readOldRow() 또는 readNewRow() 수행 시,
    //                현재 oid node 위치
    // mCurOidIndex : readOldRow() 또는 readNewRow() 수행 시,
    //                현재 oid node 위치의 oid record 위치
    // mLstOidNode  : 트랜잭션의 oid list 중에서 cursor close 당시 마지막
    //                oid node 위치
    // mLstOidCount : 트랜잭션의 oid list 중에서 cursor close 당시 마지막
    //                oid node의 마지막 record 위치
    //----------------------------------------
    void*                    mFstOidNode;
    UInt                     mFstOidCount;
    void*                    mCurOidNode;
    UInt                     mCurOidIndex;
    void*                    mLstOidNode;
    UInt                     mLstOidCount;
    /*
      Open시 Transaction의 OID List의 마지막 위치를 가리킴.
      ex)
      Trans->1 Node (1,2,3) , 2 Node (1,2,3), 3 Node (1, 2).
      이면 mOidNode = 3Node, mOidCount = 2가 된다.
    void*                    mOidNode;
    UInt                     mOidCount;
    */

    // Seek Function Pointer (FetchNext, FetchPrev)
    smSeekFunc              mSeek[2];
    
    // mTable에 대한 연산자(Disk, Memory, Temp Table)
    smTableCursorInsertFunc  mInsert;
    smTableCursorUpdateFunc  mUpdate;
    smTableCursorRemoveFunc  mRemove;
    smTableCursorLockRowFunc mLockRow;


    const smSeekFunc*       mSeekFunc;
    ULong                   mModifyIdxBit;
    ULong                   mCheckUniqueIdxBit;
    SChar                 * mUpdateInplaceEscalationPoint;

    /* Table Info: 현재 이 Cursor가 Table에 대해 Insert, Delet한 Record 갯수
       : PRJ-1496 */
    smxTableInfo*            mTableInfo;

    // Member 초기화
    void init( void );

    // PROJ-2068
    void                   * mDPathSegInfo;

    // 마지막에 Insert된 Row Info
    scGRID                   mLstInsRowGRID;
    SChar                  * mLstInsRowPtr;

public:

    void initialize();

   /****************************************/
   /* For A4 : Interface Function Pointers */
   /****************************************/

    /* For A4 :
       인자를 줄이기 위해 Iterator관련 인자를 없애고 내부적으로 생성하며
       커서관련 여러 Property들을 하나의 구조체(smiCursorProperties)로
       묶어서 전달함
    */
    IDE_RC open( smiStatement*        aStatement,
                 const void*          aTable,
                 const void*          aIndex,
                 smSCN                aSCN,
                 const smiColumnList* aColumns,
                 const smiRange*      aKeyRange,
                 const smiRange*      aKeyFilter,
                 const smiCallBack*   aRowFilter,
                 UInt                 aFlag,
                 smiCursorType        aCursorType,
                 smiCursorProperties* aProperties,
                 idBool               aIsDequeue = ID_FALSE );
    /*
      Cursor를 시작위치로 다시 옮긴다.
      내부적으로 Iterator를 초기화한다.
    */
    IDE_RC restart( const smiRange*      aKeyRange,
                    const smiRange*      aKeyFilter,
                    const smiCallBack*   aRowFilter );
    // Cursor를 Close한다.
    IDE_RC close( void );

    /* Cursor의 Key Range조건을 만족하는 첫번째 Row의 바로전 위치로
       Iterator를 Move.*/
    IDE_RC beforeFirst( void );

    /* Cursor의 Key Range조건을 만족하는 마지막 Row의 바로 다음 위치로
       Iterator를 Move.*/
    IDE_RC afterLast( void );

    /* FOR A4 : readRow
       Disk Table의 tuple에 대한 id를 지원해주기 위해 RID를 같이 반환
    */

    /* Cursor의 Iterator를 aFlag의 값에 따라서 이동시키면서 원하는
       Row를 읽어온다.*/
    IDE_RC readRow( const void  ** aRow,
                    scGRID       * aGRID,
                    UInt           aFlag /* SMI_FIND ... */ );

    IDE_RC readRowFromGRID( const void  ** aRow,
                            scGRID         aRowGRID );

    IDE_RC setRowPosition( void   * aRow,
                           scGRID   aRowGRID );

    // Cursor의 Range조건과 Filter조건을 만족하는 모든 Row의 갯수를 구한다.
    IDE_RC countRow( const void ** aRow,
                     SLong *       aRowCount );

    IDE_RC insertRow(const smiValue  * aValueList,
                     void           ** aRow,
                     scGRID          * aGRID);

    scGRID getLastInsertedRID( );

    // 현재 Cursor가 가리키는 Row를 update한다.
    IDE_RC updateRow( const smiValue        * aValueList,
                      const smiDMLRetryInfo * aRetryInfo );

    IDE_RC updateRow( const smiValue* aValueList )
    {
        return updateRow( aValueList,
                          NULL ); // retry info
    };

    /* BUG-39507 */
    IDE_RC isUpdatedRowBySameStmt( idBool     * aIsUpdatedRowBySameStmt );

    //PROJ-1362 Internal LOB
    IDE_RC updateRow( const smiValue        * aValueList,
                      const smiDMLRetryInfo * aRetryInfo,
                      void                 ** aRow,
                      scGRID                * aGRID );

    IDE_RC updateRow( const smiValue* aValueList,
                      void**          aRow,
                      scGRID*         aGRID )
    {
        return updateRow( aValueList,
                          NULL, // retry info
                          aRow,
                          aGRID );
    }


    // 현재 Cursor가 가리키는 Row를 Delete한다.
    IDE_RC deleteRow( const smiDMLRetryInfo * aRetryInfo );

    IDE_RC deleteRow()
    {
        return deleteRow( NULL );
    };

    // 현재 Cursor가 가리키는 Row에 대해서 Lock을 잡는다.
    IDE_RC lockRow();

    IDE_RC doCommonJobs4Open( smiStatement*        aStatement,
                              const void*          aIndex,
                              const smiColumnList* aColumns,
                              UInt                 aFlag,
                              smiCursorProperties* aProperties,
                              smlLockNode **       aCurLockNodePtr,
                              smlLockSlot **       aCurLockSlotPtr );

    inline IDE_RC getCurPos( smiCursorPosInfo * aPosInfo );

    inline IDE_RC setCurPos( smiCursorPosInfo * aPosInfo );

    IDE_RC getLastModifiedRow( void ** aRowBuf,
                               UInt    aLength );

    IDE_RC getModifiedRow( void  ** aRowBuf,
                           UInt     aLength,
                           void   * aRow,
                           scGRID   aRowGRID );

    IDE_RC getLastRow( const void ** aRow,
                       scGRID      * aRowGRID );

    // PROJ-1509
    IDE_RC beforeFirstModified( UInt aFlag );

    IDE_RC readOldRow( const void ** aRow,
                       scGRID      * aRowGRID );

    IDE_RC readNewRow( const void ** aRow,
                       scGRID      * aRowGRID );

    // PROJ-1618 Online Dump: Dump Table의 Object를 설정한다.
    void   setDumpObject( void * aDumpObject );

    IDE_RC getTableNullRow( void ** aRow, scGRID * aRid );

    // TASK-2398 Disk/Memory Log분리
    // Hybrid Transaction 검사 실시
    IDE_RC checkHybridTrans( UInt             aTableType,
                             smiCursorType    aCursorType );

/****************************************/
/* For A4 : MRDB, VRDB Specific Functions     */
/****************************************/
    static IDE_RC openMRVRDB( smiTableCursor *     aCursor,
                              const void*          aTable,
                              smSCN                aSCN,
                              const smiRange*      aKeyRange,
                              const smiRange*      aKeyFilter,
                              const smiCallBack*   aRowFilter,
                              smlLockNode *        aCurLockNodePtr,
                              smlLockSlot *        aCurLockSlotPtr );

    static IDE_RC closeMRVRDB( smiTableCursor * aCursor );

    static IDE_RC updateRowMRVRDB( smiTableCursor       * aCursor,
                                   const smiValue       * aValueList,
                                   const smiDMLRetryInfo* aRetryInfo );

    static IDE_RC deleteRowMRVRDB( smiTableCursor        * aCursor,
                                   const smiDMLRetryInfo * aRetryInfo);

    // PROJ-1509
    static IDE_RC beforeFirstModifiedMRVRDB( smiTableCursor * aCursor,
                                             UInt             aFlag );

    static IDE_RC readOldRowMRVRDB( smiTableCursor * aCursor,
                                    const void    ** aRow,
                                    scGRID         * aRowGRID );

    static IDE_RC readNewRowMRVRDB( smiTableCursor * aCursor,
                                    const void    ** aRow,
                                    scGRID         * aRowGRID );

    static IDE_RC normalInsertRow(smiTableCursor  * aCursor,
                                  const smiValue  * aValueList,
                                  void           ** aRow,
                                  scGRID          * aGRID);

    static IDE_RC dpathInsertRow(smiTableCursor  * aCursor,
                                 const smiValue  * aValueList,
                                 void           ** aRow,
                                 scGRID          * aGRID);

    /* PROJ-2264 */
    static IDE_RC insertRowWithIgnoreUniqueError( smiTableCursor  * aCursor,
                                                  smcTableHeader  * aTableHeader,
                                                  smiValue        * aValueList,
                                                  smOID           * aOID,
                                                  void           ** aRow );

    static IDE_RC getParallelDegree(smiTableCursor * aCursor,
                                    UInt           * aParallelDegree);

    /* TASK-5030 */
    static IDE_RC makeColumnNValueListMRDB(
                        smiTableCursor      * aCursor,
                        const smiValue      * aValueList,
                        smiUpdateColumnList * aUpdateColumnList,
                        UInt                  aUpdateColumnCount,
                        SChar               * aOldValueBuffer,
                        smiColumnList       * aNewColumnList,
                        smiValue            * aNewValueList);

    static IDE_RC makeFullUpdateMRDB( smiTableCursor  * aCursor,
                                      const smiValue  * aValueList,
                                      smiColumnList  ** aNewColumnList,
                                      smiValue       ** aNewValueList,
                                      SChar          ** aOldValueBuffer,
                                      idBool          * aIsReadBeforeImg );

    static IDE_RC destFullUpdateMRDB( smiColumnList  ** aNewColumnList,
                                      smiValue       ** aNewValueList,
                                      SChar          ** aOldValueBuffer );

/****************************************/
/* For A4 : DRDB Specific Functions     */
/****************************************/
    static IDE_RC openDRDB( smiTableCursor *     aCursor,
                            const void*          aTable,
                            smSCN                aSCN,
                            const smiRange*      aKeyRange,
                            const smiRange*      aKeyFilter,
                            const smiCallBack*   aRowFilter,
                            smlLockNode *        aCurLockNodePtr,
                            smlLockSlot *        aCurLockSlotPtr );

    static IDE_RC closeDRDB( smiTableCursor * aCursor );

    static IDE_RC updateRowDRDB( smiTableCursor       * aCursor,
                                 const smiValue       * aValueList,
                                 const smiDMLRetryInfo* aRetryInfo );

    static IDE_RC deleteRowDRDB( smiTableCursor        * aCursor,
                                 const smiDMLRetryInfo * aRetryInfo );

    // PROJ-1509
    static IDE_RC beforeFirstModifiedDRDB( smiTableCursor * aCursor,
                                           UInt             aFlag );

    static IDE_RC readOldRowDRDB( smiTableCursor * aCursor,
                                  const void    ** aRow,
                                  scGRID         * aRowGRID );

    static IDE_RC readNewRowDRDB( smiTableCursor * aCursor,
                                  const void    ** aRow,
                                  scGRID         * aRowGRID );

    // PROJ-1665
    static void * getTableHeaderFromCursor( void * aCursor );

    /* TASK-5030 */
    static IDE_RC makeSmiValueListInFetch(
                       const smiColumn   * aIndexColumn,
                       UInt                aCopyOffset,
                       const smiValue    * aColumnValue,
                       void              * aIndexInfo );

    static IDE_RC makeColumnNValueListDRDB(
                        smiTableCursor      * aCursor,
                        const smiValue      * aValueList,
                        sdcIndexInfo4Fetch  * aIndexInfo4Fetch,
                        smiFetchColumnList  * aFetchColumnList,
                        SChar               * aBeforeRowBufferSource,
                        smiColumnList       * aNewColumnList );

    static IDE_RC makeFetchColumnList( smiTableCursor      * aCursor,
                                       smiFetchColumnList  * aFetchColumnList,
                                       UInt                * aMaxRowSize );

    static IDE_RC makeFullUpdateDRDB( smiTableCursor      * aCursor,
                                      const smiValue      * aValueList,
                                      smiColumnList      ** aNewColumnList,
                                      smiValue           ** aNewValueList,
                                      SChar              ** aOldValueBuffer,
                                      idBool              * aIsReadBeforeImg );

    static IDE_RC destFullUpdateDRDB( smiColumnList  * aNewColumnList,
                                      smiValue       * aNewValueList,
                                      SChar          * aOldValueBuffer );

/***********************************************************/
/* For A4 : Remote Query Specific Functions(PROJ-1068)     */
/***********************************************************/
    static IDE_RC (*openRemoteQuery)(smiTableCursor *     aCursor,
                                     const void*          aTable,
                                     smSCN                aSCN,
                                     const smiRange*      aKeyRange,
                                     const smiRange*      aKeyFilter,
                                     const smiCallBack*   aRowFilter,
                                     smlLockNode *        aCurLockNodePtr,
                                     smlLockSlot *        aCurLockSlotPtr);

    static IDE_RC (*closeRemoteQuery)(smiTableCursor * aCursor);

    static IDE_RC (*updateRowRemoteQuery)(smiTableCursor       * aCursor,
                                          const smiValue       * aValueList,
                                          const smiDMLRetryInfo* aRetryInfo);

    static IDE_RC (*deleteRowRemoteQuery)(smiTableCursor        * aCursor,
                                          const smiDMLRetryInfo * aRetryInfo);

    // PROJ-1509
    static IDE_RC (*beforeFirstModifiedRemoteQuery)(smiTableCursor * aCursor,
                                                    UInt             aFlag);

    static IDE_RC (*readOldRowRemoteQuery)(smiTableCursor * aCursor,
                                           const void    ** aRow,
                                           scGRID         * aRowGRID );

    static IDE_RC (*readNewRowRemoteQuery)(smiTableCursor * aCursor,
                                           const void    ** aRow,
                                           scGRID         * aRowGRID );

    static IDE_RC setRemoteQueryCallback( smiTableSpecificFuncs * aFuncs,
                                          smiGetRemoteTableNullRowFunc aGetNullRowFunc );

    static const smiRange * getDefaultKeyRange( );

    static const smiCallBack * getDefaultFilter( );

private:
    static void makeUpdatedIndexList( smcTableHeader      * aTableHeader,
                                      const smiColumnList * aCols,
                                      ULong               * aModifyIdxBit,
                                      ULong               * aUniqueCheckIdxBit );

    static IDE_RC insertKeyIntoIndices( smiTableCursor * aCursor,
                                        SChar *          aRow,
                                        scGRID           aRowGRID,
                                        SChar *          aNullRow,
                                        SChar **         aExistUniqueRow );

    static IDE_RC insertKeyIntoIndices( smiTableCursor    *aCursor,
                                        scGRID             aRowGRID,
                                        const smiValue    *aValueList,
                                        idBool             aForce );

    static inline IDE_RC insertKeyIntoIndicesForInsert( smiTableCursor    *aCursor,
                                                        scGRID             aRowGRID,
                                                        const smiValue    *aValueList );

    static inline IDE_RC insertKeyIntoIndicesForUpdate( smiTableCursor    *aCursor,
                                                        scGRID             aRowGRID,
                                                        const smiValue    *aValueList );

    static IDE_RC insertKeysWithUndoSID( smiTableCursor * aCursor );

    static IDE_RC deleteKeys( smiTableCursor * aCursor,
                              scGRID           aRowGRID,
                              idBool           aForce );
    
    static inline IDE_RC deleteKeysForDelete( smiTableCursor * aCursor,
                                              scGRID           aRowGRID );
        
    static inline IDE_RC deleteKeysForUpdate( smiTableCursor * aCursor,
                                              scGRID           aRowGRID );

    // PROJ-1509
    static IDE_RC setNextUndoRecSID( sdpSegInfo     * aSegInfo,
                                     sdpExtInfo     * aExtInfo,
                                     smiTableCursor * aCursor,
                                     UChar          * aCurUndoRecHdr,
                                     idBool         * aIsFixPage );

    static IDE_RC setNextUndoRecSID4NewRow( sdpSegInfo     * aSegInfo,
                                            sdpExtInfo     * aExtInfo,
                                            smiTableCursor * aCursor,
                                            UChar          * aCurUndoRecHdr,
                                            idBool         * aIsFixPage );

    static IDE_RC closeDRDB4InsertCursor( smiTableCursor * aCursor );

    static IDE_RC closeDRDB4UpdateCursor( smiTableCursor * aCursor );

    static IDE_RC closeDRDB4DeleteCursor( smiTableCursor * aCursor );


    /* BUG-31993 [sm_interface] The server does not reset Iterator 
     *           ViewSCN after building index for Temp Table
     * ViewSCN이 Statement/TableCursor/Iterator간에 동일한지 평가한다. */
    inline IDE_RC checkVisibilityConsistency();

    inline void prepareRetryInfo( const smiDMLRetryInfo ** aRetryInfo )
    {
        IDE_ASSERT( aRetryInfo != NULL );

        if( *aRetryInfo != NULL )
        {
            if( (*aRetryInfo)->mIsWithoutRetry == ID_FALSE )
            {
                *aRetryInfo = NULL;
            }
        }
    };

    // PROJ-2264
    static IDE_RC setAgingIndexFlag( smiTableCursor * aCursor,
                                     SChar          * aRecPtr,
                                     UInt             aFlag );

    static smiGetRemoteTableNullRowFunc mGetRemoteTableNullRowFunc;
};

/***********************************************************************
 * Description : Cursor의 현재 위치를 가져온다.
 *
 * aPosInfo - [OUT] Cursor의 현재 위치를 넘겨받는 인자.
 **********************************************************************/
inline IDE_RC smiTableCursor::getCurPos( smiCursorPosInfo * aPosInfo )
{
    // To Fix PR-8110
    IDE_DASSERT( mIndexModule != NULL );

    return ((smnIndexModule*)mIndexModule)->mGetPosition( mIterator,
                                                          aPosInfo );

}

/***********************************************************************
 * Description : Cursor의 현재 위치를 aPosInfo가 가리키는 위치로 옮긴다.
 *
 * aPosInfo - [IN] Cursor의 새로운 위치
 **********************************************************************/
inline IDE_RC smiTableCursor::setCurPos( smiCursorPosInfo * aPosInfo )
{
    // To Fix PR-8110
    IDE_DASSERT( mIndexModule != NULL );

    return ((smnIndexModule*)mIndexModule)->mSetPosition( mIterator,
                                                          aPosInfo );
}

/***********************************************************************
 * Description :
 *
 *    PROJ-1618 Online Dump
 *
 *    Dump Table을 위한 Table Cursor인 경우에 사용하며
 *    Dump Object 정보를 설정한다.
 *
 **********************************************************************/

inline void
smiTableCursor::setDumpObject( void * aDumpObject )
{
    mDumpObject = aDumpObject;
}

/***********************************************************************
 * PROJ-1655
 * Description : table cursor의 table header를 반환한다.
 *
 **********************************************************************/
inline void * smiTableCursor::getTableHeaderFromCursor( void * aCursor )
{
    return (void*)(((smiTableCursor*)aCursor)->mTable);
}


inline IDE_RC smiTableCursor::insertKeyIntoIndicesForInsert( smiTableCursor    *aCursor,
                                                             scGRID             aRowGRID,
                                                             const smiValue    *aValueList )
{
    return insertKeyIntoIndices( aCursor,
                                 aRowGRID,
                                 aValueList,
                                 ID_TRUE /* aForce */ );
}

inline IDE_RC smiTableCursor::insertKeyIntoIndicesForUpdate( smiTableCursor    *aCursor,
                                                             scGRID             aRowGRID,
                                                             const smiValue    *aValueList )
{
    return insertKeyIntoIndices( aCursor,
                                 aRowGRID,
                                 aValueList,
                                 ID_FALSE /* aForce */ );
}

inline IDE_RC smiTableCursor::deleteKeysForDelete( smiTableCursor * aCursor,
                                                   scGRID           aRowGRID )
{
    return deleteKeys( aCursor,
                       aRowGRID,
                       ID_TRUE /* aForce */ );
}
        
inline IDE_RC smiTableCursor::deleteKeysForUpdate( smiTableCursor * aCursor,
                                                   scGRID           aRowGRID )
{
    return deleteKeys( aCursor,
                       aRowGRID,
                       ID_FALSE /* aForce */ );
}
 
#endif /* _O_SMI_TABLE_CURSOR_H_ */
