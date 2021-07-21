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
 * $Id: smiTable.h 89014 2020-10-22 04:24:43Z returns $
 **********************************************************************/

#ifndef _O_SMI_TABLE_H_
# define _O_SMI_TABLE_H_ 1

# include <smcDef.h>
# include <smiDef.h>
# include <smlDef.h>
# include <smrDef.h>

/* PROJ-1442 Replication Online 중 DDL 허용
 * Table Meta Log Record의 Body에 해당하는 부분
 */
typedef smrTableMeta smiTableMeta;
typedef smrDDLStmtMeta smiDDLStmtMeta;

//mtcDef.h 에 MTC_POLICY_NAME_SIZE와 동일 해야 합니다.
#define SM_POLICY_NAME_SIZE (16 - 1)

typedef struct smiHashPartitionCondition
{
    UInt          mPartitionOrder;
} smiHashPartitionCondition;

typedef struct smiNonHashPartitionCondition
{
    SChar         mPartCondMinValues[SM_MAX_PARTKEY_COND_VALUE_LEN + 1 + 7]; // 7 Byte 는 Dummy
    SChar         mPartCondMaxValues[SM_MAX_PARTKEY_COND_VALUE_LEN + 1 + 7]; // 7 Byte 는 Dummy
} smiNonHashPartitionCondition;

typedef struct smiPartTableMeta
{
    UInt          mPartitionMethod;

    UInt          mPartitionCount;

    union {
        smiHashPartitionCondition       mHashCondition;
        smiNonHashPartitionCondition    mNonHashCondition;
    };
} smiPartTableMeta;

//* BUG-26891 : 보안 컬럼 정보  */
struct smiColumnEncryptAttr {
    SInt        mEncPrecision;                      // 암호 데이타 타입의 precision
    SChar       mPolicy[SM_POLICY_NAME_SIZE + 1];    // 보안 정책의 이름 (Null Terminated String)
};

struct smiColumnSRIDAttr {
    SInt        mSrid;
};

// column의 부가정보
union smiColumnAttr {
    struct smiColumnEncryptAttr  mEncAttr;
    struct smiColumnSRIDAttr     mSridAttr;
};

typedef struct smiColumnMeta
{
    /* RP Column */
    SChar       mName[SM_MAX_NAME_LEN + 1];

    /* MT Column */
    UInt        mMTDatatypeID;
    UInt        mMTLanguageID;
    UInt        mMTFlag;
    SInt        mMTPrecision;
    SInt        mMTScale;

    // column의 부가정보
    union smiColumnAttr  mMTColumnAttr;

    /* SM Column */
    UInt        mSMID;
    UInt        mSMFlag;
    UInt        mSMOffset;
    UInt        mSMVarOrder;
    UInt        mSMSize;
    smOID       mSMDictTblOID;
    scSpaceID   mSMColSpace;

    /* QP Column */
    UInt        mQPFlag;
    SChar      *mDefaultValStr;
} smiColumnMeta;

typedef struct smiIndexMeta
{
    /* Index Identifier */
    UInt        mIndexID;
    SChar       mName[SM_MAX_NAME_LEN + 1];

    /* Index Feature */
    UInt        mTypeID;
    idBool      mIsUnique;
    idBool      mIsLocalUnique;
    idBool      mIsRange;
} smiIndexMeta;

typedef struct smiIndexColumnMeta
{
    UInt        mID;
    UInt        mFlag;
    UInt        mCompositeOrder;
} smiIndexColumnMeta;

typedef struct smiCheckMeta
{
    SChar         mName[SM_MAX_NAME_LEN + 1];
    vULong        mTableOID;
    UInt          mConstraintID;
    SChar         mCheckName[SM_MAX_NAME_LEN + 1];
    SChar       * mCondition;
} smiCheckMeta;

typedef struct smiCheckColumnMeta
{
    SChar         mName[SM_MAX_NAME_LEN + 1];
    vULong        mTableOID;
    UInt          mConstraintID;
    UInt          mColumnID;
} smiCheckColumnMeta;

class smiTable
{
public:
    static IDE_RC createTable( idvSQL*               aStatistics, 
                               scSpaceID             aTableSpaceID,
                               UInt                  aTableID,
                               smiStatement*         aStatement,
                               const smiColumnList*  aColumns,
                               UInt                  aColumnSize,
                               const void*           aInfo,
                               UInt                  aInfoSize,
                               const smiValue*       aNullRow,
                               UInt                  aFlag,
                               ULong                 aMaxRow,
                               smiSegAttr            aSegmentAttr,
                               smiSegStorageAttr     aSegmentStoAttr,
                               UInt                  aParallelDegree,
                               const void**          aTable );

    /* MEMORY/DISK Table을 Drop 한다 */
    static IDE_RC dropTable( smiStatement      * aStatement,
                             const void        * aTable,
                             smiTBSLockValidType aTBSLvType );

    static IDE_RC modifyTableInfo( smiStatement*        aStatement,
                                   const void*          aTable,
                                   const smiColumnList* aColumns,
                                   UInt                 aColumnSize,
                                   const void*          aInfo,
                                   UInt                 aInfoSize,
                                   UInt                 aFlag,
                                   smiTBSLockValidType  aTBSLvType );

    static IDE_RC modifyTableInfo( smiStatement*        aStatement,
                                   const void*          aTable,
                                   const smiColumnList* aColumns,
                                   UInt                 aColumnSize,
                                   const void*          aInfo,
                                   UInt                 aInfoSize,
                                   UInt                 aFlag,
                                   smiTBSLockValidType  aTBSLvType,
                                   ULong                aMaxRow,
                                   UInt                 aParallelDegree,
                                   idBool               aIsInitRowTemplate );

    // PROJ-1671 Bitmap-based Tablespace And Segment Storage Attribute
    // ALTER TABLE .. PCTFREE..  PCTUSED..
    static IDE_RC alterTableSegAttr( smiStatement    * aStatement,
                                     const void      * aTable,
                                     smiSegAttr        aSegAttr );
    // ALTER TABLE .. STORAGE ..
    static IDE_RC alterTableSegStoAttr( smiStatement      * aStatement,
                                        const void        * aTable,
                                        smiSegStorageAttr   aSegStoAttr );

    // ALTER TABLE... ALLOCATED EXTENT ( SIZE .. )
    static IDE_RC alterTableAllocExts( smiStatement* aStatement,
                                       const void*   aTable,
                                       ULong         aExtendPageSize );

    // ALTER INDEX .. INITRANS .. MAXTRANS ..
    static IDE_RC alterIndexSegAttr( smiStatement  * aStatement,
                                     const void    * aTable,
                                     const void    * aIndex,
                                     smiSegAttr      aSegAttr );
    // ALTER INDEX .. STORAGE ..
    static IDE_RC alterIndexSegStoAttr( smiStatement  *   aStatement,
                                        const void    *   aTable,
                                        const void    *   aIndex,
                                        smiSegStorageAttr aSegStoAttr );

    // ALTER INDEX .. ALLOCATED EXTENT ( SIZE .. )
    static IDE_RC alterIndexAllocExts( smiStatement* aStatement,
                                       const void*   aTable,
                                       const void*   aIndex,
                                       ULong         aExtendPageSize );

    static IDE_RC touchTable( smiStatement        * aStatement,
                              const void          * aTable,
                              smiTBSLockValidType   aTBSLvType );

    // MEMORY TABLE에 FREE PAGE들을 DB에 반납한다.
    static IDE_RC compactTable( smiStatement * aStatement,
                                const void   * aTable,
                                ULong          aPages );

    // DISK TABLE의 Garbage Version을 제거한다.
    static IDE_RC agingTable( smiStatement * aStatement,
                              const void   * aTable );

    // PROJ-1704 MVCC Renewal
    // DISK INDEX의 Garbage Version을 제거한다.
    static IDE_RC agingIndex( smiStatement * aStatement,
                              const void   * aIndex );

    static IDE_RC truncate( idvSQL*       aStatistics,
                            smiStatement* aStatement,
                            const void*   aTable );

     static IDE_RC lockTable( smiTrans*        aTrans,
                             const void*      aTable,
                             smiTableLockMode aLockMode,
                             ULong            aLockWaitMicroSec);

    static IDE_RC lockTable( smiTrans*   aTrans,
                             const void* aTable );

    // BUG-17477 : rp에서 필요한 함수
    static IDE_RC lockTable( SInt          aSlot,
                             smlLockItem  *aLockItem,
                             smlLockMode   aLockMode,
                             ULong         aLockWaitMicroSec,
                             smlLockMode  *aCurLockMode,
                             idBool       *aLocked,
                             smlLockNode **aLockNode,
                             smlLockSlot **aLockSlot );

    static IDE_RC createIndex( idvSQL*              aStatistics,
                               smiStatement*        aStatement,
                               scSpaceID            aTblSpaceID,
                               const void*          aTable,
                               SChar *              aName,
                               UInt                 aId,
                               UInt                 aType,
                               const smiColumnList* aKeyColumns,
                               UInt                 aFlag,
                               UInt                 aParallelDegree,
                               UInt                 aBuildFlag,
                               smiSegAttr           aSegmentAttr,
                               smiSegStorageAttr    aSegmentStoAttr,
                               ULong                aDirectKeyMaxSize,
                               const void**         aIndex );

    static IDE_RC dropIndex( smiStatement       * aStatement,
                             const void         * aTable,
                             const void         * aIndex, 
                             smiTBSLockValidType aTBSLvType );

    static UInt getIndexInfo(const void*  aIndex);
    static smiSegAttr getIndexSegAttr(const void* aIndex);
    static smiSegStorageAttr getIndexSegStoAttr(const void* aIndex);
    
    /* PROJ-2433 */
    static UInt getIndexMaxKeySize( const void * aIndex );
    static void setIndexInfo( void * aIndex,
                              UInt   aFlag );
    static void setIndexMaxKeySize( void  * aIndex,
                                    UInt    aMaxKeySize );
    /* PROJ-2433 end */
    
    static IDE_RC alterIndexInfo(smiStatement* aStatement,
                                 const void*   aTable,
                                 const void*   aIndex,
                                 const UInt    aFlag);

    // fix BUG-22395
    // ALTER INDEX .. RENAME TO ..
    static IDE_RC alterIndexName( idvSQL              *aStatistics,
                                  smiStatement     *   aStatement,
                                  const void       *   aTable,
                                  const void       *   aIndex,
                                  SChar            *   aName);

    // for pin/unpin and alter table ( add column, drop column, compact )
    static IDE_RC backupMemTable(smiStatement * aStatement,
                                 const void   * aTable,
                                 SChar        * aBackupFileName,
                                 idvSQL       * aStatistics = NULL);
    
    /* PROJ-1594 Volatile TBS */
    /* Volatile Table을 backup한다. */
    static IDE_RC backupVolatileTable(smiStatement * aStatement,
                                      const void   * aTable,
                                      SChar        * aBackupFileName,
                                      idvSQL       * aStatistics = NULL);


    // for PR-4360 alter table (add column, drop column, compact)
    static IDE_RC backupTableForAlterTable(smiStatement * aStatement,
                                           const void   * aSrcTable,
                                           const void   * aDstTable,
                                           idvSQL       * aStatistics );

    /* Memory table에 대해 restore 작업을 수행한다. */
    static IDE_RC restoreMemTable(void                  * aTrans,
                                  const void            * aSrcTable,
                                  const void            * aDstTable,
                                  smOID                   aTableOID,
                                  smiColumnList         * aSrcColumnList,
                                  smiColumnList         * aBothColumnList,
                                  smiValue              * aNewRow,
                                  idBool                  aUndo,
                                  smiAlterTableCallBack * aCallBack );

    /* Volatile table에 대해 restore 작업을 수행한다. */
    static IDE_RC restoreVolTable(void                  * aTrans,
                                  const void            * aSrcTable,
                                  const void            * aDstTable,
                                  smOID                   aTableOID,
                                  smiColumnList         * aSrcColumnList,
                                  smiColumnList         * aBothColumnList,
                                  smiValue              * aNewRow,
                                  idBool                  aUndo,
                                  smiAlterTableCallBack * aCallBack );

    static IDE_RC restoreTableForAlterTable(smiStatement          * aStatement,
                                            const void            * aSrcTable,
                                            const void            * aDstTable,
                                            smOID                   aTableOID,
                                            smiColumnList         * aSrcColumnList,
                                            smiColumnList         * aBothColumnList,
                                            smiValue              * aNewRow,
                                            smiAlterTableCallBack * aCallBack );

    static IDE_RC  restoreTableByAbort( void         * aTrans,
                                        smOID          aSrcHeaderOID,
                                        smOID          aDstHeaderOID,
                                        idBool         aDoRestart );


    static IDE_RC createSequence( smiStatement    * aStatement,
                                  SLong             aStartSequence,
                                  SLong             aIncSequence,
                                  SLong             aSyncInterval,
                                  SLong             aMaxSequence,
                                  SLong             aMinSequence,
                                  UInt              aFlag,
                                  const void     ** aTable );

    static IDE_RC dropSequence( smiStatement    * aStatement,
                                const void      * aTable );

    static IDE_RC alterSequence(smiStatement    * aStatement,
                                const void      * aTable,
                                SLong             aIncSequence,
                                SLong             aSyncInterval,
                                SLong             aMaxSequence,
                                SLong             aMinSequence,
                                UInt              aFlag,
                                idBool            aIsRestart,
                                SLong             aStartSequence,
                                SLong           * aLastSyncSeq);

    /* BUG-45929 */
    static IDE_RC resetSequence( smiStatement    * aStatement,
                                 const void      * aTable );
    
    static IDE_RC readSequence(smiStatement        * aStatement,
                               const void          * aTable,
                               SInt                  aFlag,
                               SLong               * aSequence,
                               smiSeqTableCallBack * aCallBack );

    static IDE_RC setSequence( smiStatement     * aStatement,
                               const void       * aTable,
                               SLong              aSeqValue );

    static IDE_RC getSequence( const void      * aTable,
                               SLong           * aStartSequence,
                               SLong           * aIncSequence,
                               SLong           * aSyncInterval,
                               SLong           * aMaxSequence,
                               SLong           * aMinSequence,
                               UInt            * aFlag );

    static IDE_RC refineSequence(smiStatement  * aStatement,
                                 const void    * aTable);

    static IDE_RC disableIndex( smiStatement  * aStatement,
                                const void    * aTable,
                                const void    * aIndex );
    
    static IDE_RC disableAllIndex( smiStatement  * aStatement,
                                   const void    * aTable );
    
    static IDE_RC enableAllIndex( smiStatement  * aStatement,
                                  const void    * aTable );

    static IDE_RC rebuildAllIndex( smiStatement  * aStatement,
                                   const void    * aTable );

    static void  getTableInfo( const void  * aTable,
                               void       ** aTableInfo );

    static void  getTableInfoSize( const void *aTable, 
                                   UInt       *aInfoSize );

    
    static IDE_RC  createMemIndex(idvSQL              *aStatistics,
                                  smiStatement*        aStatement,
                                  scSpaceID            aTableSpaceID,
                                  smcTableHeader*      aTable,
                                  SChar *              aName,
                                  UInt                 aId,
                                  UInt                 aType,
                                  const smiColumnList* aKeyColumns,
                                  UInt                 aFlag,
                                  UInt                 aParallelDegree,
                                  UInt                 aBuildFlag,
                                  smiSegAttr         * aSegmentAttr,
                                  smiSegStorageAttr  * /*aSegmentStoAttr*/,
                                  ULong                aDirectKeyMaxSize,
                                  const void**         aIndex);
    
    static IDE_RC  createVolIndex(idvSQL              *aStatistics,
                                  smiStatement*        aStatement,
                                  scSpaceID            aTableSpaceID,
                                  smcTableHeader*      aTable,
                                  SChar *              aName,
                                  UInt                 aId,
                                  UInt                 aType,
                                  const smiColumnList* aKeyColumns,
                                  UInt                 aFlag,
                                  UInt                 aParallelDegree,
                                  UInt                 aBuildFlag,
                                  smiSegAttr         * aSegmentAttr,
                                  smiSegStorageAttr  * /*aSegmentStoAttr*/,
                                  ULong                /* aDirectKeyMaxSize */,
                                  const void**         aIndex);

    static IDE_RC  createDiskIndex(idvSQL              *aStatistics,
                                   smiStatement*        aStatement,
                                   scSpaceID            aTableSpaceID,
                                   smcTableHeader*      aTable,
                                   SChar *              aName,
                                   UInt                 aId,
                                   UInt                 aType,
                                   const smiColumnList* aKeyColumns,
                                   UInt                 aFlag,
                                   UInt                 aParallelDegree,
                                   UInt                 aBuildFlag,
                                   smiSegAttr         * aSegmentAttr,
                                   smiSegStorageAttr  * aSegmentStoAttr,
                                   ULong                /* aDirectKeyMaxSize */,
                                   const void**         aIndex);
    
    // Table의 Flag를 변경한다.
    static IDE_RC alterTableFlag( smiStatement *aStatement,
                                  const void   *aTable,
                                  UInt          aFlagMask,
                                  UInt          aFlagValue );

    // Table Meta Log Record를 기록한다.
    static IDE_RC writeTableMetaLog(smiTrans     * aTrans,
                                    smiTableMeta * aTableMeta,
                                    const void   * aLogBody,
                                    UInt           aLogBodyLen);

    static IDE_RC modifyRowTemplate( smiStatement * aStatement,
                                     const void   * aTable );

    static IDE_RC indexReorganization( void    * aHeader );

    static IDE_RC swapIndexID( smiStatement * aStatement,
                               void         * aTable1,
                               void         * aIndex1,
                               void         * aTable2,
                               void         * aIndex2 );

    static IDE_RC modifyIndexID( smiStatement * aStatement,
                                 const void   * aTable,
                                 void         * aIndex,
                                 UInt           aIndexID );

    static IDE_RC swapIndexColumns( smiStatement * aStatement,
                                    void         * aIndex1,
                                    void         * aIndex2 );

    static IDE_RC modifyIndexColumns( smiStatement  * aStatement,
                                      void          * aIndex,
                                      UInt          * aColumns );
private:
    static IDE_RC makeKeyColumnList( void            * aTableHeader,
                                     void            * aIndexHeader,
                                     smiColumnList   * aColumnList,
                                     smiColumn       * aColumns );
};

#endif /* _O_SMI_TABLE_H_ */
