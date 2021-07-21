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
 * $Id: qcmProc.h 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#ifndef _O_QCM_PROC_H_
#define _O_QCM_PROC_H_ 1

#include    <qc.h>
#include    <qcm.h>
#include    <qtc.h>
#include    <qsParseTree.h>

/* BUG-45306 PSM AUTHID */
enum qcmProcAuthidType 
{
    QCM_PROC_DEFINER      = 0,
    QCM_PROC_CURRENT_USER = 1
};

enum qcmProcStatusType 
{
    QCM_PROC_VALID = 0,
    QCM_PROC_INVALID = 1
};

// for procedures and functions
typedef struct qcmProcedures 
{
    UInt         userID;
    qsOID        procOID;
    SChar        procName[ QC_MAX_OBJECT_NAME_LEN + 1 ] ;
} qcmProcedures;

#define QCM_MAX_PROC_LEN (100)
#define QCM_MAX_PROC_LEN_STR "100"

class qcmProc
{
private :
    /*************************************
             SetMember functions
    **************************************/
    static IDE_RC procSetMember(
        idvSQL        * aStatistics,
        const void    * aRow,
        qcmProcedures * aProcedures );
    
    static IDE_RC getTypeFieldValues (
                  qcStatement * aStatement,
                  qtcNode     * aTypeNode,
                  SChar       * aReturnTypeType,
                  SChar       * aReturnTypeLang,
                  SChar       * aReturnTypeSize ,
                  SChar       * aReturnTypePrecision,
                  SChar       * aReturnTypeScale,
                  UInt          aMaxBufferLength );

public:

    /*************************************
             main functions 
    **************************************/
    static IDE_RC insert ( qcStatement      * aStatement,
                           qsProcParseTree  * aProcParse );
    
    static IDE_RC remove ( qcStatement      * aStatement,
                           qsOID              aProcOID,
                           UInt               aUserID,
                           SChar            * aProcName,
                           UInt               aProcNamesize,
                           idBool             aPreservePrivInfo );

    static IDE_RC remove ( qcStatement      * aStatement,
                           qsOID              aProcOID,
                           qcNamePosition     aUserNamePos,
                           qcNamePosition     aProcNamePos,
                           idBool             aPreservePrivInfo );
    
    /*************************************
             qcmProcedures 
    **************************************/
    static IDE_RC procInsert( qcStatement      * aStatement,
                              qsProcParseTree  * aProcParse );

    static IDE_RC getProcExistWithEmptyByName(
                  qcStatement     * aStatement,
                  UInt              aUserID,
                  qcNamePosition    aProcName,
                  qsOID           * aProcOID );
    
     static IDE_RC getProcExistWithEmptyByNamePtr(
         qcStatement     * aStatement,
         UInt              aUserID,
         SChar           * aProcName,
         SInt              aProcNameSize,
         qsOID           * aProcOID );
    
    static IDE_RC procUpdateStatus(
                  qcStatement         * aStatement,
                  qsOID                 aProcOID,
                  qcmProcStatusType     aStatus);
   
    // 별도의 트랜잭션을 사용해서 프로시저의 상태를 바꾼다. 
    static IDE_RC procUpdateStatusTx(
                  qcStatement         * aStatement,
                  qsOID                 aProcOID,
                  qcmProcStatusType     aStatus);
    
    static IDE_RC procRemove( qcStatement     * aStatement,
                              qsOID             aProcOID );

    static IDE_RC procSelectCount (
                  smiStatement         * aSmiStmt,
                  vSLong               * aRowCount );

    static IDE_RC procSelectAll (
                  smiStatement         * aSmiStmt,
                  vSLong                 aMaxProcedureCount,
                  vSLong               * aSelectedProcedureCount,
                  qcmProcedures        * aProcedureArray );

    static IDE_RC procSelectCountWithUserId (
                  qcStatement          * aStatement,
                  UInt                   aUserId,
                  vSLong               * aRowCount );

    static IDE_RC procSelectAllWithUserId (
                  qcStatement          * aStatement,
                  UInt                   aUserId,
                  vSLong                 aMaxProcedureCount,
                  vSLong               * aSelectedProcedureCount,
                  qcmProcedures        * aProcedureArray );

    // PROJ-1075 TYPESET
    static IDE_RC getProcObjType(
                  qcStatement * aStatement,
                  qsOID         aProcOID,
                  SInt        * aProcObjType );

    // BUG-19389
    static IDE_RC getProcUserID ( 
                  qcStatement * aStatement,
                  qsOID         aProcOID,
                  UInt        * aProcUserID );
        
    /*************************************
             qcmProcParas 
    **************************************/
    static IDE_RC paraInsert(
                  qcStatement     * aStatement,
                  qsProcParseTree * aProcParse,
                  qsVariableItems * aParaDeclParse);

    static IDE_RC paraRemoveAll(
                  qcStatement     * aStatement,
                  qsOID             aProcOID);

    /*************************************
             qcmProcParse 
    **************************************/
    static IDE_RC prsInsert(
                  qcStatement     * aStatement,
                  qsProcParseTree * aProcParse );
    
    static IDE_RC prsInsertFragment(
                  qcStatement     * aStatement,
                  SChar           * aStmtBuffer,
                  qsProcParseTree * aProcParse,
                  UInt              aSeqNo,
                  UInt              aOffset,
                  UInt              aLength );

    static IDE_RC prsRemoveAll(
                  qcStatement     * aStatement,
                  qsOID             aProcOID);

    static IDE_RC prsCopyStrDupAppo (
                  SChar         * aDest,
                  SChar         * aSrc,
                  UInt            aLength );

    // PROJ-1579 NCHAR
    static IDE_RC convertToUTypeString(
                  qcStatement   * aStatement,
                  qcNamePosList * aNcharList,
                  SChar         * aDest,
                  UInt            aBufferSize );

    /*************************************
             qcmProcRelated 
    **************************************/

    static IDE_RC relInsert(
                  qcStatement      * aStatement,
                  qsProcParseTree  * aProcParse,
                  qsRelatedObjects * aRelatedObjList );

    static IDE_RC relSetInvalidProcOfRelated(
                  qcStatement    * aStatement,
                  UInt             aRelatedUserID,
                  SChar          * aRelatedObjectName,
                  UInt             aRelatedObjectNameSize,
                  qsObjectType     aRelatedObjectType,
                  idBool           aIsUseTx = ID_FALSE ); // BUG-46416
    
    static IDE_RC relRemoveAll(
                  qcStatement     * aStatement,
                  qsOID             aProcOID);
    
    /*************************************
             qcmConstraintRelated
    **************************************/

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relInsertRelatedToConstraint(
                  qcStatement             * aStatement,
                  qsConstraintRelatedProc * aRelatedProc );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relRemoveRelatedToConstraintByUserID(
                  qcStatement * aStatement,
                  UInt          aUserID );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relRemoveRelatedToConstraintByTableID(
                  qcStatement * aStatement,
                  UInt          aTableID );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relRemoveRelatedToConstraintByConstraintID(
                  qcStatement * aStatement,
                  UInt          aConstraintID );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relIsUsedProcByConstraint(
                  qcStatement    * aStatement,
                  qcNamePosition * aRelatedProcName,
                  UInt             aRelatedUserID,
                  idBool         * aIsUsed );

    /*************************************
             qcmIndexRelated
    **************************************/

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relInsertRelatedToIndex(
                  qcStatement        * aStatement,
                  qsIndexRelatedProc * aRelatedProc );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relRemoveRelatedToIndexByUserID(
                  qcStatement * aStatement,
                  UInt          aUserID );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relRemoveRelatedToIndexByTableID(
                  qcStatement * aStatement,
                  UInt          aTableID );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relRemoveRelatedToIndexByIndexID(
                  qcStatement * aStatement,
                  UInt          aIndexID );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    static IDE_RC relIsUsedProcByIndex(
                  qcStatement    * aStatement,
                  qcNamePosition * aRelatedProcName,
                  UInt             aRelatedUserID,
                  idBool         * aIsUsed );

    /*************************************
             Fixed Table  
    **************************************/
    
    static IDE_RC procSelectAllForFixedTable (
                  smiStatement         * aSmiStmt,
                  vSLong                 aMaxProcedureCount,
                  vSLong               * aSelectedProcedureCount,
                  void                 * aFixedTableInfo );
    
    static IDE_RC buildProcText(  idvSQL        * aStatistics,
                                  const void    * aRow,
                                  void          * aFixedTableInfo );
    
    static IDE_RC buildRecordForPROCTEXT( idvSQL      * aStatistics,
                                          void        * aHeader,
                                          void        * aDumpObj,
                                          iduFixedTableMemory   *aMemory);

    /*************************************
             X$PROCINFO
    **************************************/
    
    static IDE_RC procSelectAllForBuildProcInfo (
                  smiStatement         * aSmiStmt,
                  vSLong                 aMaxProcedureCount,
                  vSLong               * aSelectedProcedureCount,
                  void                 * aFixedTableInfo );
    
    static IDE_RC buildProcInfo(  idvSQL        * aStatistics,
                                  const void    * aRow,
                                  void          * aFixedTableInfo );
    
    static IDE_RC buildRecordForPROCInfo( idvSQL      * aStatistics,
                                          void        * aHeader,
                                          void        * aDumpObj,
                                          iduFixedTableMemory   *aMemory);

};

#endif /* _O_QCM_PROC_H_ */

 
