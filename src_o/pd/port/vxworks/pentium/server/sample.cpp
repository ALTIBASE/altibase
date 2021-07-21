/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <sqlcli.h>
#include <taskLib.h>

#define SQL_LEN 1000
#define MSG_LEN 1024

SQLRETURN alloc_handle(SQLHENV &, SQLHDBC &);
SQLRETURN db_connect(SQLHDBC &, int &);
void free_handle(SQLHENV &, SQLHDBC &, int &);

SQLRETURN execute_select(SQLHDBC &dbc);
void execute_err(SQLHDBC aCon, SQLHSTMT aStmt, char* q);

int sqlcli( int aArg1 );

extern "C" int sample( int aArg1 )
{
    taskSpawn( "a1", 100, 0x19, 100000, (FUNCPTR)sqlcli, 0,0,0,0,0,0,0,0,0,0 );
    taskSpawn( "a2", 100, 0x19, 100000, (FUNCPTR)sqlcli, 0,0,0,0,0,0,0,0,0,0 );
    taskSpawn( "a3", 100, 0x19, 100000, (FUNCPTR)sqlcli, 0,0,0,0,0,0,0,0,0,0 );
}

int sqlcli( int aArg1 )
{
    SQLRETURN    rc;


    SQLHENV  env;  // Environment Handle
    SQLHDBC  dbc;  // Connection Handle
    int      conn_flag;

    env = SQL_NULL_HENV;
    dbc = SQL_NULL_HDBC;
    conn_flag = 0;

    /* allocate handle */
    rc = alloc_handle(env, dbc);
    if ( rc != SQL_SUCCESS )
    {
        free_handle(env, dbc, conn_flag);
        exit(1);
    }

    /* Connect to Altibase Server */
    rc = db_connect(dbc, conn_flag);
    if ( rc != SQL_SUCCESS )
    {
        free_handle(env, dbc, conn_flag);
        exit(1);
    }

    /* select data */
    rc = execute_select(dbc);
    if ( rc != SQL_SUCCESS )
    {
        free_handle(env, dbc, conn_flag);
        exit(1);
    }

    /* disconnect, free handles */
    free_handle(env, dbc, conn_flag);
}

void execute_err(SQLHDBC aCon, SQLHSTMT aStmt, char* q)
{
    SQLINTEGER errNo;
    SQLSMALLINT msgLength;
    SQLCHAR errMsg[MSG_LEN];

    printf("Error : %s\n",q);

    if (SQLError ( SQL_NULL_HENV, aCon, aStmt,
                   NULL, &errNo,
                   errMsg, MSG_LEN, &msgLength ) == SQL_SUCCESS) 
    {
        printf(" Error : # %ld, %s\n", errNo, errMsg);
    }
}

SQLRETURN alloc_handle(SQLHENV &env, SQLHDBC &dbc)
{
    /* allocate Environment handle */
    if (SQLAllocEnv(&env) != SQL_SUCCESS)
    {
        printf("SQLAllocEnv error!!\n");
        return SQL_ERROR;
    }

    /* allocate Connection handle */
    if (SQLAllocConnect(env, &dbc) != SQL_SUCCESS) 
    {
        printf("SQLAllocConnect error!!\n");
        return SQL_ERROR;
    }
    return SQL_SUCCESS;
}

void free_handle(SQLHENV &env, SQLHDBC &dbc, int &conn_flag)
{
    if ( conn_flag == 1 )
    {
        /* close connection */
        SQLDisconnect( dbc );
    }
    /* free connection handle */
    if ( dbc != NULL )
    {
        SQLFreeConnect( dbc );
    }
    if ( env != NULL )
    {
        SQLFreeEnv( env );
    }
}

SQLRETURN db_connect(SQLHDBC &dbc, int &conn_flag)
{
    char    *USERNAME = "SYS";        // user name
    char    *PASSWD   = "MANAGER";    // user password
    char    *NLS      = "US7ASCII";   // NLS_USE ( KO16KSC5601, US7ASCII )
    char     connStr[1024];

    sprintf(connStr,
            "DSN=127.0.0.1;UID=%s;PWD=%s;CONNTYPE=%d;NLS_USE=%s;PORT_NO=20300", /* ;PORT_NO=20300", */
            USERNAME, PASSWD, 4, NLS);

    printf( "%s\n", connStr );

    /* establish connection */
    if (SQLDriverConnect( dbc, NULL, (SQLCHAR*)connStr, SQL_NTS,
                          NULL, 0, NULL,
                          SQL_DRIVER_NOPROMPT ) != SQL_SUCCESS)
    {
        execute_err(dbc, SQL_NULL_HSTMT, "SQLDriverConnect");
        return SQL_ERROR;
    }

    conn_flag = 1;

    return SQL_SUCCESS;
}

SQLRETURN execute_select(SQLHDBC &dbc)
{
    SQLHSTMT     stmt = SQL_NULL_HSTMT;
    SQLRETURN    rc;
    char         query[SQL_LEN];

    char                 id[8+1];
    char                 name[20+1];
//  char                 plan[256+1];
    char                *plan;
    SQLINTEGER           age;

//  SQLINTEGER   plan_ind;

    /* allocate Statement handle */
    if (SQL_ERROR == SQLAllocStmt(dbc, &stmt)) 
    {
        printf("SQLAllocStmt error!!\n");
        return SQL_ERROR;
    }

//  sprintf(query,"alter session set explain plan=on");
//  if (SQLExecDirect(stmt,query, SQL_NTS) != SQL_SUCCESS)
//  {
//      execute_err(dbc, stmt, query);
//      SQLFreeStmt(stmt, SQL_DROP);
//      return SQL_ERROR;
//  }

    sprintf(query,"SELECT * FROM DEMO_PLAN WHERE id='20000000'");
    if (SQLExecDirect(stmt,(SQLCHAR*)query, SQL_NTS) != SQL_SUCCESS)
    {
        execute_err(dbc, stmt, query);
        SQLFreeStmt(stmt, SQL_DROP);
        return SQL_ERROR;
    }

    /* binds application data buffers to columns in the result set */
    if (SQLBindCol(stmt, 1, SQL_C_CHAR,
                   id, sizeof(id), NULL) != SQL_SUCCESS) 
    {
        printf("SQLBindCol error!!!\n");
        execute_err(dbc, stmt, query);
        SQLFreeStmt(stmt, SQL_DROP);
        return SQL_ERROR;
    }
    if (SQLBindCol(stmt, 2, SQL_C_CHAR,
                   name, sizeof(name), NULL) != SQL_SUCCESS) 
    {
        printf("SQLBindCol error!!!\n");
        execute_err(dbc, stmt, query);
        SQLFreeStmt(stmt, SQL_DROP);
        return SQL_ERROR;
    }
    if (SQLBindCol(stmt, 3, SQL_C_SLONG,
                   &age, 0, NULL) != SQL_SUCCESS) 
    {
        printf("SQLBindCol error!!!\n");
        execute_err(dbc, stmt, query);
        SQLFreeStmt(stmt, SQL_DROP);
        return SQL_ERROR;
    }

    /* fetches the next rowset of data from the result set and print to stdout */
    printf("id\tName\tAge\n");
    printf("===========================================\n");

    while ( (rc = SQLFetch(stmt)) != SQL_NO_DATA )
    {
        if ( rc == SQL_ERROR ) 
        {
            execute_err(dbc, stmt, query);
            break;
        }
        printf("%-10s%-20s%-5d\n", id, name, (int)age);
    }

    printf("\nExplain Plan\n");
//  if ( SQLGetStmtAttr(stmt, SQL_ATTR_EXPLAIN_PLAN, plan, sizeof(plan), &plan_ind) != SQL_SUCCESS )
    if ( SQLGetPlan(stmt, (SQLCHAR**)&plan) != SQL_SUCCESS )
    {
//      execute_err(dbc, stmt, "SQLGetStmtAttr(SQL_ATTR_EXPLAIN_PLAN) : ");
        execute_err(dbc, stmt, "SQLGetPlan() : ");
        SQLFreeStmt(stmt, SQL_DROP);
        return SQL_ERROR;
    }
    else
    {
        printf("%s", plan);
    }

    /* SQLFreeStmt(stmt, SQL_CLOSE); 
     * If a cursor fetched the last record of a result set, SQL_CLOSE can be omitted.
     * Howerver, if there are some records to be fetched by SLQFetch() and SQLExecute() is re-executed,
     * invalid cusor error occurs. */

    SQLFreeStmt(stmt, SQL_DROP);

    return SQL_SUCCESS;
}

