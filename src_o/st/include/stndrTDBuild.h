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
 * $Id: stndrTDBuild.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 * Buffer Flush 쓰레드
 *
 * # Concept
 *
 * - 버퍼 매니저의 dirty page list를 주기적으로
 *   flush 하는 쓰레드
 *
 * # Architecture
 * 
 * - 서버 startup시 하나의 buffer flush 쓰레드 동작
 * 
 **********************************************************************/

#ifndef _O_STNDR_TDBUILD_H_
#define _O_STNDR_TDBUILD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <idv.h>
#include <smcDef.h>
#include <smnDef.h>

class stndrTDBuild : public idtBaseThread
{
public:

    /* 쓰레드 초기화 */
    IDE_RC initialize( UInt             aTotalThreadCnt,
                       UInt             aID,
                       smcTableHeader * aTable,
                       smnIndexHeader * aIndex,
                       smSCN            aTxBeginSCN,
                       idBool           aIsNeedValidation,
                       UInt             aBuildFlag,
                       idvSQL         * aStatistics,
                       idBool         * aContinue );
    
    IDE_RC destroy();         /* 쓰레드 해제 */
    
    virtual void run();       /* main 실행 루틴 */

    stndrTDBuild();
    virtual ~stndrTDBuild();

    inline UInt getErrorCode() { return mErrorCode; };
    inline ideErrorMgr* getErrorMgr() { return &mErrorMgr; };

private:

    idBool            mFinished;    /* 쓰레드 실행 여부 flag */
    idBool          * mContinue;    /* 쓰레드 중단 여부 flag */
    UInt              mErrorCode;
    ideErrorMgr       mErrorMgr;
    
    UInt              mTotalThreadCnt;
    UInt              mID;
    void            * mTrans;
    smcTableHeader  * mTable;
    smnIndexHeader  * mIndex;
    idBool            mIsNeedValidation;
    UInt              mBuildFlag;
    idvSQL          * mStatistics;
};

#endif // _O_STNDR_TDBUILD_H_





