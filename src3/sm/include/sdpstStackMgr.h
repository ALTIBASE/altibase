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
 * $Id: sdpstStackMgr.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 위치 이력정보를 관리하는
 * Stack 관리자의 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_STACK_MGR_H_
# define _O_SDPST_STACK_MGR_H_ 1

# include <sdpstDef.h>

class sdpstStackMgr
{
public:

    static void initialize( sdpstStack * aStack );
    static void destroy();

    static inline void push( sdpstStack     * aStack,
                             sdpstPosItem   * aPosItem )
    {
        IDE_ASSERT( aStack->mDepth < (SInt)SDPST_BMP_TYPE_MAX );
        aStack->mPosition[++aStack->mDepth] = *aPosItem;
        return;
    }

    static inline void push( sdpstStack     * aStack,
                             scPageID         aNodePID,
                             SShort           aIndex )
    {
        IDE_ASSERT( aStack->mDepth < (SInt)SDPST_BMP_TYPE_MAX );
        aStack->mPosition[++(aStack->mDepth)].mNodePID = aNodePID;
        aStack->mPosition[  (aStack->mDepth)].mIndex   = aIndex;
        return;
    }

    static inline sdpstPosItem pop( sdpstStack * aStack )
    {
        IDE_ASSERT( aStack->mDepth != (SInt)SDPST_EMPTY );
        return aStack->mPosition[ aStack->mDepth-- ];
    }

    static inline void setCurrPos( sdpstStack * aStack,
                                   scPageID     aNodePID,
                                   SShort       aIndex )
    {
        IDE_ASSERT( aStack->mDepth != (SInt)SDPST_EMPTY );
        aStack->mPosition[ aStack->mDepth ].mNodePID = aNodePID;
        aStack->mPosition[ aStack->mDepth ].mIndex = aIndex;
        return;
    }

    static inline sdpstPosItem getSeekPos( sdpstStack      * aStack,
                                           sdpstBMPType   aDepth )
    {
        IDE_ASSERT( aStack->mDepth >= (SInt)aDepth );
        return aStack->mPosition[ aDepth ];
    }

    static inline sdpstPosItem  getCurrPos( sdpstStack * aStack )
    {
        IDE_ASSERT( aStack->mDepth != (SInt)SDPST_EMPTY );
        return aStack->mPosition[ aStack->mDepth ];
    }

    static inline sdpstPosItem* getAllPos( sdpstStack * aStack )
    {
        IDE_ASSERT( aStack->mDepth != (SInt)SDPST_EMPTY );
        return aStack->mPosition;
    }

    static inline sdpstBMPType getDepth( sdpstStack * aStack )
    {
        return (sdpstBMPType)aStack->mDepth;
    }

    static void dump( sdpstStack    * aStack );

    static SShort compareStackPos( sdpstStack * aLHS,
                                   sdpstStack * aRHS );

    static SShort getDistInVtDepth( sdpstPosItem       * aLHS,
                                    sdpstPosItem       * aRHS );

    static SShort getDistInRtDepth( sdpstPosItem       * aLHS,
                                    sdpstPosItem       * aRHS );

    static SShort getDistInItDepth( sdpstPosItem       * aLHS,
                                    sdpstPosItem       * aRHS );

    static SShort getDistInLfDepth( sdpstPosItem       * aLHS,
                                    sdpstPosItem       * aRHS );

    static inline SShort getDist( sdpstPosItem * aLHS,
                                  sdpstPosItem * aRHS );
};

// 같은 Level의 PosItem간에 거리를 구한다.
inline SShort sdpstStackMgr::getDist( sdpstPosItem * aLHS,
                                      sdpstPosItem * aRHS )
{
    SShort  sDist;

    if ( aLHS->mNodePID == aRHS->mNodePID )
    {
        sDist = aLHS->mIndex - aRHS->mIndex;
    }
    else
    {
        // 다른 페이지이므로 distance를 측정할 수 없다.
        sDist  = SDPST_FAR_AWAY_OFF;
    }

    return sDist;
}


#endif // _O_SDPST_STACK_MGR_H_
