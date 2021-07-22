/** 
 * Copyright: (C) Advanced Interfaces Group,
 *            University of Manchester.
 *
 *            This software is free for non-commercial use. It may be copied,
 *            modified, and redistributed provided that this copyright notice
 *            is preserved on all copies. The intellectual property rights of
 *            the algorithms used reside with the University of Manchester
 *            Advanced Interfaces Group.
 *
 *            You may not use this software, in whole or in part, in support
 *            of any commercial product without the express consent of the
 *            author.
 *
 *            There is no warranty or other guarantee of fitness of this
 *            software for any purpose. It is provided solely "as is".
 */
 

/***********************************************************************
 * $Id: stdGpc.h 18883 2007-04-11 01:48:40Z leekmo $
 *
 * Description:
 *
 *    BUG-17180
 *
 *    GPC(General Polygon Clipper) 알고리즘을
 *        altibase coding convention에 맞게 수정함.
 *
 *    KGI에서 개발당시 도입한 코드로
 *        해당 코드에 대한 이해가 없는 상태에서 포팅되었음.
 *        역시나 leekmo 도 해당 코드에 대한 이해가 전무함.
 *
 **********************************************************************/

#ifndef _O_STD_GPC_H_
#define _O_STD_GPC_H_        1

#include <idTypes.h>

//===================================================================
//                             Constants
//      cf) Original Code 의 정보 관리를 위해 주석 처리하고 남겨둠.
//===================================================================

// #define GPC_VERSION "2.32"

// Increase GPC_EPSILON to encourage merging of near coincident edges
// #define STD_GPC_EPSILON (DBL_EPSILON)

// Equal to MATH_TOL in stdCommon.h
#define  STD_GPC_EPSILON         (0.0000001)

#define  STD_GPC_CHUNK_MAX       510

//===================================================================
//                         Public Data Types
//===================================================================

typedef enum                         /* Set operation type          */
{
    STD_GPC_DIFF = 0,                /* Difference                  */
    STD_GPC_INT,                     /* Intersection                */
    STD_GPC_XOR,                     /* Exclusive or                */
    STD_GPC_UNION,                   /* Union                       */
    STD_GPC_MAX_NONE                 /* No Operation                */
} stdGpcOP;

typedef struct                       /* Polygon vertex structure    */
{
    SDouble            mX;           /* Vertex x component          */
    SDouble            mY;           /* Vertex y component          */
} stdGpcVertex;

typedef struct                       /* Vertex list structure       */
{
    SInt               mNumVertices; /* Number of vertices in list  */
    stdGpcVertex     * mVertex;      /* Vertex array pointer        */
} stdGpcVertexList;

typedef struct                       /* Polygon set structure       */
{
    SInt               mNumContours; /* Number of contours in polygon  */
    SInt             * mHole;        /* Hole / external contour flags  */
    stdGpcVertexList * mContour;     /* Contour array pointer          */
} stdGpcPolygon;

typedef struct                       /* Tristrip set structure      */
{
    SInt               mNumStrips;   /* Number of tristrips         */
    stdGpcVertexList * mStrip;       /* Tristrip array pointer      */
} stdGpcTristrip;

//===================================================================
//                          Private Data Types
//===================================================================

typedef enum                        /* Edge intersection classes     */
{
    STD_GPC_NUL = 0,                /* Empty non-intersection        */
    STD_GPC_EMX,                    /* External maximum              */
    STD_GPC_ELI,                    /* External left intermediate    */
    STD_GPC_TED,                    /* Top edge                      */
    STD_GPC_ERI,                    /* External right intermediate   */
    STD_GPC_RED,                    /* Right edge                    */
    STD_GPC_IMM,                    /* Internal maximum and minimum  */
    STD_GPC_IMN,                    /* Internal minimum              */
    STD_GPC_EMN,                    /* External minimum              */
    STD_GPC_EMM,                    /* External maximum and minimum  */
    STD_GPC_LED,                    /* Left edge                     */
    STD_GPC_ILI,                    /* Internal left intermediate    */
    STD_GPC_BED,                    /* Bottom edge                   */
    STD_GPC_IRI,                    /* Internal right intermediate   */
    STD_GPC_IMX,                    /* Internal maximum              */
    STD_GPC_FUL                     /* Full non-intersection         */
} stdGpcVertexType;

typedef enum                        /* Horizontal edge states        */
{
    STD_GPC_NH = 0,                 /* No horizontal edge            */
    STD_GPC_BH,                     /* Bottom horizontal edge        */
    STD_GPC_TH                      /* Top horizontal edge           */
} stdGpcHState;

typedef enum                        /* Edge bundle state                 */
{
    STD_GPC_UNBUNDLED = 0,          /* Isolated edge not within a bundle */
    STD_GPC_BUNDLE_HEAD,            /* Bundle head node                  */
    STD_GPC_BUNDLE_TAIL             /* Passive bundle tail node          */
} stdGpcBundleState;

typedef struct stdGpcVShape         /* Internal vertex list datatype  */
{
    SDouble               mX;      /* X coordinate component         */
    SDouble               mY;       /* Y coordinate component         */
    struct stdGpcVShape * mNext;    /* Pointer to next vertex in list */
} stdGpcVertexNode;

typedef struct stdGpcPShape         /* Internal contour / tristrip type  */
{
    SInt                  mActive;  /* Active flag / vertex count        */
    SInt                  mHole;    /* Hole / external contour flag      */
    stdGpcVertexNode    * mV[2];    /* Left and right vertex list ptrs   */
    struct stdGpcPShape * mNext;    /* Pointer to next polygon contour   */
    struct stdGpcPShape * mProxy;   /* Pointer to actual structure used  */
} stdGpcPolygonNode;

typedef struct stdGpcEdgeShape
{
    stdGpcVertex          mVertex;  /* Piggy-backed contour vertex data  */
    stdGpcVertex          mBot;     /* Edge lower (x, y) coordinate      */
    stdGpcVertex          mTop;     /* Edge upper (x, y) coordinate      */
    SDouble               mXB;      /* Scanbeam bottom x coordinate      */
    SDouble               mXT;      /* Scanbeam top x coordinate         */
    SDouble               mDX;      /* Change in x for a unit y increase */
    SInt                  mType;    /* Clip / subject edge flag          */
    SInt                  mBundle[2][2]; /* Bundle edge flags            */
    SInt                  mBside[2];  /* Bundle left / right indicators  */
    stdGpcBundleState     mBstate[2]; /* Edge bundle state               */
    stdGpcPolygonNode   * mOutp[2]; /* Output polygon / tristrip pointer */
    struct stdGpcEdgeShape * mPrev; /* Previous edge in the AET          */
    struct stdGpcEdgeShape * mNext; /* Next edge in the AET              */
    struct stdGpcEdgeShape * mPred; /* Edge connected at the lower end   */
    struct stdGpcEdgeShape * mSucc; /* Edge connected at the upper end   */
    struct stdGpcEdgeShape * mNextBound; /* Pointer to next bound in LMT */
} stdGpcEdgeNode;

typedef struct stdGpcLMTShape       /* Local minima table                */
{
    SDouble                mY;      /* Y coordinate at local minimum     */
    stdGpcEdgeNode       * mFirstBound; /* Pointer to bound list         */
    struct stdGpcLMTShape *mNext;   /* Pointer to next local minimum     */
} stdGpcLMTNode;

typedef struct stdGpcSBTShape       /* Scanbeam tree                     */
{
    SDouble                 mY;     /* Scanbeam node y value             */
    struct stdGpcSBTShape * mLess;  /* Pointer to nodes with lower y     */
    struct stdGpcSBTShape * mMore;  /* Pointer to nodes with higher y    */
} stdGpcSBTree;

typedef struct stdGpcITShape        /* Intersection table                */
{
    stdGpcEdgeNode       * mIE[2];  /* Intersecting edge (bundle) pair   */
    stdGpcVertex           mPoint;  /* Point of intersection             */
    struct stdGpcITShape * mNext;   /* The next intersection table node  */
} stdGpcITNode;

typedef struct stdGpcSTShape        /* Sorted edge table                 */
{
    stdGpcEdgeNode       * mEdge;   /* Pointer to AET edge               */
    SDouble                mXB;     /* Scanbeam bottom x coordinate      */
    SDouble                mXT;     /* Scanbeam top x coordinate         */
    SDouble                mDX;     /* Change in x for a unit y increase */
    struct stdGpcSTShape * mPrev;   /* Previous edge in sorted list      */
} stdGpcSTNode;

typedef struct stdGpcBboxShape      /* Contour axis-aligned bounding box */
{
    SDouble              mXmin;   /* Minimum x coordinate              */
    SDouble              mYmin;   /* Minimum y coordinate              */
    SDouble              mXmax;   /* Maximum x coordinate              */
    SDouble              mYmax;   /* Maximum y coordinate              */
} stdGpcBBox;

typedef struct stdGpcChunk                       /* Scanbeam Chunk */
{
    SDouble              mY[STD_GPC_CHUNK_MAX];  /* Count  */
    ULong                mCount;                 
    struct stdGpcChunk * mNext;                  
}stdGpcChunk;

    

//===================================================================
//                     Public Function Prototypes
//===================================================================

class stdGpc
{
public:

    static IDE_RC polygon2tristrip( iduMemory       * aQmxMem,
                                    stdGpcPolygon   * aPolygon,
                                    stdGpcTristrip  * aTristrip );
    
    static IDE_RC addContour( iduMemory        * aQmxMem,
                              stdGpcPolygon    * aPolygon,
                              stdGpcVertexList * aContour,
                              SInt               aIsHole );

    static IDE_RC polygonClip( iduMemory       * aQmxMem,
                               stdGpcOP          aOP,
                               stdGpcPolygon   * aSubjPoly,
                               stdGpcPolygon   * aClipPoly,
                               stdGpcPolygon   * aOutPoly);

    static IDE_RC tristripClip( iduMemory       * aQmxMem,
                                stdGpcOP          aOP,
                                stdGpcPolygon   * aSubjPoly,
                                stdGpcPolygon   * aClipPoly,
                                stdGpcTristrip  * aOutTristrip );


private:
    
    static void insertBound( stdGpcEdgeNode ** aBoundNode,
                             stdGpcEdgeNode  * aNewNode );
    
    static IDE_RC boundList( iduMemory        * aQmxMem,
                             stdGpcLMTNode   ** aLMTNode,
                             SDouble            aY,
                             stdGpcEdgeNode *** aReturnEdge );

    static IDE_RC add2SChunk( iduMemory     * aQmxMem,
                              SInt         * aEntries,
                              stdGpcChunk **aChunk,
                              SDouble       aY );

    static void buildSChunk( SInt          * aEntries,
                             SDouble       * aSBT,
                             stdGpcChunk   * aChunk );

    // To Fix BUG-17179
    static SInt countOptimalVertices( stdGpcVertexList * aVertexList );

    static IDE_RC buildLMT( iduMemory       * aQmxMem,
                            stdGpcLMTNode  ** aLmtNode,
                            stdGpcChunk    ** aChunk,
                            SInt            * aSBTEntries,
                            stdGpcPolygon   * aPolygon,
                            SInt              aType,
                            stdGpcOP          aOP,
                            stdGpcEdgeNode ** aReturnEdge );
    
    static void addEdge2AET( stdGpcEdgeNode ** aAET,
                             stdGpcEdgeNode  * aEdge,
                             stdGpcEdgeNode  * aPrev );

    static IDE_RC addIntersection( iduMemory      * aQmxMem,
                                   stdGpcITNode  ** aITNode,
                                   stdGpcEdgeNode * aEdge0,
                                   stdGpcEdgeNode * aEdge1,
                                   SDouble          aX,
                                   SDouble          aY );
    
    static IDE_RC addSTEdge( iduMemory      * aQmxMem,
                             stdGpcSTNode  ** aSTNode,
                             stdGpcITNode  ** aITNode,
                             stdGpcEdgeNode * aEdge,
                             SDouble          aDY );
    
    static IDE_RC buildIntersectionTable( iduMemory      * aQmxMem,
                                          stdGpcITNode  ** aITNode,
                                          stdGpcEdgeNode * aAET,
                                          SDouble          aDY );
    
    static IDE_RC countContours( stdGpcPolygonNode * aPolygon,
                                 SInt              * aCount );
    
    static IDE_RC addLeft( iduMemory         * aQmxMem,
                           stdGpcPolygonNode * aPolygon,
                           SDouble             aX,
                           SDouble             aY );
    
    static IDE_RC mergeLeft( stdGpcPolygonNode * aPolygon1,
                           stdGpcPolygonNode * aPolygon2,
                           stdGpcPolygonNode * aPolygonList );
    
    static IDE_RC addRight( iduMemory         * aQmxMem,
                            stdGpcPolygonNode * aPolygon,
                            SDouble             aX,
                            SDouble             aY );
    
    static IDE_RC mergeRight( stdGpcPolygonNode * aPolygon1,
                            stdGpcPolygonNode * aPolygon2,
                            stdGpcPolygonNode * aPolygonList );
    
    static IDE_RC addLocalMin( iduMemory          * aQmxMem,
                               stdGpcPolygonNode ** aPolygon,
                               stdGpcEdgeNode     * aEdge,
                               SDouble              aX,
                               SDouble              aY );

    static SInt countTristrips( stdGpcPolygonNode * aNode );

    static IDE_RC addVertex( iduMemory         * aQmxMem,
                             stdGpcVertexNode ** aVertex,
                             SDouble             aX,
                             SDouble             aY );

    static IDE_RC newTristrip( iduMemory          * aQmxMem,
                               stdGpcPolygonNode ** aTristrip,
                               stdGpcEdgeNode     * aEdge,
                               SDouble              aX,
                               SDouble              aY );

    static IDE_RC createContourBBoxes( iduMemory     * aQmxMem,
                                       stdGpcPolygon * aPolygon,
                                       stdGpcBBox   ** aResult );
    
    static IDE_RC miniMaxTest( iduMemory     * aQmxMem,
                               stdGpcPolygon * aSubj,
                               stdGpcPolygon * aClip,
                               stdGpcOP        aOP );
};

extern "C" SInt cmpDouble(const void * aV1, const void * aV2);

#endif /* _O_STD_GPC_H_ */


