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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_IDU_SHM_PERSLIST_H_
#define _O_IDU_SHM_PERSLIST_H_ 1

#include <idl.h>
#include <idTypes.h>
#include <iduShmDef.h>
#include <idu.h>

//BUGBUG
//following structure will be refered from VLogShmMemList
struct iduStShmMemList;
struct iduStShmMemChunk;
struct iduStShmMemSlot;

#include <iduShmMemList.h>
#include <iduVLogShmPersList.h>

class iduShmPersList
{
    friend class iduShmPersPool;
    friend void iduCheckPersConsistency( iduStShmMemList *aMemList );

public:
    iduShmPersList();
    ~iduShmPersList();

    static IDE_RC initialize( idvSQL             * aStatistics,
                              iduShmTxInfo       * aShmTxInfo,
                              iduMemoryClientIndex aIndex,
                              UInt                 aSeqNumber,
                              SChar              * aStrName,
                              vULong               aElemSize,
                              vULong               aElemCnt,
                              vULong               aAutofreeChunkLimit,
                              idBool               aUseLatch,
                              UInt                 aAlignByte,
                              idShmAddr            aAddr4MemList,
                              iduStShmMemList    * aMemListInfo );

    static IDE_RC destroy( idvSQL             * aStatistics,
                           iduShmTxInfo       * aShmTxInfo,
                           idrSVP             * aSavepoint,
                           iduStShmMemList    * aMemListInfo,
                           idBool               aCheck );

    /* ------------------------------------------------
     *  Special case for only 1 allocation & free
     * ----------------------------------------------*/
    static IDE_RC cralloc( idvSQL          * aStatistics,
                           iduShmTxInfo    * aShmTxInfo,
                           iduStShmMemList * aMemListInfo,
                           idShmAddr       * aAddr4Mem,
                           void           ** aMem );

    static IDE_RC alloc( idvSQL          * aStatistics,
                         iduShmTxInfo    * aShmTxInfo,
                         iduStShmMemList * aMemListInfo,
                         idShmAddr       * aAddr4Mem,
                         void           ** aMem );

    static IDE_RC memfree( idvSQL          * aStatistics,
                           iduShmTxInfo    * aShmTxInfo,
                           idrSVP          * aSavepoint,
                           iduStShmMemList * aMemListInfo,
                           idShmAddr         aAddr4Mem,
                           void            * aMem );

    static UInt   getUsedMemory( iduStShmMemList * aMemListInfo );
    static void   status( iduStShmMemList * aMemListInfo );

    static IDE_RC shrink( idvSQL          * aStatistics,
                          iduShmTxInfo    * aShmTxInfo,
                          iduStShmMemList * aMemListInfo,
                          UInt            * aSize );

    static void   fillMemPoolInfo( iduStShmMemList    * aMemListInfo,
                                   iduShmMemPoolState * aInfo );

    static void   dumpPartialFreeChunk4TSM( iduStShmMemList * aMemListInfo );

private:
    static inline void lock( idvSQL          * aStatistics,
                             iduShmTxInfo    * aShmTxInfo,
                             iduStShmMemList * aMemListInfo )
    {
        iduShmLatchAcquire( aStatistics, aShmTxInfo, &aMemListInfo->mLatch );
    }

    static IDE_RC grow( idvSQL          * aStatistics,
                        iduShmTxInfo    * aShmTxInfo,
                        iduStShmMemList * aMemListInfo );

    static inline void unlink( iduShmTxInfo     * aShmTxInfo,
                               iduStShmMemChunk * aChunk );

    static inline void link( iduShmTxInfo       * aShmTxInfo,
                             iduStShmMemChunk   * aBefore,
                             iduStShmMemChunk   * aChunk );
};

inline void iduShmPersList::unlink( iduShmTxInfo      * aShmTxInfo,
                                   iduStShmMemChunk  * aChunk )
{
    iduStShmMemChunk *sBefore =
        (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR_CHECK( aChunk->mAddrPrev );
    iduStShmMemChunk *sAfter  =
        (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aChunk->mAddrNext );

    if( sAfter != NULL )
    {
        iduVLogShmMemList::writeUnlinkWithAfter( aShmTxInfo,
                                                 sBefore,
                                                 sAfter );
        // duplicate  for logging performance
        sBefore->mAddrNext = aChunk->mAddrNext;
        sAfter->mAddrPrev  = aChunk->mAddrPrev;
    } else
    {

        iduVLogShmMemList::writeUnlinkOnlyBefore( aShmTxInfo,
                                                  sBefore );
        // duplicate  for logging performance
        sBefore->mAddrNext = aChunk->mAddrNext;
    }
}

inline void iduShmPersList::link( iduShmTxInfo         * aShmTxInfo,
                                 iduStShmMemChunk     * aBefore,
                                 iduStShmMemChunk     * aChunk )
{
    iduStShmMemChunk *sAfter =
        (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aBefore->mAddrNext );

    if( sAfter != NULL )
    {
        iduVLogShmMemList::writeLinkWithAfter( aShmTxInfo,
                                               aBefore,
                                               aChunk,
                                               sAfter );

        // duplicate for logging performance
        aBefore->mAddrNext = aChunk->mAddrSelf;
        aChunk->mAddrPrev  = aBefore->mAddrSelf;

        sAfter->mAddrPrev = aChunk->mAddrSelf;
        aChunk->mAddrNext = sAfter->mAddrSelf;
    }
    else
    {
        iduVLogShmMemList::writeLinkOnlyBefore( aShmTxInfo,
                                                aBefore,
                                                aChunk );

        //  duplicate for logging performance
        aBefore->mAddrNext = aChunk->mAddrSelf;
        aChunk->mAddrPrev  = aBefore->mAddrSelf;

        aChunk->mAddrNext = IDU_SHM_NULL_ADDR;
    }

}

#endif  /* _O_IDU_SHM_PERSLIST_H_ */

