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

#ifndef _O_RPD_LOCK_TABLE_INFO_H_
#define _O_RPD_LOCK_TABLE_INFO_H_ 1

#include <idl.h>

#include <rpdMeta.h>

class smiTrans;
class smiStatement;

typedef struct rpdLockTableInfo
{
    smOID         mTableOID;
    void        * mTableHandle;
    smSCN         mTableSCN;

}rpdLockTableInfo;

class rpdLockTableManager
{
private:
    rpdReplications       mReplication;
    rpdMetaItem         * mMetaItemArray;
    UInt                  mMetaItemArrayCount;
    idBool                mIsLastMetaItem;

    /* replication giveup 시 quick start 로 sender 를 수행한다.
     * 기존 repl sender 가 읽어 있던 sn 기준으로 시작 하는것이 아니라 
     * 최신 redo log 부터 읽기 때문에 build 시점과 build 대상 되는 정보가 변경 되었다.
     * 그렇지만 validate 가 이미 수행 되었다면 다른 thread 에서는 해당 정보를 변경을
     * 못하기 때문에 계속 진행해도 된다.
     */
    idBool                mIsValidateComplete;

    rpdLockTableInfo    * mTableInfoArray;
    UInt                  mTableInfoCount;

public:

private:
    IDE_RC buildLastMetaItemArray( smiStatement      * aSmiStatement,
                                   SChar             * aRepName,
                                   idBool              aIsUpdateFlag,
                                   rpdMetaItem       * aMetaItemArray,
                                   UInt                aMetaItemArrayCount );

    IDE_RC buildOldMetaItemArray( iduVarMemList       * aMemory,
                                  smiStatement        * aSmiStatement,
                                  SChar               * aRepName,
                                  rpdMetaItem         * aMetaItemArray,
                                  UInt                  aMetaItemArrayCount );

    UInt getLockTableCount( void );

    IDE_RC buildLockTable( idvSQL                * aStatistics,
                           smiStatement          * aSmiStatement,
                           rpdReplItems          * aReplItem,
                           rpdLockTableInfo      * aLockTableInfo,
                           UInt                  * aAddTableCount );

    IDE_RC buildLockTableArray( idvSQL           * aStatistics,
                                iduVarMemList    * aMemory,
                                smiStatement     * aSmiStatement );

    IDE_RC equalReplItem( rpdReplItems   *  aItem1,
                          rpdReplItems   *  aItem2 );

    IDE_RC equalMetaItemArray( rpdMetaItem   * aMetaItemArray1,
                               UInt            aMetaItemArrayCount1,
                               rpdMetaItem   * aMetaItemArray2,
                               UInt            aMetaItemArrayCount2 );

public:
    IDE_RC build( idvSQL                * aStatistics,
                  iduVarMemList         * aMemory,
                  SChar                 * aRepName,
                  RP_META_BUILD_TYPE      aMetaBuildType );

    IDE_RC build( idvSQL                * aStatistics,
                  iduVarMemList         * aMemory,
                  smiStatement          * aSmiStatement,
                  SChar                 * aRepName,
                  RP_META_BUILD_TYPE      aMetaBuildType );

    inline idBool needToValidateAndLock( void )
    {
        return mIsLastMetaItem;
    }

    IDE_RC validateAndLock( smiTrans            * aTrans,
                            smiTBSLockValidType   aTBSLvType,
                            smiTableLockMode      aLockMode );

    IDE_RC validateLockTable( idvSQL                * aStatistics,
                              iduVarMemList         * aMemory,
                              smiStatement          * aParentSmiStatement,
                              SChar                 * aRepName,
                              RP_META_BUILD_TYPE      aMetaBuildType );

};

#endif
