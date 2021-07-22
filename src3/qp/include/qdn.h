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
 * $Id: qdn.h 86367 2019-11-18 07:32:25Z donghyun1 $
 **********************************************************************/
#ifndef _O_QDN_H_
#define  _O_QDN_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

#define QDN_ON_CREATE_TABLE   0
#define QDN_ON_ADD_COLUMN     1
#define QDN_ON_ADD_CONSTRAINT 2
#define QDN_ON_MODIFY_COLUMN  3

#define QDN_NOT_MATCH_COLUMN        0
#define QDN_MATCH_COLUMN_WITH_ORDER 1

// qd coNstraint -- add constraint, drop constraint
class qdn
{
public:
    static IDE_RC validateAddConstr(
        qcStatement * aStatement);
    static IDE_RC validateDropConstr(
        qcStatement * aStatement);
    static IDE_RC validateDropUnique(
        qcStatement * aStatement);
    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC validateDropLocalUnique(
        qcStatement * aStatement);
    static IDE_RC validateDropPrimary(
        qcStatement * aStatement);

    static IDE_RC validateRenameConstr(
        qcStatement * aStatement);
    // PROJ-1874 FK NOVALIDATE
    static IDE_RC validateModifyConstr(
        qcStatement * aStatement);

    static IDE_RC validateConstraints(
        qcStatement      * aStatement,
        qcmTableInfo     * aTableInfo,
        UInt               aTableOwnerID,  
        scSpaceID          aTableTBSID,    
        smiTableSpaceType  aTableTBSType,
        qdConstraintSpec * aConstr,
        UInt               aConstraintOption,
        UInt             * aUniqueKeyCount );

    /* PROJ-2461 */
    static IDE_RC checkLocalIndex( qcStatement      * aStatement,
                                   qdConstraintSpec * aConstr,
                                   qcmTableInfo     * aTableInfo,
                                   idBool           * aIsLocalIndex,
                                   idBool             aIsDiskTBS );
    
    static IDE_RC executeAddConstr(
        qcStatement * aStatement);
    static IDE_RC executeDropConstr(
        qcStatement * aStatement);
    static IDE_RC executeDropUnique(
        qcStatement * aStatement);
    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC executeDropLocalUnique(
        qcStatement * aStatement);
    static IDE_RC executeDropPrimary(
        qcStatement * aStatement);

    static IDE_RC executeRenameConstr(
        qcStatement * aStatement);
    // PROJ-1874 FK NOVALIDATE
    static IDE_RC executeModifyConstr(
        qcStatement * aStatement);

    static IDE_RC insertConstraintIntoMeta(
        qcStatement *aStatement,
        UInt         aUserID,
        UInt         aTableID,
        UInt         aConstrID,
        SChar*       aConstrName,
        UInt         aConstrType,
        UInt         aIndexID,
        UInt         aColumnCnt,
        UInt         aReferencedTblID,
        UInt         aReferencedIndexID,
        UInt         aReferencedRule, // PROJ-1509
        SChar       *aCheckCondition, /* PROJ-1107 Check Constraint Áö¿ø */
        idBool       aValidated ); // PROJ-1874
    
    static IDE_RC insertConstraintColumnIntoMeta(
        qcStatement *aStatement,
        UInt         aUserID,
        UInt         aTableID,
        UInt         aConstrID,
        UInt         aConstColOrder,
        UInt         aColID);

    static IDE_RC copyConstraintRelatedMeta( qcStatement * aStatement,
                                             UInt          aToTableID,
                                             UInt          aToConstraintID,
                                             UInt          aFromConstraintID );

    static idBool matchColumnId( qcmColumn * aColumnList,
                                 UInt      * aColIdList,
                                 UInt        aKeyColCount,
                                 UInt      * aColFlagList );

    static idBool matchColumnIdOutOfOrder( qcmColumn * aColumnList,
                                           UInt      * aColIdList,
                                           UInt        aKeyColCount,
                                           UInt      * aColFlagList );

    static idBool intersectColumn( UInt * aColumnIDList1,
                                   UInt   aColumnIDCount1,
                                   UInt * aColumnIDList2,
                                   UInt   aColumnIDCount2 );

    static idBool intersectColumn( mtcColumn  * aColumnList1,
                                   UInt         aColumnCount1,
                                   UInt       * aColumnIDList2,
                                   UInt         aColumnIDCount2 );

    static IDE_RC getColumnFromDefinitionList( qcStatement       * aStatement,
                                               qcmColumn         * aColumnList,
                                               qcNamePosition      aColumnName,
                                               qcmColumn        ** aColumn );
    
    static idBool matchColumnList( qcmColumn   *aColList1,
                                   qcmColumn   *aColList2 );

    static idBool matchColumnListOutOfOrder( qcmColumn   *aColList1,
                                             qcmColumn   *aColList2 );

    static IDE_RC existSameConstrName( qcStatement * aStatment,
                                       SChar       * aConstrName,
                                       UInt          aTableOwnerID,
                                       idBool      * aExistSameConstrName );

private:

    static idBool existNotNullConstraint( qcmTableInfo * aTableInfo,
                                          UInt         * aCols,
                                          UInt           aColCount );
    
    static SInt matchColumnOffsetLengthOrder(qcmColumn *aColList1,
                                             qcmColumn *aColList2);
    
    static IDE_RC validateDuplicateConstraintSpec(
        qdConstraintSpec * aConstr,
        qcmTableInfo * aTableInfo,
        qdConstraintSpec * aAllConst);

    static IDE_RC checkOperatableForReplication( qcStatement     * aStatement,
                                                 qcmTableInfo    * aTableInfo,
                                                 UInt              aConstrType,
                                                 smOID           * aReplicatedTableOIDArray,
                                                 UInt              aReplicatedTableOIDCount );
    static UInt getConstraintType( qcmTableInfo       * aTableInfo,
                                   UInt                 aConstraintID );
};

#endif // _O_QDN_H_
