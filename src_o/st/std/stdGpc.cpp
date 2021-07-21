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
 * $Id: stdGpc.cpp 18883 2007-04-11 01:48:40Z leekmo $
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

#include <idl.h>
#include <ide.h>

#include <iduMemory.h>

#include <ste.h>
#include <stdGpc.h>


//=====================================================================
//                              Constants
//=====================================================================
                                
#define STD_GPC_LEFT              (0)
#define STD_GPC_RIGHT             (1)

#define STD_GPC_ABOVE             (0)
#define STD_GPC_BELOW             (1)

#define STD_GPC_CLIP              (0)
#define STD_GPC_SUBJ              (1)

#define STD_GPC_FALSE             (0)
#define STD_GPC_TRUE              (1)

#define STD_GPC_INVERT_TRISTRIPS  (STD_GPC_FALSE)

//=====================================================================
//                              Macros
//=====================================================================

#define STD_GPC_EQ(a, b)          \
            ( (idlOS::fabs((a) - (b)) <= STD_GPC_EPSILON) ? \
              ID_TRUE : ID_FALSE )

#define STD_GPC_PREV_INDEX(i, n)  ((i - 1 + n) % n)
#define STD_GPC_NEXT_INDEX(i, n)  ((i + 1    ) % n)

#define STD_GPC_OPTIMAL(v, i, n)  \
            ( ((v[STD_GPC_PREV_INDEX(i, n)].mY != v[i].mY) || \
               (v[STD_GPC_NEXT_INDEX(i, n)].mY != v[i].mY)) ? \
              ID_TRUE : ID_FALSE )

#define STD_GPC_FWD_MIN(v, i, n)  \
            ( ((v[STD_GPC_PREV_INDEX(i, n)].mVertex.mY >= v[i].mVertex.mY) && \
               (v[STD_GPC_NEXT_INDEX(i, n)].mVertex.mY > v[i].mVertex.mY)) ?  \
              ID_TRUE : ID_FALSE )

#define STD_GPC_NOT_FMAX(v, i, n) \
            ( (v[STD_GPC_NEXT_INDEX(i, n)].mVertex.mY > v[i].mVertex.mY) ? \
              ID_TRUE : ID_FALSE )

#define STD_GPC_REV_MIN(v, i, n)  \
            (((v[STD_GPC_PREV_INDEX(i, n)].mVertex.mY > v[i].mVertex.mY) &&  \
              (v[STD_GPC_NEXT_INDEX(i, n)].mVertex.mY >= v[i].mVertex.mY)) ? \
             ID_TRUE : ID_FALSE )

#define STD_GPC_NOT_RMAX(v, i, n) \
            ((v[STD_GPC_PREV_INDEX(i, n)].mVertex.mY > v[i].mVertex.mY) ? \
             ID_TRUE : ID_FALSE )

#define STD_GPC_VERTEX(m, e,p,s,x,y)                        \
            {IDE_TEST( addVertex(m, &((e)->mOutp[(p)]->mV[(s)]), x, y) != IDE_SUCCESS); \
             (e)->mOutp[(p)]->mActive++;}

#define STD_GPC_P_EDGE(d,e,p,i,j) \
            {(d)= (e); \
             do {(d)= (d)->mPrev;} while (!(d)->mOutp[(p)]); \
             (i)= (d)->mBot.mX + (d)->mDX * ((j)-(d)->mBot.mY);}

#define STD_GPC_N_EDGE(d,e,p,i,j) \
            {(d)= (e); \
             do {(d)= (d)->mNext;} while (!(d)->mOutp[(p)]); \
             (i)= (d)->mBot.mX + (d)->mDX * ((j)-(d)->mBot.mY);}

//=====================================================================
//                             Global Data
//=====================================================================

/* Horizontal edge state transitions within scanbeam boundary */
const stdGpcHState stdGpcNextHState[3][6]=
{
  /*        ABOVE                 BELOW                 CROSS        */
  /*        L          R          L          R          L          R */  
  /* NH */ {STD_GPC_BH,STD_GPC_TH,STD_GPC_TH,STD_GPC_BH,STD_GPC_NH,STD_GPC_NH},
  /* BH */ {STD_GPC_NH,STD_GPC_NH,STD_GPC_NH,STD_GPC_NH,STD_GPC_TH,STD_GPC_TH},
  /* TH */ {STD_GPC_NH,STD_GPC_NH,STD_GPC_NH,STD_GPC_NH,STD_GPC_BH,STD_GPC_BH}
};

//=========================================================================
//                             Public Functions
//=========================================================================

IDE_RC
stdGpc::polygon2tristrip( iduMemory       * aQmxMem,
                          stdGpcPolygon   * aPolygon,
                          stdGpcTristrip  * aTristrip )
{
    stdGpcPolygon sPoly;

    sPoly.mNumContours = 0;
    sPoly.mHole = NULL;
    sPoly.mContour = NULL;
    IDE_TEST( tristripClip( aQmxMem,
                            STD_GPC_DIFF,
                            aPolygon,
                            & sPoly,
                            aTristrip ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::addContour( iduMemory        * aQmxMem,
                    stdGpcPolygon    * aPolygon,
                    stdGpcVertexList * aContour,
                    SInt               aIsHole )
{
    SInt               c = 0;
    SInt               v;
    SInt             * sExtendHole;
    stdGpcVertexList * sExtendContour;

    sExtendHole = NULL;
    sExtendContour = NULL;
    
    /* Create an extended hole array */
    IDE_TEST( aQmxMem->alloc( (aPolygon->mNumContours + 1)
                              * ID_SIZEOF(SInt),
                              (void **) & sExtendHole )
              != IDE_SUCCESS );

    /* Create an extended contour array */
    IDE_TEST( aQmxMem->alloc( (aPolygon->mNumContours + 1)
                              * ID_SIZEOF(stdGpcVertexList),
                              (void **) & sExtendContour )
              != IDE_SUCCESS );

    /* Copy the old contour and hole data into the extended arrays */
    for ( c = 0; c < aPolygon->mNumContours; c++)
    {
        sExtendHole[c] = aPolygon->mHole[c];
        sExtendContour[c] = aPolygon->mContour[c];
    }

    /* Copy the new contour and hole onto the end of the extended arrays */
    c = aPolygon->mNumContours;
    sExtendHole[c] = aIsHole;
    sExtendContour[c].mNumVertices = aContour->mNumVertices;

    IDE_TEST( aQmxMem->alloc( aContour->mNumVertices
                              * ID_SIZEOF( stdGpcVertex ),
                              (void **) & sExtendContour[c].mVertex )
              != IDE_SUCCESS );

    for ( v = 0; v < aContour->mNumVertices; v++ )
    {
        sExtendContour[c].mVertex[v] = aContour->mVertex[v];
    }

    /* Dispose of the old contour */
    aPolygon->mContour = NULL;
    aPolygon->mHole = NULL;

    /* Update the polygon information */
    aPolygon->mNumContours++;
    aPolygon->mHole = sExtendHole;
    aPolygon->mContour = sExtendContour;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::polygonClip( iduMemory       * aQmxMem,
                     stdGpcOP          aOP,
                     stdGpcPolygon   * aSubjPoly,
                     stdGpcPolygon   * aClipPoly,
                     stdGpcPolygon   * aOutPoly)
{
    stdGpcChunk   * sChunk  = NULL;

    stdGpcITNode  * sITList = NULL;
    stdGpcITNode  * sITNode;

    stdGpcEdgeNode   * sEdgeNode;
    stdGpcEdgeNode   * sPrevEdge;
    stdGpcEdgeNode   * sNextEdge;
    stdGpcEdgeNode   * sSuccEdge;
    stdGpcEdgeNode   * sEdge0;
    stdGpcEdgeNode   * sEdge1;

    stdGpcEdgeNode   * sAET = NULL;
    stdGpcEdgeNode   * sClipHeap = NULL;
    stdGpcEdgeNode   * sSubjHeap = NULL;

    stdGpcLMTNode * sLMT = NULL;
    stdGpcLMTNode * sLocalMin;

    stdGpcPolygonNode * sOutPoly = NULL;
    stdGpcPolygonNode * sPoly1;
    stdGpcPolygonNode * sPoly2;
    stdGpcPolygonNode * sPoly;
    stdGpcPolygonNode * sNextPoly;
    stdGpcPolygonNode * sCFPoly = NULL;

    stdGpcVertexNode  * sVertex;
    stdGpcVertexNode  * sNextVtx;

    stdGpcHState   sHoriz[2];

    SInt           sInside[2];
    SInt           sExistCnt[2];
    SInt           sParity[2] = {STD_GPC_LEFT, STD_GPC_LEFT};

    SInt           c, v;
    SInt           sContribute;
    SInt           sSearch;
    SInt           sScanBeam = 0;
    SInt           sSBTEntries = 0;

    SInt           sVClass;
    SInt           sBL;
    SInt           sBR;
    SInt           sTL;
    SInt           sTR;

    SDouble      * sSBT = NULL;
    SDouble        sXB;
    SDouble        sPX;
    SDouble        sYB;
    SDouble        sYT = 0;
    SDouble        sDY;
    SDouble        sIX;
    SDouble        sIY;

    SInt           sTypePos;
    SInt           sOppoPos;
    
    /* Test for trivial NULL result cases */
    if ( ((aSubjPoly->mNumContours == 0) && (aClipPoly->mNumContours == 0)) ||
         ((aSubjPoly->mNumContours == 0) &&
          ((aOP == STD_GPC_INT) || (aOP == STD_GPC_DIFF))) ||
         ((aClipPoly->mNumContours == 0) &&  (aOP == STD_GPC_INT)))
    {
        aOutPoly->mNumContours = 0;
        aOutPoly->mHole = NULL;
        aOutPoly->mContour = NULL;
        IDE_RAISE( STD_GPC_POLYGON_GLIP_RETURN );
    }

    /* Identify potentialy contributing contours */
    if ( ((aOP == STD_GPC_INT) || (aOP == STD_GPC_DIFF)) &&
         (aSubjPoly->mNumContours > 0) &&
         (aClipPoly->mNumContours > 0) )
    {
        IDE_TEST( miniMaxTest( aQmxMem, aSubjPoly, aClipPoly, aOP ) != IDE_SUCCESS );
    }

    /* Build LMT */
    if ( aSubjPoly->mNumContours > 0 )
    {
        IDE_TEST( buildLMT( aQmxMem,
                            &sLMT,
                            &sChunk,
                            &sSBTEntries,
                            aSubjPoly,
                            STD_GPC_SUBJ,
                            aOP,
                            & sSubjHeap )
                  != IDE_SUCCESS );
    }
        
    if ( aClipPoly->mNumContours > 0)
    {
        IDE_TEST( buildLMT( aQmxMem,
                            &sLMT,
                            &sChunk,
                            &sSBTEntries,
                            aClipPoly,
                            STD_GPC_CLIP,
                            aOP,
                            & sClipHeap )
                  != IDE_SUCCESS );
    }
    

    /* Return a NULL result if no contours contribute */
    if ( sLMT == NULL )
    {
        aOutPoly->mNumContours = 0;
        aOutPoly->mHole= NULL;
        aOutPoly->mContour = NULL;
        
        sSubjHeap = NULL;
        sClipHeap = NULL;
        
        IDE_RAISE( STD_GPC_POLYGON_GLIP_RETURN );
    }

    /* Build scanbeam table from scanbeam tree */
    IDE_TEST( aQmxMem->alloc( sSBTEntries * ID_SIZEOF(SDouble),
                              (void**) & sSBT )
              != IDE_SUCCESS );

    buildSChunk( &sSBTEntries , sSBT, sChunk );
    sScanBeam = 0;

    /* Allow pointer re-use without causing memory leak */
    if ( aSubjPoly == aOutPoly )
    {
        aSubjPoly = NULL;
    }
    
    if ( aClipPoly == aOutPoly )
    {
        aClipPoly = NULL;
    }

    /* Invert clip polygon for difference operation */
    if ( aOP == STD_GPC_DIFF )
    {
        sParity[STD_GPC_CLIP] = STD_GPC_RIGHT;
    }

    sLocalMin= sLMT;
    
    if ( sSBTEntries < 2 )
    {
        sYT = sSBT[0];
        sDY= 0;
    }
    
    /* Process each scanbeam */
    while (sScanBeam < sSBTEntries)
    {
        /* Set yb and yt to the bottom and top of the scanbeam */
        sYB = sSBT[sScanBeam++];
        if (sScanBeam < sSBTEntries)
        {
            sYT= sSBT[sScanBeam];
            sDY= sYT - sYB;
        }

        /* === SCANBEAM BOUNDARY PROCESSING ================================ */

        /* If LMT node corresponding to yb exists */
        if ( sLocalMin != NULL )
        {
            if (sLocalMin->mY == sYB)
            {
                /* Add edges starting at this local minimum to the AET */
                for ( sEdgeNode = sLocalMin->mFirstBound;
                      sEdgeNode != NULL;
                      sEdgeNode = sEdgeNode->mNextBound )
                {
                    addEdge2AET( &sAET, sEdgeNode, NULL);
                }

                sLocalMin = sLocalMin->mNext;
            }
        }
        if(sAET == NULL)
        {
            continue;
        }

        /* Set dummy previous x value */
        sPX = -DBL_MAX;

        /* Create bundles within AET */
        sEdge0 = sAET;
        sEdge1 = sAET;

        /* Set up bundle fields of first edge */
        sTypePos = ( sAET->mType == STD_GPC_TRUE) ? 1 : 0;
        sOppoPos = (sTypePos + 1) % 2;
    
        sAET->mBundle[STD_GPC_ABOVE][sTypePos] =
            (sAET->mTop.mY != sYB) ? STD_GPC_TRUE : STD_GPC_FALSE;
        sAET->mBundle[STD_GPC_ABOVE][sOppoPos] = STD_GPC_FALSE;
        sAET->mBstate[STD_GPC_ABOVE]= STD_GPC_UNBUNDLED;

        for ( sNextEdge = sAET->mNext;
              sNextEdge != NULL;
              sNextEdge = sNextEdge->mNext )
        {
            /* Set up bundle fields of next edge */
            sTypePos = (sNextEdge->mType == STD_GPC_TRUE) ? 1 : 0;
            sOppoPos = (sTypePos + 1) % 2;
            
            sNextEdge->mBundle[STD_GPC_ABOVE][sTypePos]
                = (sNextEdge->mTop.mY != sYB) ? STD_GPC_TRUE : STD_GPC_FALSE;
            sNextEdge->mBundle[STD_GPC_ABOVE][sOppoPos] = STD_GPC_FALSE;
            sNextEdge->mBstate[STD_GPC_ABOVE]= STD_GPC_UNBUNDLED;

            /* Bundle edges above the scanbeam boundary if they coincide */
            if ( sNextEdge->mBundle[STD_GPC_ABOVE][sTypePos] == STD_GPC_TRUE )
            {
                if ( STD_GPC_EQ( sEdge0->mXB, sNextEdge->mXB) &&
                     STD_GPC_EQ( sEdge0->mDX, sNextEdge->mDX) &&
                     (sEdge0->mTop.mY != sYB) )
                {
                    sNextEdge->mBundle[STD_GPC_ABOVE][sTypePos]
                        ^= sEdge0->mBundle[STD_GPC_ABOVE][sTypePos];
                    sNextEdge->mBundle[STD_GPC_ABOVE][sOppoPos]= 
                        sEdge0->mBundle[STD_GPC_ABOVE][sOppoPos];
                    
                    sNextEdge->mBstate[STD_GPC_ABOVE]= STD_GPC_BUNDLE_HEAD;
                    sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP]
                        = STD_GPC_FALSE;
                    sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]
                        = STD_GPC_FALSE;
                    sEdge0->mBstate[STD_GPC_ABOVE]= STD_GPC_BUNDLE_TAIL;
                }
                sEdge0= sNextEdge;
            }
        }
    
        sHoriz[STD_GPC_CLIP] = STD_GPC_NH;
        sHoriz[STD_GPC_SUBJ] = STD_GPC_NH;

        /* Process each edge at this scanbeam boundary */
        for ( sEdgeNode= sAET; sEdgeNode; sEdgeNode= sEdgeNode->mNext )
        {
            sExistCnt[STD_GPC_CLIP] =
                sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] + 
                ( sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_CLIP] << 1);
            sExistCnt[STD_GPC_SUBJ] =
                sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] + 
                ( sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ] << 1);

            if ( ( sExistCnt[STD_GPC_CLIP] > 0) ||
                 ( sExistCnt[STD_GPC_SUBJ] > 0) )
            {
                /* Set bundle side */
                sEdgeNode->mBside[STD_GPC_CLIP] = sParity[STD_GPC_CLIP];
                sEdgeNode->mBside[STD_GPC_SUBJ] = sParity[STD_GPC_SUBJ];

                /* Determine contributing status and quadrant occupancies */
                switch (aOP)
                {
                    case STD_GPC_DIFF:
                    case STD_GPC_INT:
                        sContribute =
                            ( sExistCnt[STD_GPC_CLIP] &&
                              ( sParity[STD_GPC_SUBJ] ||
                                sHoriz[STD_GPC_SUBJ] ) ) ||
                            ( sExistCnt[STD_GPC_SUBJ] &&
                              ( sParity[STD_GPC_CLIP] ||
                                sHoriz[STD_GPC_CLIP] ) ) ||
                            ( sExistCnt[STD_GPC_CLIP] &&
                              sExistCnt[STD_GPC_SUBJ] &&
                              ( sParity[STD_GPC_CLIP]
                                == sParity[STD_GPC_SUBJ] ));
                        sBR =
                            ( sParity[STD_GPC_CLIP] ) &&
                            ( sParity[STD_GPC_SUBJ] );
                        sBL =
                            ( sParity[STD_GPC_CLIP] ^
                              sEdgeNode->
                              mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ) &&
                            ( sParity[STD_GPC_SUBJ] ^
                              sEdgeNode->
                              mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] );
                        sTR =
                            ( sParity[STD_GPC_CLIP] ^
                              ( sHoriz[STD_GPC_CLIP] != STD_GPC_NH ) ) &&
                            ( sParity[STD_GPC_SUBJ] ^
                              ( sHoriz[STD_GPC_SUBJ] != STD_GPC_NH ) );
                        sTL =
                            ( sParity[STD_GPC_CLIP] ^
                              (sHoriz[STD_GPC_CLIP] != STD_GPC_NH ) ^
                              sEdgeNode->
                              mBundle[STD_GPC_BELOW][STD_GPC_CLIP] ) &&
                            ( sParity[STD_GPC_SUBJ] ^
                              ( sHoriz[STD_GPC_SUBJ] != STD_GPC_NH ) ^
                              sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ]);
                        break;
                        
                    case STD_GPC_XOR:
                        sContribute =
                            sExistCnt[STD_GPC_CLIP] || sExistCnt[STD_GPC_SUBJ];
                        sBR= (sParity[STD_GPC_CLIP]) ^ (sParity[STD_GPC_SUBJ]);
                        sBL=
                            ( sParity[STD_GPC_CLIP] ^
                              sEdgeNode->
                              mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ) ^
                            ( sParity[STD_GPC_SUBJ] ^
                              sEdgeNode->
                              mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] );
                        sTR=
                            ( sParity[STD_GPC_CLIP] ^
                              ( sHoriz[STD_GPC_CLIP] != STD_GPC_NH ) ) ^
                            ( sParity[STD_GPC_SUBJ] ^
                              ( sHoriz[STD_GPC_SUBJ] != STD_GPC_NH ) );
                        sTL =
                            ( sParity[STD_GPC_CLIP] ^
                              ( sHoriz[STD_GPC_CLIP] != STD_GPC_NH ) ^
                              sEdgeNode->
                              mBundle[STD_GPC_BELOW][STD_GPC_CLIP] ) ^
                            ( sParity[STD_GPC_SUBJ] ^
                              ( sHoriz[STD_GPC_SUBJ] != STD_GPC_NH ) ^
                              sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ]);
                        break;
                        
                    case STD_GPC_UNION:
                        sContribute =
                            ( sExistCnt[STD_GPC_CLIP] &&
                              ( !sParity[STD_GPC_SUBJ] ||
                                sHoriz[STD_GPC_SUBJ] ) ) ||
                            ( sExistCnt[STD_GPC_SUBJ] &&
                              ( !sParity[STD_GPC_CLIP] ||
                                sHoriz[STD_GPC_CLIP] ) ) ||
                            ( sExistCnt[STD_GPC_CLIP] &&
                              sExistCnt[STD_GPC_SUBJ] &&
                              ( sParity[STD_GPC_CLIP]
                                == sParity[STD_GPC_SUBJ]) );
                        sBR =
                            (sParity[STD_GPC_CLIP]) || (sParity[STD_GPC_SUBJ]);
                        sBL =
                            ( sParity[STD_GPC_CLIP] ^
                              sEdgeNode->
                              mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ) ||
                            ( sParity[STD_GPC_SUBJ] ^
                              sEdgeNode->
                              mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] );
                        sTR =
                            ( sParity[STD_GPC_CLIP] ^
                              ( sHoriz[STD_GPC_CLIP] != STD_GPC_NH ) ) ||
                            ( sParity[STD_GPC_SUBJ] ^
                              ( sHoriz[STD_GPC_SUBJ] != STD_GPC_NH ) );
                        sTL =
                            ( sParity[STD_GPC_CLIP] ^
                              ( sHoriz[STD_GPC_CLIP] != STD_GPC_NH ) ^
                              sEdgeNode->
                              mBundle[STD_GPC_BELOW][STD_GPC_CLIP] ) ||
                            ( sParity[STD_GPC_SUBJ] ^
                              ( sHoriz[STD_GPC_SUBJ] != STD_GPC_NH ) ^
                              sEdgeNode->
                              mBundle[STD_GPC_BELOW][STD_GPC_SUBJ] );
                        break;
                    default:
                        IDE_ASSERT(0);
                        break;
                }

                /* Update parity */
                sParity[STD_GPC_CLIP] ^=
                    sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP];
                sParity[STD_GPC_SUBJ] ^=
                    sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ];

                /* Update sHorizontal state */
                if ( sExistCnt[STD_GPC_CLIP] > 0 )
                {
                    sHoriz[STD_GPC_CLIP] =
                        stdGpcNextHState[sHoriz[STD_GPC_CLIP]]
                        [ ((sExistCnt[STD_GPC_CLIP] - 1) << 1)
                          + sParity[STD_GPC_CLIP] ];
                }
                
                if ( sExistCnt[STD_GPC_SUBJ] > 0)
                {
                    sHoriz[STD_GPC_SUBJ]=
                        stdGpcNextHState[sHoriz[STD_GPC_SUBJ]]
                        [ ((sExistCnt[STD_GPC_SUBJ] - 1) << 1)
                          + sParity[STD_GPC_SUBJ] ];
                }
                

                sVClass= sTR + (sTL << 1) + (sBR << 2) + (sBL << 3);

                if ( sContribute == STD_GPC_TRUE )
                {
                    sXB = sEdgeNode->mXB;

                    switch (sVClass)
                    {
                        case STD_GPC_EMN:
                        case STD_GPC_IMN:
                            IDE_TEST( addLocalMin( aQmxMem,
                                                   &sOutPoly,
                                                   sEdgeNode,
                                                   sXB,
                                                   sYB) != IDE_SUCCESS );
                            sPX = sXB;
                            sCFPoly = sEdgeNode->mOutp[STD_GPC_ABOVE];
                            break;
                        case STD_GPC_ERI:
                            if (sXB != sPX)
                            {
                                IDE_TEST( addRight(aQmxMem, sCFPoly, sXB, sYB)
                                          != IDE_SUCCESS );
                                sPX= sXB;
                            }
                            sEdgeNode->mOutp[STD_GPC_ABOVE] = sCFPoly;
                            sCFPoly = NULL;
                            break;
                        case STD_GPC_ELI:
                            IDE_TEST( addLeft( aQmxMem,
                                               sEdgeNode->mOutp[STD_GPC_BELOW],
                                               sXB,
                                               sYB) != IDE_SUCCESS );
                            sPX = sXB;
                            sCFPoly = sEdgeNode->mOutp[STD_GPC_BELOW];
                            break;
                        case STD_GPC_EMX:
                            if (sXB != sPX)
                            {
                                IDE_TEST( addLeft( aQmxMem, sCFPoly, sXB, sYB )
                                          != IDE_SUCCESS );
                                sPX= sXB;
                            }
                            IDE_TEST( mergeRight( sCFPoly,
                                        sEdgeNode->mOutp[STD_GPC_BELOW],
                                                  sOutPoly ) != IDE_SUCCESS );
                            sCFPoly = NULL;
                            break;
                        case STD_GPC_ILI:
                            if (sXB != sPX)
                            {
                                IDE_TEST( addLeft(aQmxMem, sCFPoly, sXB, sYB)
                                          != IDE_SUCCESS );
                                sPX= sXB;
                            }
                            sEdgeNode->mOutp[STD_GPC_ABOVE] = sCFPoly;
                            sCFPoly = NULL;
                            break;
                        case STD_GPC_IRI:
                            IDE_TEST( addRight( aQmxMem,
                                                sEdgeNode->mOutp[STD_GPC_BELOW],
                                                sXB,
                                                sYB )
                                      != IDE_SUCCESS );
                            
                            sPX = sXB;
                            sCFPoly = sEdgeNode->mOutp[STD_GPC_BELOW];
                            sEdgeNode->mOutp[STD_GPC_BELOW] = NULL;
                            break;
                        case STD_GPC_IMX:
                            if (sXB != sPX)
                            {
                                IDE_TEST( addRight(aQmxMem, sCFPoly, sXB, sYB)
                                          != IDE_SUCCESS );
                                sPX = sXB;
                            }
                            IDE_TEST( mergeLeft( sCFPoly,
                                       sEdgeNode->mOutp[STD_GPC_BELOW],
                                                 sOutPoly) != IDE_SUCCESS );
                            sCFPoly = NULL;
                            sEdgeNode->mOutp[STD_GPC_BELOW] = NULL;
                            break;
                        case STD_GPC_IMM:
                            if (sXB != sPX)
                            {
                                IDE_TEST( addRight(aQmxMem, sCFPoly, sXB, sYB)
                                          != IDE_SUCCESS );
                                sPX = sXB;
                            }
                            IDE_TEST( mergeLeft( sCFPoly,
                                       sEdgeNode->mOutp[STD_GPC_BELOW],
                                                sOutPoly ) != IDE_SUCCESS);
                            sEdgeNode->mOutp[STD_GPC_BELOW] = NULL;
                            IDE_TEST( addLocalMin( aQmxMem,
                                                   &sOutPoly,
                                                   sEdgeNode,
                                                   sXB,
                                                   sYB )
                                      != IDE_SUCCESS );
                            
                            sCFPoly = sEdgeNode->mOutp[STD_GPC_ABOVE];
                            break;
                        case STD_GPC_EMM:
                            if ( sXB != sPX )
                            {
                                IDE_TEST( addLeft(aQmxMem, sCFPoly, sXB, sYB)
                                          != IDE_SUCCESS );
                                sPX = sXB;
                            }
                            IDE_TEST ( mergeRight( sCFPoly,
                                        sEdgeNode->mOutp[STD_GPC_BELOW],
                                                   sOutPoly ) != IDE_SUCCESS );
                            sEdgeNode->mOutp[STD_GPC_BELOW] = NULL;
                            IDE_TEST( addLocalMin( aQmxMem,
                                                   &sOutPoly,
                                                   sEdgeNode,
                                                   sXB,
                                                   sYB) != IDE_SUCCESS );
                            sCFPoly= sEdgeNode->mOutp[STD_GPC_ABOVE];
                            break;
                        case STD_GPC_LED:
                            if ( sEdgeNode->mBot.mY == sYB )
                            {
                                IDE_TEST( addLeft( aQmxMem,
                                                   sEdgeNode->mOutp[STD_GPC_BELOW],
                                                   sXB,
                                                   sYB)
                                          != IDE_SUCCESS );
                            }
                            
                            sEdgeNode->mOutp[STD_GPC_ABOVE] =
                                sEdgeNode->mOutp[STD_GPC_BELOW];
                            sPX = sXB;
                            break;
                        case STD_GPC_RED:
                            if (sEdgeNode->mBot.mY == sYB)
                            {
                                IDE_TEST( addRight(aQmxMem,
                                                   sEdgeNode->mOutp[STD_GPC_BELOW],
                                                   sXB,
                                                   sYB)
                                          != IDE_SUCCESS );
                            }
                            
                            sEdgeNode->mOutp[STD_GPC_ABOVE] =
                                sEdgeNode->mOutp[STD_GPC_BELOW];
                            sPX = sXB;
                            break;
                        default:
                            break;
                    } /* End of switch */
                } /* End of contributing conditional */
            } /* End of edge exists conditional */
        } /* End of AET loop */

        /* Delete terminating edges from the AET, otherwise compute xt */
        for ( sEdgeNode = sAET;
              sEdgeNode != NULL;
              sEdgeNode = sEdgeNode->mNext )
        {
            if ( sEdgeNode->mTop.mY == sYB )
            {
                sPrevEdge = sEdgeNode->mPrev;
                sNextEdge = sEdgeNode->mNext;
                
                if (sPrevEdge != NULL)
                {
                    sPrevEdge->mNext = sNextEdge;
                }
                else
                {
                    sAET = sNextEdge;
                }
                
                if (sNextEdge != NULL)
                {
                    sNextEdge->mPrev = sPrevEdge;
                }
                

                /* Copy bundle head state to the adjacent tail edge
                   if required */
                if ( ( sEdgeNode->mBstate[STD_GPC_BELOW]
                       == STD_GPC_BUNDLE_HEAD ) &&
                     ( sPrevEdge != NULL ) )
                {
                    if ( sPrevEdge->mBstate[STD_GPC_BELOW]
                         == STD_GPC_BUNDLE_TAIL )
                    {
                        sPrevEdge->mOutp[STD_GPC_BELOW] =
                            sEdgeNode->mOutp[STD_GPC_BELOW];
                        sPrevEdge->mBstate[STD_GPC_BELOW] =
                            STD_GPC_UNBUNDLED;
                        if ( sPrevEdge->mPrev != NULL )
                        {
                            if ( sPrevEdge->mPrev->mBstate[STD_GPC_BELOW]
                                 == STD_GPC_BUNDLE_TAIL )
                            {
                                sPrevEdge->mBstate[STD_GPC_BELOW] =
                                    STD_GPC_BUNDLE_HEAD;
                            }
                        }
                    }
                }
            }
            else
            {
                if ( sEdgeNode->mTop.mY == sYT )
                {
                    sEdgeNode->mXT = sEdgeNode->mTop.mX;
                }
                else
                {
                    sEdgeNode->mXT = sEdgeNode->mBot.mX +
                        sEdgeNode->mDX * (sYT - sEdgeNode->mBot.mY);
                }
            }
        }

        if ((sAET != NULL) && (sScanBeam < sSBTEntries))
        {
            /* === SCANBEAM INTERIOR PROCESSING =========================== */

            IDE_TEST( buildIntersectionTable( aQmxMem, & sITList, sAET, sDY )
                      != IDE_SUCCESS );

            /* Process each node in the intersection table */
            for ( sITNode = sITList;
                  sITNode != NULL;
                  sITNode = sITNode->mNext )
            {
                sEdge0 = sITNode->mIE[0];
                sEdge1 = sITNode->mIE[1];

                /* Only generate output for contributing intersections */
                if ( ( sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ||
                       sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ) &&
                     ( sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ||
                       sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ))
                {
                    sPoly1 = sEdge0->mOutp[STD_GPC_ABOVE];
                    sPoly2 = sEdge1->mOutp[STD_GPC_ABOVE];
                    sIX = sITNode->mPoint.mX;
                    sIY = sITNode->mPoint.mY + sYB;
 
                    sInside[STD_GPC_CLIP] =
                        ( sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                          !sEdge0->mBside[STD_GPC_CLIP] ) ||
                        ( sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                          sEdge1->mBside[STD_GPC_CLIP] ) ||
                        ( !sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                          !sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                          sEdge0->mBside[STD_GPC_CLIP] &&
                          sEdge1->mBside[STD_GPC_CLIP] );
                    sInside[STD_GPC_SUBJ] =
                        ( sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                          !sEdge0->mBside[STD_GPC_SUBJ] ) ||
                        ( sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                          sEdge1->mBside[STD_GPC_SUBJ] ) ||
                        ( !sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                          !sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                          sEdge0->mBside[STD_GPC_SUBJ] &&
                          sEdge1->mBside[STD_GPC_SUBJ] );
       
                    /* Determine quadrant occupancies */
                    switch ( aOP )
                    {
                        case STD_GPC_DIFF:
                        case STD_GPC_INT:
                            sTR =
                                (sInside[STD_GPC_CLIP] ) &&
                                (sInside[STD_GPC_SUBJ] );
                            sTL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] )
                                &&
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBR =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                &&
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                &&
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            break;
                        case STD_GPC_XOR:
                            sTR =
                                (sInside[STD_GPC_CLIP]) ^
                                (sInside[STD_GPC_SUBJ]);
                            sTL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ^
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBR =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ^
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ^
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            break;
                        case STD_GPC_UNION:
                            sTR =
                                (sInside[STD_GPC_CLIP]) ||
                                (sInside[STD_GPC_SUBJ]);
                            sTL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ||
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBR =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ||
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ||
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            break;
                        default:
                            IDE_ASSERT(0);
                            break;
                    }
	  
                    sVClass= sTR + (sTL << 1) + (sBR << 2) + (sBL << 3);

                    switch (sVClass)
                    {
                        case STD_GPC_EMN:
                            IDE_TEST( addLocalMin( aQmxMem,
                                                   &sOutPoly,
                                                   sEdge0,
                                                   sIX,
                                                   sIY )
                                      != IDE_SUCCESS );
                            sEdge1->mOutp[STD_GPC_ABOVE] =
                                sEdge0->mOutp[STD_GPC_ABOVE];
                            break;
                        case STD_GPC_ERI:
                            if (sPoly1 != NULL)
                            {
                                IDE_TEST( addRight( aQmxMem, sPoly1, sIX, sIY )
                                          != IDE_SUCCESS );
                                
                                sEdge1->mOutp[STD_GPC_ABOVE] = sPoly1;
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_ELI:
                            if (sPoly2 != NULL)
                            {
                                IDE_TEST( addLeft( aQmxMem, sPoly2, sIX, sIY )
                                          != IDE_SUCCESS );
                                
                                sEdge0->mOutp[STD_GPC_ABOVE] = sPoly2;
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_EMX:
                            if ( (sPoly1 != NULL) && (sPoly2 != NULL) )
                            {
                                IDE_TEST( addLeft( aQmxMem, sPoly2, sIX, sIY )
                                          != IDE_SUCCESS );
                                
                                IDE_TEST ( mergeRight( sPoly1, sPoly2, sOutPoly ) != IDE_SUCCESS);
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_IMN:
                            IDE_TEST( addLocalMin(aQmxMem, &sOutPoly,sEdge0, sIX, sIY)
                                      != IDE_SUCCESS );
                            
                            sEdge1->mOutp[STD_GPC_ABOVE] =
                                sEdge0->mOutp[STD_GPC_ABOVE];
                            break;
                        case STD_GPC_ILI:
                            if (sPoly1 != NULL)
                            {
                                IDE_TEST( addLeft( aQmxMem, sPoly1, sIX, sIY )
                                          != IDE_SUCCESS );
                                
                                sEdge1->mOutp[STD_GPC_ABOVE] = sPoly1;
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_IRI:
                            if (sPoly2 != NULL)
                            {
                                IDE_TEST( addRight( aQmxMem, sPoly2, sIX, sIY )
                                          != IDE_SUCCESS );
                                
                                sEdge0->mOutp[STD_GPC_ABOVE] = sPoly2;
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_IMX:
                            if ( (sPoly1 != NULL) && (sPoly2 != NULL) )
                            {
                                IDE_TEST( addRight(aQmxMem, sPoly1, sIX, sIY)
                                          != IDE_SUCCESS );
                                
                                IDE_TEST ( mergeLeft( sPoly1, sPoly2, sOutPoly ) != IDE_SUCCESS );
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_IMM:
                            if ( (sPoly1 != NULL) && (sPoly2 != NULL) )
                            {
                                IDE_TEST( addRight( aQmxMem, sPoly1, sIX, sIY )
                                          != IDE_SUCCESS );
                                
                                IDE_TEST( mergeLeft( sPoly1, sPoly2, sOutPoly ) != IDE_SUCCESS );
                                IDE_TEST( addLocalMin( aQmxMem,
                                                       &sOutPoly,
                                                       sEdge0,
                                                       sIX,
                                                       sIY )
                                          != IDE_SUCCESS );
                                
                                sEdge1->mOutp[STD_GPC_ABOVE] =
                                    sEdge0->mOutp[STD_GPC_ABOVE];
                            }
                            break;
                        case STD_GPC_EMM:
                            if ( (sPoly1 != NULL ) && (sPoly2 != NULL) )
                            {
                                IDE_TEST( addLeft( aQmxMem, sPoly1, sIX, sIY )
                                          != IDE_SUCCESS );
                                IDE_TEST ( mergeRight(sPoly1, sPoly2, sOutPoly) != IDE_SUCCESS );
                                IDE_TEST( addLocalMin( aQmxMem,
                                                       &sOutPoly,
                                                       sEdge0,
                                                       sIX,
                                                       sIY )
                                          != IDE_SUCCESS );
                                
                                sEdge1->mOutp[STD_GPC_ABOVE] =
                                    sEdge0->mOutp[STD_GPC_ABOVE];
                            }
                            break;
                        default:
                            break;
                    } /* End of switch */
                } /* End of contributing intersection conditional */

                /* Swap bundle sides in response to edge crossing */
                if (sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                {
                    sEdge1->mBside[STD_GPC_CLIP] =
                        !sEdge1->mBside[STD_GPC_CLIP];
                }
                
                if (sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                {
                    sEdge0->mBside[STD_GPC_CLIP] =
                        !sEdge0->mBside[STD_GPC_CLIP];
                }
                
                if (sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ])
                {
                    sEdge1->mBside[STD_GPC_SUBJ] =
                        !sEdge1->mBside[STD_GPC_SUBJ];
                }
                
                if (sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ])
                {
                    sEdge0->mBside[STD_GPC_SUBJ] =
                        !sEdge0->mBside[STD_GPC_SUBJ];
                }

                /* Swap e0 and e1 bundles in the AET */
                sPrevEdge = sEdge0->mPrev;
                sNextEdge = sEdge1->mNext;
                
                if (sNextEdge != NULL)
                {
                    sNextEdge->mPrev = sEdge0;
                }
                

                if (sEdge0->mBstate[STD_GPC_ABOVE] == STD_GPC_BUNDLE_HEAD)
                {
                    sSearch = STD_GPC_TRUE;
                    while (sSearch)
                    {
                        sPrevEdge = sPrevEdge->mPrev;
                        if (sPrevEdge != NULL)
                        {
                            if (sPrevEdge->mBstate[STD_GPC_ABOVE]
                                != STD_GPC_BUNDLE_TAIL)
                            {
                                sSearch = STD_GPC_FALSE;
                            }
                        }
                        else
                        {
                            sSearch = STD_GPC_FALSE;
                        }
                    }
                }
                if (sPrevEdge == NULL)
                {
                    sAET->mPrev = sEdge1;
                    sEdge1->mNext = sAET;
                    sAET = sEdge0->mNext;
                }
                else
                {
                    sPrevEdge->mNext->mPrev = sEdge1;
                    sEdge1->mNext = sPrevEdge->mNext;
                    sPrevEdge->mNext = sEdge0->mNext;
                }
                sEdge0->mNext->mPrev = sPrevEdge;
                sEdge1->mNext->mPrev = sEdge1;
                sEdge0->mNext = sNextEdge;
            } /* End of IT loop*/

            /* Prepare for next scanbeam */
            for ( sEdgeNode = sAET; sEdgeNode != NULL; sEdgeNode = sNextEdge)
            {
                sNextEdge = sEdgeNode->mNext;
                sSuccEdge = sEdgeNode->mSucc;

                if ( (sEdgeNode->mTop.mY == sYT) && (sSuccEdge != NULL) )
                {
                    /* Replace AET edge by its successor */
                    sSuccEdge->mOutp[STD_GPC_BELOW] =
                        sEdgeNode->mOutp[STD_GPC_ABOVE];
                    sSuccEdge->mBstate[STD_GPC_BELOW] =
                        sEdgeNode->mBstate[STD_GPC_ABOVE];
                    sSuccEdge->mBundle[STD_GPC_BELOW][STD_GPC_CLIP] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP];
                    sSuccEdge->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ];
                    sPrevEdge = sEdgeNode->mPrev;
                    if (sPrevEdge != NULL)
                    {
                        sPrevEdge->mNext= sSuccEdge;
                    }
                    else
                    {
                        sAET = sSuccEdge;
                    }
                    
                    if (sNextEdge != NULL)
                    {
                        sNextEdge->mPrev = sSuccEdge;
                    }
                    
                    sSuccEdge->mPrev = sPrevEdge;
                    sSuccEdge->mNext = sNextEdge;
                }
                else
                {
                    /* Update this edge */
                    sEdgeNode->mOutp[STD_GPC_BELOW] =
                        sEdgeNode->mOutp[STD_GPC_ABOVE];
                    sEdgeNode->mBstate[STD_GPC_BELOW] =
                        sEdgeNode->mBstate[STD_GPC_ABOVE];
                    sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_CLIP] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP];
                    sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ];
                    sEdgeNode->mXB = sEdgeNode->mXT;
                }
                sEdgeNode->mOutp[STD_GPC_ABOVE] = NULL;
            }
        }
    } /* === END OF SCANBEAM PROCESSING ================================== */

    /* Generate result polygon from sOutPoly */
    aOutPoly->mContour = NULL;
    aOutPoly->mHole = NULL;
    IDE_TEST( countContours( sOutPoly, & aOutPoly->mNumContours )
              != IDE_SUCCESS );
    if (aOutPoly->mNumContours > 0)
    {
        IDE_TEST( aQmxMem->alloc( aOutPoly->mNumContours * ID_SIZEOF(SInt),
                                  (void**) & aOutPoly->mHole )
                  != IDE_SUCCESS );
        
        IDE_TEST( aQmxMem->alloc( aOutPoly->mNumContours
                                  * ID_SIZEOF( stdGpcVertexList ),
                                  (void **) & aOutPoly->mContour )
                  != IDE_SUCCESS );

        c = 0;
        for ( sPoly = sOutPoly; sPoly != NULL; sPoly = sNextPoly )
        {
            sNextPoly = sPoly->mNext;
            if ( sPoly->mActive )
            {
                aOutPoly->mHole[c] = sPoly->mProxy->mHole;
                aOutPoly->mContour[c].mNumVertices = sPoly->mActive;
                IDE_TEST( aQmxMem->alloc( aOutPoly->mContour[c].mNumVertices
                                          * ID_SIZEOF( stdGpcVertex ),
                                          (void**) & aOutPoly->mContour[c].mVertex )
                          != IDE_SUCCESS );

                /*
                v = aOutPoly->mContour[c].mNumVertices - 1;
                for ( sVertex = sPoly->mProxy->mV[STD_GPC_LEFT];
                      sVertex != NULL;
                      sVertex = sNextVtx)
                {
                    sNextVtx = sVertex->mNext;
                    aOutPoly->mContour[c].mVertex[v].mX = sVertex->mX;
                    aOutPoly->mContour[c].mVertex[v].mY = sVertex->mY;

                    sVertex = NULL;
                    v--;
                }
                */

                for ( v = 0, sVertex = sPoly->mProxy->mV[STD_GPC_LEFT];
                      (v < aOutPoly->mContour[c].mNumVertices) &&
                          (sVertex != NULL);
                      sVertex = sNextVtx, v++ )
                {
                    sNextVtx = sVertex->mNext;

                    aOutPoly->mContour[c].mVertex[v].mX = sVertex->mX;
                    aOutPoly->mContour[c].mVertex[v].mY = sVertex->mY;

                    if ( (v > 0) && (sNextVtx != NULL) )
                    {
                        // 동일한 점인 경우 제거한다.
                        if ((STD_GPC_EQ( aOutPoly->mContour[c].mVertex[v-1].mX,
                                         sVertex->mX ) == ID_TRUE )
                             &&
                            (STD_GPC_EQ( aOutPoly->mContour[c].mVertex[v-1].mY,
                                         sVertex->mY ) == ID_TRUE ) )
                        {
                            v--;
                        }
                    }
                    
                    sVertex = NULL;
                }
                aOutPoly->mContour[c].mNumVertices = v;
                      
                c++;
            }
            sPoly = NULL;
        }
    }
    else
    {
        /* free sOutPoly */
    }

    IDE_EXCEPTION_CONT(STD_GPC_POLYGON_GLIP_RETURN);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::tristripClip( iduMemory       * aQmxMem,
                      stdGpcOP          aOP,
                      stdGpcPolygon   * aSubjPoly,
                      stdGpcPolygon   * aClipPoly,
                      stdGpcTristrip  * aOutTristrip )
{
    stdGpcChunk*       sChunk       = NULL;

    stdGpcITNode*      sITList      = NULL;
    stdGpcITNode*      sITNode;

    stdGpcEdgeNode*    sEdgeNode;
    stdGpcEdgeNode*    sPrevEdge;
    stdGpcEdgeNode*    sNextEdge;
    stdGpcEdgeNode*    sSuccEdge;
    stdGpcEdgeNode*    sEdge0;
    stdGpcEdgeNode*    sEdge1;

    stdGpcEdgeNode*    sAET        = NULL;
    stdGpcEdgeNode*    sClipHeap   = NULL;
    stdGpcEdgeNode*    sSubjHeap   = NULL;
    stdGpcEdgeNode*    sCFEdge     = NULL;

    stdGpcLMTNode*     sLMT        = NULL;
    stdGpcLMTNode*     sLocalMin;

    stdGpcPolygonNode* sTriList    = NULL;
    stdGpcPolygonNode* sTriNode;
    stdGpcPolygonNode* sTriNextNode;
    stdGpcPolygonNode* sPoly1;
    stdGpcPolygonNode* sPoly2;

    stdGpcVertexNode*  sLeftVtx;
    stdGpcVertexNode*  sLeftNextVtx;
    stdGpcVertexNode*  sRightVtx;
    stdGpcVertexNode*  sRightNextVtx;

    stdGpcHState       sHoriz[2];
    stdGpcVertexType   sCFType     = STD_GPC_NUL ;

    SInt               sInside[2];
    SInt               sExistCnt[2];
    SInt               sParity[2]  = {STD_GPC_LEFT, STD_GPC_LEFT};
    SInt               s, v;
    SInt               sContribute;
    SInt               sSearch;
    SInt               sScanBeam   = 0;
    SInt               sSBTEntries = 0;
    SInt               sVClass;
    SInt               sBL;
    SInt               sBR;
    SInt               sTL;
    SInt               sTR;
    SDouble*           sSBT        = NULL;
    SDouble            sXB;
    SDouble            sPX;
    SDouble            sNX;
    SDouble            sYB;
    SDouble            sYT = 0;
    SDouble            sDY;
    SDouble            sIX;
    SDouble            sIY;

    SInt               sTypePos;
    SInt               sOppoPos;

    /* Test for trivial NULL result cases */
    if ( ( ( aSubjPoly->mNumContours == 0 ) &&
           ( aClipPoly->mNumContours == 0 ) )
         ||
         ( ( aSubjPoly->mNumContours == 0 ) &&
           ( (aOP == STD_GPC_INT) || (aOP == STD_GPC_DIFF)))
         ||
         ( ( aClipPoly->mNumContours == 0 ) &&
           ( aOP == STD_GPC_INT ) ) )
    {
        aOutTristrip->mNumStrips = 0;
        aOutTristrip->mStrip = NULL;
        IDE_RAISE(STD_GPC_TRISTRIP_GLIP_RETURN);
    }

    /* Identify potentialy contributing contours */
    if ( ( ( aOP == STD_GPC_INT ) ||
           ( aOP == STD_GPC_DIFF ) ) &&
         ( aSubjPoly->mNumContours > 0 ) &&
         ( aClipPoly->mNumContours > 0 ) )
    {
        IDE_TEST( miniMaxTest( aQmxMem, aSubjPoly, aClipPoly, aOP ) != IDE_SUCCESS );
    }

    /* Build LMT */
    if (aSubjPoly->mNumContours > 0)
    {
        IDE_TEST( buildLMT( aQmxMem,
                            & sLMT,
                            & sChunk,
                            & sSBTEntries,
                            aSubjPoly,
                            STD_GPC_SUBJ,
                            aOP,
                            & sSubjHeap ) != IDE_SUCCESS );
    }
    
    if (aClipPoly->mNumContours > 0)
    {
        IDE_TEST( buildLMT( aQmxMem,
                            & sLMT,
                            & sChunk,
                            & sSBTEntries,
                            aClipPoly,
                            STD_GPC_CLIP,
                            aOP,
                            & sClipHeap ) != IDE_SUCCESS );
    }

    /* Return a NULL result if no contours contribute */
    if ( sLMT == NULL )
    {
        aOutTristrip->mNumStrips = 0;
        aOutTristrip->mStrip = NULL;

        sSubjHeap = NULL;
        sClipHeap = NULL;
        
        IDE_RAISE(STD_GPC_TRISTRIP_GLIP_RETURN);
    }

    /* Build scanbeam table from scanbeam tree */
    IDE_TEST( aQmxMem->alloc( sSBTEntries * ID_SIZEOF(SDouble),
                              (void**) & sSBT )
              != IDE_SUCCESS );

    buildSChunk( & sScanBeam, sSBT, sChunk );
    sScanBeam = 0;

    /* Invert clip polygon for difference operation */
    if ( aOP == STD_GPC_DIFF )
    {
        sParity[STD_GPC_CLIP]= STD_GPC_RIGHT;
    }

    sLocalMin = sLMT;

    if ( sSBTEntries < 2 )
    {
        sYT = sSBT[0];
        sDY = 0;
    }
        

    /* Process each scanbeam */
    while ( sScanBeam < sSBTEntries )
    {
        /* Set yb and yt to the bottom and top of the scanbeam */
        sYB= sSBT[sScanBeam++];
        if (sScanBeam < sSBTEntries)
        {
            sYT = sSBT[sScanBeam];
            sDY = sYT - sYB;
        }

        /* === SCANBEAM BOUNDARY PROCESSING ================================ */

        /* If LMT node corresponding to yb exists */
        if ( sLocalMin != NULL )
        {
            if ( sLocalMin->mY == sYB)
            {
                /* Add edges starting at this local minimum to the AET */
                for ( sEdgeNode = sLocalMin->mFirstBound;
                      sEdgeNode != NULL;
                      sEdgeNode = sEdgeNode->mNextBound )
                {
                    addEdge2AET( & sAET, sEdgeNode, NULL);
                }

                sLocalMin = sLocalMin->mNext;
            }
        }
        if(sAET == NULL)
        {
            continue;
        }

        /* Set dummy previous x value */
        sPX = -DBL_MAX;

        /* Create bundles within AET */
        sEdge0 = sAET;
        sEdge1 = sAET;

        /* Set up bundle fields of first edge */
        sTypePos = ( sAET->mType ) ? STD_GPC_TRUE : STD_GPC_FALSE;
        sOppoPos = ( sTypePos + 1 ) % 2;
        
        sAET->mBundle[STD_GPC_ABOVE][sTypePos] = (sAET->mTop.mY != sYB);
        sAET->mBundle[STD_GPC_ABOVE][sOppoPos] = STD_GPC_FALSE;
        sAET->mBstate[STD_GPC_ABOVE] = STD_GPC_UNBUNDLED;

        for ( sNextEdge = sAET->mNext;
              sNextEdge != NULL;
              sNextEdge = sNextEdge->mNext )
        {
            /* Set up bundle fields of next edge */
            sTypePos = ( sNextEdge->mType ) ? STD_GPC_TRUE : STD_GPC_FALSE;
            sOppoPos = ( sTypePos + 1 ) % 2;
            
            sNextEdge->mBundle[STD_GPC_ABOVE][sTypePos] =
                (sNextEdge->mTop.mY != sYB);
            sNextEdge->mBundle[STD_GPC_ABOVE][sOppoPos] = STD_GPC_FALSE;
            sNextEdge->mBstate[STD_GPC_ABOVE] = STD_GPC_UNBUNDLED;

            /* Bundle edges above the scanbeam boundary if they coincide */
            if ( sNextEdge->mBundle[STD_GPC_ABOVE][sNextEdge->mType] )
            {
                if ( STD_GPC_EQ( sEdge0->mXB, sNextEdge->mXB ) &&
                     STD_GPC_EQ( sEdge0->mDX, sNextEdge->mDX ) &&
                     ( sEdge0->mTop.mY != sYB ) )
                {
                    sNextEdge->mBundle[STD_GPC_ABOVE][sTypePos] ^= 
                        sEdge0->mBundle[STD_GPC_ABOVE][sTypePos];
                    sNextEdge->mBundle[STD_GPC_ABOVE][sOppoPos] = 
                        sEdge0->mBundle[STD_GPC_ABOVE][sOppoPos]; 
                    sNextEdge->mBstate[STD_GPC_ABOVE] = STD_GPC_BUNDLE_HEAD;
                    
                    sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP]
                        = STD_GPC_FALSE;
                    sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]
                        = STD_GPC_FALSE;
                    sEdge0->mBstate[STD_GPC_ABOVE] = STD_GPC_BUNDLE_TAIL;
                }
                sEdge0 = sNextEdge;
            }
        }

        sHoriz[STD_GPC_CLIP] = STD_GPC_NH;
        sHoriz[STD_GPC_SUBJ] = STD_GPC_NH;

        /* Process each edge at this scanbeam boundary */
        for ( sEdgeNode = sAET;
              sEdgeNode != NULL;
              sEdgeNode = sEdgeNode->mNext )
        {
            sExistCnt[STD_GPC_CLIP] =
                sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] + 
                ( sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_CLIP] << 1 );
            sExistCnt[STD_GPC_SUBJ] =
                sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] + 
                (sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ] << 1);

            if ( ( sExistCnt[STD_GPC_CLIP] > 0 ) ||
                 ( sExistCnt[STD_GPC_SUBJ] > 0 ) )
            {
                /* Set bundle side */
                sEdgeNode->mBside[STD_GPC_CLIP] = sParity[STD_GPC_CLIP];
                sEdgeNode->mBside[STD_GPC_SUBJ] = sParity[STD_GPC_SUBJ];

                /* Determine contributing status and quadrant occupancies */
                switch ( aOP )
                {
                    case STD_GPC_DIFF:
                    case STD_GPC_INT:
                        sContribute =
                            (sExistCnt[STD_GPC_CLIP] &&
                             (sParity[STD_GPC_SUBJ] || sHoriz[STD_GPC_SUBJ]))
                            ||
                            (sExistCnt[STD_GPC_SUBJ] &&
                             (sParity[STD_GPC_CLIP] || sHoriz[STD_GPC_CLIP]))
                            ||
                            (sExistCnt[STD_GPC_CLIP] &&
                             sExistCnt[STD_GPC_SUBJ] &&
                             (sParity[STD_GPC_CLIP] == sParity[STD_GPC_SUBJ]));
                        sBR =
                            (sParity[STD_GPC_CLIP]) && (sParity[STD_GPC_SUBJ]);
                        sBL =
                            (sParity[STD_GPC_CLIP] ^
                             sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                            &&
                            (sParity[STD_GPC_SUBJ] ^
                             sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                        sTR =
                            (sParity[STD_GPC_CLIP] ^
                             (sHoriz[STD_GPC_CLIP] != STD_GPC_NH))
                            &&
                            (sParity[STD_GPC_SUBJ] ^
                             (sHoriz[STD_GPC_SUBJ] != STD_GPC_NH));
                        sTL =
                            (sParity[STD_GPC_CLIP] ^
                             (sHoriz[STD_GPC_CLIP] != STD_GPC_NH) ^
                             sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_CLIP]) 
                            &&
                            (sParity[STD_GPC_SUBJ] ^
                             (sHoriz[STD_GPC_SUBJ] != STD_GPC_NH) ^
                             sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ]);
                        break;
                        
                    case STD_GPC_XOR:
                        sContribute =
                            sExistCnt[STD_GPC_CLIP] || sExistCnt[STD_GPC_SUBJ];
                        sBR =
                            (sParity[STD_GPC_CLIP]) ^ (sParity[STD_GPC_SUBJ]);
                        sBL =
                            (sParity[STD_GPC_CLIP] ^
                             sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                            ^
                            (sParity[STD_GPC_SUBJ] ^
                             sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                        sTR =
                            (sParity[STD_GPC_CLIP] ^
                             (sHoriz[STD_GPC_CLIP] != STD_GPC_NH))
                            ^
                            (sParity[STD_GPC_SUBJ] ^
                             (sHoriz[STD_GPC_SUBJ] != STD_GPC_NH));
                        sTL =
                            (sParity[STD_GPC_CLIP] ^
                             (sHoriz[STD_GPC_CLIP] != STD_GPC_NH) ^
                             sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_CLIP])
                            ^
                            (sParity[STD_GPC_SUBJ] ^
                             (sHoriz[STD_GPC_SUBJ] != STD_GPC_NH) ^
                             sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ]);
                        break;
                        
                    case STD_GPC_UNION:
                        sContribute =
                            (sExistCnt[STD_GPC_CLIP] &&
                             (!sParity[STD_GPC_SUBJ] || sHoriz[STD_GPC_SUBJ]))
                            ||
                            (sExistCnt[STD_GPC_SUBJ] &&
                             (!sParity[STD_GPC_CLIP] || sHoriz[STD_GPC_CLIP]))
                            ||
                            (sExistCnt[STD_GPC_CLIP] &&
                             sExistCnt[STD_GPC_SUBJ] &&
                             (sParity[STD_GPC_CLIP] == sParity[STD_GPC_SUBJ]));
                        sBR =
                            (sParity[STD_GPC_CLIP]) || (sParity[STD_GPC_SUBJ]);
                        sBL =
                            (sParity[STD_GPC_CLIP] ^
                             sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                            ||
                            (sParity[STD_GPC_SUBJ] ^
                             sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                        sTR =
                            (sParity[STD_GPC_CLIP] ^
                             (sHoriz[STD_GPC_CLIP] != STD_GPC_NH))
                            ||
                            (sParity[STD_GPC_SUBJ] ^
                             (sHoriz[STD_GPC_SUBJ] != STD_GPC_NH));
                        sTL =
                            (sParity[STD_GPC_CLIP] ^
                             (sHoriz[STD_GPC_CLIP] != STD_GPC_NH) ^
                             sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_CLIP])
                            ||
                            (sParity[STD_GPC_SUBJ] ^
                             (sHoriz[STD_GPC_SUBJ] != STD_GPC_NH) ^
                             sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ]);
                        break;
                    default:
                        IDE_ASSERT(0);
                        break;
                }

                /* Update parity */
                sParity[STD_GPC_CLIP] ^=
                    sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP];
                sParity[STD_GPC_SUBJ] ^=
                    sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ];

                /* Update sHorizontal state */
                if ( sExistCnt[STD_GPC_CLIP] > 0 )
                {
                    sHoriz[STD_GPC_CLIP] =
                        stdGpcNextHState[sHoriz[STD_GPC_CLIP]]
                        [ ((sExistCnt[STD_GPC_CLIP] - 1) << 1) +
                          sParity[STD_GPC_CLIP] ];
                }
                
                if ( sExistCnt[STD_GPC_SUBJ] > 0 )
                {
                    
                    sHoriz[STD_GPC_SUBJ] =
                        stdGpcNextHState[sHoriz[STD_GPC_SUBJ]]
                        [ ((sExistCnt[STD_GPC_SUBJ] - 1) << 1) +
                          sParity[STD_GPC_SUBJ] ];
                }
                
                sVClass= sTR + (sTL << 1) + (sBR << 2) + (sBL << 3);

                if ( sContribute )
                {
                    sXB = sEdgeNode->mXB;

                    switch (sVClass)
                    {
                        case STD_GPC_EMN:
                            IDE_TEST( newTristrip( aQmxMem,
                                                   &sTriList,
                                                   sEdgeNode,
                                                   sXB,
                                                   sYB )
                                      != IDE_SUCCESS );
                            sCFEdge = sEdgeNode;
                            break;
                        case STD_GPC_ERI:
                            sEdgeNode->mOutp[STD_GPC_ABOVE] =
                                sCFEdge->mOutp[STD_GPC_ABOVE];
                                      
                            if (sXB != sCFEdge->mXB)
                            {
                                STD_GPC_VERTEX( aQmxMem,
                                                sEdgeNode,
                                                STD_GPC_ABOVE,
                                                STD_GPC_RIGHT,
                                                sXB,
                                                sYB );
                            }
                            
                            sCFEdge = NULL;
                            break;
                        case STD_GPC_ELI:
                            STD_GPC_VERTEX( aQmxMem,
                                            sEdgeNode,
                                            STD_GPC_BELOW,
                                            STD_GPC_LEFT,
                                            sXB,
                                            sYB );
                            sEdgeNode->mOutp[STD_GPC_ABOVE] = NULL;
                            sCFEdge = sEdgeNode;
                            break;
                        case STD_GPC_EMX:
                            if ( sXB != sCFEdge->mXB )
                            {
                                STD_GPC_VERTEX( aQmxMem,
                                                sEdgeNode,
                                                STD_GPC_BELOW,
                                                STD_GPC_RIGHT,
                                                sXB,
                                                sYB );
                            }
                            
                            sEdgeNode->mOutp[STD_GPC_ABOVE] = NULL;
                            sCFEdge = NULL;
                            break;
                        case STD_GPC_IMN:
                            if ( sCFType == STD_GPC_LED )
                            {
                                if (sCFEdge->mBot.mY != sYB)
                                {
                                    STD_GPC_VERTEX( aQmxMem,
                                                    sCFEdge,
                                                    STD_GPC_BELOW,
                                                    STD_GPC_LEFT,
                                                    sCFEdge->mXB,
                                                    sYB );
                                }
                                
                                IDE_TEST( newTristrip( aQmxMem,
                                                       &sTriList,
                                                       sCFEdge,
                                                       sCFEdge->mXB,
                                                       sYB )
                                          != IDE_SUCCESS );
                            }
                            
                            sEdgeNode->mOutp[STD_GPC_ABOVE] =
                                sCFEdge->mOutp[STD_GPC_ABOVE];
                            STD_GPC_VERTEX( aQmxMem,
                                            sEdgeNode,
                                            STD_GPC_ABOVE,
                                            STD_GPC_RIGHT,
                                            sXB,
                                            sYB );
                            break;
                            
                        case STD_GPC_ILI:
                            IDE_TEST( newTristrip( aQmxMem,
                                                   &sTriList,
                                                   sEdgeNode,
                                                   sXB,
                                                   sYB )
                                      != IDE_SUCCESS );
                            sCFEdge = sEdgeNode;
                            sCFType = STD_GPC_ILI;
                            break;
                            
                        case STD_GPC_IRI:
                            if (sCFType == STD_GPC_LED)
                            {
                                if (sCFEdge->mBot.mY != sYB)
                                {
                                    STD_GPC_VERTEX( aQmxMem,
                                                    sCFEdge,
                                                    STD_GPC_BELOW,
                                                    STD_GPC_LEFT,
                                                    sCFEdge->mXB,
                                                    sYB );
                                }
                                
                                IDE_TEST( newTristrip( aQmxMem,
                                                       &sTriList,
                                                       sCFEdge,
                                                       sCFEdge->mXB,
                                                       sYB )
                                          != IDE_SUCCESS );
                            }
                            STD_GPC_VERTEX( aQmxMem,
                                            sEdgeNode,
                                            STD_GPC_BELOW,
                                            STD_GPC_RIGHT,
                                            sXB,
                                            sYB );
                            sEdgeNode->mOutp[STD_GPC_ABOVE] = NULL;
                            break;
                            
                        case STD_GPC_IMX:
                            STD_GPC_VERTEX( aQmxMem,
                                            sEdgeNode,
                                            STD_GPC_BELOW,
                                            STD_GPC_LEFT,
                                            sXB,
                                            sYB );
                            sEdgeNode->mOutp[STD_GPC_ABOVE] = NULL;
                            sCFType = STD_GPC_IMX;
                            break;
                            
                        case STD_GPC_IMM:
                            STD_GPC_VERTEX( aQmxMem,
                                            sEdgeNode,
                                            STD_GPC_BELOW,
                                            STD_GPC_LEFT,
                                            sXB,
                                            sYB );
                            sEdgeNode->mOutp[STD_GPC_ABOVE] =
                                sCFEdge->mOutp[STD_GPC_ABOVE];
                            
                            if (sXB != sCFEdge->mXB)
                            {
                                STD_GPC_VERTEX( aQmxMem,
                                                sCFEdge,
                                                STD_GPC_ABOVE,
                                                STD_GPC_RIGHT,
                                                sXB,
                                                sYB );
                            }
                            
                            sCFEdge = sEdgeNode;
                            break;
                            
                        case STD_GPC_EMM:
                            STD_GPC_VERTEX( aQmxMem,
                                            sEdgeNode,
                                            STD_GPC_BELOW,
                                            STD_GPC_RIGHT,
                                            sXB,
                                            sYB );
                            sEdgeNode->mOutp[STD_GPC_ABOVE] = NULL;
                            IDE_TEST( newTristrip( aQmxMem,
                                                   &sTriList,
                                                   sEdgeNode,
                                                   sXB,
                                                   sYB )
                                      != IDE_SUCCESS );
                            sCFEdge = sEdgeNode;
                            break;
                            
                        case STD_GPC_LED:
                            if (sEdgeNode->mBot.mY == sYB)
                            {
                                STD_GPC_VERTEX( aQmxMem,
                                                sEdgeNode,
                                                STD_GPC_BELOW,
                                                STD_GPC_LEFT,
                                                sXB,
                                                sYB );
                            }
                            
                            sEdgeNode->mOutp[STD_GPC_ABOVE] =
                                sEdgeNode->mOutp[STD_GPC_BELOW];
                            sCFEdge = sEdgeNode;
                            sCFType = STD_GPC_LED;
                            break;
                            
                        case STD_GPC_RED:
                            sEdgeNode->mOutp[STD_GPC_ABOVE] =
                                sCFEdge->mOutp[STD_GPC_ABOVE];
                            if (sCFType == STD_GPC_LED)
                            {
                                if (sCFEdge->mBot.mY == sYB)
                                {
                                    STD_GPC_VERTEX( aQmxMem,
                                                    sEdgeNode,
                                                    STD_GPC_BELOW,
                                                    STD_GPC_RIGHT,
                                                    sXB,
                                                    sYB );
                                }
                                else
                                {
                                    if (sEdgeNode->mBot.mY == sYB)
                                    {
                                        STD_GPC_VERTEX( aQmxMem,
                                                        sCFEdge,
                                                        STD_GPC_BELOW,
                                                        STD_GPC_LEFT,
                                                        sCFEdge->mXB,
                                                        sYB );
                                        STD_GPC_VERTEX( aQmxMem,
                                                        sEdgeNode,
                                                        STD_GPC_BELOW,
                                                        STD_GPC_RIGHT,
                                                        sXB,
                                                        sYB );
                                    }
                                }
                            }
                            else
                            {
                                STD_GPC_VERTEX( aQmxMem,
                                                sEdgeNode,
                                                STD_GPC_BELOW,
                                                STD_GPC_RIGHT,
                                                sXB,
                                                sYB );
                                STD_GPC_VERTEX( aQmxMem,
                                                sEdgeNode,
                                                STD_GPC_ABOVE,
                                                STD_GPC_RIGHT,
                                                sXB,
                                                sYB );
                            }
                            sCFEdge = NULL;
                            break;
                            
                        default:
                            break;
                    } /* End of switch */
                } /* End of contributing conditional */
            } /* End of edge exists conditional */
        } /* End of AET loop */

        /* Delete terminating edges from the AET, otherwise compute xt */
        for ( sEdgeNode = sAET;
              sEdgeNode != NULL;
              sEdgeNode = sEdgeNode->mNext )
        {
            if (sEdgeNode->mTop.mY == sYB)
            {
                sPrevEdge = sEdgeNode->mPrev;
                sNextEdge = sEdgeNode->mNext;
                if ( sPrevEdge != NULL )
                {
                    sPrevEdge->mNext = sNextEdge;
                }
                else
                {
                    sAET = sNextEdge;
                }
                
                if ( sNextEdge != NULL )
                {
                    sNextEdge->mPrev = sPrevEdge;
                }
                

                /* Copy bundle head state to the adjacent tail edge
                   if required */
                if ( (sEdgeNode->mBstate[STD_GPC_BELOW] == STD_GPC_BUNDLE_HEAD)
                     &&
                     (sPrevEdge != NULL ) )
                {
                    if ( sPrevEdge->mBstate[STD_GPC_BELOW]
                         == STD_GPC_BUNDLE_TAIL )
                    {
                        sPrevEdge->mOutp[STD_GPC_BELOW] =
                            sEdgeNode->mOutp[STD_GPC_BELOW];
                        sPrevEdge->mBstate[STD_GPC_BELOW] = STD_GPC_UNBUNDLED;
                        if ( sPrevEdge->mPrev != NULL )
                        {
                            if (sPrevEdge->mPrev->mBstate[STD_GPC_BELOW]
                                == STD_GPC_BUNDLE_TAIL)
                            {
                                sPrevEdge->mBstate[STD_GPC_BELOW] =
                                    STD_GPC_BUNDLE_HEAD;
                            }
                        }
                        
                    }
                }
            }
            else
            {
                if (sEdgeNode->mTop.mY == sYT)
                {
                    sEdgeNode->mXT= sEdgeNode->mTop.mX;
                }
                else
                {
                    sEdgeNode->mXT =
                        sEdgeNode->mBot.mX +
                        sEdgeNode->mDX * (sYT - sEdgeNode->mBot.mY);
                }
            }
        }

        if (sScanBeam < sSBTEntries)
        {
            /* === SCANBEAM INTERIOR PROCESSING =========================== */
  
            IDE_TEST( buildIntersectionTable( aQmxMem,
                                              &sITList,
                                              sAET,
                                              sDY )
                      != IDE_SUCCESS );

            /* Process each node in the intersection table */
            for ( sITNode = sITList;
                  sITNode != NULL;
                  sITNode = sITNode->mNext )
            {
                sEdge0 = sITNode->mIE[0];
                sEdge1 = sITNode->mIE[1];

                /* Only generate output for contributing intersections */
                if ( ( sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ||
                       sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] )
                     &&
                     ( sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ||
                       sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ) )
                {
                    sPoly1 = sEdge0->mOutp[STD_GPC_ABOVE];
                    sPoly2 = sEdge1->mOutp[STD_GPC_ABOVE];
                    sIX = sITNode->mPoint.mX;
                    sIY = sITNode->mPoint.mY + sYB;

                    sInside[STD_GPC_CLIP] =
                        ( sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                          !sEdge0->mBside[STD_GPC_CLIP] )
                        ||
                        ( sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                          sEdge1->mBside[STD_GPC_CLIP] )
                        ||
                        (!sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                         !sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] &&
                         sEdge0->mBside[STD_GPC_CLIP] &&
                         sEdge1->mBside[STD_GPC_CLIP]);
                    sInside[STD_GPC_SUBJ] =
                        ( sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                          !sEdge0->mBside[STD_GPC_SUBJ] )
                        ||
                        ( sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                          sEdge1->mBside[STD_GPC_SUBJ])
                        ||
                        (!sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                         !sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] &&
                         sEdge0->mBside[STD_GPC_SUBJ] &&
                         sEdge1->mBside[STD_GPC_SUBJ]);

                    /* Determine quadrant occupancies */
                    switch ( aOP )
                    {
                        case STD_GPC_DIFF:
                        case STD_GPC_INT:
                            sTR =
                                (sInside[STD_GPC_CLIP]) &&
                                (sInside[STD_GPC_SUBJ]);
                            sTL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                &&
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBR =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                &&
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                &&
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            break;
                            
                        case STD_GPC_XOR:
                            sTR =
                                (sInside[STD_GPC_CLIP]) ^
                                (sInside[STD_GPC_SUBJ]);
                            sTL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ^
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBR =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ^
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ^
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            break;
                            
                        case STD_GPC_UNION:
                            sTR =
                                (sInside[STD_GPC_CLIP])
                                ||
                                (sInside[STD_GPC_SUBJ]);
                            sTL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ||
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBR =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ||
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            sBL =
                                (sInside[STD_GPC_CLIP] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                                ||
                                (sInside[STD_GPC_SUBJ] ^
                                 sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] ^
                                 sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]);
                            break;
                        default:
                            IDE_ASSERT(0);
                            break;
                    }

                    sVClass = sTR + (sTL << 1) + (sBR << 2) + (sBL << 3);
                    switch (sVClass)
                    {
                        case STD_GPC_EMN:
                            IDE_TEST( newTristrip( aQmxMem,
                                                   &sTriList,
                                                   sEdge1,
                                                   sIX,
                                                   sIY )
                                      != IDE_SUCCESS );
                            
                            sEdge0->mOutp[STD_GPC_ABOVE] =
                                sEdge1->mOutp[STD_GPC_ABOVE];
                            break;
                        case STD_GPC_ERI:
                            if (sPoly1 != NULL)
                            {
                                STD_GPC_P_EDGE( sPrevEdge,
                                                sEdge0,
                                                STD_GPC_ABOVE,
                                                sPX,
                                                sIY );
                                STD_GPC_VERTEX( aQmxMem,
                                                sPrevEdge,
                                                STD_GPC_ABOVE,
                                                STD_GPC_LEFT,
                                                sPX,
                                                sIY );
                                STD_GPC_VERTEX( aQmxMem,
                                                sEdge0,
                                                STD_GPC_ABOVE,
                                                STD_GPC_RIGHT,
                                                sIX,
                                                sIY );
                                sEdge1->mOutp[STD_GPC_ABOVE] =
                                    sEdge0->mOutp[STD_GPC_ABOVE];
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_ELI:
                            if (sPoly2 != NULL)
                            {
                                STD_GPC_N_EDGE(sNextEdge,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               sNX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sIX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sNextEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sNX,
                                               sIY);
                                sEdge0->mOutp[STD_GPC_ABOVE] =
                                    sEdge1->mOutp[STD_GPC_ABOVE];
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_EMX:
                            if ( (sPoly1 != NULL) && (sPoly2 != NULL) )
                            {
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sIX,
                                               sIY);
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_IMN:
                            STD_GPC_P_EDGE(sPrevEdge,
                                           sEdge0,
                                           STD_GPC_ABOVE,
                                           sPX,
                                           sIY);
                            STD_GPC_VERTEX(aQmxMem,
                                           sPrevEdge,
                                           STD_GPC_ABOVE,
                                           STD_GPC_LEFT,
                                           sPX,
                                           sIY);
                            STD_GPC_N_EDGE(sNextEdge, sEdge1,
                                           STD_GPC_ABOVE,
                                           sNX,
                                           sIY);
                            STD_GPC_VERTEX(aQmxMem,
                                           sNextEdge,
                                           STD_GPC_ABOVE,
                                           STD_GPC_RIGHT,
                                           sNX,
                                           sIY);
                            IDE_TEST( newTristrip(aQmxMem,
                                                  &sTriList,
                                                  sPrevEdge,
                                                  sPX,
                                                  sIY)
                                      != IDE_SUCCESS );
                            
                            sEdge1->mOutp[STD_GPC_ABOVE] =
                                sPrevEdge->mOutp[STD_GPC_ABOVE];
                            STD_GPC_VERTEX(aQmxMem,
                                           sEdge1,
                                           STD_GPC_ABOVE,
                                           STD_GPC_RIGHT,
                                           sIX,
                                           sIY);
                            IDE_TEST( newTristrip( aQmxMem,
                                                   &sTriList,
                                                   sEdge0,
                                                   sIX,
                                                   sIY)
                                      != IDE_SUCCESS );
                            sNextEdge->mOutp[STD_GPC_ABOVE] =
                                sEdge0->mOutp[STD_GPC_ABOVE];
                            STD_GPC_VERTEX(aQmxMem,
                                           sNextEdge,
                                           STD_GPC_ABOVE,
                                           STD_GPC_RIGHT,
                                           sNX,
                                           sIY);
                            break;
                            
                        case STD_GPC_ILI:
                            if ( sPoly1 != NULL )
                            {
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sIX,
                                               sIY);
                                STD_GPC_N_EDGE(sNextEdge,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               sNX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sNextEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sNX,
                                               sIY);
                                sEdge1->mOutp[STD_GPC_ABOVE] =
                                    sEdge0->mOutp[STD_GPC_ABOVE];
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                        case STD_GPC_IRI:
                            if ( sPoly2 != NULL )
                            {
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sIX,
                                               sIY);
                                STD_GPC_P_EDGE(sPrevEdge,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               sPX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sPrevEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sPX,
                                               sIY);
                                sEdge0->mOutp[STD_GPC_ABOVE] =
                                    sEdge1->mOutp[STD_GPC_ABOVE];
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                            }
                            break;
                            
                        case STD_GPC_IMX:
                            if ((sPoly1 != NULL) && (sPoly2 != NULL))
                            {
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sIX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sIX,
                                               sIY);
                                sEdge0->mOutp[STD_GPC_ABOVE] = NULL;
                                sEdge1->mOutp[STD_GPC_ABOVE] = NULL;
                                STD_GPC_P_EDGE(sPrevEdge,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               sPX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sPrevEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sPX,
                                               sIY);
                                IDE_TEST( newTristrip(aQmxMem,
                                                      &sTriList,
                                                      sPrevEdge,
                                                      sPX,
                                                      sIY)
                                          != IDE_SUCCESS );
                                STD_GPC_N_EDGE(sNextEdge,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               sNX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sNextEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sNX,
                                               sIY);
                                sNextEdge->mOutp[STD_GPC_ABOVE] =
                                    sPrevEdge->mOutp[STD_GPC_ABOVE];
                                STD_GPC_VERTEX(aQmxMem,
                                               sNextEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sNX,
                                               sIY);
                            }
                            break;
                            
                        case STD_GPC_IMM:
                            if ( (sPoly1 != NULL) && (sPoly2 != NULL) )
                            {
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sIX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sIX,
                                               sIY);
                                STD_GPC_P_EDGE(sPrevEdge,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               sPX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sPrevEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sPX,
                                               sIY);
                                IDE_TEST( newTristrip(aQmxMem,
                                                      &sTriList,
                                                      sPrevEdge,
                                                      sPX,
                                                      sIY)
                                          != IDE_SUCCESS );
                                STD_GPC_N_EDGE(sNextEdge,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               sNX,
                                               sIY);
                                STD_GPC_VERTEX(aQmxMem,
                                               sNextEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sNX,
                                               sIY);
                                sEdge1->mOutp[STD_GPC_ABOVE] =
                                    sPrevEdge->mOutp[STD_GPC_ABOVE];
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge1,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sIX,
                                               sIY);
                                IDE_TEST( newTristrip(aQmxMem,
                                                      &sTriList,
                                                      sEdge0,
                                                      sIX,
                                                      sIY)
                                          != IDE_SUCCESS );
                                sNextEdge->mOutp[STD_GPC_ABOVE] =
                                    sEdge0->mOutp[STD_GPC_ABOVE];
                                STD_GPC_VERTEX(aQmxMem,
                                               sNextEdge,
                                               STD_GPC_ABOVE,
                                               STD_GPC_RIGHT,
                                               sNX,
                                               sIY);
                            }
                            break;
                        case STD_GPC_EMM:
                            if ((sPoly1 != NULL) && (sPoly2 != NULL))
                            {
                                STD_GPC_VERTEX(aQmxMem,
                                               sEdge0,
                                               STD_GPC_ABOVE,
                                               STD_GPC_LEFT,
                                               sIX,
                                               sIY);
                                IDE_TEST( newTristrip(aQmxMem,
                                                      &sTriList,
                                                      sEdge1,
                                                      sIX,
                                                      sIY)
                                          != IDE_SUCCESS );
                                sEdge0->mOutp[STD_GPC_ABOVE] =
                                    sEdge1->mOutp[STD_GPC_ABOVE];
                            }
                            break;
                        default:
                            break;
                    } /* End of switch */
                } /* End of contributing intersection conditional */

                /* Swap bundle sides in response to edge crossing */
                if (sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                {
                    sEdge1->mBside[STD_GPC_CLIP] =
                        !sEdge1->mBside[STD_GPC_CLIP];
                }
                
                if (sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP])
                {
                    sEdge0->mBside[STD_GPC_CLIP] =
                        !sEdge0->mBside[STD_GPC_CLIP];
                }
                
                if (sEdge0->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ])
                {
                    sEdge1->mBside[STD_GPC_SUBJ] =
                        !sEdge1->mBside[STD_GPC_SUBJ];
                }
                
                if (sEdge1->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ])
                {
                    sEdge0->mBside[STD_GPC_SUBJ] =
                        !sEdge0->mBside[STD_GPC_SUBJ];
                }
                

                /* Swap e0 and e1 bundles in the AET */
                sPrevEdge = sEdge0->mPrev;
                sNextEdge = sEdge1->mNext;
                if (sEdge1->mNext != NULL)
                {
                    sEdge1->mNext->mPrev = sEdge0;
                }

                if (sEdge0->mBstate[STD_GPC_ABOVE] == STD_GPC_BUNDLE_HEAD)
                {
                    sSearch = STD_GPC_TRUE;
                    while (sSearch)
                    {
                        sPrevEdge = sPrevEdge->mPrev;
                        if (sPrevEdge != NULL)
                        {
                            if (sPrevEdge->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP]
                                ||
                                sPrevEdge->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ]
                                ||
                                (sPrevEdge->mBstate[STD_GPC_ABOVE]
                                 == STD_GPC_BUNDLE_HEAD))
                            {
                                sSearch = STD_GPC_FALSE;
                            }
                        }
                        else
                        {
                            sSearch = STD_GPC_FALSE;
                        }
                    }
                }
                if ( sPrevEdge == NULL )
                {
                    sEdge1->mNext = sAET;
                    sAET = sEdge0->mNext;
                }
                else
                {
                    sEdge1->mNext = sPrevEdge->mNext;
                    sPrevEdge->mNext = sEdge0->mNext;
                }
                sEdge0->mNext->mPrev = sPrevEdge;
                sEdge0->mNext = sNextEdge;
                if( sEdge1->mNext != NULL )
                {
                    sEdge1->mNext->mPrev = sEdge1;
                }
            } /* End of IT loop*/

            /* Prepare for next scanbeam */
            for ( sEdgeNode = sAET;
                  sEdgeNode != NULL;
                  sEdgeNode = sNextEdge )
            {
                sNextEdge = sEdgeNode->mNext;
                sSuccEdge = sEdgeNode->mSucc;

                if ((sEdgeNode->mTop.mY == sYT) && (sSuccEdge != NULL) )
                {
                    /* Replace AET edge by its successor */
                    sSuccEdge->mOutp[STD_GPC_BELOW] =
                        sEdgeNode->mOutp[STD_GPC_ABOVE];
                    sSuccEdge->mBstate[STD_GPC_BELOW] =
                        sEdgeNode->mBstate[STD_GPC_ABOVE];
                    sSuccEdge->mBundle[STD_GPC_BELOW][STD_GPC_CLIP] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP];
                    sSuccEdge->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ];
                    sPrevEdge = sEdgeNode->mPrev;
                    if ( sPrevEdge != NULL )
                    {
                        sPrevEdge->mNext = sSuccEdge;
                    }
                    else
                    {
                        sAET = sSuccEdge;
                    }
                    
                    if ( sNextEdge != NULL )
                    {
                        sNextEdge->mPrev = sSuccEdge;
                    }
                    
                    sSuccEdge->mPrev = sPrevEdge;
                    sSuccEdge->mNext = sNextEdge;
                }
                else
                {
                    /* Update this edge */
                    sEdgeNode->mOutp[STD_GPC_BELOW] =
                        sEdgeNode->mOutp[STD_GPC_ABOVE];
                    sEdgeNode->mBstate[STD_GPC_BELOW] =
                        sEdgeNode->mBstate[STD_GPC_ABOVE];
                    sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_CLIP] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP];
                    sEdgeNode->mBundle[STD_GPC_BELOW][STD_GPC_SUBJ] =
                        sEdgeNode->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ];
                    sEdgeNode->mXB = sEdgeNode->mXT;
                }
                sEdgeNode->mOutp[STD_GPC_ABOVE] = NULL;
            }
        }
    } /* === END OF SCANBEAM PROCESSING ================================== */

    /* Generate result tristrip from tlist */
    aOutTristrip->mStrip = NULL;
    aOutTristrip->mNumStrips = countTristrips(sTriList);
    if (aOutTristrip->mNumStrips > 0)
    {
        IDE_TEST( aQmxMem->alloc( aOutTristrip->mNumStrips
                                  * ID_SIZEOF(stdGpcVertexList),
                                  (void**) & aOutTristrip->mStrip )
                  != IDE_SUCCESS );

        s = 0;
        for ( sTriNode = sTriList;
              sTriNode != NULL;
              sTriNode = sTriNextNode )
        {
            sTriNextNode = sTriNode->mNext;

            if ( sTriNode->mActive > 2 )
            {
                /* Valid tristrip: copy the vertices and free the heap */
                aOutTristrip->mStrip[s].mNumVertices = sTriNode->mActive;
                IDE_TEST( aQmxMem->alloc( sTriNode->mActive *
                                          ID_SIZEOF(stdGpcVertex),
                                          (void**) & aOutTristrip->mStrip[s].mVertex )
                          != IDE_SUCCESS );
                
                v = 0;
                if ( STD_GPC_INVERT_TRISTRIPS )
                {
                    sLeftVtx = sTriNode->mV[STD_GPC_RIGHT];
                    sRightVtx = sTriNode->mV[STD_GPC_LEFT];
                }
                else
                {
                    sLeftVtx = sTriNode->mV[STD_GPC_LEFT];
                    sRightVtx = sTriNode->mV[STD_GPC_RIGHT];
                }
                while ( (sLeftVtx != NULL) || (sRightVtx != NULL) )
                {
                    if (sLeftVtx != NULL)
                    {
                        sLeftNextVtx = sLeftVtx->mNext;
                        aOutTristrip->mStrip[s].mVertex[v].mX = sLeftVtx->mX;
                        aOutTristrip->mStrip[s].mVertex[v].mY = sLeftVtx->mY;
                        v++;
                        sLeftVtx = sLeftNextVtx;
                    }
                    if (sRightVtx != NULL)
                    {
                        sRightNextVtx = sRightVtx->mNext;
                        aOutTristrip->mStrip[s].mVertex[v].mX = sRightVtx->mX;
                        aOutTristrip->mStrip[s].mVertex[v].mY = sRightVtx->mY;
                        v++;
                        sRightVtx = sRightNextVtx;
                    }
                }
                s++;
            }
            else
            {
                /* Invalid tristrip: just free the heap */
                /* sTriNode->mV[STD_GPC_LEFT]  */
                /* sTriNode->mV[STD_GPC_RIGHT] */
            }

            sTriNode = NULL;
        }
    }

    IDE_EXCEPTION_CONT(STD_GPC_TRISTRIP_GLIP_RETURN);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


//=========================================================================
//                             Private Functions
//=========================================================================

void
stdGpc::insertBound(stdGpcEdgeNode ** aBoundNode,
                    stdGpcEdgeNode  * aNewNode )
{
    stdGpcEdgeNode *sExistingBound;
    while(1)
    {
        if ( *aBoundNode == NULL )
        {
        /* Link node e to the tail of the list */
            *aBoundNode = aNewNode;
            break;
        }
        else
        {
            /* Do primary sort on the x field */
            if ( aNewNode[0].mBot.mX < (*aBoundNode)[0].mBot.mX )
            {
                /* Insert a new node mid-list */
                sExistingBound = *aBoundNode;
                *aBoundNode = aNewNode;
                (*aBoundNode)->mNextBound = sExistingBound;
                break;
                
            }
            else
            {
                if ( aNewNode[0].mBot.mX == (*aBoundNode)[0].mBot.mX )
                {
                    /* Do secondary sort on the dx field */
                    if ( aNewNode[0].mDX < (*aBoundNode)[0].mDX )
                    {
                        /* Insert a new node mid-list */
                        sExistingBound = *aBoundNode;
                        *aBoundNode = aNewNode;
                        (*aBoundNode)->mNextBound = sExistingBound;
                        break;
                        
                    }
                    else
                    {
                        /* Head further down the list */
                        aBoundNode = &((*aBoundNode)->mNextBound);
                    }
                }
                else
                {
                    /* Head further down the list */
                    aBoundNode = &((*aBoundNode)->mNextBound);
                }
            }
            
        }
    }
}

IDE_RC
stdGpc::boundList( iduMemory        * aQmxMem,
                   stdGpcLMTNode   ** aLMTNode,
                   SDouble            aY,
                   stdGpcEdgeNode *** aReturnEdge )
{
    stdGpcEdgeNode ** sResult;
    
    stdGpcLMTNode * sExistingNode;
    
    while(1)
    {
        
        if ( *aLMTNode == NULL )
        {
            /* Add node onto the tail end of the LMT */
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcLMTNode),
                                      (void **) aLMTNode )
                      != IDE_SUCCESS );

            (*aLMTNode)->mY = aY;
            (*aLMTNode)->mFirstBound = NULL;
            (*aLMTNode)->mNext = NULL;
            sResult = &((*aLMTNode)->mFirstBound);
            break;
            
        }
        else
        {
            if ( aY < (*aLMTNode)->mY )
            {
                /* Insert a new LMT node before the current node */
                sExistingNode= *aLMTNode;
                IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcLMTNode),
                                          (void **) aLMTNode )
                          != IDE_SUCCESS );
                (*aLMTNode)->mY = aY;
                (*aLMTNode)->mFirstBound = NULL;
                (*aLMTNode)->mNext = sExistingNode;
                sResult = &((*aLMTNode)->mFirstBound);
                break;
                
            }
            else if ( aY > (*aLMTNode)->mY )
            {
                /* Head further up the LMT */
                aLMTNode = &((*aLMTNode)->mNext);
            }
            else
            {
                /* Use this existing LMT node */
                sResult = &((*aLMTNode)->mFirstBound);
                break;
                
            }
        }
        
    }

    *aReturnEdge = sResult;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
stdGpc::add2SChunk( iduMemory     * aQmxMem,
                    SInt          * aEntries,
                    stdGpcChunk  ** aChunk,
                    SDouble         aY )
{
    stdGpcChunk *sPtr = * aChunk;
    stdGpcChunk *sBackup = * aChunk;

    while( sPtr != NULL  && sPtr->mCount == STD_GPC_CHUNK_MAX )
    {
        sBackup = sPtr;
        sPtr = sPtr->mNext;
    }

    if ( sPtr == NULL)
    {
        IDE_TEST( aQmxMem->alloc( ID_SIZEOF( stdGpcChunk ),
                                  (void**) &sPtr )
                  != IDE_SUCCESS );
        if (*aChunk == NULL )
        {
            *aChunk = sPtr;
        }
        else
        {
            sBackup->mNext = sPtr;
        }
        sPtr->mCount = 0;
        sPtr->mNext = NULL;
    }

    sPtr->mY[(sPtr->mCount)++] = aY;
    (*aEntries)++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

void
stdGpc::buildSChunk( SInt          * aEntries,
                     SDouble       * aSBT,
                     stdGpcChunk   * aChunk )
{
    UInt sCurr = 0;
    UInt sTotal = 0;
    UInt sIndex = 1;
    stdGpcChunk *sPtr = aChunk;
    SDouble sComp;
    
    while( sPtr )
    {
        idlOS::memcpy(aSBT + sTotal, (sPtr->mY), ID_SIZEOF (SDouble) * ( sPtr->mCount ));
        sTotal += sPtr->mCount;
        sPtr = sPtr->mNext;
    }
    
    idlOS::qsort( aSBT, sTotal, ID_SIZEOF(SDouble), cmpDouble);

    sComp = aSBT[0];
    
    for (sCurr = 1 ; sCurr < sTotal ; sCurr++)
    {
        while( sCurr < sTotal && sComp == aSBT[sCurr] )
        {
            sCurr++;
        }

        if ( sCurr >= sTotal )
        {
            break;
        }
        sComp = aSBT[sCurr];
        
        aSBT[sIndex++ ] = sComp;
        
    }

    *aEntries = sIndex ;
}

SInt
stdGpc::countOptimalVertices( stdGpcVertexList * aVertexList )
{
    SInt i;
    SInt sResult = 0;

    IDE_DASSERT( aVertexList != NULL );
    
    /* Ignore non-contributing contours */
    if ( aVertexList->mNumVertices > 0 )
    {
        for ( i = 0; i < aVertexList->mNumVertices; i++ )
        {
            /* Ignore superfluous vertices embedded in horizontal edges */
            if ( STD_GPC_OPTIMAL( aVertexList->mVertex,
                                  i,
                                  aVertexList->mNumVertices) == ID_TRUE )
            {
                sResult++;
            }
        }
        
    }
    return sResult;
}


IDE_RC
stdGpc::buildLMT( iduMemory       * aQmxMem,
                  stdGpcLMTNode  ** aLmtNode,
                  stdGpcChunk    ** aChunk,
                  SInt            * aSBTEntries,
                  stdGpcPolygon   * aPolygon,
                  SInt              aType,
                  stdGpcOP          aOP,
                  stdGpcEdgeNode ** aReturnEdge )
{
    stdGpcEdgeNode   * sResult;
    
    stdGpcEdgeNode   * sEdgeNode;
    stdGpcEdgeNode  ** sBoundNode;

    SInt c, i, v;
    SInt sMin, sMax;
    SInt sNumEdges, sNumVertices;
    
    SInt sTotalVertices = 0;
    SInt sEdgeIndex = 0;

    sResult = NULL;
    
    for ( c = 0; c < aPolygon->mNumContours; c++ )
    {
        sTotalVertices += countOptimalVertices( & aPolygon->mContour[c] );
    }

    if(sTotalVertices > 0)
    {
        /* Create the entire input polygon edge table in one go */
        IDE_TEST( aQmxMem->alloc( sTotalVertices * ID_SIZEOF(stdGpcEdgeNode),
                                  (void**) & sResult ) != IDE_SUCCESS );

        for ( c= 0; c < aPolygon->mNumContours; c++ )
        {
            if ( aPolygon->mContour[c].mNumVertices < 0 )
            {
                /* Ignore the non-contributing contour
                   and repair the vertex count */
                aPolygon->mContour[c].mNumVertices -=
                    aPolygon->mContour[c].mNumVertices;
            }
            else
            {
                /* Perform contour optimisation */
                sNumVertices= 0;
                for ( i = 0; i < aPolygon->mContour[c].mNumVertices; i++ )
                {
                    if ( STD_GPC_OPTIMAL( aPolygon->mContour[c].mVertex,
                                          i,
                                          aPolygon->mContour[c].mNumVertices )
                         == ID_TRUE )
                    {
                        sResult[sNumVertices].mVertex.mX =
                            aPolygon->mContour[c].mVertex[i].mX;
                        sResult[sNumVertices].mVertex.mY =
                            aPolygon->mContour[c].mVertex[i].mY;

                        /* Record vertex in the scanbeam table */
                        IDE_TEST( add2SChunk( aQmxMem,
                                              aSBTEntries,
                                              aChunk,
                                              sResult[sNumVertices].mVertex.mY )
                                  != IDE_SUCCESS );

                        sNumVertices++;
                    }
                }

                /* Do the contour forward pass */
                for ( sMin= 0; sMin < sNumVertices; sMin++ )
                {
                    /* If a forward local minimum... */
                    if ( STD_GPC_FWD_MIN(sResult, sMin, sNumVertices) == ID_TRUE )
                    {
                        /* Search for the next local maximum... */
                        sNumEdges = 1;
                        sMax= STD_GPC_NEXT_INDEX( sMin, sNumVertices );
                        while ( STD_GPC_NOT_FMAX( sResult,
                                                  sMax,
                                                  sNumVertices ) == ID_TRUE )
                        {
                            sNumEdges++;
                            sMax= STD_GPC_NEXT_INDEX( sMax, sNumVertices);
                        }

                        /* Build the next edge list */
                        sEdgeNode = &sResult[sEdgeIndex];
                        sEdgeIndex += sNumEdges;
                        v= sMin;
                        sEdgeNode[0].mBstate[STD_GPC_BELOW]= STD_GPC_UNBUNDLED;
                        sEdgeNode[0].mBundle[STD_GPC_BELOW][STD_GPC_CLIP]
                            = STD_GPC_FALSE;
                        sEdgeNode[0].mBundle[STD_GPC_BELOW][STD_GPC_SUBJ]
                            = STD_GPC_FALSE;
                    
                        for ( i = 0; i < sNumEdges; i++ )
                        {
                            sEdgeNode[i].mXB = sResult[v].mVertex.mX;
                            sEdgeNode[i].mBot.mX = sResult[v].mVertex.mX;
                            sEdgeNode[i].mBot.mY = sResult[v].mVertex.mY;

                            v = STD_GPC_NEXT_INDEX( v, sNumVertices );

                            sEdgeNode[i].mTop.mX = sResult[v].mVertex.mX;
                            sEdgeNode[i].mTop.mY = sResult[v].mVertex.mY;
                            sEdgeNode[i].mDX =
                                ( sResult[v].mVertex.mX - sEdgeNode[i].mBot.mX ) /
                                ( sEdgeNode[i].mTop.mY - sEdgeNode[i].mBot.mY );
                            sEdgeNode[i].mType = aType;
                            sEdgeNode[i].mOutp[STD_GPC_ABOVE] = NULL;
                            sEdgeNode[i].mOutp[STD_GPC_BELOW] = NULL;
                            sEdgeNode[i].mNext = NULL;
                            sEdgeNode[i].mPrev = NULL;
                            sEdgeNode[i].mSucc
                                = ( (sNumEdges > 1) && (i < (sNumEdges - 1)) )
                                ? &(sEdgeNode[i + 1]) : NULL;
                            sEdgeNode[i].mPred
                                = ((sNumEdges > 1) && (i > 0))
                                ? &(sEdgeNode[i - 1]) : NULL;
                            sEdgeNode[i].mNextBound = NULL;
                            sEdgeNode[i].mBside[STD_GPC_CLIP]
                                = (aOP == STD_GPC_DIFF)
                                ? STD_GPC_RIGHT : STD_GPC_LEFT;
                            sEdgeNode[i].mBside[STD_GPC_SUBJ]= STD_GPC_LEFT;
                        }

                        IDE_TEST( boundList( aQmxMem,
                                             aLmtNode,
                                             sResult[sMin].mVertex.mY,
                                             & sBoundNode ) != IDE_SUCCESS );
                    
                        insertBound( sBoundNode, sEdgeNode );
                    }
                }

                /* Do the contour reverse pass */
                for ( sMin = 0; sMin < sNumVertices; sMin++ )
                {
                    /* If a reverse local minimum... */
                    if ( STD_GPC_REV_MIN( sResult, sMin, sNumVertices) == ID_TRUE )
                    {
                        /* Search for the previous local maximum... */
                        sNumEdges = 1;
                        sMax = STD_GPC_PREV_INDEX( sMin, sNumVertices );
                        while ( STD_GPC_NOT_RMAX( sResult, sMax, sNumVertices)
                                == ID_TRUE )
                        {
                            sNumEdges++;
                            sMax= STD_GPC_PREV_INDEX( sMax, sNumVertices );
                        }

                        /* Build the previous edge list */
                        sEdgeNode = &sResult[sEdgeIndex];
                        sEdgeIndex += sNumEdges;
                        v = sMin;
                        sEdgeNode[0].mBstate[STD_GPC_BELOW] = STD_GPC_UNBUNDLED;
                        sEdgeNode[0].mBundle[STD_GPC_BELOW][STD_GPC_CLIP]
                            = STD_GPC_FALSE;
                        sEdgeNode[0].mBundle[STD_GPC_BELOW][STD_GPC_SUBJ]
                            = STD_GPC_FALSE;
                        for ( i = 0; i < sNumEdges; i++ )
                        {
                            sEdgeNode[i].mXB = sResult[v].mVertex.mX;
                            sEdgeNode[i].mBot.mX = sResult[v].mVertex.mX;
                            sEdgeNode[i].mBot.mY = sResult[v].mVertex.mY;

                            v = STD_GPC_PREV_INDEX( v, sNumVertices );

                            sEdgeNode[i].mTop.mX = sResult[v].mVertex.mX;
                            sEdgeNode[i].mTop.mY = sResult[v].mVertex.mY;
                            sEdgeNode[i].mDX
                                = (sResult[v].mVertex.mX - sEdgeNode[i].mBot.mX)
                                / (sEdgeNode[i].mTop.mY - sEdgeNode[i].mBot.mY);
                            sEdgeNode[i].mType = aType;
                            sEdgeNode[i].mOutp[STD_GPC_ABOVE] = NULL;
                            sEdgeNode[i].mOutp[STD_GPC_BELOW] = NULL;
                            sEdgeNode[i].mNext = NULL;
                            sEdgeNode[i].mPrev = NULL;
                            sEdgeNode[i].mSucc
                                = ((sNumEdges > 1) && (i < (sNumEdges - 1)))
                                ? &(sEdgeNode[i + 1]) : NULL;
                            sEdgeNode[i].mPred
                                = ((sNumEdges > 1) && (i > 0))
                                ? &(sEdgeNode[i - 1]) : NULL;
                            sEdgeNode[i].mNextBound = NULL;
                            sEdgeNode[i].mBside[STD_GPC_CLIP]
                                = (aOP == STD_GPC_DIFF)
                                ? STD_GPC_RIGHT : STD_GPC_LEFT;
                            sEdgeNode[i].mBside[STD_GPC_SUBJ]= STD_GPC_LEFT;
                        }

                        IDE_TEST( boundList( aQmxMem,
                                             aLmtNode,
                                             sResult[sMin].mVertex.mY,
                                             & sBoundNode ) != IDE_SUCCESS );
                    
                        insertBound( sBoundNode, sEdgeNode );
                    
                    }
                }
            }
        }
    }

    *aReturnEdge = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void
stdGpc::addEdge2AET( stdGpcEdgeNode ** aAET,
                     stdGpcEdgeNode  * aEdge,
                     stdGpcEdgeNode  * aPrev )
{
    while(1)
    {
        if ( *aAET == NULL )
        {
            /* Append edge onto the tail end of the AET */
            *aAET = aEdge;
            aEdge->mPrev = aPrev;
            aEdge->mNext = NULL;
            break;
        }
        else
        {
            /* Do primary sort on the xb field */
            if ( aEdge->mXB < (*aAET)->mXB )
            {
                /* Insert edge here (before the AET edge) */
                aEdge->mPrev = aPrev;
                aEdge->mNext = *aAET;
                (*aAET)->mPrev= aEdge;
                *aAET = aEdge;
                break;
            }
            else
            {
                if ( aEdge->mXB == (*aAET)->mXB )
                {
                    /* Do secondary sort on the dx field */
                    if ( aEdge->mDX < (*aAET)->mDX )
                    {
                        /* Insert edge here (before the AET edge) */
                        aEdge->mPrev = aPrev;
                        aEdge->mNext = *aAET;
                        (*aAET)->mPrev = aEdge;
                        *aAET = aEdge;
                        break;
                    }
                    else
                    {
                        /* Head further into the AET */
                        aPrev = *aAET;
                        aAET = &((*aAET)->mNext);
                    }
                }
                else
                {
                    /* Head further into the AET */
                    aPrev = *aAET;
                    aAET = & ((*aAET)->mNext);
                }
            }
        }
    }
}

IDE_RC
stdGpc::addIntersection( iduMemory      * aQmxMem,
                         stdGpcITNode  ** aITNode,
                         stdGpcEdgeNode * aEdge0,
                         stdGpcEdgeNode * aEdge1,
                         SDouble          aX,
                         SDouble          aY )
{
    stdGpcITNode * sExistingNode;

    while(1)
    {
        if ( *aITNode == NULL )
        {
            /* Append a new node to the tail of the list */
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcITNode),
                                      (void **) aITNode )
                      != IDE_SUCCESS );
        
            (*aITNode)->mIE[0] = aEdge0;
            (*aITNode)->mIE[1] = aEdge1;
            (*aITNode)->mPoint.mX = aX;
            (*aITNode)->mPoint.mY = aY;
            (*aITNode)->mNext = NULL;
            break;
        }
        else
        {
            if ( (*aITNode)->mPoint.mY > aY )
            {
                /* Insert a new node mid-list */
                sExistingNode = *aITNode;
                IDE_TEST( aQmxMem->alloc( ID_SIZEOF( stdGpcITNode ),
                                          (void**) aITNode )
                          != IDE_SUCCESS );
            
                (*aITNode)->mIE[0] = aEdge0;
                (*aITNode)->mIE[1] = aEdge1;
                (*aITNode)->mPoint.mX = aX;
                (*aITNode)->mPoint.mY = aY;
                (*aITNode)->mNext = sExistingNode;
                break;
            }
            else
            {
                /* Head further down the list */
                aITNode = &((*aITNode)->mNext);
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::addSTEdge( iduMemory      * aQmxMem,
                   stdGpcSTNode  ** aSTNode,
                   stdGpcITNode  ** aITNode,
                   stdGpcEdgeNode * aEdge,
                   SDouble          aDY )
{
    stdGpcSTNode * sExistingNode;
    SDouble   r,x,y;
    SDouble   sDen;

    if ( *aSTNode == NULL )
    {
        /* Append edge onto the tail end of the ST */
        IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcSTNode),
                                  (void **) aSTNode )
                  != IDE_SUCCESS );
        
        (*aSTNode)->mEdge = aEdge;
        (*aSTNode)->mXB = aEdge->mXB;
        (*aSTNode)->mXT = aEdge->mXT;
        (*aSTNode)->mDX = aEdge->mDX;
        (*aSTNode)->mPrev = NULL;
    }
    else
    {
        sDen = ((*aSTNode)->mXT - (*aSTNode)->mXB) - (aEdge->mXT - aEdge->mXB);

        /* If new edge and ST edge don't cross */
        if ( (aEdge->mXT >= (*aSTNode)->mXT) ||
             (aEdge->mDX == (*aSTNode)->mDX) || 
             (idlOS::fabs(sDen) <= STD_GPC_EPSILON) )
        {
            /* No intersection - insert edge here (before the ST edge) */
            sExistingNode= *aSTNode;
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcSTNode),
                                      (void **) aSTNode )
                  != IDE_SUCCESS );
            (*aSTNode)->mEdge = aEdge;
            (*aSTNode)->mXB = aEdge->mXB;
            (*aSTNode)->mXT = aEdge->mXT;
            (*aSTNode)->mDX = aEdge->mDX;
            (*aSTNode)->mPrev = sExistingNode;
        }
        else
        {
            /* Compute intersection between new edge and ST edge */
            r = (aEdge->mXB - (*aSTNode)->mXB) / sDen;
            x = (*aSTNode)->mXB + r * ((*aSTNode)->mXT - (*aSTNode)->mXB);
            y = r * aDY;

            /* Insert the edge pointers and the intersection point in the IT */
            IDE_TEST( addIntersection( aQmxMem,
                                       aITNode,
                                       (*aSTNode)->mEdge,
                                       aEdge,
                                       x,
                                       y )
                      != IDE_SUCCESS );
            
            /* Head further into the ST */
            IDE_TEST( addSTEdge( aQmxMem,
                                 &((*aSTNode)->mPrev),
                                 aITNode,
                                 aEdge,
                                 aDY )
                      != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::buildIntersectionTable( iduMemory      * aQmxMem,
                                stdGpcITNode  ** aITNode,
                                stdGpcEdgeNode * aAET,
                                SDouble          aDY )
{
    stdGpcSTNode   * sSTNode;
    stdGpcEdgeNode * sEdge;

    /* Build intersection table for the current scanbeam */
    *aITNode = NULL;
    
    sSTNode = NULL;

    /* Process each AET edge */
    for ( sEdge= aAET; sEdge != NULL; sEdge= sEdge->mNext )
    {
        if ( (sEdge->mBstate[STD_GPC_ABOVE] == STD_GPC_BUNDLE_HEAD) ||
             (sEdge->mBundle[STD_GPC_ABOVE][STD_GPC_CLIP] == ID_TRUE) ||
             (sEdge->mBundle[STD_GPC_ABOVE][STD_GPC_SUBJ] == ID_TRUE) )
        {
            IDE_TEST( addSTEdge ( aQmxMem, &sSTNode, aITNode, sEdge, aDY )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::countContours( stdGpcPolygonNode * aPolygon,
                       SInt              * aResult )
{
    SInt         sNumContour;
    SInt         sNumVertex;

    stdGpcVertexNode * sNode;
    
    for ( sNumContour = 0;
          aPolygon != NULL;
          aPolygon = aPolygon->mNext )
    {
        if ( aPolygon->mActive > 0 )
        {
            /* Count the vertices in the current contour */
            sNumVertex = 0;
            for ( sNode = aPolygon->mProxy->mV[STD_GPC_LEFT];
                  sNode != NULL;
                  sNode = sNode->mNext)
            {
                sNumVertex++;
            }

            /* Record valid vertex counts in the active field */
            if ( sNumVertex > 2)
            {
                aPolygon->mActive = sNumVertex;
                sNumContour++;
            }
            else
            {
                /* Invalid contour: just free the heap */
                /* aPolygon->mProxy->mV[STD_GPC_LEFT] */
                aPolygon->mActive = 0;
            }
        }
    }
    
    *aResult = sNumContour;

    return IDE_SUCCESS;
}

IDE_RC
stdGpc::addLeft( iduMemory         * aQmxMem,
                 stdGpcPolygonNode * aPolygon,
                 SDouble             aX,
                 SDouble             aY )
{
    stdGpcVertexNode * sNode;

    IDE_TEST_RAISE( aPolygon == NULL, err_incompatible_type );
    
    /* Create a new vertex node and set its fields */
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcVertexNode),
                              (void**) & sNode )
              != IDE_SUCCESS );
    
    sNode->mX = aX;
    sNode->mY = aY;

    /* Add vertex nv to the left end of the polygon's vertex list */
    sNode->mNext = aPolygon->mProxy->mV[STD_GPC_LEFT];

    /* Update proxy->[LEFT] to point to nv */
    aPolygon->mProxy->mV[STD_GPC_LEFT] = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_incompatible_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::mergeLeft( stdGpcPolygonNode * aPolygon1,
                   stdGpcPolygonNode * aPolygon2,
                   stdGpcPolygonNode * aPolygonList )
{
    stdGpcPolygonNode * sTarget;

    IDE_TEST_RAISE( aPolygon1 == NULL, err_invalid_geometry );
    IDE_TEST_RAISE( aPolygon2 == NULL, err_invalid_geometry );    

    /* Label contour as a hole */
    aPolygon2->mProxy->mHole = ID_TRUE;

    if ( aPolygon1->mProxy != aPolygon2->mProxy )
    {
        /* Assign p's vertex list to the left end of q's list */
        aPolygon1->mProxy->mV[STD_GPC_RIGHT]->mNext =
            aPolygon2->mProxy->mV[STD_GPC_LEFT];
        aPolygon2->mProxy->mV[STD_GPC_LEFT] =
            aPolygon1->mProxy->mV[STD_GPC_LEFT];

        /* Redirect any p->proxy references to q->proxy */
    
        for ( sTarget = aPolygon1->mProxy;
              aPolygonList != NULL;
              aPolygonList = aPolygonList->mNext )
        {
            if ( aPolygonList->mProxy == sTarget )
            {
                aPolygonList->mActive = 0;
                aPolygonList->mProxy= aPolygon2->mProxy;
            }
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_geometry);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
stdGpc::addRight( iduMemory         * aQmxMem,
                  stdGpcPolygonNode * aPolygon,
                  SDouble             aX,
                  SDouble             aY )
{
    stdGpcVertexNode * sNode;

    IDE_TEST_RAISE( aPolygon == NULL, err_invalid_geometry );
    
    /* Create a new vertex node and set its fields */
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcVertexNode),
                              (void**) & sNode )
              != IDE_SUCCESS );
    
    sNode->mX = aX;
    sNode->mY = aY;
    sNode->mNext = NULL;

    /* Add vertex nv to the right end of the polygon's vertex list */
    aPolygon->mProxy->mV[STD_GPC_RIGHT]->mNext = sNode;

    /* Update proxy->mV[RIGHT] to point to nv */
    aPolygon->mProxy->mV[STD_GPC_RIGHT] = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_geometry);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::mergeRight( stdGpcPolygonNode * aPolygon1,
                    stdGpcPolygonNode * aPolygon2,
                    stdGpcPolygonNode * aPolygonList )
{
    stdGpcPolygonNode * sTarget;

    IDE_TEST_RAISE( aPolygon1 == NULL, err_invalid_geometry );
    IDE_TEST_RAISE( aPolygon2 == NULL, err_invalid_geometry );

    /* Label contour as external */
    aPolygon2->mProxy->mHole = STD_GPC_FALSE;

    if ( aPolygon1->mProxy != aPolygon2->mProxy)
    {
        /* Assign p's vertex list to the right end of q's list */
        aPolygon2->mProxy->mV[STD_GPC_RIGHT]->mNext =
            aPolygon1->mProxy->mV[STD_GPC_LEFT];
        aPolygon2->mProxy->mV[STD_GPC_RIGHT] =
            aPolygon1->mProxy->mV[STD_GPC_RIGHT];

        /* Redirect any p->proxy references to q->proxy */
        for ( sTarget = aPolygon1->mProxy;
              aPolygonList != NULL;
              aPolygonList = aPolygonList->mNext )
        {
            if ( aPolygonList->mProxy == sTarget )
            {
                aPolygonList->mActive = 0;
                aPolygonList->mProxy = aPolygon2->mProxy;
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_geometry );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::addLocalMin( iduMemory          * aQmxMem,
                     stdGpcPolygonNode ** aPolygon,
                     stdGpcEdgeNode     * aEdge,
                     SDouble              aX,
                     SDouble              aY )
{
    stdGpcPolygonNode * sExistingMin;
    stdGpcVertexNode  * sVertex;

    sExistingMin = *aPolygon;

    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcPolygonNode),
                              (void **) aPolygon )
              != IDE_SUCCESS );

    /* Create a new vertex node and set its fields */
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcVertexNode),
                              (void **) & sVertex )
              != IDE_SUCCESS );
    
    sVertex->mX = aX;
    sVertex->mY = aY;
    sVertex->mNext = NULL;

    /* Initialise proxy to point to p itself */
    (*aPolygon)->mProxy = (*aPolygon);
    (*aPolygon)->mActive = 1;
    (*aPolygon)->mNext = sExistingMin;

    /* Make v[LEFT] and v[RIGHT] point to new vertex nv */
    (*aPolygon)->mV[STD_GPC_LEFT] = sVertex;
    (*aPolygon)->mV[STD_GPC_RIGHT] = sVertex;

    /* Assign polygon p to the edge */
    aEdge->mOutp[STD_GPC_ABOVE]= *aPolygon;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SInt
stdGpc::countTristrips( stdGpcPolygonNode * aNode )
{
    SInt sTotal;

    for ( sTotal = 0; aNode != NULL; aNode = aNode->mNext )
    {
        if ( aNode->mActive > 2)
        {
            sTotal++;
        }
    }
    
    return sTotal;
}

IDE_RC
stdGpc::addVertex( iduMemory         * aQmxMem,
                   stdGpcVertexNode ** aVertex,
                   SDouble             aX,
                   SDouble             aY )
{
    while(1)
    {
        if ( *aVertex == NULL )
        {
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(stdGpcVertexNode),
                                      (void**) aVertex )
                      != IDE_SUCCESS );
        
            (*aVertex)->mX = aX;
            (*aVertex)->mY = aY;
            (*aVertex)->mNext = NULL;
            break;
        }
        else
        {
            /* Head further down the list */
            aVertex  = &((*aVertex)->mNext);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::newTristrip( iduMemory          * aQmxMem,
                     stdGpcPolygonNode ** aTristrip,
                     stdGpcEdgeNode     * aEdge,
                     SDouble              aX,
                     SDouble              aY )
{
    while(1)
    {
        if ( *aTristrip == NULL )
        {
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF( stdGpcPolygonNode ),
                                      (void **) aTristrip )
                      != IDE_SUCCESS );
                                     
            (*aTristrip)->mNext = NULL;
            (*aTristrip)->mV[STD_GPC_LEFT] = NULL;
            (*aTristrip)->mV[STD_GPC_RIGHT] = NULL;
            (*aTristrip)->mActive = 1;
            IDE_TEST( addVertex( aQmxMem, &((*aTristrip)->mV[STD_GPC_LEFT]), aX, aY)
                      != IDE_SUCCESS );
            aEdge->mOutp[STD_GPC_ABOVE] = *aTristrip;
            break;
        }
        else
        {
            /* Head further down the list */
            aTristrip = &((*aTristrip)->mNext);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::createContourBBoxes( iduMemory     * aQmxMem,
                             stdGpcPolygon * aPolygon,
                             stdGpcBBox   ** aResult )
{
    stdGpcBBox * sBox;
    SInt         c, v;

    IDE_TEST( aQmxMem->alloc( aPolygon->mNumContours
                              * ID_SIZEOF( stdGpcBBox ),
                              (void**) & sBox )
              != IDE_SUCCESS );

    /* Construct contour bounding boxes */
    for ( c = 0; c < aPolygon->mNumContours; c++)
    {
        /* Initialise bounding box extent */
        sBox[c].mXmin = DBL_MAX;
        sBox[c].mYmin = DBL_MAX;
        sBox[c].mXmax = -DBL_MAX;
        sBox[c].mYmax = -DBL_MAX;

        for ( v = 0; v < aPolygon->mContour[c].mNumVertices; v++ )
        {
            /* Adjust bounding box */
            if ( aPolygon->mContour[c].mVertex[v].mX < sBox[c].mXmin )
            {
                sBox[c].mXmin = aPolygon->mContour[c].mVertex[v].mX;
            }

            if ( aPolygon->mContour[c].mVertex[v].mY < sBox[c].mYmin )
            {
                sBox[c].mYmin = aPolygon->mContour[c].mVertex[v].mY;
            }
            
            if ( aPolygon->mContour[c].mVertex[v].mX > sBox[c].mXmax )
            {
                sBox[c].mXmax = aPolygon->mContour[c].mVertex[v].mX;
            }
            
            if ( aPolygon->mContour[c].mVertex[v].mY > sBox[c].mYmax )
            {
                sBox[c].mYmax = aPolygon->mContour[c].mVertex[v].mY;
            }
        }
    }

    *aResult = sBox;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stdGpc::miniMaxTest( iduMemory     * aQmxMem,
                     stdGpcPolygon * aSubj,
                     stdGpcPolygon * aClip,
                     stdGpcOP        aOP )
{
    stdGpcBBox * sSubjBox;
    stdGpcBBox * sClipBox;

    SInt     s, c;
    SInt   * sOvpTable;
    SInt     sOverlap;

    sSubjBox = NULL;
    sClipBox = NULL;
    sOvpTable = NULL;
    
    IDE_TEST( createContourBBoxes( aQmxMem, aSubj, & sSubjBox ) != IDE_SUCCESS );
    IDE_TEST( createContourBBoxes( aQmxMem, aClip, & sClipBox ) != IDE_SUCCESS );

    IDE_TEST( aQmxMem->alloc( aSubj->mNumContours * aClip->mNumContours
                              * ID_SIZEOF(SInt),
                              (void**) & sOvpTable )
              != IDE_SUCCESS );

    /* Check all subject contour bounding boxes against clip boxes */
    for ( s = 0; s < aSubj->mNumContours; s++)
    {
        for ( c = 0; c < aClip->mNumContours; c++ )
        {
            sOvpTable[c * aSubj->mNumContours + s] =
                ( !( (sSubjBox[s].mXmax < sClipBox[c].mXmin) ||
                     (sSubjBox[s].mXmin > sClipBox[c].mXmax)) ) &&
                ( !( (sSubjBox[s].mYmax < sClipBox[c].mYmin) ||
                     (sSubjBox[s].mYmin > sClipBox[c].mYmax)) )
                ? STD_GPC_TRUE : STD_GPC_FALSE;
        }
    }
    
    /* For each clip contour, search for any subject contour overlaps */
    for ( c = 0; c < aClip->mNumContours; c++)
    {
        sOverlap = STD_GPC_FALSE;
        for ( s = 0;
              (sOverlap != ID_TRUE) && (s < aSubj->mNumContours);
              s++ )
        {
            sOverlap = sOvpTable[c * aSubj->mNumContours + s];
        }

        if ( sOverlap != ID_TRUE )
        {
            /* Flag non contributing status by negating vertex count */
            aClip->mContour[c].mNumVertices = -aClip->mContour[c].mNumVertices;
        }
    }  

    if ( aOP == STD_GPC_INT )
    {  
        /* For each subject contour, search for any clip contour overlaps */
        for ( s = 0; s < aSubj->mNumContours; s++)
        {
            sOverlap = STD_GPC_FALSE;
            for ( c = 0;
                  ( sOverlap != ID_TRUE ) && (c < aClip->mNumContours);
                  c++ )
            {
                sOverlap = sOvpTable[c * aSubj->mNumContours + s];
            }

            if ( sOverlap != ID_TRUE )
            {
                /* Flag non contributing status by negating vertex count */
                aSubj->mContour[s].mNumVertices =
                    - aSubj->mContour[s].mNumVertices;
            }
        }  
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
