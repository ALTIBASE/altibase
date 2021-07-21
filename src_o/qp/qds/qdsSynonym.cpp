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
 

#include <qs.h>
#include <qcg.h>
#include <qcmSynonym.h>
#include <qcmUser.h>
#include <qcmView.h>
#include <qcmProc.h>
#include <qcuSqlSourceInfo.h>
#include <qdpPrivilege.h>
#include <qdsSynonym.h>
#include <qdParseTree.h>
#include <qmv.h>
#include <qcmPkg.h>
#include <qdpRole.h>

IDE_RC qdsSynonym::validateCreateSynonym(qcStatement *aStatement)
{
/***********************************************************************
 *
 * Description : Validate Create Synonym statement.
 *
 * Implementation :
 *
 ***********************************************************************/

    qdSynonymParseTree * sParseTree;
    qsOID                sObjectID;
    idBool               sObjectExist = ID_FALSE;
    idBool               sSynonymExist = ID_FALSE;
    qcuSqlSourceInfo     sqlInfo;
    qcmSynonymInfo       sSynonymInfo;

    sParseTree = (qdSynonymParseTree*) (aStatement->myPlan->parseTree);

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->synonymName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &(sParseTree->synonymName) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    // check existence of object
    IDE_TEST( qcm::existObject( aStatement,
                                sParseTree->isPublic,
                                sParseTree->synonymOwnerName,
                                sParseTree->synonymName,
                                QS_OBJECT_MAX,
                                &sParseTree->synonymOwnerID,
                                &sObjectID,
                                &sObjectExist)
              != IDE_SUCCESS );

    // BUG-38852
    if ( ( sParseTree->flag & QDS_SYN_OPT_REPLACE_MASK ) == QDS_SYN_OPT_REPLACE_FALSE )
    {
        // CREATE SYNONYM statement
        IDE_TEST_RAISE( sObjectExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );
    }
    else
    {
        // CREATE OR REPLACE SYNONYM statement
        if ( sObjectExist == ID_TRUE )
        {
            // check existence of synonym
            IDE_TEST( qcmSynonym::getSynonym(
                        aStatement,
                        sParseTree->synonymOwnerID,
                        (sParseTree->synonymName.stmtText) + (sParseTree->synonymName.offset),
                        sParseTree->synonymName.size,
                        &sSynonymInfo,
                        &sSynonymExist )
                      != IDE_SUCCESS );

            // SYNONYM 이 아닌 object
            IDE_TEST_RAISE( sSynonymExist == ID_FALSE, ERR_EXIST_OBJECT_NAME );
        }
        else
        {
            // Nothing to do.
        }
    }

    //check privilege
    if ( sParseTree->isPublic == ID_TRUE )
    {
        IDE_TEST( qdpRole::checkDDLCreatePublicSynonymPriv( aStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        // check semantics error
        // private synonym이름 중복 검사
        IDE_TEST( checkSemanticsError( aStatement ) != IDE_SUCCESS );
        
        IDE_TEST( qdpRole::checkDDLCreateSynonymPriv( aStatement,
                                                      sParseTree->synonymOwnerID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXIST_OBJECT_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC qdsSynonym::validateDropSynonym(qcStatement *aStatement)
{
/***********************************************************************
 *
 * Description : Validate drop Synonym statement.
 *
 * Implementation :
 *
 ***********************************************************************/

    qdSynonymParseTree * sParseTree;
    idBool               sSynonymExist;
    qcmSynonymInfo       sSynonymInfo;
    
    sParseTree = (qdSynonymParseTree*) aStatement->myPlan->parseTree;

    if( sParseTree->isPublic == ID_TRUE )
    {
        sParseTree->synonymOwnerID = QC_PUBLIC_USER_ID;
    }
    else
    {
        // username omitted => session user id
        if( QC_IS_NULL_NAME(sParseTree->synonymOwnerName) == ID_TRUE )
        {
            sParseTree->synonymOwnerID = QCG_GET_SESSION_USER_ID(aStatement);
        }
        // get user ID
        else
        {
            IDE_TEST(
                qcmUser::getUserID(
                    aStatement,
                    sParseTree->synonymOwnerName,
                    &sParseTree->synonymOwnerID)
                != IDE_SUCCESS);
        }
    }

    // check existence of synonym
    IDE_TEST(
        qcmSynonym::getSynonym(
            aStatement,
            sParseTree->synonymOwnerID,
            (sParseTree->synonymName.stmtText) + (sParseTree->synonymName.offset),
            sParseTree->synonymName.size,
            &sSynonymInfo,
            &sSynonymExist)
        != IDE_SUCCESS);

    IDE_TEST_RAISE(sSynonymExist == ID_FALSE, ERR_NOT_FOUND_SYNONYM);

    //check privilege
    if ( sParseTree->isPublic == ID_TRUE )
    {
        IDE_TEST( qdpRole::checkDDLDropPublicSynonymPriv( aStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qdpRole::checkDDLDropSynonymPriv( aStatement,
                                                    sParseTree->synonymOwnerID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_SYNONYM);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NOT_FOUND_SYNONYM));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC qdsSynonym::executeCreateSynonym(qcStatement *aStatement)
{
/***********************************************************************
 *
 * Description : Execute Create Synonym statement.
 *
 * Implementation :
 *     1. Insert synonym into system_.sys_synonyms_
 *
 ***********************************************************************/

    qdSynonymParseTree * sParseTree;
    SChar              * sSqlStr;
    vSLong               sRowCnt;
    SChar                sSynonymName[QC_MAX_OBJECT_NAME_LEN + 1];
    UChar                sObjectName [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                sObjectOwnerName[QC_MAX_OBJECT_NAME_LEN + 1];

    sParseTree = (qdSynonymParseTree *) (aStatement->myPlan->parseTree);

    IDU_FIT_POINT( "qdsSynonym::executeCreateSynonym::alloc::sSqlStr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    // copy synonym name
    QC_STR_COPY( sSynonymName, sParseTree->synonymName );

    // copy object owner name
    if (QC_IS_NULL_NAME(sParseTree->objectOwnerName) == ID_TRUE)
    {
        IDE_TEST(qcmUser::getUserName(aStatement,
                                      QCG_GET_SESSION_USER_ID(aStatement),
                                      sObjectOwnerName)
                 != IDE_SUCCESS);
    }
    else
    {
        QC_STR_COPY( sObjectOwnerName, sParseTree->objectOwnerName );
    }

    // copy object name
    QC_STR_COPY( sObjectName, sParseTree->objectName );

    if(QC_PUBLIC_USER_ID == sParseTree->synonymOwnerID)
    {
        // public synonym의 경우
        // user_id는 NULL이다.
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SYNONYMS_ VALUES( "
                         "NULL, "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "SYSDATE, SYSDATE )",
                         sSynonymName,
                         sObjectOwnerName,
                         sObjectName );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SYNONYMS_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "SYSDATE, SYSDATE )",
                         sParseTree->synonymOwnerID,
                         sSynonymName,
                         sObjectOwnerName,
                         sObjectName );
    }

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen(sSynonymName),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdsSynonym::executeDropSynonym(qcStatement *aStatement)
{
/***********************************************************************
 *
 * Description : Execute drop Synonym statement.
 *
 * Implementation :
 *     1. delete synonym from of system_.sys_synonyms_
 *
 ***********************************************************************/

    qdSynonymParseTree * sParseTree;

    SChar              * sSqlStr;
    SChar                sSynonymName[QC_MAX_OBJECT_NAME_LEN + 1];
    vSLong               sRowCnt;

    sParseTree = (qdSynonymParseTree*) (aStatement->myPlan->parseTree);

    IDU_FIT_POINT( "qdsSynonym::executeDropSynonym::alloc::sSqlStr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    // copy synonym name
    QC_STR_COPY( sSynonymName, sParseTree->synonymName );

    if(QC_PUBLIC_USER_ID == sParseTree->synonymOwnerID)
    {
        // public synonym의 경우
        // user_id는 NULL이다.
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_SYNONYMS_ "
                         "WHERE SYNONYM_OWNER_ID IS NULL "
                         "AND SYNONYM_NAME = VARCHAR'%s'",
                         sSynonymName );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_SYNONYMS_ "
                         "WHERE SYNONYM_OWNER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND SYNONYM_NAME = VARCHAR'%s'",
                         sParseTree->synonymOwnerID,
                         sSynonymName );
    }

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    IDE_TEST( qcmView::setInvalidViewOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen(sSynonymName),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen(sSynonymName),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    // PROJ-1073 Package
    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen(sSynonymName),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdsSynonym::checkSemanticsError(qcStatement *aStatement)
{
/***********************************************************************
 *
 * Description : private synonym 생성시 동일한 aliase를 참조하는지 검사
 *               private synonym은 동일한 schema내 객체이름과 같아서는 안됨.
 * Implementation :
 *
 ***********************************************************************/

    qdSynonymParseTree * sParseTree;
    SChar                sTempName[QC_MAX_OBJECT_NAME_LEN + 1];
    idBool               sSameSynonymObject = ID_FALSE;

    sParseTree = (qdSynonymParseTree*) (aStatement->myPlan->parseTree);

    // synonym 이름과 synonym 대상 객체의 이름이 다른 경우
    if ( QC_IS_NAME_CASELESS_MATCHED( sParseTree->synonymName, sParseTree->objectName ) == ID_FALSE )
    {
        return IDE_SUCCESS;
    }

    IDE_TEST( qcmUser::getUserName(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sTempName)
              != IDE_SUCCESS);
    
    if (QC_IS_NULL_NAME(sParseTree->synonymOwnerName) == ID_TRUE)
    {
        if (QC_IS_NULL_NAME(sParseTree->objectOwnerName)
            == ID_TRUE)
        {
            // ERROR
            // create synonym s1 for s1;
            sSameSynonymObject = ID_TRUE;
        }
        else
        {
            if ( QC_IS_STR_CASELESS_MATCHED( sParseTree->objectOwnerName, sTempName ) )
            {
                // ERROR
                // create synonym s1 for sys.s1;
                sSameSynonymObject = ID_TRUE;
            }
        }
    }
    else
    {
        if (QC_IS_NULL_NAME(sParseTree->objectOwnerName))
        {
            if ( QC_IS_STR_CASELESS_MATCHED( sParseTree->synonymOwnerName, sTempName ) )
            {
                // ERROR
                // create synonym sys.s1 for s1;
                sSameSynonymObject = ID_TRUE;
            }
        }
        else
        {
            if ( QC_IS_NAME_CASELESS_MATCHED( sParseTree->synonymOwnerName, sParseTree->objectOwnerName ) )
            {
                // ERROR
                // create synonym sys.s1 for sys.s1;
                sSameSynonymObject = ID_TRUE;
            }
        }
    }

    IDE_TEST_RAISE(sSameSynonymObject == ID_TRUE, ERR_SAME_OBJECT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SAME_OBJECT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_SAME_SYNONYM));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsSynonym::executeRecreate( qcStatement *aStatement )
{
/***********************************************************************
 *
 * Description : Execute CREATE OR REPLACE SYNONYM statement
 *
 * Implementation : 
 *
 *      1. delete meta info
 *      2. insert meta info
 *
 ***********************************************************************/

    qdSynonymParseTree * sParseTree;
    SChar              * sSqlStr;
    vSLong               sRowCnt;
    SChar                sSynonymName[QC_MAX_OBJECT_NAME_LEN + 1];
    UChar                sObjectName [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                sObjectOwnerName[QC_MAX_OBJECT_NAME_LEN + 1];

    sParseTree = (qdSynonymParseTree *) (aStatement->myPlan->parseTree);

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    // copy synonym name
    QC_STR_COPY( sSynonymName, sParseTree->synonymName );

    // copy object owner name
    if ( QC_IS_NULL_NAME( sParseTree->objectOwnerName ) == ID_TRUE )
    {
        IDE_TEST( qcmUser::getUserName( aStatement,
                                        QCG_GET_SESSION_USER_ID( aStatement ),
                                        sObjectOwnerName )
                  != IDE_SUCCESS );
    }
    else
    {
        QC_STR_COPY( sObjectOwnerName, sParseTree->objectOwnerName );
    }

    // copy object name
    QC_STR_COPY( sObjectName, sParseTree->objectName );

    //--------------------------------------------
    // delete
    //--------------------------------------------

    if( QC_PUBLIC_USER_ID == sParseTree->synonymOwnerID )
    {
        // public synonym의 경우
        // user_id는 NULL이다.
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_SYNONYMS_ "
                         "WHERE SYNONYM_OWNER_ID IS NULL "
                         "AND SYNONYM_NAME = VARCHAR'%s'",
                         sSynonymName );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_SYNONYMS_ "
                         "WHERE SYNONYM_OWNER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND SYNONYM_NAME = VARCHAR'%s'",
                         sParseTree->synonymOwnerID,
                         sSynonymName );
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt > 1, ERR_META_CRASH );

    IDE_TEST( qcmView::setInvalidViewOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen( sSynonymName ),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen( sSynonymName ),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    // PROJ-1073 Package
    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen( sSynonymName ),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    //--------------------------------------------
    // insert
    //--------------------------------------------

    if ( QC_PUBLIC_USER_ID == sParseTree->synonymOwnerID )
    {
        // public synonym의 경우
        // user_id는 NULL이다.
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SYNONYMS_ VALUES( "
                         "NULL, "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "SYSDATE, SYSDATE )",
                         sSynonymName,
                         sObjectOwnerName,
                         sObjectName );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SYNONYMS_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "VARCHAR'%s', "
                         "SYSDATE, SYSDATE )",
                         sParseTree->synonymOwnerID,
                         sSynonymName,
                         sObjectOwnerName,
                         sObjectName );
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated(
                  aStatement,
                  sParseTree->synonymOwnerID,
                  sSynonymName,
                  idlOS::strlen( sSynonymName ),
                  QS_SYNONYM )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

