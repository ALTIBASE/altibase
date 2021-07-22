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

#ifndef _O_ULN_CURSOR_H_
#define _O_ULN_CURSOR_H_ 1

#include <ulnPrivate.h>

/*
 * Note : Cursor 는 "cursor" 일 뿐 아무것도 아니다.
 *        다시 말해서, 근본적으로, 커서는 "포인터" 이다.
 *        거기 더해서 stmt 의 cursor 와 관련된 속성을 가질 따름이다.
 */

typedef enum
{
    ULN_CURSOR_STATE_CLOSED,
    ULN_CURSOR_STATE_OPEN
} ulnCursorState;

typedef enum
{
    ULN_CURSOR_DIR_NEXT =  1,
    ULN_CURSOR_DIR_PREV = -1,

    ULN_CURSOR_DIR_MAX
} ulnCursorDir;

struct ulnCursor
{
    ulnStmt        *mParentStmt;
    acp_sint64_t    mPosition;   // To Fix BUG-20482
    ulnCursorDir    mDirection;  /* PROJ-1789 Updatable Scrollable Cursor */
    ulnCursorState  mState;
    ulnCursorState  mServerCursorState; /* Note : 서버 커서의 상태
                                           ColumnCount > 0 인 statement 를 Execute 한 직후 -> 열림.
                                           Fetch 해서 서버 result set 의 끝까지 읽었음 -> 닫힘.
                                           서버 커서 상태와 관계없이 CLOSE CURSOR REQ 를 서버로
                                           보내도 된다. 닫힌 상태에서 전송해도 그냥 서버에서
                                           무시한다. 이 변수의 역할은 단지 한번이라도
                                           I/O transaction 을 줄이기 위한것임 */

    /*
     * STMT 의 cursor 와 관련된 attribute 들
     */
    acp_uint32_t    mAttrCursorSensitivity; /* SQL_ATTR_CURSOR_SENSITIVITY */
    acp_uint32_t    mAttrCursorType;        /* SQL_ATTR_CURSOR_TYPE */
    acp_uint32_t    mAttrCursorScrollable;  /* SQL_ATTR_CURSOR_SCROLLABLE */
    acp_uint32_t    mAttrSimulateCursor;    /* SQL_ATTR_SIMULATE_CURSOR */
    acp_uint32_t    mAttrConcurrency;       /* SQL_ATTR_CONCURRENCY */
    acp_uint32_t    mAttrHoldability;       /* SQL_ATTR_CURSOR_HOLD */

    /* PROJ-1789 Updatable Scrollable Cursor */
    acp_uint16_t    mRowsetPos;             /**< rowset내에서의 position. SetPos를 통해 설정. */
};

void   ulnCursorInitialize(ulnCursor *aCursor, ulnStmt *aParentStmt);

ACI_RC ulnCursorClose(ulnFnContext *aFnContext, ulnCursor *aCursor);
void   ulnCursorOpen(ulnCursor *aCursor);

ACP_INLINE ulnCursorState ulnCursorGetState(ulnCursor *aCursor)
{
    return aCursor->mState;
}

ACP_INLINE void ulnCursorSetState(ulnCursor *aCursor, ulnCursorState aNewState)
{
    aCursor->mState = aNewState;
}

ACP_INLINE void ulnCursorSetServerCursorState(ulnCursor *aCursor, ulnCursorState aState)
{
    aCursor->mServerCursorState = aState;
}

ACP_INLINE ulnCursorState ulnCursorGetServerCursorState(ulnCursor *aCursor)
{
    return aCursor->mServerCursorState;
}

ACP_INLINE void ulnCursorSetType(ulnCursor *aCursor, acp_uint32_t aType)
{
    aCursor->mAttrCursorType = aType;
}

ACP_INLINE acp_uint32_t ulnCursorGetType(ulnCursor *aCursor)
{
    return aCursor->mAttrCursorType;
}

acp_uint32_t ulnCursorGetSize(ulnCursor *aCursor);

ACP_INLINE acp_sint64_t ulnCursorGetPosition(ulnCursor *aCursor)
{
    return aCursor->mPosition;
}

void         ulnCursorSetPosition(ulnCursor *aCursor, acp_sint64_t aPosition);

void         ulnCursorSetConcurrency(ulnCursor *aCursor, acp_uint32_t aConcurrency);
acp_uint32_t ulnCursorGetConcurrency(ulnCursor *aCursor);

ACP_INLINE void ulnCursorSetScrollable(ulnCursor *aCursor, acp_uint32_t aScrollable)
{
    aCursor->mAttrCursorScrollable = aScrollable;
}

ACP_INLINE acp_uint32_t ulnCursorGetScrollable(ulnCursor *aCursor)
{
    return aCursor->mAttrCursorScrollable;
}

void         ulnCursorSetSensitivity(ulnCursor *aCursor, acp_uint32_t aSensitivity);
acp_uint32_t ulnCursorGetSensitivity(ulnCursor *aCursor);

/*
 * 몇개의 row 를 사용자 버퍼로 복사해야 하는지 계산하는 함수
 */
acp_uint32_t ulnCursorCalcRowCountToCopyToUser(ulnCursor *aCursor);

/*
 * 커서의 위치 선정과 관련된 함수들
 */
void ulnCursorMovePrior(ulnFnContext *aFnContext, ulnCursor *aCursor);
void ulnCursorMoveNext(ulnFnContext *aFnContext, ulnCursor *aCursor);
void ulnCursorMoveRelative(ulnFnContext *aFnContext, ulnCursor *aCursor, acp_sint32_t aOffset);
void ulnCursorMoveAbsolute(ulnFnContext *aFnContext, ulnCursor *aCursor, acp_sint32_t aOffset);
void ulnCursorMoveFirst(ulnCursor *aCursor);
void ulnCursorMoveLast(ulnCursor *aCursor);

#define ULN_CURSOR_POS_BEFORE_START     (-1)
#define ULN_CURSOR_POS_AFTER_END        (-2)

/* PROJ-1381 Fetch Across Commit */

/**
 * Cursor Hold 속성을 얻는다.
 *
 * @param[in] aCursor cursor object
 *
 * @return Holdable이면 SQL_CURSOR_HOLD_ON, 아니면 SQL_CURSOR_HOLD_OFF
 */
ACP_INLINE acp_uint32_t ulnCursorGetHoldability(ulnCursor *aCursor)
{
    return aCursor->mAttrHoldability;
}

/**
 * Cursor Hold 속성을 설정한다.
 *
 * @param[in] aCursor      cursor object
 * @param[in] aHoldability holdablility. SQL_CURSOR_HOLD_ON or SQL_CURSOR_HOLD_OFF.
 */
ACP_INLINE void ulnCursorSetHoldability(ulnCursor *aCursor, acp_uint32_t aHoldability)
{
    aCursor->mAttrHoldability = aHoldability;
}


/* PROJ-1789 Updatable Scrollable Cursor */

void         ulnCursorMoveByBookmark(ulnFnContext *aFnContext, ulnCursor *aCursor, acp_sint32_t aOffset);
ulnCursorDir ulnCursorGetDirection(ulnCursor *aCursor);
void         ulnCursorSetDirection(ulnCursor *aCursor, ulnCursorDir aDirection);

/**
 * rowset position을 얻는다.
 *
 * @param[in] aCursor cursor object
 *
 * @return rowset position
 */
ACP_INLINE acp_uint16_t ulnCursorGetRowsetPosition(ulnCursor *aCursor)
{
    return aCursor->mRowsetPos;
}


/**
 * rowset position을 설정한다.
 *
 * @param[in] aCursor    cursor object
 * @param[in] aRowsetPos rowset position
 */
ACP_INLINE void ulnCursorSetRowsetPosition(ulnCursor *aCursor, acp_uint16_t aRowsetPos)
{
    aCursor->mRowsetPos = aRowsetPos;
}

/* BUG-46100 Session SMN Update */
acp_bool_t ulnCursorHasNoData(ulnCursor *aCursor);

#endif /* _O_ULN_CURSOR_H_ */
