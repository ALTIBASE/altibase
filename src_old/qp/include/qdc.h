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
 * $Id: qdc.h 82630 2018-03-27 00:23:41Z kclee $
 **********************************************************************/
#ifndef _O_QDC_H_
#define _O_QDC_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

typedef enum qdcFeatureVersion
{
    QDC_OPTIMIZER_FEATURE_VERSION_NONE = 1,
    QDC_OPTIMIZER_FEATURE_VERSION_6_1_1_0_6,
    QDC_OPTIMIZER_FEATURE_VERSION_6_1_1_0_7,
    QDC_OPTIMIZER_FEATURE_VERSION_6_1_1_4_3,
    QDC_OPTIMIZER_FEATURE_VERSION_6_3_1_0_1,
    QDC_OPTIMIZER_FEATURE_VERSION_6_5_1_0_0,
    QDC_OPTIMIZER_FEATURE_VERSION_7_1_0_0_0,
    QDC_OPTIMIZER_FEATURE_VERSION_MAX
} qdcFeatureVersion;

typedef struct qdcFeatureProperty
{
    SChar * mName;       // optimizer property name
    SChar * mOldValue;   // old value
    SChar * mNewValue;   // new value
    qdcFeatureVersion    mVersion;    // enum for version compare
    qcPlanPropertyKind   mPlanName;   // enum for plan property
} qdcFeatureProperty;

// qd Common
//    -- alter system checkpoint, backup, compact
class qdc
{
public:
    // validate
    static IDE_RC validate( qcStatement * aStatement );

    static IDE_RC validateCreateDatabase(qcStatement * aStatement);
    static IDE_RC validateAlterDatabase(qcStatement * aStatement);
    /* BUG-39257 The statement 'ALTER DATABASE' has many parsetrees */
    static IDE_RC validateAlterDatabaseOpt2(qcStatement * aStatement);
    static IDE_RC validateDropDatabase(qcStatement * aStatement);

    /* PROJ-2626 Snapshot Export */
    static IDE_RC validateAlterDatabaseSnapshot( qcStatement * aStatement );

    // execute
    static IDE_RC execute( qcStatement * aStatement );

    static IDE_RC checkpoint( qcStatement * aStatement );

    static IDE_RC shrinkMemPool( qcStatement * aStatement );

    static IDE_RC dumpAllCallstacks( qcStatement * aStatement );

    static IDE_RC reorganize( qcStatement * aStatement );

    static IDE_RC verify( qcStatement * aStatement );

    static IDE_RC setSystemProperty( qcStatement * aStatement, idpArgument *aArg );

    static IDE_RC checkExecDDLdisableProperty( void );

    static IDE_RC executeCreateDatabase(qcStatement * aStatement);
    static IDE_RC executeAlterDatabase(qcStatement * aStatement);
    static IDE_RC executeUpgradeMeta(qcStatement * aStatement);
    static IDE_RC executeDropDatabase(qcStatement * aStatement);

    static IDE_RC archivelog( qcStatement * aStatement );
    static IDE_RC alterArchiveMode( qcStatement * aStatement );
    static IDE_RC executeFullBackup( qcStatement* aStatement );
    static IDE_RC executeMediaRecovery( qcStatement* aStatement );

    static IDE_RC executeIncrementalBackup( qcStatement* aStatement );
    static IDE_RC executeRestore( qcStatement* aStatement );
    static IDE_RC executeChangeTracking( qcStatement* aStatement );
    static IDE_RC executeRemoveBackupInfoFile( qcStatement* aStatement );
    static IDE_RC executeRemoveObsoleteBackupFile( qcStatement* aStatement );
    static IDE_RC executeChangeIncrementanBackupDir( qcStatement* aStatement );
    static IDE_RC executeMoveBackupFile( qcStatement* aStatement );
    static IDE_RC executeChangeBackupDirectory( qcStatement* aStatement );

    /* PROJ-2626 Snapshot Export */
    static IDE_RC executeAlterDatabaseSnapshot( qcStatement * aStatement );

    static IDE_RC switchLogFile( qcStatement * aStatement );

    static IDE_RC flushBufferPool( qcStatement * aStatement );

    static IDE_RC flusherOnOff( qcStatement * aStatement,
                                UInt          aFlusherID,
                                idBool        aStart );
    static IDE_RC compactPlanCache( qcStatement * aStatement );
    static IDE_RC resetPlanCache( qcStatement * aStatement );

    static IDE_RC rebuildMinViewSCN( qcStatement *aStatement );

    static IDE_RC startSecurity( qcStatement * aStatement );
    static IDE_RC stopSecurity( qcStatement * aStatement );

    static IDE_RC startAudit( qcStatement * aStatement );
    static IDE_RC stopAudit( qcStatement * aStatement );
    static IDE_RC reloadAudit( qcStatement * aStatement );
    static IDE_RC auditOption( qcStatement * aStatement );
    static IDE_RC noAuditOption( qcStatement * aStatement );
    static IDE_RC delAuditOption( qcStatement * aStatement );
    
    // BUG-43533 OPTIMIZER_FEATURE_ENABLE
    static IDE_RC changeFeatureProperty4Startup( SChar * aNewValue );

    /* PROJ-2624 [扁瓷己] MM - 蜡楷茄 access_list 包府规过 力傍 */
    static IDE_RC reloadAccessList( qcStatement * aStatement );

private:
    static IDE_RC checkPrivileges(qcStatement * aStatement);

    static IDE_RC changeFeatureProperty( qcStatement * aStatement,
                                         SChar       * aNewValue,
                                         void        * aArg );

    static IDE_RC getFeatureVersionNo( SChar             * aVersionStr,
                                       qdcFeatureVersion * aVersionNo );

};

#endif // _O_QDC_H_
