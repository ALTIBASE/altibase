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
 * $Id$
 **********************************************************************/

#ifndef _O_QDM_H_
#define _O_QDM_H_ 1

#include <smiDef.h>
#include <qc.h>
#include <qdParseTree.h>
#include <qmsParseTree.h>

#define QDM_MVIEW_VIEW_SUFFIX       ((SChar *)"$VIEW")
#define QDM_MVIEW_VIEW_SUFFIX_PROC  "$VIEW"
#define QDM_MVIEW_VIEW_SUFFIX_SIZE  (5)

class qdm
{
public:
    /* Validate */
    static IDE_RC validateCreate( qcStatement * aStatement );

    static IDE_RC validateAlterRefresh( qcStatement * aStatement );

    /* Execute */
    static IDE_RC executeCreate( qcStatement * aStatement );

    static IDE_RC executeAlterRefresh( qcStatement * aStatement );

private:
    static IDE_RC existMViewByName(
                    smiStatement * aSmiStmt,
                    UInt           aUserID,
                    SChar        * aMViewName,
                    UInt           aMViewNameSize,
                    idBool       * aExist );

    static IDE_RC insertMViewSpecIntoMeta(
                    qcStatement * aStatement,
                    UInt          aMViewID,
                    UInt          aTableID,
                    UInt          aViewID );

    static IDE_RC insertIntoMViewsMeta(
                    qcStatement        * aStatement,
                    UInt                 aUserID,
                    UInt                 aMViewID,
                    SChar              * aMViewName,
                    UInt                 aTableID,
                    UInt                 aViewID,
                    qdMViewRefreshType   aRefreshType,
                    qdMViewRefreshTime   aRefreshTime );

    static IDE_RC updateRefreshTypeFromMeta(
                    qcStatement        * aStatement,
                    UInt                 aMViewID,
                    qdMViewRefreshType   aRefreshType );

    static IDE_RC updateRefreshTimeFromMeta(
                    qcStatement        * aStatement,
                    UInt                 aMViewID,
                    qdMViewRefreshTime   aRefreshTime );

    static IDE_RC updateLastDDLTimeFromMeta(
                    qcStatement        * aStatement,
                    UInt                 aMViewID );

    static IDE_RC getNextMViewID(
                    qcStatement * aStatement,
                    UInt        * aMViewID );

    static IDE_RC searchMViewID(
                    smiStatement * aSmiStmt,
                    SInt           aMViewID,
                    idBool       * aExist );
};

#endif // _O_QDM_H_
