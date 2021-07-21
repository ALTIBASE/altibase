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

#ifndef _ULP_LIB_CONNECTION_H_
#define _ULP_LIB_CONNECTION_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <sqlcli.h>
#include <ulpLibHash.h>
#include <ulpLibStmtCur.h>
#include <ulpLibOption.h>
#include <ulpLibErrorMgr.h>
#include <ulpLibMacro.h>
#include <ulpLibMultiErrorMgr.h>

/*
 * The node managed by connection hash table.
 */
typedef struct ulpLibConnNode ulpLibConnNode;
struct ulpLibConnNode
{
    SQLHENV        mHenv;
    SQLHDBC        mHdbc;
    SQLHSTMT       mHstmt;         /* single thread일 경우 공유되는 statement handle */
    acp_char_t     mConnName[MAX_CONN_NAME_LEN + 1];  /* connection 이름 (AT절) */
    acp_char_t    *mUser;          /* user id */
    acp_char_t    *mPasswd;        /* password */
    acp_char_t    *mConnOpt1;
    acp_char_t    *mConnOpt2;      /* 첫번째 ConnOpt1 으로 접속 실패할 경우 이를 이용하여 시도 */

    /* statement hash table */
    ulpLibStmtHASHTAB mStmtHashT;

    /* cursor hash table */
    ulpLibStmtHASHTAB mCursorHashT;

    /* unnamed statement cache list */
    ulpLibStmtLIST mUnnamedStmtCacheList;

    /* TASK-7218 Handling Multiple Errors
     * 이 변수를 ConnNode 내에 두는 이유:
     *  에러 갯수가 가변적이므로 동적 메모리 할당이 불가피한데,
     *  sqlca처럼 thread local 변수를 사용할 경우 해제할 방법이 없다. 
     *  따라서, ConnNode 내에 두어서 ConnNode가 삭제될 때 이 변수에
     *  할당된 메모리도 해제되도록 함.
     */
    ulpMultiErrorMgr mMultiErrorMgr;

    acp_bool_t       mIsXa;       /* Xa접속 관련 정보 */
    acp_sint32_t     mXaRMID;
    acp_thr_rwlock_t mLatch4Xa;

    ulpLibConnNode *mNext;       /* bucket list link */

    /* 사용가능한 상태인가를 나타낸다.ulpConnect()에서 호출시에는 false로 초기화되며
       나머지는 true로 초기화 된다.*/
    acp_bool_t      mValid;
};


/*
 * The connection hash table
 */
typedef struct  ulpLibConnHashTab
{
    acp_sint32_t     mSize    ;        /* Max number of buckets in the table */
    acp_sint32_t     mNumNode ;        /* number of nodes currently in the table */
    acp_thr_rwlock_t mLatch;           /* table latch */
    acp_uint32_t     (*mHash) (acp_uint8_t *);       /* hash function */

    /* comparison funct, cmp(name,bucket_p); */
    acp_sint32_t     (*mCmp ) (
                       const acp_char_t*,
                       const acp_char_t*,
                       acp_size_t );

    /* actual hash table    */
    ulpLibConnNode *mTable[MAX_NUM_CONN_HASH_TAB_ROW];

} ulpLibConnHASHTAB;


/*
 * The connection hash table manager.
 */
typedef struct ulpLibConnMgr
{
    /* the connection hash table */
    ulpLibConnHASHTAB mConnHashTab;

    /* for connection concurrency */
    acp_thr_rwlock_t mConnLatch;

} ulpLibConnMgr;

extern ulpLibConnMgr gUlpConnMgr;


/*
 * Functions for managing connections.
 */

ACI_RC          ulpLibConInitialize( void );

void            ulpLibConFinalize( void );

/* 
 * create new connection node with aConnName,aValidInit.
 *     aConnName : connection name
 *     aValidInit: initial boolean value for mValid fileld 
 */
ulpLibConnNode *ulpLibConNewNode( acp_char_t *aConnName, acp_bool_t  aValidInit );

/* init default connection node */
void            ulpLibConInitDefaultConn( void );

/* default connection 객체를 얻어온다 */
ulpLibConnNode *ulpLibConGetDefaultConn();

/* connection 이름을 가지고 해당 connection객체를 찾는다 */
ulpLibConnNode *ulpLibConLookupConn(acp_char_t* aConName);

/* 새로운 connection을 연결 hash table에 추가한다. */
ulpLibConnNode *ulpLibConAddConn( ulpLibConnNode *aConnNode );

/* 해당 이름의 connection을 제거한다 */
ACI_RC          ulpLibConDelConn(acp_char_t* aConName);

/* hash table의 모든 ConnNode들을 제거한다 */
void            ulpLibConDelAllConn( void );

/* ConnNode 자료구조 해제함. */
void            ulpLibConFreeConnNode( ulpLibConnNode *aConnNode );

/* BUG-28209 : AIX 에서 c compiler로 컴파일하면 생성자 호출안됨. */
ACI_RC          ulpLibConInitConnMgr( void );


#endif
