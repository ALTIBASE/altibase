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

#ifndef _ULP_LIB_STMTCUR_H_
#define _ULP_LIB_STMTCUR_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <sqlcli.h>
#include <ulpLibHash.h>
#include <ulpLibInterface.h>
#include <ulpLibErrorMgr.h>
#include <ulpLibOption.h>
#include <ulpLibMacro.h>
#include <ulpTypes.h>

/* ulpBindHostVarCore(...) 함수 내에서 ulpBindInfo 필드값들을 setting한다.             *
 * row/column-wise binding시 ulpSetStmtAttrCore(...) 함수에서 해당 필드값들을 참조한다. */
typedef struct ulpStmtBindInfo
{
    acp_uint16_t mNumofInHostvar;          /* 해당 구문의 host 변수 개수를 갖는다. */
    acp_uint16_t mNumofOutHostvar;
    acp_uint16_t mNumofExtraHostvar;
    acp_bool_t   mIsScrollCursor;    /* Scrollable Cusor를 선언하면 true값을 갖는다.   *
                                      * 이값을 보고 추가적인 statement 속성을 세팅해준다. */
    acp_bool_t   mIsArray;           /* column-wise binding이 필요한지 여부,         *
                                      * 모든 host 변수의 타입이 array일 경우 참이 된다. */
    acp_uint32_t mArraySize;         /* array size값.  */
    acp_bool_t   mIsStruct;          /* row-wise binding이 필요한지 여부. */
    acp_uint32_t mStructSize;        /* size of struct*/
    acp_bool_t   mIsAtomic;

    acp_bool_t   mCursorScrollable;
    acp_bool_t   mCursorSensitivity;
    acp_bool_t   mCursorWithHold;

} ulpStmtBindInfo;


/* binding된 host variable/indicator pointer를 저장하는 자료구조. */
typedef struct  ulpHostVarPtr
{
    void*        mHostVar;           /* host변수 값*/
    /* BUG-27833 :
        변수가 다르더라도 포인터가 같을수 있기 때문에 type까지 비교해야함. */
    acp_sint16_t mHostVarType;       /* host변수 type*/
    void*        mHostInd;           /* host변수의 indicator 값*/

} ulpHostVarPtr;

/* BUG-45779 APRE에서 PSM 인자로 Array type을 지원해야 합니다. */
#define APRE_PSM_ARRAY_META_VERSION 808377001

/* BUG-45779 server와의 통신을 위한 meta정의이며, 전체 size가  8의 배수여야 한다. */
typedef struct psmArrayMetaInfo
{
    acp_uint32_t    mUlpVersion;            /* VERSION                       */
    acp_sint32_t    mUlpSqltype;            /* SQL TYPE                      */
    acp_uint32_t    mUlpElemCount;          /* from client to server         */
    acp_uint32_t    mUlpReturnElemCount;    /* from server to client         */
    acp_uint32_t    mUlpHasNull;            /* out parameter has null        */
    acp_uint8_t     mUlpPadding[20];        /* padding & reserved            */
} psmArrayMetaInfo;

typedef struct ulpPSMArrDataInfo
{
    void         *mData;            /* Meta Data + Host Variable Data */
    ulpHostVar   *mUlpHostVar;      /* Host Variable Point address */
} ulpPSMArrDataInfo;

typedef struct ulpPSMArrInfo
{
    acp_bool_t         mIsPSMArray;             /* PSM이 array type 인자를 가지고 있는지? */
    acp_list_t         mPSMArrDataInfoList;     /* ulpPSMArrDataInfo List */
} ulpPSMArrInfo;

/* statement에 대한 상태 정보 type */
typedef enum ulpStmtState
{
    S_INITIAL = 0,
    S_PREPARE,
    S_BINDING,
    S_SETSTMTATTR,
    S_EXECUTE,
    S_CLOSE
} ulpStmtState;


/* cursor에 대한 상태 정보 type */
typedef enum ulpCurState
{
    C_INITIAL = 0,
    C_DECLARE,
    C_OPEN,
    C_FETCH,
    C_CLOSE
} ulpCurState;


/*
 * statement/cursor hash table에서 관리되고 있는 node.
 */
typedef struct ulpLibStmtNode ulpLibStmtNode;
struct ulpLibStmtNode
{
    SQLHSTMT      mHstmt;             /* statement handle */
    /* statement name ( PREPARE, DECLARE STATEMENT구문 수행시 이름) */
    acp_char_t    mStmtName[MAX_STMT_NAME_LEN];
    acp_char_t    mCursorName[MAX_CUR_NAME_LEN];  /* cursor에 대한 이름값을 갖는다. */
    /* BUG-30789: Memory usage per prepared statement is too BIG. (>300k) */
    acp_char_t   *mQueryStr;                      /* query구문 저장 */
    acp_sint32_t  mQueryStrLen;

    ulpStmtState       mStmtState;    /* statement에 대한 상태정보를 갖는다.*/
    ulpCurState        mCurState;     /* cursor에 대한 상태정보를 갖는다.   */
    ulpStmtBindInfo    mBindInfo;     /* row/column-wise bind에대한 정보가 저장된다. */

    /* host 변수 정보를 가리키는 포인터   */
    ulpHostVarPtr     *mInHostVarPtr;
    ulpHostVarPtr     *mOutHostVarPtr;
    ulpHostVarPtr     *mExtraHostVarPtr; /* INOUT, PSM OUT type일경우 저장됨. */

    SQLUINTEGER  mRowsFetched;
     /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
    acp_bool_t   mNeedReprepare; /* failover성공후 reprepare되게 하기위한 flag */

    ulpSqlca    *mSqlca;
    ulpSqlcode  *mSqlcode;
    ulpSqlstate *mSqlstate;

    ulpLibStmtNode *mNextStmt;           /* stmt hash table의 버켓 list link   */
    ulpLibStmtNode *mNextCur;            /* cursor hash table의 버켓 list에 link */

    /* BUG-31467 : APRE should consider the thread safety of statement */
    /* unnamed statement에 대한 thread-safty 고려 */
    acp_thr_rwlock_t mLatch;

    /* BUG-45779 */
    ulpPSMArrInfo    mUlpPSMArrInfo;
};


typedef struct  ulpLibStmtBucket
{
    acp_thr_rwlock_t mLatch;
    ulpLibStmtNode  *mList;

} ulpLibStmtBUCKET;


typedef struct  ulpLibStmtHashTab
{
    acp_sint32_t  mSize    ;                /* Max number of elements in table */
    acp_sint32_t  mNumNode ;                /* number of elements currently in table */
    acp_uint32_t  (*mHash) (acp_uint8_t *);       /* hash function */
    acp_sint32_t  (*mCmp ) (
                    const acp_char_t*,
                    const acp_char_t*,
                    acp_size_t );       /* comparison funct */
    ulpLibStmtBUCKET mTable[MAX_NUM_STMT_HASH_TAB_ROW];  /* actual hash table    */

} ulpLibStmtHASHTAB;


typedef struct  ulpLibStmtList
{
    acp_sint32_t        mSize;
    acp_sint32_t        mNumNode;
    acp_thr_rwlock_t    mLatch;
    ulpLibStmtNode     *mList;

} ulpLibStmtLIST;


/*
 * statement/cursor hash table managing functions.
 */

/* 새로운 statement node를 만든다. */
ulpLibStmtNode* ulpLibStNewNode(ulpSqlstmt* aSqlstmt, acp_char_t *aStmtName );

/* stmt hash table에서 해당 stmt이름으로 stmt node를 찾는다. */
ulpLibStmtNode* ulpLibStLookupStmt( ulpLibStmtHASHTAB *aStmtHashT,
                                    acp_char_t *aStmtName );

/* stmt hash table에 해당 stmt이름으로 stmt node를 추가한다. List의 제일 뒤에 매단다. */
ulpLibStmtNode* ulpLibStAddStmt( ulpLibStmtHASHTAB *aStmtHashT,
                                 ulpLibStmtNode *aStmtNode );

/* stmt hash table의 모든 statement를 제거한다.                                   *
 * Prepare나 declare statement에 의해 생성된 stmt node들은 disconnect될때 해제된다. */
/* BUG-26370 [valgrind error] :  cursor hash table을 해제해야한다. */
void ulpLibStDelAllStmtCur( ulpLibStmtHASHTAB *aStmtTab,
                            ulpLibStmtHASHTAB *aCurTab );

/* cursor hash table에서 해당 cursor이름으로 cursor node를 찾는다. */
ulpLibStmtNode* ulpLibStLookupCur( ulpLibStmtHASHTAB *aCurHashT, acp_char_t *aCurName );

/* cursor hash table에 해당 cursor이름을 갖는 cursor node를 제거한다.                *
 * Statement 이름이 있은경우 link만 제거하며, 없는 경우에는 utpStmtNode 객체를 제거한다. */
ACI_RC ulpLibStDeleteCur( ulpLibStmtHASHTAB *aCurHashT, acp_char_t *aCurName );

/* cursor hash table의 bucket list 마지막에 해당 stmt node에 대한 새link를 연결한다. */
ACI_RC ulpLibStAddCurLink( ulpLibStmtHASHTAB *aCurHashT,
                           ulpLibStmtNode *aStmtNode,
                           acp_char_t *aCurName );

/* unnamed stmt list에서 해당 query로 stmt node를 찾는다. */
ulpLibStmtNode *ulpLibStLookupUnnamedStmt( ulpLibStmtLIST *aStmtList,
                                           acp_char_t *aQuery );

/* unnamed stmt list에 새 stmt node를 추가한다. */
ACI_RC ulpLibStAddUnnamedStmt( ulpLibStmtLIST *aStmtList,
                               ulpLibStmtNode *aStmtNode );

/* unnamed stmt list에 매달린 모든 stmt node들을 제거한다. */
void ulpLibStDelAllUnnamedStmt( ulpLibStmtLIST *aStmtList );


#endif
