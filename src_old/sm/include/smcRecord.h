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
 * $Id: smcRecord.h 90083 2021-02-26 00:58:48Z et16 $
 **********************************************************************/

#ifndef _O_SMC_RECORD_H_
#define _O_SMC_RECORD_H_ 1


#include <smDef.h>
#include <smcDef.h>

class smcRecord
{
public:
    /* Table에 새로운 Record를 Insert */
    static IDE_RC insertVersion( idvSQL          * aStatistics,
                                 void            * aTrans,
                                 void            * aTableInfoPtr,
                                 smcTableHeader  * aHeader,
                                 smSCN             aSCN,
                                 SChar          ** aRow,
                                 scGRID          * ,// aRetInsertSlotGRID,
                                 const smiValue  * aValueList,
                                 UInt              aFlag );

    /* Table의 Record를 MVCC기반으로 Update */
    static IDE_RC updateVersion( idvSQL                * /*aStatistics*/,
                                 void                  * aTrans,
                                 smSCN                   aViewSCN,
                                 void                  * /*aTableInfoPtr*/,
                                 smcTableHeader        * aHeader,
                                 SChar                 * aOldRow,
                                 scGRID                  /*aOldGRID*/,
                                 SChar                ** aRow,
                                 scGRID                * aRetSlotGRID,
                                 const smiColumnList   * aColumnList,
                                 const smiValue        * aValueList,
                                 const smiDMLRetryInfo * aRetryInfo,
                                 smSCN                   aInfinite,
                                 void                  * /*aLobInfo4Update*/,
                                 ULong                   /*aModifyIdxBit*/,
                                 idBool                  aForbiddenToRetry );

    static IDE_RC updateVersionInternal( void                 * aTrans,
                                         smSCN                  aViewSCN,
                                         smcTableHeader       * aHeader,
                                         SChar                * aOldRow,
                                         SChar               ** aRow,
                                         scGRID               * aRetSlotGRID ,
                                         const smiColumnList  * aColumnList,
                                         const smiValue       * aValueList,
                                         const smiDMLRetryInfo* aRetryInfo,
                                         smSCN                  aInfinite,
                                         smcUpdateOpt           aOpt,
                                         idBool                 aForbiddenToRetry );

    static IDE_RC updateUnitedVarColumns( void             *  aTrans,
                                          smcTableHeader   *  aHeader,
                                          SChar            *  aOldRow,
                                          SChar            *  aNewRow,
                                          const smiColumn  ** aColumns,
                                          smiValue         *  aValues,
                                          UInt                aVarColumnCount,
                                          UInt             *  aLogVarCount, 
                                          UInt             *  aImageSize );

    /* Table의 Record를 Inplace로 Update */
    static IDE_RC updateInplace( idvSQL                 * /*aStatistics*/,
                                 void                   * aTrans,
                                 smSCN                    /*aViewSCN*/,
                                 void                   * /*aTableInfoPtr*/,
                                 smcTableHeader         * aHeader,
                                 SChar                  * aOldRow,
                                 scGRID                   /*aOldGRID*/,
                                 SChar                 ** aRow,
                                 scGRID                 * aRetSlotGRID,
                                 const smiColumnList    * aColumnList,
                                 const smiValue         * aValueList,
                                 const smiDMLRetryInfo  * /*aRetryInfo*/,
                                 smSCN                    /*aInfinite*/,
                                 void*                    /*aLobInfo4Update*/,
                                 ULong                    aModifyIdxBit,
                                 idBool                   /*aForbiddenToRetry*/ );

    /* Table의 Record를 삭제. */
    static IDE_RC removeVersion( idvSQL               * aStatistics,
                                 void                 * aTrans,
                                 smSCN                  aViewSCN,
                                 void                 * aTableInfoPtr,
                                 smcTableHeader       * aHeader,
                                 SChar                * aRow,
                                 scGRID                 aSlotGRID,
                                 smSCN                  aSCN,
                                 const smiDMLRetryInfo* aRetryInfo,
                                 idBool                 aIsDequeue,
                                 idBool                 aForbiddenToRetry );


    static IDE_RC freeVarRowPending( void            * aTrans,
                                     smcTableHeader  * aHeader,
                                     smOID             aPieceOID,
                                     SChar           * aRow );
    
    static IDE_RC freeFixRowPending( void            * aTrans,
                                     smcTableHeader  * aHeader,
                                     SChar           * aRow );

    static IDE_RC setFreeVarRowPending( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        smOID             aPieceOID,
                                        SChar           * aRow,
                                        smSCN             aSCN );
    
    static IDE_RC setFreeFixRowPending( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        SChar           * aRow,
                                        smSCN             aSCN );
    
    static IDE_RC setSCN( scSpaceID  aSpaceID,
                          SChar     *aRow,
                          smSCN      aSCN );

    /* PROJ-2429 Dictionary based data compress for on-disk DB */
    static IDE_RC setSCNLogging( void           * aTrans, 
                                 smcTableHeader * aHeader,
                                 SChar          * aRow,
                                 smSCN            aSCN );
    
    static IDE_RC setSCN4InDoubtTrans( scSpaceID  aSpaceID,
                                       smTID      aTID,
                                       SChar     *aRow );
    
    static IDE_RC setRowNextToSCN( scSpaceID  aSpaceID,
                                   SChar     *aRow,
                                   smSCN      aSCN);

    static IDE_RC setDeleteBitOnHeader( scSpaceID       aSpaceID,
                                        smpSlotHeader * aRow );

    static IDE_RC setDeleteBitOnHeaderInternal( scSpaceID       aSpaceID,
                                                void          * aRowHeader,
                                               idBool          aIsSetDeleteBit);
    
    static IDE_RC setDeleteBit( void          * aTrans,
                                scSpaceID       aSpaceID,
                                void          * aRow,
                                SInt            aFlag);

    static IDE_RC setDropTable( void            * aTrans,
                                SChar           * aRow);

    /* Variable Column Piece의 Header에 Free Flag를 설정한다. */
    static IDE_RC setFreeFlagAtVCPieceHdr( void         * aTrans,
                                           smOID          aTableOID,
                                           scSpaceID      aSpaceID,
                                           smOID          aVCPieceOID,
                                           void         * aVCPiecePtr,
                                           UShort         aVCPieceFreeFlag,
                                           idBool         aLogging = ID_TRUE );  /*BUG-46854*/ 
    
    static IDE_RC setIndexDropFlag( void    * aTrans,
                                    smOID     aTableOID,
                                    smOID     aIndexOID,
                                    void    * aIndexHeader,
                                    UShort    aDropFlag );
    
    static IDE_RC lockRow(void           * aTrans,
                          smSCN            aViewSCN,
                          smcTableHeader * aHeader,
                          SChar          * aRow,
                          ULong            aLockWaitTime,
                          idBool           aForbiddenToRetry );

    static IDE_RC lockRowInternal(void           * aTrans,
                                  smcTableHeader * aHeader,
                                  SChar          * aRow );

    static IDE_RC unlockRow(void           * aTrans,
                            scSpaceID        aSpaceID,
                            SChar          * aRow);

    static IDE_RC nextOIDall( smcTableHeader * aHeader,
                              SChar          * aCurRow,
                              SChar         ** aNxtRow );

    /* Memory table의 nullrow을 삽입하고, Table header에 Assign한다. */
    static IDE_RC makeNullRow( void*           aTrans,
                               smcTableHeader* aHeader,
                               smSCN           aSCN,
                               const smiValue* aNullRow,
                               UInt            aLoggingFlag,
                               smOID*          aNullRowOID );

    /* 32k 이상의 Variable Column을 Out Mode로 Insert.
       Out-Mode : Variable Column을 Table의 Variable Page List에 Insert한다.
    */
    static IDE_RC insertLargeVarColumn( void            *aTrans,
                                        smcTableHeader  *aHeader,
                                        SChar           *aFixedRow,
                                        const smiColumn *aColumn,
                                        idBool           aAddOIDFlag,
                                        const void      *aValue,
                                        UInt             aLength,
                                        smOID           *aFstPieceOID);
    
    static IDE_RC insertUnitedVarColumns( void             *  aTrans,
                                          smcTableHeader   *  aHeader,
                                          SChar            *  aFixedRow,
                                          const smiColumn  ** aColumns,
                                          idBool              aAddOIDFlag,
                                          smiValue         *  aValues,
                                          SInt                aVarColumnCount,
                                          UInt             *  aImageSize );

    static IDE_RC insertUnitedVarPiece( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        idBool            aAddOIDFlag,
                                        smiValue        * aValues,
                                        const smiColumn** aColumns, /* BUG-43117 */
                                        UInt              aTotalVarLen,
                                        UShort            aVarColCount,
                                        smOID             aNxtVCPieceOID,
                                        smOID           * aCurVCPieceOID,
                                        UInt            * aVarPieceLen4Log /* BUG-43117 */);

    /* aSize에 해당하는 Variable Column저장시 필요한 Variable Column Piece 갯수 */
    static inline UInt getVCPieceCount(UInt aSize);

    /* Variable Column Descriptor를 Return */
    static inline smVCDesc* getVCDesc(const smiColumn *aColumn,
                                      const SChar     *aFixedRow);

    /* Variable Column Piece Header 를 Return */
    static IDE_RC getVCPieceHeader( const void      *  aRow,
                                    const smiColumn *  aColumn,
                                    smVCPieceHeader ** aVCPieceHeader,
                                    UShort          *  aOffsetIdx );

    static UInt getUnitedVCLogSize( const smcTableHeader  * aHeader, smOID aOID );
    /* United Variable Column에 들어있는 column 의 개수를 구함 */
    static UInt getUnitedVCColCount( scSpaceID aSpaceID, smOID aOID );
    /* MVCC: Variable Column After Image 의 Log Size <- 이름 변경 ? */
    static inline UInt getVCAMLogSize(UInt aLength);
    /* MVCC: Variable Column Before Image 의 Log Size */
    static inline UInt getVCBMLogSize(UInt aLength);
    /* MVCC: Fixed Column Image 의 Log Size */
    static inline UInt getFCMVLogSize(UInt aLength);

    /* Update Inplace: Fixed Column Image 의 Log Size  */
    static inline UInt getFCUILogSize(UInt aLength);

    /* Update Inplace: Variable Column Before Image 의 Log Size  */
    static inline UInt getVCUILogBMSize(smcLogReplOpt aIsReplSenderSend,
                                        SInt          aStoreMode,
                                        UInt          aLength);
    /* Update Inplace: Variable Column After Image 의 Log Size */
    static inline UInt getVCUILogAMSize(SInt aStoreMode, UInt aLength);

    /* Variable Column의 Store Mode를 구한다. */
    static inline SInt getVCStoreMode(const smiColumn *aColumn,
                                      UInt aLength);

    /* aVCDesc이 가리키는 Variable Column을 삭제한다. */
    static IDE_RC deleteVC(void              *aTrans,
                           smcTableHeader    *aHeader,
                           smOID              aFstOID,
                           idBool             aLogging = ID_TRUE);   /*BUG-46854*/ 

    static IDE_RC deleteLob(void              *aTrans,
                            smcTableHeader    *aHeader,
                            const smiColumn   *aColumn,
                            SChar             *aRow,
                            idBool             aLogging = ID_TRUE);  /*BUG-46854*/

    static inline SChar* getColumnPtr(const smiColumn *aColumn, SChar* aRowPtr);
    static UInt   getColumnLen(const smiColumn *aColumn, SChar* aRowPtr);
    static UInt   getVarColumnLen(const smiColumn *aColumn, const SChar* aRowPtr);
    static inline smcLobDesc* getLOBDesc(const smiColumn *aColumn, SChar* aRowPtr);

    static void logSlotInfo(const void *aRow);

    static SChar* getVarRow( const void*       aRow,
                             const smiColumn * aColumn,
                             UInt              aFstPos,
                             UInt            * aLength,
                             SChar           * aBuffer,
                             idBool            aIsReturnLength );
    static SChar* getVarLarge( const void*       aRow,
                               const smiColumn * aColumn,
                               UInt              aFstPos,
                               UInt            * aLength,
                               SChar           * aBuffer,
                               idBool            aIsReturnLength );
    /* BUG-39507 */
    static IDE_RC isUpdatedVersionBySameStmt( void            * aTrans,
                                              smSCN             aViewSCN,
                                              smcTableHeader  * aHeader,
                                              SChar           * aRow,
                                              smSCN             aInfinite,
                                              idBool          * aIsUpdatedBySameStmt,
                                              idBool            aForbiddenToRetry );

private:

    /* BUG-39507 */
    static IDE_RC checkUpdatedVersionBySameStmt( void             * aTrans,
                                                 smSCN              aViewSCN,
                                                 smcTableHeader   * aHeader,
                                                 SChar            * aRow,
                                                 smSCN              aInfiniteSCN,
                                                 idBool           * aIsUpdatedBySameStmt,
                                                 idBool             aForbiddenToRetry );

    static IDE_RC recordLockValidation( void                  * aTrans,
                                        smSCN                   aViewSCN,
                                        smcTableHeader        * aHeader,
                                        SChar                ** aRow,
                                        ULong                   aLockWaitTime,
                                        UInt                  * sState,
                                        const smiDMLRetryInfo * aRetryInfo,
                                        idBool                  aForbiddenToRetry );

    static idBool isSameColumnValue( scSpaceID               aSpaceID,
                                     const smiColumnList   * aChkColumnList,
                                     smpSlotHeader         * aPrvSlotHeaderPtr,
                                     smpSlotHeader         * aCurSlotHeaderPtr );

    static IDE_RC validateUpdateTargetRow(void                  * aTrans,
                                          scSpaceID               aSpaceID,
                                          smSCN                   aViewSCN,
                                          void                  * aRowPtr,
                                          const smiDMLRetryInfo * aRetryInfo);
    
    /* aVCDesc이 가리키는 Variable Column을 삭제한다. */
    static inline IDE_RC deleteVC(void              *aTrans,
                                  smcTableHeader    *aHeader,
                                  const smiColumn   *aColumn,
                                  SChar             *aRow,
                                  idBool             aLogging = ID_TRUE);    /*BUG-46854*/
}; 

/***********************************************************************
 * Description : MVCC로 DML(Insert, Delete, Update) 로그에서 Variable Column에대한
 *               Log의 Log Size를 구한다. Variable Log는 다음과 같이 구성된다.
 *
 *  각각의 variable Column에 대해서 
 *  Var Log : Column ID(UInt) | LENGTH(UInt) | Value | OID(1) OID(2) ... OID(n)
 *
 *  aLength - [IN] Variable Column 길이
 *
 ***********************************************************************/
inline UInt smcRecord::getVCAMLogSize(UInt aLength)
{
    return ( ID_SIZEOF(UInt)/*ColumnID*/ +
             ID_SIZEOF(UInt)/*Length*/ +
             aLength +
             ID_SIZEOF(UInt)/*OID Cnt */ +
             SMP_GET_VC_PIECE_COUNT(aLength) * ID_SIZEOF(smOID) );
}

/***********************************************************************
 * Description : MVCC로 DML(Insert, Delete, Update) 로그에서 Variable Column에대한
 *               Log의 Log Size를 구한다. Variable Log는 다음과 같이 구성된다.
 *
 *  각각의 variable Column에 대해서 
 *  Var Log Header : Column ID(UInt) | LENGTH(UInt) 
 *  Body: Value
 *
 *  aLength - [IN] Variable Column 길이
 *
 ***********************************************************************/
inline UInt smcRecord::getVCBMLogSize(UInt aLength)
{
    return (ID_SIZEOF(UInt)/*ColumnID*/ + ID_SIZEOF(UInt)/*Length*/ + aLength);
}

/***********************************************************************
 * Description : MVCC로 DML(Insert, Delete, Update) 로그에서 Fixed Column에대한
 *               Log의 Log Size를 구한다. Fixed Column Log는 다음과 같이 구성된다.
 *               MVCC의 Update Version시 Update되는 Fixed Column의 Before Image가
 *               다음과 같이 기록된다.
 *
 *  각각의 variable Column에 대해서 
 *  Var Log Header : Column ID(UInt) | LENGTH(UInt) 
 *  Body: Value
 *
 *  aLength - [IN] Fixed Column 길이
 *
 ***********************************************************************/
inline UInt smcRecord::getFCMVLogSize(UInt aLength)
{
    return (ID_SIZEOF(UInt)/*ColumnID*/ + ID_SIZEOF(UInt)/*Length*/ + aLength);
}

/***********************************************************************
 * Description : DML(Insert, Delete, Update) 로그에서 Fixed Column에대한
 *               Log의 Log Size를 구한다. Fixed Column Log는 다음과 같이 구성된다.
 *               이 Fixed Column로그는 Update Inplace시에만 기록된다.
 *
 *  각각의 Fixed Column에 대해서 
 *    Flag   (SChar) : SMP_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 *    Offset (UInt)  : Offset
 *    ID     (UInt)  : Column ID
 *    Length (UInt)  : Column Length
 *  Body: Value
 *
 *  aLength - [IN] Fixed Column 길이
 *
 ***********************************************************************/
inline UInt smcRecord::getFCUILogSize(UInt aLength)
{
    return (ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/ +
            ID_SIZEOF(UInt)/*ColumnID*/ + ID_SIZEOF(UInt)/*Length*/ + aLength);
}

/***********************************************************************
 * Description : DML(Insert, Delete, Update) 로그에서 Variable Column에대한
 *               Log의 Log Size를 구한다. Variable Log는 다음과 같이 구성된다.
 *
 * aIsReplSenderSend - [IN] : Replication Sender가 이 로그를 읽어서
 *                     보낸다면 SMC_LOG_REPL_SENDER_SEND_OK이고 아니면
 *                     SMC_LOG_REPL_SENDER_SEND_NO 이다.
 *
 * aDescFlag - [IN] : Variable Column이 저장되는 Mode
 *              In-Mode : SM_VCDESC_MODE_IN
 *              Out-Mode : SM_VCDESC_MODE_OUT
 *
 * aLength - [IN] : Variable Column의 길이
 *
 * 공통 Head:
 *    Flag   (SChar) : SMP_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 *    Offset (UInt)  : Offset
 *    ID     (UInt)  : Column ID
 *    Length (UInt)  : Column Length
 * Body
 *    1. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_OUT | REPLICATION
 *       Value  : Variable Column Value
 *       OID    : Variable Column을 구성하는 첫번째 VC Piece OID
 *
 *    2. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_IN | REPLICATION
 *       Value  : Variable Column Value
 *
 *    3. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_OUT
 *       OID : Variable Column을 구성하는 첫번째 VC Piece OID
 *
 *    4. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_IN 
 *       Value  : Variable Column Value

 *  aLength - [IN] Variable Column 길이
 *
 ***********************************************************************/
inline UInt smcRecord::getVCUILogBMSize(smcLogReplOpt aIsReplSenderSend,
                                        SInt          aStoreMode,
                                        UInt          aLength)
{
    UInt sSize;
    
    sSize = (ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/ +
             ID_SIZEOF(UInt)/*ColumnID*/ + ID_SIZEOF(UInt)/*Length*/);

    if( ( aIsReplSenderSend == SMC_LOG_REPL_SENDER_SEND_OK ) ||
        ( aStoreMode == SM_VCDESC_MODE_IN ) )
    {
        sSize += aLength;
    }
    
    if( aStoreMode== SM_VCDESC_MODE_OUT )
    {
        sSize += ID_SIZEOF(smOID);
    }
    
    return sSize;
}

/***********************************************************************
 * Description : DML(Insert, Delete, Update) 로그에서 Variable Column에대한
 *               Log의 Log Size를 구한다. Variable Log는 다음과 같이 구성된다.
 *
 * aDescFlag - [IN] : Variable Column이 저장되는 Mode
 *              In-Mode : SM_VCDESC_MODE_IN
 *              Out-Mode : SM_VCDESC_MODE_OUT
 *
 * aLength - [IN] : Variable Column의 길이
 *
 * 공통 Head:
 *    Flag   (SChar) : SMP_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 *    Offset (UInt)  : Offset
 *    ID     (UInt)  : Column ID
 *    Length (UInt)  : Column Length
 * Body
 *    1. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_OUT
 *       Value  : Variable Column Value
 *       OID Cnt:
 *       OID 들 : Variable Column을 구성하는 VC Piece OID들
 *
 *    2. SMI_COLUMN_TYPE_VARIABLE | SM_VCDESC_MODE_IN
 *       Value  : Variable Column Value
 *
 *  aLength - [IN] Variable Column 길이
 *
 ***********************************************************************/
inline UInt smcRecord::getVCUILogAMSize(SInt aStoreMode, UInt aLength)
{
    UInt sSize;
    
    sSize = (ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/ +
             ID_SIZEOF(UInt)/*ColumnID*/ + ID_SIZEOF(UInt)/*Length*/) + aLength;
    
    if( aStoreMode == SM_VCDESC_MODE_OUT )
    {
        sSize += ID_SIZEOF(UInt);
        sSize += SMP_GET_VC_PIECE_COUNT(aLength) * ID_SIZEOF(smOID);
    }
    
    return sSize;
}

/***********************************************************************
 * Description : aRow에서 aColumn이 가리키는 VC의 VCDesc를 구한다.
 *
 * aColumn    - [IN] Column Desc
 * aFixedRow  - [IN] Fixed Row Pointer
 ***********************************************************************/
inline smVCDesc* smcRecord::getVCDesc(const smiColumn *aColumn,
                                      const SChar     *aFixedRow)
{
    IDE_DASSERT( ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ||
                 ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_LOB ) );
    IDE_DASSERT ( SMI_IS_VARIABLE_LARGE_COLUMN(aColumn->flag) ||
                  SMI_IS_LOB_COLUMN(aColumn->flag) ); 
                  
    
    return (smVCDesc*)(aFixedRow + aColumn->offset);
}


/***********************************************************************
 * Description : aColumn이 가리키는 VC가 저장되는 Mode를 Return한다.
 *
 * aColumn    - [IN] Column Desc
 * aLength    - [IN] Column Length
 ***********************************************************************/
inline SInt smcRecord::getVCStoreMode(const smiColumn *aColumn,
                                      UInt             aLength)
{
    IDE_DASSERT( ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE ) ||
                 ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ||
                 ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_LOB ) );
    
    if( aColumn->vcInOutBaseSize >= aLength)
    {
        return SM_VCDESC_MODE_IN;
    }
    else
    {
        return SM_VCDESC_MODE_OUT;
    }
}

/***********************************************************************
 * Description : Variable Column의 크기가 aSize일 경우 VC Piece개수를 구한다.
 *
 * aSize    - [IN] Variable Column 크기
 ***********************************************************************/
inline UInt smcRecord::getVCPieceCount(UInt aSize)
{
    UInt sPieceCount = (aSize + SMP_VC_PIECE_MAX_SIZE - 1) / SMP_VC_PIECE_MAX_SIZE;
    
    return sPieceCount;
}

/***********************************************************************
 * Description : aRow의 aColumn이 가리키는 Variable Column을 지운다.
 *
 * aTrans    - [IN] Transaction Pointer
 * aHeader   - [IN] Table Header
 * aColumn   - [IN] Table Column List
 * aRow      - [IN] Row Pointer
 ***********************************************************************/
inline IDE_RC smcRecord::deleteVC(void              *aTrans,
                                  smcTableHeader    *aHeader,
                                  const smiColumn   *aColumn,
                                  SChar             *aRow,
                                  idBool             aLogging)    /*BUG-46854*/
{
    smVCDesc *sVCDesc;

    IDE_DASSERT( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                 == SMI_COLUMN_TYPE_VARIABLE_LARGE );

    sVCDesc = getVCDesc(aColumn, aRow);
    
    /* InMode 는 삭제할 VC 가 없다 */
    IDE_TEST_CONT( SM_VCDESC_IS_MODE_IN( sVCDesc ), SKIP );
    IDE_TEST( deleteVC( aTrans,
                        aHeader,
                        sVCDesc->fstPieceOID,
                        aLogging)   /*BUG-46854*/
              != IDE_SUCCESS ); 

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
  
    return IDE_FAILURE;

}

/***********************************************************************
 * Description : aRowPtr이 가리키는 Row에서 aColumn에 해당하는
 *               Column을 가져온다.
 *      
 * aColumn - [IN] Column정보.
 * aRowPtr - [IN] Row Pointer
 ***********************************************************************/
inline SChar* smcRecord::getColumnPtr(const smiColumn *aColumn,
                                      SChar           *aRowPtr)
{
    return aRowPtr + aColumn->offset;
}

/***********************************************************************
 * Description : aRowPtr이 가리키는 Row에서 aColumn에 해당하는
 *               LOB Column의 Desc를 구한다.
 *      
 * aColumn - [IN] Column정보.
 * aRowPtr - [IN] Row Pointer
 ***********************************************************************/
smcLobDesc* smcRecord::getLOBDesc(const smiColumn *aColumn,
                                  SChar           *aRowPtr)
{
    IDE_ASSERT((aColumn->flag & SMI_COLUMN_TYPE_MASK)
               == SMI_COLUMN_TYPE_LOB);
    return (smcLobDesc*)(aRowPtr + aColumn->offset);
}

#endif /* _O_SMC_RECORD_H_ */
