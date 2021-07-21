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
 * $Id: qsc.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QSC_H_
#define  _O_QSC_H_  1

#include <iduMemory.h>
#include <iduLatch.h>
#include <qc.h>
#include <qcg.h>
#include <qsxEnv.h>
#include <qsvEnv.h>
#include <qcpManager.h>
#include <qtc.h>
#include <qsParseTree.h>
#include <qsxTemplateCache.h>
#include <qmcThr.h>

struct qscExecInfo;
struct qscConcMgr;
struct qscErrors;

// member of qcSessionSpecific
typedef struct qscConcMgr
{
    iduVarMemList * memory;
    qscErrors     * errArr;
    UInt            errMaxCnt;
    UInt            errCnt;
    iduMutex        mutex;
} qscConcMgr;

// member of qscConcMgr
// Stores an error from thread executed.
typedef struct qscErrors
{
    UInt    reqID;
    UInt    errCode;
    SChar * execStr;
    SChar * errMsg;
} qscErrors;

// privateArgs of qmcThrObj
typedef struct qscExecInfo
{
    SChar      * execStr;
    void       * mmSession;
    idvSQL     * statistics;
    UInt         reqID;
    qscConcMgr * concMgr;
} qscExecInfo;

class qsc
{
public:
    // Called by qci::initializeSession
    static IDE_RC initialize( qmcThrMgr  ** aThrMgr,
                              qscConcMgr ** aConcMgr );

    // Called by qci::finalizeSession
    static IDE_RC finalize( qmcThrMgr  ** aThrMgr,
                            qscConcMgr ** aConcMgr );

    // Called by DBMS_CONCURRENT_EXEC.INITIALIZE
    static IDE_RC prepare( qmcThrMgr  * aThrMgr,
                           qscConcMgr * aConcMgr,
                           UInt         aThrCnt );

    // Called by DBMS_CONCURRENT_EXEC.REQUEST -> function for thread execution
    // Execute a procedure and handle an error.
    static IDE_RC execute( qmcThrObj * aThrArg );

private:
    // Called by qsc::execute
    // Setting an error to qscConcMgr->errArr
    static void setError( qmcThrObj * aThrArg );
};

#endif /* _O_QSC_H_ */
