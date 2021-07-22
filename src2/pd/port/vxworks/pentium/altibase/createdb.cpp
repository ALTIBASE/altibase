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
 
/* for database create */
#include <stdio.h>
#include <stdlib.h>
#include <sqlcli.h>
#include <envLib.h>
#include <idl.h>
#include <idtBaseThread.h>
#include <idsCrypt.h>

#define SQL_LEN 1000
#define MSG_LEN 1024

static SQLHENV  env;  // Environment Handle
static SQLHDBC  hdbc;  // Connection Handle
static int      conn_flag;

static SQLRETURN alloc_handle();
static SQLRETURN db_connect();
static void free_handle();

static SQLRETURN execute_select();
static void execute_err(SQLHDBC aCon, SQLHSTMT aStmt, char* q);

static int gSize = 0;

//******************************************************
static int createdb_main( int argc, char **argv );

class Createdb : public idtBaseThread
{
public:
    Createdb() {};
    ~Createdb() {};

    void run();
};

void Createdb::run()
{
    createdb_main(0, NULL);
}

extern "C" void createdb( int aArg1 )
{
    gSize = aArg1;
    if( gSize == 0 )
    {
        gSize = 10;
    }

    Createdb a;
    a.start();
    idlOS::thr_join( a.getHandle(), NULL );
}
//******************************************************

void makeSyspassword()
{
/*
    SChar      sFile[1024];
    SChar      sPassword[12];
    FILE     * sFP;
    SChar    * sCryptStr;
    idsCrypt   sCryptor;

    idlOS::snprintf(sFile, 
                    sizeof(sFile),
                    "%s/conf/syspassword", 
                    getenv(IDP_HOME_ENV));

    if( idlOS::access( sFile, F_OK ) != 0 )
    {
        sFP = idlOS::fopen(sFile, "w");

        if( sFP != NULL )
        {
            idlOS::memset(sPassword, 0, ID_SIZEOF(sPassword));
            idlOS::snprintf(sPassword, ID_SIZEOF(sPassword), "MANAGER");

            sCryptStr = (SChar *)sCryptor.crypt((UChar *)sPassword);
            idlOS::fwrite(sCryptStr, 1, idlOS::strlen(sCryptStr), sFP);
            idlOS::fclose(sFP);
        }
        else  
        {
            idlOS::printf( "syspassword error: %d\n", errno );
            idlOS::exit(-1);
        }
    }
*/
}

int createdb_main( int argc, char **argv )
{
    SQLRETURN    rc;

    env = SQL_NULL_HENV;
    hdbc = SQL_NULL_HDBC;
    conn_flag = 0;

    (void)makeSyspassword();

    /* allocate handle */
    rc = alloc_handle();
    if ( rc != SQL_SUCCESS )
    {
        free_handle();
        exit(1);
    }

    /* Connect to Altibase Server */
    rc = db_connect();
    if ( rc != SQL_SUCCESS )
    {
        free_handle();
        exit(1);
    }

    /* select data */
    rc = execute_select();
    if ( rc != SQL_SUCCESS )
    {
        free_handle();
        exit(1);
    }

    /* disconnect, free handles */
    free_handle();
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

SQLRETURN alloc_handle()
{
    /* allocate Environment handle */
    if (SQLAllocEnv(&env) != SQL_SUCCESS)
    {
        printf("SQLAllocEnv error!!\n");
        return SQL_ERROR;
    }

    /* allocate Connection handle */
    if (SQLAllocConnect(env, &hdbc) != SQL_SUCCESS) 
    {
        printf("SQLAllocConnect error!!\n");
        return SQL_ERROR;
    }
    return SQL_SUCCESS;
}

void free_handle()
{
    if ( conn_flag == 1 )
    {
        /* close connection */
        SQLDisconnect( hdbc );
    }
    /* free connection handle */
    if ( hdbc != NULL )
    {
        SQLFreeConnect( hdbc );
    }
    if ( env != NULL )
    {
        SQLFreeEnv( env );
    }
}

SQLRETURN db_connect()
{
    char    *USERNAME = "SYS";        // user name
    char    *PASSWD   = "MANAGER";    // user password
    char    *NLS      = "US7ASCII";   // NLS_USE ( KO16KSC5601, US7ASCII )
    char     connStr[1024];

    sprintf(connStr,
            "DSN=127.0.0.1;UID=%s;PWD=%s;CONNTYPE=%d;NLS_USE=%s;PORT_NO=20300", /* ;PORT_NO=20300", */
            USERNAME, PASSWD, 5, NLS);

    /* establish connection */
    if (SQLDriverConnect( hdbc, NULL, (SQLCHAR*)connStr, SQL_NTS,
                          NULL, 0, NULL,
                          SQL_DRIVER_NOPROMPT ) != SQL_SUCCESS)
    {
        execute_err(hdbc, SQL_NULL_HSTMT, "SQLDriverConnect");
        return SQL_ERROR;
    }

    conn_flag = 1;

    return SQL_SUCCESS;
}

SQLRETURN execute_select()
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
    if (SQL_ERROR == SQLAllocStmt(hdbc, &stmt)) 
    {
        printf("SQLAllocStmt error!!\n");
        return SQL_ERROR;
    }

    //sprintf(query,"create database mydb initsize=%dM noarchivelog", gSize);
    sprintf(query,"create database mydb initsize=128K noarchivelog");
    printf( "%s\n", query );
    if (SQLExecDirect(stmt,(SQLCHAR *)query, SQL_NTS) != SQL_SUCCESS)
    {
        execute_err(hdbc, stmt, query);
        SQLFreeStmt(stmt, SQL_DROP);
        return SQL_ERROR;
    }

    //sprintf(query,"alter database mydb shutdown exit");
    //if (SQLExecDirect(stmt,(SQLCHAR *)query, SQL_NTS) != SQL_SUCCESS)
    //{
    //    execute_err(hdbc, stmt, query);
    //    SQLFreeStmt(stmt, SQL_DROP);
    //    return SQL_ERROR;
    //}

    SQLFreeStmt(stmt, SQL_DROP);

    return SQL_SUCCESS;
}
