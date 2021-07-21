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
 * $Id: qsvEnv.cpp 86306 2019-10-24 07:17:35Z donovan.seo $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qs.h>
#include <qsvEnv.h>
#include <qcuSqlSourceInfo.h>
#include <qsxProc.h>

IDE_RC qsvEnv::allocEnv( qcStatement * aStatement )
{
    qsvEnvInfo  * sEnv;

    IDE_TEST(STRUCT_ALLOC(aStatement->qmeMem, qsvEnvInfo, &sEnv)
             != IDE_SUCCESS);
    
    qsvEnv::clearEnv( sEnv );

    aStatement->spvEnv = sEnv;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qsvEnv::clearEnv(qsvEnvInfo * aEnv)
{
    aEnv->allVariables      = NULL;
    aEnv->loopCount         = 0;
    aEnv->exceptionCount    = 0;
    aEnv->nextId            = QS_ID_INIT_VALUE;
    aEnv->currStmt          = NULL;
    aEnv->flag              = 0;
    aEnv->relatedObjects    = NULL;
    aEnv->createProc        = NULL;
    aEnv->objectSynonymList = NULL;
    aEnv->procPlanList      = NULL;
    aEnv->latched           = ID_TRUE;   // procPlanList를 생성하면서
                                         // ID_TRUE가 되어야 하지만
                                         // procPlanList는 NULL이므로
                                         // 생성시 ID_TRUE가 되어도 무방하다.
    aEnv->modifiedTableList = NULL;
    aEnv->currDeclItem      = NULL;
  
    //fix PROJ-1596 
    aEnv->sql = NULL;
    aEnv->sqlSize = 0;

    // fix BUG-32837
    aEnv->allParaDecls = NULL;
   
    // PROJ-1073 Package
    aEnv->createPkg         = NULL;
    aEnv->pkgPlanTree       = NULL;
    aEnv->currSubprogram    = NULL;

    // BUGBUG BUG-36203
    aEnv->mStmtList         = NULL;
    aEnv->mStmtList2        = NULL;

    /* BUG-39004
       package intialize section에 대해서
       validation 진행 여부 표시 */
    aEnv->isPkgInitializeSection = ID_FALSE;
}

SInt qsvEnv::getNextId (qsvEnvInfo * aEnvInfo)
{
    (aEnvInfo->nextId)--;   // nextId is negative value

    return (aEnvInfo->nextId);
}

