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
 * $Id: qsxProc.h 90009 2021-02-17 06:54:43Z khkwak $
 **********************************************************************/

#ifndef _Q_QSX_PROC_H_
#define _Q_QSX_PROC_H_ 1

#include <iduMemory.h>
#include <iduLatch.h>
#include <qc.h>
#include <qsParseTree.h>
#include <qsxTemplateCache.h>

#define RETRY_SLEEP_USEC          (200) // less than 100 usec has no means
                                        // because wait time can be always
                                        // graterthan 100 usec  10->200
#define MAX_RETRY_COUNT           (1000)

typedef struct qsxProcInfo
{
    qsOID              procOID;
    UInt               modifyCount;

    idBool             isValid;
    UInt               sessionID; // BUG-36291 invalid로 변경한 SessionID

    sdiSplitMethod     shardSplitMethod; // TASK-7244 Set shard split method to PSM info 

    iduVarMemList    * qmsMem;
    qsProcParseTree  * planTree;
    qsRelatedObjects * relatedObjects;
    qcTemplate       * tmplate;
    UInt               privilegeCount;
    UInt             * granteeID; // arrary: granteeID[privilegeCount]

    qsxTemplateCache * cache;     // BUG-36203 PSM Optimize
} qsxProcInfo;

typedef struct qsxProcObjectInfo
{
    iduLatch           latch;
    iduLatch           latchForStatus;
    idBool             isAvailable;
    qsxProcInfo      * procInfo;
} qsxProcObjectInfo;

class qsxProc
{
public :
    static IDE_RC createProcObjectInfo( qsOID                aProcOID,
                                        qsxProcObjectInfo ** aProcObjectInfo );
    
    static IDE_RC destroyProcObjectInfo( qsxProcObjectInfo ** aProcObjectInfo );
    
    static IDE_RC disableProcObjectInfo( qsOID          aProcOID );
    
    static IDE_RC getProcObjectInfo( qsOID                aProcOID,
                                     qsxProcObjectInfo ** aProcObjectInfo );
    
    static IDE_RC setProcObjectInfo( qsOID               aProcOID,
                                     qsxProcObjectInfo * aProcObjectInfo );
    
    static IDE_RC createProcInfo (qsOID          aProcOID,
                                  qsxProcInfo ** aProcInfo);

    static IDE_RC destroyProcInfo(qsxProcInfo ** aProcInfo);

    static IDE_RC getProcInfo( qsOID          aProcOID,
                               qsxProcInfo ** aProcInfo );
    
    static IDE_RC setProcInfo( qsOID          aProcOID,
                               qsxProcInfo  * aProcInfo );

    static IDE_RC setProcText(smiStatement * aSmiStmt,
                              qsOID          aProcOID,
                              SChar        * aProcText,
                              SInt           aProcTextLen);
    
    static IDE_RC getProcText(qcStatement * aStatement,
                              qsOID         aProcOID,
                              SChar      ** aProcText,
                              SInt        * aProcTextLen);
    
    static IDE_RC latchS( qsOID          aProcOID );
   
    static IDE_RC latchX( qsOID          aProcOID,
                          idBool         aTryLock );

    // fix BUG-18854
    static IDE_RC latchXForRecompile( qsOID          aProcOID );
    
    static IDE_RC unlatch( qsOID          aProcOID );

    static IDE_RC makeStatusValid( qcStatement * aStatement,
                                   qsOID         aProcOID );

    static IDE_RC makeStatusValidTx( qcStatement * aStatement,
                                     qsOID         aProcOID );

    static IDE_RC makeStatusInvalid( qcStatement * aStatement,
                                     qsOID         aProcOID );

    // TASK-7244 Set shard split method to PSM info
    static IDE_RC makeStatusInvalidAndSetShardSplitMethod( qcStatement * aStatement,
                                                           qsOID         aProcOID,
                                                           SChar       * aShardSplitMethodStr );

    static IDE_RC makeStatusInvalidTx( qcStatement * aStatement,
                                       qsOID         aProcOID );

    static IDE_RC makeStatusInvalidForRecompile( qcStatement * aStatement,
                                                 qsOID         aProcOID );

    static IDE_RC latchXForStatus( qsOID          aProcOID );

    static IDE_RC unlatchForStatus( qsOID          aProcOID );

    static IDE_RC createProcObjAndInfoCallback( smiStatement * aSmiStmt,
                                                qsOID          aProcOID );

private:
    static IDE_RC makeStatusInvalidInternal( qcStatement * aStatement,
                                             qsOID         aProcOID,
                                             SChar       * aShardSplitMethodStr );
};

#endif /* _Q_QSX_PROC_H_ */

