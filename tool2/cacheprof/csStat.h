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
 * $Id: csStat.h 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#ifndef _O_CS_STAT_H_
#define _O_CS_STAT_H_

/* ------------------------------------------------
 *  통계정보를 저장 및 출력한다.
 * ----------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <cmplrs/atom.inst.h>
#include <cmplrs/atom.anal.h>
#include <assert.h>
#include <csType.h>

typedef struct csNode
{
    csNode *mNext;

    Address mAddr;
    char   *mFileName;
    char    mProcName[1024];
    long    mLine;
    long    mReadMiss;
    long    mWriteMiss;
        
} csNode;

class csStat
{
    /* ------------------------------------------------
     *  Hash Attribute
     * ----------------------------------------------*/

    long     mHashSize;
    long     mTotalMiss;
    csNode **mNodeBase;

    csNode* search(char *aProcName, long aLine);
    int     insert(int     aWriteFlag,
                   char   *aFileName,
                   char   *aProcName,
                   long    aLine);
    csNode* getNode();
    void    freeNode(csNode *);

    long    hash(char *aProcName, long aLine);
    
public:
    csStat() { }
    int initialize(int aHashSize);
    int destroy();

    int update(int aWriteFlag, char *aFileName, char *aProcName, long aLine);

    void clearInfo();
    
    void Dump(FILE *aOutput);
    
};

#endif
