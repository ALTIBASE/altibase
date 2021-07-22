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

#ifndef _O_ULN_OBJECT_H_
#define _O_ULN_OBJECT_H_ 1

#include <ulnPrivate.h>
#include <uluLock.h>

/*
 * uln에서 사용되는 객체들의 타입들.
 */
typedef enum
{
    ULN_OBJ_TYPE_MIN  = SQL_HANDLE_ENV - 1,

    /*
     * SQL_HANDLE_xxx와 맞추기 위해서 1부터 시작함.
     */

    ULN_OBJ_TYPE_ENV  = SQL_HANDLE_ENV,
    ULN_OBJ_TYPE_DBC  = SQL_HANDLE_DBC,
    ULN_OBJ_TYPE_STMT = SQL_HANDLE_STMT,
    ULN_OBJ_TYPE_DESC = SQL_HANDLE_DESC,

    ULN_OBJ_TYPE_SQL,

    ULN_OBJ_TYPE_MAX

} ulnObjType;

typedef enum
{
    ULN_DESC_TYPE_NODESC,
    ULN_DESC_TYPE_APD,
    ULN_DESC_TYPE_ARD,
    ULN_DESC_TYPE_IRD,
    ULN_DESC_TYPE_IPD,
    ULN_DESC_TYPE_MAX
} ulnDescType;

#define ULN_OBJ_GET_TYPE(x)         ( (ulnObjType)(((ulnObject *)(x))->mType) )
#define ULN_OBJ_SET_TYPE(x, a)      ( ((ulnObject *)(x))->mType = (a) )

#define ULN_OBJ_GET_DESC_TYPE(x)    ( (ulnDescType)(((ulnObject *)(x))->mDescType) )
#define ULN_OBJ_SET_DESC_TYPE(x, a) ( (((ulnObject *)(x))->mDescType) = (a))

#define ULN_OBJ_GET_STATE(x)        ( (acp_uint16_t)(((ulnObject *)(x))->mState) )
#define ULN_OBJ_SET_STATE(x, a)     ( ((ulnObject *)(x))->mState = (a) )


#define ULN_OBJ_MALLOC(aObj, aPP, aSize)                                    \
    ((ulnObject*)aObj)->mMemory->mOp->mMalloc(((ulnObject*)aObj)->mMemory,  \
                                              ((void**) aPP),               \
                                              aSize)

/*
 * BUGBUG: lock과 관련된 파일로 이동되어야 한다 {{{
 */
typedef enum
{
    ULN_MUTEX,  /* Synchronize by PDL mutex     */
    ULN_CONDV   /* Synchronize by Cond Variable */
} ulnLockType;

/*
 * Header portion of following Objects :
 *      ulnEnv
 *      ulnDbc
 *      ulnDbd
 *      ulnStmt
 *      ulnDesc
 */
struct ulnObject
{
    acp_list_node_t      mList;

    /* BUG-38755 Improve to check invalid handle in the CLI */
    volatile ulnObjType  mType;        /* Object Type */
    ulnDescType          mDescType;    /* Descriptor Type */

    acp_sint16_t         mState;

    /*
     * Statement ulnDescStmt 의 S11, S12 스테이트에서
     * NS 라는 상태전이가 있는데 이에 해당하는 state
     * 정보를 가지고 있어야 할 필요가 있음에 만든 필드임.
     * 이 필드가 0 이 아니면 이 STMT 에 대해 불리워진
     * 함수가 실행되고 있는 중이라는 것을 나타냄.
     */
    ulnFunctionId    mExecFuncID;

    uluChunkPool    *mPool;
    uluMemory       *mMemory;

    acp_thr_mutex_t *mLock;              /* Lock Data Pointer */

    ulnDiagHeader    mDiagHeader;        /* Header of Diagnostic Messages */

    /*
     * 아래의 멤버는 SQLError() 함수의 SQLGetDiagRec() 함수로의 매핑때문에 존재한다.
     * uloSqlWrapper.cpp 의 SQLError() 함수를 보면 사용례를 알 수 있다.
     */
    acp_sint32_t     mSqlErrorRecordNumber;
};

#define ULN_OBJECT_LOCK(aObject, aFuncID)   \
    acpThrMutexLock((aObject)->mLock)

#define ULN_OBJECT_UNLOCK(aObject, aFuncID)     \
    do                                          \
    {                                           \
        acpThrMutexUnlock((aObject)->mLock);    \
        ACP_UNUSED(aFuncID);                    \
    } while (0)

/*
 * Function Declarations
 */
ACI_RC ulnObjectInitialize(ulnObject    *aObject,
                           ulnObjType    aType,
                           ulnDescType   aSubType,
                           acp_sint16_t  aState,
                           uluChunkPool *aPool,
                           uluMemory    *aMemory);

#endif /* _ULN_OBJECT_H_ */

