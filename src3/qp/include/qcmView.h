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
 * $Id: qcmView.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCM_VIEW_H_
#define _O_QCM_VIEW_H_ 1

#include <qc.h>
#include <qcm.h>
#include <qtc.h>
#include <qsParseTree.h>
#include <qdParseTree.h>

class qcmView
{
public:
    // select status from SYS_VIEWS_
    static IDE_RC getStatusOfViews( 
        smiStatement        * aSmiStmt,
        UInt                  aViewID,
        qcmViewStatusType   * aStatus );

    /* BUG-36350 Updatable Join DML WITH READ ONLY    
     * select READ_ONLY from SYS_VIEWS_ */
    static IDE_RC getReadOnlyOfViews(
        smiStatement        * aSmiStmt,
        UInt                  aViewID,
        qcmViewReadOnly     * aReadOnly );
    
    // update SYS_VIEW_RELATED_ set STATUS = invalid
    static IDE_RC setInvalidViewOfRelated(
        qcStatement    * aStatement,
        UInt             aRelatedUserID,
        SChar          * aRelatedObjectName,
        UInt             aRelatedObjectNameSize,
        qsObjectType     aRelatedObjectType);

    // BUG-5939
    // recompile and update valid
    static IDE_RC recompileAndSetValidViewOfRelated(
        qcStatement    * aStatement,
        UInt             aRelatedUserID,
        SChar          * aRelatedObjectName,
        UInt             aRelatedObjectNameSize,
        qsObjectType     aRelatedObjectType);

    static IDE_RC checkCircularView(
        qcStatement       * aStatement,
        UInt                aUserID,
        SChar             * aViewName,
        UInt                aRelatedViewID,
        idBool            * aCircularViewExist );

    static IDE_RC checkCircularViewByName(
        qcStatement       * aStatement,
        UInt                aUserID,
        SChar             * aViewName,
        UInt                aRelatedUserID,
        SChar             * aRelatedObjName,
        idBool            * aCircularViewExist );

private:
    static IDE_RC updateStatusOfView(
        qcStatement       * aStatement,
        UInt                aViewID,
        qcmViewStatusType   aStatus );

    static IDE_RC findTableInfoListViewOfRelated(
        qcStatement       * aStatement,
        UInt                aRelatedUserID,
        SChar             * aRelatedObjectName,
        UInt                aRelatedObjectNameSize,
        qsObjectType        aRelatedObjectType,
        qcmTableInfoList ** aTableInfoList);
    
    static IDE_RC recompileView(
        qcStatement       * aStatement,
        UInt                aTableID );
};
#endif /* _O_QCM_VIEW_H_ */
