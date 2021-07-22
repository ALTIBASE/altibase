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
 * $Id: smpVarPageList.h 88917 2020-10-15 04:54:02Z et16 $
 **********************************************************************/

#ifndef _O_SMP_VAR_PAGELIST_H_
#define _O_SMP_VAR_PAGELIST_H_ 1

#include <smDef.h>

struct smiValue;

class smpVarPageList
{
public:
    // Runtime Item을 NULL로 설정한다.
    static IDE_RC setRuntimeNull( UInt              aVarEntryCount,
                                  smpPageListEntry* aVarEntryArray );
    
    /* runtime 정보 및 mutex 초기화 */
    static IDE_RC initEntryAtRuntime( smOID             aTableOID,
                                      smpPageListEntry* aVarEntry,
                                      smpAllocPageListEntry* aAllocPageList );
    
    /* runtime 정보 및 mutex 해제 */
    static IDE_RC finEntryAtRuntime( smpPageListEntry* aVarEntry );
    
    // PageList를 초기화한다.
    static void   initializePageListEntry( smpPageListEntry* aVarEntry,
                                           smOID             aTableOID );
        
    // PageList를 refine한다.
    static IDE_RC refinePageList( void*             aTrans,
                                  scSpaceID         aSpaceID,
                                  smpPageListEntry* aVarEntry );

    // aVarEntry를 제거하고 DB로 PageList 반납
    static IDE_RC freePageListToDB( void*             aTrans,
                                    scSpaceID         aSpaceID,
                                    smOID             aTableOID,
                                    smpPageListEntry* aVarEntry );
    
    // aPage를 초기화한다.
    static void   initializePage( vULong        aIdx,
                                  UInt          aPageListID,
                                  vULong        aSlotSize,
                                  vULong        aSlotCount,
                                  smOID         aTableOID,
                                  smpPersPage*  aPageID );
    
    // aOID에 해당하는 Slot에 aValue를 저장한다.
    static IDE_RC setValue( scSpaceID       aSpaceID,
                            smOID           aOID,
                            const void*     aValue,
                            UInt            aLength);

    /* aFstPieceOID에 해당하는 Row을 읽어온다.*/
    static SChar* getValue( scSpaceID       aSpaceID,
                            UInt            aBeginPos,
                            UInt            aReadLen,
                            smOID           aFstPieceOID,
                            SChar          *aBuffer );

    // Variable Slot을 할당한다.
    static IDE_RC allocSlot( void*              aTrans,
                             scSpaceID          aSpaceID,
                             smOID              aTableOID,
                             smpPageListEntry*  aVarEntry,
                             UInt               aPieceSize,
                             smOID              aNextOID,
                             smOID*             aPieceOID,
                             SChar**            aPiecePtr,
                             UInt               aPieceType = SM_VCPIECE_TYPE_OTHER,
                             idBool             aWriteLog = ID_TRUE ); // BUG-47366
    
    // FreeSlotList에서 Slot을 꺼내온다.
    static void   removeSlotFromFreeSlotList(
        scSpaceID          aSpaceID,
        smpFreePageHeader* aFreePageHeader,
        smOID*             aPieceOID,
        SChar**            aPiecePtr);

    // slot을 free한다.
    static IDE_RC freeSlot( void*             aTrans,
                            scSpaceID         aSpaceID,
                            smpPageListEntry* aVarEntry,
                            smOID             aPieceOID,
                            SChar*            aPiecePtr,
                            smLSN*            aLsnNTA,
                            smpTableType      aTableType,
                            smSCN             aSCN = SM_SCN_MAX );

    // 실제 FreeSlot을 FreeSlotList에 추가한다.
    static IDE_RC addFreeSlotPending( void*             aTrans,
                                      scSpaceID         aSpaceID,
                                      smpPageListEntry* aVarEntry,
                                      smOID             aPieceOID,
                                      SChar*            aPiecePtr );

    // aCurRow다음 유효한 aNxtRow를 리턴한다.
    static IDE_RC nextOIDallForRefineDB( scSpaceID          aSpaceID,
                                         smpPageListEntry * aVarEntry,
                                         smOID              aCurPieceOID,
                                         SChar            * aCurRow,
                                         smOID            * aNxtPieceOID,
                                         SChar           ** aNxtPiecePtr,
                                         UInt             * aIdx);

    // PageList의 Record 갯수를 리턴한다.
    static ULong getRecordCount( smpPageListEntry* aVarEntry );

    // VarPage의 VarIdx값을 구한다.
    static UInt getVarIdx( void* aPagePtr );

    static inline SChar* getPieceValuePtr( void* aPiecePtr, UInt aPos )
    {
        return (SChar*)((smVCPieceHeader*)aPiecePtr + 1) + aPos;
    }


    /* BUG-31206    improve usability of DUMPCI and DUMPDDF */
    /* VCPiece를 altibase_sm.log에 덤프한다 */
    static void dumpVCPieceHeader( smVCPieceHeader     * aSlotHeader );


    /* BUG-31206    improve usability of DUMPCI and DUMPDDF */
    /* VCPiec(VariableColumnPiece)를 기록한다. */
    static void dumpVCPieceHeaderByBuffer( smVCPieceHeader * aVCPHeader,
                                           idBool            aDisplayTable,
                                           SChar           * aOutBuf,
                                           UInt              aOutSize );

    /* BUG-31206    improve usability of DUMPCI and DUMPDDF */
    /* dumpVarPageByBuffer를 이용해 VarPage를 altibase_sm.log에 덤프한다 */
    static void dumpVarPage( scSpaceID         aSpaceID,
                             scPageID          aPageID );

    /* BUG-31206    improve usability of DUMPCI and DUMPDDF */
    /* VarPage를 덤프한다 */
    static void dumpVarPageByBuffer( UChar            * aPagePtr,
                                     SChar            * aOutBuf,
                                     UInt               aOutSize );

    // calcVarIdx 를 빠르게 하기 위한 AllocArray를 세팅한다.
    static void initAllocArray();

private:
    
    /* FOR A4 :  system으로부터 persistent page를 할당받는 함수 */
    static IDE_RC allocPersPages( void*             aTrans,
                                  scSpaceID         aSpaceID,
                                  smpPageListEntry* aVarEntry,
                                  UInt              aIdx );
    
    // nextOIDall을 위해 aRow에서 해당 Page를 찾아준다.
    static IDE_RC initForScan( scSpaceID          aSpaceID,
                               smpPageListEntry * aVarEntry,
                               smOID              aPieceOID,
                               SChar            * aPiecePtr,
                               smpPersPage     ** aPage,
                               smOID            * aNxtPieceOID,
                               SChar           ** aNxtPiecePtr );

    // allocSlot하기위해 PrivatePageList를 검사하여 시도
    static IDE_RC tryForAllocSlotFromPrivatePageList( void*       aTrans,
                                                      scSpaceID   aSpaceID,
                                                      smOID       aTableOID,
                                                      UInt        aIdx,
                                                      smOID*      aPieceOID,
                                                      SChar**     aPiecePtr );

    // allocSlot하기위해 FreePageList과 FreePagePool을 검사하여 시도
    static IDE_RC tryForAllocSlotFromFreePageList(
        void*             aTrans,
        scSpaceID         aSpaceID,
        smpPageListEntry* aVarEntryArray,
        UInt              aIdx,
        UInt              aPageListID,
        smOID*            aPieceOID,
        SChar**           aPiecePtr );

    // FreeSlotList 구축
    static IDE_RC buildFreeSlotList( scSpaceID         aSpaceID,
                                     smpPageListEntry* aVarEntry );

    // FreeSlot 정보를 기록한다.
    static IDE_RC setFreeSlot( void*        aTrans,
                               scSpaceID    aSpaceID,
                               scPageID     aPageID,
                               smOID        aVCPieceOID,
                               SChar*       aVCPiecePtr,
                               smLSN*       aNTA,
                               smpTableType aTableType );

    // FreePageHeader의 FreeSlotList에 FreeSlot을 추가한다.
    static void   addFreeSlotToFreeSlotList( scSpaceID aSpaceID,
                                             scPageID  aPageID,
                                             SChar*    aPiecePtr );

    // aValue에 해당하는 VarIdx값을 찾는다.
    static inline IDE_RC calcVarIdx( UInt   aLength,
                                     UInt*  aVarIdx );

#ifdef DEBUG
    // Page내의 FreeSlotList의 연결이 올바른지 검사한다.
    static idBool isValidFreeSlotList(smpFreePageHeader* aFreePageHeader );
#endif

    static void tryForAllocPagesFromOtherPools( void             * aTrans,
                                                scSpaceID          aSpaceID,
                                                smpPageListEntry * aVarEntryArray,
                                                UInt               aDstIdx,
                                                UInt               aPageListID,
                                                idBool           * aIsPageAlloced );

    static void getPagesFromOtherPool( void                 * aTrans,
                                       scSpaceID              aSpaceID,
                                       smpFreePagePoolEntry * aSrcPool,
                                       smpPageListEntry     * aDstEntry,
                                       UInt                   aDstIdx,
                                       UInt                   aPageListID,
                                       UInt                 * aSrcPoolLockState );
};

#endif /* _O_SMP_VAR_PAGELIST_H_ */
