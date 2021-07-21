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
 * $Id: smrRedoLSNMgr.cpp 82936 2018-04-30 04:22:11Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smr.h>
#include <smrDef.h>

smrRedoInfo    smrRedoLSNMgr::mRedoInfo;
smrRedoInfo*   smrRedoLSNMgr::mCurRedoInfoPtr;
smLSN          smrRedoLSNMgr::mLstLSN;

smrRedoLSNMgr::smrRedoLSNMgr()
{
}

smrRedoLSNMgr::~smrRedoLSNMgr()
{
}

/***********************************************************************
 * Description : smrRedoLSNMgr를 초기화한다.

  smrRedoInfo를 할당하고 각각의 smrRedoInfo의
  mRedoLSN이 가리키는 Log의 Head에 mSN값에 따라서 Sorting하기 위해
  mRedoLSN이 가리키는 Log를 읽어서 mSortRedoInfo에 삽입한다.

  aRedoLSN : [IN] Redo LSN 
*/
IDE_RC smrRedoLSNMgr::initialize( smLSN *aRedoLSN )
{

    mCurRedoInfoPtr = NULL;

    IDE_TEST( initializeRedoInfo( &mRedoInfo )
              != IDE_SUCCESS );

    IDE_TEST( pushRedoInfo( &mRedoInfo,
                            aRedoLSN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Redo Info를 초기화한다

   [IN] aRedoInfo - 초기화할 Redo Info
 */
IDE_RC smrRedoLSNMgr::initializeRedoInfo( smrRedoInfo * aRedoInfo )
{
    IDE_DASSERT( aRedoInfo != NULL );

    idlOS::memset( aRedoInfo, 0, ID_SIZEOF( *aRedoInfo ) );

    // Log Record가 압축 해제된 후,
    // 바로 Redo되지 않고 Hash Table에 매달린채로 남아있게 된다.
    // 이를 위해 Log의 Decompress Buffer를 재사용하지 않도록,
    // iduGrowingMemoryHandle을 사용한다.

    /* smrRedoLSNMgr_initializeRedoInfo_malloc_DecompBufferHandle1.tc */
    IDU_FIT_POINT("smrRedoLSNMgr::initializeRedoInfo::malloc::DecompBufferHandle1");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                 ID_SIZEOF( iduGrowingMemoryHandle ),
                                 (void**)&aRedoInfo->mDecompBufferHandle,
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );

    // 로그 압축버퍼 핸들의 초기화
    IDE_TEST( ((iduGrowingMemoryHandle*)aRedoInfo->mDecompBufferHandle)->
                    initialize(
                        IDU_MEM_SM_SMR,
                        // Chunk 크기
                        // ( 최대 할당가능한 압축버퍼크기
                        //   => Log Record의 최대 크기 == 로그파일크기)
                        smuProperty::getLogFileSize() )
              != IDE_SUCCESS );

    aRedoInfo->mLogFilePtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Redo Info를 파괴한다

   [IN] aRedoInfo - 파괴할 Redo Info
 */
IDE_RC smrRedoLSNMgr::destroyRedoInfo( smrRedoInfo * aRedoInfo )
{
    IDE_DASSERT( aRedoInfo != NULL );

    // 로그 압축버퍼 핸들의 파괴
    IDE_TEST( ((iduGrowingMemoryHandle*)aRedoInfo->mDecompBufferHandle)->
                destroy() != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::free( aRedoInfo->mDecompBufferHandle )
              != IDE_SUCCESS );
    aRedoInfo->mDecompBufferHandle = NULL;

    // BUGBUG Log File의 Close는 어디서 하는지?

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Decompress Log Buffer 크기를 얻어온다

   return - Decompress Log Buffer크기
 */

ULong smrRedoLSNMgr::getDecompBufferSize()
{
    return ((iduGrowingMemoryHandle*)
            mRedoInfo.mDecompBufferHandle)->getSize();
}

// Decompress Log Buffer가 할당한 모든 메모리를 해제한다.
IDE_RC smrRedoLSNMgr::clearDecompBuffer()
{

    IDE_TEST( ((iduGrowingMemoryHandle*)mRedoInfo.mDecompBufferHandle)->clear()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Redo Info를 Sort Array에 Push한다.

   [IN] aRedoInfo -  Sort Array에 Push할 Redo Info
   [IN] aRedoLSN  -  Redo 시작 위치를 가리키는 LSN

 */
IDE_RC smrRedoLSNMgr::pushRedoInfo( smrRedoInfo * aRedoInfo,
                                    smLSN       * aRedoLSN )
{
    iduMemoryHandle * sDecompBufferHandle;
    smrLogHead      * sLogHeadPtr;
    ULong             sOrgDecompBufferSize;

    IDE_DASSERT( aRedoInfo  != NULL);
    IDE_DASSERT( aRedoLSN   != NULL );

    aRedoInfo->mRedoLSN  = *aRedoLSN;
    sDecompBufferHandle  = aRedoInfo->mDecompBufferHandle;
    sOrgDecompBufferSize = sDecompBufferHandle->getSize();

    /*mRedoLSN이 가리키는 Log를 읽어서 mRedoInfo에 삽입한다.*/
    IDE_TEST(smrLogMgr::readLog( aRedoInfo->mDecompBufferHandle,
                                 &(aRedoInfo->mRedoLSN),
                                 ID_FALSE, /* don't Close Log File When aLogFile doesn't include aLSN */
                                 &(aRedoInfo->mLogFilePtr),
                                 &(aRedoInfo->mLogHead),
                                 &(aRedoInfo->mLogPtr),
                                 &(aRedoInfo->mIsValid),
                                 &(aRedoInfo->mLogSizeAtDisk))
             != IDE_SUCCESS);

    if(aRedoInfo->mIsValid == ID_TRUE)
    {
        sLogHeadPtr = &aRedoInfo->mLogHead;

        // 디스크 로그를 읽은 경우
        if ( smrLogMgr::isDiskLogType( smrLogHeadI::getType( sLogHeadPtr ) )
             == ID_TRUE )
        {
            IDE_TEST( makeCopyOfDiskLogIfNonComp(
                          aRedoInfo,
                          sDecompBufferHandle,
                          sOrgDecompBufferSize ) != IDE_SUCCESS );
        }
    }
    else
    {
        /* Active한 Transaction이 하나도 없을 경우 */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description : smrRedoLSNMgr를 해제한다.
 *
 * 할당된 Resource를 해제한다.
 */

IDE_RC smrRedoLSNMgr::destroy()
{
    IDE_TEST( smrLogMgr::getLogFileMgr().closeAllLogFile() != IDE_SUCCESS );

    IDE_TEST( destroyRedoInfo( &mRedoInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : iduPriorityQueue에서 Item들을 Compare할 때 사용하는
 *               Callback Function임. 
 *               iduPriorityQueue의 initialize할때 넘겨짐
 *
 * arg1  - [IN] compare할 smrRedoInfo 1
 * arg2  - [IN] compare할 smrRedoInfo 2
*/
SInt smrRedoLSNMgr::compare( const void *arg1,const void *arg2 )
{
    smLSN sLSN1;
    smLSN sLSN2;

    SM_GET_LSN( sLSN1, ((*(smrRedoInfo**)arg1))->mRedoLSN );
    SM_GET_LSN( sLSN2, ((*(smrRedoInfo**)arg2))->mRedoLSN );

    if ( smrCompareLSN::isGT(&sLSN1, &sLSN2 ) )
    {
        return 1;
    }
    else
    {
        if ( smrCompareLSN::isLT(&sLSN1, &sLSN2 ) )
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}

/***********************************************************************
 * Description : Redo할 Log를 읽어서 준다.
 *
 * mSortRedoInfo에 들어있는 smrRedoInfo중에서 가장 작은 mSN값을
 * 가진 Log를 읽어들인다.
 *
 * [OUT] aLSN       - Log의 LSN
 * [OUT] aLogHead   - aLSN이 가리키는 log의 LogHead
 * [OUT] aLogPtr    - aLSN이 가리키는 log의 Log Buffr Ptr
 * [OUT] aLogSizeAtDisk - disk 상에서의 log의 길이
 * [OUT] aIsValid   - aLSN이 가리키는 log가  Valid하면 ID_TRUE아니면 ID_FALSE
 ***********************************************************************/
IDE_RC smrRedoLSNMgr::readLog( smLSN      ** aLSN,
                               smrLogHead ** aLogHead,
                               SChar      ** aLogPtr,
                               UInt        * aLogSizeAtDisk,
                               idBool      * aIsValid )
{
    smrLogFile      * sOrgLogFile ;
    iduMemoryHandle * sDecompBufferHandle;
    ULong             sOrgDecompBufferSize;
    smrRedoInfo     * sRedoInfoPtr      = NULL;
    smrLogHead      * sLogHeadPtr;
    idBool            sUnderflow        = ID_FALSE;
#ifdef DEBUG
    smLSN             sDebugLSN;
#endif  

    IDE_DASSERT( aLSN           != NULL );
    IDE_DASSERT( aLogHead       != NULL );
    IDE_DASSERT( aLogPtr        != NULL );
    IDE_DASSERT( aLogSizeAtDisk != NULL );
    IDE_DASSERT( aIsValid       != NULL );

    *aIsValid = ID_TRUE;

    if ( mCurRedoInfoPtr != NULL )
    {
        if ( mCurRedoInfoPtr->mIsValid == ID_TRUE )
        {
            /* BUG-35392
             * Dummy Log 를 건너뛰면서 정상 로그를 읽을때 까지 무한 반복 */
            while(1)
            {
                sDecompBufferHandle = mCurRedoInfoPtr->mDecompBufferHandle;

                sOrgLogFile = mCurRedoInfoPtr->mLogFilePtr;
                sOrgDecompBufferSize = sDecompBufferHandle->getSize();


                /* 마지막으로 Redo연산을 한 smrRedoInfo의 mRedoLSN은
                 * smrRecoveryMgr의 Redo에서 update가 된다. 따라서 다시
                 * mRedoLSN이 가리키는 로그를 읽어서 smrRedoInfo를 갱신하고
                 * 다시 mSortRedoInfo에 넣어야 한다.*/
                IDE_TEST( smrLogMgr::readLog( mCurRedoInfoPtr->mDecompBufferHandle,
                                              &(mCurRedoInfoPtr->mRedoLSN),
                                              ID_FALSE, /* don't Close Log File When aLogFile doesn't include aLSN */
                                              &(mCurRedoInfoPtr->mLogFilePtr),
                                              &(mCurRedoInfoPtr->mLogHead),
                                              &(mCurRedoInfoPtr->mLogPtr),
                                              &(mCurRedoInfoPtr->mIsValid),
                                              &(mCurRedoInfoPtr->mLogSizeAtDisk) )
                          != IDE_SUCCESS);

                if ( mCurRedoInfoPtr->mIsValid == ID_TRUE )
                {

                    // 새로운 로그파일을 읽은 경우
                    if ( sOrgLogFile != mCurRedoInfoPtr->mLogFilePtr )
                    {
                        // 기존 로그파일을 Close한다.
                        // 이유 : makeCopyOfDiskLog의 주석 참고
                        IDE_TEST( smrLogMgr::closeLogFile( sOrgLogFile )
                                  != IDE_SUCCESS );
                    }

                    sLogHeadPtr = &mCurRedoInfoPtr->mLogHead;

                    /* BUG-35392 */
                    if ( smrLogHeadI::isDummyLog( sLogHeadPtr ) == ID_TRUE )
                    {
                        mCurRedoInfoPtr->mRedoLSN.mOffset += mCurRedoInfoPtr->mLogSizeAtDisk;
                        continue;
                    }

                    // 디스크 로그를 읽은 경우
                    if ( smrLogMgr::isDiskLogType( smrLogHeadI::getType( sLogHeadPtr ))
                         == ID_TRUE )
                    {
                        IDE_TEST( makeCopyOfDiskLogIfNonComp( 
                                                    mCurRedoInfoPtr,
                                                    sDecompBufferHandle,
                                                    sOrgDecompBufferSize )
                                  != IDE_SUCCESS );
                    }
                }
                else
                {
                    /* nothing to do */
                }
                break;
            } // end of while
        }
    }

    sRedoInfoPtr = &mRedoInfo;

    if ( sUnderflow == ID_FALSE )
    {
        mCurRedoInfoPtr = sRedoInfoPtr;
    }

    // fix BUG-26934 : [codeSonar] Null Pointer Dereference
    if ( mCurRedoInfoPtr != NULL )
    {
        if ( mCurRedoInfoPtr->mIsValid == ID_TRUE )
        {
#ifdef DEBUG
            sDebugLSN = smrLogHeadI::getLSN( &(mCurRedoInfoPtr->mLogHead) ); 
            IDE_ASSERT( smrCompareLSN::isEQ( &(mCurRedoInfoPtr->mRedoLSN), &sDebugLSN ) );
#endif
            SM_GET_LSN( mLstLSN, mCurRedoInfoPtr->mRedoLSN );
            *aLSN     = &(mCurRedoInfoPtr->mRedoLSN);
            *aLogHead = &(mCurRedoInfoPtr->mLogHead);
            *aLogPtr  = mCurRedoInfoPtr->mLogPtr;
            *aLogSizeAtDisk = mCurRedoInfoPtr->mLogSizeAtDisk;
        }
        else
        {
            *aLSN     = &(mCurRedoInfoPtr->mRedoLSN);
            *aIsValid = ID_FALSE;
        }
    }
    else
    {
        *aIsValid = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Description : 압축버퍼를 사용하지 않은 Disk Log memcpy
 *
 * To Fix BUG-21078
 * 압축되지 않은 DiskLog를 별도의 Memory 공간에 memcpy 한다.
 */
IDE_RC smrRedoLSNMgr::makeCopyOfDiskLogIfNonComp(
                          smrRedoInfo     * aCurRedoInfoPtr,
                          iduMemoryHandle * aDecompBufferHandle,
                          ULong             aOrgDecompBufferSize )
{
    smrLogHead      * sLogHeadPtr;

    sLogHeadPtr = &aCurRedoInfoPtr->mLogHead;

    // 디스크 로그를 읽은 경우
    if ( smrLogMgr::isDiskLogType( smrLogHeadI::getType( sLogHeadPtr ))
         == ID_TRUE )
    {
        // 압축버퍼를 사용하지 않은 경우
        if ( aOrgDecompBufferSize ==
             aDecompBufferHandle->getSize() )
        {
            // 압축되지 않은 로그이다.
            // 원본로그크기와 Disk상의 Log크기가 같아야 함
            IDE_DASSERT( aCurRedoInfoPtr->mLogSizeAtDisk ==
                         smrLogHeadI::getSize(sLogHeadPtr) );

            // To Fix BUG-18686
            //   Disk Log의 Redo시 Decompression Buffer를 기준으로
            //   Hash 된 Log Record들을 Apply
            IDE_TEST( makeCopyOfDiskLog(
                                  aDecompBufferHandle,
                                  aCurRedoInfoPtr->mLogPtr,
                                  smrLogHeadI::getSize(sLogHeadPtr),
                                  & aCurRedoInfoPtr->mLogPtr )
                      != IDE_SUCCESS );

            // 로그 Head는 (aCurRedoInfoPtr->mLogHead)
            // 이미 메모리 복사된 상태이다.
            // 별도 처리 하지 않아도 된다.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// 메모리 핸들로부터 할당한 메모리에 로그를 복사한다.
//
// To Fix BUG-18686
//   Disk Log의 Redo시 Decompression Buffer를 기준으로
//   Hash 된 Log Record들을 Apply
//
// Disk Log의 경우 Page ID기준으로 Hash Table에
// 달아두었다가 한꺼번에 Redo한다.
//
// 로그 레코드는 로그 파일 객체의 로그버퍼
// 메모리를 가리키게 되는데, 이러한 메모리를
// 유지하기 위해서는 로그 파일을
// open해두어야 한다.
//
// 이 경우, Disk Log뿐 아니라 Memory Log까지
// Open된 로그파일 안에 존재하여
// 메모리 낭비가 발생한다.

// 압축해제된 로그의 경우 Decompress Buffer에
// 로그가 있으며, 이 버퍼가 특정 크기 이상으로
// 커지면 Disk Log를 Hash로부터 Buffer에 반영한다.
//
// Disk로그의 경우 압축되지 않은 로그도
// Decompress Buffer로부터 할당된 메모리에
// 로그를 복사하고 이를 PID-Log Hash Table에서
// 가리키도록 한다.
//
// 이렇게 하면 로그파일을 Open된채로 유지할
// 필요가 없으므로,
// 하나의 로그파일을 다 읽게되면 close를 한다.
/*
   [IN] aMemoryHandle - 로그를 복사할 메모리를 할당받을 핸들
   [IN] aOrgLogPtr - 원본로그 주소
   [IN] aOrgLogSize - 원본로그 크기
   [OUT] aCopiedLogPtr -복사된 로그의 주소
 */
IDE_RC smrRedoLSNMgr::makeCopyOfDiskLog( iduMemoryHandle * aMemoryHandle,
                                         SChar           * aOrgLogPtr,
                                         UInt              aOrgLogSize,
                                         SChar          ** aCopiedLogPtr )
{
    IDE_DASSERT( aMemoryHandle != NULL );
    IDE_DASSERT( aOrgLogSize > 0 );
    IDE_DASSERT( aOrgLogPtr != NULL );
    IDE_DASSERT( aCopiedLogPtr != NULL );

    SChar * sLogMemory;

    IDE_TEST( aMemoryHandle->prepareMemory( aOrgLogSize,
                                            (void**) & sLogMemory )
              != IDE_SUCCESS );

    idlOS::memcpy( sLogMemory, aOrgLogPtr, aOrgLogSize );

    *aCopiedLogPtr = sLogMemory;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
