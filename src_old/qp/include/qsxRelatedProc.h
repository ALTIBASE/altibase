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
 * $Id: qsxRelatedProc.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QSX_RELATED_H_
#define _O_QSX_RELATED_H_ 1

#include <qc.h>

typedef struct qsxProcPlanList
{
    /* procedure, function, package spec, table 등의 정보 */
    qsOID              objectID;
    UInt               modifyCount;
    /* BUG-38844 object type */
    SInt               objectType;
    smSCN              objectSCN; /* PROJ-2268 */
    /* BUG-38720 package body 정보 */
    qsOID              pkgBodyOID;
    UInt               pkgBodyModifyCount;
    smSCN              pkgBodySCN; /* PROJ-2268 */
    void             * planTree;
    qsxProcPlanList  * next;
} qsxProcPlanList;

class qsxRelatedProc 
{
private :
    static IDE_RC doPrepareRelatedPlanTree (
                       qcStatement         * aQcStmt,
                       qsRelatedObjects    * aObjectIdList,
                       qsxProcPlanList    ** aAllObjectPlanListHead ); /* out */ 
 
    static IDE_RC preparePlanTree (
                       qcStatement        * aQcStmt,
                       qsxProcPlanList    * aObjectPlan,        /* member out */
                       qsRelatedObjects  ** aRelatedObjects );         /* out */

    static IDE_RC preparePkgPlanTree (
                       qcStatement        * aQcStmt,
                       SInt                 aObjectType,
                       qsxProcPlanList    * aObjectPlan,        /* member out */
                       qsRelatedObjects  ** aRelatedObjects );         /* out */
public :
    static IDE_RC latchObjects( qsxProcPlanList * aObjectPlanList );

    static IDE_RC checkObjects( qsxProcPlanList * aObjectPlanList );

    static IDE_RC unlatchObjects( qsxProcPlanList * aObjectPlanList );

    static IDE_RC findPlanTree(
                      qcStatement        * aQcStmt,
                      qsOID                aObjectID,
                      qsxProcPlanList   ** aObjectPlan );       /* out */

    static IDE_RC prepareRelatedPlanTree (
                      qcStatement         * aQcStmt,
                      qsOID                 aObjectID,
                      SInt                  objType,
                      qsxProcPlanList    ** aObjectPlanList );  /* out */
};

#endif /* _O_QSX_RELATED_H_ */
