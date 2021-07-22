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
 *
 * Description : PROJ-2223 audit
 *
 **********************************************************************/

#ifndef _O_QDC_AUDIT_H_
#define _O_QDC_AUDIT_H_  1

#include <idl.h>
#include <ide.h>
#include <qc.h>
#include <qci.h>
#include <qcmTableInfo.h>
#include <qs.h>


#define QDC_AUDIT_MAX_PATH_LEN (1024)

//===========================================================
// Audit을 위한 Parse Tree
//===========================================================

// Example )
//     AUDIT INSERT,UPDATE ON userName.tableName
//     BY ACCESS
//     WHENEVER SUCCESSFUL;
//
//     AUDIT ALL ON userName.tableName
//     BY ACCESS
//     WHENEVER SUCCESSFUL;
//
//     AUDIT ALL ON userName
//     BY ACCESS
//     WHENEVER SUCCESSFUL;

typedef struct qdcAuditOperation
{
    UInt    operation[QCI_AUDIT_OPER_MAX];
    idBool  isAll;
} qdcAuditOperation;

typedef struct qdcAuditParseTree
{
    //---------------------------------------------
    // Parsing 정보
    //---------------------------------------------

    qcParseTree                common;

    // user 정보와 object 정보
    qcNamePosition             userName;
    qcNamePosition             objectName;

    // operation 정보
    qdcAuditOperation        * operations;
    
    //---------------------------------------------
    // Validation 정보
    //---------------------------------------------

    UInt                       userID;

} qdcAuditParseTree;

#define QDC_AUDIT_PARSE_TREE_INIT( _dst_ )     \
{                                              \
    SET_EMPTY_POSITION((_dst_)->userName);     \
    SET_EMPTY_POSITION((_dst_)->objectName);   \
}

/* BUG-39074 */
#define QDC_DELAUDIT_PARSE_TREE_INIT( _dst_ )  \
{                                              \
    SET_EMPTY_POSITION((_dst_)->userName);     \
    SET_EMPTY_POSITION((_dst_)->objectName);   \
}

//===========================================================
// NoAudit을 위한 Parse Tree
//===========================================================

// Example )
//     NOAUDIT UPDATE ON userName.tableName
//     WHENEVER SUCCESSFUL;

typedef struct qdcNoAuditParseTree
{
    //---------------------------------------------
    // Parsing 정보
    //---------------------------------------------

    qcParseTree                common;

    // object 정보
    qcNamePosition             userName;
    qcNamePosition             objectName;

    // operation 정보
    qdcAuditOperation        * operations;
    
    //---------------------------------------------
    // Validation 정보
    //---------------------------------------------

    UInt                       userID;

} qdcNoAuditParseTree;

/* BUG-39074 */
typedef struct qdcDelAuditParseTree
{
    //---------------------------------------------
    // Parsing 정보
    //---------------------------------------------

    qcParseTree                common;

    // object 정보
    qcNamePosition             userName;
    qcNamePosition             objectName;

    // operation 정보
    qdcAuditOperation        * operations;
    
    //---------------------------------------------
    // Validation 정보
    //---------------------------------------------

    UInt                       userID;

} qdcDelAuditParseTree;

#define QDC_NOAUDIT_PARSE_TREE_INIT( _dst_ )   \
{                                              \
    SET_EMPTY_POSITION((_dst_)->userName);     \
    SET_EMPTY_POSITION((_dst_)->objectName);   \
}

//=========================================================
// Audit을 위한
//=========================================================

class qdcAudit
{
public:
    //----------------------------------------------
    // Audit을 위한 함수
    //----------------------------------------------

    static IDE_RC auditOption( qcStatement * aStatement );

    //----------------------------------------------
    // NoAudit을 위한 함수
    //----------------------------------------------

    static IDE_RC noAuditOption( qcStatement * aStatement );

    //----------------------------------------------
    // DelAudit을 위한 함수 - BUG-39074
    //----------------------------------------------

    static IDE_RC delAuditOption( qcStatement * aStatement );
    
    //----------------------------------------------
    // Audit Control을 위한 함수
    //----------------------------------------------

    static IDE_RC start( qcStatement * aStatement );
    static IDE_RC stop( qcStatement * aStatement );
    static IDE_RC reload( qcStatement * aStatement );
    
    //----------------------------------------------
    // Audit Referenced Object를 위한 함수
    //----------------------------------------------
    
    static IDE_RC setAllRefObjects( qcStatement * aStatement );
    static void setOperation( qcStatement * aStatement );
    static void getOperation( qcStatement     * aStatement,
                              qciAuditOperIdx * aOperation );
    static void getOperationString( qciAuditOperIdx    aOperation,
                                    const SChar     ** aString );
    
private:

    //----------------------------------------------
    // Audit을 위한 함수
    //----------------------------------------------

};

#endif // _O_QDC_AUDIT_H_
