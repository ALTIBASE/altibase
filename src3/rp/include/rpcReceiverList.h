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

#ifndef _O_RPC_RECEIVER_LIST_H_
#define _O_RPC_RECEIVER_LIST_H_ 1

#include <idl.h>
#include <rpxReceiver.h>

typedef enum rpcReceiverListStatus
{
    RPC_RECEIVER_LIST_UNUSED,
    RPC_RECEIVER_LIST_RESERVED,
    RPC_RECEIVER_LIST_USED
}rpcReceiverListStatus;

typedef struct rpcReceiver 
{
    rpxReceiver           * mReceiver;
    rpcReceiverListStatus   mStatus;
} rpcReceiver;

class rpcReceiverList
{
private:
    iduMutex             mReceiverMutex;

    rpcReceiver        * mReceiverList;
    UInt                 mMaxReceiverCount;

public:
    IDE_RC initialize( const UInt    aMaxReceiverCount );
    void finalize( void );

    void lock( void );
    void tryLock( idBool   & aIsLock );
    void unlock( void );

    IDE_RC getUnusedIndexAndReserve( UInt  * aReservedIndex );
    void setReceiver( const UInt          aReservedIndex,
                      rpxReceiver       * aReceiver );
    void unsetReceiver( const UInt     aReservedIndex );

    IDE_RC setAndStartReceiver( const UInt       aReservedIndex,
                                rpxReceiver    * aReceiver );

    rpxReceiver * getReceiver( const SChar    * aReplName );
    rpxReceiver * getReceiver( const UInt aIndex );

    inline UInt getMaxReceiverCount( void )
    {
        return mMaxReceiverCount;
    };

private:

};

#endif
