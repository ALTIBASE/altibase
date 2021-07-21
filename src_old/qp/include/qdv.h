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
 * $Id: qdv.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QDV_H_
#define  _O_QDV_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qsParseTree.h>
#include <qdParseTree.h>

class qdv
{
public:
    // parse
    static IDE_RC parseCreateViewAsSelect( qcStatement * aStatement );

    // validate.
    static IDE_RC validateCreate( qcStatement * aStatement );

    static IDE_RC validateAlter( qcStatement * aStatement );

    // execute
    static IDE_RC executeCreate( qcStatement * aStatement );

    static IDE_RC executeRecreate( qcStatement * aStatement );

    static IDE_RC executeAlter( qcStatement * aStatement );

    static IDE_RC makeParseTreeForViewInSelect( 
        qcStatement * aStatement,
        qmsTableRef * aTableRef );

    static IDE_RC insertViewSpecIntoMeta(
        qcStatement * aStatement,
        UInt          aViewID,
        smOID         aTableOID);

private:
    static IDE_RC insertIntoViewsMeta(
        qcStatement * aStatement,
        UInt          aUserID,
        UInt          aViewID,
        UInt          aStatus,
        UInt          aWithReadOnly );

    static IDE_RC insertIntoViewParseMeta(
        qcStatement   * aStatement,
        qcNamePosList * aNcharList,
        UInt            aUserID,
        UInt            aViewID);

    static IDE_RC insertIntoViewParseMetaOneRecord(
        qcStatement * aStatement,
        SChar       * aStmtBuffer,
        UInt          aUserID,
        UInt          aViewID,
        UInt          aSeqNo,
        UInt          aOffset,
        UInt          aLength );

    static IDE_RC insertIntoViewRelatedMeta(
        qcStatement         * aStatement,
        UInt                  aUserID,
        UInt                  aViewID,
        qsRelatedObjects    * aRelatedObjList );

    static IDE_RC makeParseTreeForAlter(
        qcStatement     * aStatement,
        qcmTableInfo    * aTableInfo );

    static IDE_RC makeOneIntegerQcmColumn(
        qcStatement     * aStatement,
        qcmColumn      ** aColumn );
};

#endif // _O_QDV_H_
 
