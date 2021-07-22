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
 * $Id: smxTouchPageList.h 84177 2018-10-16 06:44:58Z justin.kwon $
 **********************************************************************/

# ifndef _O_SMX_TOUCH_PAGE_LIST_H_
# define _O_SMX_TOUCH_PAGE_LIST_H_

# include <idu.h>

# include <smDef.h>
# include <smxDef.h>

class smxTrans;



class smxTouchPageList
{

public:

    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    IDE_RC initialize( smxTrans * aTrans );
    inline void init( idvSQL * aStatistics );

    IDE_RC reset();
    IDE_RC destroy();

    IDE_RC add( scSpaceID  aSpaceID,
                scPageID   aPageID,
                SShort     aCTSIdx );

    idBool isEmpty();

    IDE_RC runFastStamping( smSCN * aCommitSCN );

private:

    IDE_RC alloc();
    void reuse(); /* TASK-6950 */

    static inline IDE_RC freeMem( smxTouchNode *aNode );

    inline void initNode( smxTouchNode *aNode );

public:

    smxTrans          * mTrans;
    idvSQL            * mStatistics;
    ULong               mItemCnt;
    smxTouchNode      * mNodeList; /* TASK-6950 : Touch Page Node들은 circular linked list 형태로 연결되어있고,
                                                  이 mNodeList는 그중 현재 사용중인 Touch Page Node 하나를 가르킨다. */
    ULong               mMaxCachePageCnt;
    static iduMemPool   mMemPool;
    static UInt         mTouchNodeSize;
    static UInt         mTouchPageCntByNode;
};

inline void smxTouchPageList::init( idvSQL * aStatistics )
{
    mItemCnt    = 0;
    mNodeList   = NULL;
    mStatistics = aStatistics;

    ULong sPageCntPerChunk;
    ULong sNeedChunkCnt;

    /*
     * 버퍼 Resize 연산이나 TRANSACTION_TOUCH_PAGE_CACHE_RATIO가 운영중에
     * 변경될 수 있기때문에 동적으로 변경될 수 있다.
     */
    mMaxCachePageCnt = (ULong)
        ( (smuProperty::getBufferAreaSize() / SD_PAGE_SIZE)
        * (smuProperty::getTransTouchPageCacheRatio() * 0.01) );

    if ( mMaxCachePageCnt != 0 )
    {
        /* (TASK-6950)
           할당된 mempool chunk의 모든 element를 사용하도록
           mMaxCachePageCnt를 조정한다.*/

        /* chunk당 page정보 갯수 */
        sPageCntPerChunk = mMemPool.mElemCnt * mTouchPageCntByNode;

        /* mMaxCachePageCnt가 들어갈수 있는 chunk 갯수 */
        sNeedChunkCnt    = ( ( mMaxCachePageCnt - 1 ) / ( sPageCntPerChunk ) ) + 1 ;

        /* chunk에 최대 들어갈수있는 page갯수로 보정한다. */
        mMaxCachePageCnt = sNeedChunkCnt * sPageCntPerChunk;
    }
    else
    {
        /* (TASK-6950)
           프로퍼티 TRANSACTION_TOUCH_PAGE_CACHE_RATIO_ = 0 으로 세팅되어 들어오면,
           Touch Page List를 사용하지 않는다는 의미이다. (테스트용) */
    }

    return;
}

inline void smxTouchPageList::initNode( smxTouchNode *aNode )
{
    aNode->mNxtNode = aNode;
    aNode->mPageCnt = 0;
    aNode->mAlign   = 0;
}

inline IDE_RC smxTouchPageList::freeMem( smxTouchNode *aNode )
{
    return mMemPool.memfree( aNode );
}

inline idBool smxTouchPageList::isEmpty()
{
    if ( mNodeList == NULL )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}


# endif
