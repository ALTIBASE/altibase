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
 
#ifndef _O_STD_POLYCLIP_H_
#define _O_STD_POLYCLIP_H_ 1

typedef Segment *SegmentList;
typedef Chain   *ChainList;
typedef SInt     Direction;

typedef idBool (*SegmentRule)( Segment*, 
                               Direction*,
                               UInt );

// BUG-33634 
typedef struct Ring
{
    struct Ring* mPrev;
    struct Ring* mNext;
    struct Ring* mParent;
    Segment*     mFirstSeg;
    stdMBR       mMBR;
    UInt         mRingNumber;
    UInt         mPointCnt;
    SInt         mOrientation;
} Ring;

typedef struct ResRingList
{
    Ring*  mBegin;
    Ring*  mEnd;
    UInt   mRingCnt;
} ResRingList;

class stdPolyClip{
public:
    static void     addSegment( Chain*   aList, 
                                Segment* aSeg );
    
    static void     addChain( ChainList* aList, 
                              Chain*     aChain );

    static void     addVertex( Vertex*  aList, 
                               Segment* aSeg );
    
    static void     sortVertexList( Vertex* aList );
    
    static SDouble  getSegmentAngle( Segment*   aSeg, 
                                     stdPoint2D mCoord );
    
    static idBool   segmentRuleUnion( Segment*   aSeg,
                                      Direction* aDir, 
                                      UInt       aPN  );

    static idBool   segmentRuleIntersect( Segment*   aSeg,
                                          Direction* aDir,
                                          UInt       aPN );

    static idBool   segmentRuleDifference( Segment*   aSeg,
                                           Direction* aDir,
                                           UInt       aPN );

    static idBool   segmentRuleSymDifference( Segment*   aSeg,
                                              Direction* aDir,
                                              UInt       aPN );

    static IDE_RC   clip( iduMemory*   aQmxMem,
                          Segment**    aRingList,
                          UInt         aRingCount,
                          ResRingList* aResRingList,
                          Segment**    aIndexSegList,
                          UInt*        aIndexSegCount,
                          SegmentRule  aRule,
                          UInt         aPN,
                          UChar*       aCount );

    static IDE_RC   collect( iduMemory*   aQmxMem,
                             ResRingList* aResRingList,
                             UInt         aRingNumber,
                             Segment**    aIndexSegList,
                             UInt*        aIndexSegCount,
                             Segment*     aSeg,
                             SegmentRule  arule, 
                             Direction*   aDir,
                             UInt         aPN,
                             UChar*       aCount );

    static idBool   isIntersectPoint( Segment*  aSeg, 
                                      Direction aDir );

    static Segment* jump( Segment*    aSeg,
                          SegmentRule aRule,
                          Direction*  aDir,
                          UInt        aPN,
                          UChar*      aCount );

    static IDE_RC   copyOf( iduMemory* aQmxMem, 
                            Segment*   aSeg, 
                            Direction  aDir,
                            Segment**  aRes );

    static IDE_RC   labelingSegment( Segment** aRingSegList,
                                     UInt      aRingCount, 
                                     Segment** aIndexSeg,
                                     UInt      aIndexSegTotal,
                                     UInt      aMax1,
                                     UInt      aMax2,
                                     UChar*    aCount );

    static IDE_RC   chainToRing( Segment*         aFirstSeg,
                                 UInt             aPointCnt,
                                 stdLinearRing2D* aRes,
                                 UInt             aFence );

    static IDE_RC   makeGeometryFromRings( ResRingList*       aResRingList, 
                                           stdGeometryHeader* aRes,
                                           UInt               aFence );

    static void     determineInside( Segment** aIndexSeg,
                                     UInt      aIndexSegTotal,
                                     Segment*  aSeg,
                                     UInt      aMax1,
                                     UInt      aMax2, 
                                     UChar*    aCount );

    static void     determineShare( Segment* aSeg );

    // BUG-33436
    static idBool   isSameDirection( Direction aDir1, 
                                     Direction aDir2 );

    // BUG-33634
    static Direction getDirection( Segment* aSeg );

    static void     adjustRingOrientation( Segment** aRingSegList, 
                                           UInt      aRingCount,
                                           Segment** aIndexSeg,
                                           UInt      aIndexSegTotal,
                                           UInt      aMax1, 
                                           UInt      aMax2 );

    static void appendFirst( ResRingList* aResRingList, 
                             Ring*        aRing );

    static void appendLast( ResRingList* aResRingList, 
                            Ring*        aRing );
    
    static IDE_RC removeRingFromList( ResRingList* aResRingList, 
                                      UInt         aRingNum );

    static stdPoint2D* getNextPoint( Segment* aSeg, 
                                     idBool   aIsStart );

    static stdPoint2D* getPrevPoint( Segment* aSeg, 
                                     idBool   aIsStart );
    
    static IDE_RC sortRings( ResRingList* aResRingList,
                             Segment**    aIndexSeg, 
                             UInt         aIndexSegTotal );

    static IDE_RC detectSelfTouch( iduMemory*     aQmxMem,
                                   Segment**      aIndexSeg,
                                   UInt           aIndexSegTotal,
                                   PrimInterSeg** aResPrimInterSeg);

    static IDE_RC breakRing( iduMemory*    aQmxMem,
                             ResRingList*  aResRingList,
                             PrimInterSeg* aResPrimInterSeg );

    // BUG-33904
    static IDE_RC unionPolygons2D( iduMemory*          aQmxMem,
                                   stdGeometryHeader** aGeoms,
                                   SInt                aCnt,
                                   stdGeometryHeader*  aRet,
                                   UInt                aFence );
};

#endif // _O_STD_POLYCLIP_H_
