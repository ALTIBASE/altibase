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
 * $Id: qcmCreate.h 82166 2018-02-01 07:26:29Z ahra.cho $
 **********************************************************************/

#ifndef _O_QCM_CREATE_H_
#define _O_QCM_CREATE_H_ 1

#include    <qc.h>
#include    <qcm.h>
#include    <qtc.h>
#include    <qdParseTree.h>

class qcmCreate
{
public:
    static IDE_RC createDB( idvSQL * aStatistics,
                            SChar  * aDBName, 
                            SChar  * aOwnerDN, 
                            UInt     aOwnerDNLen);

    static IDE_RC upgradeMeta( idvSQL * aStatistics,
                               SInt     aMinorVer );

    // PROJ-2689 downgrade meta
    static IDE_RC downgradeMeta( idvSQL * aStatistics,
                                 SInt     aMinorVer );

private:
    static IDE_RC createQcmTables(
        idvSQL          * aStatistics, 
        smiStatement    * aSmiStmt,
        mtcColumn       * aTblColumn);
    static IDE_RC createQcmColumns(
        idvSQL          * aStatistics, 
        smiStatement    * aSmiStmt,
        mtcColumn       * aTblColumn,
        mtcColumn       * aColColumn);
    static IDE_RC createTableIDSequence(
        smiStatement    * aStatement,
        mtcColumn       * aColumn);
    static IDE_RC createTableInfoForCreateDB(
        qcmTableInfo  * aTableInfo,
        mtcColumn     * aMtcColumn,
        UInt            aColCount,
        UInt            aTableID,
        const void    * aTableHandle,
        SChar         * aTableName,
        SChar         * aColumnNames[]);
  
    static IDE_RC insertIntoQcmTables(
        smiStatement    * aSmiStmt,
        mtcColumn       * aColumn );
    static IDE_RC insertIntoQcmColumns(
        smiStatement    * aSmiStmt,
        mtcColumn       * aTblColumn,
        mtcColumn       * aColColumn);

    static IDE_RC makeSignature(smiStatement * aStatement);

    static IDE_RC runDDLforMETA( idvSQL   * aStatistics,
                                 smiTrans * aTrans, 
                                 SChar    * aDBName, 
                                 SChar    * aOwnerDN, 
                                 UInt       aOwnerDNLen );

    static IDE_RC executeDDL(
        qcStatement * aStatement,
        SChar       * aText );

    // Meta Table의 SM Flag를 초기화 한다.
    static IDE_RC initMetaTableFlag( smiStatement     * aSmiStmt );

    // Meta Table의 Log Flush Flag를 끈다.
    static IDE_RC turnOffMetaTableLogFlushFlag( smiStatement * aSmiStmt );

    // BUG-45571 Upgrade meta for authid clause of PSM
    static IDE_RC createBuiltInPSM( idvSQL       * aStatistics,
                                    qcStatement  * aStatement,
                                    smiStatement * aSmiStmt,
                                    smiStatement * aDummySmiStmt );
};

#endif /* _O_QCM_CREATE_H_ */
