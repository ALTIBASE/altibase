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
 *
 * $Id: sdpscCache.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment의 Segment Runtime Cache에 대한
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPSC_CACHE_H_
# define _O_SDPSC_CACHE_H_ 1

# include <sdpscDef.h>

class sdpscCache
{

public:

    // [ INTERFACE ] Segment Handle의 Cache할당 및 초기화
    static IDE_RC initialize( sdpSegHandle * aSegHandle,
                              scSpaceID      aSpaceID,
                              sdpSegType     aSegType,
                              smOID          aObjectID );

    // [ INTERFACE ] Segment Handle의 Cache 해제
    static IDE_RC destroy( sdpSegHandle * aSegHandle );

    static inline sdpSegType getSegType( sdpSegHandle * aSegHandle );

    static inline void setSegSizeByBytes( sdpSegHandle * aSegHandle,
                                          ULong          aSegSizeByBytes );

    static inline sdpscSegCache * getCache( sdpSegHandle * aSegHandle );

    static IDE_RC getSegCacheInfo( idvSQL             * /*aStatistics*/,
                                   sdpSegHandle       * /*aSegHandle*/,
                                   sdpSegCacheInfo    * aSegCacheInfo );

    static IDE_RC setLstAllocPage( idvSQL           * /*aStatistics*/,
                                   sdpSegHandle     * /*aSegHandle*/,
                                   scPageID           /*aLstAllocPID*/,
                                   ULong              /*aLstAllocSeqNo*/ );
};


inline sdpSegType sdpscCache::getSegType( sdpSegHandle * aSegHandle )
{
    return getCache(aSegHandle)->mCommon.mSegType;
}

inline sdpscSegCache * sdpscCache::getCache( sdpSegHandle * aSegHandle )
{
    return ((sdpscSegCache*)aSegHandle->mCache);
}

inline void sdpscCache::setSegSizeByBytes( sdpSegHandle * aSegHandle,
                                           ULong          aSegSizeByBytes )
{
    IDE_ASSERT( aSegSizeByBytes > 0 );

    getCache(aSegHandle)->mCommon.mSegSizeByBytes = aSegSizeByBytes;
}

#endif // _O_SDPSC_CACHE_H_
