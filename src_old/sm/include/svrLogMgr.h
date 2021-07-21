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
 
#ifndef _O_SVR_LOGMGR_H_
#define _O_SVR_LOGMGR_H_ 1

#include <svrDef.h>

class svrLogMgr
{
  public:
    /* svrLogMgr을 사용하기 위해 static 멤버들을 초기화한다. */
    static IDE_RC initializeStatic();

    /* svrLogMgr이 사용했던 자원을 해제한다. */
    static IDE_RC destroyStatic();

    /* writeLog를 하기 위해 필요한 자료구조를 초기화한다. */
    static IDE_RC initEnv(svrLogEnv *aEnv, idBool aAlignForce);

    /* writeLog를 하면서 생성된 로그 버퍼 메모리를 모두 해제한다. */
    static IDE_RC destroyEnv(svrLogEnv *aEnv);

    /* svrLogEnv에서 로그를 기록하기 위해 할당한 메모리의 총합 */
    static UInt getAllocMemSize(svrLogEnv *aEnv);

    /* 로그 버퍼에 로그를 기록한다. */
    static IDE_RC writeLog(svrLogEnv *aEnv, svrLog *aLogData, UInt aLogSize);

    /* 로그 버퍼에 sub 로그를 기록한다. */
    static IDE_RC writeSubLog(svrLogEnv *aEnv, svrLog *aLogData, UInt aLogSize);

    /* 사용자가 할당한 메모리 공간에 로그 내용을 읽어  복사한다. */
    static IDE_RC readLogCopy(svrLogEnv *aEnv,
                              svrLSN     aLSNToRead,
                              svrLog    *aBufToLoadAt,
                              svrLSN    *aUndoNextLSN,
                              svrLSN    *aNextSubLSN);

    /* 로그 내용이 기록된 메모리 주소를 반환한다. */
    static IDE_RC readLog(svrLogEnv *aEnv,
                          svrLSN     aLSNToRead,
                          svrLog   **aLogData,
                          svrLSN    *aUndoNextLSN,
                          svrLSN    *aNextSubLSN);

    /* 주어진 LSN 이후의 모든 로그들을 지우고 로그 페이지 메모리를 해제한다.*/
    static IDE_RC removeLogHereafter(svrLogEnv *aEnv, svrLSN aThisLSN);

    /* 사용자가 기록한 마지막 로그 레코드의 LSN을 구한다. */
    static svrLSN getLastLSN(svrLogEnv *aEnv);

    /* 현재 log env에 대해 갱신한 적이 한번이라도 있는지 묻는다. */
    static idBool isOnceUpdated(svrLogEnv *aEnv);
};

#endif /* _O_SVR_LOGMGR_H_ */
