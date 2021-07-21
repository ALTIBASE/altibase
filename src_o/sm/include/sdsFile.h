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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer
 **********************************************************************/

#ifndef  _O_SDS_FILE_H_
#define  _O_SDS_FILE_H_  1


#include <idl.h>
#include <idu.h>

#include <smDef.h>
#include <smuProperty.h>

#include <smiDef.h>

#define SDS_FILEHDR_SEQ_MAX     (ID_UINT_MAX)    
#define SDS_FILEHDR_IOB_MAX     (ID_UINT_MAX)
    
typedef struct sdsFileHdr
{
    /* SM Version */
    UInt     mSmVersion;
    /* Recovery를 위한 RedoLSN */
    smLSN    mRedoLSN;
    /* */
    smLSN    mCreateLSN;
    /* 익스텐트당 meta data 수 */
    UInt     mFrameCntInExt;
    /* 마지막으로 내려간 Group의 번호 */
    UInt     mLstMetaTableSeqNum;
} sdsFileHdr;

typedef struct sdsFileNode
{
    sdsFileHdr       mFileHdr;
    smiDataFileState mState;        /* */
    /* 최대 page 개수: File Size + MetaTable */
    ULong            mPageCnt;
    /* 화일 이름 */
    SChar            mName[SMI_MAX_SBUFFER_FILE_NAME_LEN];
    /* 파일이 열려있는가. */
    idBool           mIsOpened;

    iduFile          mFile;         
    /* Loganchor 메모리 버퍼내의 DBF 속성 위치 */
    UInt             mAnchorOffset;         
} sdsFileNode;

class sdsFile
{
public:
    IDE_RC initialize( UInt aMetaTableMaxCnt,
                       UInt aPageCnt );

    IDE_RC destroy( void );

    IDE_RC initializeStatic( UInt aMetaTableMaxCnt,
                             UInt aPageCnt );

    IDE_RC destroyStatic( void );
 
    IDE_RC identify( idvSQL  * aStatistics );

    IDE_RC open( idvSQL * aStatistics );

    IDE_RC close( idvSQL *  aStatistics );
 
    IDE_RC create( idvSQL  * aStatistics, idBool aIsCreate );

    IDE_RC load( smiSBufferFileAttr    * aFileAttr,
                 UInt                    aAnchorOffset );

    IDE_RC read( idvSQL     * aStatistics,
                 UInt         aFrameID,
                 ULong        aWhere,
                 ULong        aPageCount,
                 UChar      * aBuffer );

    IDE_RC write( idvSQL    * aStatistics,
                  UInt        aFrameID,
                  ULong       aWhere,
                  ULong       aPageCount,
                  UChar     * aBuffer );

    IDE_RC sync( void );

    IDE_RC syncAllSB( idvSQL  * aStatistics );

    void getFileAttr( sdsFileNode        * aFileNode,
                      smiSBufferFileAttr * aFileAttr);

    IDE_RC readHdrInfo( idvSQL  * aStatistics,
                        UInt    * aLstMetaTableSeqNum,
                        smLSN   * aRecoveryLSN ); 

    IDE_RC readFileHdr( idvSQL      * aStatistics,
                        sdsFileHdr  * aFileHdr );

    IDE_RC setAndWriteFileHdr( idvSQL   * aStatistics,
                               smLSN    * aRedoLSN,
                               smLSN    * aCreateLSN,
                               UInt     * aMetaCount,
                               UInt     * aLstMetaTableSeqNum );

    IDE_RC repairFailureSBufferHdr( idvSQL  * aStatistics,
                                    smLSN   * aResetLogsLSN );

    inline void getFileNode( sdsFileNode  ** aFileNode );

    inline ULong getTotalPageCount( sdsFileNode   * aFileNode );

    inline void getLock( iduMutex  ** aMutex );

    inline IDE_RC lock( idvSQL  * aStatistics );

    inline IDE_RC unlock( void );
   
private:

    IDE_RC initFileNode( ULong           aFilePageCnt,  
                         SChar         * aFileName,
                         sdsFileNode  ** aFileNode );

    IDE_RC freeFileNode( void );

    IDE_RC writeFileHdr( idvSQL        * aStatistics,
                         sdsFileNode   * aFileNode );

    void setFileHdr( sdsFileHdr    * aFileHdr,
                     smLSN         * aRedoLSN,
                     smLSN         * aCreateLSN,
                     UInt          * aFrameCount,
                     UInt          * aSeqNum );

    IDE_RC writeEmptyPages( idvSQL         * aStatistics,
                            sdsFileNode    * aFileNode );

    /* HDR을 검사함 */ 
    IDE_RC checkValidationHdr( idvSQL       * aStatistics,  
                               sdsFileNode  * aFileNode,
                               idBool       * aCompatibleFrameCnt );

    /* HDR의 값 범위를 검사 함 */
    IDE_RC checkValuesOfHdr( sdsFileHdr  * aFileHdr );

    /* 통계정보를 위한 함수  */ 
    IDE_RC dumpSBufferNode( sdsFileNode  * aFileNode );
    
    IDE_RC makeValidABSPath( SChar*         aValidName );

    /* 파일의 생성및 확인시 쓰는 함수들 */
    IDE_RC makeSureValidationNode( idvSQL       * aStatistics,
                                   sdsFileNode  * aFileNode,
                                   idBool       * aAlreadyExist,
                                   idBool       * aIsCreate );

public:

private:
    SChar         * mSBufferDirName;
    /* Group을 구성하는 페이지의 수 */
    UInt            mPageCntInGrp;
    /* Group 당 하나씩 존재하는 메타 테이블을 구성하는 페이지 수 */
    UInt            mPageCntInMetaTable;
    UInt            mGroupCnt;
    UInt            mFrameCntInExt;

    sdsFileNode   * mFileNode;

    iduMutex        mFileMutex;       
};

/***********************************************************************
 * Description :  
 ***********************************************************************/
void sdsFile::getFileNode( sdsFileNode  **aFileNode )
{
    *aFileNode = mFileNode;
}

/***********************************************************************
 * Description :  
 ***********************************************************************/
ULong sdsFile::getTotalPageCount( sdsFileNode   * aFileNode )
{
    IDE_DASSERT( aFileNode != NULL );

    return aFileNode->mPageCnt;
}

IDE_RC sdsFile::lock( idvSQL     * aStatistics )
{ 
    return mFileMutex.lock( aStatistics );   
}

IDE_RC sdsFile::unlock( ) 
{ 
    return mFileMutex.unlock(); 
}

void sdsFile::getLock( iduMutex ** aMutex ) 
{ 
    *aMutex = &mFileMutex; 
}

#endif
