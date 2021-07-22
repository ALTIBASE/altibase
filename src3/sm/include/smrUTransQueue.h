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
 * $Id: smrUTransQueue.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMR_UTRANS_QUEUE_H_
#define _O_SMR_UTRANS_QUEUE_H_ 1

#include <smDef.h>
#include <iduPriorityQueue.h>
#include <smrLogFile.h>

typedef struct smrUndoTransInfo
{
    void        *mTrans;
    /* mTrans의 mLstUndoNxtLSN이 가리키는 Log의 LogHead */
    smrLogHead   mLogHead;
    /* mTrans의 mLstUndoNxtLSN이 가리키는 Log의 LogBuffer Ptr */
    SChar*       mLogPtr;
    /* mTrans의 mLstUndoNxtLSN이 가리키는 Log가 있는 Logfile Ptr */
    smrLogFile*  mLogFilePtr;
    /* mTrans의 mLstUndoNxtLSN이 가리키는 Log가 Valid하면 ID_TRUE, 아니면 ID_FALSE */
    idBool       mIsValid;
         
    /* 로그파일로 부터 읽은 로그의 크기
       압축된 로그의 경우 로그의 크기와 로그파일상의 로그크기가 다르다
     */
    UInt         mLogSizeAtDisk;
} smrUndoTransInfo;

class smrUTransQueue
{
public:

    IDE_RC initialize(SInt aTransCount);
    IDE_RC destroy();

    IDE_RC insert(smrUndoTransInfo*);
    smrUndoTransInfo* remove();

    static SInt compare(const void *arg1,const  void *arg2);
    IDE_RC insertActiveTrans(void* aTrans);
    
        
    SInt getItemCount() {return mPQueueTransInfo.getDataCnt();}

    smrUTransQueue();
    virtual ~smrUTransQueue();

private:
    /*현재 Undo할 Transactino의 갯수 */
    UInt              mCurUndoTransCount;
    /*Undo Transaction의 최대 갯수 */
    UInt              mMaxTransCount;

    /*smrUndoTransInfo Array */
    smrUndoTransInfo *mArrUndoTransInfo;

    /*smrUndoTransInfo를 mLoghead의 mSN이 큰 순서로
      Sorting해서 유지하는 Sort Array */
    iduPriorityQueue      mPQueueTransInfo;
};

#endif /* _O_SMR_UTRANS_QUEUE_H_ */
