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
 * $Id: sdpstStackMgr.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 위치 이력정보를 관리하는
 * Stack 관리자의 구현파일이다.
 *
 ***********************************************************************/

# include <sdpstStackMgr.h>

/***********************************************************************
 * Description : Stack을 초기화한다.
 ***********************************************************************/
void sdpstStackMgr::initialize( sdpstStack  * aStack )
{
    aStack->mDepth = SDPST_EMPTY;
    idlOS::memset( &(aStack->mPosition), 
                   0x00, 
                   ID_SIZEOF(sdpstPosItem)*SDPST_BMP_TYPE_MAX);
    return;
}

/***********************************************************************
 * Description : Stack을 해제한다.
 ***********************************************************************/
void sdpstStackMgr::destroy()
{
    return;
}

/***********************************************************************
 * Description : Virtual Stack Depth 에서의 Slot 간의 (aLHS 와 aRHS)
 *               거리차를 반환한다.
 ***********************************************************************/
SShort sdpstStackMgr::getDistInVtDepth( sdpstPosItem       * aLHS,
                                        sdpstPosItem       * aRHS )
{
    IDE_DASSERT( aLHS != NULL );
    IDE_DASSERT( aRHS != NULL );
    return getDist( &(aLHS[ (UInt)SDPST_VIRTBMP ]),
                    &(aRHS[ (UInt)SDPST_VIRTBMP ]) );
}

/***********************************************************************
 * Description : Root Stack Depth 에서의 Slot 간의 (aLHS 와 aRHS)
 *               거리차를 반환한다.
 ***********************************************************************/
SShort sdpstStackMgr::getDistInRtDepth( sdpstPosItem       * aLHS,
                                        sdpstPosItem       * aRHS )
{
    IDE_DASSERT( aLHS != NULL );
    IDE_DASSERT( aRHS != NULL );
    return getDist( &(aLHS[ (UInt)SDPST_RTBMP ]),
                    &(aRHS[ (UInt)SDPST_RTBMP ]) );
}

/***********************************************************************
 * Description : Internal Stack Depth 에서의 Slot 간의 (aLHS 와 aRHS)
 *               거리차를 반환한다.
 ***********************************************************************/
SShort sdpstStackMgr::getDistInItDepth( sdpstPosItem       * aLHS,
                                        sdpstPosItem       * aRHS )
{
    IDE_DASSERT( aLHS != NULL );
    IDE_DASSERT( aRHS != NULL );
    return getDist( &(aLHS[ (UInt)SDPST_ITBMP ]),
                    &(aRHS[ (UInt)SDPST_ITBMP ]) );
}

/***********************************************************************
 * Description : Leaf Stack Depth 에서의 Slot 간의 (aLHS 와 aRHS)
 *               거리차를 반환한다.
 ***********************************************************************/
SShort sdpstStackMgr::getDistInLfDepth( sdpstPosItem       * aLHS,
                                        sdpstPosItem       * aRHS )
{
    IDE_DASSERT( aLHS != NULL );
    IDE_DASSERT( aRHS != NULL );
    return getDist( &(aLHS[ (UInt)SDPST_LFBMP ]),
                    &(aRHS[ (UInt)SDPST_LFBMP ]) );
}

/***********************************************************************
 * Description : Stack Depth에 상관없이 두 위치간의 선후관계를 반환한다.
 ***********************************************************************/
SShort sdpstStackMgr::compareStackPos( sdpstStack  * aLHS,
                                       sdpstStack  * aRHS )
{
    SShort            sDist = 0;
    sdpstPosItem    * sLHS;
    sdpstPosItem    * sRHS;
    SInt              sDepth;
    sdpstBMPType   sDepthL;
    sdpstBMPType   sDepthR;

    IDE_DASSERT( aLHS != NULL );
    IDE_DASSERT( aRHS != NULL );

    sDepthL = getDepth( aLHS );
    sDepthR = getDepth( aRHS );

    if ( sDepthL == SDPST_EMPTY )
    {
        if ( sDepthR == SDPST_EMPTY )
        {
            return 0; // 둘다 empty 인경우
        }
        else
        {
            return 1; // lhs만 empty인 경우
        }
    }
    else
    {
        if ( sDepthR == SDPST_EMPTY )
        {
            return -1; // rhs 만 empty인 경우
        }
        else
        {
            // 둘다 empty가 아닌경우
        }
    }

    sLHS = getAllPos( aLHS );
    sRHS = getAllPos( aRHS );

    sDepth = (SInt)(sDepthL > sDepthR ? sDepthR : sDepthL);

    for ( ;
          sDepth > (SInt)SDPST_EMPTY;
          sDepth-- )
    {
        sDist = getDist( &(sLHS[ sDepth ]), &(sRHS[ sDepth ]));

        // 모든 stack level에서 distance가 0여야 한다.
        if ( sDist == SDPST_FAR_AWAY_OFF )
        {
            // 판단이 되지 않으므로 상위비교가 필요하다.
        }
        else
        {
            if ( sDist == 0)
            {
                // 하위에서부터 비교하기 때문에 동일하지 않으면,
                // 그 상위에서는 절대 같은 경우가 나오지 않는다.
                // 왜냐하면, 아래 else에서 다 걸러지기 때문이다.
                break;
            }
            else
            {
                sDist = ( sDist > 0 ? 1 : -1 );
                break;
            }
        }
    }
    return sDist;
}

void sdpstStackMgr::dump( sdpstStack    * aStack )
{
    IDE_ASSERT( aStack != NULL );

    ideLog::logMem( IDE_SERVER_0,
                    (UChar*)aStack,
                    ID_SIZEOF( sdpstStack ) );

    ideLog::log( IDE_SERVER_0,
                 "-------------------- Stack Begin --------------------\n"
                 "mDepth: %d\n"
                 "mPosition[VT].mNodePID: %u\n"
                 "mPosition[VT].mIndex: %u\n"
                 "mPosition[RT].mNodePID: %u\n"
                 "mPosition[RT].mIndex: %u\n"
                 "mPosition[IT].mNodePID: %u\n"
                 "mPosition[IT].mIndex: %u\n"
                 "mPosition[LF].mNodePID: %u\n"
                 "mPosition[LF].mIndex: %u\n"
                 "--------------------  Stack End  --------------------\n",
                 aStack->mDepth,
                 aStack->mPosition[SDPST_VIRTBMP].mNodePID,
                 aStack->mPosition[SDPST_VIRTBMP].mIndex,
                 aStack->mPosition[SDPST_RTBMP].mNodePID,
                 aStack->mPosition[SDPST_RTBMP].mIndex,
                 aStack->mPosition[SDPST_ITBMP].mNodePID,
                 aStack->mPosition[SDPST_ITBMP].mIndex,
                 aStack->mPosition[SDPST_LFBMP].mNodePID,
                 aStack->mPosition[SDPST_LFBMP].mIndex );
}
