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
 * $Id: rpxPJChild.h 83319 2018-06-21 06:51:10Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_RPX_PJ_CHILD_H_    // Parallel Job Child
#define _O_RPX_PJ_CHILD_H_ 1

#include <idl.h>
#include <ideErrorMgr.h>
#include <idtBaseThread.h>

#include <cm.h>
#include <qcuProperty.h>

#include <rp.h>
#include <rpdMeta.h>
#include <rpxSender.h>
#include <rpnMessenger.h>

// signal section
#define RPX_PJ_SIGNAL_NONE     0x00000001 // not Running : Just destroyed
#define RPX_PJ_SIGNAL_EXIT     0x00000002 // Normal Á¾·á : to be join..
#define RPX_PJ_SIGNAL_ERROR    0x00000004 // Error
#define RPX_PJ_SIGNAL_RUNNING  0x00000008 // Running : Now Running
#define RPX_PJ_SIGNAL_SLEEP    0x00000010 // sleep

class rpxPJMgr;
class smiStatement;
class rpnMessenger;

class rpxPJChild : public idtBaseThread

{

private:
    UInt               mChildCount;
    UInt               mNumber;
    UInt               mStatus;
    rpxPJMgr         * mParent;
    rpdMetaItem      * mTable;
    iduList          * mSyncList;

    rpdMeta          * mMeta;
    idBool           * mExitFlag;

    smiStatement     * mStatement;

    rpnMessenger       mMessenger;

    RP_SOCKET_TYPE     mSocketType;
    SChar            * mIPAddress;
    UInt               mPortNo;
    rpIBLatency        mIBLatency;
    
    iduListNode * getFirstNode();
    iduListNode * getNextNode( iduListNode *aNode );
    IDE_RC doSync( rpxSyncItem * aSyncItem );

public:
    rpxPJChild();

    IDE_RC initialize( rpxPJMgr     * aParent,
                       rpdMeta      * aMeta,
                       RP_SOCKET_TYPE aSocketType,
                       SChar        * aIPAddress,
                       UInt           aPortNo,
                       rpIBLatency    aIBLatency,
                       smiStatement * aStatement,
                       UInt           aChildCount,
                       UInt           aNumber,
                       iduList      * aSyncList,
                       idBool       * aExitFlag ); 
    void   destroy();

    /* BUG-38533 numa aware thread initialize */
    IDE_RC initializeThread();
    void   finalizeThread();

    UInt   getStatus()
    {
        return mStatus;
    }

    void   run();
};

#endif
