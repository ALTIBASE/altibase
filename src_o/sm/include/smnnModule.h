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
 * $Id: smnnModule.h 89495 2020-12-14 05:19:22Z emlee $
 **********************************************************************/

#ifndef _O_SMNN_MODULE_H_
#define _O_SMNN_MODULE_H_ 1

#include <smnDef.h>
#include <smnnDef.h>

extern smnIndexModule smnnModule;

class smnnSeq
{
public:
    static IDE_RC prepareIteratorMem( const smnIndexModule* );

    static IDE_RC releaseIteratorMem(const smnIndexModule* );

    static IDE_RC init( smnnIterator         * aIterator,
                        void                 * aTrans,
                        smcTableHeader       * aTable,
                        smnIndexHeader       * aIndex,
                        void                 * aDumpObject,
                        const smiRange       * aKeyRange,
                        const smiRange       * aKeyFilter,
                        const smiCallBack    * aRowFilter,
                        UInt                   aFlag,
                        smSCN                  aSCN,
                        smSCN                  aInfinite,
                        idBool                 aUntouchable,
                        smiCursorProperties  * aProperties,
                        const smSeekFunc    ** aSeekFunc,
                        smiStatement         * aStatement );

    static IDE_RC dest( smnnIterator* aIterator );

    static IDE_RC NA( void );

    static IDE_RC AA( smnnIterator* aIterator, const void**  aRow );

    static IDE_RC beforeFirst( smnnIterator*       aIterator,
                               const smSeekFunc**  aSeekFunc );
    static IDE_RC afterLast( smnnIterator*       aIterator,
                             const smSeekFunc**  aSeekFunc );
    static IDE_RC beforeFirstU( smnnIterator*       aIterator,
                                const smSeekFunc**  aSeekFunc );
    static IDE_RC afterLastU( smnnIterator*       aIterator,
                              const smSeekFunc**  aSeekFunc );
    static IDE_RC beforeFirstR( smnnIterator*       aIterator,
                                const smSeekFunc**  aSeekFunc );
    static IDE_RC afterLastR( smnnIterator*       aIterator,
                              const smSeekFunc**  aSeekFunc );
    static IDE_RC moveNextUsingFLI( smnnIterator*       aIterator );
    static IDE_RC movePrevUsingFLI( smnnIterator*       aIterator );
    static IDE_RC movePageUsingFLI( smnnIterator*       aIterator,
                                    scPageID    *       aNextPID );
    static IDE_RC fetchNext( smnnIterator*       aIterator,
                             const void**        aRow );
    static IDE_RC makeRowCache ( smnnIterator*       aIterator,
                                 const void**        aRow,
                                 idBool*             aFound    );
    static IDE_RC fetchPrev( smnnIterator*       aIterator,
                             const void**        aRow );
    static IDE_RC fetchNextU( smnnIterator*       aIterator,
                              const void**        aRow );
    static IDE_RC fetchPrevU( smnnIterator*       aIterator,
                              const void**        aRow );
    static IDE_RC fetchNextR( smnnIterator*       aIterator );

    static IDE_RC allocIterator( void ** aIteratorMem );

    static IDE_RC freeIterator( void * aIteratorMem );

    static IDE_RC gatherStat( idvSQL         * aStatistics,
                              void           * aTrans,
                              SFloat           aPercentage,
                              SInt             aDegree,
                              smcTableHeader * aHeader,
                              void           * aTotalTableArg,
                              smnIndexHeader * aIndex,
                              void           * aStats,
                              idBool           aDynamicMode );

    static IDE_RC gatherStat4Fixed( idvSQL         * aStatistics,
                                    void           * aTableArgument,
                                    SFloat           aPercentage,
                                    smcTableHeader * aHeader,
                                    void           * aTotalTableArg );

    static IDE_RC gatherStat4Var( idvSQL         * aStatistics,
                                  void           * aTableArgument,
                                  SFloat           aPercentage,
                                  smcTableHeader * aHeader );

private:
    static IDE_RC moveNextNonBlock( smnnIterator  * aIterator );

    static IDE_RC moveNextBlock( smnnIterator   * aIterator );

    static IDE_RC moveNext( smnnIterator   * aIterator )
    {
        if( smuProperty::getScanlistMoveNonBlock() == ID_TRUE )
        {
            return moveNextNonBlock( aIterator );
        }
        else
        {
            return moveNextBlock( aIterator );
        }
    };
    // PROJ-2402
    static IDE_RC moveNextParallelNonBlock( smnnIterator * aIterator );

    static IDE_RC moveNextParallelBlock( smnnIterator * aIterator );

    static IDE_RC moveNextParallel( smnnIterator * aIterator )
    {
        if( smuProperty::getScanlistMoveNonBlock() == ID_TRUE )
        {
            return moveNextParallelNonBlock( aIterator );
        }
        else
        {
            return moveNextParallelBlock( aIterator );
        }
    };
    static IDE_RC movePrevNonBlock( smnnIterator*       aIterator );

    static IDE_RC movePrevBlock( smnnIterator*       aIterator );

    static IDE_RC movePrev( smnnIterator*       aIterator )
    {
        if( smuProperty::getScanlistMoveNonBlock() == ID_TRUE )
        {
            return movePrevNonBlock( aIterator );
        }
        else
        {
            return movePrevBlock( aIterator );
        }
    };

private:
    static iduMemPool   mIteratorPool;
};

#endif /* _O_SMNN_MODULE_H_ */
