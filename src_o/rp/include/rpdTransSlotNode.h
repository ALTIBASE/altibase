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

#ifndef _O_RPD_TRANS_SLOT_NODE_H_
#define _O_RPD_TRANS_SLOT_NODE_H_ 1

#include <idl.h>

#include <smDef.h>

class rpdReplicatedTransGroupNode;

class rpdTransSlotNode
{
    /* Variable */
private:
    SInt                              mTransSlotIndex;
    rpdReplicatedTransGroupNode     * mNext;

public:


    /* Function */
private:

public:
    void            initialize( void );
    void            finalize( void );

    void            setTransInfo( UInt       aTransSlotIndex );

    rpdReplicatedTransGroupNode * 
                    getNextReplicatedTransGroupNode( void );
    void            setNextReplicatedTransGroupNode( rpdReplicatedTransGroupNode   * aNode );

    UInt            getTransSlotIndex( void );

    IDE_RC          buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                           smTID                 aGroupTransID,
                                                           void                * aHeader,
                                                           void                * aDumpObj,
                                                           iduFixedTableMemory * aMemory );

};

#endif
