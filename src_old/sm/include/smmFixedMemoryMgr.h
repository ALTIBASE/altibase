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
 * $Id: smmFixedMemoryMgr.h 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

#ifndef _O_SMM_FIXED_MEMORY_MGR_H_
#define _O_SMM_FIXED_MEMORY_MGR_H_ 1
#include <idl.h>
#include <idu.h>
#include <smm.h>
#include <smmDef.h>

#include <iduMemListOld.h>



struct smmMemBase;

class smmFixedMemoryMgr
{
    static iduMemListOld   mSCHMemList;
    
    static void setValidation(smmTBSNode * aTBSNode, idBool aFlag);

private:
    smmFixedMemoryMgr();
    ~smmFixedMemoryMgr();
    
public:
    /* ------------------------------------------------
     * [] Class Initialization & Destroy
     * ----------------------------------------------*/
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();
    static IDE_RC initialize(smmTBSNode * aTBSNode);

    // TBSNode의 정보중 공유메모리 관리자 관련 부분을 해제한다.
    static IDE_RC destroy(smmTBSNode * aTBSNode);

    /* ------------------------------------------------
     * [] create & delete &  attach & detach
     * ----------------------------------------------*/
    static IDE_RC attach(smmTBSNode * aTBSNode, smmShmHeader *aRsvShmHeader);
    static IDE_RC detach(smmTBSNode * aTBSNode);
    static IDE_RC create(smmTBSNode * aTBSNode, UInt aPageCount);
    static IDE_RC remove(smmTBSNode * aTBSNode);

    /* ------------------------------------------------
     *  [] Allocation & Deallocation 
     * ----------------------------------------------*/
    static inline void linkShmPage(smmTBSNode * aTBSNode,
                                   smmTempPage   *aPage);
    static IDE_RC allocNewChunk(smmTBSNode * aTBSNode,
                                key_t        aShmkey,
                                UInt         aCount);
    static IDE_RC extendShmPage(smmTBSNode * aTBSNode,
                                UInt         aCount,
                                key_t      * aCreatedShmKey = NULL );
    
    static IDE_RC freeShmPage(smmTBSNode *  aTBSNode,
                              smmTempPage * aPage);
    static IDE_RC allocShmPage(smmTBSNode *  aTBSNode,
                               smmTempPage **aPage);

    // Tablespace의 첫번째 공유메모리 Chunk를 생성한다.
    static IDE_RC createFirstChunk( smmTBSNode * aTBSNode,
                                    UInt         aPageCount);
    
    /* ------------------------------------------------
     * [] Check of DB Infomation
     * ----------------------------------------------*/
    static IDE_RC checkExist(key_t         aShmKey,
                             idBool&       aExist,
                             smmShmHeader *aHeader);
    
    static IDE_RC checkDBSignature(smmTBSNode * aTBSNode,
                                   UInt         aDbNumber);

    /* ------------------------------------------------
     * [*] Database Validation for Shared Memory
     * ----------------------------------------------*/
    static void validate(smmTBSNode * aTBSNode)
    {
        setValidation(aTBSNode, ID_TRUE);
    }
    static void invalidate(smmTBSNode * aTBSNode)
    {
        setValidation(aTBSNode, ID_FALSE);
    }

    /* ------------------------------------------------
     * [] Get Information from SmmFixedMemoryMgr
     * ----------------------------------------------*/
    static key_t  getShmKey(smmTBSNode * aTBSNode)
    {
        return aTBSNode->mTBSAttr.mMemAttr.mShmKey;
    }
    static smmSCH* getBaseSCH(smmTBSNode * aTBSNode)
    {
        return &aTBSNode->mBaseSCH;
    }
};

#endif
