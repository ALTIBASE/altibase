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

#ifndef _O_DKD_REMOTE_TABLE_MGR_H_
#define _O_DKD_REMOTE_TABLE_MGR_H_ 1


#include <dkd.h>
#include <dkdTypeConverter.h>
#include <dkpDef.h>


class dkdRemoteTableMgr
{
private:
    /* 원격서버로부터 모든 레코드를 다 가져왔는지 여부 */
    idBool                  mIsEndOfFetch;
    /* fetch row buffer 의 레코드들을 다 읽어갔는지 여부 */
    idBool                  mIsFetchBufReadAll;
    /* fetch row buffer 가 다 찼는지 여부 */
    idBool                  mIsFetchBufFull;
    /* Converted Row's size */
    UInt                    mRowSize;
    /* Fetch row buffer's size */
    UInt                    mFetchRowBufSize;
    /* Converted row 가 저장되는 temp buffer's size */
    UInt                    mConvertedRowBufSize;
    /* 한번에 원격서버에서 가져오는 레코드의 개수 */
    UInt                    mFetchRowCnt;
    /* 원격서버에서 가져온 fetch row buffer 에 존재하는 레코드 개수 */
    UInt                    mInsertRowCnt;
    /* 전체 레코드 개수 */
    ULong                   mRecordCnt;
    /* QP 에서 레코드를 가져가기 전 일시적으로 레코드를 보관할 버퍼 */
    SChar                 * mFetchRowBuffer;
    /* Converted 된 row 하나가 저장되는 temp buffer */
    SChar                 * mConvertedRowBuffer;
    /* QP 에서 fetch 를 수행한 후 가리키는 레코드 */
    SChar                 * mInsertPos;    
    /* Type converter */
    struct dkdTypeConverter    * mTypeConverter;
    /* fetch 한 row 를 일시적으로 보관 */
    SChar                 * mCurRow; 

public:
    /* Initialize / Activate / Finalize */
    IDE_RC                  initialize( UInt * aBuffSize );
    IDE_RC                  activate();
    IDE_RC                  makeFetchRowBuffer();
    void                    freeFetchRowBuffer();
    void                    freeConvertedRowBuffer();
    void                    cleanFetchRowBuffer();
    void                    finalize();

    /* Data converter */
    IDE_RC                  createTypeConverter( dkpColumn   *aColMetaArr, 
                                                 UInt         aColCount );
    IDE_RC                  destroyTypeConverter();
    IDE_RC                  getConvertedMeta( mtcColumn **aMeta );
    IDE_RC                  getRecordLength();

    /* Operations */
    void                    fetchRow( void  **aRow );
    IDE_RC                  insertRow( void *aRow );
    IDE_RC                  restart();

    /* Setter / Getter */
    inline void             setEndOfFetch( idBool   aIsEndOfFetch );
    inline idBool           getEndOfFetch();

    inline UInt             getFetchRowSize();
    inline UInt             getFetchRowCnt();

    inline UInt             getInsertRowCnt();

    inline dkdTypeConverter * getTypeConverter();

    inline void             moveFirst();

    inline void             setCurRowNull();

    inline UInt             getConvertedRowBufSize();

    inline SChar *          getConvertedRowBuffer();
    
    inline idBool           isFirst();
};

inline void dkdRemoteTableMgr::setEndOfFetch( idBool   aIsEndOfFetch )
{
    mIsEndOfFetch = aIsEndOfFetch;
}

inline idBool   dkdRemoteTableMgr::getEndOfFetch()
{
    return mIsEndOfFetch;
}

inline UInt dkdRemoteTableMgr::getFetchRowSize()
{
    return mRowSize;
}

inline UInt dkdRemoteTableMgr::getFetchRowCnt()
{
    return mFetchRowCnt;
}

inline UInt dkdRemoteTableMgr::getInsertRowCnt()
{
    return mInsertRowCnt;
}

inline dkdTypeConverter *   dkdRemoteTableMgr::getTypeConverter()
{
    return mTypeConverter;
}

inline void dkdRemoteTableMgr::moveFirst()
{
    mCurRow = mFetchRowBuffer;
}

inline void dkdRemoteTableMgr::setCurRowNull()
{
    mCurRow = NULL;
}

inline UInt dkdRemoteTableMgr::getConvertedRowBufSize()
{
    return mConvertedRowBufSize;
}

inline SChar *  dkdRemoteTableMgr::getConvertedRowBuffer()
{
    return mConvertedRowBuffer;
}

inline idBool dkdRemoteTableMgr::isFirst()
{
    idBool sResult = ID_FALSE;

    if ( mCurRow == mFetchRowBuffer )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

#endif /* _O_DKD_REMOTE_TABLE_MGR_H_ */
