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
 * $Id: qcmPkg.h 84052 2018-09-20 09:39:56Z khkwak $
 **********************************************************************/

#ifndef _O_QCM_PKG_H_
#define _O_QCM_PKG_H_ 1

#include    <qc.h>
#include    <qcm.h>
#include    <qtc.h>
#include    <qsParseTree.h>

/* BUG-45306 PSM AUTHID */
enum qcmPkgAuthidType 
{
    QCM_PKG_DEFINER      = 0,
    QCM_PKG_CURRENT_USER = 1
};

enum qcmPkgStatusType 
{
    QCM_PKG_VALID   = 0,
    QCM_PKG_INVALID = 1
};

typedef struct qcmPkgs 
{
    UInt         userID;
    qsOID        pkgOID;
    SChar        pkgName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SInt         pkgType;   /* BUG-39340 */
} qcmPkgs;

#define QCM_MAX_PKG_LEN (100)
#define QCM_MAX_PKG_LEN_STR "100"

class qcmPkg
{
private :
    /*************************************
             SetMember functions
    **************************************/
    static IDE_RC pkgSetMember(
        idvSQL        * aStatistics,
        const void    * aRow,
        qcmPkgs       * aPkgs );
    
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
                           qsPkgParseTree   * aPkgParse );
    
    static IDE_RC remove ( qcStatement      * aStatement,
                           qsOID              aPkgOID,
                           UInt               aUserID,
                           SChar            * aPkgName,
                           UInt               aPkgNamesize,
                           qsObjectType       aPkgType,
                           idBool             aPreservePrivInfo );

    static IDE_RC remove ( qcStatement      * aStatement,
                           qsOID              aPkgOID,
                           qcNamePosition     aUserNamePos,
                           qcNamePosition     aPkgNamePos,
                           qsObjectType       aPkgType,
                           idBool             aPreservePrivInfo );
    
    /*************************************
             qcmPkgs 
    **************************************/
    static IDE_RC pkgInsert( qcStatement      * aStatement,
                             qsPkgParseTree  * aPkgParse );

    static IDE_RC getPkgExistWithEmptyByName(
                  qcStatement     * aStatement,
                  UInt              aUserID,
                  qcNamePosition    aPkgName,
                  qsObjectType      aObjType,
                  qsOID           * aPkgOID );
    
     static IDE_RC getPkgExistWithEmptyByNamePtr(
         qcStatement     * aStatement,
         UInt              aUserID,
         SChar           * aPkgName,
         SInt              aPkgNameSize,
         qsObjectType      aObjType,
         qsOID           * aPkgOID );
    
    static IDE_RC pkgUpdateStatus(
                  qcStatement         * aStatement,
                  qsOID                 aPkgOID,
                  qcmPkgStatusType      aStatus);
   
    // 별도의 트랜잭션을 사용해서 프로시저의 상태를 바꾼다. 
    static IDE_RC pkgUpdateStatusTx(
                  qcStatement         * aStatement,
                  qsOID                 aPkgOID,
                  qcmPkgStatusType      aStatus);
    
    static IDE_RC pkgRemove( qcStatement     * aStatement,
                             qsOID             aPkgOID );

    static IDE_RC pkgSelectCountWithUserId (
                  qcStatement          * aStatement,
                  UInt                   aUserId,
                  vSLong               * aRowCount );

    static IDE_RC pkgSelectAllWithUserId (
                  qcStatement          * aStatement,
                  UInt                   aUserId,
                  vSLong                 aMaxPkgCount,
                  vSLong               * aSelectedPkgCount,
                  qcmPkgs              * aPkgArray );

    static IDE_RC pkgSelectCountWithObjectType (
        smiStatement         * aSmiStmt,
        UInt                   aObjType,
        vSLong               * aRowCount );

    static IDE_RC pkgSelectAllWithObjectType (
        smiStatement         * aSmiStmt,
        UInt                   aObjType,
        vSLong                 aMaxPkgCount,
        vSLong               * aSelectedPkgCount,
        qcmPkgs              * aPkgArray );

    // BUG-19389
    static IDE_RC getPkgUserID ( 
                  qcStatement * aStatement,
                  qsOID         aPkgOID,
                  UInt        * aPkgUserID );
        
    /*************************************
             qcmPkgParas 
    **************************************/
    static IDE_RC paraInsert(
                  qcStatement     * aStatement,
                  qsPkgParseTree  * aPkgParse );

    static IDE_RC paraRemoveAll(
                  qcStatement     * aStatement,
                  qsOID             aPkgOID);

    /*************************************
             qcmPkgParse 
    **************************************/
    static IDE_RC prsInsert(
                  qcStatement     * aStatement,
                  qsPkgParseTree  * aPkgParse );
    
    static IDE_RC prsInsertFragment(
                  qcStatement     * aStatement,
                  SChar           * aStmtBuffer,
                  qsPkgParseTree  * aPkgParse,
                  UInt              aSeqNo,
                  UInt              aOffset,
                  UInt              aLength );

    static IDE_RC prsRemoveAll(
                  qcStatement     * aStatement,
                  qsOID             aPkgOID);

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
             qcmPkgRelated 
    **************************************/

    static IDE_RC relInsert(
                  qcStatement      * aStatement,
                  qsPkgParseTree   * aPkgParse,
                  qsRelatedObjects * aRelatedObjList );

    static IDE_RC relSetInvalidPkgOfRelated(
                  qcStatement    * aStatement,
                  UInt             aRelatedUserID,
                  SChar          * aRelatedObjectName,
                  UInt             aRelatedObjectNameSize,
                  qsObjectType     aRelatedObjectType,
                  idBool           aIsUseTx = ID_FALSE ); // BUG-46416

    /* BUG-39340
       alter package ~ compile specification하면,
       package body만 invalid된다. */
    static IDE_RC relSetInvalidPkgBody(
                  qcStatement    * aStatement,
                  UInt             aPkgBodyUserID,
                  SChar          * aPkgBodyObjectName,
                  UInt             aPkgBodyObjectNameSize,
                  qsObjectType     aPkgBodyObjectType);
    
    static IDE_RC relRemoveAll(
                  qcStatement     * aStatement,
                  qsOID             aPkgOID);

    // BUG-36975
    static IDE_RC makeRelatedObjectNodeForInsertMeta(
                  qcStatement       * aStatement,
                  UInt                aUserID,
                  qsOID               aObjectID,
                  qcNamePosition      aObjectName,
                  SInt                aObjectType,
                  qsRelatedObjects ** aRelObj );
    
    /*************************************
             Fixed Table  
    **************************************/
    
    static IDE_RC pkgSelectAllForFixedTable (
                  smiStatement         * aSmiStmt,
                  vSLong                 aMaxPkgCount,
                  vSLong               * aSelectedPkgCount,
                  void                 * aFixedTableInfo );
    
    static IDE_RC buildPkgText( idvSQL        * aStatistics,
                                const void    * aRow,
                                void          * aFixedTableInfo );

    static IDE_RC buildRecordForPkgTEXT ( idvSQL              * aStatistics,
                                          void                * aHeader,
                                          void                * aDumpObj,
                                          iduFixedTableMemory * aMemory);

};

#endif /* _O_QCM_PKG_H_ */

