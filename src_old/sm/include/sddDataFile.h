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
 * $Id: sddDataFile.h 86490 2020-01-02 05:59:08Z et16 $
 *
 * Description :
 *
 * 본 파일은 디스크관리자의 datafile 노드에 대한 헤더파일이다.
 *
 *
 **********************************************************************/

#ifndef _O_SDD_DATA_FILE_H_
#define _O_SDD_DATA_FILE_H_ 1

#include <smDef.h>
#include <sddDef.h>

class sddDataFile
{
public:
    static IDE_RC initialize( scSpaceID         aTableSpaceID,
                              sddDataFileNode*  aDataFileNode,
                              smiDataFileAttr*  aDataFileAttr,
                              UInt              aMaxDataFileSize,
                              idBool  aCheckPathVal = ID_TRUE);

    static IDE_RC destroy( sddDataFileNode*  aDataFileNode );

    static IDE_RC create( idvSQL          * aStatistics,
                          sddDataFileNode * aDataFileNode,
                          sddDataFileHdr  * aDBFileHdr = NULL );

    static IDE_RC reuse( idvSQL          * aStatistics,
                         sddDataFileNode * aDataFileNode );

    static IDE_RC open( sddDataFileNode * aDataFileNode,
                        idBool            aIsDirectIO );

    static IDE_RC close( sddDataFileNode*  aDataFileNode );

    static IDE_RC extend( idvSQL          * aStatistics,
                          sddDataFileNode * aDataFileNode,
                          ULong             aExtendSize );

    static IDE_RC truncate( sddDataFileNode*  aDataFileNode,
                            ULong             aNewDataFileSize );

    inline static IDE_RC read( idvSQL*          aStatistics,
                               sddDataFileNode* aDataFileNode,
                               ULong            aWhere,
                               ULong            aReadByteSize,
                               UChar*           aBuffer );

    inline static IDE_RC readv( idvSQL         * aStatistics,
                                sddDataFileNode* aDataFileNode,
                                ULong            aWhere,
                                iduFileIOVec&    aVec );

    inline static IDE_RC write( idvSQL*          aStatistics,
                                sddDataFileNode* aDataFileNode,
                                ULong            aWhere,
                                ULong            aWriteByteSize,
                                UChar*           aBuffer,
                                iduEmergencyFuncType aSetEmergencyFunc );

    inline static IDE_RC writev( idvSQL*          aStatistics,
                                 sddDataFileNode* aDataFileNode,
                                 ULong            aWhere,
                                 iduFileIOVec&    aVec,
                                 iduEmergencyFuncType aSetEmergencyFunc );

    inline static IDE_RC sync( sddDataFileNode*  aDataFileNode,
                               iduEmergencyFuncType aSetEmergencyFunc );

    static IDE_RC addPendingOperation( 
        void             *aTrans,
        sddDataFileNode  *aDataFileNode,
        idBool            aIsCommit,  /* 동작 시기 결정 */  
        sctPendingOpType  aPendingOpType,
        sctPendingOp    **aPendingOp = NULL );
    
    inline static void prepareIO( idvSQL          * aStatistics,
                                  sddDataFileNode * aDataFileNode );

    inline static void completeIO( idvSQL          * aStatistics,
                                   sddDataFileNode * aDataFileNode,
                                   sddIOMode         aIOMode );

    static void setAutoExtendProp( sddDataFileNode*  aDataFileNode,
                                   idBool            aAutoExtendMode,
                                   ULong             aNextSize,
                                   ULong             aMaxSize );

    static void setCurrSize( sddDataFileNode*  aDataFileNode,
                             ULong             aSize );

    static void setInitSize(sddDataFileNode*  aDataFileNode,
                            ULong             aSize );

    /* PRJ-1149 데이타노드와 파일header를 비교하여 media recovery가
       필요한지 검사 */
    static IDE_RC checkValidationDBFHdr( 
                       sddDataFileNode*   aFileNode,
                       sddDataFileHdr*    aFileMetaHdr,
                       idBool*            aNeedRecovery );
    
    //PROJ-2133 incremental backup aDataFileDescSlotID추가
    static void setDBFHdr( sddDataFileHdr*              aFileMetaHdr,
                           smLSN*                       aRedoLSN,
                           smLSN*                       aCreateLSN,
                           smLSN*                       aLstLSN,
                           smiDataFileDescSlotID*       aDataFileDescSlotID );

    static IDE_RC checkValuesOfDBFHdr( sddDataFileHdr* aDBFileHdr );
    
    static IDE_RC setDataFileName( 
                     sddDataFileNode*  aDataFileNode,
                     SChar*            aNewName,
                     idBool            aCheckAccess = ID_TRUE);

    static void getDataFileAttr(sddDataFileNode* aDataFileNode,
                                smiDataFileAttr* aDataFileAttr);

    /* datafile 노드의 정보를 출력 */
    static IDE_RC dumpDataFileNode(sddDataFileNode* aDataFileNode);

    // PRJ-1548 User Memory Tablespace
    // 데이타파일 노드의 Open이 되어 있는지 반환
    static idBool isOpened( sddDataFileNode  * aFileNode ) 
    { return aFileNode->mIsOpened; }

    // 데이타파일 노드를 참조하는 I/O 개수 반환
    static UInt getIOCount( sddDataFileNode * aFileNode )
    { return aFileNode->mIOCount; }

    // Datafile에 Header를 기록한다.
    static IDE_RC writeDBFileHdr(
                     idvSQL          * aStatistics,
                     sddDataFileNode * aDataFileNode,
                     sddDataFileHdr  * aDBFileHdr );

    // 임의의 경로의 Datafile에 Header를 기록한다.
    static IDE_RC writeDBFileHdrByPath(
                     SChar           * aDBFilePath,
                     sddDataFileHdr  * aDBFileHdr );

    static inline IDE_RC setMaxFDCount( sddDataFileNode *aDataFileNode,
                                        UInt             aMaxFDCnt);

    // File의 Open과 Close그리고 IsOpened 를 보호한다.
    static void lockFileNode( idvSQL          *aStatistics,
                              sddDataFileNode *aDataFileNode )
    {   aDataFileNode->mMutex.lock( aStatistics ); };

    static void unlockFileNode( sddDataFileNode *aDataFileNode )
    {   aDataFileNode->mMutex.unlock(); };

private:
    // BUG-17415 데이터 파일 생성 또는 reuse시 파일 초기화 writing을 수행한다.
    static IDE_RC writeNewPages( idvSQL          *aStatistics,
                                 sddDataFileNode *aDataFileNode );

};

inline IDE_RC sddDataFile::setMaxFDCount( sddDataFileNode *aDataFileNode,
                                          UInt             aMaxFDCnt )
{
    return aDataFileNode->mFile.setMaxFDCnt( aMaxFDCnt );
}

/***********************************************************************
 * Description : datafile의 pageoffset부터 readsize만큼 data 판독
 **********************************************************************/
IDE_RC sddDataFile::read( idvSQL         * aStatistics,
                          sddDataFileNode* aDataFileNode,
                          ULong            aWhere,
                          ULong            aReadByteSize,
                          UChar*           aBuffer )
{
    return aDataFileNode->mFile.read( aStatistics,
                                      aWhere + SM_DBFILE_METAHDR_PAGE_SIZE,
                                      (void*)aBuffer,
                                      aReadByteSize,
                                      NULL );
}

/***********************************************************************
 * Description : datafile의 pageoffset부터 readsize만큼 data 판독
 **********************************************************************/
IDE_RC sddDataFile::readv( idvSQL         * aStatistics,
                           sddDataFileNode* aDataFileNode,
                           ULong            aWhere,
                           iduFileIOVec   & aVec )
{
    return aDataFileNode->mFile.readv( aStatistics,
                                       aWhere + SM_DBFILE_METAHDR_PAGE_SIZE,
                                       aVec);
}


/***********************************************************************
 * Description : datafile의 pageoffset부터 writesize만큼 data 기록
 * size만큼 extend하는 경우에 write page size는 extend의 크기의
 * 배수가 될 것이다.
 **********************************************************************/
IDE_RC sddDataFile::write( idvSQL         * aStatistics,
                           sddDataFileNode* aDataFileNode,
                           ULong            aWhere,
                           ULong            aWriteByteSize,
                           UChar*           aBuffer,
                           iduEmergencyFuncType aSetEmergencyFunc )
{
    return aDataFileNode->mFile.writeUntilSuccess( aStatistics,
                                                   aWhere + SM_DBFILE_METAHDR_PAGE_SIZE,
                                                   (void*)aBuffer,
                                                   aWriteByteSize,
                                                   aSetEmergencyFunc );
}


/***********************************************************************
 * Description : datafile의 pageoffset부터 writesize만큼 data 기록
 * size만큼 extend하는 경우에 write page size는 extend의 크기의
 * 배수가 될 것이다.
 **********************************************************************/
IDE_RC sddDataFile::writev( idvSQL         * aStatistics,
                            sddDataFileNode* aDataFileNode,
                            ULong            aWhere,
                            iduFileIOVec   & aVec,
                            iduEmergencyFuncType aSetEmergencyFunc )
{
    return aDataFileNode->mFile.writevUntilSuccess( aStatistics,
                                                    aWhere + SM_DBFILE_METAHDR_PAGE_SIZE,
                                                    aVec,
                                                    aSetEmergencyFunc );
}

/***********************************************************************
 * Description : datafile을 sync한다.
 **********************************************************************/
IDE_RC sddDataFile::sync( sddDataFileNode*     aDataFileNode,
                          iduEmergencyFuncType aSetEmergencyFunc )
{
    IDE_DASSERT( aDataFileNode != NULL );

    return aDataFileNode->mFile.syncUntilSuccess( aSetEmergencyFunc );
}

/***********************************************************************
 * Description : datafile에 대한 I/O 준비작업 수행
 **********************************************************************/
void sddDataFile::prepareIO( idvSQL          * aStatistics,
                             sddDataFileNode * aDataFileNode )
{
    IDE_DASSERT( aDataFileNode != NULL );

    (void)aDataFileNode->mMutex.lock( aStatistics );
    aDataFileNode->mIOCount++;
    (void)aDataFileNode->mMutex.unlock();
    return;
}

/***********************************************************************
 * Description : datafile에 대한 I/O 마무리 작업 수행
 **********************************************************************/
void sddDataFile::completeIO( idvSQL          * aStatistics,
                              sddDataFileNode * aDataFileNode,
                              sddIOMode         aIOMode )
{
    IDE_DASSERT(aDataFileNode != NULL);

    (void)aDataFileNode->mMutex.lock( aStatistics );

    if (aIOMode == SDD_IO_WRITE)
    {
        aDataFileNode->mIsModified = ID_TRUE;
    }

    aDataFileNode->mIOCount--;
    (void)aDataFileNode->mMutex.unlock();

    return;
}

#endif // _O_SDD_DATA_FILE_H_


