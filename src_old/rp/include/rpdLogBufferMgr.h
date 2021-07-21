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
 * $Id: rpdLogBufferMgr.h 22507 2007-07-10 01:29:09Z lswhh $
 **********************************************************************/

#ifndef _O_RPD_LOGBUFFERMGR_H_
#define _O_RPD_LOGBUFFERMGR_H_

#include <rp.h>
#include <smiLogRec.h>

/*
 * Write pointer: Service가 로그를 복사할 때 무조건 갱신
 * Writable Flag(Copy Mode Flag): Service가 로그 버퍼에 복사하는 것을 포기할 때 OFF/
 *                  Service가 Accessing Info Count확인(Sender가 모든 로그를 다 읽어가면) 후 ON/
 *                  Sender가 진입 가능한지 확인할 때 읽음/ 더 읽을 로그가 없을 때 확인
 * Accessing Info Count : Sender가 진입할 때와 나올때 갱신/
 * Min Read Pointer:Service가 로그를 복사할때 필요한 경우 갱신/
 *                  Sender가 진입할때 갱신 / Sender가 읽고 있는 버퍼의 바로 앞을 가리키고 있음
 * Min SN,  Max SN: Service가 모든 로그 처리시 갱신
 */
#define RP_BUF_NULL_PTR     (ID_UINT_MAX)
#define RP_BUF_NULL_ID      (ID_UINT_MAX)

typedef enum
{
    RP_ON = 0,
    RP_OFF
} flagSwitch;


typedef struct AccessingInfo
{
    UInt    mReadPtr;
    UInt    mNextPtr;
} AccessingInfo;
/***************************************************************************************
 *   RP LOG Buffer는 service thread가 로그를 write하고 Sender가 읽어가는 구조로 되어있음,
 *   환형으로 유지되기 때문에 Sender가 읽는 위치가 Service Thread가 쓰는 위치를 따라갔는지, 
 *   Service Thread가 쓰는 위치가 Sender가 읽는 위치를 따라갔는지 확인할 수 있어야 하며, 
 *   이것을 구분하기 위해 Service Thread가 쓰기를 완료한 곳의 위치(mWrotePtr)와 Sender가 
 *   읽기를 완료한 곳의 위치(mMinReadPtr)를 기억한다. Sender는 Service Thread가 쓰기를 완료한 
 *   위치인 mWritePtr까지 mMinReadPtr을 이동할 수 있으며, 두 값이 같은 경우  
 *   Service Thread가 쓴 곳을 Sender가 모두 읽었다는 뜻이된다.
 *   그리고 Service Thread는 mMinReadPtr이 가리키는 곳에 쓰기를 할 수 없도록 보호하기 때문에 
 *   Service Thread가 Sender가 읽는 곳을 끝까지 따라간다고 하더라도 두 값이 같아 질 수 없다.
 ***************************************************************************************/
class rpdLogBufferMgr
{
public:
    IDE_RC initialize(UInt aSize, UInt aMaxSenderCnt);

    void   destroy();
    void   finalize();

    SChar*        mBasePtr;
    UInt          mBufSize;
    UInt          mMaxSenderCnt;
    UInt          mEndPtr;

    smiDummyLog   mBufferEndLog;
    UInt          mBufferEndLogSize;


    /*Buffer Info*/
    /* mMinReadPtr: 버퍼의 최소 위치 (Sender가 읽은 버퍼의 최소 위치로 버퍼의 최소 위치를 정함)
     * Sender는 진입 시 사용하고, Service는 overwrite하지 않기 위해사용*/
    UInt           mMinReadPtr;
    /*Service Thread가 쓰기를 완료한 버퍼의 위치*/
    UInt           mWrotePtr;
    /*Service Thread가 다음번에 써야 할 버퍼의 위치*/
    UInt           mWritePtr;

    /* Service Thread가 버퍼에 작업을 해야하는 지 여부를 나타냄
     * Sender는 Service Thread가 버퍼에 작업을 하고 있는지 여부를 확인하는 데 사용
     */
    flagSwitch     mWritableFlag;

    iduMutex       mBufInfoMutex;
    /*버퍼의 최소 SN과 최대 SN*/
    smSN           mMinSN;
    smSN           mMaxSN;
    smLSN          mMaxLSN;
    /*버퍼에서 로그를 읽고 있는 Sender들에 관한 정보*/
    UInt           mAccessInfoCnt;
    AccessingInfo* mAccessInfoList;

    /*aCopyFlag가 TRUE일 때 Log를 Buffer에 복사한다, 그렇지 않으면 Control정보만 변경한다*/
    IDE_RC copyToRPLogBuf(idvSQL * aStatistics,
                          UInt     aSize, 
                          SChar  * aLogPtr, 
                          smLSN    aLSN);



    /*Log를 Buffer에서 다음에 읽어야하는 로그의 포인터를 넘겨준다.*/
    IDE_RC readFromRPLogBuf(UInt       aSndrID,
                            smiLogHdr* aLogHead,
                            void**     aLogPtr,
                            smSN*      aSN,
                            smLSN*     aLSN,
                            idBool*    aIsLeave);

    /*buffer모드로 진입 가능한지 테스트 하고 모드를 변경한 후 ID를 반환한다. by sender*/
    idBool tryEnter(smSN    aNeedSN,
                    smSN*   aMinSN,
                    smSN*   aMaxSN,
                    UInt*   aSndrID );

    void getSN(smSN* aMinSN, smSN* aMaxSN);

    void leave(UInt aSndrID);

private:
    void updateMinReadPtr();

    IDE_RC writeBufferEndLogToRPBuf();
    void printBufferInfo();
    inline idBool isBufferEndLog( smiLogHdr * aLogHdr )
    {
        return smiLogRec::isDummyLog( aLogHdr );
    }

    inline UInt getWritePtr(UInt aWrotePtr)
    {
        return ((aWrotePtr + 1) % mBufSize);
    }
    inline UInt getReadPtr(UInt aReadPtr)
    {
        return ((aReadPtr + 1) % mBufSize);
    }
};

#endif
