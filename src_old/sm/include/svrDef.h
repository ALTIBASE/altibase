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
 
#ifndef _O_SVR_DEF_H_
#define _O_SVR_DEF_H_ 1

#include <smDef.h>

#define SVR_LSN_BEFORE_FIRST      (NULL)

typedef struct svrLog             svrLog;
typedef struct svrLogEnv          svrLogEnv;
typedef struct svrLogRec          svrLogRec;
typedef svrLogRec*                svrLSN;
typedef struct svrLogPage         svrLogPage;

typedef IDE_RC (*svrUndoFunc)(svrLogEnv *aEnv,
                              svrLog    *aLog,
                              svrLSN     aSubLSN);


/* svrLogRec과 svrLog의 차이점 */
/* svrLogRec은 svrLogMgr 내부에서 사용하는 자료구조로서,
   head와 body로 구성된다.
   svrLog는 user log의 최상위 타입으로서
   mType을 가지고 있다. svrLogMgr의 readLog, writeLog시
   대상 타입으로 사용된다. 그리고 svrLog는 svrLogRec의 body에 해당한다. */
typedef struct svrLog
{
    svrUndoFunc     mUndo;
} svrLog;

typedef struct svrLogEnv
{
    svrLogPage     *mHeadPage;
    svrLogPage     *mCurrentPage;

    /* mPageOffset은 로그 레코드가 기록될 위치를 가리킨다.
       mAlignForce가 ID_TRUE이면 mPageOffset은
       항상 align된 상태이어야 한다. */
    UInt            mPageOffset;
    svrLogRec      *mLastLogRec;

    /* 마지막에 기록했던 sub log record */
    svrLogRec      *mLastSubLogRec;
    idBool          mAlignForce;
    UInt            mAllocPageCount;

    /* 이 Env를 사용한 transaction이 기록한 처음 LSN */
    svrLSN          mFirstLSN;
} svrLogEnv;

#endif
