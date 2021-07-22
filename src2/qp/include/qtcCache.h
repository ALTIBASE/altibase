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
 *
 *      PROJ-2448 Subquery Caching 
 *      PROJ-2452 Caching for DETERMINISTIC Function
 *
 **********************************************************************/

#ifndef _O_QTC_CACHE_H_
#define _O_QTC_CACHE_H_ 1

#include <qc.h>
#include <qtcHash.h>
#include <qsParseTree.h>

// qtcCacheObj.flag
#define QTC_CACHE_HASH_MASK       (0x00000001)
#define QTC_CACHE_HASH_ENABLE     (0x00000000)
#define QTC_CACHE_HASH_DISABLE    (0x00000001)

#define QTC_IS_CACHE_TYPE( aModule )             \
   (                                             \
       ( ( (aModule)->id == MTD_BIGINT_ID )   || \
         ( (aModule)->id == MTD_BOOLEAN_ID )  || \
         ( (aModule)->id == MTD_CHAR_ID )     || \
         ( (aModule)->id == MTD_DATE_ID )     || \
         ( (aModule)->id == MTD_DOUBLE_ID )   || \
         ( (aModule)->id == MTD_FLOAT_ID )    || \
         ( (aModule)->id == MTD_INTEGER_ID )  || \
         ( (aModule)->id == MTD_NUMBER_ID )   || \
         ( (aModule)->id == MTD_NUMERIC_ID )  || \
         ( (aModule)->id == MTD_REAL_ID )     || \
         ( (aModule)->id == MTD_SMALLINT_ID ) || \
         ( (aModule)->id == MTD_VARCHAR_ID )  || \
         ( (aModule)->id == MTD_NCHAR_ID )    || \
         ( (aModule)->id == MTD_NVARCHAR_ID ) || \
         ( (aModule)->id == MTD_ECHAR_ID )    || \
         ( (aModule)->id == MTD_EVARCHAR_ID )    \
       ) ? ID_TRUE : ID_FALSE                    \
   )

#define QTC_CACHE_INIT_CACHE_OBJ( aCacheObj, aCacheMaxSize )   \
{                                                              \
    (aCacheObj)->mCurrRecord = NULL;                           \
    (aCacheObj)->mHashTable  = NULL;                           \
    (aCacheObj)->mType       = QTC_CACHE_TYPE_NONE;            \
    (aCacheObj)->mState      = QTC_CACHE_STATE_BEGIN;          \
    (aCacheObj)->mRemainSize = aCacheMaxSize;                  \
    (aCacheObj)->mParamCnt   = 0;                              \
    (aCacheObj)->mHitCnt     = 0;                              \
    (aCacheObj)->mMissCnt    = 0;                              \
    (aCacheObj)->mFlag &= ~QTC_CACHE_HASH_MASK;                \
    (aCacheObj)->mFlag |=  QTC_CACHE_HASH_ENABLE;              \
}

#define QTC_CACHE_SET_STATE( aCacheObj, aState )   \
{                                                  \
    IDE_DASSERT( aCacheObj != NULL );              \
    (aCacheObj)->mState = aState;                  \
}

typedef enum
{
    QTC_CACHE_FORCE_FUNCTION_NONE          = 0,  // no force
    QTC_CACHE_FORCE_FUNCTION_CACHE         = 1,  // force cache of basic function
    QTC_CACHE_FORCE_FUNCTION_DETERMINISTIC = 2   // force DETERMINISTIC of basic function
} qtcCacheForceFunc;

typedef enum
{
    QTC_CACHE_FORCE_SUBQUERY_CACHE_NONE    = 0,  // no force cache disable of subquery
    QTC_CACHE_FORCE_SUBQUERY_CACHE_DISABLE = 1,  // force cache disable of subquery
} qtcCacheForceSubq;

typedef enum
{
    QTC_CACHE_STATE_BEGIN              = 0,
    QTC_CACHE_STATE_SEARCH_HASH_TABLE  = 1,
    QTC_CACHE_STATE_RETURN_INVOKE      = 2,
    QTC_CACHE_STATE_RETURN_CURR_RECORD = 3,
    QTC_CACHE_STATE_INVOKE_MAKE_RECORD = 4,
    QTC_CACHE_STATE_MAKE_HASH_TABLE    = 5,
    QTC_CACHE_STATE_INSERT_HASH_RECORD = 6,
    QTC_CACHE_STATE_END                = 7
} qtcCacheState;

typedef enum
{
    QTC_CACHE_TYPE_NONE                    = 0,
    QTC_CACHE_TYPE_DETERMINISTIC_FUNCTION  = 1,
    QTC_CACHE_TYPE_SCALAR_SUBQUERY_SURE    = 2,
    QTC_CACHE_TYPE_SCALAR_SUBQUERY_UNKNOWN = 3,
    QTC_CACHE_TYPE_LIST_SUBQUERY_SURE      = 4,
    QTC_CACHE_TYPE_LIST_SUBQUERY_TWICE     = 5,
    QTC_CACHE_TYPE_EXISTS_SUBQUERY         = 6,
    QTC_CACHE_TYPE_NOT_EXISTS_SUBQUERY     = 7
} qtcCacheType;

typedef struct qtcCacheObj
{
    qtcHashRecord * mCurrRecord;    // latest performed record
    qtcHashTable  * mHashTable;     // hash table for caching
    qtcCacheType    mType;          // cache type
    qtcCacheState   mState;         // cache state
    UInt            mRemainSize;    // remain size for caching
    UInt            mParamCnt;      // parameter count
    UInt            mFlag;          // flag for masking
    UInt            mHitCnt;        // cache hit count
    UInt            mMissCnt;       // cache miss count
} qtcCacheObj;

class qtcCache
{
public:

    /* For Validation */
    static IDE_RC setIsCachableForFunction( qsVariableItems  * aParaDecls,
                                            mtdModule       ** aParamModules,
                                            mtdModule        * aReturnModule,
                                            idBool           * aIsDeterministic,
                                            idBool           * aIsCachable );

    static IDE_RC validateFunctionCache( qcTemplate * aTemplate,
                                         qtcNode    * aNode );

    static IDE_RC validateSubqueryCache( qcTemplate * aTemplate,
                                         qtcNode    * aNode );

    /* For Optimization */
    static IDE_RC preprocessSubqueryCache( qcStatement * aStatement,
                                           qtcNode     * aNode );

    /* For Execution */
    static IDE_RC searchCache( qcTemplate     * aTemplate,
                               qtcNode        * aNode,
                               mtcStack       * aStack,
                               qtcCacheType     aCacheType,
                               qtcCacheObj   ** aCacheObj,
                               UInt           * aParamCnt,
                               qtcCacheState  * aState );

    static IDE_RC executeCache( iduMemory     * aMemory,
                                mtcNode       * aNode,
                                mtcStack      * aStack,
                                qtcCacheObj   * aCacheObj,
                                UInt            aBucketCnt,
                                qtcCacheState   aState,
                                const UInt      aSubqueryMode );

    static IDE_RC getCacheValue( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 qtcCacheObj * aCacheObj,
                                 const UInt    aSubqueryMode );

private:

    /* For Execution */
    static IDE_RC getColumnAndValue( qcTemplate    * aTemplate,
                                     qmsOuterNode  * aOuterColumn,
                                     mtcColumn    ** aColumn,
                                     void         ** aValue );

    static IDE_RC compareCurrRecord( mtcStack    * aStack,
                                     qtcCacheObj * aCacheObj,
                                     idBool      * aResult );

    static IDE_RC searchHashTable( mtcStack    * aStack,
                                   qtcCacheObj * aCacheObj,
                                   idBool      * aResult );

    static IDE_RC allocAndSetCurrRecord( iduMemory   * aMemory,
                                         mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         qtcCacheObj * aCacheObj,
                                         const UInt    aSubqueryMode );

    static IDE_RC resetCurrRecord( iduMemory   * aMemory,
                                   mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   qtcCacheObj * aCacheObj,
                                   const UInt    aSubqueryMode );

    static IDE_RC getKeyAndSize( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 qtcCacheObj * aCacheObj,
                                 UInt        * aKey,
                                 UInt        * aKeyDataSize,
                                 UInt        * aValueSize,
                                 const UInt    aSubqueryMode );

    static IDE_RC setKeyAndSize( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 qtcCacheObj * aCacheObj,
                                 UInt          aKey,
                                 const UInt    aSubqueryMode );
};

#endif /* _O_QTC_CACHE_H_ */
