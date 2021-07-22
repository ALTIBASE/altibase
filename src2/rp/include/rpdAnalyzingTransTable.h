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

#ifndef _O_RPD_ANALZYING_TRANS_TABLE_H_
#define _O_RPD_ANALZYING_TRANS_TABLE_H_ 1

#include <idl.h>

#include <smDef.h>

typedef struct rpdAnalyzingTransNode
{
    smTID       mTransID;
    smSN        mBeginSN;
    idBool      mIsActive;
    idBool      mAllowToGroup;
    idBool      mIsReplicationTrans;
    idBool      mIsDDLTrans;
    iduList     mItemMetaList;
} rpdAnalyzingTransNode;

class rpdAnalyzingTransTable
{
    /* Variable */
private:
    rpdAnalyzingTransNode       * mTransTable;
    UInt                          mTransTableSize;

public:


    /* Function */
private:
    void            clearTransNode( rpdAnalyzingTransNode  * aNode );

    UInt            getIndex( smTID    aTransID );

public:
    IDE_RC          initialize( void );
    void            finalize( void );

    void            clearActiveTransTable( void );
    void            clearTransNodeFromTransID( smTID aTransID );

    void            setActiveTrans( smTID  aTransID, 
                                    smSN   aBeginSN );
    idBool          isActiveTrans( smTID     aTransID );

    void            setDisableToGroup( smTID        aTransID );
    idBool          shouldBeGroup( smTID     aTransID );

    smSN            getBeginSN( smTID       aTransID );

    void            setReplicationTrans( smTID     aTransID );
    idBool          isReplicationTrans( smTID    aTransID );

    smTID           getTransIDByIndex( UInt     aIndex );
    smSN            getBeginSNByIndex( UInt     aIndex );
    void            setDisableToGroupByIndex( UInt     aIndex );
    idBool          isActiveTransByIndex( UInt  aIndex );
    idBool          isReplicationTransByIndex( UInt aIndex );

    void            setDDLTrans( smTID aTransID );
    idBool          isDDLTrans( smTID aTransID );

    /* Online DDL */
    IDE_RC          addItemMetaEntry( smTID          aTID,
                                      smiTableMeta * aItemMeta,
                                      const void   * aItemMetaLogBody,
                                      UInt           aItemMetaLogBodySize,
                                      smSN           aLogSN );
    void            getFirstItemMetaEntry( smTID               aTID,
                                           rpdItemMetaEntry ** aItemMetaEntry );
    void            removeFirstItemMetaEntry( smTID aTID );
    idBool          existItemMeta( smTID aTID );
};

#endif
