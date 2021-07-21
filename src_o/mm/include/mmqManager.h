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

#ifndef _O_MMQ_MANAGER_H_
#define _O_MMQ_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smuHash.h>

class mmqQueueInfo;

class mmqManager
{
    friend class mmqQueueInfo;

    static iduMutex    mMutex;
    static iduMemPool  mQueueInfoPool;
    static smuHashBase mQueueInfoHash;

public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC findQueue(UInt aTableID, mmqQueueInfo **aQueueInfo);

    static IDE_RC freeQueue(mmqQueueInfo *aQueueInfo);
    static IDE_RC freeQueueInfo(mmqQueueInfo *aQueueInfo);

public:
    /*
     * Queue Callback from QP
     */

    static IDE_RC createQueue(void *aArg);
    static IDE_RC dropQueue(void *aArg);
    static IDE_RC enqueue(void *aArg);
    //PROJ-1677
    static IDE_RC dequeue(void *aArg,idBool aBookDeq);
    static IDE_RC setQueueStamp(void* aArg);
};

#endif
