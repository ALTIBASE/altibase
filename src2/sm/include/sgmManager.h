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
 
#ifndef _O_SGM_MANAGER_H_
#define _O_SGM_MANAGER_H_ 1

#include <smDef.h>
#include <sctTableSpaceMgr.h>
#include <smmManager.h>
#include <svmManager.h>

#if 0 //not used
#define SGM_GET_NEXT_SCN_AND_TID( aSpaceID, aHeaderNext, aTID,                \
                                  aNextSCN, aNextTID,    aTmpSlotHeader ) {   \
    if( sctTableSpaceMgr::isMemTableSpace(aSpaceID) == ID_TRUE )              \
    {                                                                         \
        SMP_GET_NEXT_SCN_AND_TID( aSpaceID, aHeaderNext, aTID,                \
                                  aNextSCN, aNextTID,    aTmpSlotHeader );    \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        IDE_DASSERT( sctTableSpaceMgr::isVolatileTableSpace(aSpaceID)         \
                     == ID_TRUE );                                            \
        SVP_GET_NEXT_SCN_AND_TID( aSpaceID, aHeaderNext, aTID,                \
                                  aNextSCN, aNextTID,    aTmpSlotHeader );    \
    }                                                                         \
}
#endif

class sgmManager
{
  public:
    static inline  void * getOIDPtr( scSpaceID     aSpaceID,
                                     smOID         aOID );

    static smVCPieceHeader* getNxtVCPieceHeader( smVCPieceHeader *  aVCPieceHeader,
                                                 const smiColumn *  aColumn,
                                                 UShort          *  aOffsetIdx ); 
   static inline smVCPieceHeader* getVCPieceHeader( const void      *  aRow,
                                                    const smiColumn *  aColumn,
                                                    UShort          *  aOffsetIdx );
    static inline SChar* getVarColumnDirect( SChar           * aRow,
                                             const smiColumn * aColumn,
                                             UInt*             aLength );
    static SChar* getVarColumn( SChar           * aRow,
                                const smiColumn * aColumn,
                                UInt*             aLength );
    static SChar* getVarColumnDisk( SChar           * aRow,
                                    const smiColumn * aColumn,
                                    UInt*             aLength );
    static SChar* getVarColumn( SChar           * aRow,
                                const smiColumn * aColumn,
                                SChar           * aDestBuffer );
    // PROJ-2264
    static SChar* getCompressionVarColumn( SChar           * aRow,
                                           const smiColumn * aColumn,
                                           UInt            * aLength );
};

/***********************************************************************
 * Description : smiGetVarColumn() 용 내부 함수, BUG-48513
 *               get Valiable Column 성능 우선
 ***********************************************************************/
SChar* sgmManager::getVarColumnDirect( SChar           * aRow,
                                       const smiColumn * aColumn,
                                       UInt            * aLength )
{
    UShort          * sCurrOffsetPtr;
    smVCPieceHeader * sVCPieceHeader;
    UShort            sOffsetIdx ;

    /*  aColumns의 타입이 Memory type이면 */
    if ( (aColumn->flag & SMI_COLUMN_STORAGE_MASK)
         != SMI_COLUMN_STORAGE_MEMORY )
    {
        return getVarColumnDisk( aRow, aColumn, aLength );
    }

    if ( ( aColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_VARIABLE_LARGE )
    {
        return smcRecord::getVarLarge( aRow,
                                       aColumn,
                                       0,
                                       aLength,
                                       (SChar*)(aColumn->value),
                                       ID_TRUE );
    }
    
    if (( (smpSlotHeader*)aRow)->mVarOID != SM_NULL_OID )
    {
        sVCPieceHeader = getVCPieceHeader( aRow, aColumn, &sOffsetIdx );
        if ( sVCPieceHeader != NULL ) 
        {
            IDE_DASSERT( sOffsetIdx != ID_USHORT_MAX );
            IDE_DASSERT( sOffsetIdx < sVCPieceHeader->colCount );

            /* +1 은 헤더 사이즈를 건너뛰고 offset array 를 탐색하기 위함이다. */
            sCurrOffsetPtr = ((UShort*)(sVCPieceHeader + 1) + sOffsetIdx);

            IDE_DASSERT( *(sCurrOffsetPtr + 1) >= *sCurrOffsetPtr );

            /* next offset 과의 차가 길이가 된다. */
            *aLength    = ( *( sCurrOffsetPtr + 1 )) - ( *sCurrOffsetPtr );

            if ( *aLength != 0 )
            {
                return (SChar*)sVCPieceHeader + (*sCurrOffsetPtr);
            }
        }
    }

    *aLength    = 0;
    
    return NULL;
}

/***********************************************************************
 * Description : Variable slot header 를 가져온다. 
 ***********************************************************************/
smVCPieceHeader* sgmManager::getVCPieceHeader( const void      *  aRow,
                                               const smiColumn *  aColumn,
                                               UShort          *  aOffsetIdx )
{
    UShort            sOffsetIdx;
    smVCPieceHeader * sVCPieceHeader ;
    smOID             sOID = ((smpSlotHeader*)aRow)->mVarOID;

    IDE_DASSERT( sOID != SM_NULL_OID );

    sVCPieceHeader = (smVCPieceHeader*)getOIDPtr( aColumn->colSpace,
                                                  sOID );
    if ( sVCPieceHeader == NULL )
    {
        /* BUG-47474 smpSlotHeader의 mVarOID와 smpFreeSlotHeader의 next free slot pointer를
         *           동일한 위치에 사용하고 있으므로 인덱스에서 lock을 잡지 않고 인터널노드를 탐색하는 경우
         *           이미 정리된 slot을 살아있는 slot으로 판단하고 접근할 경우가 발생할 수 있다.
         *           이 경우 next free slot pointer를 OID로 판단하는 문제가 발생할 수 있으므로
         *           이 현상이 발생한 경우 서버를 죽이지 않고 상위로 올리면 인덱스 모듈에서
         *           노드의 변경을 확인하고 탐색을 재수행한다. */
        *aOffsetIdx = ID_USHORT_MAX;

        return NULL;
    }

    sOffsetIdx = aColumn->varOrder;

    if ( sOffsetIdx >= sVCPieceHeader->colCount )
    {
        return getNxtVCPieceHeader( sVCPieceHeader,
                                    aColumn,
                                    aOffsetIdx );
    }

    *aOffsetIdx     = sOffsetIdx;
    return sVCPieceHeader;
}

/***********************************************************************
 * Description : getOIDPtr 간략화 버전
 ***********************************************************************/
void * sgmManager::getOIDPtr( scSpaceID     aSpaceID,
                              smOID         aOID )
{
    scOffset sOffset  = (scOffset)(aOID & SM_OFFSET_MASK);
    scPageID sPageID  = (scPageID)(aOID >> SM_OFFSET_BIT_SIZE);
    SChar  * sPagePtr = (SChar*)smmManager::getPagePtr( aSpaceID, sPageID );

    if ( sPagePtr != NULL )
    {
        return (void *)( sPagePtr + sOffset  );
    }
    
    return NULL;
}

#endif

