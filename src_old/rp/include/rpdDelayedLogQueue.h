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
 * $Id: 
 **********************************************************************/

#ifndef _O_RPD_DELAYED_LOG_QUEUE_H_
#define _O_RPD_DELAYED_LOG_QUEUE_H_ 1

#include <idl.h>
#include <smiLogRec.h>

typedef struct rpdDelayedLogQueueNode rpdDelayedLogQueueNode;

typedef struct rpdDelayedLogQueueNode
{
    void        * mLogPtr;
    smLSN         mReadLSN;
    rpdDelayedLogQueueNode  * mNext;
} rpdDelayedLogQueueNode;

class rpdDelayedLogQueue
{
    /* Variable */
private:
    iduMemPool      mNodePool;

    rpdDelayedLogQueueNode      * mHead;
    rpdDelayedLogQueueNode      * mTail;

public:

    /* Function */
private:

    IDE_RC          initailizeNode( rpdDelayedLogQueueNode     ** aNode );
    void            setObject( rpdDelayedLogQueueNode  * aNode,
                               void                    * aLogPtr,
                               smLSN                   * aReadLSN );

public:
    IDE_RC          initialize( SChar   * aRepName );
    void            finalize( void );

    IDE_RC          enqueue( void   * aLogPtr, 
                             smLSN  * aReadLSN );

    IDE_RC          dequeue( void       ** aLogPtr,
                             smLSN       * aReadLSN,
                             idBool      * aIsEmpty );

    IDE_RC          dequeueALL();

    idBool          isEmpty( void );

};
#endif
