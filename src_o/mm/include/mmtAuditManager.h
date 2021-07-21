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

#ifndef _O_MMT_AUDIT_MANAGER_H_
#define _O_MMT_AUDIT_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <cm.h>
#include <mmcDef.h>
#include <mmcStatement.h>
#include <iduStringHash.h>
#include <qci.h>

typedef struct mmtAuditMetaInfo
{
    qciAuditOperation       *mObjectOperations;
    UInt                     mObjectOperationCount;

    qciAuditOperation       *mUserOperations;
    UInt                     mUserOperationCount;

    qciAuditOperation       *mAllUserCondition; /* BUG-39311 */

} mmtAuditMetaInfo;

class mmtAuditManager
{
private:
    static idBool             mIsAuditStarted;

    static iduLatch           mAuditMetaLatch;
    
    static mmtAuditMetaInfo   mMetaInfo;

private:
    static void setAuditStarted( idBool aIsStarted );

    static void lockAuditMetaRead();
    static void lockAuditMetaWrite();
    static void unlockAuditMeta();

    /* for object auditing */
    static idBool isAuditWriteObjectByAccess( qciAuditRefObject *aAuditObject,
                                              UInt               aOperIndex,
                                              mmcExecutionFlag   aExecutionFlag );

    static idBool isAuditWriteObjectBySession( qciAuditRefObject *aAuditObject,
                                               UInt              aOperIndex,
                                               idvSQL            *aStat );

    /* for user auditing */
    static idBool isAuditWriteUserByAccess( UInt               aUserID,
                                            UInt               aOperIndex,
                                            mmcExecutionFlag   aExecutionFlag );

    static idBool isAuditWriteUserBySession( UInt          aUserID,
                                             UInt          aOperIndex,
                                             idvSQL       *aStat );

    /* for audit all - BUG-39311 Prioritization of audit conditions */
    static idBool isAuditWriteAllByAccess( UInt               aOperIndex,
                                           mmcExecutionFlag   aExecutionFlag );

    static idBool isAuditWriteAllBySession( UInt          aOperIndex,
                                            idvSQL       *aStat );

public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    /* Auditing status */
    static idBool isAuditStarted();

    /* callback functions to communicate with qp */
    static void reloadAuditCondCallback( qciAuditOperation *aObjectOperations,
                                         UInt               aObjectOperationCount, 
                                         qciAuditOperation *aUserOperations, 
                                         UInt               aUserOperationCount );
    static void stopAuditCallback();
    
    static void auditByAccess( mmcStatement     *aStatement,
                               mmcExecutionFlag  aExecutionFlag );
    
    static void auditBySession( mmcStatement *aStatement );

    /* BUG-41986 Connection info can be audited. */
    static void auditConnectInfo( idvAuditTrail  *aAuditTrail );
    static void initAuditConnInfo( mmcSession    *aSession, 
                                   idvAuditTrail *aAuditTrail,
                                   qciUserInfo   *aUserInfo,
                                   UInt           aResultCode,
                                   UInt           aActionCode );


};

inline void mmtAuditManager::lockAuditMetaRead()
{
    IDE_ASSERT( mAuditMetaLatch.lockRead(NULL, NULL) == IDE_SUCCESS );
}

inline void mmtAuditManager::lockAuditMetaWrite()
{
    IDE_ASSERT( mAuditMetaLatch.lockWrite(NULL, NULL) == IDE_SUCCESS );
}

inline void mmtAuditManager::unlockAuditMeta()
{
    IDE_ASSERT ( mAuditMetaLatch.unlock() == IDE_SUCCESS );
}

#endif
