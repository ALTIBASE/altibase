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
 * $Id: qcmJob.h 82251 2018-02-13 00:15:24Z donovan.seo $
 *
 * PROJ-1438 Job Scheduler
 **********************************************************************/

#ifndef _O_QCM_JOB_H_
#define _O_QCM_JOB_H_ 1

#include <qci.h>
#include <qcm.h>

typedef enum qcmJobState
{
    QCM_JOB_STATE_NONE = 0,
    QCM_JOB_STATE_EXEC
} qcmJobState;

class qcmJob
{
public:

    static IDE_RC isReadyJob( mtdDateType * aStart,
                              mtdDateType * aCurrent,
                              mtdDateType * aLastExec,
                              SInt          aInterval,
                              UChar       * aIntervalType,
                              idBool      * aResult );

    static IDE_RC getExecuteJobItems( smiStatement  * aSmiStmt,
                                      SInt          * aItems,
                                      UInt          * aCount,
                                      UInt            aMaxCount );

    static IDE_RC getJobInfo( smiStatement * aSmiStmt,
                              SInt           aJobID,
                              SChar        * aJobName,
                              SChar        * aExecQuery,
                              UInt         * aExecQueryLen,
                              SInt         * aExecCount,
                              qcmJobState  * aState );

    static IDE_RC updateStateAndLastExecTime( smiStatement * aSmiStmt,
                                              SInt           aJobID );

    static IDE_RC updateExecuteResult( smiStatement * aSmiStmt,
                                       SInt           aJobID,
                                       SInt           aExecCount,
                                       UInt           aErrorCode );

    static IDE_RC isExistJob( smiStatement   * aSmiStmt,
                              qcNamePosition * aJobName,
                              idBool         * aIsExist );

    static IDE_RC updateInitialJobState( smiStatement * aSmiStmt );
};

#endif /* _O_QCM_JOB_H_ */
