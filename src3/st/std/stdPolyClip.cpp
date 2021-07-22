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
 ** $Id$
 ***********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtdTypes.h>
#include <mtuProperty.h>

#include <ste.h>
#include <stcDef.h>
#include <stdTypes.h>
#include <stdUtils.h>
#include <stdParsing.h>
#include <stdPrimitive.h>
#include <qcuProperty.h>
#include <stfAnalysis.h>

#include <stdPolyClip.h>

/***********************************************************************
 * Description :
 * Vertex에 등록되어 있는 VertexEntry를 각 Entry가 참조하는 세그먼트의 
 * 각도에 따라 정렬하고, VertexEntry의 시작과 끝을 이어서 순환형태로
 * 만든다. 
 * 
 *                      Seg2 (Entry2)
 *                      /
 *                     /
 * (Entry1) Seg1 -----*----- Seg3 (Entry3)
 *            aVertex
 * 
 * 위 그림과 같은 경우 Vertex에서 VertexEntry의 순서는 
 * 
 *   3->2->1->3 과 같은 형태로 정렬된다.
 * 
 * Vertex* aVertex : Vertex 포인터
 **********************************************************************/
void stdPolyClip::sortVertexList( Vertex* aVertex )
{
    VertexEntry* sStart;
    VertexEntry* sEnd;
    VertexEntry* sCurr;
    VertexEntry* sComp;   
    VertexEntry  sTmp;

    if ( aVertex->mList != NULL )
    {
        if ( aVertex->mNext != aVertex )
        {
            sStart = aVertex->mList;
            sCurr = sStart;

            do
            {
                sCurr->mAngle = getSegmentAngle( sCurr->mSeg, aVertex->mCoord );
                sCurr = sCurr->mNext;
            } while ( sCurr != sStart );

            sCurr = sStart;
            sEnd = sStart->mPrev;
            do
            {
                sComp = sStart;
                do
                {
                    if ( sComp->mAngle > sComp->mNext->mAngle )
                    {
                        sTmp.mAngle          = sComp->mAngle;
                        sTmp.mSeg            = sComp->mSeg;
                        sComp->mAngle        = sComp->mNext->mAngle;
                        sComp->mSeg          = sComp->mNext->mSeg;
                        sComp->mNext->mAngle = sTmp.mAngle;
                        sComp->mNext->mSeg   = sTmp.mSeg;
                    }
                    else
                    {
                        // Nothing to do
                    }

                    sComp = sComp->mNext;
                }while ( sComp != sEnd );
        
                sCurr = sCurr->mNext;
            }while ( sCurr != sStart );

            aVertex->mNext = aVertex;
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do.
    }
}

/***********************************************************************
 * Description :
 * 세그먼트가 x축과 이루는 각을 구한다.
 *
 *          aSeg
 *           /
 *          / ) 각
 * mCoord  *-------- x축
 *
 * Segment*   aSeg   : 세그먼트
 * stdPoint2D mCoord : 교점의 좌표
 **********************************************************************/
SDouble stdPolyClip::getSegmentAngle( Segment*   aSeg, 
                                      stdPoint2D mCoord )
{
    SDouble sRet;
   
    if ( ( stdUtils::isSamePoints2D4Func( &(aSeg->mStart), &mCoord ) == ID_TRUE ) )
    {
       sRet = idlOS::atan2( aSeg->mEnd.mY - mCoord.mY, aSeg->mEnd.mX - mCoord.mX ); 
    }
    else
    {
       sRet = idlOS::atan2( aSeg->mStart.mY - mCoord.mY, aSeg->mStart.mX - mCoord.mX );
    }

    return sRet;
}

/***********************************************************************
 * Description :
 * 입력된 세그먼트가 Intersection 연산 결과에 포함되는지 판단한다.
 * 만약 포함된다면, 세그먼트를 입력된 방향대로 사용할 지, 
 * 반대 방향으로 사용할지를 결정하여 aDir로 넘긴다. 
 *
 * Segment*   aSeg : 세그먼트
 * Direction* aDir : 현재 세그먼트를 사용 할 경우 취해야 하는 방향 
 * UInt       aPN  : 사용하지 않음
 **********************************************************************/
idBool stdPolyClip::segmentRuleIntersect( Segment*   aSeg, 
                                          Direction* aDir, 
                                          UInt       /* aPN */ )
{
    idBool sRet;

    if ( ( aSeg->mLabel == ST_SEG_LABEL_INSIDE ) || 
         ( aSeg->mLabel == ST_SEG_LABEL_SHARED1) ) 
    {
        *aDir = ST_SEG_DIR_FORWARD;
         sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

/***********************************************************************
 * Description :
 * 입력된 세그먼트가 Union 연산 결과에 포함되는지 판단한다.
 * 만약 포함된다면, 세그먼트를 입력된 방향대로 사용할 지, 
 * 반대 방향으로 사용할지를 결정하여 aDir로 넘긴다. 
 *
 * Segment*   aSeg : 세그먼트
 * Direction* aDir : 현재 세그먼트를 사용 할 경우 취해야 하는 방향 
 * UInt       aPN  : 사용하지 않음
 **********************************************************************/
idBool stdPolyClip::segmentRuleUnion( Segment*   aSeg, 
                                      Direction* aDir, 
                                      UInt       /* aPN */ )
{
    idBool sRet;

    if ( ( aSeg->mLabel == ST_SEG_LABEL_OUTSIDE ) ||
         ( aSeg->mLabel == ST_SEG_LABEL_SHARED1 ) )
    {
        *aDir = ST_SEG_DIR_FORWARD;
         sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }
    
    return sRet;
}

/***********************************************************************
 * Description :
 * 입력된 세그먼트가 Difference 연산 결과에 포함되는지 판단한다.
 * 만약 포함된다면, 세그먼트를 입력된 방향대로 사용할 지, 
 * 반대 방향으로 사용할지를 결정하여 aDir로 넘긴다. 
 *
 * A difference B 와 같은 연산을 하는 경우 입력받은 세그먼트가 속한
 * 폴리곤의 번호가 aPN 보다 작으면 세그먼트는 A에 속하는 세그먼트이고, 
 * 크거나 같으면 B에 속하는 세그먼트이다.
 *
 * Segment*   aSeg : 세그먼트
 * Direction* aDir : 현재 세그먼트를 사용 할 경우 취해야 하는 방향 
 * UInt       aPN  : 폴리곤 번호 
 **********************************************************************/
idBool stdPolyClip::segmentRuleDifference( Segment*   aSeg, 
                                           Direction* aDir, 
                                           UInt       aPN )
{
    idBool sRet = ID_FALSE;

    if ( ( ( aSeg->mLabel == ST_SEG_LABEL_OUTSIDE ) ||
           ( aSeg->mLabel == ST_SEG_LABEL_SHARED2 ) ) && 
           ( aSeg->mParent->mPolygonNum < aPN ) )
    {
        *aDir = ST_SEG_DIR_FORWARD;
         sRet = ID_TRUE;
    }
    else
    {

        if ( ( ( aSeg->mLabel == ST_SEG_LABEL_INSIDE  ) || 
               ( aSeg->mLabel == ST_SEG_LABEL_SHARED2 ) ) && 
               ( aSeg->mParent->mPolygonNum >= aPN ) )
        {
            *aDir = ST_SEG_DIR_BACKWARD;
             sRet = ID_TRUE;
        }
        else
        {
            // Nothing to do
        }
    }

    return sRet;
}

/***********************************************************************
 * Description :
 * 입력된 세그먼트가 SymDifference 연산 결과에 포함되는지 판단한다.
 * 만약 포함된다면, 세그먼트를 입력된 방향대로 사용할 지, 
 * 반대 방향으로 사용할지를 결정하여 aDir로 넘긴다. 
 *
 * Segment*   aSeg : 세그먼트
 * Direction* aDir : 현재 세그먼트를 사용 할 경우 취해야 하는 방향 
 * UInt       aPN  : 사용하지 않음
 **********************************************************************/
idBool stdPolyClip::segmentRuleSymDifference( Segment*   aSeg, 
                                              Direction* aDir, 
                                              UInt       /* aPN */ )
{
    idBool sRet = ID_FALSE;

    if ( aSeg->mLabel == ST_SEG_LABEL_OUTSIDE )
    {
        *aDir = ST_SEG_DIR_FORWARD;
         sRet = ID_TRUE;
    }
    else
    {
        if ( aSeg->mLabel == ST_SEG_LABEL_INSIDE )
        {
            *aDir = ST_SEG_DIR_BACKWARD;
             sRet = ID_TRUE;
        }
        else
        {
            // Nothing to do
        }
    }

    return sRet;
}

/***********************************************************************
 * Description :
 * 입력받은 링을 탐색하여 Clip 결과에 포함되어야 하는 세그먼트를 찾고, 
 * 찾아낸 세그먼트를 시작으로 하는 링을 구성하도록 Collect를 호출한다.
 *
 * iduMemory*   aQmxMem        :
 * Segment**    aRingList      : 링 리스트
 * UInt         aRingCount     : 링 개수
 * ResRingList* aResRingList   : 결과 링 리스트
 * Segment**    aIndexSegList  : 결과에 대한 인덱스
 * UInt*        aIndexSegCount : 인덱스 수
 * SegmentRule  aRule          : 결과에 포함될지를 결정하는 함수의 포인터
 * UInt         aPN            : 폴리곤 번호
 **********************************************************************/
IDE_RC stdPolyClip::clip( iduMemory*   aQmxMem,    
                          Segment**    aRingList, 
                          UInt         aRingCount,
                          ResRingList* aResRingList,
                          Segment**    aIndexSegList,
                          UInt*        aIndexSegCount,
                          SegmentRule  aRule, 
                          UInt         aPN,
                          UChar*       aCount )
{
    UInt      i;
    Segment*  sCurrSeg;
    Direction sDir = ST_SEG_DIR_FORWARD;
    UInt      sRingCnt = 0;
    
    *aIndexSegCount = 0;

    for ( i = 0 ; i < aRingCount ; i++ )
    {
        sCurrSeg = aRingList[i];
        do
        {
            if ( ( sCurrSeg->mUsed == ST_SEG_USABLE ) && 
                 ( aRule( sCurrSeg, &sDir, aPN ) == ID_TRUE ) )
            {
                if ( aCount == NULL )
                {
                    IDE_TEST ( collect( aQmxMem, 
                                        aResRingList,
                                        sRingCnt,
                                        aIndexSegList, 
                                        aIndexSegCount, 
                                        sCurrSeg, 
                                        aRule, 
                                        &sDir, 
                                        aPN,
                                        aCount ) 
                               != IDE_SUCCESS );
                    sRingCnt++;
                }
                else
                {
                    if ( sCurrSeg->mLabel == ST_SEG_LABEL_OUTSIDE )
                    {
                        IDE_TEST ( collect( aQmxMem, 
                                            aResRingList,
                                            sRingCnt,
                                            aIndexSegList, 
                                            aIndexSegCount, 
                                            sCurrSeg, 
                                            aRule, 
                                            &sDir, 
                                            aPN,
                                            aCount ) 
                                   != IDE_SUCCESS );
                        sRingCnt++;
                    }
                    else
                    {
                        // Nothing to do
                    }
                }
            }
            else
            {
                // Nothing to do
            }
            sCurrSeg = stdUtils::getNextSeg( sCurrSeg );

        }while ( sCurrSeg != aRingList[i] );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 입력받은 세그먼트를 시작으로 결과에 포함되어야 하는 세그먼트를 
 * 찾아 링을 구성하고 결과 링 리스트에 추가한다.
 *
 * iduMemory*   aQmxMem
 * ResRingList* aResRingList   : 결과 링 리스트 
 * UInt         aRingNumber    : 생성할 링의 번호
 * Segment**    aIndexSegList  : 결과에 대한 인덱스
 * UInt*        aIndexSegCount : 인덱스 수
 * Segment*     aSeg           : 시작 세그먼트
 * SegmentRule  aRule          : 세그먼트 판별 함수 포인터
 * Direction*   aDir           : 세그먼트 선택 방향
 * UInt         aPN            : 폴리곤 번호
 **********************************************************************/
IDE_RC  stdPolyClip::collect( iduMemory*   aQmxMem,
                              ResRingList* aResRingList,
                              UInt         aRingNumber,
                              Segment**    aIndexSegList,
                              UInt*        aIndexSegCount,
                              Segment*     aSeg, 
                              SegmentRule  aRule, 
                              Direction*   aDir, 
                              UInt         aPN, 
                              UChar*       aCount )
{
    Segment*     sSeg           = aSeg;
    Segment*     sNewSeg;
    Segment*     sMaxSeg        = NULL;
    VertexEntry* sCurrEntry;
    VertexEntry* sStart;
    SInt         sPointsCount   = 0;
    SInt         sVaritation    = ST_NOT_SET;
    SInt         sReverse       = ST_NOT_SET;
    SInt         sPreVar        = ST_NOT_SET;
    SInt         sPreRev        = ST_NOT_SET;
    Chain*       sChain         = NULL;
    Chain*       sParent        = NULL;
    Chain*       sPrevChain     = NULL;
    Chain*       sFirstChain    = NULL;
    idBool       sIsCreateChain = ID_FALSE;
    stdPoint2D   sPt;
    stdPoint2D   sNextPt;
    stdPoint2D   sStartPt;
    stdPoint2D   sMaxPt;
    Ring*        sRing;
    stdMBR       sTotalMbr;
    stdMBR       sSegmentMbr;

    stdUtils::initMBR( &sTotalMbr );

    if ( isSameDirection( *aDir, getDirection( sSeg ) ) == ID_TRUE )
    {
        sStartPt = sSeg->mStart;
    }
    else
    {
        sStartPt = sSeg->mEnd;
    }

    sMaxPt = sStartPt;
    
    do
    {
        IDE_TEST_RAISE( sSeg == NULL, ERR_UNKNOWN );

        sPointsCount++;
        sSeg->mUsed = ST_SEG_USED;

        if ( isSameDirection( *aDir, getDirection( sSeg ) ) == ID_TRUE )
        {
            sPt     = sSeg->mStart;
            sNextPt = sSeg->mEnd;
        }
        else
        {
            sPt     = sSeg->mEnd;
            sNextPt = sSeg->mStart;
        }

        /* BUG-33634
         * 셀프터치가 있는 링을 찾거나, 링 간 포함관계를 계산할 수 있도록
         * Clip 결과에 대해서도 인덱스를 생성한다. */

        stdUtils::getSegProperty( &sPt,
                                  &sNextPt,
                                  &sVaritation,
                                  &sReverse ) ;

        IDE_TEST ( aQmxMem->alloc( ID_SIZEOF(Segment),
                                   (void**) &sNewSeg )
                   != IDE_SUCCESS );
        
        sNewSeg->mBeginVertex = NULL;
        sNewSeg->mEndVertex   = NULL;

        if ( sReverse == ST_NOT_REVERSE )
        {
            sNewSeg->mStart = sPt;
            sNewSeg->mEnd   = sNextPt;
        }
        else
        {
            sNewSeg->mStart = sNextPt;
            sNewSeg->mEnd   = sPt;
        }

        sNewSeg->mPrev = NULL;
        sNewSeg->mNext = NULL;

        if ( ( sVaritation != sPreVar ) || ( sReverse != sPreRev ) )
        {
            if ( sParent != NULL )
            {
                aIndexSegList[*aIndexSegCount] = sParent->mBegin;
                (*aIndexSegCount)++;
            }
            else
            {
                // Nothing to do
            }

            /* 속성이 다르면 체인을 생성한다. */
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Chain),
                                      (void**) &sChain )
                      != IDE_SUCCESS );

            if ( sIsCreateChain == ID_FALSE )
            {
                sIsCreateChain = ID_TRUE;
                sFirstChain    = sChain;
            }
            else
            {
                // Nothing to do
            }

            stdUtils::initChain( sNewSeg, 
                                 sChain, 
                                 sVaritation, 
                                 sReverse, 
                                 0,             /* 폴리곤 번호, 사용 안함 */
                                 aRingNumber, 
                                 ST_PARALLEL,   /* 링 방향, 사용 안함 */
                                 ST_CHAIN_NOT_OPPOSITE );

            if ( sPrevChain != NULL )
            {
                sChain->mPrev     = sPrevChain;
                sPrevChain->mNext = sChain;
            }
            else
            {
                sChain->mNext = NULL;
                sChain->mPrev = NULL;
            }

            sParent = sChain;
            IDE_TEST_RAISE( sParent == NULL, ERR_UNKNOWN );
            sNewSeg->mParent = sParent;

            sPreVar = sVaritation;
            sPreRev = sReverse;
        }
        else
        {
            sParent = sChain;
            IDE_TEST_RAISE( sParent == NULL, ERR_UNKNOWN );
            sNewSeg->mParent = sParent;

            if ( sReverse == ST_NOT_REVERSE )
            {
                stdUtils::appendLastSeg( sNewSeg, sChain );
            }
            else
            {
                stdUtils::appendFirstSeg( sNewSeg, sChain );
            }
        }
        sPrevChain = sChain;
        // classify end
        
        if ( ( sSeg->mLabel == ST_SEG_LABEL_SHARED1 ) || 
             ( sSeg->mLabel == ST_SEG_LABEL_SHARED2 ) )
        {
            // mark used shared segment
            sStart     = sSeg->mBeginVertex->mList;
            sCurrEntry = sStart;
            do
            {
                if ( ( sCurrEntry->mSeg != sSeg )                             &&
                     ( stdUtils::isSamePoints2D4Func( &(sCurrEntry->mSeg->mStart), 
                                                 &(sSeg->mStart) ) 
                       == ID_TRUE )                                           &&
                     ( stdUtils::isSamePoints2D4Func( &(sCurrEntry->mSeg->mEnd), 
                                                 &(sSeg->mEnd) ) 
                       == ID_TRUE ) )
                {
                    sCurrEntry->mSeg->mUsed = ST_SEG_USED;
                }
                else
                {
                    // Nothing to do
                }
                sCurrEntry = sCurrEntry->mNext;

            }while ( ( sCurrEntry != NULL ) && ( sCurrEntry != sStart ) );
        }
        else
        {
            // Nothing to do
        }

        if ( sMaxSeg == NULL )
        {
            sMaxSeg = sNewSeg;
        }

        if ( sMaxPt.mY < sNextPt.mY )
        {
            sMaxSeg = sNewSeg;
            sMaxPt  = sNextPt;
        }
        else
        {
            if ( sMaxPt.mY == sNextPt.mY )
            {
                if ( sMaxPt.mX <= sNextPt.mX )
                {
                    sMaxSeg = sNewSeg;
                    sMaxPt  = sNextPt;
                }
                else
                {
                    // Nothing to do
                }
            }
            else
            {
                // Nothing to do
            }
        }

        sSegmentMbr.mMinX = sSeg->mStart.mX;
        sSegmentMbr.mMaxX = sSeg->mEnd.mX;
        if( sSeg->mStart.mY < sSeg->mEnd.mY )
        {
            sSegmentMbr.mMinY = sSeg->mStart.mY;
            sSegmentMbr.mMaxY = sSeg->mEnd.mY;
        }
        else
        {
            sSegmentMbr.mMinY = sSeg->mEnd.mY;
            sSegmentMbr.mMaxY = sSeg->mStart.mY;
        }

        stdUtils::mergeOrMBR( &sTotalMbr, &sTotalMbr, &sSegmentMbr );

        if ( stdUtils::isSamePoints2D4Func( &sStartPt, &sNextPt ) == ID_TRUE )
        {
            /* 링의 시작점과 현재 세그먼트의 끝점이 같다면
             * 닫힌 링이 구성된 것이므로 루프에서 빠져나온다. */
            break;
        }
        else
        {
            if ( isIntersectPoint( sSeg, *aDir ) == ID_TRUE )
            {
                /* 다음 점이 교점이라면, 다음에 선택해야 할 
                 * 세그먼트를 찾아야 한다. */
                sSeg = jump( sSeg, aRule, aDir, aPN, aCount );
            }
            else
            {
                /* 현재 선택방향으로 다음 세그먼트를 선택한다. */
                if ( *aDir == ST_SEG_DIR_FORWARD )
                {
                    sSeg = stdUtils::getNextSeg( sSeg );
                }
                else
                {
                    sSeg = stdUtils::getPrevSeg( sSeg );
                }
            }
        } // end of if

    }while(1);

    if ( ( sIsCreateChain == ID_TRUE ) && ( sChain != sFirstChain ) )
    {
        aIndexSegList[ *aIndexSegCount ] = sParent->mBegin;
        ( *aIndexSegCount )++;

        sChain->mNext      = sFirstChain;
        sFirstChain->mPrev = sChain;
        
        IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Ring),
                                  (void**) &sRing )
                  != IDE_SUCCESS );

        sRing->mPrev       = NULL;
        sRing->mNext       = NULL;
        sRing->mParent     = NULL;
        sRing->mFirstSeg   = sFirstChain->mBegin;
        sRing->mRingNumber = aRingNumber;
        sRing->mPointCnt   = sPointsCount;
        stdUtils::copyMBR( &(sRing->mMBR), &sTotalMbr );

        /* 링의 방향 계산 */
        if ( sMaxSeg->mParent->mReverse == ST_NOT_REVERSE )
        {
            sRing->mOrientation = stdUtils::CCW( *getPrevPoint( sMaxSeg, ID_FALSE ),
                                                 sMaxSeg->mEnd,
                                                 *getNextPoint( sMaxSeg, ID_FALSE ) );
        }
        else
        {
            sRing->mOrientation = stdUtils::CCW( *getPrevPoint( sMaxSeg, ID_TRUE ),
                                                 sMaxSeg->mStart,
                                                 *getNextPoint( sMaxSeg, ID_TRUE ) );
        }

        /* 링의 방향에 따라 반시계 방향은 리스트의 앞에, 
         * 시계방향은 리스트의 뒤에 추가한다. */
        if ( sRing->mOrientation == ST_COUNTERCLOCKWISE )
        {
            appendFirst( aResRingList, sRing );
        }
        else
        {
            appendLast( aResRingList, sRing );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNKNOWN );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNKNOWN_POLYGON ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 세그먼트 선택 방향에 따라 세그먼트의 끝 점이 교점인지 여부를 반환한다.
 *
 * Segment*  aSeg : 세그먼트 
 * Direction aDir : 세그먼트 선택 방향 
 **********************************************************************/
idBool  stdPolyClip::isIntersectPoint( Segment*  aSeg, 
                                       Direction aDir )
{
    idBool sRet;

    if ( isSameDirection( aDir, getDirection( aSeg ) ) == ID_TRUE )
    {
        if ( aSeg->mEndVertex != NULL )
        {
            sRet = ID_TRUE;
        }
        else
        {
            sRet = ID_FALSE;
        }
    }
    else
    {
        if ( aSeg->mBeginVertex != NULL )
        {
            sRet = ID_TRUE;
        }
        else
        {
            sRet = ID_FALSE;
        }
    }

    return sRet;
}

/***********************************************************************
 * Description :
 * 입력받은 세그먼트에서 다음으로 선택해야 하는 세그먼트를 찾아 반환한다.
 *
 *              (1)
 *             /
 *            /
 * aSeg  --->*------ (2)
 *            \
 *             \
 *              (3)
 *
 * aSeg에서 반시계 방향으로 돌면서 결과에 포함되어야 하는 세그먼트를 찾는다.
 * (1), (2), (3)의 순서로 탐색한다. 
 *
 * Segment*    aSeg  : 세그먼트
 * SegmentRule aRule : 세그먼트 판별 함수 포인터
 * Direction   aDir  : 세그먼트 선택 방향
 * UInt        aPN   : 폴리곤 번호
 **********************************************************************/
Segment* stdPolyClip::jump( Segment*    aSeg, 
                            SegmentRule aRule, 
                            Direction*  aDir, 
                            UInt        aPN,
                            UChar*      aCount )
{
    VertexEntry* sCurrEntry;
    VertexEntry* sStartEntry;
    Segment*     sRet        = NULL;
    
    if ( isSameDirection( *aDir, getDirection( aSeg ) ) == ID_TRUE )
    {
        sStartEntry = aSeg->mEndVertex->mList;
    }
    else
    {
        sStartEntry = aSeg->mBeginVertex->mList;
    }

    sCurrEntry = sStartEntry;
    do
    {
        if ( sCurrEntry->mSeg == aSeg )
        {
            break;
        }
        else
        {
            // Nothing to do
        }

        sCurrEntry = sCurrEntry->mNext;

    }while ( sCurrEntry != sStartEntry );

    sStartEntry = sCurrEntry;

    if ( aCount == NULL )
    {
        do
        {
           sCurrEntry = sCurrEntry->mPrev;
           if ( ( sCurrEntry->mSeg->mUsed == ST_SEG_USABLE )        && 
                ( aRule( sCurrEntry->mSeg, aDir, aPN ) == ID_TRUE ) )
           {
               sRet = sCurrEntry->mSeg;
               break;
           }
           else
           {
               // Nothing to do
           }
        }while ( sCurrEntry != sStartEntry );
    }
    else
    {
        do
        {
           sCurrEntry = sCurrEntry->mNext;
           if ( ( sCurrEntry->mSeg->mUsed == ST_SEG_USABLE )        && 
                ( aRule( sCurrEntry->mSeg, aDir, aPN ) == ID_TRUE ) )
           {
               sRet = sCurrEntry->mSeg;
               break;
           }
           else
           {
               // Nothing to do
           }
        }while ( sCurrEntry != sStartEntry );
    }

    return sRet;
}

/***********************************************************************
 * Description :
 * 각 세그먼트의 라벨을 붙인다. 
 * A (op) B에서 A의 속하는 세그먼트가 
 * B의 외부에 있으면       ST_SEG_LABEL_OUTSIDE,
 * B의 내부에 있으면       ST_SEG_LABEL_INSIDE,
 * B의 세그먼트와 일치하면 ST_SEG_LABEL_SHARE1 (방향까지 일치)
 *                         ST_SEG_LABEL_SHARE2 (방향은 다름)
 *
 * B에 속하는 세그먼트도 마찬가지 방법으로 라벨을 붙인다. 
 *
 *
 * 예) ㄱ 자 모양의 폴리곤 A와 ㅁ 자 모양의 폴리곤 B 
 *
 *                         ST_SEG_LABEL_OUTSIDE
 *                                /
 *                          +--------------------------+
 *  ST_SEG_LABEL_OUTSIDE    |  ( ST_SEG_LABEL_INSIDE ) |
 *                       \  |     /                    |
 *                       +--*------+                   |
 *                       |  |      |                   |
 *                       +--*--+   |                   |
 *                          |  |   |                   |
 *                          +--*===*-------------------+
 *                                \ 
 *                           ST_SEG_LABEL_SHARE1 (방향이 같은 경우)
 *                           ST_SEG_LABEL_SHARE2 (방향이 다른 경우)
 *
 * Segment** aRingSegList   : 링 리스트 
 * UInt      aRingCount     : 링 개수
 * Segment** aIndexSeg      : 인덱스 
 * UInt      aIndexSegTotal : 인덱스 수
 * UInt      aMax1          : A (op) B 에서 A에 속하는 폴리곤 번호 중 가장 큰 수
 * UInt      aMax2          : B에 속하는 폴리곤 번호 중 가장 큰 수
 **********************************************************************/
IDE_RC stdPolyClip::labelingSegment( Segment** aRingSegList, 
                                     UInt      aRingCount, 
                                     Segment** aIndexSeg, 
                                     UInt      aIndexSegTotal,
                                     UInt      aMax1, 
                                     UInt      aMax2,
                                     UChar*    aCount )
{
    UInt     i;
    Segment* sCurrSeg;
    idBool   sIsFirst;

    for ( i = 0 ; i < aRingCount ; i++ )
    {
        sIsFirst    = ID_TRUE;
        sCurrSeg    = aRingSegList[i];
        
        do
        {
            sCurrSeg->mUsed  = ST_SEG_USABLE;
            sCurrSeg->mLabel = ST_SEG_LABEL_OUTSIDE;
            if ( ( sCurrSeg->mBeginVertex != NULL ) && ( sCurrSeg->mEndVertex != NULL ) )
            {
                /* BUG-33436
                 * 세그먼트의 양 끝점이 모두 교점이라면 Shared 세그먼트일 가능성이 있다.
                 * Share 인지 체크한 후, 아닌 경우에는 Inside 또는 Outside인지 체크해야한다. */

                determineShare( sCurrSeg );
                if ( ( sCurrSeg->mLabel != ST_SEG_LABEL_SHARED1 ) && 
                     ( sCurrSeg->mLabel != ST_SEG_LABEL_SHARED2 ) )
                {
                    determineInside( aIndexSeg, aIndexSegTotal, sCurrSeg, aMax1, aMax2, aCount);
                }
                else
                {
                    // Nothing to do 
                }

                sortVertexList( sCurrSeg->mBeginVertex );
                sortVertexList( sCurrSeg->mEndVertex );
            }
            else
            {
                // Nothing to do 
            }

            if ( ( sCurrSeg->mBeginVertex == NULL ) && ( sCurrSeg->mEndVertex == NULL ) )
            {
                if ( sIsFirst == ID_TRUE )
                {
                    determineInside( aIndexSeg, aIndexSegTotal, sCurrSeg, aMax1, aMax2, aCount );
                    sIsFirst = ID_FALSE;
                }
                else
                {
                    sCurrSeg->mLabel = ( stdUtils::getPrevSeg( sCurrSeg ) )->mLabel;
                }
            }
            else
            {
                // Nothing to do
            }

            if ( ( sCurrSeg->mBeginVertex != NULL ) && ( sCurrSeg->mEndVertex == NULL ) )
            {
                if ( ( sCurrSeg->mParent->mReverse == ST_NOT_REVERSE ) ||
                     ( sIsFirst == ID_TRUE ) )
                {
                    determineInside( aIndexSeg, aIndexSegTotal, sCurrSeg, aMax1, aMax2, aCount );
                    sIsFirst = ID_FALSE;
                }
                else
                {
                    sCurrSeg->mLabel = ( stdUtils::getPrevSeg( sCurrSeg ) )->mLabel;
                }
                
                sortVertexList( sCurrSeg->mBeginVertex );
            }
            else
            {
                // Nothing to do
            }

            if ( ( sCurrSeg->mEndVertex != NULL ) && ( sCurrSeg->mBeginVertex == NULL ) )
            {
                if ( ( sCurrSeg->mParent->mReverse == ST_REVERSE ) || 
                     ( sIsFirst == ID_TRUE ) )
                {
                    determineInside( aIndexSeg, aIndexSegTotal, sCurrSeg, aMax1, aMax2, aCount );
                    sIsFirst = ID_FALSE;                   
                }
                else
                {
                    sCurrSeg->mLabel = ( stdUtils::getPrevSeg( sCurrSeg ) )->mLabel;
                }

                sortVertexList( sCurrSeg->mEndVertex );
            }
            else
            {
                // Nothing to do
            }

            sCurrSeg = stdUtils::getNextSeg( sCurrSeg );

        }while ( sCurrSeg != aRingSegList[i] );
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 * 결과 링을 바탕으로 stdLinearRing2D를 생성한다.
 * 
 * Segment*         aFirstSeg : 링의 첫 번째 세그먼트
 * UInt             aPointCnt : 링의 포인트 수
 * stdLinearRing2D* aRes      : 결과
 * UInt             aFence    : stdLinearRing이 가질 수 있는 최대 크기
 **********************************************************************/
IDE_RC stdPolyClip::chainToRing( Segment*         aFirstSeg,
                                 UInt             aPointCnt,
                                 stdLinearRing2D* aRes, 
                                 UInt             aFence )
{
    stdPoint2D* sPoint;
    Segment*    sCurrSeg;

    IDE_TEST_RAISE( (aPointCnt + 1) * sizeof( stdPoint2D ) > aFence, ERR_SIZE_ERROR );
 
    STD_N_POINTS(aRes) = aPointCnt + 1;
    sPoint             = STD_FIRST_PT2D(aRes);

    sCurrSeg = aFirstSeg;
    do
    {
        if ( sCurrSeg->mParent->mReverse == ST_NOT_REVERSE ) 
        {
            idlOS::memcpy( sPoint, &(sCurrSeg->mStart), STD_PT2D_SIZE );
        }
        else
        {
            idlOS::memcpy( sPoint, &(sCurrSeg->mEnd), STD_PT2D_SIZE );
        }

        sPoint++;
        sCurrSeg = stdUtils::getNextSeg( sCurrSeg );

    }while ( sCurrSeg != aFirstSeg );

    // To fix BUG-33472 CodeSonar warining

    if ( sPoint != STD_FIRST_PT2D(aRes))
    {    
        idlOS::memcpy( sPoint, STD_FIRST_PT2D(aRes), STD_PT2D_SIZE );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SIZE_ERROR );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_BUFFER_OVERFLOW ));
    } 

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * Clip 결과를 바탕으로 Geometry를 생성한다.
 * 
 * ResRingList*       aResRingList : Clip 후 링 리스트 
 * stdGeometryHeader* aRes         : Geometry
 * UInt               aFence       : Geometry의 최대 크기
 **********************************************************************/
IDE_RC stdPolyClip::makeGeometryFromRings( ResRingList*       aResRingList, 
                                           stdGeometryHeader* aRes, 
                                           UInt               aFence )
{
    stdMultiPolygon2DType* sMultiPolygon;
    stdPolygon2DType*      sPolygon;
    stdLinearRing2D*       sRing;
    stdMBR                 sTotalMbr;
    UInt                   sPolygonCount = 0;
    Ring*                  sResRing;

    if ( aResRingList->mRingCnt <= 0 )
    {
        stdUtils::makeEmpty(aRes);
    }
    else
    {
        stdUtils::setType( aRes, STD_MULTIPOLYGON_2D_TYPE );

        sMultiPolygon        = (stdMultiPolygon2DType*) aRes;
        sMultiPolygon->mSize = STD_MPOLY2D_SIZE;
        sPolygon             = STD_FIRST_POLY2D(sMultiPolygon);
        sResRing             = aResRingList->mBegin;
        sTotalMbr            = sResRing->mMBR;

        while ( sResRing != NULL )
        {
            stdUtils::setType( (stdGeometryHeader*) sPolygon, STD_POLYGON_2D_TYPE );

            sPolygon->mSize      = STD_POLY2D_SIZE;
            sPolygon->mMbr       = sResRing->mMBR;
            sPolygon->mNumRings  = 1;
            sRing                = STD_FIRST_RN2D(sPolygon);

            stdUtils::mergeOrMBR( &sTotalMbr, &sTotalMbr, &(sResRing->mMBR) );

            IDE_TEST( chainToRing( sResRing->mFirstSeg, 
                                   sResRing->mPointCnt, 
                                   sRing, 
                                   aFence - sMultiPolygon->mSize ) 
                      != IDE_SUCCESS);

            sPolygon->mSize += STD_RN2D_SIZE + (STD_PT2D_SIZE * STD_N_POINTS(sRing));

            sResRing = sResRing->mNext;
            
            while ( sResRing != NULL )
            {
                /* 내부링이 있는 경우 폴리곤에 내부링들을 추가한다. */
                if ( sResRing->mOrientation == ST_CLOCKWISE )
                {
                    sRing = STD_NEXT_RN2D(sRing);

                    IDE_TEST( chainToRing( sResRing->mFirstSeg, 
                                           sResRing->mPointCnt, 
                                           sRing, 
                                           aFence - sMultiPolygon->mSize - sPolygon->mSize ) 
                              != IDE_SUCCESS );

                    sPolygon->mSize += STD_RN2D_SIZE + (STD_PT2D_SIZE * STD_N_POINTS(sRing));
                    sPolygon->mNumRings++;

                    sResRing = sResRing->mNext;
                }
                else
                {
                    break;
                }
            }
            
            sMultiPolygon->mSize += sPolygon->mSize;
            sPolygonCount++;

            sPolygon = STD_NEXT_POLY2D(sPolygon);
        }
        sMultiPolygon->mMbr        = sTotalMbr;
        sMultiPolygon->mNumObjects = sPolygonCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

void stdPolyClip::determineInside( Segment** aIndexSeg, 
                                   UInt      aIndexSegTotal, 
                                   Segment*  aSeg, 
                                   UInt      aMax1, 
                                   UInt      aMax2,
                                   UChar*    aCount )
{
    idBool sIsInside;

    if ( aCount == NULL )
    {
        if ( aSeg->mParent->mPolygonNum < aMax1 )
        {
            sIsInside = stdUtils::isRingInSide( aIndexSeg, 
                                                aIndexSegTotal, 
                                                aSeg, 
                                                aMax1, 
                                                aMax1 + aMax2 - 1,
                                                aCount );

            aSeg->mLabel = ( sIsInside == ID_TRUE ) ? 
                             ST_SEG_LABEL_INSIDE :
                             ST_SEG_LABEL_OUTSIDE;
        }
        else
        {
            sIsInside = stdUtils::isRingInSide( aIndexSeg, 
                                                aIndexSegTotal, 
                                                aSeg, 
                                                0, 
                                                aMax1 - 1,
                                                aCount );

            aSeg->mLabel = ( sIsInside == ID_TRUE ) ? 
                             ST_SEG_LABEL_INSIDE : 
                             ST_SEG_LABEL_OUTSIDE;
        }
    }
    else
    {
        sIsInside = stdUtils::isRingInSide( aIndexSeg,
                                            aIndexSegTotal,
                                            aSeg,
                                            0,
                                            aMax2,
                                            aCount );

        aSeg->mLabel = ( sIsInside == ID_TRUE ) ? 
                         ST_SEG_LABEL_INSIDE : 
                         ST_SEG_LABEL_OUTSIDE;
    }

    return ;    
}

void stdPolyClip::determineShare( Segment* aSeg )
{
    VertexEntry* sCurrEntry;
    VertexEntry* sStart      = aSeg->mBeginVertex->mList;
   
    if ( ( aSeg->mLabel != ST_SEG_LABEL_SHARED1 ) && 
         ( aSeg->mLabel != ST_SEG_LABEL_SHARED2 ) )
    {            
        sCurrEntry = sStart;
        do
        {
            if ( ( sCurrEntry->mSeg != aSeg )                             &&
                 ( sCurrEntry->mSeg->mBeginVertex == aSeg->mBeginVertex ) &&
                 ( sCurrEntry->mSeg->mEndVertex == aSeg->mEndVertex ) )

            {
                if ( sCurrEntry->mSeg->mParent->mReverse == aSeg->mParent->mReverse )
                {
                    aSeg->mLabel             = ST_SEG_LABEL_SHARED1;
                    sCurrEntry->mSeg->mLabel = ST_SEG_LABEL_SHARED1;
                }
                else
                {
                    aSeg->mLabel             = ST_SEG_LABEL_SHARED2;
                    sCurrEntry->mSeg->mLabel = ST_SEG_LABEL_SHARED2;
                }
            }
            else
            {
                // Nothing to do 
            }
            sCurrEntry = sCurrEntry->mNext;

        }while ( ( sCurrEntry != NULL ) && ( sCurrEntry != sStart ) );
    }
}

// BUG-33436
/***********************************************************************
 * Description:
 *   입력받은 세그먼트의 방향이 같은지를 판별한다.
 *
 *   Direction aDir1 : 1의 방향
 *   Direction aDir2 : 2의 방향
 **********************************************************************/
idBool stdPolyClip::isSameDirection( Direction aDir1, 
                                     Direction aDir2 )
{
    idBool sRet;

    if ( aDir1 == aDir2 )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

// BUG-33634
Direction stdPolyClip::getDirection( Segment* aSeg ) 
{
    Direction sRet;

    if( aSeg->mParent->mReverse == ST_NOT_REVERSE )
    {
        sRet = ST_SEG_DIR_FORWARD;
    }
    else
    {
        sRet = ST_SEG_DIR_BACKWARD;
    }

    return sRet;
}

/***********************************************************************
* Description:
*   링의 방향이 올바른지 검사하고, 아닌경우 반대로 뒤집는다. 
*
*   aRingSegList   : 각 링의 첫 세그먼트 리스트
*   aRingCount     : 링의 개수
*   aIndexSeg      : 각 체인의 첫 세그먼트 리스트
*   aIndexSegTotal : 체인의 개수
*   aMax1          : A (op) B 중 A에 속하는 폴리곤의 수 
*   aMax2          : A (op) B 중 B에 속하는 폴리곤의 수
**********************************************************************/
void stdPolyClip::adjustRingOrientation( Segment** aRingSegList, 
                                         UInt      aRingCount,
                                         Segment** aIndexSeg,
                                         UInt      aIndexSegTotal,
                                         UInt      aMax1,
                                         UInt      aMax2 )
{
    UInt     i;
    SInt     sOrientation;
    Segment* sRingSeg;
    Chain*   sChain;
    idBool   sIsInside;

    for ( i = 0 ; i < aRingCount ; i++ )
    {
        sRingSeg = aRingSegList[i];
       
        if ( sRingSeg->mParent->mPolygonNum < aMax1 )
        {
            sIsInside = stdUtils::isRingInSide( aIndexSeg, 
                                                aIndexSegTotal,
                                                sRingSeg, 
                                                0, 
                                                aMax1 - 1,
                                                NULL );

            sOrientation = ( sIsInside == ID_TRUE ) ? 
                             ST_CLOCKWISE : 
                             ST_COUNTERCLOCKWISE;
        }
        else
        {
            sIsInside = stdUtils::isRingInSide( aIndexSeg, 
                                                aIndexSegTotal, 
                                                sRingSeg, 
                                                aMax1, 
                                                aMax1 + aMax2 - 1,
                                                NULL );

            sOrientation = ( sIsInside == ID_TRUE ) ? 
                             ST_CLOCKWISE : 
                             ST_COUNTERCLOCKWISE;
        }

        if ( sRingSeg->mParent->mOrientaion != sOrientation )
        {
            /* 링의 방향을 반대로 바꾼다. 
             * 이에 따라 링을 구성하는 체인의 방향도 수정해야 한다. */ 
            sChain = sRingSeg->mParent;
            do
            {
                sChain->mOrientaion = ( sChain->mOrientaion == ST_CLOCKWISE ) ?
                                        ST_COUNTERCLOCKWISE :
                                        ST_CLOCKWISE ;

                if ( sChain->mReverse == ST_NOT_REVERSE )
                {
                    sChain->mReverse = ST_REVERSE;
                }
                else
                {
                    sChain->mReverse = ST_NOT_REVERSE;
                }

                if ( sChain->mOpposite == ST_CHAIN_NOT_OPPOSITE )
                {
                    sChain->mOpposite = ST_CHAIN_OPPOSITE;
                }
                else
                {
                    sChain->mOpposite = ST_CHAIN_OPPOSITE;
                }

                sChain = sChain->mNext;

            }while ( sChain != sRingSeg->mParent );
        }
        else
        {
            // Nothing to do
        }
    }
}

void stdPolyClip::appendFirst( ResRingList* aResRingList, 
                               Ring*        aRing )
{
    aRing->mNext = aResRingList->mBegin;

    if ( aResRingList->mBegin != NULL )
    {
        aResRingList->mBegin->mPrev = aRing;
        aResRingList->mBegin        = aRing;
    }
    else
    {
        aResRingList->mBegin = aRing;
        aResRingList->mEnd   = aRing;
    }
    aResRingList->mRingCnt++;
}

void stdPolyClip::appendLast( ResRingList* aResRingList, 
                              Ring*        aRing )
{
    aRing->mPrev = aResRingList->mEnd;

    if ( aResRingList->mEnd != NULL )
    {
        aResRingList->mEnd->mNext = aRing;
        aResRingList->mEnd        = aRing;
    }
    else
    {
        aResRingList->mBegin = aRing;
        aResRingList->mEnd   = aRing;
    }
    aResRingList->mRingCnt++;
}

IDE_RC stdPolyClip::removeRingFromList( ResRingList* aResRingList, 
                                        UInt         aRingNum )
{
    Ring* sRing;

    sRing = aResRingList->mBegin;

    while ( sRing != NULL )
    {
        if ( sRing->mRingNumber == aRingNum )
        {
            break;
        }
        else
        {
            sRing = sRing->mNext;
        }
    }

    if ( sRing != NULL )
    {
        if ( sRing->mPrev != NULL )
        {
            sRing->mPrev->mNext = sRing->mNext;
        }
        else
        {
            // Nothing to do 
        }

        if ( sRing->mNext != NULL )
        {
            sRing->mNext->mPrev = sRing->mPrev;
        }
        else
        {
            // Nothing to do 
        }

        if ( aResRingList->mBegin == sRing )
        {
            aResRingList->mBegin = sRing->mNext;
        }
        else
        {
            // Nothing to do 
        }

        if ( aResRingList->mEnd == sRing )
        {
            aResRingList->mEnd = sRing->mPrev;
        }
        else
        {
            // Nothing to do 
        }
    }
    else
    {
        /* aRingNum에 해당하는 링을 찾을 수 없는 경우 */
        IDE_RAISE( ERR_UNKNOWN );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNKNOWN );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNKNOWN_POLYGON ));
    } 
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
* Description:
* 세그먼트에서 다음 점의 주소를 반환한다. aIsStart가 TRUE이면 세그먼트
* 시작 점의 다음 점(세그먼트의 끝 점)을 반환하고, FALSE이면 끝 점의 
* 다음 점 (다음 세그먼트의 끝 점)을 반환하게 된다.
*
* Segment* aSeg     : 세그먼트
* idBool   aIsStart : 세그먼트의 시작점인가?
**********************************************************************/
stdPoint2D* stdPolyClip::getNextPoint( Segment* aSeg, 
                                       idBool   aIsStart )
{
    Segment*    sTmp;
    stdPoint2D* sRet;

    if ( aIsStart == ID_TRUE )
    {
        if ( aSeg->mParent->mReverse == ST_NOT_REVERSE ) 
        {
            sRet = &(aSeg->mEnd);
        }
        else
        {
            sTmp = stdUtils::getNextSeg( aSeg );

            if ( aSeg->mParent->mReverse == sTmp->mParent->mReverse ) 
            {
                sRet = &(sTmp->mStart);
            }
            else
            {
                sRet = &(sTmp->mEnd);
            }
        }
    }
    else
    {
        if ( aSeg->mParent->mReverse == ST_NOT_REVERSE )
        {
            sTmp = stdUtils::getNextSeg( aSeg );
            
            if ( aSeg->mParent->mReverse == sTmp->mParent->mReverse ) 
            {
                sRet = &(sTmp->mEnd);
            }
            else
            {
                sRet = &(sTmp->mStart);
            }
        }
        else
        {
            sRet = &(aSeg->mStart);
        }
    }

    return sRet;
}

/***********************************************************************
* Description:
* 세그먼트에서 이전 점의 주소를 반환한다. aIsStart가 TRUE이면 세그먼트
* 시작 점의 이전 점(이전 세그먼트의 시작 점)을 반환하고, FALSE이면 
* 세그먼트 끝 점의 이전 점 (세그먼트의 시작 점)을 반환하게 된다.
*
* Segment* aSeg     : 세그먼트
* idBool   aIsStart : 세그먼트의 시작점인가?
**********************************************************************/
stdPoint2D* stdPolyClip::getPrevPoint( Segment* aSeg, 
                                       idBool   aIsStart )
{
    Segment*    sTmp;
    stdPoint2D* sRet;

    if ( aIsStart == ID_TRUE )
    {
        if ( aSeg->mParent->mReverse == ST_NOT_REVERSE )
        {
            sTmp = stdUtils::getPrevSeg( aSeg );

            if ( aSeg->mParent->mReverse == sTmp->mParent->mReverse ) 
            {
                sRet = &(sTmp->mStart);
            }
            else
            {
                sRet = &(sTmp->mEnd);
            }
        }
        else
        {
            sRet =  &(aSeg->mEnd);
        }
    }
    else
    {
        if ( aSeg->mParent->mReverse == ST_NOT_REVERSE )
        {
            sRet =  &(aSeg->mStart);
        }
        else
        {
            sTmp = stdUtils::getPrevSeg( aSeg );

            if ( aSeg->mParent->mReverse == sTmp->mParent->mReverse )
            {
                sRet = &(sTmp->mEnd);
            }
            else
            {
                sRet = &(sTmp->mStart);
            }
        }
    }

    return sRet;
}

/***********************************************************************
 * Description :
 *   Clip 결과 링의 순서를 정렬한다. 가장 먼저 외부링들을 정렬하고, 
 *   내부링이 자신이 포함되어야 하는 외부링의 뒤에 위치하도록 리스트를 
 *   정렬한다. 
 *   Ring의 부모는 자신을 포함할 수 있는 가장 작은 외부링을 의미한다.
 *
 *   ex) 초기 aResRingList
 *   - 외부링1->외부링2->외부링3->내부링1->내부링2
 *   
 *   정렬 후
 *   - 외부링1->내부링1->내부링2->외부링2->외부링3
 *
 *   ResRingList* aResRingList   : Clip 결과 링의 리스트
 *   Segment**    aIndexSeg,     : Clip 결과 링의 인덱스 세그먼트 리스트
 *   UInt         aIndexSegTotal : 인덱스 세그먼트의 수 
 **********************************************************************/
IDE_RC stdPolyClip::sortRings( ResRingList* aResRingList,
                               Segment**    aIndexSeg, 
                               UInt         aIndexSegTotal )
{
    Ring* sCurrRing = aResRingList->mBegin->mNext;
    Ring* sCompRing;
    Ring* sTmp;

    /* 외부링 */
    while ( ( sCurrRing != NULL ) )
    {
        if ( sCurrRing->mOrientation != ST_COUNTERCLOCKWISE )
        {
            break;
        }
        else
        {
            // Nothing to do
        }

        sCompRing = aResRingList->mBegin;

        while ( sCompRing != sCurrRing )
        {
            if ( ( sCurrRing->mParent == sCompRing->mParent )    &&
                 ( stdUtils::isMBRIntersects( &(sCurrRing->mMBR), 
                                              &(sCompRing->mMBR) )
                   == ID_TRUE ) )
            {
                /* 부모가 같고, 두 링의 MBR이 겹치는 경우에만 두 링의 포함관계를
                 * 계산한다. 부모가 다르거나, MBR이 겹치지 않는 경우에는 
                 * 두 링 사이에 포함관계가 있을 수 없다. */

                if ( stdUtils::isRingInSide( aIndexSeg,
                                             aIndexSegTotal,
                                             sCurrRing->mFirstSeg,
                                             sCompRing->mRingNumber,
                                             sCompRing->mRingNumber,
                                             NULL )
                     == ID_TRUE ) 
                {
                    sCurrRing->mParent = sCompRing;
                }
                else
                {
                    if ( stdUtils::isRingInSide( aIndexSeg,
                                                 aIndexSegTotal,
                                                 sCompRing->mFirstSeg,
                                                 sCurrRing->mRingNumber,
                                                 sCurrRing->mRingNumber,
                                                 NULL )
                         == ID_TRUE )
                    {
                        sCompRing->mParent = sCurrRing;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            else
            {
                /* Nothing to do */
            }
            sCompRing = sCompRing->mNext;
        }

        sTmp = sCurrRing->mNext;

        if ( sCurrRing->mParent == NULL )
        {
            /* 부모가 없다면 리스트의 맨 앞으로 보낸다. */
            if ( sTmp != NULL )
            {
                sTmp->mPrev = sCurrRing->mPrev;
            }
            else
            {
                // Nothing to do
            }

            sCurrRing->mPrev->mNext     = sTmp;

            sCurrRing->mPrev            = NULL;
            sCurrRing->mNext            = aResRingList->mBegin;
            
            aResRingList->mBegin->mPrev = sCurrRing;
            aResRingList->mBegin        = sCurrRing;
        }
        else
        {
            /* 부모가 있다면 부모 링 뒤로 보낸다. */
            if ( sCurrRing->mPrev != sCurrRing->mParent )
            {
                if ( sTmp != NULL )
                {
                    sTmp->mPrev = sCurrRing->mPrev;
                }
                else
                {
                    // Nothing to do 
                }

                sCurrRing->mPrev->mNext = sTmp;

                sCurrRing->mPrev        = sCurrRing->mParent;
                sCurrRing->mNext        = sCurrRing->mParent->mNext;

                sCurrRing->mParent->mNext = sCurrRing;
            }
            else
            {
                // Nothing to do
            }
        }
        sCurrRing = sTmp;
    }

    /* 내부링 */
    while ( sCurrRing != NULL )
    {
        sCompRing = aResRingList->mBegin;

        while ( sCompRing != sCurrRing )
        {
            if ( ( sCurrRing->mParent      == sCompRing->mParent  ) && 
                 ( sCompRing->mOrientation == ST_COUNTERCLOCKWISE ) &&
                 ( stdUtils::isMBRIntersects( &(sCurrRing->mMBR), 
                                              &(sCompRing->mMBR) )
                  == ID_TRUE ) )
            {
                if ( stdUtils::isRingInSide( aIndexSeg,
                                             aIndexSegTotal,
                                             sCurrRing->mFirstSeg,
                                             sCompRing->mRingNumber,
                                             sCompRing->mRingNumber,
                                             NULL )
                     == ID_TRUE ) 
                {
                    sCurrRing->mParent = sCompRing;
                }
                else
                {
                    // Nothing to do
                }

            }
            else
            {
                // Nothing to do
            }
            
            sCompRing = sCompRing->mNext;
        }

        sTmp = sCurrRing->mNext;


        /* 부모링이 없는 내부링은 존재할 수 없다.*/
        IDE_TEST_RAISE( sCurrRing->mParent == NULL, ERR_UNKNOWN ); 
        
        if ( sCurrRing->mPrev != sCurrRing->mParent )
        {
            if ( sTmp != NULL )
            {
                sTmp->mPrev = sCurrRing->mPrev;
            }
            else
            {
                // Nothing to do
            }

            sCurrRing->mPrev->mNext   = sTmp;

            sCurrRing->mPrev          = sCurrRing->mParent;
            sCurrRing->mNext          = sCurrRing->mParent->mNext;

            sCurrRing->mParent->mNext = sCurrRing;
        }
        else
        {
            // Nothing to do
        }

        sCurrRing = sTmp;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNKNOWN )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNKNOWN_POLYGON ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *   Clip 결과에서 셀프터치가 있는 지 찾아서 aResPrimInterSeg에 등록한다.
 * 
 *   iduMemory*     aQmxMem        
 *   Segment**      aIndexSeg,       : Clip 결과 링의 인덱스 세그먼트 리스트
 *   UInt           aIndexSegTotal   : 인덱스 세그먼트의 수
 *   PrimInterSeg** aResPrimInterSeg : 셀프터치가 발생한 점의 리스트 
 **********************************************************************/
IDE_RC stdPolyClip::detectSelfTouch( iduMemory*     aQmxMem,
                                     Segment**      aIndexSeg,
                                     UInt           aIndexSegTotal,
                                     PrimInterSeg** aResPrimInterSeg )
{
    UInt             i;
    UInt             sIndexSegTotal    = aIndexSegTotal;
    ULong            sReuseSegCount    = 0;
    SInt             sIntersectStatus;
    Segment**        sIndexSeg         = aIndexSeg;
    Segment*         sCurrSeg          = NULL;
    Segment*         sCmpSeg;    
    Segment**        sReuseSeg;
    Segment**        sTempIndexSeg;    
    iduPriorityQueue sPQueue;
    idBool           sOverflow         = ID_FALSE;
    idBool           sUnderflow        = ID_FALSE;
    Segment*         sCurrNext;
    Segment*         sCurrPrev;
    stdPoint2D       sInterResult[4];
    SInt             sInterCount;
    stdPoint2D*      sStartPt[2];
    stdPoint2D*      sNextPt[2];

    
    IDE_TEST( sPQueue.initialize( aQmxMem,
                                  sIndexSegTotal,
                                  ID_SIZEOF(Segment*),
                                  cmpSegment )
              != IDE_SUCCESS);
    
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Segment*) * sIndexSegTotal,
                              (void**)  &sReuseSeg )
              != IDE_SUCCESS);
        
    for ( i = 0; i < sIndexSegTotal; i++ )
    {
        sPQueue.enqueue( sIndexSeg++, &sOverflow);
        IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
    }

    while(1)
    {
        sPQueue.dequeue( (void*) &sCurrSeg, &sUnderflow );

        sCurrNext = stdUtils::getNextSeg(sCurrSeg);
        sCurrPrev = stdUtils::getPrevSeg(sCurrSeg);
        
        if ( sUnderflow == ID_TRUE )
        {
            break;
        }
        else
        {
            // Nothing to do 
        }

        sReuseSegCount = 0;
        sTempIndexSeg  = sReuseSeg;  

        while(1)
        {      
            sPQueue.dequeue( (void*) &sCmpSeg, &sUnderflow );

            if ( sUnderflow == ID_TRUE )
            {
                break;
            }
            else
            {
                // Nothing to do
            }


            sReuseSeg[sReuseSegCount++] = sCmpSeg;
            
            if ( sCmpSeg->mStart.mX > sCurrSeg->mEnd.mX )
            {
                /* 잘 못 뽑은 놈은 재사용에 넣어야 한다. */
                break;                
            }
            else
            {
                // Nothing to do
            }
            
            do
            {
                /*
                  여기서 intersect와 방향에 대한 개념을 넣어 쳐낼수 있다.                  
                 */
                if ( ( sCurrNext != sCmpSeg ) && ( sCurrPrev != sCmpSeg ) )
                {
                    IDE_TEST( stdUtils::intersectCCW4Func( sCurrSeg->mStart,
                                                           sCurrSeg->mEnd,
                                                           sCmpSeg->mStart,
                                                           sCmpSeg->mEnd,
                                                           &sIntersectStatus,
                                                           &sInterCount,
                                                           sInterResult)
                              != IDE_SUCCESS );

                    switch( sIntersectStatus )
                    {
                        case ST_POINT_TOUCH:
                            /* BUG-33436 
                               셀프터치가 발생하는 점으로 들어가는 방향의 세그먼트와 
                               발생한 점에서 나오는 방향의 세그먼트를 등록한다.  */
                            if ( sCurrSeg->mParent->mReverse == ST_NOT_REVERSE )
                            {
                                sStartPt[0] = &(sCurrSeg->mStart);
                                sNextPt[0]  = &(sCurrSeg->mEnd);
                            }
                            else
                            {
                                sStartPt[0] = &(sCurrSeg->mEnd);
                                sNextPt[0]  = &(sCurrSeg->mStart);
                            }

                            if ( sCmpSeg->mParent->mReverse == ST_NOT_REVERSE )
                            {
                                sStartPt[1] = &(sCmpSeg->mStart);
                                sNextPt[1]  = &(sCmpSeg->mEnd);
                            }
                            else
                            {
                                sStartPt[1] = &(sCmpSeg->mEnd);
                                sNextPt[1]  = &(sCmpSeg->mStart);
                            }

                            if ( stdUtils::isSamePoints2D( sNextPt[0], sStartPt[1] )
                                 == ID_TRUE )
                            {
                                IDE_TEST( stdUtils::addPrimInterSeg( aQmxMem,
                                                                     aResPrimInterSeg,
                                                                     sCurrSeg,
                                                                     sCmpSeg,
                                                                     sIntersectStatus,
                                                                     sInterCount,
                                                                     sInterResult )
                                          != IDE_SUCCESS );
                            }
                            else
                            {
                                // Nothing to do
                            }

                            if ( stdUtils::isSamePoints2D( sNextPt[1], sStartPt[0] ) 
                                 == ID_TRUE )
                            {
                                IDE_TEST( stdUtils::addPrimInterSeg( aQmxMem,
                                                                     aResPrimInterSeg,
                                                                     sCmpSeg,
                                                                     sCurrSeg,
                                                                     sIntersectStatus,
                                                                     sInterCount,
                                                                     sInterResult )
                                          != IDE_SUCCESS );
                            }
                            else
                            {
                                // Nothing to do
                            }
                            break;

                        case ST_TOUCH:                       
                        case ST_INTERSECT:
                        case ST_SHARE:
                            // Clip 결과에서 INTERSECT, SHARE가 발생할 수 없다. 
                            IDE_RAISE( ERR_UNKNOWN );
                            break;

                        case ST_NOT_INTERSECT:
                            break;                        
                    }
                }
                else
                {
                    // Nothing to do 
                }

                sCmpSeg = sCmpSeg->mNext;

                if ( sCmpSeg == NULL )
                {
                    break;                    
                }
            
                /* 끝까지 조사한다. */

            }while( sCmpSeg->mStart.mX <= sCurrSeg->mEnd.mX );            
        }

        /* 재사용을 정리 한다. */
    
        
        for ( i =0; i < sReuseSegCount ; i++)
        {
            sPQueue.enqueue( sTempIndexSeg++, &sOverflow);
            IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
            /* Overflow 검사 */
        }

        if ( sCurrSeg->mNext != NULL )
        {
            sPQueue.enqueue( &(sCurrSeg->mNext), &sOverflow);
            IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNKNOWN )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNKNOWN_POLYGON ));
    }
    IDE_EXCEPTION( ERR_ABORT_ENQUEUE_ERROR )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdPolyClip::detectSelfTouch",
                                  "enqueue overflow" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stdPolyClip::breakRing( iduMemory*    aQmxMem, 
                               ResRingList*  aResRingList,
                               PrimInterSeg* aResPrimInterSeg )
{
    UInt          i;
    PrimInterSeg* sPrim         = aResPrimInterSeg;
    Segment*      sSeg1;
    Segment*      sSeg2;
    Segment*      sMaxSeg;
    Segment*      sFirstSeg[2];
    Ring*         sRing;
    UInt          sPointCnt;
    stdPoint2D*   sNextPt;
    stdPoint2D*   sMaxPt;
    Chain*        sChain1;
    Chain*        sChain2;
    Chain*        sNewChain1;
    Chain*        sNewChain2;
    stdMBR        sTotalMbr;
    stdMBR        sSegmentMbr;

    while ( sPrim != NULL )
    {
        sSeg1   = stdUtils::getNextSeg( sPrim->mFirst );
        sSeg2   = stdUtils::getPrevSeg( sPrim->mSecond );

        sChain1 = sPrim->mFirst->mParent;
        sChain2 = sPrim->mSecond->mParent;

        if ( ( sSeg1 == stdUtils::getPrevSeg( sSeg2 ) ) ||
             ( sSeg1 == stdUtils::getNextSeg( sSeg2 ) ) ||
             ( sChain1->mPolygonNum != sChain2->mPolygonNum ) )
        {
            /* sSeg2는 sSeg1의 이전,다음 세그먼트가 아니어야 한다.
             * 만약 그렇다면 이미 분할이 일어난 후 이므로
             * 이것을 무시하고 다음으로 넘어간다. 
             * 두 세그먼트의 링 번호가 다른 경우에도 다음으로 넘어간다. */

            sPrim = sPrim->mNext;
            continue;
        }
        else
        {
            // Nothing to do
        }

        /* Clip 결과에서 셀프터치가 있는 링을 제거한다. */
        IDE_TEST( removeRingFromList( aResRingList, sChain1->mPolygonNum )
                  != IDE_SUCCESS );

        /* 셀프터치가 있는 점으로 들어가는 세그먼트와 바로 그 다음 세그먼트의
         * 부모(체인)이 같은 경우, Chain을 분리해야 한다. */
        if ( sSeg1->mParent == sChain1 )
        {
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Chain), 
                                      (void**) &sNewChain1 )
                      != IDE_SUCCESS );

            if ( sChain1->mReverse == ST_NOT_REVERSE )
            {
                stdUtils::initChain( sSeg1,
                                     sNewChain1, 
                                     sChain1->mVaritation,
                                     sChain1->mReverse,
                                     0,                         /* Ring 번호, 사용 안함 */
                                     aResRingList->mRingCnt,
                                     sChain1->mOrientaion,
                                     sChain1->mOpposite );

                sNewChain1->mEnd          = sChain1->mEnd;

                sChain1->mEnd             = sSeg1->mPrev;
                sChain1->mEnd->mNext      = NULL;

                sNewChain1->mBegin->mPrev = NULL;

                while ( sSeg1 != NULL )
                {
                    sSeg1->mParent = sNewChain1;
                    sSeg1          = sSeg1->mNext;
                }
            }
            else
            {
                stdUtils::initChain( sChain1->mBegin, 
                                     sNewChain1,
                                     sChain1->mVaritation,
                                     sChain1->mReverse,
                                     0,
                                     aResRingList->mRingCnt,
                                     sChain1->mOrientaion,
                                     sChain1->mOpposite );

                sNewChain1->mEnd        = sSeg1;

                sChain1->mBegin         = sSeg1->mNext;
                sChain1->mBegin->mPrev  = NULL;

                sNewChain1->mEnd->mNext = NULL;

                while ( sSeg1 != NULL )
                {
                    sSeg1->mParent = sNewChain1;
                    sSeg1          = sSeg1->mPrev;
                }
            }

            sNewChain1->mNext = sChain1->mNext;
        }
        else
        {
            sNewChain1 = sSeg1->mParent;
        }
        sChain1->mNext = NULL;

        /* 셀프터치가 있는 점으로 들어가는 세그먼트와 바로 그 다음 세그먼트의
         * 부모(체인)이 같은 경우, Chain을 분리해야 한다. */
        if ( sSeg2->mParent == sChain2 )
        {
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Chain), 
                                      (void**) &sNewChain2 )
                      != IDE_SUCCESS );

            if ( sChain2->mReverse == ST_NOT_REVERSE )
            {
                stdUtils::initChain( sChain2->mBegin,
                                     sNewChain2, 
                                     sChain2->mVaritation,
                                     sChain2->mReverse,
                                     0,
                                     aResRingList->mRingCnt,
                                     sChain2->mOrientaion,
                                     sChain2->mOpposite );
                
                sNewChain2->mEnd        = sSeg2;
                
                sChain2->mBegin         = sSeg2->mNext;
                sChain2->mBegin->mPrev  = NULL;

                sNewChain2->mEnd->mNext = NULL;

                while ( sSeg2 != NULL )
                {
                    sSeg2->mParent = sNewChain2;
                    sSeg2          = sSeg2->mPrev;
                }
            }
            else
            {
                stdUtils::initChain( sSeg2,
                                     sNewChain2,
                                     sChain2->mVaritation,
                                     sChain2->mReverse,
                                     0,
                                     aResRingList->mRingCnt,
                                     sChain2->mOrientaion,
                                     sChain2->mOpposite );

                sNewChain2->mEnd          = sChain2->mEnd;
                
                sChain2->mEnd             = sSeg2->mPrev;
                sChain2->mEnd->mNext      = NULL;

                sNewChain2->mBegin->mPrev = NULL;

                while ( sSeg2 != NULL )
                {
                    sSeg2->mParent = sNewChain2;
                    sSeg2          = sSeg2->mNext;
                }
            }

            sNewChain2->mPrev = sChain2->mPrev;
        }
        else
        {
            sNewChain2 = sSeg2->mParent;
        }
        sChain2->mPrev = NULL;

        /* 두 개의 링으로 분리되도록 체인을 수정한다. */
        sChain1->mNext = sChain2;
        sChain2->mPrev = sChain1;

        sNewChain1->mPrev = sNewChain2;
        sNewChain2->mNext = sNewChain1;

        if ( sNewChain1->mNext == sChain2 )
        {
            sNewChain1->mNext = sNewChain2;
        }
        else
        {
            // Nothing to do 
        }

        if ( sNewChain2->mPrev == sChain1 )
        {
            sNewChain2->mPrev = sNewChain1;
        }
        else
        {
            // Nohting to do   
        }

        sNewChain1->mNext->mPrev = sNewChain1;
        sNewChain2->mPrev->mNext = sNewChain2;

        /* 분리된 두 링의 MBR과 방향을 다시 계산하고, 
         * Clip 결과에 다시 등록 한다. */

        sFirstSeg[0] = sChain1->mBegin;
        sFirstSeg[1] = sNewChain1->mBegin;

        for ( i = 0 ; i < 2 ; i++ )
        {
            sSeg1     = sFirstSeg[i];
            sPointCnt = 0;
            sMaxSeg   = sSeg1;

            stdUtils::initMBR( &sTotalMbr );

            if ( sSeg1->mParent->mReverse == ST_NOT_REVERSE )
            {
                sMaxPt = &sSeg1->mStart;
            }
            else
            {
                sMaxPt = &sSeg1->mEnd;
            }

            do
            {
                if ( sSeg1->mParent->mReverse == ST_NOT_REVERSE )
                {
                    sNextPt = &sSeg1->mEnd;
                }
                else
                {
                    sNextPt = &sSeg1->mStart;
                }

                if ( sMaxPt->mY < sNextPt->mY )
                {
                    sMaxSeg = sSeg1;
                    sMaxPt  = sNextPt;
                }
                else
                {
                    if ( sMaxPt->mY == sNextPt->mY )
                    {
                        if ( sMaxPt->mX <= sNextPt->mX )
                        {
                            sMaxSeg = sSeg1;
                            sMaxPt  = sNextPt;
                        }
                        else
                        {
                            // Nothing to do
                        }
                    }
                    else
                    {
                        // Nothing to do
                    }
                }

                sSegmentMbr.mMinX = sSeg1->mStart.mX;
                sSegmentMbr.mMaxX = sSeg1->mEnd.mX;
                if( sSeg1->mStart.mY < sSeg1->mEnd.mY )
                {
                    sSegmentMbr.mMinY = sSeg1->mStart.mY;
                    sSegmentMbr.mMaxY = sSeg1->mEnd.mY;
                }
                else
                {
                    sSegmentMbr.mMinY = sSeg1->mEnd.mY;
                    sSegmentMbr.mMaxY = sSeg1->mStart.mY;
                }

                stdUtils::mergeOrMBR( &sTotalMbr, &sTotalMbr, &sSegmentMbr );
               
                sPointCnt++;
                sSeg1 = stdUtils::getNextSeg( sSeg1 );

            }while ( sSeg1 != sFirstSeg[i] );

            sChain1 = sFirstSeg[i]->mParent;

            do
            {
                sChain1->mPolygonNum = aResRingList->mRingCnt;
                sChain1              = sChain1->mNext;
            }while ( sChain1 != sFirstSeg[i]->mParent );


            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Ring),
                                      (void**) &sRing )
                      != IDE_SUCCESS );

            sRing->mPrev       = NULL;
            sRing->mNext       = NULL;
            sRing->mParent     = NULL;
            sRing->mFirstSeg   = sFirstSeg[i];
            sRing->mRingNumber = aResRingList->mRingCnt;
            sRing->mPointCnt   = sPointCnt;
            stdUtils::copyMBR( &(sRing->mMBR), &sTotalMbr );

            if ( sMaxSeg->mParent->mReverse == ST_NOT_REVERSE )
            {
                sRing->mOrientation = stdUtils::CCW( *getPrevPoint( sMaxSeg, ID_FALSE ),
                                                     sMaxSeg->mEnd,
                                                     *getNextPoint ( sMaxSeg, ID_FALSE) );
            }
            else
            {
                sRing->mOrientation = stdUtils::CCW( *getPrevPoint( sMaxSeg, ID_TRUE ),
                                                     sMaxSeg->mStart,
                                                     *getNextPoint( sMaxSeg, ID_TRUE ) );
            }

            if ( sRing->mOrientation == ST_COUNTERCLOCKWISE )
            {
                appendFirst( aResRingList, sRing );
            }
            else
            {
                appendLast( aResRingList, sRing );
            }
        }

        sPrim = sPrim->mNext;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stdPolyClip::unionPolygons2D( iduMemory*          aQmxMem,
                                     stdGeometryHeader** aGeoms,
                                     SInt                aCnt,
                                     stdGeometryHeader*  aRet,
                                     UInt                aFence )
{
    SInt i;
    idlOS::memcpy( aRet, aGeoms[0], STD_GEOM_SIZE(aGeoms[0]) );
    for ( i = 1 ; i < aCnt ; i++ )
    {
        IDE_TEST( stfAnalysis::getUnionAreaArea2D4Clip( aQmxMem,
                                                        (stdGeometryType*) aRet,
                                                        (stdGeometryType*) aGeoms[i],
                                                        aRet,
                                                        aFence )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
