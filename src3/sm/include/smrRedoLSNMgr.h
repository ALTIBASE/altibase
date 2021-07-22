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
 * $Id: smrRedoLSNMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMR_REDO_LSN_MGR_H_
#define _O_SMR_REDO_LSN_MGR_H_ 1

#include <smDef.h>
#include <smrLogFile.h>
#include <iduMemoryHandle.h>

/* Parallel Logging : 이정보를 Redo시
   smrRedoLSNMgr에서 유지한다. smrRedoLSNMgr은 smrRedoInfo를
   mSN으로 Sorting하고 Redo시 가장작은
   mSN값을 가지는 smrRedoInfo의 mRedoLSN이 가지키는 Log부터
   Redo를 한다.*/
typedef struct smrRedoInfo
{
    /* Redo할 Log LSN */
    smLSN       mRedoLSN;
    /* mRedoLSN이 가리키는 Log의 LogHead */
    smrLogHead  mLogHead;
    /* mRedoLSN이 가리키는 Log의 LogBuffer Ptr */
    SChar*      mLogPtr;
    /* mRedoLSN이 가리키는 Log가 있는 logfile Ptr */
    smrLogFile* mLogFilePtr;
    /* mRedoLSN이 가리키는 Log가 Valid하면 ID_TRUE, 아니면 ID_FALSE */
    idBool      mIsValid;

    /* 로그 압축해제버퍼 핸들*/
    iduMemoryHandle * mDecompBufferHandle;

    /* 로그파일로 부터 읽은 로그의 크기
       압축된 로그의 경우 로그의 크기와 로그파일상의 로그크기가 다르다
     */
    UInt         mLogSizeAtDisk;
} smrRedoInfo;

class smrRedoLSNMgr
{
public:
    static IDE_RC initialize( smLSN *aRedoLSN );
    static IDE_RC destroy();


    // Decompress Log Buffer 크기를 얻어온다
    static ULong getDecompBufferSize();

    // Decompress Log Buffer가 할당한 모든 메모리를 해제한다.
    static IDE_RC clearDecompBuffer();

   /*
    mSortRedoInfo에 들어있는 smrRedoInfo중에서 가장 작은 mSN값을
    가진 Log를 읽어들인다.
   */
    static IDE_RC readLog(smLSN         ** aLSN,
                          smrLogHead    ** aLogHead,
                          SChar         ** aLogPtr,
                          UInt           * aLogSizeAtDisk,
                          idBool         * aIsValid);

    /* Check한 또는 Check할 log의 LSN값 리턴*/
    /* 호출 시점에 따라 달라짐 */
    static inline smLSN getLstCheckLogLSN()
    {
        return mRedoInfo.mRedoLSN;
    }

    /* 마지막 Redo 로그의 LSN값을 리턴*/
    static inline smLSN getNextLogLSNOfLstRedoLog()
    {
        return mCurRedoInfoPtr->mRedoLSN;
    }

    /* smrRedoInfo를 Invalid하게 만든다.*/
    static void setRedoLSNToBeInvalid();

    smrRedoLSNMgr();
    virtual ~smrRedoLSNMgr();

private:
    static SInt   compare(const void *arg1,const void *arg2);

    // Redo Info를 초기화한다
    static IDE_RC initializeRedoInfo( smrRedoInfo * aRedoInfo );

    // Redo Info를 파괴한다
    static IDE_RC destroyRedoInfo( smrRedoInfo * aRedoInfo );


    // Redo Info를 Sort Array에 Push한다.
    static IDE_RC pushRedoInfo( smrRedoInfo * aRedoInfo,
                                smLSN *aRedoLSN );


    // 메모리 핸들로부터 할당한 메모리에 로그를 복사한다.
    static IDE_RC makeCopyOfDiskLog( iduMemoryHandle * aMemoryHandle,
                                     SChar *      aOrgLogPtr,
                                     UInt         aOrgLogSize,
                                     SChar**      aCopiedLogPtr );

    static IDE_RC makeCopyOfDiskLogIfNonComp(
                          smrRedoInfo     * aCurRedoInfoPtr,
                          iduMemoryHandle * aDecompBufferHandle,
                          ULong             aOrgDecompBufferSize );
private:
    /* Redo정보를 가지고 있다. */
    static smrRedoInfo   mRedoInfo;
    /* 현재 Redo를 수행중인 smrRedoInfo */
    static smrRedoInfo * mCurRedoInfoPtr;
    /* 마지막 Redo를 수행한 Redo Log의 mLSN */
    static smLSN         mLstLSN;

};

/* smrRedoInfo를 Invalid하게 만든다.*/
inline void smrRedoLSNMgr::setRedoLSNToBeInvalid()
{
    mRedoInfo.mIsValid = ID_FALSE;
}

#endif /* _O_SMR_REDO_LSN_MGR_H_ */
