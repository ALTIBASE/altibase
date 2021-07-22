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
 

/*****************************************************************************
 * $Id: csSimul.h 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#ifndef _O_CS_SIMUL_H_
#define _O_CS_SIMUL_H_

#include <stdio.h>
#include <stdlib.h>
#include <cmplrs/atom.inst.h>
#include <cmplrs/atom.anal.h>
#include <assert.h>

#include <csType.h>
#include <csStat.h>

class csSimul
{
    /* ------------------------------------------------
     *  Statictics object
     * ----------------------------------------------*/
    csStat mStat;
    
    /* ------------------------------------------------
     *  Cache Memory Attribute
     * ----------------------------------------------*/
    
    Address  mCapacity;
    Address  mLineSize;
    Address  mWay;
    Address  mCountOfLineEachWay;
    Address *mCacheMem;

    /* ------------------------------------------------
     *  Profiling Action Attribute
     * ----------------------------------------------*/
    
    idBool   mStarted;  // profiling을 시작했나?
    ThreadId mTargetId; // profiling 대상 쓰레드

    static   ThreadMutex mMutex;

public:
    csSimul();
    int initialize(Address aCapacity, Address aLineSize, Address aWay);
    int destroy();

    int doRefer(Address aAddr); // miss가 발생하면 1을 리턴
    void refer(ThreadId      aThreadId, 
               Address       aAddr,
               int           aWriteFlag,
               char         *aFileName,
               char         *aProcName,
               long          aLine);

    void writeHeader(FILE *fp);
    void Save(char *aFileName);
    void Clear();
    
    void setStartFlag(idBool aFlag) { mStarted = aFlag; }
    void setTargetThread(ThreadId aID) { mTargetId = aID; }
    
    void Dump(FILE *);
    void DumpTest(Address aAddr);
};

#endif
