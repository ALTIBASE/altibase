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
 * $Id: rpcValidate.h 53446 2012-06-12 15:59:00Z junyoung $
 **********************************************************************/

#ifndef  _O_RPC_VALIDATE_H_
#define _O_RPC_VALIDATE_H_  1

#include <qci.h>

#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_USER_NAME_MASK       (0x00000001)
#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_USER_NAME_SET        (0x00000001)
#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_USER_NAME_UNSET      (0x00000000)

#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_TABLE_NAME_MASK      (0x00000002)
#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_TABLE_NAME_SET       (0x00000002)
#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_TABLE_NAME_UNSET     (0x00000000)

#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_PARTITION_NAME_MASK  (0x00000004)
#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_PARTITION_NAME_SET   (0x00000004)
#define RPC_VALIDATE_CHECK_LOCAL_REMOTE_PARTITION_NAME_UNSET (0x00000000)



class rpcValidate
{
public:
    static qciValidateReplicationCallback mCallback;

    static IDE_RC validateCreate(void * aQcStatement);
    static IDE_RC validateOneReplItem(void * aQcStatement,
                                      SChar        * aReplName,
                                      qriReplItem  * aReplItem,
                                      SInt          aRole,
                                      idBool        aIsDDLReplicateOpt,
                                      idBool        aIsRecoveryOpt,
                                      SInt          aReplMode);
    static IDE_RC validateAlterAddTbl(void * aQcStatement);
    static IDE_RC validateAlterDropTbl(void * aQcStatement);
    static IDE_RC validateAlterAddHost(void * aQcStatement);
    static IDE_RC validateAlterDropHost(void * aQcStatement);
    static IDE_RC validateAlterSetHost(void * aQcStatement);
    static IDE_RC validateAlterSetMode(void * aQcStatement);
    static IDE_RC validateDrop(void * aQcStatement);
    static IDE_RC validateStart(void * aQcStatement);

    /* PROJ-1915 */
    static IDE_RC validateOfflineStart(void * aQcStatement);
    static IDE_RC validateQuickStart(void * aQcStatement);
    static IDE_RC validateSync(void * aQcStatement);
    static IDE_RC validateSyncTbl(void * aQcStatement);
    static IDE_RC validateTempSync(void * aQcStatement);
    static IDE_RC validateReset(void * aQcStatement);
    static IDE_RC validateAlterSetRecovery(void * aQcStatement);
    static IDE_RC validateAlterSetOffline(void * aQcStatement);

    static idBool isValidIPFormat(SChar * aIP);

    /* PROJ-1969 */
    static IDE_RC validateAlterSetGapless(void * aQcStatement);
    static IDE_RC validateAlterSetParallel(void * aQcStatement);
    static IDE_RC validateAlterSetGrouping(void * aQcStatement);

    static IDE_RC validateAlterSetDDLReplicate( void * aQcStatement );

    /* BUG-42851 */
    static IDE_RC validateAlterPartition( void * aQcStatement,
                                          qcmTableInfo * aPartInfo );


    static IDE_RC validateDeleteItemReplaceHistory(void * aQcStatement);
    static IDE_RC validateFailback(void * aQcStatement);

    static IDE_RC validateFailover( void * aQcStatement );

private:
    static IDE_RC lockAllPartitionForDDLValidation( void                 * aQcStatement,
                                                    qcmPartitionInfoList * aPartInfoList );

    static IDE_RC applierBufferSizeCheck( UChar aType, ULong aSize );

    static IDE_RC makeTableOIDArray( void         * aQcStatement,
                                     UInt           aReplItemCount,
                                     qriReplItem  * aReplItems,
                                     UInt         * aTableOIDCount,
                                     smOID       ** aTableOIDArray );

    static IDE_RC allocAndBuildLockTable( idvSQL               * aStatistics,
                                          iduVarMemList        * aMemory,
                                          smiStatement         * aSmiStatement,
                                          SChar                * aRepName,
                                          RP_META_BUILD_TYPE     aMetaBuildType,
                                          void                ** aLockTable );

    static IDE_RC validateCheckLocalRemoteName( void * aReplItem, UInt aCheckMask );

};

#endif  // _O_RPC_VALIDATE_H_
