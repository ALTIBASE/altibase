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
 * $Id: qsxLibrary.h 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#ifndef _O_QSX_LIBRARY_H_
#define _O_QSX_LIBRARY_H_ 1

#include <qs.h>
#include <idx.h>

#define QSX_LIBRARY_TIME_STRING_SIZE 48
#define QSX_LIBRARY_HASH_POOL_SIZE   64
#define QSX_LIBRARY_HASH_KEY_SIZE    64

#define QSX_INIT_LIBRARY( _libNode_ )                 \
{                                                     \
(_libNode_)->mRefCount    = 0;                        \
(_libNode_)->mHandle      = PDL_SHLIB_INVALID_HANDLE; \
(_libNode_)->mFunctionPtr = NULL;                     \
(_libNode_)->mFileSize    = 0;                        \
(_libNode_)->mOpenTime    = 0;                        \
(_libNode_)->mCreateTime  = 0;                        \
}

// PROJ-2717 Internal procedure
typedef struct qsxLibraryNode
{
    PDL_SHLIB_HANDLE        mHandle;
    void                  * mFunctionPtr;

    SChar                   mLibPath[QSX_LIBRARY_HASH_KEY_SIZE];

    SInt                    mRefCount;
    UInt                    mFileSize;

    iduMutex                mMutex;

    time_t                  mCreateTime;
    time_t                  mOpenTime;
} qsxLibraryNode;

// PROJ-2717 Internal procedure
//   for X$LIBRARY
typedef struct qsLibraryRecord
{
    SChar                 * mLibPath;
    SInt                    mRefCount;
    UInt                    mFileSize;

    SChar                   mCreateTimeString[QSX_LIBRARY_TIME_STRING_SIZE];
    SChar                   mOpenTimeString[QSX_LIBRARY_TIME_STRING_SIZE];
} qsLibraryRecord;

class qsxLibrary 
{
public:
static IDE_RC initializeStatic();

static void finalizeStatic();

static IDE_RC loadLibrary( qsxLibraryNode ** aLibraryNode,
                           void           ** aFunctionPtr,
                           SChar           * aLibPath,
                           SChar           * aFunctionName );

static IDE_RC unloadLibrary( qsxLibraryNode * aLibraryNode,
                             void          ** aFunctionPtr );

static IDE_RC callintProc( iduMemory      * aExeMem,
                           idxIntProcMsg  * aMsg,
                           void           * aArg );

static IDE_RC buildRecordForLibrary( idvSQL * /* aStatistics */,
                                     void   *  aHeader,
                                     void   * /* aDumpObj */,
                                     iduFixedTableMemory * aMemory );
private:
static IDE_RC findLibraryFromHashTable( SChar           * aLibPath,
                                        qsxLibraryNode ** aLibNode );

static IDE_RC makeAndAddLibraryToHashTable( SChar           * aLibPath,
                                            qsxLibraryNode ** aLibraryNode );

static IDE_RC addLibraryToHashTable( qsxLibraryNode * aLibraryNode,
                                     idBool         * aAlreadyExist );

static void getTimeString( SChar * aBuf, time_t aTime );
};

#endif
