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

#include <idl.h>
#include <ide.h>
#include <qci.h>
#include <qdcAudit.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <qcm.h>
#include <qcmUser.h>
#include <qcmAudit.h>
#include <qcuSqlSourceInfo.h>

const static SChar * gOperColumnName[QCI_AUDIT_OPER_MAX][2] = { 
    { "SELECT_SUCCESS",        "SELECT_FAILURE"        },   // 0
    { "INSERT_SUCCESS",        "INSERT_FAILURE"        },  
    { "UPDATE_SUCCESS",        "UPDATE_FAILURE"        },  
    { "DELETE_SUCCESS",        "DELETE_FAILURE"        },  
    { "MOVE_SUCCESS",          "MOVE_FAILURE"          },  
    { "MERGE_SUCCESS",         "MERGE_FAILURE"         },  
    { "ENQUEUE_SUCCESS",       "ENQUEUE_FAILURE"       },  
    { "DEQUEUE_SUCCESS",       "DEQUEUE_FAILURE"       },  
    { "LOCK_SUCCESS",          "LOCK_FAILURE"          },  
    { "EXECUTE_SUCCESS",       "EXECUTE_FAILURE"       },
    { "COMMIT_SUCCESS",        "COMMIT_FAILURE"        },   // 10
    { "ROLLBACK_SUCCESS",      "ROLLBACK_FAILURE"      },
    { "SAVEPOINT_SUCCESS",     "SAVEPOINT_FAILURE"     },
    { "CONNECT_SUCCESS",       "CONNECT_FAILURE"       },
    { "DISCONNECT_SUCCESS",    "DISCONNECT_FAILURE"    },
    { "ALTER_SESSION_SUCCESS", "ALTER_SESSION_FAILURE" },
    { "ALTER_SYSTEM_SUCCESS",  "ALTER_SYSTEM_FAILURE"  },
    { "DDL_SUCCESS",           "DDL_FAILURE"           }    // 17
};

const static SChar * gOperName[QCI_AUDIT_OPER_MAX] = {
    "SELECT",   // 0
    "INSERT",
    "UPDATE",
    "DELETE",
    "MOVE",
    "MERGE",
    "ENQUEUE",
    "DEQUEUE",
    "LOCK",
    "EXECUTE",
    "COMMIT",   // 10
    "ROLLBACK",
    "SAVEPOINT",
    "CONNECT",
    "DISCONNECT",
    "ALTER SESSION",
    "ALTER SYSTEM",
    "DDL"      // 17
};

IDE_RC qdcAudit::auditOption( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qdcAuditParseTree  * sParseTree;
    qciAuditOperation    sOperations;
    qcuSqlSourceInfo     sqlInfo;
    idBool               sExist = ID_FALSE;
    qsOID                sProcID;
    SChar              * sSqlStr;
    UInt                 sSqlStrLen;
    SChar                sComma[3] = { 0, 0, 0 };
    SChar                sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                sOperValue[QCI_AUDIT_OPER_MAX][2][2];
    vSLong               sRowCnt = 0;
    UInt                 sOper;
    UInt                 sOperIdx;

    sParseTree = (qdcAuditParseTree *) aStatement->myPlan->parseTree;
    
    if ( QC_IS_NULL_NAME(sParseTree->objectName) == ID_FALSE )
    {
        // object auditing
        IDE_TEST( qcm::existObject( aStatement,
                                    ID_FALSE,
                                    sParseTree->userName,
                                    sParseTree->objectName,
                                    QS_OBJECT_MAX,
                                    &sParseTree->userID,
                                    &sProcID,
                                    &sExist )
                  != IDE_SUCCESS );
        
        if ( sExist == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement, &(sParseTree->objectName) );
            IDE_RAISE( ERR_NOT_EXIST_OBJECT );
        }
        else
        {
            // Nothing to do.
        }
        
        QC_STR_COPY( sObjectName, sParseTree->objectName );
    }
    else
    {
        // statement, privilege auditing
        if ( QC_IS_NULL_NAME(sParseTree->userName) == ID_FALSE )
        {
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          sParseTree->userName,
                                          &sParseTree->userID )
                      != IDE_SUCCESS );
        }
        else
        {
            // all users
            sParseTree->userID = QC_EMPTY_USER_ID;
        }
        
        sObjectName[0] = '\0';
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( qcmAudit::getOption( QC_SMI_STMT( aStatement ),
                                   sParseTree->userID,
                                   sObjectName,
                                   idlOS::strlen(sObjectName),
                                   & sOperations,
                                   & sExist )
              != IDE_SUCCESS );

    if ( sExist == ID_FALSE )
    {
        //------------------------------------------------
        // DML, DCL options
        //------------------------------------------------
        
        for ( sOperIdx = 0; sOperIdx < QCI_AUDIT_OPER_MAX; sOperIdx++ )
        {

            sOper = sParseTree->operations->operation[sOperIdx];

            switch( (qciAuditOperIdx)sOperIdx )
            {
                /* The below operations must be set to 'T' or '-' 
                   since they cannot have a logging mode. */
                case QCI_AUDIT_OPER_CONNECT:
                case QCI_AUDIT_OPER_DISCONNECT:
                case QCI_AUDIT_OPER_DDL:

                    if ( ( ( sOper & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK ) ==
                           QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE )
                         ||
                         ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK ) ==
                           QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE ) )
                    {
                        sOperValue[sOperIdx][0][0] = 'T';
                        sOperValue[sOperIdx][0][1] = 0;
                    }
                    else
                    {
                        sOperValue[sOperIdx][0][0] = '-';
                        sOperValue[sOperIdx][0][1] = 0;
                    }

                    if ( ( ( sOper & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK ) ==
                           QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE )
                         ||
                         ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK ) ==
                           QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE ) )
                    {
                        sOperValue[sOperIdx][1][0] = 'T';
                        sOperValue[sOperIdx][1][1] = 0;
                    }
                    else
                    {
                        sOperValue[sOperIdx][1][0] = '-';
                        sOperValue[sOperIdx][1][1] = 0;
                    }

                    break;

                /* DML, and DCL statements that can have a logging mode. */
                case QCI_AUDIT_OPER_SELECT:
                case QCI_AUDIT_OPER_INSERT:
                case QCI_AUDIT_OPER_UPDATE:
                case QCI_AUDIT_OPER_DELETE:
                case QCI_AUDIT_OPER_MOVE:
                case QCI_AUDIT_OPER_MERGE:
                case QCI_AUDIT_OPER_ENQUEUE:
                case QCI_AUDIT_OPER_DEQUEUE:
                case QCI_AUDIT_OPER_LOCK:
                case QCI_AUDIT_OPER_EXECUTE:
                case QCI_AUDIT_OPER_COMMIT:
                case QCI_AUDIT_OPER_ROLLBACK:
                case QCI_AUDIT_OPER_SAVEPOINT:
                case QCI_AUDIT_OPER_ALTER_SESSION:
                case QCI_AUDIT_OPER_ALTER_SYSTEM:

                    if ( ( sOper & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK ) ==
                         QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE )
                    {
                        sOperValue[sOperIdx][0][0] = 'S';
                        sOperValue[sOperIdx][0][1] = 0;
                    }
                    else if ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK ) ==
                              QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE )
                    {
                        sOperValue[sOperIdx][0][0] = 'A';
                        sOperValue[sOperIdx][0][1] = 0;
                    }
                    else
                    {
                        sOperValue[sOperIdx][0][0] = '-';
                        sOperValue[sOperIdx][0][1] = 0;
                    }
                    
                    if ( ( sOper & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK ) ==
                         QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE )
                    {
                        sOperValue[sOperIdx][1][0] = 'S';
                        sOperValue[sOperIdx][1][1] = 0;
                    }
                    else if ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK ) ==
                         QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE )
                    {
                        sOperValue[sOperIdx][1][0] = 'A';
                        sOperValue[sOperIdx][1][1] = 0;
                    }
                    else
                    {
                        sOperValue[sOperIdx][1][0] = '-';
                        sOperValue[sOperIdx][1][1] = 0;
                    }

                    break;

                default:
                    break;
            } /* switch */
        } /* for */

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_AUDIT_ALL_OPTS_ VALUES ( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "
                         "CHAR'%s', CHAR'%s', "  /* SELECT        */
                         "CHAR'%s', CHAR'%s', "  /* INSERT        */
                         "CHAR'%s', CHAR'%s', "  /* UPDATE        */
                         "CHAR'%s', CHAR'%s', "  /* DELETE        */
                         "CHAR'%s', CHAR'%s', "  /* MOVE          */
                         "CHAR'%s', CHAR'%s', "  /* MERGE         */
                         "CHAR'%s', CHAR'%s', "  /* ENQUEUE       */
                         "CHAR'%s', CHAR'%s', "  /* DEQUEUE       */
                         "CHAR'%s', CHAR'%s', "  /* LOCK          */
                         "CHAR'%s', CHAR'%s', "  /* EXECUTE       */
                         "CHAR'%s', CHAR'%s', "  /* COMMIT        */
                         "CHAR'%s', CHAR'%s', "  /* ROLLBACK      */
                         "CHAR'%s', CHAR'%s', "  /* SAVEPOINT     */
                         "CHAR'%s', CHAR'%s', "  /* CONNECT       */
                         "CHAR'%s', CHAR'%s', "  /* DISCONNECT    */
                         "CHAR'%s', CHAR'%s', "  /* ALTER SESSION */
                         "CHAR'%s', CHAR'%s', "  /* ALTER SYSTEM  */
                         "CHAR'%s', CHAR'%s', "  /* DDL           */
                         "SYSDATE, NULL )",
                         sParseTree->userID,
                         sObjectName,
                         sOperValue[QCI_AUDIT_OPER_SELECT][0],
                         sOperValue[QCI_AUDIT_OPER_SELECT][1],
                         sOperValue[QCI_AUDIT_OPER_INSERT][0],
                         sOperValue[QCI_AUDIT_OPER_INSERT][1],
                         sOperValue[QCI_AUDIT_OPER_UPDATE][0],
                         sOperValue[QCI_AUDIT_OPER_UPDATE][1],
                         sOperValue[QCI_AUDIT_OPER_DELETE][0],
                         sOperValue[QCI_AUDIT_OPER_DELETE][1],
                         sOperValue[QCI_AUDIT_OPER_MOVE][0],
                         sOperValue[QCI_AUDIT_OPER_MOVE][1],
                         sOperValue[QCI_AUDIT_OPER_MERGE][0],
                         sOperValue[QCI_AUDIT_OPER_MERGE][1],
                         sOperValue[QCI_AUDIT_OPER_ENQUEUE][0],
                         sOperValue[QCI_AUDIT_OPER_ENQUEUE][1],
                         sOperValue[QCI_AUDIT_OPER_DEQUEUE][0],
                         sOperValue[QCI_AUDIT_OPER_DEQUEUE][1],
                         sOperValue[QCI_AUDIT_OPER_LOCK][0],
                         sOperValue[QCI_AUDIT_OPER_LOCK][1],
                         sOperValue[QCI_AUDIT_OPER_EXECUTE][0],
                         sOperValue[QCI_AUDIT_OPER_EXECUTE][1],
                         sOperValue[QCI_AUDIT_OPER_COMMIT][0],
                         sOperValue[QCI_AUDIT_OPER_COMMIT][1],
                         sOperValue[QCI_AUDIT_OPER_ROLLBACK][0],
                         sOperValue[QCI_AUDIT_OPER_ROLLBACK][1],
                         sOperValue[QCI_AUDIT_OPER_SAVEPOINT][0],
                         sOperValue[QCI_AUDIT_OPER_SAVEPOINT][1],
                         sOperValue[QCI_AUDIT_OPER_CONNECT][0],
                         sOperValue[QCI_AUDIT_OPER_CONNECT][1],
                         sOperValue[QCI_AUDIT_OPER_DISCONNECT][0],
                         sOperValue[QCI_AUDIT_OPER_DISCONNECT][1],
                         sOperValue[QCI_AUDIT_OPER_ALTER_SESSION][0],
                         sOperValue[QCI_AUDIT_OPER_ALTER_SESSION][1],
                         sOperValue[QCI_AUDIT_OPER_ALTER_SYSTEM][0],
                         sOperValue[QCI_AUDIT_OPER_ALTER_SYSTEM][1],
                         sOperValue[QCI_AUDIT_OPER_DDL][0],
                         sOperValue[QCI_AUDIT_OPER_DDL][1] );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_AUDIT_ALL_OPTS_ SET " );

        //------------------------------------------------
        // DML, DCL options
        //------------------------------------------------
        
        for ( sOperIdx = 0; sOperIdx < QCI_AUDIT_OPER_MAX; sOperIdx++ )
        {
            sOper = sParseTree->operations->operation[sOperIdx];
           
            sSqlStrLen = idlOS::strlen( sSqlStr );

            switch ( (qciAuditOperIdx)sOperIdx )
            {
                case QCI_AUDIT_OPER_CONNECT:
                case QCI_AUDIT_OPER_DISCONNECT:
                case QCI_AUDIT_OPER_DDL:

                    if ( ( ( sOper & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK ) ==
                           QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE )
                         ||
                         ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK ) ==
                           QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE ) )
                    {
                        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                                         "%s%s = CHAR'T'",
                                         sComma,
                                         gOperColumnName[sOperIdx][0] );
                        sComma[0] = ',';
                        sComma[1] = ' ';
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sSqlStrLen = idlOS::strlen( sSqlStr );

                    if ( ( ( sOper & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK ) ==
                           QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE )
                         ||
                         ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK ) ==
                           QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE ) )
                    {
                        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                                         "%s%s = CHAR'T'",
                                         sComma,
                                         gOperColumnName[sOperIdx][1] );
                        sComma[0] = ',';
                        sComma[1] = ' ';
                    }
                    else
                    {
                        // Nothing to do.
                    }

                   break;

                /* DML, and DCL statements that can be audited by session, or by access. */
                case QCI_AUDIT_OPER_SELECT:
                case QCI_AUDIT_OPER_INSERT:
                case QCI_AUDIT_OPER_UPDATE:
                case QCI_AUDIT_OPER_DELETE:
                case QCI_AUDIT_OPER_MOVE:
                case QCI_AUDIT_OPER_MERGE:
                case QCI_AUDIT_OPER_ENQUEUE:
                case QCI_AUDIT_OPER_DEQUEUE:
                case QCI_AUDIT_OPER_LOCK:
                case QCI_AUDIT_OPER_EXECUTE:
                case QCI_AUDIT_OPER_COMMIT:
                case QCI_AUDIT_OPER_ROLLBACK:
                case QCI_AUDIT_OPER_SAVEPOINT:
                case QCI_AUDIT_OPER_ALTER_SESSION:
                case QCI_AUDIT_OPER_ALTER_SYSTEM:

                    if ( ( sOper & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK ) ==
                         QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE )
                    {
                        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                                         "%s%s = CHAR'S'",
                                         sComma,
                                         gOperColumnName[sOperIdx][0] );
                        sComma[0] = ',';
                        sComma[1] = ' ';
                    }
                    else if ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK ) ==
                              QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE )
                    {
                        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                                         "%s%s = CHAR'A'",
                                         sComma,
                                         gOperColumnName[sOperIdx][0] );
                        sComma[0] = ',';
                        sComma[1] = ' ';
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sSqlStrLen = idlOS::strlen( sSqlStr );

                    if ( ( sOper & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK ) ==
                         QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE )
                    {
                        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                                         "%s%s = CHAR'S'",
                                         sComma,
                                         gOperColumnName[sOperIdx][1] );
                        sComma[0] = ',';
                        sComma[1] = ' ';
                    }
                    else if ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK ) ==
                         QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE )
                    {
                        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                                         "%s%s = CHAR'A'",
                                         sComma,
                                         gOperColumnName[sOperIdx][1] );
                        sComma[0] = ',';
                        sComma[1] = ' ';
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                   break;

                default:
                   break;
            } /* switch */

        } /* for */

        sSqlStrLen = idlOS::strlen( sSqlStr );

        if ( QC_IS_NULL_NAME(sParseTree->objectName) == ID_FALSE )
        {
            idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                             "%sLAST_DDL_TIME = SYSDATE "
                             "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                             "AND OBJECT_NAME = VARCHAR'%s'",
                             sComma,
                             sParseTree->userID,
                             sObjectName );
        }
        else
        {
            idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                             "%sLAST_DDL_TIME = SYSDATE "
                             "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                             "AND OBJECT_NAME IS NULL",
                             sComma,
                             sParseTree->userID );
        }
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_OBJECT )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcAudit::noAuditOption( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qdcNoAuditParseTree * sParseTree;
    qciAuditOperation     sOperations;
    qcuSqlSourceInfo      sqlInfo;
    idBool                sExist = ID_FALSE;
    SChar               * sSqlStr;
    UInt                  sSqlStrLen;
    SChar                 sComma[3] = { 0, 0, 0 };
    SChar                 sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    vSLong                sRowCnt = 0;
    UInt                  sOper;
    UInt                  i;

    sParseTree = (qdcNoAuditParseTree *) aStatement->myPlan->parseTree;

    if ( QC_IS_NULL_NAME(sParseTree->objectName) == ID_FALSE )
    {
        if ( QC_IS_NULL_NAME(sParseTree->userName) == ID_FALSE )
        {
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          sParseTree->userName,
                                          &sParseTree->userID )
                      != IDE_SUCCESS );
        }
        else
        {
            sParseTree->userID = QCG_GET_SESSION_USER_ID(aStatement);
        }
        
        QC_STR_COPY( sObjectName, sParseTree->objectName );
    }
    else
    {
        // statement, privilege auditing
        if ( QC_IS_NULL_NAME(sParseTree->userName) == ID_FALSE )
        {
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          sParseTree->userName,
                                          &sParseTree->userID )
                      != IDE_SUCCESS );
        }
        else
        {
            // all users
            sParseTree->userID = QC_EMPTY_USER_ID;
        }
        
        sObjectName[0] = '\0';
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( qcmAudit::getOption( QC_SMI_STMT( aStatement ),
                                   sParseTree->userID,
                                   sObjectName,
                                   idlOS::strlen(sObjectName),
                                   & sOperations,
                                   & sExist )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NO_AUDIT_CONDITION );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_AUDIT_ALL_OPTS_ SET " );

    for ( i = 0; i < QCI_AUDIT_OPER_MAX; i++ )
    {
        sOper = sParseTree->operations->operation[i];

        sSqlStrLen = idlOS::strlen( sSqlStr );
        
        if ( ( ( sOper & QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK ) ==
               QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE )
             ||
             ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK ) ==
               QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE ) )
        {
            idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                             "%s%s = CHAR'-'",
                             sComma,
                             gOperColumnName[i][0] );
            sComma[0] = ','; sComma[1] = ' ';
        }
        else
        {
            // Nothing to do.
        }
            
        sSqlStrLen = idlOS::strlen( sSqlStr );
        
        if ( ( ( sOper & QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK ) ==
               QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE )
             ||
             ( ( sOper & QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK ) ==
               QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE ) )
        {
            idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                             "%s%s = CHAR'-'",
                             sComma,
                             gOperColumnName[i][1] );
            sComma[0] = ','; sComma[1] = ' ';
        }
        else
        {
            // Nothing to do.
        }
    }
        
    sSqlStrLen = idlOS::strlen( sSqlStr );

    if ( QC_IS_NULL_NAME(sParseTree->objectName) == ID_FALSE )
    {
        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                         "%sLAST_DDL_TIME = SYSDATE "
                         "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND OBJECT_NAME = VARCHAR'%s'",
                         sComma,
                         sParseTree->userID,
                         sObjectName );
    }
    else
    {
        idlOS::snprintf( sSqlStr + sSqlStrLen, QD_MAX_SQL_LENGTH - sSqlStrLen,
                         "%sLAST_DDL_TIME = SYSDATE "
                         "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND OBJECT_NAME IS NULL",
                         sComma,
                         sParseTree->userID );
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_AUDIT_CONDITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDC_AUDIT_NO_AUDIT_CONDITION_EXISTS_ON_THE_OBJ, sObjectName ));
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcAudit::delAuditOption( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : BUG-39074
 *
 * Implementation : 
 *
 ***********************************************************************/
    
    qdcDelAuditParseTree * sParseTree;
    qciAuditOperation      sOperations;
    qcuSqlSourceInfo       sqlInfo;
    idBool                 sExist = ID_FALSE;
    SChar                * sSqlStr;
    SChar                  sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    vSLong                 sRowCnt = 0;
    idBool                 sIsStarted;

    IDE_TEST( qcmAudit::getStatus( QC_SMI_STMT( aStatement ),
                                   & sIsStarted )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsStarted == ID_TRUE, ERR_ALREADY_STARTED );

    sParseTree = (qdcDelAuditParseTree *) aStatement->myPlan->parseTree;

    if ( QC_IS_NULL_NAME(sParseTree->objectName) == ID_FALSE )
    {
        if ( QC_IS_NULL_NAME(sParseTree->userName) == ID_FALSE )
        {
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          sParseTree->userName,
                                          &sParseTree->userID )
                      != IDE_SUCCESS );
        }
        else
        {
            sParseTree->userID = QCG_GET_SESSION_USER_ID(aStatement);
        }
        
        QC_STR_COPY( sObjectName, sParseTree->objectName );
    }
    else
    {
        // statement, privilege auditing
        if ( QC_IS_NULL_NAME(sParseTree->userName) == ID_FALSE )
        {
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          sParseTree->userName,
                                          &sParseTree->userID )
                      != IDE_SUCCESS );
        }
        else
        {
            // all users
            sParseTree->userID = QC_EMPTY_USER_ID;
        }
        
        sObjectName[0] = '\0';
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( qcmAudit::getOption( QC_SMI_STMT( aStatement ),
                                   sParseTree->userID,
                                   sObjectName,
                                   idlOS::strlen(sObjectName),
                                   & sOperations,
                                   & sExist )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NO_AUDIT_CONDITION );

    if ( QC_IS_NULL_NAME(sParseTree->objectName) == ID_FALSE )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_AUDIT_ALL_OPTS_ "
                         "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND OBJECT_NAME = VARCHAR'%s'",
                         sParseTree->userID,
                         sObjectName );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_AUDIT_ALL_OPTS_ "
                         "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND OBJECT_NAME IS NULL",
                         sParseTree->userID );
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALREADY_STARTED )
    {    
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDC_AUDIT_ALREADY_STARTED));
    }
    IDE_EXCEPTION( ERR_NO_AUDIT_CONDITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDC_AUDIT_NO_AUDIT_CONDITION_EXISTS_ON_THE_OBJ, sObjectName ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcAudit::start( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool               sIsStarted;
    SChar              * sSqlStr;
    vSLong               sRowCnt = 0;
    qciAuditOperation  * sObjectOptions = NULL;
    UInt                 sObjectOptionCount;
    qciAuditOperation  * sUserOptions = NULL;
    UInt                 sUserOptionCount;

    IDE_TEST( qcmAudit::getStatus( QC_SMI_STMT( aStatement ),
                                   & sIsStarted )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsStarted == ID_TRUE, ERR_ALREADY_STARTED );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( qcmAudit::getAllOptions( QC_SMI_STMT( aStatement ),
                                       & sObjectOptions,
                                       & sObjectOptionCount,
                                       & sUserOptions,
                                       & sUserOptionCount )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sObjectOptionCount + sUserOptionCount == 0,
                    ERR_NO_AUDIT_CONDITION_EXISTS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_AUDIT_ SET "
                     "IS_STARTED = INTEGER'1', "
                     "START_TIME = SYSDATE, "
                     "STOP_TIME = NULL, "
                     "RELOAD_TIME = SYSDATE ");
    
    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    // 더 이상 실패하지 않는 시점에 호출한다.
    /* Call a callback function of AuditManager to set the audit status */
    qci::mAuditManagerCallback.mReloadAuditCond( sObjectOptions,
                                                 sObjectOptionCount,
                                                 sUserOptions,
                                                 sUserOptionCount );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_ALREADY_STARTED )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDC_AUDIT_ALREADY_STARTED));
    }
    IDE_EXCEPTION( ERR_NO_AUDIT_CONDITION_EXISTS )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDC_AUDIT_NO_AUDIT_CONDITION_EXISTS));
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sObjectOptions != NULL )
    {
        (void) iduMemMgr::free(sObjectOptions);
    }
    
    if ( sUserOptions != NULL )
    {
        (void) iduMemMgr::free(sUserOptions);
    }
    
    return IDE_FAILURE;
}

IDE_RC qdcAudit::stop( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/
    
    SChar   * sSqlStr;
    vSLong    sRowCnt = 0;
    idBool    sIsStarted;

    IDE_TEST( qcmAudit::getStatus( QC_SMI_STMT( aStatement ),
                                   & sIsStarted )
              != IDE_SUCCESS );

    if ( sIsStarted == ID_TRUE )
    {
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS);

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_AUDIT_ SET "
                         "IS_STARTED = INTEGER'0', "
                         "START_TIME = NULL, "
                         "STOP_TIME = SYSDATE" );
        
        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );
    
        IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
        
        /* Call a callback function of AuditManager to set the audit status */
        qci::mAuditManagerCallback.mStopAudit();
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcAudit::reload( qcStatement * aStatement  )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/
    
    SChar                sSqlStr[QD_MAX_SQL_LENGTH + 1];
    vSLong               sRowCnt = 0;
    idBool               sIsStarted;
    qciAuditOperation  * sObjectOptions = NULL;
    UInt                 sObjectOptionCount;
    qciAuditOperation  * sUserOptions = NULL;
    UInt                 sUserOptionCount;

    IDE_TEST( qcmAudit::getStatus( QC_SMI_STMT( aStatement ),
                                   & sIsStarted )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsStarted == ID_FALSE, ERR_NOT_STARTED );

    if ( sIsStarted == ID_TRUE )
    {
        IDE_TEST( qcmAudit::getAllOptions( QC_SMI_STMT( aStatement ),
                                           & sObjectOptions,
                                           & sObjectOptionCount,
                                           & sUserOptions,
                                           & sUserOptionCount )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sObjectOptionCount + sUserOptionCount == 0,
                        ERR_NO_AUDIT_CONDITION_EXISTS );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_AUDIT_ SET "
                         "RELOAD_TIME = SYSDATE" );
     
        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

        qci::mAuditManagerCallback.mReloadAuditCond( sObjectOptions,
                                                     sObjectOptionCount,
                                                     sUserOptions,
                                                     sUserOptionCount );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION( ERR_NOT_STARTED )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDC_AUDIT_NOT_STARTED));
    }
    IDE_EXCEPTION( ERR_NO_AUDIT_CONDITION_EXISTS )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDC_AUDIT_NO_AUDIT_CONDITION_EXISTS));
    }
    IDE_EXCEPTION_END;

    if ( sObjectOptions != NULL )
    {
        (void) iduMemMgr::free(sObjectOptions);
    }
    
    if ( sUserOptions != NULL )
    {
        (void) iduMemMgr::free(sUserOptions);
    }
    
    return IDE_FAILURE;
}

IDE_RC qdcAudit::setAllRefObjects( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    qcgPlanEnv              * sPlanEnv;
    qcgPlanPrivilege        * sPrivilege;
    qcgPlanObject           * sObject;
    qcgEnvTablePrivList     * sTablePriv;
    qcgEnvSequencePrivList  * sSeqPriv;
    qcgEnvProcList          * sProc;
    qcgEnvPkgList           * sPkg;          // BUG-36973
    qciAuditRefObject       * sRefObjects;
    UInt                      sCount = 0;

    if ( aStatement->myPlan->planEnv != NULL )
    {
        sPlanEnv   = aStatement->myPlan->planEnv;
        sObject    = & sPlanEnv->planObject;
        sPrivilege = & sPlanEnv->planPrivilege;

        for ( sTablePriv = sPrivilege->tableList;
              sTablePriv != NULL;
              sTablePriv = sTablePriv->next )
        {
            sCount++;
        }

        for ( sSeqPriv = sPrivilege->sequenceList;
              sSeqPriv != NULL;
              sSeqPriv = sSeqPriv->next )
        {
            sCount++;
        }

        for ( sProc = sObject->procList;
              sProc != NULL;
              sProc = sProc->next )
        {
            sCount++;    
        }

        // BUG-36973
        for( sPkg = sObject->pkgList;
             sPkg != NULL;
             sPkg = sPkg->next )
        {
            sCount++;
        }

        if( sCount > 0 )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                          ID_SIZEOF(qciAuditRefObject) * sCount,
                          (void**) & sRefObjects )
                      != IDE_SUCCESS );

            aStatement->myPlan->planEnv->auditInfo.refObjects
                = sRefObjects;
            aStatement->myPlan->planEnv->auditInfo.refObjectCount
                = sCount;
            
            //---------------------------------------------
            // table, view, queue
            //---------------------------------------------
        
            for ( sTablePriv = sPrivilege->tableList;
                  sTablePriv != NULL;
                  sTablePriv = sTablePriv->next )
            {
                sRefObjects->userID = sTablePriv->tableOwnerID;
                
                idlOS::strncpy( sRefObjects->objectName,
                                sTablePriv->tableName,
                                QC_MAX_OBJECT_NAME_LEN + 1 );
                sRefObjects->objectName[QC_MAX_OBJECT_NAME_LEN] = '\0';

                sRefObjects->objectType = QCI_AUDIT_OBJECT_TABLE;
            
                sRefObjects++;
            }

            //---------------------------------------------
            // sequence
            //---------------------------------------------
        
            for ( sSeqPriv = sPrivilege->sequenceList;
                  sSeqPriv != NULL;
                  sSeqPriv = sSeqPriv->next )
            {
                sRefObjects->userID = sSeqPriv->sequenceOwnerID;
                
                idlOS::strncpy( sRefObjects->objectName,
                                sSeqPriv->name,
                                QC_MAX_OBJECT_NAME_LEN + 1 );
                sRefObjects->objectName[QC_MAX_OBJECT_NAME_LEN] = '\0';
            
                sRefObjects->objectType = QCI_AUDIT_OBJECT_SEQ;

                sRefObjects++;
            }
        
            //---------------------------------------------
            // procedure, function
            //---------------------------------------------
        
            for ( sProc = sObject->procList;
                  sProc != NULL;
                  sProc = sProc->next )
            {
                sRefObjects->userID = sProc->planTree->userID;

                QC_STR_COPY( sRefObjects->objectName, sProc->planTree->procNamePos );
            
                sRefObjects->objectType = QCI_AUDIT_OBJECT_PROC;

                sRefObjects++;
            }

            // BUG-36973
            //---------------------------------------------
            // package
            //---------------------------------------------

            for ( sPkg = sObject->pkgList;
                  sPkg != NULL;
                  sPkg = sPkg->next )
            {
                sRefObjects->userID = sPkg->planTree->userID;

                QC_STR_COPY( sRefObjects->objectName, sPkg->planTree->pkgNamePos );

                sRefObjects->objectType = QCI_AUDIT_OBJECT_PKG;

                sRefObjects++;
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdcAudit::setOperation( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    qciAuditOperIdx  sOper;
    
    if ( aStatement->myPlan->planEnv != NULL )
    {
        getOperation( aStatement, & sOper );

        aStatement->myPlan->planEnv->auditInfo.operation = sOper;
    }
    else
    {
        // Nothing to do.
    }
}

void qdcAudit::getOperation( qcStatement     * aStatement,
                             qciAuditOperIdx * aOperation )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    qciStmtType      sStmtKind;
    qciAuditOperIdx  sOper;
    
    sOper = QCI_AUDIT_OPER_MAX;

    if ( aStatement->myPlan->parseTree != NULL )
    {
        sStmtKind = aStatement->myPlan->parseTree->stmtKind;
        
        if ( ( sStmtKind & QCI_STMT_MASK_MASK ) == QCI_STMT_MASK_DDL )
        {
            sOper = QCI_AUDIT_OPER_DDL;
        }
        else if ( ( sStmtKind & QCI_STMT_MASK_MASK ) == QCI_STMT_MASK_SP )
        {
            sOper = QCI_AUDIT_OPER_EXECUTE;
        }
        else
        {
            switch ( sStmtKind )
            {
                case QCI_STMT_SELECT:
                case QCI_STMT_SELECT_FOR_UPDATE:
                    sOper = QCI_AUDIT_OPER_SELECT;
                    break;

                case QCI_STMT_INSERT:
                    sOper = QCI_AUDIT_OPER_INSERT;
                    break;

                case QCI_STMT_UPDATE:
                    sOper = QCI_AUDIT_OPER_UPDATE;
                    break;

                case QCI_STMT_DELETE:
                    sOper = QCI_AUDIT_OPER_DELETE;
                    break;

                case QCI_STMT_MOVE:
                    sOper = QCI_AUDIT_OPER_MOVE;
                    break;

                case QCI_STMT_MERGE:
                    sOper = QCI_AUDIT_OPER_MERGE;
                    break;

                case QCI_STMT_ENQUEUE:
                    sOper = QCI_AUDIT_OPER_ENQUEUE;
                    break;

                case QCI_STMT_DEQUEUE:
                    sOper = QCI_AUDIT_OPER_DEQUEUE;
                    break;
                    
                case QCI_STMT_LOCK_TABLE:
                    sOper = QCI_AUDIT_OPER_LOCK;
                    break;

                case QCI_STMT_COMMIT:
                case QCI_STMT_COMMIT_FORCE:
                    sOper = QCI_AUDIT_OPER_COMMIT;
                    break;

                case QCI_STMT_ROLLBACK:
                case QCI_STMT_ROLLBACK_FORCE:
                case QCI_STMT_ROLLBACK_TO_SAVEPOINT:  /* BUG-48216 */
                    sOper = QCI_AUDIT_OPER_ROLLBACK;
                    break;

                case QCI_STMT_SAVEPOINT:
                    sOper = QCI_AUDIT_OPER_SAVEPOINT;
                    break;

                case QCI_STMT_CONNECT:
                    sOper = QCI_AUDIT_OPER_CONNECT;
                    break;

                case QCI_STMT_DISCONNECT:
                    sOper = QCI_AUDIT_OPER_DISCONNECT;
                    break;

                case QCI_STMT_SET_AUTOCOMMIT_TRUE:
                case QCI_STMT_SET_AUTOCOMMIT_FALSE:
                case QCI_STMT_SET_REPLICATION_MODE:
                case QCI_STMT_SET_PLAN_DISPLAY_ONLY:
                case QCI_STMT_SET_PLAN_DISPLAY_ON:
                case QCI_STMT_SET_PLAN_DISPLAY_OFF:
                case QCI_STMT_SET_TX:
                case QCI_STMT_SET_STACK:
                case QCI_STMT_SET_SESSION_PROPERTY:
                    sOper = QCI_AUDIT_OPER_ALTER_SESSION;
                    break;

                case QCI_STMT_ALT_SYS_CHKPT:
                case QCI_STMT_ALT_SYS_SHRINK_MEMPOOL:
                case QCI_STMT_ALT_SYS_MEMORY_COMPACT:
                case QCI_STMT_SET_SYSTEM_PROPERTY:
                case QCI_STMT_ALT_SYS_REORGANIZE:
                case QCI_STMT_ALT_SYS_VERIFY:
                case QCI_STMT_ALT_SYS_ARCHIVELOG:
                case QCI_STMT_ALT_SYS_SWITCH_LOGFILE:
                case QCI_STMT_ALT_SYS_FLUSH_BUFFER_POOL:
                case QCI_STMT_FLUSHER_ONOFF:
                case QCI_STMT_ALT_SYS_COMPACT_PLAN_CACHE:
                case QCI_STMT_ALT_SYS_RESET_PLAN_CACHE:
                case QCI_STMT_ALT_SYS_SECURITY:
                    sOper = QCI_AUDIT_OPER_ALTER_SYSTEM;
                    break;

                case QCI_STMT_ALT_TABLESPACE_CHKPT_PATH:
                case QCI_STMT_ALT_TABLESPACE_DISCARD:
                case QCI_STMT_ALT_DATAFILE_ONOFF:
                case QCI_STMT_ALT_RENAME_DATAFILE:
                case QCI_STMT_ALT_TABLESPACE_BACKUP:
                    // dcl이지만 ddl로 취급한다.
                    sOper = QCI_AUDIT_OPER_DDL;
                    break;
                    
                default:
                    break;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    *aOperation = sOper;
}

void qdcAudit::getOperationString( qciAuditOperIdx    aOperation,
                                   const SChar     ** aString )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( aOperation < QCI_AUDIT_OPER_MAX )
    {
        *aString = gOperName[aOperation];
    }
    else
    {
        *aString = NULL;
    }
}
