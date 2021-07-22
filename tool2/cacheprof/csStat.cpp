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
 * $Id: csStat.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <csStat.h>

int csStat::initialize(int aHashSize)
{
    long i;
    
    mHashSize = aHashSize;
    
    mNodeBase = (csNode **)calloc(mHashSize, sizeof(csNode *));
    for (i = 0; i < mHashSize; i++)
    {
        mNodeBase[i] = getNode(); // set Dummy Node
    }
    mTotalMiss = 0;
    return 0;
}

int csStat::destroy()
{
    // to do
    return 0;
}

int csStat::update(int aWriteFlag, char *aFileName, char *aProcName, long aLine)
{
    csNode *sNode;

    sNode = search(aProcName, aLine);
    if (sNode != NULL)
    {
        if (aWriteFlag == 0)
        {
            sNode->mReadMiss++;
        }
        else
        {
            sNode->mWriteMiss++;
        }
    }
    else
    {
        insert(aWriteFlag, aFileName, aProcName, aLine);
    }
    mTotalMiss++;
    return 0;
}

int csStat::insert(int aWriteFlag, char *aFileName, char *aProcName, long aLine)
{
    long sHash;
    csNode *sNode;

    sNode = getNode();
    
    sNode->mNext     = NULL;

    memset(sNode->mProcName, 0, 1024);
    strncpy(sNode->mProcName, aProcName, 1023);

    sNode->mFileName = aFileName;
    sNode->mLine     = aLine;
    if (aWriteFlag == 0)
    {
        sNode->mReadMiss =  1;
        sNode->mWriteMiss = 0;
    }
    else
    {
        sNode->mReadMiss  = 0;
        sNode->mWriteMiss = 1;
    }
    
    sHash = hash(aProcName, aLine);

    sNode->mNext = mNodeBase[sHash]->mNext;
    mNodeBase[sHash]->mNext = sNode;

    return 0;
}

csNode* csStat::search(char *aProcName, long aLine)
{
    long sHash;
    csNode *sStartNode;
    csNode *sNode;
    
    sHash      = hash(aProcName, aLine);
    sStartNode = mNodeBase[sHash];

    for (sNode = sStartNode->mNext; sNode != NULL; sNode = sNode->mNext)
    {
        if (strcmp(aProcName, sNode->mProcName) == 0 &&
            aLine == sNode->mLine)
        {
            return sNode;
        }
    }
    return NULL;
}

long csStat::hash(char *aProcName, long aLine)
{
    long sTotal;
    int  i;

    for (i = 0; i < strlen(aProcName) && i < 10; i++)
    {
        sTotal = (long)aProcName[i];
    }
    sTotal += (long)(aLine);

    return (sTotal % mHashSize);
}

void csStat::clearInfo()
{
    long    i;
    csNode *sCurNode, *sBeforeNode;

    //fprintf(aOutput, "Dump of Cache Miss Statistics \n");
    for (i = 0; i < mHashSize;  i++)
    {
        sBeforeNode = NULL;
        for (sCurNode  = mNodeBase[i]->mNext;
             sCurNode != NULL;
             sCurNode  = sCurNode->mNext)
        {
            if (sBeforeNode != NULL)
            {
                freeNode(sBeforeNode);
            }
            
            sBeforeNode = sCurNode;
        }
        if (sBeforeNode != NULL)
        {
            freeNode(sBeforeNode);
        }
        mNodeBase[i]->mNext = NULL;
    }
    mTotalMiss = 0;
}

csNode* csStat::getNode()
{
    csNode *sNode;
    
    sNode = (csNode *)malloc(sizeof(csNode));
    assert(sNode != NULL);
    return sNode;
}

void csStat::freeNode(csNode *aNode)
{
    free(aNode);
}

void csStat::Dump(FILE *aOutput)
{
    long    i;
    csNode *sNode;
    long    sTotalMiss = 0;
    fprintf(aOutput, "Dump of Cache Miss Statistics (Total = %ld) \n", mTotalMiss);
    fprintf(aOutput, "SumOf Miss ||  FileName:Procedure:Line:ReadMiss:WriteMiss \n");
    for (i = 0; i < mHashSize;  i++)
    {
        //fprintf(aOutput, "Num=%ld \n", i);
        for (sNode = mNodeBase[i]->mNext; sNode != NULL; sNode = sNode->mNext)
        {
            fprintf(aOutput,
                    "%10ld ||  %s:%s:%ld:%ld:%ld\n",
                    (sNode->mReadMiss + sNode->mWriteMiss),
                    sNode->mFileName,
                    sNode->mProcName,
                    sNode->mLine,
                    sNode->mReadMiss,
                    sNode->mWriteMiss);
        }
    }
    fflush(aOutput);
}


