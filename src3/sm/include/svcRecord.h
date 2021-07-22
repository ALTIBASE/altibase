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
 
#ifndef _O_SVC_RECORD_H_
#define _O_SVC_RECORD_H_ 1


#include <smDef.h>
#include <smcDef.h>

class svcRecord
{
public:
    /* Table에 새로운 Record를 Insert */
    static IDE_RC insertVersion( idvSQL          * aStatistics,
                                 void            * aTrans,
                                 void            * aTableInfoPtr,
                                 smcTableHeader  * aHeader,
                                 smSCN             aSCN,
                                 SChar          ** aRow,
                                 scGRID          *,// aRetInsertSlotGRID,
                                 const smiValue  * aValueList,
                                 UInt              aAddOIDFlag );

    /* Table의 Record를 MVCC기반으로 Update */
    static IDE_RC updateVersion( idvSQL                * aStatistics,
                                 void                  * aTrans,
                                 smSCN                   aViewSCN,
                                 void                  * aTableInfoPtr,
                                 smcTableHeader        * aHeader,
                                 SChar                 * aOldRow,
                                 scGRID                  aOldGRID,
                                 SChar                ** aRow,
                                 scGRID                * aRetSlotGRID,
                                 const smiColumnList   * aColumnList,
                                 const smiValue        * aValueList,
                                 const smiDMLRetryInfo * aRetryInfo,
                                 smSCN                   aInfinite,
                                 void                  * /*aLobInfo4Update*/,
                                 ULong                   aModifyIdxBit,
                                 idBool                  aForbiddenToRetry );

    /* Table의 Record를 Inplace로 Update */
    static IDE_RC updateInplace(idvSQL                * /*aStatistics*/,
                                void                  * aTrans,
                                smSCN                   /*aViewSCN*/,
                                void                  * aTableInfoPtr,
                                smcTableHeader        * aHeader,
                                SChar                 * aOldRow,
                                scGRID                  /*aOldGRID*/,
                                SChar                ** aRow,
                                scGRID                * aRetSlotGRID,
                                const smiColumnList   * aColumnList,
                                const smiValue        * aValueList,
                                const smiDMLRetryInfo * /*aRetryInfo*/,
                                smSCN                   /*aInfinite*/,
                                void                  * /*aLobInfo4Update*/,
                                ULong                   aModifyIdxBit,
                                idBool                  /*aForbiddenToRetry*/ );

    /* Table의 Record를 삭제. */
    static IDE_RC removeVersion( idvSQL                * aStatistics,
                                 void                  * aTrans,
                                 smSCN                   aViewSCN,
                                 void                  * aTableInfoPtr,
                                 smcTableHeader        * aHeader,
                                 SChar                 * aRow,
                                 scGRID                  aSlotGRID,
                                 smSCN                   aSCN,
                                 const smiDMLRetryInfo * aRetryInfo,
                                 idBool                  aIsDequeue,
                                 idBool                  aForbiddenToRetry );

    static IDE_RC updateUnitedVarColumns( void             *  aTrans,
                                          smcTableHeader   *  aHeader,
                                          SChar            *  aOldRow,
                                          SChar            *  aNewRow,
                                          const smiColumn  ** aColumns,
                                          smiValue         *  aValues,
                                          UInt                aVarColumnCount );

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
    
    static IDE_RC setSCN( SChar     *aRow,
                          smSCN      aSCN );

    static IDE_RC setDeleteBitOnHeader( smpSlotHeader * aRow );
    
    static IDE_RC setDeleteBit( scSpaceID       aSpaceID,
                                void          * aRow );

    static IDE_RC setDropTable( void            * aTrans,
                                SChar           * aRow);

    /* PROJ-2174 Supporting LOB in the volatile tablespace */
    /* Variable Column Piece의 Header에 Free Flag를 설정한다. */
    static IDE_RC setFreeFlagAtVCPieceHdr( void         * aVCPiecePtr,
                                           UShort         aVCPieceFreeFlag);

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

    static IDE_RC insertUnitedVarColumns( void             *  aTrans,
                                          smcTableHeader   *  aHeader,
                                          SChar            *  aFixedRow,
                                          const smiColumn  ** aColumns,
                                          idBool              aAddOIDFlag,
                                          smiValue         *  aValues,
                                          SInt                aVarColumnCount );

    static IDE_RC insertUnitedVarPiece( void            * aTrans,
                                        smcTableHeader  * aHeader,
                                        idBool            aAddOIDFlag,
                                        smiValue        * aValues,
                                        const smiColumn** aColumns, /* BUG-43117 */
                                        UInt              aTotalVarLen,
                                        UShort            aVarColCount,
                                        smOID             aNxtVCPieceOID,
                                        smOID           * aCurVCPieceOID );

    /* aSize에 해당하는 Variable Column저장시 필요한 Variable Column Piece 갯수 */
    static inline UInt getVCPieceCount(UInt aSize);

    /* Variable Column Piece Header 를 Return */
    static IDE_RC getVCPieceHeader( const void      *  aRow,
                                    const smiColumn *  aColumn,
                                    smVCPieceHeader ** aVCPieceHeader,
                                    UShort          *  aOffsetIdx );

    /* Variable Column Descriptor를 Return */
    static inline smVCDesc* getVCDesc(const smiColumn *aColumn,
                                      const SChar     *aFixedRow);

    /* Variable Column의 Store Mode를 구한다. */
    static inline SInt getVCStoreMode(const smiColumn *aColumn,
                                      UInt aLength);

    /* aVCDesc이 가리키는 Variable Column을 삭제한다. */
    static IDE_RC deleteVC(void              *aTrans,
                           smcTableHeader    *aHeader,
                           smOID              aFstOID);

    static IDE_RC deleteLob(void              * aTrans,
                            smcTableHeader    * aHeader,
                            const smiColumn   * aColumn,
                            SChar             * aRow);

    static inline SChar* getColumnPtr(const smiColumn *aColumn, SChar* aRowPtr);
    static UInt   getColumnLen(const smiColumn *aColumn, SChar* aRowPtr);
    static UInt   getVarColumnLen(const smiColumn *aColumn, const SChar* aRowPtr);

    static SChar* getVarRow( const void      * aRow,
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
                                        scSpaceID               aSpaceID,
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

}; 

/***********************************************************************
 * Description : aRow에서 aColumn이 가리키는 VC의 VCDesc를 구한다.
 *
 * aColumn    - [IN] Column Desc
 * aFixedRow  - [IN] Fixed Row Pointer
 ***********************************************************************/
inline smVCDesc* svcRecord::getVCDesc( const smiColumn *aColumn,
                                       const SChar     *aFixedRow )
{
    /* PROJ-2174 Supporting LOB in the volatile tablespace */
    IDE_DASSERT( ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ||
                 ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_LOB ) );
    
    return (smVCDesc*)(aFixedRow + aColumn->offset);
}

/***********************************************************************
 * Description : aColumn이 가리키는 VC가 저장되는 Mode를 Return한다.
 *
 * aColumn    - [IN] Column Desc
 * aLength    - [IN] Column Length
 ***********************************************************************/
inline SInt svcRecord::getVCStoreMode( const smiColumn *aColumn,
                                       UInt             aLength )
{
    /* PROJ-2174 Supporting LOB in the volatile tablespace */
    IDE_DASSERT( ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE ) ||
                 ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE_LARGE ) ||
                 ( (aColumn->flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_LOB ) );

    if ( aColumn->vcInOutBaseSize >= aLength )
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
inline UInt svcRecord::getVCPieceCount(UInt aSize)
{
    UInt sPieceCount = (aSize + SMP_VC_PIECE_MAX_SIZE - 1) / SMP_VC_PIECE_MAX_SIZE;
    
    return sPieceCount;
}

/***********************************************************************
 * Description : aRowPtr이 가리키는 Row에서 aColumn에 해당하는
 *               Column을 가져온다.
 *      
 * aColumn - [IN] Column정보.
 * aRowPtr - [IN] Row Pointer
 ***********************************************************************/
inline SChar* svcRecord::getColumnPtr(const smiColumn *aColumn,
                                      SChar           *aRowPtr)
{
    return aRowPtr + aColumn->offset;
}

#endif /* _O_SVC_RECORD_H_ */



