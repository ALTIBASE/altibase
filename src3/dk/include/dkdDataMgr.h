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
 * $id$
 **********************************************************************/

#ifndef _O_DKD_DATA_MGR_H_
#define _O_DKD_DATA_MGR_H_ 1


#include <dkd.h>
#include <dkdRecordBufferMgr.h>
#include <dkdDiskTempTableMgr.h>
#include <dkdDataBufferMgr.h>
#include <dkdTypeConverter.h>
#include <dkpDef.h>

typedef enum
{
    DKD_DATA_MGR_TYPE_DISK,
    DKD_DATA_MGR_TYPE_MEMORY
} dkdDataMgrType;

typedef IDE_RC  (* dkdFetchRowFunc)( void *aMgrHandle, void **aRow );
typedef IDE_RC  (* dkdInsertRowFunc)( void *aMgrHandle, dkdRecord *aRecord );
typedef IDE_RC  (* dkdRestartFunc)( void *aMgrHandle );


class dkdDataMgr
{
private:
    /* 현재 iteration 해야 할 data 가 record buffer 에 존재하는지 disk temp
       table 에 존재하는지 여부 */
    idBool                mIsRecordBuffer;
    /* 마지막 data block 을 다 읽었고 readRecordDataFromProtocol 의 결과
       DK_RC_EOF 인 경우 TRUE 로 설정 */
    idBool                mIsEndOfFetch;
    /* 전체 record 의 갯수, insertRow 가 성공할 때마다 1씩 증가 */
    UInt                  mRecordCnt;
    /* Converted record 의 길이 */
    UInt                  mRecordLen;
    /* Converted Row's size */
    UInt                  mRowSize;
    /* Disk temp table 을 위해 동적 할당받은 record catridge */
    dkdRecord           * mRecord;
    /* Record buffer 를 사용하는 경우 가용 record buffer 의 최대크기 */
    UInt                  mRecordBufferSize;
    /* AltiLinker 프로세스로부터 packet 을 하나 받을 때마다 증가하는 
       data block 의 count. 추후 통계치 확보에도 사용될 수 있다. */
    UInt                  mDataBlockRecvCount;
    /* Data manager 가 읽은 data block 의 count. */
    UInt                  mDataBlockReadCount;
    /* 현재 data block 에서 가리키고 있는 지점 */
    SChar               * mDataBlockPos;

    struct dkdTypeConverter    * mTypeConverter;

    iduMemAllocator     * mAllocator;
    dkdRecordBufferMgr  * mRecordBufferMgr;
    dkdDiskTempTableMgr * mDiskTempTableMgr;

    void                * mMgrHandle;

    void                * mCurRow; /* fetch 한 row 를 일시적으로 보관 */

    dkdFetchRowFunc       mFetchRowFunc;
    dkdInsertRowFunc      mInsertRowFunc;
    dkdRestartFunc        mRestartFunc;

    dkdDataMgrType        mDataMgrType;

    UInt                  mFetchRowCount;
    
    void                * mNullRow;

    idBool                mFlagRestartOnce;
    
public:
    IDE_RC      initialize( UInt  *aBuffSize );
     /* PROJ-2417 */
    IDE_RC      initializeRecordBuffer();
    IDE_RC      activate( void  *aQcStatement );
    /* BUG-37487 */
    void        finalize();
    IDE_RC      getRecordLength();
    IDE_RC      getRecordBufferSize();

    /* Operations */
    IDE_RC      moveNextRow( idBool *aEndFlag );
    IDE_RC      fetchRow( void  **aRow );
    IDE_RC      insertRow( void  *aRow, void *aQcStatement );
    IDE_RC      restart();
    IDE_RC      restartOnce( void );

    /* Record buffer manager */
    IDE_RC      createRecordBufferMgr( UInt  aAllocableBlockCnt );
    /* BUG-37487 */
    void        destroyRecordBufferMgr();

    /* Disk temp table manager */
    IDE_RC      createDiskTempTableMgr( void  *aQcStatement );
    /* BUG-37487 */
    void        destroyDiskTempTableMgr();

    /* Data converter */
    IDE_RC      createTypeConverter( dkpColumn   *aColMetaArr, 
                                     UInt         aColCount );
    IDE_RC      destroyTypeConverter();

    IDE_RC      getConvertedMeta( mtcColumn **aMeta );

    /* Switch from record buffer manager to disk temp table manager */
    void        switchToDiskTempTable(); /* BUG-37487 */
    IDE_RC      moveRecordToDiskTempTable();

    /* inline functions */
    inline dkdTypeConverter *   getTypeConverter();
    inline void *   getColValue( UInt   aOffset );

    inline void     setEndOfFetch( idBool   aIsEndOfFetch );
    inline idBool   getEndOfFetch();
    inline idBool   isRemainedRecordBuffer();

    IDE_RC getNullRow( void ** aRow, scGRID * aRid );
    idBool isNeedFetch( void );
    
    inline idBool   isFirst();
};


inline dkdTypeConverter *   dkdDataMgr::getTypeConverter()
{
    return mTypeConverter;
}

inline void *   dkdDataMgr::getColValue( UInt   aOffset )
{
    return (void *)((SChar *)mCurRow + aOffset);
}

inline void dkdDataMgr::setEndOfFetch( idBool   aIsEndOfFetch )
{
    mIsEndOfFetch = aIsEndOfFetch;
}

inline idBool   dkdDataMgr::getEndOfFetch()
{
    return mIsEndOfFetch;
}

inline idBool   dkdDataMgr::isRemainedRecordBuffer()
{
    return ( mRecordBufferSize - ( mRecordCnt * mRecordLen ) >= mRecordLen ) 
        ? ID_TRUE 
        : ID_FALSE;
}

inline idBool   dkdDataMgr::isFirst()
{
    idBool  sResult = ID_FALSE;

    if ( mFetchRowCount == 0 )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

#endif /* _O_DKD_DATA_MGR_H_ */

