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
 * $Id$ sdlStatement.h
 **********************************************************************/

#ifndef _O_SDL_STATEMENT_H_
#define _O_SDL_STATEMENT_H_ 1

#include <idu.h>

/* +-----------------+
 * | sdiStatement    |    +------------------+       +------------------+
 * |  - mNodeListHead+----+ sdlRemoteNode    +-------+ sdlRemoteNode    +----...
 * +-----------------+    |  - mNodeId       |       |  - mNodeId       |
 *                        |  - mStmtListHead +---+   |  - mStmtListHead +----...
 *                        +------------------+   |   +------------------+
 *                                               |                
 *                                               |   +---------------+    +---------------+         
 *                                               +---+ sdlRemoteStmt +----+ sdlRemoteStmt +----...
 *                                                   |  - mStmt      |    |  - mStmt      |
 *                                                   |  - mState     |    |  - mState     |
 *                                                   +---------------+    +---------------+
 */

/******************************************************************************
 * sdlFreeFlag definition
 * ***************************************************************************/
#define SDL_STMT_FREE_FLAG_IN_USE_MASK              (0x00000001)
#define SDL_STMT_FREE_FLAG_IN_USE_FALSE             (0x00000000)
#define SDL_STMT_FREE_FLAG_IN_USE_TRUE              (0x00000001)

#define SDL_STMT_FREE_FLAG_ALLOCATED_MASK           (0x00000010)
#define SDL_STMT_FREE_FLAG_ALLOCATED_FALSE          (0x00000000)
#define SDL_STMT_FREE_FLAG_ALLOCATED_TRUE           (0x00000010)

#define SDL_STMT_FREE_FLAG_TRY_BIND_PARAM_MASK      (0x00000020)
#define SDL_STMT_FREE_FLAG_TRY_BIND_PARAM_FALSE     (0x00000000)
#define SDL_STMT_FREE_FLAG_TRY_BIND_PARAM_TRUE      (0x00000020)

#define SDL_STMT_FREE_FLAG_TRY_EXECUTE_MASK         (0x00000040)
#define SDL_STMT_FREE_FLAG_TRY_EXECUTE_FALSE        (0x00000000)
#define SDL_STMT_FREE_FLAG_TRY_EXECUTE_TRUE         (0x00000040)

#define SDL_STMT_FREE_FLAG_DROP_MASK                (0x00000070)    /* ALLOCATED+BIND_PARAM+EXECUTE */
#define SDL_STMT_FREE_FLAG_DROP                     (0x00000000)

typedef UInt sdlFreeFlag;

/******************************************************************************
 * sdlRemoteStmt definition
 * ***************************************************************************/
struct sdlRemoteStmt
{
    iduListNode    mListNode;
    void         * mStmt;
    sdlFreeFlag    mFreeFlag;
    UInt           mRemoteStmtId;
};

typedef struct sdlRemoteNode sdlRemoteNode;

/******************************************************************************
 * sdlRemoteNode definition
 * ***************************************************************************/
struct sdlRemoteNode
{
    iduListNode    mListNode;
    UInt           mNodeId;
    UInt           mCurRemoteStmtId;
    iduList        mStmtListHead;
};

typedef struct sdlRemoteStmt sdlRemoteStmt;

#endif  // _O_SDL_STATEMENT_H_
