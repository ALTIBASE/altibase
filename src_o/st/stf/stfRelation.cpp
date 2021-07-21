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
 * $Id: stfRelation.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체와 Geometry 객체간의 관계 함수 구현
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtc.h>
#include <mtd.h>
#include <mtdTypes.h>

#include <qc.h>

#include <ste.h>
#include <stdTypes.h>
#include <stdUtils.h>
#include <stfRelation.h>
#include <stdPolyClip.h>

extern mtdModule stdGeometry;

/* Calculate MBR ******************************************************/

SInt stfRelation::isMBRIntersects( mtdValueInfo * aValueInfo1, // from index.
                                   mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR1 = (stdMBR*)aValueInfo1->value;
    sMBR2 = (stdMBR*)aValueInfo2->value;

    if((sMBR1->mMinX > sMBR2->mMaxX) || (sMBR1->mMaxX < sMBR2->mMinX) ||
       (sMBR1->mMinY > sMBR2->mMaxY) || (sMBR1->mMaxY < sMBR2->mMinY) )
    {
        return 0;
    }
    else
    {
        return 1; // TRUE
    }
}

SInt stfRelation::isMBRContains( mtdValueInfo * aValueInfo1, // from index.
                                 mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR1 = (stdMBR*)aValueInfo1->value;
    sMBR2 = (stdMBR*)aValueInfo2->value;

    if((sMBR1->mMinX <= sMBR2->mMinX) && (sMBR1->mMaxX >= sMBR2->mMaxX) &&
       (sMBR1->mMinY <= sMBR2->mMinY) && (sMBR1->mMaxY >= sMBR2->mMaxY)  )
    {
        return 1; // TRUE
    }
    else
    {
        return 0;
    }
}

SInt stfRelation::isMBRWithin( mtdValueInfo * aValueInfo1, // from index.
                               mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR1 = (stdMBR*)aValueInfo1->value;
    sMBR2 = (stdMBR*)aValueInfo2->value;

    if((sMBR1->mMinX >= sMBR2->mMinX) && (sMBR1->mMaxX <= sMBR2->mMaxX) &&
       (sMBR1->mMinY >= sMBR2->mMinY) && (sMBR1->mMaxY <= sMBR2->mMaxY) )
    {
        return 1; // TRUE
    }
    else
    {
        return 0;
    }
}

SInt stfRelation::isMBREquals( mtdValueInfo * aValueInfo1, // from index.
                               mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR1 = (stdMBR*)aValueInfo1->value;
    sMBR2 = (stdMBR*)aValueInfo2->value;

    if((sMBR1->mMinX == sMBR2->mMinX) && (sMBR1->mMaxX == sMBR2->mMaxX) &&
       (sMBR1->mMinY == sMBR2->mMinY) && (sMBR1->mMaxY == sMBR2->mMaxY) )
    {
        return 1; // TRUE
    }
    else
    {
        return 0;
    }
}

// TASK-3171 B-Tree Spatial
SInt stfRelation::compareMBR( mtdValueInfo * aValueInfo1, // from column 
                              mtdValueInfo * aValueInfo2 )// from row
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR2 = (stdMBR*)aValueInfo2->value;
    sMBR1 = &((const stdGeometryHeader*)
              mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                   aValueInfo1->value,
                                   aValueInfo1->flag,
                                   stdGeometry.staticNull ))->mMbr;

    if (sMBR1->mMinX > sMBR2 ->mMinX )
    {
        return 1;
    }
    else if (sMBR1->mMinX < sMBR2->mMinX)
    {
        return -1;
    }
    else { // sMBR1->minX == mbr2->minx

        if (sMBR1->mMinY > sMBR2->mMinY)
        {
            return 1;
        }
        else if (sMBR1->mMinY <sMBR2->mMinY)
        {
            return -1;
        }
        else { // sMBR1->minY == mbr2->minY
            if (sMBR1->mMaxX > sMBR2->mMaxX)
            {
                return 1;
            }
            else if (sMBR1->mMaxX < sMBR2->mMaxX)
            {
                return -1;
            }
            else // maxx == maxx
            {
                if (sMBR1->mMaxY > sMBR2->mMaxY)
                {
                    return 1;
                }
                else if (sMBR1->mMaxY < sMBR2->mMaxY)
                {
                    return -1;
                }
            }
        }
    }
    return 0; // exactly same two mbr!.
}

SInt stfRelation::compareMBRKey( mtdValueInfo * aValueInfo1, // from index 
                                 mtdValueInfo * aValueInfo2 )// from index 
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR1 = (stdMBR*)aValueInfo1->value;
    sMBR2 = (stdMBR*)aValueInfo2->value;
    if (sMBR1->mMinX > sMBR2 ->mMinX )
    {
        return 1;
    }
    else if (sMBR1->mMinX < sMBR2->mMinX)
    {
        return -1;
    }
    else { // sMBR1->minX == mbr2->minx

        if (sMBR1->mMinY > sMBR2->mMinY)
        {
            return 1;
        }
        else if (sMBR1->mMinY <sMBR2->mMinY)
        {
            return -1;
        }
        else { // sMBR1->minY == mbr2->minY
            if (sMBR1->mMaxX > sMBR2->mMaxX)
            {
                return 1;
            }
            else if (sMBR1->mMaxX < sMBR2->mMaxX)
            {
                return -1;
            }
            else // maxx == maxx
            {
                if (sMBR1->mMaxY > sMBR2->mMaxY)
                {
                    return 1;
                }
                else if (sMBR1->mMaxY < sMBR2->mMaxY)
                {
                    return -1;
                }
            }
        }
    }
    return 0; // exactly same two mbr!.
}

SInt stfRelation::stfMinXLTEMinX( mtdValueInfo * aValueInfo1, // from index.
                                  mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR1 = (stdMBR*)aValueInfo1->value;
    sMBR2 = (stdMBR*)aValueInfo2->value;

    if (sMBR1->mMinX <= sMBR2->mMinX)
    {
        return 1; // TRUE
    }
    else
    {
        return 0;
    }
}


SInt stfRelation::stfMinXLTEMaxX( mtdValueInfo * aValueInfo1, // from index.
                                  mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sMBR1;
    const stdMBR* sMBR2;

    sMBR1 = (stdMBR*)aValueInfo1->value;
    sMBR2 = (stdMBR*)aValueInfo2->value;

    if (sMBR1->mMinX <= sMBR2->mMaxX)
    {
        return 1; // TRUE
    }
    else
    {
        return 0;
    }
}

SInt stfRelation::stfFilterIntersects( mtdValueInfo * aValueInfo1, // from index.
                                       mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sIdxMBR;
    const stdMBR* sColMBR;

    sIdxMBR = (stdMBR*)aValueInfo1->value;
    sColMBR = (stdMBR*)aValueInfo2->value;

    if ((sIdxMBR->mMinY <= sColMBR->mMaxY) &&
        (sIdxMBR->mMaxX >= sColMBR->mMinX) &&
        (sIdxMBR->mMaxY >= sColMBR->mMinY))
    {
        return 1; // TRUE
    }
    else
    {
        return 0;
    }
}

SInt stfRelation::stfFilterContains( mtdValueInfo * aValueInfo1, // from index.
                                     mtdValueInfo * aValueInfo2 )// from key range.
{
    const stdMBR* sIdxMBR;
    const stdMBR* sColMBR;

    sIdxMBR = (stdMBR*)aValueInfo1->value;
    sColMBR = (stdMBR*)aValueInfo2->value;

    if ((sIdxMBR->mMinY <= sColMBR->mMinY) &&
        (sIdxMBR->mMaxX >= sColMBR->mMaxX) &&
        (sIdxMBR->mMaxY >= sColMBR->mMaxY))
    {
        return 1; // TRUE
    }
    else
    {
        return 0;
    }
}



/* Relation Functions **************************************************/

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Equals 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isEquals(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader* sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader* sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType     sIsEquals;
    SChar              sPattern[10];

    qcTemplate       * sQcTmplate;
    iduMemory        * sQmxMem;
    iduMemoryStatus    sQmxMemStatus;
    UInt               sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        if( stdUtils::isMBREquals( &sValue1->mMbr, &sValue2->mMbr )==ID_TRUE )
        {
            IDE_TEST( matrixEquals(
                (stdGeometryType*)sValue1,
                (stdGeometryType*)sValue2,
                sPattern) != IDE_SUCCESS );
            if( idlOS::strlen((SChar*)sPattern) > 0 )
            {
                // Memory 재사용을 위하여 현재 위치 기록
                IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
                sStage = 1;

                IDE_TEST( stfRelation::relate( sQmxMem,
                                               (stdGeometryType*)sValue1,
                                               (stdGeometryType*)sValue2,
                                               sPattern,
                                               &sIsEquals )
                          != IDE_SUCCESS );
                
                // Memory 재사용을 위한 Memory 이동
                sStage = 0;
                IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
            }
            else
            {
                sIsEquals = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            sIsEquals = MTD_BOOLEAN_FALSE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsEquals;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Disjoint 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isDisjoint(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType          sIsDisjoint;
    SChar                   sPattern[10];//"FF*FF****"; // Disjoint Metrix

    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        if( stdUtils::isMBRIntersects(&sValue1->mMbr,&sValue2->mMbr)==ID_TRUE )
        {
            IDE_TEST( matrixDisjoint(
                (stdGeometryType*)sValue1,
                (stdGeometryType*)sValue2,
                sPattern) != IDE_SUCCESS );
            if( idlOS::strlen((SChar*)sPattern) > 0 )
            {
                // Memory 재사용을 위하여 현재 위치 기록
                IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
                sStage = 1;

                IDE_TEST( stfRelation::relate( sQmxMem,
                                               (stdGeometryType*)sValue1,
                                               (stdGeometryType*)sValue2,
                                               sPattern,
                                               &sIsDisjoint )
                          != IDE_SUCCESS );
            
                // Memory 재사용을 위한 Memory 이동
                sStage = 0;
                IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
            }
            else
            {
                sIsDisjoint = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            sIsDisjoint = MTD_BOOLEAN_TRUE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsDisjoint;

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Intersects 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isIntersects(
                        mtcNode*     aNode,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        void*        aInfo,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType          sIsIntersects;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        IDE_TEST( isDisjoint( aNode,
                              aStack,
                              aRemain,
                              aInfo,
                              aTemplate ) != IDE_SUCCESS );

        if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
        {
            sIsIntersects = MTD_BOOLEAN_FALSE;
        }
        else
        {
            sIsIntersects = MTD_BOOLEAN_TRUE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsIntersects;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Within 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isWithin(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType          sIsWithin;
    SChar                   sPattern[10];//="T*F**F***";

    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        if( stdUtils::isMBRWithin( &sValue1->mMbr, &sValue2->mMbr )==ID_TRUE )
        {
            IDE_TEST( matrixWithin(
                (stdGeometryType*)sValue1,
                (stdGeometryType*)sValue2,
                sPattern) != IDE_SUCCESS );
            if( idlOS::strlen((SChar*)sPattern) > 0 )
            {
                // Memory 재사용을 위하여 현재 위치 기록
                IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
                sStage = 1;

                IDE_TEST( stfRelation::relate( sQmxMem,
                                               (stdGeometryType*)sValue1,
                                               (stdGeometryType*)sValue2,
                                               sPattern,
                                               &sIsWithin )
                          != IDE_SUCCESS );
            
                // Memory 재사용을 위한 Memory 이동
                sStage = 0;
                IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
            }
            else
            {
                sIsWithin = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            sIsWithin = MTD_BOOLEAN_FALSE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsWithin;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Contains 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isContains(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType          sIsContains;
    SChar                   sPattern[10];

    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        if( stdUtils::isMBRContains(&sValue1->mMbr,&sValue2->mMbr)==ID_TRUE )
        {
            IDE_TEST( matrixWithin(
                (stdGeometryType*)sValue2,
                (stdGeometryType*)sValue1,
                sPattern) != IDE_SUCCESS );
            if( idlOS::strlen((SChar*)sPattern) > 0 )
            {
                // Memory 재사용을 위하여 현재 위치 기록
                IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
                sStage = 1;

                IDE_TEST( stfRelation::relate( sQmxMem,
                                               (stdGeometryType*)sValue2,
                                               (stdGeometryType*)sValue1,
                                               sPattern,
                                               &sIsContains )
                          != IDE_SUCCESS );
                
                // Memory 재사용을 위한 Memory 이동
                sStage = 0;
                IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
            }
            else
            {
                sIsContains = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            sIsContains = MTD_BOOLEAN_FALSE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsContains;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Crosses 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isCrosses(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType          sIsCrosses;
    SChar                   sPattern[10];

    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        if( stdUtils::isMBRIntersects(&sValue1->mMbr,&sValue2->mMbr)==ID_TRUE )
        {
            IDE_TEST( matrixCrosses(
                (stdGeometryType*)sValue1,
                (stdGeometryType*)sValue2,
                sPattern) != IDE_SUCCESS );
            if( idlOS::strlen((SChar*)sPattern) > 0 )
            {
                // Memory 재사용을 위하여 현재 위치 기록
                IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
                sStage = 1;

                IDE_TEST( stfRelation::relate( sQmxMem,
                                               (stdGeometryType*)sValue1,  // Fix BUG-15432
                                               (stdGeometryType*)sValue2,
                                               sPattern,
                                               &sIsCrosses )
                          != IDE_SUCCESS );
                          
                // Memory 재사용을 위한 Memory 이동
                sStage = 0;
                IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
            }
            else
            {
                sIsCrosses = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            sIsCrosses = MTD_BOOLEAN_FALSE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsCrosses;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Overlaps 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isOverlaps(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType          sIsOverlaps;
    SChar                   sPattern[10];

    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        if( stdUtils::isMBRIntersects(&sValue1->mMbr,&sValue2->mMbr)==ID_TRUE )
        {
            IDE_TEST( matrixOverlaps(
                (stdGeometryType*)sValue1,
                (stdGeometryType*)sValue2,
                sPattern) != IDE_SUCCESS );
            if( idlOS::strlen((SChar*)sPattern) > 0 )
            {
                // Memory 재사용을 위하여 현재 위치 기록
                IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
                sStage = 1;

                IDE_TEST( stfRelation::relate( sQmxMem,
                                               (stdGeometryType*)sValue1,
                                               (stdGeometryType*)sValue2,
                                               sPattern,
                                               &sIsOverlaps )
                          != IDE_SUCCESS );
            
                // Memory 재사용을 위한 Memory 이동
                sStage = 0;
                IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
            }
            else
            {
                sIsOverlaps = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            sIsOverlaps = MTD_BOOLEAN_FALSE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsOverlaps;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 Touches 인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isTouches(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1 = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2 = (stdGeometryHeader *)aStack[2].value;
    mtdBooleanType          sIsTouches;
    UInt                    i;
    UInt                    sPatternCnt = 0;
    SChar                   sPattern[3][10]; /*={ "FT*******",
                                                  "F**T*****",
                                                  "F***T****" };*/

    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        *(mtdBooleanType*) aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        if( stdUtils::isMBRIntersects(&sValue1->mMbr,&sValue2->mMbr)==ID_TRUE )
        {
            IDE_TEST( matrixTouches(
                (stdGeometryType*)sValue1,
                (stdGeometryType*)sValue2,
                sPattern,
                sPatternCnt) != IDE_SUCCESS );
            if( idlOS::strlen((SChar*)sPattern) > 0 )
            {
                sIsTouches = MTD_BOOLEAN_FALSE;
                
                for( i = 0; ((i < sPatternCnt) && (sPattern[i] != 0x00)); i++ )
                {
                    // Memory 재사용을 위하여 현재 위치 기록
                    IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
                    sStage = 1;

                    IDE_TEST( stfRelation::relate( sQmxMem,
                                                   (stdGeometryType*)sValue1,
                                                   (stdGeometryType*)sValue2,
                                                   sPattern[i],
                                                   &sIsTouches )
                              != IDE_SUCCESS );
            
                    // Memory 재사용을 위한 Memory 이동
                    sStage = 0;
                    IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
                    
                    if( sIsTouches == MTD_BOOLEAN_TRUE )
                    {
                        break;
                    }
                }
            }
            else
            {
                sIsTouches = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            sIsTouches = MTD_BOOLEAN_FALSE;
        }

        *(mtdBooleanType*) aStack[0].value = sIsTouches;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

// BUG-16478
/***********************************************************************
 * Description:
 * Pattern스트링의 내용이 맞는지 검증 
 * '*TF012' 로만 구성되어야 한다.
 **********************************************************************/
idBool stfRelation::isValidPatternContents( const SChar *aPattern )
{
    SInt          i;
    idBool        sIsValid;
    static  SChar sValidChars[ 7 ] = "*TF012";
    
    sIsValid = ID_TRUE;
    for( i=0; i<9; i++ )
    {
        if( idlOS::strchr( sValidChars, aPattern[ i ] ) == NULL )
        {
            sIsValid = ID_FALSE;
            break;
        }
    }

    return sIsValid;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 관계가 주어지는 DE-9I 매트릭스와 맞는지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfRelation::isRelate(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue1  = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sValue2  = (stdGeometryHeader *)aStack[2].value;
    mtdCharType*            sPattern = (mtdCharType *)aStack[3].value;
    extern mtdModule        mtdChar;
    idBool                  sIsFParam2D;
    idBool                  sIsSParam2D;
    SChar*                  sPatternValue;
    mtdBooleanType          sIsRelated;
    SInt                    i;

    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

    // BUG-16478
    sPatternValue = (SChar*)sPattern->value;
    if( (sPattern->length != 9) ||
        (isValidPatternContents( sPatternValue )==ID_FALSE) )
    {
        IDE_RAISE( err_invalid_pattern );
    }
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue1 )==ID_TRUE) ||
        (stdGeometry.isNull( NULL, sValue2 )==ID_TRUE) ||
        (mtdChar.isNull( NULL, sPattern )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else if( (stdUtils::isEmpty(sValue1) == ID_TRUE) ||
             (stdUtils::isEmpty(sValue2) == ID_TRUE) )
    {
        // BUG-16451
        sIsRelated    = MTD_BOOLEAN_TRUE;
        for( i = 0; i < 9; i++ )
        {
            if( sPatternValue[i] != '*')
            {
                sIsRelated = checkMatch( sPatternValue[i], 'F' );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
        *(mtdBooleanType*) aStack[0].value = sIsRelated;
   }
    else
    {
        // fix BUG-16481
        sIsFParam2D = stdUtils::is2DType(sValue1->mType);
        sIsSParam2D = stdUtils::is2DType(sValue2->mType);
        IDE_TEST_RAISE( sIsFParam2D != sIsSParam2D, err_incompatible_mType );

        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
        sStage = 1;

        IDE_TEST( stfRelation::relate( sQmxMem,
                                       (stdGeometryType*)sValue1,
                                       (stdGeometryType*)sValue2,
                                       (SChar*)sPattern->value,
                                       &sIsRelated )
                  != IDE_SUCCESS );
            
        *(mtdBooleanType*) aStack[0].value = sIsRelated;
        
        // Memory 재사용을 위한 Memory 이동
        sStage = 0;
        IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_incompatible_mType);
    {
        // To fix BUG-15300
        SChar   sType1[32];
        SChar   sType2[32];
        SChar   sBuffer[128];
        UShort  sLen1;
        UShort  sLen2;

        stdUtils::getTypeText( sValue1->mType, sType1, &sLen1 );
        stdUtils::getTypeText( sValue2->mType, sType2, &sLen2 );
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                "(\"%s\" : \"%s\")",
                sType1, sType2 );

        IDE_SET(ideSetErrorCode(stERR_ABORT_CONVERSION_MODULE_NOT_FOUND,
            sBuffer));
    }
    // BUG-16478
    IDE_EXCEPTION(err_invalid_pattern);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_RELATE_PATTERN));
    }
    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 타입에 맞는 DE-9I Equals 매트릭스 출력
 *
 * stdGeometryType*    aGeom1(In): 객체1
 * stdGeometryType*    aGeom2(In): 객체2
 * SChar*              aPattern(Out): 관계 매트릭스가 출력될 버퍼
 **********************************************************************/
IDE_RC stfRelation::matrixEquals(
                    stdGeometryType*    aGeom1,
                    stdGeometryType*    aGeom2,
                    SChar*              aPattern )
{
    SInt    sDim1, sDim2;

    switch(aGeom1->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 0 )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            break;
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 1 )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            break;
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 2 )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 0 )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            break;
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 1 )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            break;
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 2 )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim1 = stdUtils::getDimension(&aGeom1->header);
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            
            if( (sDim1 == 0)&&(sDim2 == 0) )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else if( (sDim1 == 1)&&(sDim2 == 1) )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else if( (sDim1 == 2)&&(sDim2 == 2) )
            {
                idlOS::strcpy( (SChar*)aPattern, "T*F**FFF*");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    default:
        IDE_RAISE( err_incompatible_mType );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_match);
    {
        idlOS::strcpy( (SChar*)aPattern, "");
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_incompatible_mType);
    {
        // To fix BUG-15300
        SChar   sType1[32];
        SChar   sType2[32];
        SChar   sBuffer[128];
        UShort  sLen1;
        UShort  sLen2;

        stdUtils::getTypeText( aGeom1->header.mType, sType1, &sLen1 );
        stdUtils::getTypeText( aGeom2->header.mType, sType2, &sLen2 );
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                "(\"%s\" : \"%s\")",
                sType1, sType2 );

        IDE_SET(ideSetErrorCode(stERR_ABORT_CONVERSION_MODULE_NOT_FOUND,
            sBuffer));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 타입에 맞는 DE-9I Disjoint 매트릭스 출력
 *
 * stdGeometryType*    aGeom1(In): 객체1
 * stdGeometryType*    aGeom2(In): 객체2
 * SChar*              aPattern(Out): 관계 매트릭스가 출력될 버퍼
 **********************************************************************/
IDE_RC stfRelation::matrixDisjoint(
                    stdGeometryType*    aGeom1,
                    stdGeometryType*    aGeom2,
                    SChar*              aPattern )
{

    switch(aGeom1->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "F********");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "FF*******");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "F**F*****");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "FF*FF****");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "F**F*****");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "FF*FF****");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "F********");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "FF*******");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "F**F*****");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "FF*FF****");
            break;
        default                       :
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "F**F*****");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "FF*FF****");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "FF*FF****");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    default:
        IDE_RAISE( err_incompatible_mType );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_incompatible_mType);
    {
        // To fix BUG-15300
        SChar   sType1[32];
        SChar   sType2[32];
        SChar   sBuffer[128];
        UShort  sLen1;
        UShort  sLen2;

        stdUtils::getTypeText( aGeom1->header.mType, sType1, &sLen1 );
        stdUtils::getTypeText( aGeom2->header.mType, sType2, &sLen2 );
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                "(\"%s\" : \"%s\")",
                sType1, sType2 );

        IDE_SET(ideSetErrorCode(stERR_ABORT_CONVERSION_MODULE_NOT_FOUND,
            sBuffer));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 타입에 맞는 DE-9I Within 매트릭스 출력
 *
 * stdGeometryType*    aGeom1(In): 객체1
 * stdGeometryType*    aGeom2(In): 객체2
 * SChar*              aPattern(Out): 관계 매트릭스가 출력될 버퍼
 **********************************************************************/
IDE_RC stfRelation::matrixWithin(
                    stdGeometryType*    aGeom1,
                    stdGeometryType*    aGeom2,
                    SChar*              aPattern )
{
    switch(aGeom1->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**F***");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**F***");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**F***");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**F***");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**F***");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**F***");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern, "T*F**F***");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    default:
        IDE_RAISE( err_incompatible_mType );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_match);
    {
        idlOS::strcpy( (SChar*)aPattern, "");
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_incompatible_mType);
    {
        // To fix BUG-15300
        SChar   sType1[32];
        SChar   sType2[32];
        SChar   sBuffer[128];
        UShort  sLen1;
        UShort  sLen2;

        stdUtils::getTypeText( aGeom1->header.mType, sType1, &sLen1 );
        stdUtils::getTypeText( aGeom2->header.mType, sType2, &sLen2 );
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                "(\"%s\" : \"%s\")",
                sType1, sType2 );

        IDE_SET(ideSetErrorCode(stERR_ABORT_CONVERSION_MODULE_NOT_FOUND,
            sBuffer));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 타입에 맞는 DE-9I Crosses 매트릭스 출력
 *
 * stdGeometryType*    aGeom1(In): 객체1
 * stdGeometryType*    aGeom2(In): 객체2
 * SChar*              aPattern(Out): 관계 매트릭스가 출력될 버퍼
 **********************************************************************/
IDE_RC stfRelation::matrixCrosses(
                    stdGeometryType*    aGeom1,
                    stdGeometryType*    aGeom2,
                    SChar*              aPattern )
{
    SInt    sDim1, sDim2;
    switch(aGeom1->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern,"T*T******");
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:  // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( (sDim2 == 1) || (sDim2 == 2) )
            {
                idlOS::strcpy( (SChar*)aPattern,"T*T******");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern,"0********");
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern,"T*T******");
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:  // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 1 )
            {
                idlOS::strcpy( (SChar*)aPattern,"0********");
            }
            else if( sDim2 == 2 )
            {
                idlOS::strcpy( (SChar*)aPattern,"T*T******");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
        IDE_RAISE( err_not_match );
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
        sDim1 = stdUtils::getDimension(&aGeom1->header);
        sDim2 = stdUtils::getDimension(&aGeom2->header);
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:  // Fix BUG-15518
            if( ((sDim1 == 0)&&(sDim2 == 1)) ||
                ((sDim1 == 0)&&(sDim2 == 2)) ||
                ((sDim1 == 1)&&(sDim2 == 2)) )
            {
                idlOS::strcpy( (SChar*)aPattern,"T*T******");
            }
            else if( (sDim1 == 1)&&(sDim2 == 1) )
            {
                idlOS::strcpy( (SChar*)aPattern,"0********");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    default:
        IDE_RAISE( err_incompatible_mType );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_match);
    {
        idlOS::strcpy( (SChar*)aPattern, "");
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_incompatible_mType);
    {
        // To fix BUG-15300
        SChar   sType1[32];
        SChar   sType2[32];
        SChar   sBuffer[128];
        UShort  sLen1;
        UShort  sLen2;

        stdUtils::getTypeText( aGeom1->header.mType, sType1, &sLen1 );
        stdUtils::getTypeText( aGeom2->header.mType, sType2, &sLen2 );
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                "(\"%s\" : \"%s\")",
                sType1, sType2 );

        IDE_SET(ideSetErrorCode(stERR_ABORT_CONVERSION_MODULE_NOT_FOUND,
            sBuffer));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 타입에 맞는 DE-9I Overlaps 매트릭스 출력
 *
 * stdGeometryType*    aGeom1(In): 객체1
 * stdGeometryType*    aGeom2(In): 객체2
 * SChar*              aPattern(Out): 관계 매트릭스가 출력될 버퍼
 **********************************************************************/
IDE_RC stfRelation::matrixOverlaps(
                    stdGeometryType*    aGeom1,
                    stdGeometryType*    aGeom2,
                    SChar*              aPattern )
{
    SInt    sDim1, sDim2;
    switch(aGeom1->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern,"T*T***T**");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 0 )
            {
                idlOS::strcpy( (SChar*)aPattern,"T*T***T**");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern,"1*T***T**");
            break;
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 1 )
            {
                idlOS::strcpy( (SChar*)aPattern,"1*T***T**");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            idlOS::strcpy( (SChar*)aPattern,"T*T***T**");
            break;
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
            sDim2 = stdUtils::getDimension(&aGeom2->header);
            if( sDim2 == 2 )
            {
                idlOS::strcpy( (SChar*)aPattern,"T*T***T**");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE: // Fix BUG-15518
        sDim1 = stdUtils::getDimension(&aGeom1->header);
        sDim2 = stdUtils::getDimension(&aGeom2->header);
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:  // Fix BUG-15518
            if( ((sDim1 == 0)&&(sDim2 == 0)) ||
                ((sDim1 == 2)&&(sDim2 == 2)) )
            {
                idlOS::strcpy( (SChar*)aPattern,"T*T***T**");
            }
            else if( (sDim1 == 1)&&(sDim2 == 1) )
            {
                idlOS::strcpy( (SChar*)aPattern,"1*T***T**");
            }
            else
            {
                IDE_RAISE( err_not_match );
            }
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    default:
        IDE_RAISE( err_incompatible_mType );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_match);
    {
        idlOS::strcpy( (SChar*)aPattern, "");
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_incompatible_mType);
    {
        // To fix BUG-15300
        SChar   sType1[32];
        SChar   sType2[32];
        SChar   sBuffer[128];
        UShort  sLen1;
        UShort  sLen2;

        stdUtils::getTypeText( aGeom1->header.mType, sType1, &sLen1 );
        stdUtils::getTypeText( aGeom2->header.mType, sType2, &sLen2 );
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                "(\"%s\" : \"%s\")",
                sType1, sType2 );

        IDE_SET(ideSetErrorCode(stERR_ABORT_CONVERSION_MODULE_NOT_FOUND,
            sBuffer));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 두 Geometry 객체의 타입에 맞는 DE-9I Touches 매트릭스 출력
 *
 * stdGeometryType*    aGeom1(In): 객체1
 * stdGeometryType*    aGeom2(In): 객체2
 * SChar*              aPattern(Out): 관계 매트릭스가 출력될 버퍼
 * UInt&               aPatternCnt: 매트릭스 개수
 **********************************************************************/
IDE_RC stfRelation::matrixTouches(
                    stdGeometryType*    aGeom1,
                    stdGeometryType*    aGeom2,
                    SChar               aPattern[3][10],
                    UInt&               aPatternCnt )
{
    switch(aGeom1->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            IDE_RAISE( err_not_match );
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            aPatternCnt = 1;
            idlOS::strcpy( (SChar*)aPattern[0],"F0*******");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
        switch(aGeom2->header.mType)
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            aPatternCnt = 1;
            idlOS::strcpy( (SChar*)aPattern[0],"F**0*****");
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            aPatternCnt = 3;
            idlOS::strcpy( (SChar*)aPattern[0],"FT*******");
            idlOS::strcpy( (SChar*)aPattern[1],"F**T*****");
            idlOS::strcpy( (SChar*)aPattern[2],"F***T****");
            break;
        default:
            IDE_RAISE( err_incompatible_mType );
        }
        break;
    default:
        IDE_RAISE( err_incompatible_mType );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_match);
    {
        idlOS::strcpy( (SChar*)aPattern, "");
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_incompatible_mType);
    {
        // To fix BUG-15300
        SChar   sType1[32];
        SChar   sType2[32];
        SChar   sBuffer[128];
        UShort  sLen1;
        UShort  sLen2;

        stdUtils::getTypeText( aGeom1->header.mType, sType1, &sLen1 );
        stdUtils::getTypeText( aGeom2->header.mType, sType2, &sLen2 );
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                "(\"%s\" : \"%s\")",
                sType1, sType2 );

        IDE_SET(ideSetErrorCode(stERR_ABORT_CONVERSION_MODULE_NOT_FOUND,
            sBuffer));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Calculate Relation *************************************************/

/***********************************************************************
 * Description:
 * 두 Geometry 객체가 aPattern 매트릭스와 동일한 관계를 갖는지 판별
 * 관계가 동일하면 MTD_BOOLEAN_TRUE 아니면 MTD_BOOLEAN_FALSE 리턴
 *
 * const stdGeometryType* aObj1(In): 객체1
 * const stdGeometryType* aObj2(In): 객체2
 * SChar*                 aPattern(In): 패턴 매트릭스
 **********************************************************************/
IDE_RC stfRelation::relate( iduMemory*             aQmxMem,
                            const stdGeometryType* aObj1,
                            const stdGeometryType* aObj2,
                            SChar*                 aPattern,
                            mtdBooleanType*        aReturn )
{
    SChar          sResult;
    SInt           i;
    mtdBooleanType sIsRelated = MTD_BOOLEAN_TRUE;

    if( (aObj1 == NULL) || (aObj2 == NULL) )
    {
        sIsRelated = MTD_BOOLEAN_FALSE;
        IDE_RAISE( normal_exit );
    }
    
    if( ( stdUtils::getGroup((stdGeometryHeader*)aObj1) == STD_POLYGON_2D_GROUP ) &&
        ( stdUtils::getGroup((stdGeometryHeader*)aObj2) == STD_POLYGON_2D_GROUP ) )
    {
        IDE_TEST( relateAreaArea( aQmxMem, aObj1, aObj2, aPattern, &sIsRelated ) 
                    != IDE_SUCCESS );
        IDE_RAISE( normal_exit );
    }


    switch(aObj1->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
        for( i = 0; i < 9; i++ )
        {
            if( aPattern[i] != '*')
            {
                sResult = pointTogeometry( i, aObj1, aObj2 );
                sIsRelated = checkMatch( aPattern[i], sResult );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
        for( i = 0; i < 9; i++ )
        {
            if( aPattern[i] != '*')
            {
                sResult = linestringTogeometry( i, aObj1, aObj2 );
                sIsRelated = checkMatch( aPattern[i], sResult );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
        break;
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
        for( i = 0; i < 9; i++ )
        {
            if( aPattern[i] != '*')
            {
                IDE_TEST( polygonTogeometry( aQmxMem, i, aObj1, aObj2, &sResult )
                          != IDE_SUCCESS );

                sIsRelated = checkMatch( aPattern[i], sResult );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
        break;
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        for( i = 0; i < 9; i++ )
        {
            if( aPattern[i] != '*')
            {
                sResult = multipointTogeometry( i, aObj1, aObj2 );

                sIsRelated = checkMatch( aPattern[i], sResult );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
        break;
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
        for( i = 0; i < 9; i++ )
        {
            if( aPattern[i] != '*')
            {
                sResult = multilinestringTogeometry( i, aObj1, aObj2 );

                sIsRelated = checkMatch( aPattern[i], sResult );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
        break;
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
        for( i = 0; i < 9; i++ )
        {
            if( aPattern[i] != '*')
            {
                IDE_TEST( multipolygonTogeometry( aQmxMem, i, aObj1, aObj2, &sResult )
                          != IDE_SUCCESS );

                sIsRelated = checkMatch( aPattern[i], sResult );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
        break;
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
        for( i = 0; i < 9; i++ )
        {
            if( aPattern[i] != '*')
            {
                IDE_TEST( collectionTogeometry( aQmxMem, i, aObj1, aObj2, &sResult )
                          != IDE_SUCCESS );
                sIsRelated = checkMatch( aPattern[i], sResult );
                if( sIsRelated == MTD_BOOLEAN_FALSE )
                {
                    break;
                }
            }
        }
    default:
        break;
    }

    IDE_EXCEPTION_CONT( normal_exit );

    *aReturn = sIsRelated;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Calculate Relation *************************************************/

/***********************************************************************
 * Description:
 * 포인트 객체와 다른 객체의 매트릭스 인덱스에 해당하는 관계를 연산
 * 겹치는 공간의 차원을 리턴한다.
 * '0' 점
 * '1' 선
 * '2' 면
 * 'F' 존재하지 않는다.
 *
 * SInt                  aIndex(In): 매트릭스 인덱스 번호
 * const stdGeometryType* aObj1(In): 객체
 * const stdGeometryType* aObj2(In): 비교될 객체
 **********************************************************************/
SChar stfRelation::pointTogeometry( SInt                   aIndex,
                                    const stdGeometryType* aObj1,
                                    const stdGeometryType* aObj2 )
{
    switch(aObj2->header.mType)
    {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTospi(&aObj1->point2D.mPoint, &aObj2->point2D.mPoint);
                case 2: return spiTospe(&aObj1->point2D.mPoint, &aObj2->point2D.mPoint);
                case 6: return spiTospe(&aObj2->point2D.mPoint, &aObj1->point2D.mPoint);
                case 8: return '2';
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTosli(&aObj1->point2D.mPoint, &aObj2->linestring2D);
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return spiToslb(&aObj1->point2D.mPoint, &aObj2->linestring2D);
                    }
                case 2: return spiTosle(&aObj1->point2D.mPoint, &aObj2->linestring2D);
                case 6: return '1';
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return '0';
                    }
                case 8: return '2';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTosai(&aObj1->point2D.mPoint, &aObj2->polygon2D);
                case 1: return spiTosab(&aObj1->point2D.mPoint, &aObj2->polygon2D);
                case 2: return spiTosae(&aObj1->point2D.mPoint, &aObj2->polygon2D);
                case 6: return '2';
                case 7: return '1';
                case 8: return '2';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTompi(&aObj1->point2D.mPoint, &aObj2->mpoint2D);
                case 2: return spiTompe(&aObj1->point2D.mPoint, &aObj2->mpoint2D);
                case 6: return speTompi(&aObj1->point2D.mPoint, &aObj2->mpoint2D);
                case 8: return '2';
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTomli(&aObj1->point2D.mPoint, &aObj2->mlinestring2D);
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return spiTomlb(&aObj1->point2D.mPoint, &aObj2->mlinestring2D);
                    }
                case 2: return spiTomle(&aObj1->point2D.mPoint, &aObj2->mlinestring2D);
                case 6: return '1';
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {        
                        return '0';
                    }
                case 8: return '2';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTomai(&aObj1->point2D.mPoint, &aObj2->mpolygon2D);
                case 1: return spiTomab(&aObj1->point2D.mPoint, &aObj2->mpolygon2D);
                case 2: return spiTomae(&aObj1->point2D.mPoint, &aObj2->mpolygon2D);
                case 6: return '2';
                case 7: return '1';
                case 8: return '2';
            }
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTogci(&aObj1->point2D.mPoint, &aObj2->collection2D);
                case 1: return spiTogcb(&aObj1->point2D.mPoint, &aObj2->collection2D);
                case 2: return spiTogce(&aObj1->point2D.mPoint, &aObj2->collection2D);
                case 6: return speTogci(&aObj1->point2D.mPoint, &aObj2->collection2D);
                case 7: return speTogcb(&aObj1->point2D.mPoint, &aObj2->collection2D);
                case 8: return '2';
            }
            break;
    }
    return 'F';
}

/***********************************************************************
 * Description:
 * 라인 객체와 다른 객체의 매트릭스 인덱스에 해당하는 관계를 연산
 * 겹치는 공간의 차원을 리턴한다.
 * '0' 점
 * '1' 선
 * '2' 면
 * 'F' 존재하지 않는다.
 *
 * SInt                  aIndex(In): 매트릭스 인덱스 번호
 * const stdGeometryType* aObj1(In): 객체
 * const stdGeometryType* aObj2(In): 비교될 객체
 **********************************************************************/
SChar stfRelation::linestringTogeometry( SInt                   aIndex,
                                         const stdGeometryType* aObj1,
                                         const stdGeometryType* aObj2 )
{
    switch(aObj2->header.mType)
    {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTosli(&aObj2->point2D.mPoint, &aObj1->linestring2D);
                case 2: return '1';
                case 3: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return spiToslb(&aObj2->point2D.mPoint, &aObj1->linestring2D);
                    }
                case 5: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return '0';
                    }
                case 6: return spiTosle(&aObj2->point2D.mPoint, &aObj1->linestring2D);
                case 8: return '2';
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return sliTosli(&aObj1->linestring2D, &aObj2->linestring2D);
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return sliToslb(&aObj1->linestring2D, &aObj2->linestring2D);
                    }
                case 2: return sliTosle(&aObj1->linestring2D, &aObj2->linestring2D);
                case 3: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return sliToslb(&aObj2->linestring2D, &aObj1->linestring2D);
                    }
                case 4: 
                    // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) ||
                        stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbToslb(&aObj1->linestring2D, &aObj2->linestring2D);
                    }
                case 5: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTosle(&aObj1->linestring2D, &aObj2->linestring2D);
                    }
                case 6: return sliTosle(&aObj2->linestring2D, &aObj1->linestring2D);
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTosle(&aObj2->linestring2D, &aObj1->linestring2D);
                    }
                case 8: return '2';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return sliTosai(&aObj1->linestring2D, &aObj2->polygon2D);
                case 1: return sliTosab(&aObj1->linestring2D, &aObj2->polygon2D);
                case 2: return sliTosae(&aObj1->linestring2D, &aObj2->polygon2D);
                case 3: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTosai(&aObj1->linestring2D, &aObj2->polygon2D);
                    }
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTosab(&aObj1->linestring2D, &aObj2->polygon2D);
                    }
                case 5: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTosae(&aObj1->linestring2D, &aObj2->polygon2D);
                    }
                case 6: return '2';
                case 7: return sleTosab(&aObj1->linestring2D, &aObj2->polygon2D);
                case 8: return '2';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return sliTompi(&aObj1->linestring2D, &aObj2->mpoint2D);
                case 2: return '1';
                case 3: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTompi(&aObj1->linestring2D, &aObj2->mpoint2D);
                    }
                case 5: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTompe(&aObj1->linestring2D, &aObj2->mpoint2D);
                    }
                case 6: return sleTompi(&aObj1->linestring2D, &aObj2->mpoint2D);
                case 8: return '2';
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return sliTomli(&aObj1->linestring2D, &aObj2->mlinestring2D);
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return sliTomlb(&aObj1->linestring2D, &aObj2->mlinestring2D);
                    }
                case 2: return sliTomle(&aObj1->linestring2D, &aObj2->mlinestring2D);
                case 3: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTomli(&aObj1->linestring2D, &aObj2->mlinestring2D);
                    }
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) ||
                        stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTomlb(&aObj1->linestring2D, &aObj2->mlinestring2D);
                    }
                case 5: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTomle(&aObj1->linestring2D, &aObj2->mlinestring2D);
                    }
                case 6: return sleTomli(&aObj1->linestring2D, &aObj2->mlinestring2D);
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return sleTomlb(&aObj1->linestring2D, &aObj2->mlinestring2D);
                    }
                case 8: return '2';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return sliTomai(&aObj1->linestring2D, &aObj2->mpolygon2D);
                case 1: return sliTomab(&aObj1->linestring2D, &aObj2->mpolygon2D);
                case 2: return sliTomae(&aObj1->linestring2D, &aObj2->mpolygon2D);
                case 3: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTomai(&aObj1->linestring2D, &aObj2->mpolygon2D);
                    }
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTomab(&aObj1->linestring2D, &aObj2->mpolygon2D);
                    }
                case 5: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTomae(&aObj1->linestring2D, &aObj2->mpolygon2D);
                    }
                case 6: return '2';
                case 7: return sleTomab(&aObj1->linestring2D, &aObj2->mpolygon2D);
                case 8: return '2';
            }
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return sliTogci(&aObj1->linestring2D, &aObj2->collection2D);
                case 1: return sliTogcb(&aObj1->linestring2D, &aObj2->collection2D);
                case 2: return sliTogce(&aObj1->linestring2D, &aObj2->collection2D);
                case 3: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTogci(&aObj1->linestring2D, &aObj2->collection2D);
                    }
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTogcb(&aObj1->linestring2D, &aObj2->collection2D);
                    }
                case 5: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTogce(&aObj1->linestring2D, &aObj2->collection2D);
                    }
                case 6: return sleTogci(&aObj1->linestring2D, &aObj2->collection2D);
                case 7: return sleTogcb(&aObj1->linestring2D, &aObj2->collection2D);
                case 8: return '2';
            }
            break;
    }
    return 'F';
}

/***********************************************************************
 * Description:
 * 폴리곤 객체와 다른 객체의 매트릭스 인덱스에 해당하는 관계를 연산
 * 겹치는 공간의 차원을 리턴한다.
 * '0' 점
 * '1' 선
 * '2' 면
 * 'F' 존재하지 않는다.
 *
 * SInt                  aIndex(In): 매트릭스 인덱스 번호
 * const stdGeometryType* aObj1(In): 객체
 * const stdGeometryType* aObj2(In): 비교될 객체
 **********************************************************************/
IDE_RC stfRelation::polygonTogeometry( iduMemory*             aQmxMem,
                                       SInt                   aIndex,
                                       const stdGeometryType* aObj1,
                                       const stdGeometryType* aObj2,
                                       SChar*                 aResult )
{
    SChar sResult;
    
    switch(aObj2->header.mType)
    {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = spiTosai(&aObj2->point2D.mPoint, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = spiTosab(&aObj2->point2D.mPoint, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = '1';
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = spiTosae(&aObj2->point2D.mPoint, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = sliTosai(&aObj2->linestring2D, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTosai(&aObj2->linestring2D, &aObj1->polygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = sliTosab(&aObj2->linestring2D, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTosab(&aObj2->linestring2D, &aObj1->polygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 5:
                    sResult = sleTosab(&aObj2->linestring2D, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = sliTosae(&aObj2->linestring2D, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTosae(&aObj2->linestring2D, &aObj1->polygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( saiTosai(aQmxMem, &aObj1->polygon2D, &aObj2->polygon2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = saiTosab(&aObj1->polygon2D, &aObj2->polygon2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( saiTosae(aQmxMem, &aObj1->polygon2D, &aObj2->polygon2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = saiTosab(&aObj2->polygon2D, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = sabTosab(&aObj1->polygon2D, &aObj2->polygon2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = sabTosae(&aObj1->polygon2D, &aObj2->polygon2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( saiTosae(aQmxMem, &aObj2->polygon2D, &aObj1->polygon2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = sabTosae(&aObj2->polygon2D, &aObj1->polygon2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = saiTompi(&aObj1->polygon2D, &aObj2->mpoint2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = sabTompi(&aObj1->polygon2D, &aObj2->mpoint2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = '1';
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = saeTompi(&aObj1->polygon2D, &aObj2->mpoint2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = saiTomli(&aObj1->polygon2D, &aObj2->mlinestring2D);
                    IDE_RAISE( normal_exit );
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = saiTomlb(&aObj1->polygon2D, &aObj2->mlinestring2D);
                        IDE_RAISE( normal_exit );
                    }
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = sabTomli(&aObj1->polygon2D, &aObj2->mlinestring2D);
                    IDE_RAISE( normal_exit );
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = sabTomlb(&aObj1->polygon2D, &aObj2->mlinestring2D);
                        IDE_RAISE( normal_exit );
                    }
                case 5:
                    sResult = sabTomle(&aObj1->polygon2D, &aObj2->mlinestring2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = saeTomli(&aObj1->polygon2D, &aObj2->mlinestring2D);
                    IDE_RAISE( normal_exit );
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = saeTomlb(&aObj1->polygon2D, &aObj2->mlinestring2D);
                        IDE_RAISE( normal_exit );
                    }
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( saiTomai(aQmxMem, &aObj1->polygon2D, &aObj2->mpolygon2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = saiTomab(&aObj1->polygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( saiTomae(aQmxMem, &aObj1->polygon2D, &aObj2->mpolygon2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = sabTomai(&aObj1->polygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = sabTomab(&aObj1->polygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = sabTomae(&aObj1->polygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( saeTomai(aQmxMem, &aObj1->polygon2D, &aObj2->mpolygon2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = saeTomab(&aObj1->polygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( saiTogci(aQmxMem, &aObj1->polygon2D, &aObj2->collection2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = saiTogcb(&aObj1->polygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( saiTogce(aQmxMem, &aObj1->polygon2D, &aObj2->collection2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = sabTogci(&aObj1->polygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = sabTogcb(&aObj1->polygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = sabTogce(&aObj1->polygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( saeTogci(aQmxMem, &aObj1->polygon2D, &aObj2->collection2D, &sResult )
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = saeTogcb(&aObj1->polygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
    }

    sResult = 'F';

    IDE_EXCEPTION_CONT( normal_exit );

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * 멀티포인트 객체와 다른 객체의 매트릭스 인덱스에 해당하는 관계를 연산
 * 겹치는 공간의 차원을 리턴한다.
 * '0' 점
 * '1' 선
 * '2' 면
 * 'F' 존재하지 않는다.
 *
 * SInt                  aIndex(In): 매트릭스 인덱스 번호
 * const stdGeometryType* aObj1(In): 객체
 * const stdGeometryType* aObj2(In): 비교될 객체
 **********************************************************************/
SChar stfRelation::multipointTogeometry( SInt                   aIndex,
                                         const stdGeometryType* aObj1,
                                         const stdGeometryType* aObj2 )
{
    switch(aObj2->header.mType)
    {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return spiTompi(&aObj2->point2D.mPoint, &aObj1->mpoint2D);
                case 2: return speTompi(&aObj2->point2D.mPoint, &aObj1->mpoint2D);
                case 6: return spiTompe(&aObj2->point2D.mPoint, &aObj1->mpoint2D);
                case 8: return '2';
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return sliTompi(&aObj2->linestring2D, &aObj1->mpoint2D);
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTompi(&aObj2->linestring2D, &aObj1->mpoint2D);
                    }
                case 2: return sleTompi(&aObj2->linestring2D, &aObj1->mpoint2D);
                case 6: return '1';
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return slbTompe(&aObj2->linestring2D, &aObj1->mpoint2D);
                    }
                case 8: return '2';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return saiTompi(&aObj2->polygon2D, &aObj1->mpoint2D);
                case 1: return sabTompi(&aObj2->polygon2D, &aObj1->mpoint2D);
                case 2: return saeTompi(&aObj2->polygon2D, &aObj1->mpoint2D);
                case 6: return '2';
                case 7: return '1';
                case 8: return '2';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return mpiTompi(&aObj1->mpoint2D, &aObj2->mpoint2D);
                case 2: return mpiTompe(&aObj1->mpoint2D, &aObj2->mpoint2D);
                case 6: return mpiTompe(&aObj2->mpoint2D, &aObj1->mpoint2D);
                case 8: return '2';
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return mpiTomli(&aObj1->mpoint2D, &aObj2->mlinestring2D);
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return mpiTomlb(&aObj1->mpoint2D, &aObj2->mlinestring2D);
                    }
                case 2: return mpiTomle(&aObj1->mpoint2D, &aObj2->mlinestring2D);
                case 6: return '1';
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        return mpeTomlb(&aObj1->mpoint2D, &aObj2->mlinestring2D);
                    }
                case 8: return '2';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return mpiTomai(&aObj1->mpoint2D, &aObj2->mpolygon2D);
                case 1: return mpiTomab(&aObj1->mpoint2D, &aObj2->mpolygon2D);
                case 2: return mpiTomae(&aObj1->mpoint2D, &aObj2->mpolygon2D);
                case 6: return '2';
                case 7: return '1';
                case 8: return '2';
            }
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0: return mpiTogci(&aObj1->mpoint2D, &aObj2->collection2D);
                case 1: return mpiTogcb(&aObj1->mpoint2D, &aObj2->collection2D);
                case 2: return mpiTogce(&aObj1->mpoint2D, &aObj2->collection2D);
                case 6: return mpeTogci(&aObj1->mpoint2D, &aObj2->collection2D);
                case 7: return mpeTogcb(&aObj1->mpoint2D, &aObj2->collection2D);
                case 8: return '2';
            }
            break;
    }
    return 'F';
}

/***********************************************************************
 * Description:
 * 멀티 라인 객체와 다른 객체의 매트릭스 인덱스에 해당하는 관계를 연산
 * 겹치는 공간의 차원을 리턴한다.
 * '0' 점
 * '1' 선
 * '2' 면
 * 'F' 존재하지 않는다.
 *
 * SInt                  aIndex(In): 매트릭스 인덱스 번호
 * const stdGeometryType* aObj1(In): 객체
 * const stdGeometryType* aObj2(In): 비교될 객체
 **********************************************************************/
SChar stfRelation::multilinestringTogeometry( SInt                   aIndex,
                                              const stdGeometryType* aObj1,
                                              const stdGeometryType* aObj2 )
{
    switch(aObj2->header.mType)
    {
    case STD_POINT_2D_TYPE:
    case STD_POINT_2D_EXT_TYPE:
        switch(aIndex)
        {
        case 0: return spiTomli(&aObj2->point2D.mPoint, &aObj1->mlinestring2D);
        case 2: return '1';
        case 3: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return spiTomlb(&aObj2->point2D.mPoint, &aObj1->mlinestring2D);
            }
        case 5: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return '0';
            }
        case 6: return spiTomle(&aObj2->point2D.mPoint, &aObj1->mlinestring2D);
        case 8: return '2';
        }
        break;
    case STD_LINESTRING_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
        switch(aIndex)
        {
        case 0: return sliTomli(&aObj2->linestring2D, &aObj1->mlinestring2D);
        case 1: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
            {
                break;
            }
            else
            {
                return slbTomli(&aObj2->linestring2D, &aObj1->mlinestring2D);
            }
        case 2: return sleTomli(&aObj2->linestring2D, &aObj1->mlinestring2D);
        case 3: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return sliTomlb(&aObj2->linestring2D, &aObj1->mlinestring2D);
            }
        case 4: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) ||
                stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
            {
                break;
            }
            else
            {
                return slbTomlb(&aObj2->linestring2D, &aObj1->mlinestring2D);
            }
        case 5: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return sleTomlb(&aObj2->linestring2D, &aObj1->mlinestring2D);
            }
        case 6: return sliTomle(&aObj2->linestring2D, &aObj1->mlinestring2D);
        case 7: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
            {
                break;
            }
            else
            {
                return slbTomle(&aObj2->linestring2D, &aObj1->mlinestring2D);
            }
        case 8: return '2';
        }
        break;
    case STD_POLYGON_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
        switch(aIndex)
        {
        case 0: return saiTomli(&aObj2->polygon2D, &aObj1->mlinestring2D);
        case 1: return sabTomli(&aObj2->polygon2D, &aObj1->mlinestring2D);
        case 2: return saeTomli(&aObj2->polygon2D, &aObj1->mlinestring2D);
        case 3: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return saiTomlb(&aObj2->polygon2D, &aObj1->mlinestring2D);
            }
        case 4: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return sabTomlb(&aObj2->polygon2D, &aObj1->mlinestring2D);
            }
        case 5: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return saeTomlb(&aObj2->polygon2D, &aObj1->mlinestring2D);
            }
        case 6: return '2';
        case 7: return sabTomle(&aObj2->polygon2D, &aObj1->mlinestring2D);
        case 8: return '2';
        }
        break;
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
        switch(aIndex)
        {
        case 0: return mpiTomli(&aObj2->mpoint2D, &aObj1->mlinestring2D);
        case 2: return '1';
        case 3: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mpiTomlb(&aObj2->mpoint2D, &aObj1->mlinestring2D);
            }
        case 5: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mpeTomlb(&aObj2->mpoint2D, &aObj1->mlinestring2D);
            }
        case 6: return mpiTomle(&aObj2->mpoint2D, &aObj1->mlinestring2D);
        case 8: return '2';
        }
        break;
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
        switch(aIndex)
        {
        case 0: return mliTomli(&aObj1->mlinestring2D, &aObj2->mlinestring2D);
        case 1: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
            {
                break;
            }
            else
            {
                return mliTomlb(&aObj1->mlinestring2D, &aObj2->mlinestring2D);
            }
        case 2: return mliTomle(&aObj1->mlinestring2D, &aObj2->mlinestring2D);
        case 3: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mliTomlb(&aObj2->mlinestring2D, &aObj1->mlinestring2D);
            }
        case 4: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) ||
                stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
            {
                break;
            }
            else
            {
                return mlbTomlb(&aObj1->mlinestring2D, &aObj2->mlinestring2D);
            }
        case 5: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mlbTomle(&aObj1->mlinestring2D, &aObj2->mlinestring2D);
            }
        case 6: return mliTomle(&aObj2->mlinestring2D, &aObj1->mlinestring2D);
        case 7: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
            {
                break;
            }
            else
            {
                return mlbTomle(&aObj2->mlinestring2D, &aObj1->mlinestring2D);
            }
        case 8: return '2';
        }
        break;
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
        switch(aIndex)
        {
        case 0: return mliTomai(&aObj1->mlinestring2D, &aObj2->mpolygon2D);
        case 1: return mliTomab(&aObj1->mlinestring2D, &aObj2->mpolygon2D);
        case 2: return mliTomae(&aObj1->mlinestring2D, &aObj2->mpolygon2D);
        case 3: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mlbTomai(&aObj1->mlinestring2D, &aObj2->mpolygon2D);
            }
        case 4: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mlbTomab(&aObj1->mlinestring2D, &aObj2->mpolygon2D);
            }
        case 5: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mlbTomae(&aObj1->mlinestring2D, &aObj2->mpolygon2D);
            }
        case 6: return '2';
        case 7: return mleTomab(&aObj1->mlinestring2D, &aObj2->mpolygon2D);
        case 8: return '2';
        }
        break;
    case STD_GEOCOLLECTION_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
        switch(aIndex)
        {
        case 0: return mliTogci(&aObj1->mlinestring2D, &aObj2->collection2D);
        case 1: return mliTogcb(&aObj1->mlinestring2D, &aObj2->collection2D);
        case 2: return mliTogce(&aObj1->mlinestring2D, &aObj2->collection2D);
        case 3: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mlbTogci(&aObj1->mlinestring2D, &aObj2->collection2D);
            }
        case 4: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mlbTogcb(&aObj1->mlinestring2D, &aObj2->collection2D);
            }
        case 5: // Fix BUG-15923
            if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj1 ) )
            {
                break;
            }
            else
            {
                return mlbTogce(&aObj1->mlinestring2D, &aObj2->collection2D);
            }
        case 6: return mleTogci(&aObj1->mlinestring2D, &aObj2->collection2D);
        case 7: return mleTogcb(&aObj1->mlinestring2D, &aObj2->collection2D);
        case 8: return '2';
        }
        break;
    }

    return 'F';
}

/***********************************************************************
 * Description:
 * 멀티폴리곤 객체와 다른 객체의 매트릭스 인덱스에 해당하는 관계를 연산
 * 겹치는 공간의 차원을 리턴한다.
 * '0' 점
 * '1' 선
 * '2' 면
 * 'F' 존재하지 않는다.
 *
 * SInt                  aIndex(In): 매트릭스 인덱스 번호
 * const stdGeometryType* aObj1(In): 객체
 * const stdGeometryType* aObj2(In): 비교될 객체
 **********************************************************************/
IDE_RC stfRelation::multipolygonTogeometry( iduMemory*             aQmxMem,
                                            SInt                   aIndex,
                                            const stdGeometryType* aObj1,
                                            const stdGeometryType* aObj2,
                                            SChar*                 aResult )
{
    SChar sResult;
    
    switch(aObj2->header.mType)
    {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = spiTomai(&aObj2->point2D.mPoint, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = spiTomab(&aObj2->point2D.mPoint, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = '1';
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = spiTomae(&aObj2->point2D.mPoint, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = sliTomai(&aObj2->linestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTomai(&aObj2->linestring2D, &aObj1->mpolygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = sliTomab(&aObj2->linestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTomab(&aObj2->linestring2D, &aObj1->mpolygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 5:
                    sResult = sleTomab(&aObj2->linestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = sliTomae(&aObj2->linestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTomae(&aObj2->linestring2D, &aObj1->mpolygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( saiTomai(aQmxMem, &aObj2->polygon2D, &aObj1->mpolygon2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = sabTomai(&aObj2->polygon2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( saeTomai(aQmxMem, &aObj2->polygon2D, &aObj1->mpolygon2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = saiTomab(&aObj2->polygon2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = sabTomab(&aObj2->polygon2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = saeTomab(&aObj2->polygon2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( saiTomae(aQmxMem, &aObj2->polygon2D, &aObj1->mpolygon2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = sabTomae(&aObj2->polygon2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = mpiTomai(&aObj2->mpoint2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = mpiTomab(&aObj2->mpoint2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = '1';
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = mpiTomae(&aObj2->mpoint2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = mliTomai(&aObj2->mlinestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = mlbTomai(&aObj2->mlinestring2D, &aObj1->mpolygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 2:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = mliTomab(&aObj2->mlinestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = mlbTomab(&aObj2->mlinestring2D, &aObj1->mpolygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 5:
                    sResult = mleTomab(&aObj2->mlinestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = mliTomae(&aObj2->mlinestring2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = mlbTomae(&aObj2->mlinestring2D, &aObj1->mpolygon2D);
                        IDE_RAISE( normal_exit );
                    }
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( maiTomai(aQmxMem, &aObj1->mpolygon2D, &aObj2->mpolygon2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = maiTomab(&aObj1->mpolygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( maiTomae(aQmxMem, &aObj1->mpolygon2D, &aObj2->mpolygon2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = maiTomab(&aObj2->mpolygon2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = mabTomab(&aObj1->mpolygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = mabTomae(&aObj1->mpolygon2D, &aObj2->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( maiTomae(aQmxMem, &aObj2->mpolygon2D, &aObj1->mpolygon2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = mabTomae(&aObj2->mpolygon2D, &aObj1->mpolygon2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( maiTogci(aQmxMem, &aObj1->mpolygon2D, &aObj2->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = maiTogcb(&aObj1->mpolygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( maiTogce(aQmxMem, &aObj1->mpolygon2D, &aObj2->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = mabTogci(&aObj1->mpolygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = mabTogcb(&aObj1->mpolygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = mabTogce(&aObj1->mpolygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( maeTogci(aQmxMem, &aObj1->mpolygon2D, &aObj2->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = maeTogcb(&aObj1->mpolygon2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
    }

    sResult = 'F';
    
    IDE_EXCEPTION_CONT( normal_exit );

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * 콜렉션 객체와 다른 객체의 매트릭스 인덱스에 해당하는 관계를 연산
 * 겹치는 공간의 차원을 리턴한다.
 * '0' 점
 * '1' 선
 * '2' 면
 * 'F' 존재하지 않는다.
 *
 * SInt                  aIndex(In): 매트릭스 인덱스 번호
 * const stdGeometryType* aObj1(In): 객체
 * const stdGeometryType* aObj2(In): 비교될 객체
 **********************************************************************/
IDE_RC stfRelation::collectionTogeometry( iduMemory*             aQmxMem,
                                          SInt                   aIndex,
                                          const stdGeometryType* aObj1,
                                          const stdGeometryType* aObj2,
                                          SChar*                 aResult )
{
    SChar sResult;
    
    switch(aObj2->header.mType)
    {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = spiTogci(&aObj2->point2D.mPoint, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    sResult = speTogci(&aObj2->point2D.mPoint, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = spiTogcb(&aObj2->point2D.mPoint, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = speTogcb(&aObj2->point2D.mPoint, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = spiTogce(&aObj2->point2D.mPoint, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = sliTogci(&aObj2->linestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTogci(&aObj2->linestring2D, &aObj1->collection2D);
                        IDE_RAISE( normal_exit );
                    }
                case 2:
                    sResult = sleTogci(&aObj2->linestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = sliTogcb(&aObj2->linestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTogcb(&aObj2->linestring2D, &aObj1->collection2D);
                        IDE_RAISE( normal_exit );
                    }
                case 5:
                    sResult = sleTogcb(&aObj2->linestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = sliTogce(&aObj2->linestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = slbTogce(&aObj2->linestring2D, &aObj1->collection2D);
                        IDE_RAISE( normal_exit );
                    }
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( saiTogci(aQmxMem, &aObj2->polygon2D, &aObj1->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = sabTogci(&aObj2->polygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( saeTogci(aQmxMem, &aObj2->polygon2D, &aObj1->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = saiTogcb(&aObj2->polygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = sabTogcb(&aObj2->polygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = saeTogcb(&aObj2->polygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( saiTogce(aQmxMem, &aObj2->polygon2D, &aObj1->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = sabTogce(&aObj2->polygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = mpiTogci(&aObj2->mpoint2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    sResult = mpeTogci(&aObj2->mpoint2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = mpiTogcb(&aObj2->mpoint2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = mpeTogcb(&aObj2->mpoint2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = mpiTogce(&aObj2->mpoint2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    sResult = mliTogci(&aObj2->mlinestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 1: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = mlbTogci(&aObj2->mlinestring2D, &aObj1->collection2D);
                        IDE_RAISE( normal_exit );
                    }
                case 2:
                    sResult = mleTogci(&aObj2->mlinestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = mliTogcb(&aObj2->mlinestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 4: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = mlbTogcb(&aObj2->mlinestring2D, &aObj1->collection2D);
                        IDE_RAISE( normal_exit );
                    }
                case 5:
                    sResult = mleTogcb(&aObj2->mlinestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    sResult = mliTogce(&aObj2->mlinestring2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 7: // Fix BUG-15923
                    if( stdUtils::isClosed2D( (stdGeometryHeader*)aObj2 ) )
                    {
                        break;
                    }
                    else
                    {
                        sResult = mlbTogce(&aObj2->mlinestring2D, &aObj1->collection2D);
                        IDE_RAISE( normal_exit );
                    }
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( maiTogci(aQmxMem, &aObj2->mpolygon2D, &aObj1->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = mabTogci(&aObj2->mpolygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( maeTogci(aQmxMem, &aObj2->mpolygon2D, &aObj1->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = maiTogcb(&aObj2->mpolygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = mabTogcb(&aObj2->mpolygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = maeTogcb(&aObj2->mpolygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( maiTogce(aQmxMem, &aObj2->mpolygon2D, &aObj1->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = mabTogce(&aObj2->mpolygon2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
        case STD_GEOCOLLECTION_2D_TYPE:
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            switch(aIndex)
            {
                case 0:
                    IDE_TEST( gciTogci(aQmxMem, &aObj1->collection2D, &aObj2->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 1:
                    sResult = gciTogcb(&aObj1->collection2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 2:
                    IDE_TEST( gciTogce(aQmxMem, &aObj1->collection2D, &aObj2->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 3:
                    sResult = gciTogcb(&aObj2->collection2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 4:
                    sResult = gcbTogcb(&aObj1->collection2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 5:
                    sResult = gcbTogce(&aObj1->collection2D, &aObj2->collection2D);
                    IDE_RAISE( normal_exit );
                case 6:
                    IDE_TEST( gciTogce(aQmxMem, &aObj2->collection2D, &aObj1->collection2D, &sResult)
                              != IDE_SUCCESS );
                    IDE_RAISE( normal_exit );
                case 7:
                    sResult = gcbTogce(&aObj2->collection2D, &aObj1->collection2D);
                    IDE_RAISE( normal_exit );
                case 8:
                    sResult = '2';
                    IDE_RAISE( normal_exit );
            }
            break;
    }
    
    sResult = 'F';
    
    IDE_EXCEPTION_CONT( normal_exit );

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * 두 매트릭스의 관계가 일치하는지 판별
 * '0' '1' '2'의 차원 값은 'T'값 과도 일치한다.
 * 관계가 일치하면 MTD_BOOLEAN_TRUE 아니면 MTD_BOOLEAN_FALSE 리턴
 *
 * SChar aPM(In): 매트릭스1
 * SChar aResult(In): 매트릭스2
 **********************************************************************/
mtdBooleanType stfRelation::checkMatch( SChar aPM, SChar aResult )
{
    // aResult는 0,1,2,F만 존재한다.
    switch( aPM )
    {
        case '0':
            switch( aResult )
            {
                case '0':
                case 'T': return MTD_BOOLEAN_TRUE;
            }
            break;
        case '1':
            switch( aResult )
            {
                case '1':
                case 'T': return MTD_BOOLEAN_TRUE;
            }
            break;
        case '2':
            switch( aResult )
            {
                case '2':
                case 'T': return MTD_BOOLEAN_TRUE;
            }
            break;
        case 'T':
            switch( aResult )
            {
                case '0':
                case '1':
                case '2':
                case 'T': return MTD_BOOLEAN_TRUE;
            }
            break;
        case 'F':
            switch( aResult )
            {
                case 'F': return MTD_BOOLEAN_TRUE;
            }
            break;
    }
    return MTD_BOOLEAN_FALSE;
}

/***********************************************************************
 * Description:
 *
 *    선분(Line Segment)간의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *    다음과 같은 선분과 관계되는 점들의 개수를 고려해 DE-9IM 관계를 구한다.
 *       - 점이 선분과 만나는 개수
 *       - 점과 점이 만나는 개수
 *       - 만나는 점이 종단점인지의 개수
 *
 *    선분과 만나는 점의 개수에 따른 선의 관계는 다음과 같다.
 *
 *           S-----------S : Segment 1
 *           T-----------T : Segment 2
 *
 *    점 4개가 선분과 만나는 경우
 *
 *         ST=======ST
 *
 *         ; 선에서 만남 ('1')
 *
 *    점 3개가 선분과 만나는 경우
 *
 *         ST=======S-------T
 *
 *         ; 선에서 만남 ('1')
 *
 *    점 2개가 선분과 만나는 경우
 *
 *         S-----T=====T-----S
 *
 *         ; 점끼리 만나는 경우가 없음 ('1')
 *
 *         S-----T=====S----T
 *
 *         ; 점끼리 만나는 경우가 없음 ('1')
 *
 *         S
 *         |
 *         |
 *         ST-------T
 *
 *         ; 점끼리 만남, 종단점('F')인지 연결점('0')인지 판단해야 함.
 *
 *    점 1개가 선분과 만나는 경우
 *
 *               S
 *               |
 *               |
 *         T-----S-----T
 *
 *         ; 종단점('F')인지 연결점('0')인지 판단해야 함.
 *
 *    점 0개가 선분과 만나는 경우
 *
 *         S-----S     T-----T
 *
 *
 *               S
 *               |
 *               |
 *          T----+-----T
 *               |
 *               |
 *               S 
 *
 **********************************************************************/

SChar
stfRelation::compLineSegment( stdPoint2D * aSeg1Pt1,         // LineSeg 1
                              stdPoint2D * aSeg1Pt2,         // LineSeg 1
                              idBool       aIsTermSeg1Pt1,   // is terminal
                              idBool       aIsTermSeg1Pt2,   // is terminal
                              stdPoint2D * aSeg2Pt1,         // LineSeg 2
                              stdPoint2D * aSeg2Pt2,         // LineSeg 2 
                              idBool       aIsTermSeg2Pt1,   // is terminal
                              idBool       aIsTermSeg2Pt2 )  // is terminal
{
    SInt    sMeetCnt;     // 선분과 만나는 점의 개수
    SInt    sOnPointCnt;  // 점과 점이 일치하는 개수
    SInt    sTermMeetCnt; // 선분과 만나는 점이 종단점인 경우의 개수
    SChar   sResult;

    //----------------------------------------
    // Parameter Validation
    //----------------------------------------

    IDE_DASSERT( aSeg1Pt1 != NULL );
    IDE_DASSERT( aSeg1Pt2 != NULL );
    IDE_DASSERT( aSeg2Pt1 != NULL );
    IDE_DASSERT( aSeg2Pt2 != NULL );
    
    //----------------------------------------
    // Initialization
    //----------------------------------------

    sMeetCnt = 0;
    sOnPointCnt = 0;
    sTermMeetCnt = 0;

    //----------------------------------------
    // 점들과 선분의 관계를 파악함.
    //----------------------------------------

    // Segment1 의 Point1 을 검사
    if( stdUtils::between2D( aSeg2Pt1, aSeg2Pt2, aSeg1Pt1 ) == ID_TRUE )
    {
        // 선분과 점이 만남
        sMeetCnt++;

        // 어디서 만나는지 검사
        if( (stdUtils::isSamePoints2D(aSeg2Pt1, aSeg1Pt1)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aSeg2Pt2, aSeg1Pt1)==ID_TRUE) )
        {
            // 점과 점이 만남
            sOnPointCnt++;
        }
        else
        {
            // 선분내에서 점이 만남
        }

        // 점의 종류 검사
        if ( aIsTermSeg1Pt1 == ID_TRUE )
        {
            // 종단점이 선분과 만나고 있음
            sTermMeetCnt++;
        }
        else
        {
            // 연결점이 선분과 만나고 있음
        }
    }
    else
    {
        // 만나지 않음
    }

    // Segment1 의 Point2 을 검사
    if( stdUtils::between2D( aSeg2Pt1, aSeg2Pt2, aSeg1Pt2 ) == ID_TRUE )
    {
        // 선분과 점이 만남
        sMeetCnt++;

        // 어디서 만나는지 검사
        if( (stdUtils::isSamePoints2D(aSeg2Pt1, aSeg1Pt2)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aSeg2Pt2, aSeg1Pt2)==ID_TRUE) )
        {
            // 점과 점이 만남
            sOnPointCnt++;
        }
        else
        {
            // 선분내에서 점이 만남
        }

        // 점의 종류 검사
        if ( aIsTermSeg1Pt2 == ID_TRUE )
        {
            // 종단점이 선분과 만나고 있음
            sTermMeetCnt++;
        }
        else
        {
            // 연결점이 선분과 만나고 있음
        }
    }
    else
    {
        // 만나지 않음
    }

    // Segment2 의 Point1 을 검사
    if( stdUtils::between2D( aSeg1Pt1, aSeg1Pt2, aSeg2Pt1 ) == ID_TRUE )
    {
        // 선분과 점이 만남
        sMeetCnt++;

        // 어디서 만나는지 검사
        if( (stdUtils::isSamePoints2D(aSeg1Pt1, aSeg2Pt1)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aSeg1Pt2, aSeg2Pt1)==ID_TRUE) )
        {
            // 점과 점이 만남
            sOnPointCnt++;
        }
        else
        {
            // 선분내에서 점이 만남
        }

        // 점의 종류 검사
        if ( aIsTermSeg2Pt1 == ID_TRUE )
        {
            // 종단점이 선분과 만나고 있음
            sTermMeetCnt++;
        }
        else
        {
            // 연결점이 선분과 만나고 있음
        }
    }
    else
    {
        // 만나지 않음
    }

    // Segment2 의 Point2 을 검사
    if( stdUtils::between2D( aSeg1Pt1, aSeg1Pt2, aSeg2Pt2 ) == ID_TRUE )
    {
        // 선분과 점이 만남
        sMeetCnt++;

        // 어디서 만나는지 검사
        if( (stdUtils::isSamePoints2D(aSeg1Pt1, aSeg2Pt2)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aSeg1Pt2, aSeg2Pt2)==ID_TRUE) )
        {
            // 점과 점이 만남
            sOnPointCnt++;
        }
        else
        {
            // 선분내에서 점이 만남
        }

        // 점의 종류 검사
        if ( aIsTermSeg2Pt2 == ID_TRUE )
        {
            // 종단점이 선분과 만나고 있음
            sTermMeetCnt++;
        }
        else
        {
            // 연결점이 선분과 만나고 있음
        }
    }
    else
    {
        // 만나지 않음
    }
    
    //----------------------------------------
    // 만나는 점의 개수에 따른 9IM 결과 생성
    //----------------------------------------
    
    switch ( sMeetCnt )
    {
        case 0:

            if( stdUtils::intersectI2D( aSeg1Pt1,
                                        aSeg1Pt2,
                                        aSeg2Pt1,
                                        aSeg2Pt2 ) == ID_TRUE )
            {
                sResult = '0';
            }
            else
            {
                sResult = 'F';
            }
            break;
            
        case 1:
            if ( sTermMeetCnt == 0 )
            {
                // 연결점이 선분과 만남
                sResult = '0';
            }
            else
            {
                // 종단점이 선분과 만남
                IDE_DASSERT( sTermMeetCnt == 1 );
                sResult = 'F';
            }
            break;
            
        case 2:
            if ( sOnPointCnt == 0 )
            {
                // 점에서 만나지 않고 선으로 만나는 경우
                sResult = '1';
            }
            else
            {
                // 점과 점이 만남
                IDE_DASSERT( sOnPointCnt == 2 );
                
                if ( sTermMeetCnt == 0 )
                {
                    // 모두 연결점이 만나고 있음
                    sResult = '0';
                }
                else
                {
                    // 종단점이 만나고 있음
                    IDE_DASSERT( sTermMeetCnt < 3 );
                    
                    sResult = 'F';
                }
            }
            break;
            
        case 3:
        case 4:
            sResult = '1';
            break;
        default:
            IDE_ASSERT(0);
    }
    
    return sResult;
}

/***********************************************************************
 * Description:
 * 선분1과 선분2의 관계 매트릭스를 리턴한다
 * aPtEnd의 점은 관계 연산에서 제외된다
 *
 * stdPoint2D*     aPtEnd(In): 선분1
 * stdPoint2D*     aPtEndNext(In): 선분1
 * stdPoint2D*     aPtMid1(In): 선분2
 * stdPoint2D*     aPtMid2(In): 선분2
 **********************************************************************/
SChar stfRelation::EndLineToMidLine(
                stdPoint2D*     aPtEnd,      // End Point
                stdPoint2D*     aPtEndNext,
                stdPoint2D*     aPtMid1,
                stdPoint2D*     aPtMid2)
{
    SInt    sOnPoint = 0;
    idBool  sAffectEnd = ID_FALSE;

    if(stdUtils::between2D( aPtEnd, aPtEndNext, aPtMid1 )==ID_TRUE)
    {
        sOnPoint++;
    }
    if(stdUtils::between2D( aPtEnd, aPtEndNext, aPtMid2 )==ID_TRUE)
    {
        sOnPoint++;
    }
    if(stdUtils::between2D( aPtMid1, aPtMid2, aPtEnd )==ID_TRUE)
    {
        sOnPoint++;
        sAffectEnd = ID_TRUE;
    }
    if(stdUtils::between2D( aPtMid1, aPtMid2, aPtEndNext )==ID_TRUE)
    {
        sOnPoint++;
    }

    if( sOnPoint >= 3 )
    {
        return '1';
    }
    else if( sOnPoint == 2 )
    {
        if( (stdUtils::isSamePoints2D(aPtEnd, aPtMid1)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aPtEnd, aPtMid2)==ID_TRUE) )
        {
            return 'F';
        }
        else if( (stdUtils::isSamePoints2D(aPtEndNext, aPtMid1)==ID_TRUE) ||
                 (stdUtils::isSamePoints2D(aPtEndNext, aPtMid2)==ID_TRUE) )
        {
            return '0';
        }
        else
        {
            return '1';
        }
    }
    else if( sOnPoint == 1 )
    {
        if( sAffectEnd )
        {
            return 'F';
        }
        else
        {
            return '0';
        }
    }

    if(stdUtils::intersectI2D( aPtEnd, aPtEndNext, aPtMid1, aPtMid2 )==ID_TRUE)
    {
        return '0';
    }

    return 'F';
}

/***********************************************************************
 * Description:
 * 선분1과 선분2의 관계 매트릭스를 리턴한다
 *
 * stdPoint2D*     aPt11(In): 선분1
 * stdPoint2D*     aPt12(In): 선분1
 * stdPoint2D*     aPt21(In): 선분2
 * stdPoint2D*     aPt22(In): 선분2
 **********************************************************************/
SChar stfRelation::MidLineToMidLine(
                stdPoint2D*     aPt11,
                stdPoint2D*     aPt12,
                stdPoint2D*     aPt21,
                stdPoint2D*     aPt22 )
{
    SInt    sOnPoint = 0;

    if(stdUtils::between2D( aPt11, aPt12, aPt21 )==ID_TRUE)
    {
        sOnPoint++;
    }
    if(stdUtils::between2D( aPt11, aPt12, aPt22 )==ID_TRUE)
    {
        sOnPoint++;
    }
    if(stdUtils::between2D( aPt21, aPt22, aPt11 )==ID_TRUE)
    {
        sOnPoint++;
    }
    if(stdUtils::between2D( aPt21, aPt22, aPt12 )==ID_TRUE)
    {
        sOnPoint++;
    }

    if( sOnPoint >= 3 )
    {
        return '1';
    }
    else if( sOnPoint == 2 )
    {
        if( (stdUtils::isSamePoints2D(aPt11, aPt21)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aPt11, aPt22)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aPt12, aPt21)==ID_TRUE) ||
            (stdUtils::isSamePoints2D(aPt12, aPt22)==ID_TRUE) )
        {
            return '0';
        }
        else
        {
            return '1';
        }
    }
    else if( sOnPoint == 1 )
    {
        return '0';
    }

    if(stdUtils::intersect2D( aPt11, aPt12, aPt21, aPt22 )==ID_TRUE)
    {
        return '0';
    }

    return 'F';
}

/***********************************************************************
 * Description:
 *
 *    Line Segment가 Line String 내에 존재하는지 검사
 *
 * Implementation:
 *
 *    Line Segment의 두 점과  Line String을 구성하는 Segment와의 관계와
 *    Line Segment 사이에 존재하는 모든 Line String의 점이 일직선을 이루는지로 판단
 *
 *    Segment S의 점이 모두 T의 Segment에 존재하지 않는다면 포함되지 않음
 *
 *                 S
 *                 |
 *        T----T---S---T---T
 *
 *    m 번째 Segment와 n 번째 Segment에서 각점이 포함되었다면,
 *
 *      ...Tm----S1----T.......Tn---S2----T....
 *
 *    S1과 S2 사이에 있는 모든 점 T가 일직선상에 위치한다면
 *    Segment S는 Line String T 내에 존재한다.
 *
 *    BUG-16941 과 같이 T 가 closed line string일 경우를 고려하여야 한다.
 *
 *      T-------------------------------------T
 *      |                                     |
 *      T...Tn--S2--T...TzTa...Tm---S1---T....T
 *                       
 *
 **********************************************************************/
idBool
stfRelation::lineInLineString( stdPoint2D                * aPt1,
                               stdPoint2D                * aPt2,
                               const stdLineString2DType * aLine )
{
    SInt i;
    idBool sResult;

    stdPoint2D * sFirstPt;

    // Line String의 Segment 개수
    SInt   sLSegCnt;
    idBool sClosed;

    // 점이 포함되는 Segment의 위치
    SInt sPt1SegIdx;
    SInt sPt2SegIdx;

    SInt sBeginIdx;
    SInt sEndIdx;
    
    //-------------------------------------------
    // Parameter Validation
    //-------------------------------------------

    IDE_DASSERT( aPt1 != NULL );
    IDE_DASSERT( aPt2 != NULL );
    IDE_DASSERT( aLine != NULL );

    //-------------------------------------------
    // Initialization
    //-------------------------------------------

    sResult = ID_FALSE;
    sPt1SegIdx = -1;  // 포함되는 위치가 없음
    sPt2SegIdx = -1;  // 포함되는 위치가 없음

    sLSegCnt = STD_N_POINTS(aLine) - 1;
    sClosed = stdUtils::isClosed2D( (stdGeometryHeader*) aLine );

    sFirstPt = STD_FIRST_PT2D(aLine);
    
    //-------------------------------------------
    // Segment의 두 점이
    // Line String의 어느 Segment에 포함되는지 검사
    //-------------------------------------------
    
    for( i = 0; i < sLSegCnt; i++ )
    {
        //-------------------------------
        // 점1 이 Segment에 포함되는지 검사
        //-------------------------------

        if( sPt1SegIdx == -1 )
        {
            // 위치가 결정되지 않음
            if ( stdUtils::between2D( STD_NEXTN_PT2D(sFirstPt,i),
                                      STD_NEXTN_PT2D(sFirstPt,i+1),
                                      aPt1 )
                 == ID_TRUE )
            {
                // Segment 내에 점이 존재하는 경우
                sPt1SegIdx = i;
            }
            else
            {
                // Segment 내에 존재하지 않는 경우
            }
        }
        else
        {
            // 이미 위치가 결정됨
        }

        //-------------------------------
        // 점2 가 Segment에 포함되는지 검사
        //-------------------------------

        if( sPt2SegIdx == -1 )
        {
            // 위치가 결정되지 않음
            if ( stdUtils::between2D( STD_NEXTN_PT2D(sFirstPt,i),
                                      STD_NEXTN_PT2D(sFirstPt,i+1),
                                      aPt2 )
                 == ID_TRUE )
            {
                // Segment 내에 점이 존재하는 경우
                sPt2SegIdx = i;
            }
            else
            {
                // Segment 내에 존재하지 않는 경우
            }
        }
        else
        {
            // 이미 위치가 결정됨
        }

        if( (sPt1SegIdx != -1) && (sPt2SegIdx != -1) )
        {
            // 두 점이 모두 Segment에 포함된 경우 
            break;
        }
    }

    //-------------------------------------------
    // Segment의 두 점의 위치를 이용한 검사
    //-------------------------------------------
    
    if( (sPt1SegIdx == -1) || (sPt2SegIdx == -1) )
    {
        // Line String 상에 두 점이 존재하지 않는다
        // S----ST-----T-----T
        //sResult = ID_FALSE;
    }
    else
    {
        // 두 점이 모두 Line String내의 어느 Segment에 존재함.
        if ( sPt1SegIdx == sPt2SegIdx )
        {
            // 동일한 Segment에 두 점 모두 존재하는 경우
            //
            // T...T---S====S----T...T
            
            sResult = ID_TRUE;
        }
        else
        {
            // 서로 다른 Segment에 두 점이 존재
            // T...T---S===T==T==S---T...T
            
            // 시작 Segment와 종료 Segment를 결정
            if ( sPt1SegIdx < sPt2SegIdx )
            {
                sBeginIdx = sPt1SegIdx;
                sEndIdx = sPt2SegIdx;
            }
            else
            {
                sBeginIdx = sPt2SegIdx;
                sEndIdx = sPt1SegIdx;
            }

            //--------------------------------------
            // 두 점 사이에 존재하는 Line String의 점들이
            // 일직선을 이루는지 Tb+1 부터 Te까지 검사
            //
            // T---Tb--S==T==..==Te==S--T--T--...
            //--------------------------------------
            
            for ( i = sBeginIdx + 1; i <= sEndIdx; i++ )
            {
                if( stdUtils::collinear2D( aPt1, 
                                           aPt2, 
                                           STD_NEXTN_PT2D(sFirstPt,i) )
                    == ID_TRUE )
                {
                    // 세점이 일직선을 이루고 있음.
                    sResult = ID_TRUE;
                }
                else
                {
                    // 세점이 일직선이 아님
                    //
                    //          Ti--S---T
                    //         /
                    // T---S--Ti

                    sResult = ID_FALSE;
                    break;
                }
            }

            if ( (sResult == ID_FALSE) && (sClosed == ID_TRUE) )
            {
                //--------------------------------------------
                // BUG-16941
                // Closed LineString인 경우 역방향 검사가 필요함
                //
                //                   시작점과 끝점
                //                      |
                //                      V
                // ...Te--S==T==..==T==TT0==T==..==Tb==S--T...
                //--------------------------------------------
                
                for ( i = 0; i <= sBeginIdx; i++ )
                {
                    if( stdUtils::collinear2D( aPt1, 
                                               aPt2, 
                                               STD_NEXTN_PT2D(sFirstPt,i) )
                        == ID_TRUE )
                    {
                        // 세점이 일직선을 이루고 있음.
                        sResult = ID_TRUE;
                    }
                    else
                    {
                        sResult = ID_FALSE;
                        IDE_RAISE( LINEINLINE2D_RESULT );
                    }
                }
                
                for ( i = sEndIdx + 1; i <= sLSegCnt; i++ )
                {
                    if( stdUtils::collinear2D( aPt1, 
                                               aPt2, 
                                               STD_NEXTN_PT2D(sFirstPt,i) )
                        == ID_TRUE )
                    {
                        // 세점이 일직선을 이루고 있음.
                        sResult = ID_TRUE;
                    }
                    else
                    {
                        sResult = ID_FALSE;
                        IDE_RAISE( LINEINLINE2D_RESULT );
                    }
                }
            }
            else
            {
                // Go Go
            }
        }
    }

    IDE_EXCEPTION_CONT( LINEINLINE2D_RESULT );
    
    return sResult;
}


/***********************************************************************
 * Description:
 * 한 선분이 링의 Boundary에 포함되면 ID_TRUE 아니면 ID_FASLE 리턴
 *
 * stdPoint2D*             aPt1(In): 선분
 * stdPoint2D*             aPt2(In): 선분
 * stdLinearRing2D*        aRing(In): 링
 **********************************************************************/
idBool stfRelation::lineInRing(
                            stdPoint2D*             aPt1,
                            stdPoint2D*             aPt2,
                            stdLinearRing2D*        aRing )
{
    SInt            i, sMax, sIdx1, sIdx2, sFence;
    idBool          sInside;
    stdPoint2D*     sPt;

    sIdx1 = -1;
    sIdx2 = -1;
    sMax = (SInt)(STD_N_POINTS(aRing)-1);
    sPt = STD_FIRST_PT2D(aRing);
    for( i = 0; i < sMax; i++ )
    {
        if( (sIdx1 == -1) && (
            (stdUtils::isSamePoints2D(
                STD_NEXTN_PT2D(sPt,i),
                aPt1)==ID_TRUE) ||
            (stdUtils::betweenI2D( 
                STD_NEXTN_PT2D(sPt,i), 
                STD_NEXTN_PT2D(sPt,i+1), 
                aPt1 )==ID_TRUE)) )
        {
            sIdx1 = i;
        }

        if( (sIdx2 == -1) && (
            (stdUtils::isSamePoints2D(
                STD_NEXTN_PT2D(sPt,i), 
                aPt2)==ID_TRUE) ||
            (stdUtils::betweenI2D( 
                STD_NEXTN_PT2D(sPt,i), 
                STD_NEXTN_PT2D(sPt,i+1), 
                aPt2 )==ID_TRUE)) )
        {
            sIdx2 = i;
        }

        if( (sIdx1 != -1) && (sIdx2 != -1) )
        {
            break;
        }
    }

    if( (sIdx1 == -1) || (sIdx2 == -1) )
    {
        return ID_FALSE;
    }
    else if( sIdx1 == sIdx2 )
    {
        return ID_TRUE;
    }
    else
    {
        if(sIdx1 > sIdx2)
        {
            sFence = sIdx2 + sMax;
        }
        else
        {
            sFence = sIdx2;
        }

        sInside = ID_TRUE;
        for( i = sIdx1+1; i < sFence; i++ )
        {
            if(stdUtils::collinear2D( 
                aPt1, 
                aPt2, 
                STD_NEXTN_PT2D(sPt,i%sMax) ) == ID_FALSE)
            {
                sInside = ID_FALSE;
                break;
            }
        }
        if(sInside==ID_TRUE)
        {
            return ID_TRUE;
        }

        if(sIdx1 < sIdx2)
        {
            sFence = sIdx1 + sMax;
        }
        else
        {
            sFence = sIdx1;
        }

        for( i = sIdx2+1; i < sFence; i++ )
        {
            if(stdUtils::collinear2D( 
                aPt1, 
                aPt2, 
                STD_NEXTN_PT2D(sPt,i%sMax) ) == ID_FALSE)
            {
                return ID_FALSE;
            }
        }
        return ID_TRUE;
    }
}

//==============================================================================
// 객체와 객체의 관계를 리턴하는 함수의 이름은 다음과 같다
//
// Single       s
// Multi        m
//
// Point        p
// Line         l
// Area         a
// Collection   c
//
// Interior     i
// Boundary     b
// Exterior     e
//
// 결과 값은 차원을 리턴한다
// '0' 점
// '1' 선
// '2' 면
// 'F' 존재하지 않는다.
//==============================================================================

/* Point **************************************************************/

// point to point
SChar stfRelation::spiTospi( const stdPoint2D*                  aObj1,
                             const stdPoint2D*                  aObj2 )
{
    return ((aObj1->mX == aObj2->mX ) && (aObj1->mY == aObj2->mY)) ? '0' : 'F';
}

SChar stfRelation::spiTospe( const stdPoint2D*                  aObj1,
                             const stdPoint2D*                  aObj2 )
{
    return ((aObj1->mX != aObj2->mX ) || (aObj1->mY != aObj2->mY)) ? '0' : 'F';
}

// point vs linestring
SChar stfRelation::spiTosli( const stdPoint2D*                  aObj1,
                             const stdLineString2DType*         aObj2 )
{
    UInt        i, sMax;    
    stdPoint2D* sPt;

    // Fix BUG-15923
    if(stdUtils::isClosedPoint2D(aObj1, (stdGeometryHeader*)aObj2))
    {
        return '0';
    }
    
    sPt = STD_FIRST_PT2D(aObj2);
    sMax = STD_N_POINTS(aObj2)-1;

    for( i = 0; i < sMax; i++ )
    {
        if( ( i > 0 ) && (spiTospi( sPt+i, aObj1 )  == '0' ) )
        {
            return '0';
        }

        if(stdUtils::betweenI2D( 
            STD_NEXTN_PT2D(sPt,i), 
            STD_NEXTN_PT2D(sPt,i+1), 
            aObj1 )==ID_TRUE)
        {
            return '0';
        }
    }
    return 'F';
}

SChar stfRelation::spiToslb( const stdPoint2D*                  aObj1,
                             const stdLineString2DType*         aObj2 )
{
    stdPoint2D*     sPtS = STD_FIRST_PT2D(aObj2);
    stdPoint2D*     sPtE = STD_LAST_PT2D(aObj2);
    
    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    if( spiTospi( aObj1, sPtS ) == '0' )
    {
        return '0';
    }
    if( spiTospi( aObj1, sPtE ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::spiTosle( const stdPoint2D*                  aObj1,
                             const stdLineString2DType*         aObj2 )
{
    UInt        i, sMax;
    stdPoint2D* sPt;

    sPt = STD_FIRST_PT2D(aObj2);
    sMax = STD_N_POINTS(aObj2)-1;

    // Fix BUG-15544
    for( i = 0; i < sMax; i++ )  // Fix BUG-15518
    {
        if(stdUtils::between2D( 
            STD_NEXTN_PT2D(sPt,i), 
            STD_NEXTN_PT2D(sPt,i+1), 
            aObj1 )==ID_TRUE)
        {
            return 'F';
        }
    }
    return '0';
}

//==============================================================================
// TASK-2015 한 점이 링 내부에 존재하는지 판별
//==============================================================================
// point vs polygon
SChar stfRelation::spiTosai( const stdPoint2D*                  aObj1,
                             const stdPolygon2DType*            aObj2 )
{
    stdLinearRing2D*    sRings;
    UInt                i, sMax;
    
    sRings = STD_FIRST_RN2D(aObj2);
    sMax = STD_N_RINGS(aObj2);
    
    // Fix BUG-15433
    if( stdUtils::insideRingI2D(aObj1, sRings)==ID_TRUE )
    {
        if( STD_N_RINGS(aObj2) > 1 )
        {
            sRings = STD_NEXT_RN2D(sRings);
            for( i = 1; i < sMax; i++ )
            {
                if( stdUtils::insideRing2D(aObj1, sRings)==ID_TRUE )
                {
                    return 'F';
                }
                sRings = STD_NEXT_RN2D(sRings);
            }
            return '0';
        }
        else
        {
            return '0';
        }
    }
    else
    {
        return 'F';
    }
}

SChar stfRelation::spiTosab( const stdPoint2D*                  aObj1,
                             const stdPolygon2DType*            aObj2 )
{
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPt;
    UInt                i, j , sMax, sMaxR;
    
    sRing = STD_FIRST_RN2D(aObj2);
    sMaxR = STD_N_RINGS(aObj2);
    for( i = 0; i < sMaxR; i++ )
    {
        sMax = STD_N_POINTS(sRing)-1;
        sPt = STD_FIRST_PT2D(sRing);
        for( j = 0; j < sMax; j++ )
        {
            if( stdUtils::between2D( 
                sPt, 
                STD_NEXT_PT2D(sPt), 
                aObj1)==ID_TRUE )
            {
                return '0';
            }
            sPt = STD_NEXT_PT2D(sPt);
        }
        sPt = STD_NEXT_PT2D(sPt);
        sRing = (stdLinearRing2D*)sPt;
    }
    return 'F';
}

//==============================================================================
// TASK-2015 한 점이 링 내부에 존재하는지 판별
//==============================================================================
SChar stfRelation::spiTosae( const stdPoint2D*                  aObj1,
                             const stdPolygon2DType*            aObj2 )
{
    stdLinearRing2D*    sRings;
    UInt                i, sMax;
    
    sRings = STD_FIRST_RN2D(aObj2);
    sMax = STD_N_RINGS(aObj2);

    // Fix BUG-15544
    if( stdUtils::insideRing2D(aObj1, sRings)==ID_TRUE )
    {
        if( STD_N_RINGS(aObj2) > 1 )
        {
            sRings = STD_NEXT_RN2D(sRings);
            for( i = 1; i < sMax; i++ )
            {
                if( stdUtils::insideRingI2D(aObj1, sRings)==ID_TRUE )
                {
                    return '0';
                }
                sRings = STD_NEXT_RN2D(sRings);
            }
            return 'F';
        }
        else
        {
            return 'F';
        }
    }
    else
    {
        return '0';
    }
}

// point vs multipoint
SChar stfRelation::spiTompi( const stdPoint2D*                  aObj1,
                             const stdMultiPoint2DType*         aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;
    
    sPoint = STD_FIRST_POINT2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTospi(aObj1, &sPoint->mPoint) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::spiTompe( const stdPoint2D*                  aObj1,
                             const stdMultiPoint2DType*         aObj2 )
{
    return (spiTompi(aObj1, aObj2) == '0') ? 'F' : '0';
}

SChar stfRelation::speTompi( const stdPoint2D*                  aObj1,
                             const stdMultiPoint2DType*         aObj2 )
{
    if( STD_N_OBJECTS(aObj2) < 1 )
    {
        return 'F';
    }
    if( (STD_N_OBJECTS(aObj2) == 1) && (spiTompi(aObj1, aObj2) == '0') )
    {
        return 'F';
    }
    return '0';
}

// point vs multilinestring
SChar stfRelation::spiTomli( const stdPoint2D*                  aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdLineString2DType* sLine;
    UInt                 i, sMax;
    
    // Fix BUG-15923
    if(stdUtils::isClosedPoint2D(aObj1, (stdGeometryHeader*)aObj2))
    {
        return '0';
    }
    
    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if(spiTosli( aObj1, sLine ) == '0')
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::spiTomlb( const stdPoint2D*                  aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdLineString2DType* sLine;
    UInt                 i, sMax;
    
    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }
    
    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if(spiToslb( aObj1, sLine ) == '0')
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::spiTomle( const stdPoint2D*                  aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdLineString2DType* sLine;
    UInt                 i, sMax;
    
    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    if( sMax < 1 )
    {
        return 'F';
    }
    for( i = 0; i < sMax; i++ )
    {
        if( ( spiTosli( aObj1, sLine ) == '0' ) ||
            ( spiToslb( aObj1, sLine ) == '0' ) )
        {
            return 'F';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return '0';
}

// point vs multipolygon
SChar stfRelation::spiTomai( const stdPoint2D*                  aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    
    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if(spiTosai( aObj1, sPoly ) == '0')
        {
            return '0';
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    return 'F';
}

SChar stfRelation::spiTomab( const stdPoint2D*                  aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    
    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if(spiTosab( aObj1, sPoly ) == '0')
        {
            return '0';
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    return 'F';
}

SChar stfRelation::spiTomae( const stdPoint2D*                  aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    
    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    if( sMax < 1 )
    {
        return 'F';
    }
    for( i = 0; i < sMax; i++ )
    {
        if( ( spiTosai( aObj1, sPoly ) == '0' ) ||
            ( spiTosab( aObj1, sPoly ) == '0' ) )
        {
            return 'F';
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    return '0';
}

// point vs collection
SChar stfRelation::spiTogci( const stdPoint2D*                  aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    
    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            if(spiTospi( aObj1, &sGeom->point2D.mPoint ) == '0')
            {
                return '0';
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            if(spiTosli( aObj1, &sGeom->linestring2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            if(spiTosai( aObj1, &sGeom->polygon2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            if(spiTompi( aObj1, &sGeom->mpoint2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            if(spiTomli( aObj1, &sGeom->mlinestring2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            if(spiTomai( aObj1, &sGeom->mpolygon2D ) == '0')
            {
                return '0';
            }
            break;
        default:
            return 'F';
        }
        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::spiTogcb( const stdPoint2D*                  aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    
    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            if(spiToslb( aObj1, &sGeom->linestring2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            if(spiTosab( aObj1, &sGeom->polygon2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            if(spiTomlb( aObj1, &sGeom->mlinestring2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            if(spiTomab( aObj1, &sGeom->mpolygon2D ) == '0')
            {
                return '0';
            }
            break;
        default:
            return 'F';
        }
        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::spiTogce( const stdPoint2D*                  aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    if( STD_N_GEOMS(aObj2) < 1 )
    {
        return 'F';
    }
    if( (spiTogci(aObj1, aObj2) == '0') ||
        (spiTogcb(aObj1, aObj2) == '0') )
    {
        return 'F';
    }
    return '0';
}

SChar stfRelation::speTogci( const stdPoint2D*                  aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTospe( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            return '2';
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = speTompi( aObj1, &sGeom->mpoint2D );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            return '2';
        default:
            return 'F';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {
        
            if ( sResult < sRet ) 
            {
                sResult = sRet;
            } 
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::speTogcb( const stdPoint2D*              /*aObj1*/,
                             const stdGeoCollection2DType*    aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = '0';
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            return '1';
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = '0';
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            return '1';
        default:
            return 'F';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

/***********************************************************************
 * Description:
 *
 *    SLI(Single LineString Internal)과 SLI의
 *    DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *    각 선을 Segment 단위로 쪼개어 관계를 구하고 이의 최대값을 구한다.
 *
 ***********************************************************************/

SChar
stfRelation::sliTosli( const stdLineString2DType*         aObj1,
                       const stdLineString2DType*         aObj2 )
{
    UInt         i;
    UInt         j;

    SChar        sTemp;
    SChar        sResult;
    
    stdPoint2D * sPt1;
    stdPoint2D * sPt2;

    // LineString을 구성하는 Segment의 개수
    UInt         sLSegCnt1;
    UInt         sLSegCnt2;

    // 단혀있는 LineString인지의 여부
    idBool       sClosed1;
    idBool       sClosed2;

    // To Fix BUG-16912
    // 종단점인지 연결점인지의 여부
    idBool       sIsTermSeg1Begin;
    idBool       sIsTermSeg1End;
    idBool       sIsTermSeg2Begin;
    idBool       sIsTermSeg2End;
    
    //--------------------------------
    // Parameter Validation
    //--------------------------------

    IDE_DASSERT( aObj1 != NULL );
    IDE_DASSERT( aObj2 != NULL );

    //--------------------------------
    // Inialization
    //--------------------------------

    sResult = 'F';

    sLSegCnt1 = STD_N_POINTS(aObj1) - 1;
    sLSegCnt2 = STD_N_POINTS(aObj2) - 1;
    
    sClosed1 = stdUtils::isClosed2D((stdGeometryHeader*)aObj1);
    sClosed2 = stdUtils::isClosed2D((stdGeometryHeader*)aObj2);

    //--------------------------------
    // 두 LineString을 Segment 단위로 관계를 검사
    //--------------------------------
    
    for( i = 0, sPt1 = STD_FIRST_PT2D(aObj1);
         i < sLSegCnt1;
         i++, sPt1 = STD_NEXT_PT2D(sPt1) )
    {
        //--------------------------
        // BUG-16912 Segment1의 점이 종단점인지 판단
        //--------------------------

        if ( sClosed1 != ID_TRUE )
        {
            // 열린 LineString인 경우
            if ( i == 0 )
            {
                sIsTermSeg1Begin = ID_TRUE;
            }
            else
            {
                sIsTermSeg1Begin = ID_FALSE;
            }

            if ( (i+1) == sLSegCnt1 )
            {
                sIsTermSeg1End = ID_TRUE;
            }
            else
            {
                sIsTermSeg1End = ID_FALSE;
            }
        }
        else
        {
            // 닫힌 LineString인 경우
            sIsTermSeg1Begin = ID_FALSE;
            sIsTermSeg1End = ID_FALSE;
        }

        //--------------------------
        // Segment1과 LineString의 관계를 검사
        //--------------------------
        
        for( j = 0, sPt2 = STD_FIRST_PT2D(aObj2);
             j < sLSegCnt2;
             j++, sPt2 = STD_NEXT_PT2D(sPt2) )
        {
            //--------------------------
            // BUG-16912 Segment2의 점이 종단점인지 판단
            //--------------------------

            if ( sClosed2 != ID_TRUE )
            {
                // 열린 LineString인 경우
                if ( j == 0 )
                {
                    sIsTermSeg2Begin = ID_TRUE;
                }
                else
                {
                    sIsTermSeg2Begin = ID_FALSE;
                }

                if ( (j+1) == sLSegCnt2 )
                {
                    sIsTermSeg2End = ID_TRUE;
                }
                else
                {
                    sIsTermSeg2End = ID_FALSE;
                }
            }
            else
            {
                // 닫힌 LineString인 경우
                sIsTermSeg2Begin = ID_FALSE;
                sIsTermSeg2End = ID_FALSE;
            }

            //--------------------------
            // Line Segment간의 관계 획득
            //--------------------------
            
            sTemp = compLineSegment( sPt1,
                                     STD_NEXT_PT2D(sPt1),
                                     sIsTermSeg1Begin,
                                     sIsTermSeg1End,
                                     sPt2,
                                     STD_NEXT_PT2D(sPt2),
                                     sIsTermSeg2Begin,
                                     sIsTermSeg2End );

            //--------------------------
            // 관계 결과 누적
            //--------------------------
            
            if ( sTemp == 'F' )
            {
                // 누적할 필요 없음
            }
            else
            {
                IDE_DASSERT( (sTemp == '0') || (sTemp == '1') );
                
                sResult = sTemp;
                
                // 선분이 교차하는 경우가 최대 관계임
                IDE_TEST_RAISE( sResult == '1', SLI2D_MAX_RESULT );
            }
        }
    }

    IDE_EXCEPTION_CONT(SLI2D_MAX_RESULT);
    
    return sResult;
}

SChar stfRelation::sliToslb( const stdLineString2DType*         aObj1,
                             const stdLineString2DType*         aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj2) ;
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj2);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    if( spiTosli( sPtS, aObj1 ) == '0' )
    {
        return '0';
    }

    if( spiTosli( sPtE, aObj1 ) == '0' )
    {
        return '0';
    }

    return 'F';
}

SChar stfRelation::sliTosle( const stdLineString2DType*         aObj1,
                             const stdLineString2DType*         aObj2 )
{
    stdPoint2D*     sPt1;
    UInt            i, sMax;

    // Fix BUG-15516
    sPt1 = STD_FIRST_PT2D(aObj1);
    sMax = STD_N_POINTS(aObj1)-1;
    for( i = 0; i < sMax; i++ )
    {
        if( lineInLineString( 
            STD_NEXTN_PT2D(sPt1,i), 
            STD_NEXTN_PT2D(sPt1,i+1), 
            aObj2 ) == ID_FALSE )
        {
            return '1';
        }
    }

    return 'F';
}

SChar stfRelation::slbToslb( const stdLineString2DType*         aObj1,
                             const stdLineString2DType*         aObj2 )
{
    stdPoint2D* sPt1S = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPt2S = STD_FIRST_PT2D(aObj2);
    stdPoint2D* sPt1E = STD_LAST_PT2D(aObj1);
    stdPoint2D* sPt2E = STD_LAST_PT2D(aObj2);


    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }
    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    if( stdUtils::isSamePoints2D(sPt1S, sPt2S)==ID_TRUE )
    {
        return '0';
    }
    if( stdUtils::isSamePoints2D(sPt1S, sPt2E)==ID_TRUE )
    {
        return '0';
    }
    if( stdUtils::isSamePoints2D(sPt1E, sPt2S)==ID_TRUE )
    {
        return '0';
    }
    if( stdUtils::isSamePoints2D(sPt1E, sPt2E)==ID_TRUE )
    {
        return '0';
    }

    return 'F';
}

SChar stfRelation::slbTosle( const stdLineString2DType*         aObj1,
                             const stdLineString2DType*         aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTosle( sPtS, aObj2) == '0' )
    {
        return '0';
    }
    if( spiTosle( sPtE, aObj2) == '0' )
    {
        return '0';
    }

    return 'F';
}

/***********************************************************************
 * Description:
 *
 *    SLI(Single LineString Internal)과
 *    SAI(Single Area Internal)의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *   LineString을 구성하는 [점, 선분] 과
 *   Polygon을 구성하는 [점, 선분, 면]의 관계를 통해 구해낸다.
 *
 *   ================================
 *     LineString .vs. Polygon
 *   ================================
 *
 *   1. 선분 .vs. 면
 *
 *       - 이 함수의 목적으로 다른 관계로부터 유추
 *
 *   2. 점 .vs. 면 
 *       - 점이 외부에 존재 : 유추 불가
 *       - 점이 내부에 존재 : TRUE
 *
 *                A--------A
 *                |        |
 *                |   L    |
 *                |        |
 *                A--------A
 *                 
 *   3. 선분 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 선분이 선분으로 교차 : 점과 점의 관계로부터 유추 가능
 *
 *             L---A====L----A
 *
 *             L---A====A----L
 *
 *             A---L====L----A
 *
 *       - 선분이 점에서 교차 : 점과 점의 관계로부터 유추
 *
 *                L
 *                |
 *            A---+----A
 *                |
 *                L
 *
 *          그러나, 교차하더라도 다음과 같이 Interior Ring의 한점일 경우를
 *          고려하여야 한다.
 *
 *                L
 *                |
 *            A---I----A
 *               /|\
 *              I L I
 *
 *   4. 점 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Lp                     Lp
 *               |                      |
 *         A-----L------A  ==>   Ap----AL----An  
 *               |                      |
 *               Ln                     Ln
 *
 *   5. 선분 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Ap                    Ap
 *               |                     |
 *         L-----A-----L  ==>   Lp----LA----Ln  
 *               |                     |
 *               An                    An
 *
 *   6. 점 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점에서 교차 : 점과 점의 관계로부터 유추
 *
 *                Lp
 *                |
 *         Ap----AL-----An
 *                |
 *                Ln
 *
 *   BUG-16977 : BUG-16952와 동일한 방식으로 기존 로직을 다음과 같이 수정함
 *      1) Line의 점과 Area의 선분이 교차 로직 ==> 점과 점의 교차 알고리즘으로 수정
 *      2) Line의 선분과 Area의 점이 교차 로직 ==> 점과 점의 교차 알고리즘으로 수정
 *      3) 점과 점의 교차 로직 ==> 수평에 대한 검출이 되도록 확장
 *
 *   SLI.vs.SAE 와 달리 주의하여야 할 사항이 있다.
 *
 *   점과 점의 관계에서 다음과 같이 내부에 있다 하더라도
 *   선분이 Interia Ring 때문에 내부영역과 교차함을 보장할 수 없다.
 *   따라서, 구성된 Line이 Interior Ring내부에 포함되지 않음을 검사해야 함.
 *
 *      Ap  
 *       \    Area
 *        \
 *         \      P               
 *          A------An              
 *
 *      Ap       I
 *       \   I  /  P
 *        \  | /
 *         \ |/
 *          AI-------An
 *
 ***********************************************************************/

SChar
stfRelation::sliTosai( const stdLineString2DType * aLineObj,
                       const stdPolygon2DType    * aAreaObj )
{
    UInt i, j, k, m, n;
    idBool  sMeetOnPoint;
    SChar   sResult;
    
    //----------------------------
    // Line String 정보
    //----------------------------
    
    stdPoint2D      * sLinePt;
    stdPoint2D      * sLinePrevPt;
    stdPoint2D      * sLineCurrPt;
    stdPoint2D      * sLineNextPt;
    UInt              sLineSegCnt;
    UInt              sLinePtCnt;
    
    //----------------------------
    // Ring 정보
    //----------------------------
    
    stdLinearRing2D * sRing;
    stdPoint2D      * sRingPt;
    stdPoint2D      * sRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * sRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sRingCnt;     // Ring Count of a Polygon
    UInt              sRingSegCnt;  // Segment Count of a Ring
    idBool            sRingCCWise;  // Ring 이 시계 역방향인지의 여부

    UInt              sCheckSegCnt;
    stdLinearRing2D * sCheckRing;
    stdPoint2D      * sCheckPt;
    stdPoint2D        sItxPt;  // Intersection Point

    //----------------------------------------
    // Parameter Validation
    //----------------------------------------

    IDE_DASSERT( aLineObj != NULL );
    IDE_DASSERT( aAreaObj != NULL );
        
    //----------------------------------------
    // Initialization
    //----------------------------------------
    
    sResult = 'F';
    
    sLineSegCnt = STD_N_POINTS(aLineObj) - 1;
    sLinePtCnt = STD_N_POINTS(aLineObj);

    sRingCnt = STD_N_RINGS(aAreaObj);
    
    //----------------------------------------
    // LineString의 점과 Polygon의 내부 면의 관계
    //----------------------------------------

    for ( i = 0, sLinePt = STD_FIRST_PT2D(aLineObj);
          i < sLinePtCnt;
          i++, sLinePt = STD_NEXT_PT2D(sLinePt) )
    {
        // LineString의 한점이 Polygon의 내부에 존재하는지 판단
        if( spiTosai( sLinePt, aAreaObj ) == '0' )
        {
            sResult = '1';
            IDE_RAISE( SLISAI2D_MAX_RESULT );
        }
        else
        {
            // Nothing To Do
        }
    }

    //----------------------------------------
    // LineString의 점, 선분과 Polygon의 점, 선분의 관계
    //----------------------------------------

    for ( i = 0, sLinePt = STD_FIRST_PT2D(aLineObj), sLinePrevPt = NULL;
          i < sLinePtCnt;
          i++, sLinePrevPt = sLinePt, sLinePt = STD_NEXT_PT2D(sLinePt) )
    {
        // Ring의 개수만큼 반복
        for ( j = 0, sRing = STD_FIRST_RN2D(aAreaObj);
              j < sRingCnt;
              j++, sRing = STD_NEXT_RN2D(sRing) )
        {
            sRingSegCnt = STD_N_POINTS(sRing) - 1;
            sRingCCWise = stdUtils::isCCW2D(sRing);

            // Ring을 구성하는 Segment 개수만큼 반복
            for ( k = 0, sRingPt = STD_FIRST_PT2D(sRing);
                  k < sRingSegCnt;
                  k++, sRingPt = STD_NEXT_PT2D(sRingPt) )
            {
                //----------------------------------------
                // 다음 관계를 점과 점의 관계로 만들기 위한 조정
                //  - LineString 내부 선분과 Ring의 선분과의 관계
                //  - LineString 내부 선분과 Ring의 점과의 관계
                //  - Ring의 내부 선분과 LineString의 점과의 관계
                //  - LineString의 점과 Ring의 점과의 관계
                //----------------------------------------

                sMeetOnPoint = ID_FALSE;

                //----------------------------
                // LineString의 현재점, 이전점, 이후점을 구함
                //----------------------------
                
                // sLinePrevPt :  for loop을 통해 추출
                sLineCurrPt = sLinePt;
                if ( i == sLineSegCnt )
                {
                    sLineNextPt = NULL;
                }
                else
                {
                    sLineNextPt = STD_NEXT_PT2D(sLinePt);
                }
                
                //----------------------------
                // Ring의 현재점, 이전점, 이후점을 구함
                //----------------------------
                
                sRingPrevPt = stdUtils::findPrevPointInRing2D( sRingPt,
                                                               k,
                                                               sRingSegCnt,
                                                               NULL );
                sRingCurrPt = sRingPt;
                sRingNextPt = stdUtils::findNextPointInRing2D( sRingPt,
                                                               k,
                                                               sRingSegCnt,
                                                               NULL );

                // 선분의 내부가 교차하는지 검사
                if( ( i < sLineSegCnt ) &&
                    ( stdUtils::intersectI2D( sLinePt,
                                              STD_NEXT_PT2D(sLinePt),
                                              sRingPt,
                                              STD_NEXT_PT2D(sRingPt) )
                      ==ID_TRUE ) )
                {
                    // 선분이 교차하는 경우
                    // 선분이 교차하더라도 다른 Ring의 한점인지 판단하여야 한다.
                    //
                    //      L
                    //      |
                    //  A---I----A
                    //     /|+
                    //    I L I

                    if ( sRingCnt > 1 )
                    {
                        IDE_ASSERT( stdUtils::getIntersection2D(
                                        sLinePt,
                                        STD_NEXT_PT2D(sLinePt),
                                        sRingPt,
                                        STD_NEXT_PT2D(sRingPt),
                                        & sItxPt ) == ID_TRUE );
                        
                        sMeetOnPoint = ID_TRUE;
                        
                        sLinePrevPt = sLinePt;
                        sLineCurrPt = & sItxPt;
                        sLineNextPt = STD_NEXT_PT2D(sLinePt);
                        
                        sRingPrevPt = sRingPt;
                        sRingCurrPt = & sItxPt;
                        sRingNextPt = STD_NEXT_PT2D(sRingPt);
                    }
                    else
                    {
                        sResult = '1';
                        IDE_RAISE( SLISAI2D_MAX_RESULT );
                    }
                }
                
                //----------------------------
                // 선분내에 Ring의 점이 존재하는지 검사
                //----------------------------
                
                if ( ( i < sLineSegCnt ) &&
                     ( stdUtils::betweenI2D( sLinePt,
                                             STD_NEXT_PT2D(sLinePt),
                                             sRingPt )==ID_TRUE ) )
                {
                    // 점이 선분에서 교차 => 점과 점의 관계로 변경
                    //               Ap                    Ap
                    //               |                     |
                    //         L-----A-----L  ==>   Lp----LA----Ln  
                    //               |                     |
                    //               An                    An
                    sMeetOnPoint = ID_TRUE;
                    
                    sLinePrevPt = sLinePt;
                    sLineCurrPt = sRingPt;
                    sLineNextPt = STD_NEXT_PT2D(sLinePt);
                }
                
                //----------------------------
                // Ring 선분내에 LineString의 점이 존재하는지 검사
                //----------------------------

                if ( stdUtils::betweenI2D( sRingPt,
                                           STD_NEXT_PT2D(sRingPt),
                                           sLinePt ) == ID_TRUE )
                {
                    // 점이 선분에서 교차 => 점과 점의 관계로 변경
                    //               Lp                     Lp
                    //               |                      |
                    //         A-----L------A  ==>   Ap----AL----An  
                    //               |                      |
                    //               Ln                     Ln
                    sMeetOnPoint = ID_TRUE;
                        
                    sRingPrevPt = sRingPt;
                    sRingCurrPt = sLinePt;
                    sRingNextPt = stdUtils::findNextPointInRing2D( sRingPt,
                                                                   k,
                                                                   sRingSegCnt,
                                                                   NULL );
                }

                //----------------------------
                // 점과 점이 교차하는 지 검사
                //----------------------------
                
                if ( stdUtils::isSamePoints2D( sLinePt, sRingPt ) == ID_TRUE )
                {
                    sMeetOnPoint = ID_TRUE;

                    // 이미 구해진 값을 사용
                }

                //----------------------------
                // Ring과 Ring이 겹치는 점인지 여부를 검사
                //----------------------------
                
                if ( sMeetOnPoint == ID_TRUE )
                {
                    // Ring과 Ring이 겹치는 점일 경우
                    // 내부에 존재하는 지를 판단할 수 없다.
                    // 다른 점에 의하여 판별 가능하다.
                    //
                    //              Pn
                    //       Ap       I
                    //        \   I  /  Pn
                    //         \  | /
                    //          \ |/
                    //           AIP-------An

                    for ( m = 0, sCheckRing = STD_FIRST_RN2D(aAreaObj);
                          m < sRingCnt;
                          m++, sCheckRing = STD_NEXT_RN2D(sCheckRing) )
                    {
                        if ( j == m )
                        {
                            // 자신의 Ring은 검사하지 않음
                            continue;
                        }
                        else
                        {
                            sCheckSegCnt = STD_N_POINTS(sCheckRing) - 1;
                            
                            for ( n = 0, sCheckPt = STD_FIRST_PT2D(sCheckRing);
                                  n < sCheckSegCnt;
                                  n++, sCheckPt = STD_NEXT_PT2D(sCheckPt) )
                            {
                                if ( stdUtils::between2D(
                                         sCheckPt,
                                         STD_NEXT_PT2D(sCheckPt),
                                         sLineCurrPt ) == ID_TRUE )
                                {
                                    sMeetOnPoint = ID_FALSE;
                                    break;
                                }
                            }
                            
                            if ( sMeetOnPoint != ID_TRUE )
                            {
                                break;
                            }
                            
                        }
                    }
                        
                }
                
                //----------------------------------------
                // 점과 점의 관계로부터 외부 교차의 판단
                //----------------------------------------
                
                if ( sMeetOnPoint == ID_TRUE )
                {
                    if ( hasRelLineSegRingSeg(
                             (j == 0 ) ? ID_TRUE : ID_FALSE, // Exterior Ring
                             sRingCCWise,
                             sRingPrevPt,
                             sRingNextPt,
                             sRingCurrPt,
                             sLinePrevPt,
                             sLineNextPt,
                             STF_INSIDE_ANGLE_POS ) == ID_TRUE )
                    {
                        sResult = '1';
                        IDE_RAISE( SLISAI2D_MAX_RESULT );
                    }
                    else
                    {
                        // 교차 여부를 판단할 수 없음
                    }
                }
                else // sMeetOnPoint == ID_FALSE
                {
                    // 검사 대상이 아님
                }
                
            } // for k
        } // for j
    } // for i

    IDE_EXCEPTION_CONT(SLISAI2D_MAX_RESULT);
    
    return sResult;
}


/***********************************************************************
 * Description:
 *
 *    SLI(Single LineString Internal)과
 *    SAB(Single Area Boundary)의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *    각 선을 Segment 단위로 쪼개어 관계를 구하고 이의 최대값을 구한다.
 *
 ***********************************************************************/

SChar
stfRelation::sliTosab( const stdLineString2DType*         aObj1,
                       const stdPolygon2DType*            aObj2 )
{
    UInt         i;
    UInt         j;
    UInt         k;

    SChar        sTemp;
    SChar        sResult;
    
    stdPoint2D      * sPt1;
    stdPoint2D      * sPt2;
    stdLinearRing2D * sRing;
    
    UInt         sLineLSegCnt;  // Line String의 Line Segment 개수
    UInt         sRingCnt;      // Polygon을 구성하는 Ring의 개수
    UInt         sRingLSegCnt;  // 각 Ring을 구성하는 Line Segment 개수

    // 닫혀있는 LineString인지의 여부
    idBool       sClosed;

    // To Fix BUG-16915
    // 종단점인지 연결점인지의 여부, Polygon은 종단점이 없다.
    idBool       sIsTermSegBegin;
    idBool       sIsTermSegEnd;

    //--------------------------------
    // Parameter Validation
    //--------------------------------

    IDE_DASSERT( aObj1 != NULL );
    IDE_DASSERT( aObj2 != NULL );

    //--------------------------------
    // Inialization
    //--------------------------------

    sResult = 'F';
    
    sLineLSegCnt = STD_N_POINTS(aObj1) - 1;
    sClosed = stdUtils::isClosed2D((stdGeometryHeader*)aObj1);
    
    sRingCnt = STD_N_RINGS(aObj2);

    //--------------------------------
    // LineString을 Segment 단위로 관계를 검사
    //--------------------------------
    
    for( i = 0, sPt1 = STD_FIRST_PT2D(aObj1);
         i < sLineLSegCnt;
         i++, sPt1 = STD_NEXT_PT2D(sPt1) )
    {
        //--------------------------
        // BUG-16915 Line Segment의 점이 종단점인지 판단
        //--------------------------

        if ( sClosed != ID_TRUE )
        {
            // 열린 LineString인 경우
            if ( i == 0 )
            {
                sIsTermSegBegin = ID_TRUE;
            }
            else
            {
                sIsTermSegBegin = ID_FALSE;
            }

            if ( (i+1) == sLineLSegCnt )
            {
                sIsTermSegEnd = ID_TRUE;
            }
            else
            {
                sIsTermSegEnd = ID_FALSE;
            }
        }
        else
        {
            // BUG-16915 CLOSE 여부를 판단해야 함.
            // 닫힌 LineString인 경우
            sIsTermSegBegin = ID_FALSE;
            sIsTermSegEnd = ID_FALSE;
        }
        
        //--------------------------------
        // Polygon을 구성하는 각 Ring 단위로 검사
        //--------------------------------

        // 다음 Ring을 구할때는 이전 Ring의 마지막 점으로부터 구한다.
        for( j = 0, sRing = STD_FIRST_RN2D(aObj2);
             j < sRingCnt;
             j++, sRing = (stdLinearRing2D*) STD_NEXT_PT2D(sPt2) )
        {
            //--------------------------------
            // Ring을 구성하는 Line Segment 단위로 검사
            //--------------------------------

            sRingLSegCnt = STD_N_POINTS(sRing) - 1;
            
            for( k = 0, sPt2 = STD_FIRST_PT2D(sRing);
                 k < sRingLSegCnt;
                 k++, sPt2 = STD_NEXT_PT2D(sPt2) )
            {
                //------------------------------------
                // LineString의 Segment와 Ring의 Segment간의 관계 획득
                //------------------------------------

                // Ring을 구성하는 Line Segment는 종단점이 없음
                sTemp = compLineSegment( sPt1,
                                         STD_NEXT_PT2D(sPt1),
                                         sIsTermSegBegin,
                                         sIsTermSegEnd,
                                         sPt2,
                                         STD_NEXT_PT2D(sPt2),
                                         ID_FALSE,
                                         ID_FALSE );

                //--------------------------
                // 관계 결과 누적
                //--------------------------
                
                if ( sTemp == 'F' )
                {
                    // 누적할 필요 없음
                }
                else
                {
                    IDE_DASSERT( (sTemp == '0') || (sTemp == '1') );
                    
                    sResult = sTemp;
                    
                    // 선분이 교차하는 경우가 최대 관계임
                    IDE_TEST_RAISE( sResult == '1', SLISAB2D_MAX_RESULT );
                }
                
            }
        }
    }

    IDE_EXCEPTION_CONT(SLISAB2D_MAX_RESULT);

    return sResult;
}

/***********************************************************************
 * Description:
 *
 *    SLI(Single LineString Internal)과
 *    SAE(Single Area External)의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *   LineString을 구성하는 [점, 선분] 과
 *   Polygon을 구성하는 [점, 선분, 면]의 관계를 통해 구해낸다.
 *
 *   ================================
 *     LineString .vs. Polygon
 *   ================================
 *
 *   1. 선분 .vs. 면
 *
 *       - 이 함수의 목적으로 다른 관계로부터 유추
 *
 *   2. 점 .vs. 면 
 *       - 점이 내부에 존재 : 유추 불가
 *       - 점이 외부에 존재 : TRUE
 *
 *           L    A--------A
 *                |        |
 *                |        |
 *                A--------A
 *                 
 *   3. 선분 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 선분이 선분으로 교차 : 점과 점의 관계로부터 유추 가능
 *
 *             L---A====L----A
 *
 *             L---A====A----L
 *
 *             A---L====L----A
 *
 *       - 선분이 점에서 교차 : TRUE
 *
 *                L
 *                |
 *            A---+----A
 *                |
 *                L
 *
 *   4. 점 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Lp                     Lp
 *               |                      |
 *         A-----L------A  ==>   Ap----AL----An  
 *               |                      |
 *               Ln                     Ln
 *
 *   5. 선분 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Ap                    Ap
 *               |                     |
 *         L-----A-----L  ==>   Lp----LA----Ln  
 *               |                     |
 *               An                    An
 *
 *   6. 점 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점에서 교차 : 점과 점의 관계로부터 유추
 *
 *                Lp
 *                |
 *         Ap----AL-----An
 *                |
 *                Ln
 *
 *   BUG-16952 기존 로직을 다음과 같이 수정함
 *      1) Line의 점과 Area의 선분이 교차 로직 ==> 점과 점의 교차 알고리즘으로 수정
 *      2) Line의 선분과 Area의 점이 교차 로직 ==> 점과 점의 교차 알고리즘으로 수정
 *      3) 점과 점의 교차 로직 ==> 수평에 대한 검출이 되도록 확장
 *
 *   기존의 점과 선분에 관련된 개념을 여기에 기술함
 *
 *   --------------------------------------------
 *   기존 로직) LineString의 선분과 Polygon 점의 관계
 *   --------------------------------------------
 *
 *   LineString의 선분내에 Polygon의 점이 존재하는 경우
 *
 *        ----A----
 *
 *        if Area(Ap, A, Lp) * Area(A, An, Ln) < 0,
 *        ; TRUE(SLI 와 SAE 가 교차함)
 *
 *          Ap              
 *         / \             
 *        Lp--A----Ln        
 *             \  /         
 *              An
 *
 *        ; 다음과 같은 예가 검출됨
 *
 *            A------Ap
 *            |   ** |
 *            |  Lp--A--LnA----A
 *            |      |**  |    |
 *            |      An---A    |
 *            |                |
 *            A----------------A
 *
 *        if Area(Ap, A, Lp) * Area(A, An, Ln) > 0,
 *        ; 점과 면의 관계에서 유추
 *
 *          Ap  An            
 *         / \ /  \          
 *        Lp--A----Ln
 *
 *        if Area(Ap, A, Lp) * Area(A, An, Ln) == 0,
 *        ; 선분과 선분이 선분에서 교차하는 관계에서 유추 (BUG-16952)
 *
 *          Ap
 *         /  \
 *        Lp---A----Ln--An
 *
 *   ------------------------------------------------
 *   기존 로직) LineString의 점과 Polygon 내부 선분의 관계
 *   ------------------------------------------------
 *
 *   Polygon 의 선분내에 LineString의 점이 존재하는 경우
 *
 *        ----L----
 *
 *        if L의 이전 또는 다음 점 Lp가 영역의 외부를 향하고 있다면,
 *        ; TRUE(SLI 와 SAE 가 교차함)
 *
 *        A         A
 *        |  Area   |
 *        |         |
 *        A----L----A
 *             |
 *             Lp
 *
 *        Area Segment상에 Line String의 한 점이 존재할 경우
 *        해당점의 이전 이후점이 Ring의 원생성 방향의 반대편에 존재하는 지 검사
 *
 *       외부링인 경우 
 *          시계 방향일 경우 왼쪽에 존재하면 SLI가 외부에 존재
 *          시계 반대방향일 경우 오른쪽에 존재하면 SLI가 외부에 존재
 *       내부링인 경우
 *          시계 방향일 경우 오른쪽에 존재하면 SLI가 외부에 존재
 *          시계 반대방향일 경우 왼쪽에 존재하면 SLI가 외부에 존재
 *
 *       다음과 같은 예제를 검출해냄.
 *
 *       A----->>------A                 A----------A
 *       |             |                 |          |
 *       |  A--L--<<---A                 |  I->>-I  |
 *       |  |  |                         |  |    |  |
 *       |  A--L-->>---A                 |  L----L  |
 *       |             |                 |  |    |  |
 *       A------<<-----A                 |  I-<<-I  |
 *                                       |          |
 *                                       A----------A
 *
 *
 *        if L의 이전 또는 다음 점 Lp가 영역의 내부를 향하고 있다면,
 *        ; 점과 면의 관계로부터 유추됨
 *
 *        A       Lp   A
 *        |  Area |    |
 *        |       |    |
 *        A-------L----A
 *
 *        if L의 이전 또는 다음 점 Lp가 영역의 경계와 일치한다면,
 *        ; 선분과 선분이 선분에서 교차하는 관계로부터 유추됨(BUG-16952)
 *
 *        A           A
 *        |   Area    |
 *        |           |
 *        A----L-Lp---A
 *
 ***********************************************************************/

SChar
stfRelation::sliTosae( const stdLineString2DType * aLineObj,
                       const stdPolygon2DType    * aAreaObj )
{
    UInt i, j, k;
    idBool  sMeetOnPoint;
    SChar   sResult;
    
    //----------------------------
    // Line String 정보
    //----------------------------
    
    stdPoint2D      * sLinePt;
    stdPoint2D      * sLinePrevPt;
    stdPoint2D      * sLineNextPt;
    UInt              sLineSegCnt;
    UInt              sLinePtCnt;
    
    //----------------------------
    // Ring 정보
    //----------------------------
    
    stdLinearRing2D * sRing;
    stdPoint2D      * sRingPt;
    stdPoint2D      * sRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * sRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sRingCnt;     // Ring Count of a Polygon
    UInt              sRingSegCnt;  // Segment Count of a Ring
    idBool            sRingCCWise;  // Ring 이 시계 역방향인지의 여부

    //----------------------------------------
    // Parameter Validation
    //----------------------------------------

    IDE_DASSERT( aLineObj != NULL );
    IDE_DASSERT( aAreaObj != NULL );
        
    //----------------------------------------
    // Initialization
    //----------------------------------------
    
    sResult = 'F';
    
    sLineSegCnt = STD_N_POINTS(aLineObj) - 1;
    sLinePtCnt = STD_N_POINTS(aLineObj);

    sRingCnt = STD_N_RINGS(aAreaObj);
    
    //----------------------------------------
    // LineString의 점과 Polygon의 내부 면의 관계
    //----------------------------------------

    for ( i = 0, sLinePt = STD_FIRST_PT2D(aLineObj);
          i < sLinePtCnt;
          i++, sLinePt = STD_NEXT_PT2D(sLinePt) )
    {
        // LineString의 한점이 Polygon의 외부에 존재하는지 판단
        if( spiTosae( sLinePt, aAreaObj ) == '0' )
        {
            sResult = '1';
            IDE_RAISE( SLISAE2D_MAX_RESULT );
        }
        else
        {
            // Nothing To Do
        }
    }

    //----------------------------------------
    // LineString의 점, 선분과 Polygon의 점, 선분의 관계
    //----------------------------------------

    for ( i = 0, sLinePt = STD_FIRST_PT2D(aLineObj), sLinePrevPt = NULL;
          i < sLinePtCnt;
          i++, sLinePrevPt = sLinePt, sLinePt = STD_NEXT_PT2D(sLinePt) )
    {
        // Ring의 개수만큼 반복
        for ( j = 0, sRing = STD_FIRST_RN2D(aAreaObj);
              j < sRingCnt;
              j++, sRing = STD_NEXT_RN2D(sRing) )
        {
            sRingSegCnt = STD_N_POINTS(sRing) - 1;
            sRingCCWise = stdUtils::isCCW2D(sRing);

            // Ring을 구성하는 Segment 개수만큼 반복
            for ( k = 0, sRingPt = STD_FIRST_PT2D(sRing);
                  k < sRingSegCnt;
                  k++, sRingPt = STD_NEXT_PT2D(sRingPt) )
            {
                //----------------------------------------
                // LineString 내부 선분과 Polygon 내부 선분의 관계
                //----------------------------------------
                
                // 선분의 내부가 교차하는지 검사
                if( ( i < sLineSegCnt ) &&
                    ( stdUtils::intersectI2D( sLinePt,
                                              STD_NEXT_PT2D(sLinePt),
                                              sRingPt,
                                              STD_NEXT_PT2D(sRingPt) )
                      ==ID_TRUE ) )
                {
                    // 선분이 교차하는 경우
                    sResult = '1';
                    IDE_RAISE( SLISAE2D_MAX_RESULT );
                }
                else
                {
                    // 선분이 교차하지 않는 경우
                    
                }

                //----------------------------------------
                // 다음 관계를 점과 점의 관계로 만들기 위한 조정
                //  - LineString 내부 선분과 Ring의 점과의 관계
                //  - Ring의 내부 선분과 LineString의 점과의 관계
                //  - LineString의 점과 Ring의 점과의 관계
                //----------------------------------------

                sMeetOnPoint = ID_FALSE;

                //----------------------------
                // LineString의 현재점, 이전점, 이후점을 구함
                //----------------------------
                
                // sLinePrevPt :  for loop을 통해 추출
                if ( i == sLineSegCnt )
                {
                    sLineNextPt = NULL;
                }
                else
                {
                    sLineNextPt = STD_NEXT_PT2D(sLinePt);
                }
                
                //----------------------------
                // Ring의 현재점, 이전점, 이후점을 구함
                //----------------------------
                
                sRingPrevPt = stdUtils::findPrevPointInRing2D( sRingPt,
                                                               k,
                                                               sRingSegCnt,
                                                               NULL );
                sRingCurrPt = sRingPt;
                sRingNextPt = stdUtils::findNextPointInRing2D( sRingPt,
                                                               k,
                                                               sRingSegCnt,
                                                               NULL );
                
                //----------------------------
                // 선분내에 Ring의 점이 존재하는지 검사
                //----------------------------
                
                if ( ( i < sLineSegCnt ) &&
                     ( stdUtils::betweenI2D( sLinePt,
                                             STD_NEXT_PT2D(sLinePt),
                                             sRingPt )==ID_TRUE ) )
                {
                    // 점이 선분에서 교차 => 점과 점의 관계로 변경
                    //               Ap                    Ap
                    //               |                     |
                    //         L-----A-----L  ==>   Lp----LA----Ln  
                    //               |                     |
                    //               An                    An
                    sMeetOnPoint = ID_TRUE;
                    
                    sLinePrevPt = sLinePt;
                    sLineNextPt = STD_NEXT_PT2D(sLinePt);
                }
                
                //----------------------------
                // Ring 선분내에 LineString의 점이 존재하는지 검사
                //----------------------------

                if ( stdUtils::betweenI2D( sRingPt,
                                           STD_NEXT_PT2D(sRingPt),
                                           sLinePt ) == ID_TRUE )
                {
                    // 점이 선분에서 교차 => 점과 점의 관계로 변경
                    //               Lp                     Lp
                    //               |                      |
                    //         A-----L------A  ==>   Ap----AL----An  
                    //               |                      |
                    //               Ln                     Ln
                    sMeetOnPoint = ID_TRUE;
                        
                    sRingPrevPt = sRingPt;
                    sRingCurrPt = sLinePt;
                    sRingNextPt = stdUtils::findNextPointInRing2D( sRingPt,
                                                                   k,
                                                                   sRingSegCnt,
                                                                   NULL );
                }

                //----------------------------
                // 점과 점이 교차하는 지 검사
                //----------------------------
                
                if ( stdUtils::isSamePoints2D( sLinePt, sRingPt ) == ID_TRUE )
                {
                    sMeetOnPoint = ID_TRUE;

                    // 이미 구해진 값을 사용
                }

                //----------------------------------------
                // 점과 점의 관계로부터 외부 교차의 판단
                //----------------------------------------
                
                if ( sMeetOnPoint == ID_TRUE )
                {
                    if ( hasRelLineSegRingSeg(
                             (j == 0 ) ? ID_TRUE : ID_FALSE, // Exterior Ring
                             sRingCCWise,
                             sRingPrevPt,
                             sRingNextPt,
                             sRingCurrPt,
                             sLinePrevPt,
                             sLineNextPt,
                             STF_OUTSIDE_ANGLE_POS ) == ID_TRUE )
                    {
                        sResult = '1';
                        IDE_RAISE( SLISAE2D_MAX_RESULT );
                    }
                    else
                    {
                        // 교차 여부를 판단할 수 없음
                    }
                }
                else // sMeetOnPoint == ID_FALSE
                {
                    // 검사 대상이 아님
                }
                
            } // for k
        } // for j
    } // for i

    IDE_EXCEPTION_CONT(SLISAE2D_MAX_RESULT);
    
    return sResult;
}

SChar stfRelation::slbTosai( const stdLineString2DType*         aObj1,
                             const stdPolygon2DType*            aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTosai( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTosai( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }

    return 'F';
}

SChar stfRelation::slbTosab( const stdLineString2DType*         aObj1,
                             const stdPolygon2DType*            aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTosab( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTosab( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }

    return 'F';
}

SChar stfRelation::slbTosae( const stdLineString2DType*         aObj1,
                             const stdPolygon2DType*            aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTosae( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTosae( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }

    return 'F';
}

SChar stfRelation::sleTosab( const stdLineString2DType*         aObj1,
                             const stdPolygon2DType*            aObj2 )
{
    if( STD_N_RINGS(aObj2) > 1 )
    {
        return '1';
    }

    stdLinearRing2D*    sRing = STD_FIRST_RN2D(aObj2);
    stdPoint2D*         sPt2 = STD_FIRST_PT2D(sRing);
    UInt                i, sMax;

    // Fix BUG-15516
    sMax = STD_N_POINTS(sRing)-1;
    for( i = 0; i < sMax; i++ )
    {
        if( lineInLineString( 
            STD_NEXTN_PT2D(sPt2,i), 
            STD_NEXTN_PT2D(sPt2,i+1), 
            aObj1 ) == ID_FALSE )
        {
            return '1';
        }
    }

    return 'F';
}
// linestring vs multipoint
SChar stfRelation::sliTompi( const stdLineString2DType*         aObj1,
                             const stdMultiPoint2DType*         aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;
    
    sPoint = STD_FIRST_POINT2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        if( spiTosli(&sPoint->mPoint, aObj1) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::slbTompi( const stdLineString2DType*         aObj1,
                             const stdMultiPoint2DType*         aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTompi( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTompi( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::slbTompe( const stdLineString2DType*         aObj1,
                             const stdMultiPoint2DType*         aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTompe( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTompe( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::sleTompi( const stdLineString2DType*         aObj1,
                             const stdMultiPoint2DType*         aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;
    
    sPoint = STD_FIRST_POINT2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        if( spiTosle(&sPoint->mPoint, aObj1) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

// linestring vs multilinestring
SChar stfRelation::sliTomli( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTosli(aObj1, sLine);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sliTomlb( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    stdPoint2D*             sPtS, *sPtE;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        sPtS = STD_FIRST_PT2D(sLine);
        sPtE = STD_LAST_PT2D(sLine);

        if( spiTosli( sPtS, aObj1 ) == '0' )
        {
            return '0';
        }
        if( spiTosli( sPtE, aObj1 ) == '0' )
        {
            return '0';
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::sliTomle( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdPoint2D*             sPt1;
    stdLineString2DType*    sLine2;
    UInt                    i, j, sMax1, sMax2;
    idBool                  sFound;

    // Fix BUG-15516
    sMax1 = STD_N_POINTS(aObj1)-1;
    sMax2 = STD_N_OBJECTS(aObj2);

    sPt1 = STD_FIRST_PT2D(aObj1);
    for( i = 0; i < sMax1; i++ )
    {
        sFound = ID_FALSE;
        sLine2 = STD_FIRST_LINE2D(aObj2);
        for( j = 0; j < sMax2; j++ )
        {
            if( lineInLineString( 
                STD_NEXTN_PT2D(sPt1,i), 
                STD_NEXTN_PT2D(sPt1,i+1), 
                sLine2 ) == ID_TRUE )
            {
                sFound = ID_TRUE;
                break;
            }
            sLine2 = STD_NEXT_LINE2D(sLine2);
        }

        if( sFound == ID_FALSE )
        {
            return '1';
        }
    }

    return 'F';
}

SChar stfRelation::slbTomli( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTomli( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTomli( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::slbTomlb( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }
    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    if( spiTomlb( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTomlb( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::slbTomle( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTomle( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTomle( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::sleTomli( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdPoint2D*             sPt2;
    stdLineString2DType*    sLine2;
    UInt                    i, j, sMaxObj, sMaxPt;

    // Fix BUG-15516
    sLine2 = STD_FIRST_LINE2D(aObj2);
    sMaxObj = STD_N_OBJECTS(aObj2);
    for( i = 0; i < sMaxObj; i++ )
    {
        sPt2 = STD_FIRST_PT2D(sLine2);
        sMaxPt = STD_N_POINTS(sLine2)-1;  // Fix BUG-15518
        for( j = 0; j < sMaxPt; j++ )   // Fix BUG-16227
        {
            if( lineInLineString( 
                STD_NEXTN_PT2D(sPt2,j), 
                STD_NEXTN_PT2D(sPt2,j+1),
                aObj1 ) == ID_FALSE )
            {
                return '1';
            }
        }
    }

    return 'F';
}

SChar stfRelation::sleTomlb( const stdLineString2DType*         aObj1,
                             const stdMultiLineString2DType*    aObj2 )
{
    stdLineString2DType*    sLine;
    stdPoint2D*             sPtS, *sPtE;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        sPtS = STD_FIRST_PT2D(sLine);
        sPtE = STD_LAST_PT2D(sLine);

        if( spiTosle( sPtS, aObj1 ) == '0' )
        {
            return '0';
        }
        if( spiTosle( sPtE, aObj1 ) == '0' )
        {
            return '0';
        }

        sLine = (stdLineString2DType*)STD_NEXT_PT2D(sPtE);
    }
    return 'F';
}

// linestring vs multipolygon
SChar stfRelation::sliTomai( const stdLineString2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);    
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTosai(aObj1, sPoly);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sliTomab( const stdLineString2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);    
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTosab(aObj1, sPoly);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

/***********************************************************************
 *
 * Description :
 *
 *    단일 라인 객체의 내부 영역과 다중 영역 객체의 외부 영역과의 관계를 구함.
 *    sli(single line internal), mae(multi area external)
 *
 * Implementation :
 *
 *    표기법 : Ai (객체 A의 interior 영역)
 *
 *    단일 객체 내부 영역과 다중 객체 외부 영역과의 관계는 다음과 같은 식으로 표현
 *
 *    Si ^ ( A U B U ...U N )e
 *    <==>
 *    Si ^ ( Ae ^ Be ^ ... Ne )
 *
 ***********************************************************************/

SChar stfRelation::sliTomae( const stdLineString2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet;
    SChar               sResult;

    //--------------------------------------
    // Initialization
    //--------------------------------------

    sResult = '1';
    
    //--------------------------------------
    // 모든 외부 영역과 교집합이 있는지 검사
    //--------------------------------------
    
    sPoly = STD_FIRST_POLY2D(aObj2);    
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTosae(aObj1, sPoly);

        if( sRet != '1' )
        {
            // 교집합이 없음
            sResult = 'F';
            break;
        }
        else
        {
            // 교집합이 존재함
        }
        
        sPoly = STD_NEXT_POLY2D(sPoly);
    }

    return sResult;
}


SChar stfRelation::slbTomai( const stdLineString2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTomai( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTomai( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::slbTomab( const stdLineString2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTomab( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTomab( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::slbTomae( const stdLineString2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPoint2D* sPtS = STD_FIRST_PT2D(aObj1);
    stdPoint2D* sPtE = STD_LAST_PT2D(aObj1);

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    if( spiTomae( sPtS, aObj2 ) == '0' )
    {
        return '0';
    }
    if( spiTomae( sPtE, aObj2 ) == '0' )
    {
        return '0';
    }
    return 'F';
}

SChar stfRelation::sleTomab( const stdLineString2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);    
    sMax = STD_N_OBJECTS(aObj2);    
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sleTosab(aObj1, sPoly);

        if( sRet == '1' )
        {
            return '1';
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

// linestring vs geometrycollection
SChar stfRelation::sliTogci( const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTosli( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTosli( aObj1, &sGeom->linestring2D );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sliTosai( aObj1, &sGeom->polygon2D );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = sliTompi( aObj1, &sGeom->mpoint2D );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sliTomli( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = sliTomai( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sliTogcb (const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;
    

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliToslb( aObj1, &sGeom->linestring2D );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sliTosab( aObj1, &sGeom->polygon2D );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sliTomlb( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = sliTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sliTogce( const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTosle( aObj1, &sGeom->linestring2D );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sliTosae( aObj1, &sGeom->polygon2D );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sliTomle( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = sliTomae( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::slbTogci( const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    stdPoint2D*         sPtS;
    stdPoint2D*         sPtE;
    UInt                i, sMax;
    SChar               sRet = 0x00;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sPtS = STD_FIRST_PT2D(aObj1);
    sPtE = STD_LAST_PT2D(aObj1);
    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTospi( sPtS, &sGeom->point2D.mPoint );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTospi( sPtE, &sGeom->point2D.mPoint );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = spiTosli( sPtS, &sGeom->linestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTosli( sPtE, &sGeom->linestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = spiTosai( sPtS, &sGeom->polygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTosai( sPtE, &sGeom->polygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = spiTompi( sPtS, &sGeom->mpoint2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTompi( sPtE, &sGeom->mpoint2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = spiTomli( sPtS, &sGeom->mlinestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTomli( sPtE, &sGeom->mlinestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = spiTomai( sPtS, &sGeom->mpolygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTomai( sPtE, &sGeom->mpolygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        default:
            return 'F';
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::slbTogcb( const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    stdPoint2D*         sPtS;
    stdPoint2D*         sPtE;
    UInt                i, sMax;
    SChar               sRet = 0x00;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sPtS = STD_FIRST_PT2D(aObj1);
    sPtE = STD_LAST_PT2D(aObj1);
    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = spiToslb( sPtS, &sGeom->linestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiToslb( sPtE, &sGeom->linestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = spiTosab( sPtS, &sGeom->polygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTosab( sPtE, &sGeom->polygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = spiTomlb( sPtS, &sGeom->mlinestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTomlb( sPtE, &sGeom->mlinestring2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = spiTomab( sPtS, &sGeom->mpolygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            sRet = spiTomab( sPtE, &sGeom->mpolygon2D );
            if( sRet == '0' )
            {
                return '0';
            }
            break;
        default:
            return 'F';
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::slbTogce( const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    stdPoint2D*         sPtS;
    stdPoint2D*         sPtE;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sPtS = STD_FIRST_PT2D(aObj1);
    sPtE = STD_LAST_PT2D(aObj1);
    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            if( (spiTospe( sPtS, &sGeom->point2D.mPoint ) == '0') ||
                (spiTospe( sPtE, &sGeom->point2D.mPoint ) == '0') )
            {
                sRet = '0';
            }
            else
            {
                sRet = 'F';
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            if( (spiTosle( sPtS, &sGeom->linestring2D ) == '0') ||
                (spiTosle( sPtE, &sGeom->linestring2D ) == '0') )
            {
                sRet = '0';
            }
            else
            {
                sRet = 'F';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            if( (spiTosae( sPtS, &sGeom->polygon2D ) == '0') ||
                (spiTosae( sPtE, &sGeom->polygon2D ) == '0') )
            {
                sRet = '0';
            }
            else
            {
                sRet = 'F';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            if( (spiTompe( sPtS, &sGeom->mpoint2D ) == '0') ||
                (spiTompe( sPtE, &sGeom->mpoint2D ) == '0') )
            {
                sRet = '0';
            }
            else
            {
                sRet = 'F';
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            if( (spiTomle( sPtS, &sGeom->mlinestring2D ) == '0') ||
                (spiTomle( sPtE, &sGeom->mlinestring2D ) == '0') )
            {
                sRet = '0';
            }
            else
            {
                sRet = 'F';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            if( (spiTomae( sPtS, &sGeom->mpolygon2D ) == '0') ||
                (spiTomae( sPtE, &sGeom->mpolygon2D ) == '0') )
            {
                sRet = '0';
            }
            else
            {
                sRet = 'F';
            }
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sleTogci( const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTosle( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTosle( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            return '2';
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = sleTompi( aObj1, &sGeom->mpoint2D );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sleTomli( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            return '2';
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sleTogcb( const stdLineString2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTosle( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sleTosab( aObj1, &sGeom->polygon2D );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = sleTompi( aObj1, &sGeom->mpoint2D );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sleTomli( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = sleTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

/***********************************************************************
 * Description:
 *
 *    SAI(Single Area Internal)과
 *    SAI(Single Area Internal)의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *   BUG-17003
 *
 *   Polygon을 구성하는 [점, 선분, 면] 과
 *   Polygon을 구성하는 [점, 선분, 면] 의 관계를 통해 구해낸다.
 *
 *   ================================
 *     Polygon(A) .vs. Polygon(B)
 *   ================================
 *
 *   1. 면 .vs. 면
 *
 *       - 면과 면이 교차하는 지의 여부 : 다른 관계로부터 유추
 *       - 한면과 다른 면을 완전히 포함하는 경우 : TRUE
 *
 *          A-------A
 *          |       |
 *          | B---B |
 *          | |   | |
 *          | B---B |
 *          |       |
 *          A-------A
 *
 *   1. 선분 .vs. 면
 *
 *       - 선분과 선분, 점과 점의 관계로부터 유추
 *
 *   2. 점 .vs. 면 
 *       - 점이 다른 Polygon의 외부에 존재 : 유추 불가
 *       - 점이 다른 Polygon의 내부에 존재 : TRUE
 *
 *                A--------A
 *                |        |
 *                |   B    |
 *                |        |
 *                A--------A
 *                 
 *   3. 선분 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *
 *       - 선분이 선분으로 교차 : 점과 점의 관계로부터 유추 가능
 *
 *             B---A====B----A
 *
 *             B---A====A----B
 *
 *             A---B====B----A
 *
 *       - 선분이 점에서 교차 : TRUE
 *
 *                B
 *                |
 *            A---+----A
 *                |
 *                B
 *
 *          SLI .vs. SAI와 달리 다음과 같은 경우에도 TRUE임.
 *
 *                B
 *                |
 *            A---I----A
 *               /|\
 *              I B I
 *
 *   4. 점 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Bp                     Bp
 *               |                      |
 *         A-----B------A  ==>   Ap----AB----An  
 *               |                      |
 *               Bn                     Bn
 *
 *   5. 점 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점에서 교차 : 점과 점의 관계로부터 유추
 *
 *                Bp
 *                |
 *         Ap----AB-----An
 *                |
 *                Bn
 *
 *   점과 점의 관계에서 다음과 같이 내부에 있다 하더라도
 *   선분이 Interia Ring 때문에 내부영역과 교차함을 보장할 수 없다.
 *   따라서, 구성된 Line이 Interior Ring내부에 포함되지 않음을 검사해야 함.
 *
 *      Ap  
 *       \     B
 *        \
 *         \   
 *          A------An              
 *
 *      Ap     
 *       \   I B B    I
 *        \  |     . ^ 
 *         \ | . ^
 *          AB-------An

 ***********************************************************************/

IDE_RC stfRelation::saiTosai( iduMemory *              aQmxMem,
                              const stdPolygon2DType * a1stArea,
                              const stdPolygon2DType * a2ndArea,
                              SChar *                  aResult )
{
    UInt i, j;
    UInt m, n;
    UInt x, y;
    
    idBool  sMeetOnPoint;
    SChar   sResult;
    
    //----------------------------
    // First Area 정보
    //----------------------------
    
    stdLinearRing2D * s1stRing;
    stdPoint2D      * s1stRingPt;
    stdPoint2D      * s1stRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * s1stRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * s1stRingNextPt;  // Ring Point의 다음 Point
    
    UInt              s1stRingCnt;     // Ring Count of a Polygon
    UInt              s1stRingSegCnt;  // Segment Count of a Ring
    idBool            s1stRingCCWise;  // Ring 이 시계 역방향인지의 여부

    stdPoint2D        s1stSomePt;
    
    //----------------------------
    // Second Area 정보
    //----------------------------
    
    stdLinearRing2D * s2ndRing;
    stdPoint2D      * s2ndRingPt;
    stdPoint2D      * s2ndRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * s2ndRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * s2ndRingNextPt;  // Ring Point의 다음 Point
    
    UInt              s2ndRingCnt;     // Ring Count of a Polygon
    UInt              s2ndRingSegCnt;  // Segment Count of a Ring

    stdPoint2D        s2ndSomePt;

    //----------------------------
    // Ring과 Ring의 중복점인지 검사
    //----------------------------
    
    UInt              sCheckSegCnt;
    stdLinearRing2D * sCheckRing;
    stdPoint2D      * sCheckPt;
    
    //----------------------------------------
    // Parameter Validation
    //----------------------------------------

    IDE_DASSERT( a1stArea != NULL );
    IDE_DASSERT( a2ndArea != NULL );
        
    //----------------------------------------
    // Initialization
    //----------------------------------------
    
    sResult = 'F';
    
    s1stRingCnt = STD_N_RINGS( a1stArea );
    s2ndRingCnt = STD_N_RINGS( a2ndArea );

    //----------------------------------------
    // 면과 면의 관계로부터 추출
    //----------------------------------------

    IDE_TEST( stdUtils::getPointOnSurface2D( aQmxMem, a1stArea, & s1stSomePt )
              != IDE_SUCCESS );
    IDE_TEST( stdUtils::getPointOnSurface2D( aQmxMem, a2ndArea, & s2ndSomePt )
              != IDE_SUCCESS );
    
    if( ( spiTosai( &s1stSomePt, a2ndArea ) == '0' ) ||
        ( spiTosai( &s2ndSomePt, a1stArea ) == '0' ) )
    {
        sResult = '2';
        IDE_RAISE( SAISAI2D_MAX_RESULT );
    }
    
    //----------------------------------------
    // 점과 면의 관계로부터 추출
    //----------------------------------------

    // First Area의 좌표가 Second Area의 내부에 있는지 검사
    for ( i = 0, s1stRing = STD_FIRST_RN2D(a1stArea);
          i < s1stRingCnt;
          i++, s1stRing = STD_NEXT_RN2D(s1stRing) )
    {
        s1stRingSegCnt = STD_N_POINTS(s1stRing) - 1;
        
        for ( j = 0, s1stRingPt = STD_FIRST_PT2D(s1stRing);
              j < s1stRingSegCnt;
              j++, s1stRingPt = STD_NEXT_PT2D(s1stRingPt) )
        {
            // 한점이 다른 Polygon의 내부에 존재하는지 판단
            if( spiTosai( s1stRingPt, a2ndArea ) == '0' )
            {
                sResult = '2';
                IDE_RAISE( SAISAI2D_MAX_RESULT );
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    
    // Second Area의 좌표가 First Area의 내부에 있는지 검사
    for ( m = 0, s2ndRing = STD_FIRST_RN2D(a2ndArea);
          m < s2ndRingCnt;
          m++, s2ndRing = STD_NEXT_RN2D(s2ndRing) )
    {
        s2ndRingSegCnt = STD_N_POINTS(s2ndRing) - 1;
        
        for ( n = 0, s2ndRingPt = STD_FIRST_PT2D(s2ndRing);
              n < s2ndRingSegCnt;
              n++, s2ndRingPt = STD_NEXT_PT2D(s2ndRingPt) )
        {
            // 한점이 다른 Polygon의 내부에 존재하는지 판단
            if( spiTosai( s2ndRingPt, a1stArea ) == '0' )
            {
                sResult = '2';
                IDE_RAISE( SAISAI2D_MAX_RESULT );
            }
            else
            {
                // Nothing To Do
            }
        }
    }       
    
    //----------------------------------------
    // First Area와 Second Area 의 관계 추출
    //----------------------------------------

    // First Area가 구성하는 Ring의 개수만큼 반복
    for ( i = 0, s1stRing = STD_FIRST_RN2D(a1stArea);
          i < s1stRingCnt;
          i++, s1stRing = STD_NEXT_RN2D(s1stRing) )
    {
        s1stRingSegCnt = STD_N_POINTS(s1stRing) - 1;
        s1stRingCCWise = stdUtils::isCCW2D(s1stRing);
        
        // First Area의 Ring이 구성하는 Segment개수만큼 반복
        for ( j = 0, s1stRingPt = STD_FIRST_PT2D(s1stRing);
              j < s1stRingSegCnt;
              j++, s1stRingPt = STD_NEXT_PT2D(s1stRingPt) )
        {
            // Second Area가 구성하는 Ring의 개수만큼 반복
            for ( m = 0, s2ndRing = STD_FIRST_RN2D(a2ndArea);
                  m < s2ndRingCnt;
                  m++, s2ndRing = STD_NEXT_RN2D(s2ndRing) )
            {
                s2ndRingSegCnt = STD_N_POINTS(s2ndRing) - 1;
                
                // Second Area의 Ring이 구성하는 Segment개수만큼 반복
                for ( n = 0, s2ndRingPt = STD_FIRST_PT2D(s2ndRing);
                      n < s2ndRingSegCnt;
                      n++, s2ndRingPt = STD_NEXT_PT2D(s2ndRingPt) )
                {
                    //----------------------------
                    // First Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------

                    s1stRingPrevPt =
                        stdUtils::findPrevPointInRing2D( s1stRingPt,
                                                         j,
                                                         s1stRingSegCnt,
                                                         NULL );
                    s1stRingCurrPt = s1stRingPt;
                    s1stRingNextPt =
                        stdUtils::findNextPointInRing2D( s1stRingPt,
                                                         j,
                                                         s1stRingSegCnt,
                                                         NULL );
                    
                    //----------------------------
                    // Second Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------
                
                    s2ndRingPrevPt =
                        stdUtils::findPrevPointInRing2D( s2ndRingPt,
                                                         n,
                                                         s2ndRingSegCnt,
                                                         NULL );
                    s2ndRingCurrPt = s2ndRingPt;
                    s2ndRingNextPt =
                        stdUtils::findNextPointInRing2D( s2ndRingPt,
                                                         n,
                                                         s2ndRingSegCnt,
                                                         NULL );

                    //------------------------------------
                    // 선분과 선분의 관계로부터 유추
                    //------------------------------------

                    // 선분이 점에서 교차한다면 TRUE
                    if( ( stdUtils::intersectI2D( s1stRingPt,
                                                  STD_NEXT_PT2D(s1stRingPt),
                                                  s2ndRingPt,
                                                  STD_NEXT_PT2D(s2ndRingPt) )
                          ==ID_TRUE ) )
                    {
                        // 선분이 교차하는 경우
                        sResult = '2';
                        IDE_RAISE( SAISAI2D_MAX_RESULT );
                    }
                    else
                    {
                        // 선분이 교차하지 않는 경우
                    }
                    
                    //----------------------------------------
                    // 다음 관계를 점과 점의 관계로 만들기 위한 조정
                    //  - Ring의 선분과 점과의 관계
                    //  - Ring의 점과 점의 관계
                    //----------------------------------------

                    sMeetOnPoint = ID_FALSE;
                    
                    //----------------------------
                    // First Ring 선분내에 Second Ring의 점이 존재하는지 검사
                    //----------------------------
                    
                    if ( ( stdUtils::betweenI2D( s1stRingPt,
                                                 STD_NEXT_PT2D(s1stRingPt),
                                                 s2ndRingPt )==ID_TRUE ) )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Bp                    Bp
                        //               |                     |
                        //         A-----B-----A  ==>   Ap----AB----An  
                        //               |                     |
                        //               Bn                    Bn
                        sMeetOnPoint = ID_TRUE;
                        
                        s1stRingPrevPt = s1stRingPt;
                        s1stRingCurrPt = s2ndRingPt;
                        s1stRingNextPt = STD_NEXT_PT2D(s1stRingPt);
                    }
                
                    //----------------------------
                    // Second Ring 선분내에 First Ring의 점이 존재하는지 검사
                    //----------------------------

                    if ( stdUtils::betweenI2D( s2ndRingPt,
                                               STD_NEXT_PT2D(s2ndRingPt),
                                               s1stRingPt ) == ID_TRUE )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Ap                     Ap
                        //               |                      |
                        //         B-----A------B  ==>   Bp----BA----Bn  
                        //               |                      |
                        //               An                     An
                        sMeetOnPoint = ID_TRUE;
                        
                        s2ndRingPrevPt = s2ndRingPt;
                        s2ndRingCurrPt = s1stRingPt;
                        s2ndRingNextPt = STD_NEXT_PT2D(s2ndRingPt);
                    }

                    //----------------------------
                    // 점과 점이 교차하는 지 검사
                    //----------------------------
                    
                    if ( stdUtils::isSamePoints2D( s1stRingPt,
                                                   s2ndRingPt ) == ID_TRUE )
                    {
                        sMeetOnPoint = ID_TRUE;
                        
                        // 이미 구해진 값을 사용
                    }

                    //----------------------------
                    // Ring과 Ring이 겹치는 점인지 여부를 검사
                    //----------------------------
                    
                    if ( sMeetOnPoint == ID_TRUE )
                    {
                        // Ring과 Ring이 겹치는 점일 경우
                        // 내부에 존재하는 지를 판단할 수 없다.
                        // 다른 점에 의하여 판별 가능하다.
                        //
                        //              Pn
                        //       Ap       I
                        //        \   I  /  Pn
                        //         \  | /
                        //          \ |/
                        //           AIP-------An

                        // First Area의 점이 다른 Ring과 겹치는지 검사
                        for ( x = 0, sCheckRing = STD_FIRST_RN2D(a1stArea);
                              x < s1stRingCnt;
                              x++, sCheckRing = STD_NEXT_RN2D(sCheckRing) )
                        {
                            if ( i == x )
                            {
                                // 자신의 Ring은 검사하지 않음
                                continue;
                            }
                            else
                            {
                                sCheckSegCnt = STD_N_POINTS(sCheckRing) - 1;
                                
                                for ( y=0, sCheckPt=STD_FIRST_PT2D(sCheckRing);
                                      y < sCheckSegCnt;
                                      y++, sCheckPt = STD_NEXT_PT2D(sCheckPt) )
                                {
                                    if ( stdUtils::between2D(
                                             sCheckPt,
                                             STD_NEXT_PT2D(sCheckPt),
                                             s1stRingCurrPt ) == ID_TRUE )
                                    {
                                        sMeetOnPoint = ID_FALSE;
                                        break;
                                    }
                                }
                                
                                if ( sMeetOnPoint != ID_TRUE )
                                {
                                    break;
                                }
                            }
                        }

                        if ( sMeetOnPoint != ID_TRUE )
                        {
                            continue;
                        }
                        
                        // First Area의 점이 다른 Ring과 겹치는지 검사
                        for ( x = 0, sCheckRing = STD_FIRST_RN2D(a2ndArea);
                              x < s2ndRingCnt;
                              x++, sCheckRing = STD_NEXT_RN2D(sCheckRing) )
                        {
                            if ( m == x )
                            {
                                // 자신의 Ring은 검사하지 않음
                                continue;
                            }
                            else
                            {
                                sCheckSegCnt = STD_N_POINTS(sCheckRing) - 1;
                                
                                for ( y=0, sCheckPt=STD_FIRST_PT2D(sCheckRing);
                                      y < sCheckSegCnt;
                                      y++, sCheckPt = STD_NEXT_PT2D(sCheckPt) )
                                {
                                    if ( stdUtils::between2D(
                                             sCheckPt,
                                             STD_NEXT_PT2D(sCheckPt),
                                             s2ndRingCurrPt ) == ID_TRUE )
                                    {
                                        sMeetOnPoint = ID_FALSE;
                                        break;
                                    }
                                }
                                
                                if ( sMeetOnPoint != ID_TRUE )
                                {
                                    break;
                                }
                            }
                        }
                    }
                    
                    //----------------------------------------
                    // 점과 점의 관계로부터 외부 교차의 판단
                    //----------------------------------------
                    
                    if ( sMeetOnPoint == ID_TRUE )
                    {
                        if ( hasRelLineSegRingSeg(
                                 (i == 0 ) ? ID_TRUE : ID_FALSE, // Exterior
                                 s1stRingCCWise,
                                 s1stRingPrevPt,
                                 s1stRingNextPt,
                                 s1stRingCurrPt,
                                 s2ndRingPrevPt,
                                 s2ndRingNextPt,
                                 STF_INSIDE_ANGLE_POS ) == ID_TRUE )
                        {
                            sResult = '2';
                            IDE_RAISE( SAISAI2D_MAX_RESULT );
                        }
                        else
                        {
                            // 교차 여부를 판단할 수 없음
                        }
                    }
                    else // sMeetOnPoint == ID_FALSE
                    {
                        // 검사 대상이 아님
                    }
                } // for n
            } // for m
        } // for j
    } // for i

    IDE_EXCEPTION_CONT(SAISAI2D_MAX_RESULT);

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 *
 *    SAI(Single Area Internal)과
 *    SAB(Single Area Boundary)의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *   BUG-17010
 *
 *   Polygon Boundary를 구성하는 [점, 선분] 과
 *   Polygon Internal을 구성하는 [점, 선분, 면] 의 관계를 통해 구해낸다.
 *
 *   ================================
 *     Boundary .vs. Internal
 *   ================================
 *
 *   1. 선분 .vs. 면
 *
 *       - 선분과 선분, 점과 점의 관계로부터 유추
 *
 *   2. 점 .vs. 면 
 *       - 점이 Polygon의 외부에 존재 : 유추 불가
 *       - 점이 Polygon의 내부에 존재 : TRUE
 *
 *                A--------A
 *                |        |
 *                |   B    |
 *                |        |
 *                A--------A
 *                 
 *   3. 선분 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *
 *       - 선분이 선분으로 교차 : 점과 점의 관계로부터 유추 가능
 *
 *             B---A====B----A
 *
 *             B---A====A----B
 *
 *             A---B====B----A
 *
 *       - 선분이 점에서 교차 : TRUE
 *
 *                B
 *                |
 *            A---+----A
 *                |
 *                B
 *
 *   4. 점 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Bp                     Bp
 *               |                      |
 *         A-----B------A  ==>   Ap----AB----An  
 *               |                      |
 *               Bn                     Bn
 *
 *   5. 점 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점에서 교차 : 점과 점의 관계로부터 유추
 *
 *                Bp
 *                |
 *         Ap----AB-----An
 *                |
 *                Bn
 *
 *   점과 점의 관계에서 다음과 같이 내부에 있다 하더라도
 *   선분이 Interia Ring 때문에 내부영역과 교차함을 보장할 수 없다.
 *   따라서, 구성된 Line이 Interior Ring내부에 포함되지 않음을 검사해야 함.
 *
 *      Ap  
 *       \     B
 *        \
 *         \   
 *          A------An              
 *
 *      Ap     
 *       \   I B B    I
 *        \  |     . ^ 
 *         \ | . ^
 *          AB-------An

 ***********************************************************************/

SChar
stfRelation::saiTosab( const stdPolygon2DType * aAreaInt,
                       const stdPolygon2DType * aAreaBnd )
{
    UInt i, j;
    UInt m, n;
    UInt x, y;
    
    idBool  sMeetOnPoint;
    SChar   sResult;
    
    //----------------------------
    // Area Internal 정보
    //----------------------------
    
    stdLinearRing2D * sAreaRing;
    stdPoint2D      * sAreaRingPt;
    stdPoint2D      * sAreaRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sAreaRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * sAreaRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sAreaRingCnt;     // Ring Count of a Polygon
    UInt              sAreaRingSegCnt;  // Segment Count of a Ring
    idBool            sAreaRingCCWise;  // Ring 이 시계 역방향인지의 여부

    //----------------------------
    // Area Boundary 정보
    //----------------------------
    
    stdLinearRing2D * sBndRing;
    stdPoint2D      * sBndRingPt;
    stdPoint2D      * sBndRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sBndRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sBndRingCnt;     // Ring Count of a Polygon
    UInt              sBndRingSegCnt;  // Segment Count of a Ring

    //----------------------------
    // Ring과 Ring의 중복점인지 검사
    //----------------------------
    
    UInt              sCheckSegCnt;
    stdLinearRing2D * sCheckRing;
    stdPoint2D      * sCheckPt;
    
    //----------------------------------------
    // Parameter Validation
    //----------------------------------------

    IDE_DASSERT( aAreaInt != NULL );
    IDE_DASSERT( aAreaBnd != NULL );
        
    //----------------------------------------
    // Initialization
    //----------------------------------------
    
    sResult = 'F';
    
    sAreaRingCnt = STD_N_RINGS( aAreaInt );
    sBndRingCnt = STD_N_RINGS( aAreaBnd );

    //----------------------------------------
    // 점과 면의 관계로부터 추출
    //----------------------------------------

    // Area Boundary의 좌표가 Area Internal에 포함되는 지 검사
    for ( i = 0, sBndRing = STD_FIRST_RN2D(aAreaBnd);
          i < sBndRingCnt;
          i++, sBndRing = STD_NEXT_RN2D(sBndRing) )
    {
        sBndRingSegCnt = STD_N_POINTS(sBndRing) - 1;
        
        for ( j = 0, sBndRingPt = STD_FIRST_PT2D(sBndRing);
              j < sBndRingSegCnt;
              j++, sBndRingPt = STD_NEXT_PT2D(sBndRingPt) )
        {
            // 한점이 다른 Polygon의 내부에 존재하는지 판단
            if( spiTosai( sBndRingPt, aAreaInt ) == '0' )
            {
                sResult = '1';
                IDE_RAISE( SAISAB2D_MAX_RESULT );
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    
    //----------------------------------------
    // 점과 선의 관계로부터 SAI .vs. SAB 의 관계 추출
    //----------------------------------------

    // Area Boundary가 구성하는 Ring의 개수만큼 반복
    for ( i = 0, sBndRing = STD_FIRST_RN2D(aAreaBnd);
          i < sBndRingCnt;
          i++, sBndRing = STD_NEXT_RN2D(sBndRing) )
    {
        sBndRingSegCnt = STD_N_POINTS(sBndRing) - 1;
        
        // Boundary의 Ring이 구성하는 Segment개수만큼 반복
        for ( j = 0, sBndRingPt = STD_FIRST_PT2D(sBndRing);
              j < sBndRingSegCnt;
              j++, sBndRingPt = STD_NEXT_PT2D(sBndRingPt) )
        {
            // Area Internal이 구성하는 Ring의 개수만큼 반복
            for ( m = 0, sAreaRing = STD_FIRST_RN2D(aAreaInt);
                  m < sAreaRingCnt;
                  m++, sAreaRing = STD_NEXT_RN2D(sAreaRing) )
            {
                sAreaRingSegCnt = STD_N_POINTS(sAreaRing) - 1;
                sAreaRingCCWise = stdUtils::isCCW2D(sAreaRing);
                
                // Ring이 구성하는 Segment개수만큼 반복
                for ( n = 0, sAreaRingPt = STD_FIRST_PT2D(sAreaRing);
                      n < sAreaRingSegCnt;
                      n++, sAreaRingPt = STD_NEXT_PT2D(sAreaRingPt) )
                {
                    //----------------------------
                    // Area Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------

                    sAreaRingPrevPt =
                        stdUtils::findPrevPointInRing2D( sAreaRingPt,
                                                         n,
                                                         sAreaRingSegCnt,
                                                         NULL );
                    sAreaRingCurrPt = sAreaRingPt;
                    sAreaRingNextPt =
                        stdUtils::findNextPointInRing2D( sAreaRingPt,
                                                         n,
                                                         sAreaRingSegCnt,
                                                         NULL );
                    
                    //----------------------------
                    // Boundary Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------
                
                    sBndRingPrevPt =
                        stdUtils::findPrevPointInRing2D( sBndRingPt,
                                                         j,
                                                         sBndRingSegCnt,
                                                         NULL );
                    
                    sBndRingNextPt =
                        stdUtils::findNextPointInRing2D( sBndRingPt,
                                                         j,
                                                         sBndRingSegCnt,
                                                         NULL );

                    //------------------------------------
                    // 선분과 선분의 관계로부터 유추
                    //------------------------------------

                    // 선분이 점에서 교차한다면 TRUE
                    if( ( stdUtils::intersectI2D( sAreaRingPt,
                                                  STD_NEXT_PT2D(sAreaRingPt),
                                                  sBndRingPt,
                                                  STD_NEXT_PT2D(sBndRingPt) )
                          ==ID_TRUE ) )
                    {
                        // 선분이 교차하는 경우
                        sResult = '1';
                        IDE_RAISE( SAISAB2D_MAX_RESULT );
                    }
                    else
                    {
                        // 선분이 교차하지 않는 경우
                    }
                    
                    //----------------------------------------
                    // 다음 관계를 점과 점의 관계로 만들기 위한 조정
                    //  - Ring의 선분과 점과의 관계
                    //  - Ring의 점과 점의 관계
                    //----------------------------------------

                    sMeetOnPoint = ID_FALSE;
                    
                    //----------------------------
                    // Area Ring 선분내에 Boundary Ring의 점이 존재하는지 검사
                    //----------------------------
                    
                    if ( ( stdUtils::betweenI2D( sAreaRingPt,
                                                 STD_NEXT_PT2D(sAreaRingPt),
                                                 sBndRingPt )==ID_TRUE ) )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Bp                    Bp
                        //               |                     |
                        //         A-----B-----A  ==>   Ap----AB----An  
                        //               |                     |
                        //               Bn                    Bn
                        sMeetOnPoint = ID_TRUE;
                        
                        sAreaRingPrevPt = sAreaRingPt;
                        sAreaRingCurrPt = sBndRingPt;
                        sAreaRingNextPt = STD_NEXT_PT2D(sAreaRingPt);
                    }
                
                    //----------------------------
                    // Boundary Ring 선분내에 Area Ring의 점이 존재하는지 검사
                    //----------------------------

                    if ( stdUtils::betweenI2D( sBndRingPt,
                                               STD_NEXT_PT2D(sBndRingPt),
                                               sAreaRingPt ) == ID_TRUE )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Ap                     Ap
                        //               |                      |
                        //         B-----A------B  ==>   Bp----BA----Bn  
                        //               |                      |
                        //               An                     An
                        sMeetOnPoint = ID_TRUE;
                        
                        sBndRingPrevPt = sBndRingPt;
                        sBndRingNextPt = STD_NEXT_PT2D(sBndRingPt);
                    }

                    //----------------------------
                    // 점과 점이 교차하는 지 검사
                    //----------------------------
                    
                    if ( stdUtils::isSamePoints2D( sAreaRingPt,
                                                   sBndRingPt ) == ID_TRUE )
                    {
                        sMeetOnPoint = ID_TRUE;
                        
                        // 이미 구해진 값을 사용
                    }

                    //----------------------------
                    // Ring과 Ring이 겹치는 점인지 여부를 검사
                    //----------------------------
                    
                    if ( sMeetOnPoint == ID_TRUE )
                    {
                        // Area를 구성하는 Ring간에 겹치는 점일 경우
                        // 내부에 존재하는 지를 판단할 수 없다.
                        // 다른 점에 의하여 판별 가능하다.
                        //
                        //              Bn
                        //       Ap       I
                        //        \   I  /  Bn
                        //         \  | /
                        //          \ |/
                        //           AIB-------An

                        // First Area의 점이 다른 Ring과 겹치는지 검사
                        for ( x = 0, sCheckRing = STD_FIRST_RN2D(aAreaInt);
                              x < sAreaRingCnt;
                              x++, sCheckRing = STD_NEXT_RN2D(sCheckRing) )
                        {
                            if ( m == x )
                            {
                                // 자신의 Ring은 검사하지 않음
                                continue;
                            }
                            else
                            {
                                sCheckSegCnt = STD_N_POINTS(sCheckRing) - 1;
                                
                                for ( y=0, sCheckPt=STD_FIRST_PT2D(sCheckRing);
                                      y < sCheckSegCnt;
                                      y++, sCheckPt = STD_NEXT_PT2D(sCheckPt) )
                                {
                                    if ( stdUtils::between2D(
                                             sCheckPt,
                                             STD_NEXT_PT2D(sCheckPt),
                                             sAreaRingCurrPt ) == ID_TRUE )
                                    {
                                        sMeetOnPoint = ID_FALSE;
                                        break;
                                    }
                                }
                                
                                if ( sMeetOnPoint != ID_TRUE )
                                {
                                    break;
                                }
                            }
                        }
                    }
                    
                    //----------------------------------------
                    // 점과 점의 관계로부터 외부 교차의 판단
                    //----------------------------------------
                    
                    if ( sMeetOnPoint == ID_TRUE )
                    {
                        if ( hasRelLineSegRingSeg(
                                 ( m == 0 ) ? ID_TRUE : ID_FALSE, // Exterior
                                 sAreaRingCCWise,
                                 sAreaRingPrevPt,
                                 sAreaRingNextPt,
                                 sAreaRingCurrPt,
                                 sBndRingPrevPt,
                                 sBndRingNextPt,
                                 STF_INSIDE_ANGLE_POS ) == ID_TRUE )
                        {
                            sResult = '1';
                            IDE_RAISE( SAISAB2D_MAX_RESULT );
                        }
                        else
                        {
                            // 교차 여부를 판단할 수 없음
                        }
                    }
                    else // sMeetOnPoint == ID_FALSE
                    {
                        // 검사 대상이 아님
                    }
                } // for n
            } // for m
        } // for j
    } // for i

    IDE_EXCEPTION_CONT(SAISAB2D_MAX_RESULT);
    
    return sResult;
}

/***********************************************************************
 * Description:
 *
 *    SAI(Single Area Internal)과
 *    SAE(Single Area External)의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *   BUG-17037
 *
 *   Polygon을 구성하는 [점, 선분, 면] 과
 *   Polygon을 구성하는 [점, 선분, 면] 의 관계를 통해 구해낸다.
 *
 *   ================================
 *     Polygon Interior .vs. Polygon Exterior
 *   ================================
 *
 *   1. 면 .vs. 면
 *
 *       - AreaInt 내부의 한점이 AreaExt 외부면과 교차하는지의 여부 : TRUE
 *
 *          A----------A
 *          |          |           X-----X
 *          | IX====IX |           |  X  |
 *          | ||    || |           |     |   A---A
 *          | || X  || |           X-----X   |   |
 *          | IX====IX |                     A---A
 *          |          |
 *          A----------A
 *
 *   1. 선분 .vs. 면
 *
 *       - 선분과 선분, 점과 점의 관계로부터 유추
 *
 *   2. 점 .vs. 면
 *
 *       - AreaInt의 한점이 AreaExt의 외부에 존재 : TRUE
 *
 *                X--------X
 *            A   |        |
 *                |        |
 *                |        |
 *                X--------X
 *
 *       - AreaExt의 내부링의 한점이 AreaInt의 내부에 존재 : TRUE
 *
 *
 *        X-------------X
 *        | A---------A |
 *        | |         | |
 *        | |   X--X  | |
 *        | |   |  |  | |
 *        | |   X--X  | |
 *        | |         | |
 *        | A---------A |
 *        X-------------X
 *
 *                 
 *   3. 선분 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *
 *       - 선분이 선분으로 교차 : 점과 점의 관계로부터 유추 가능
 *
 *             B---A====B----A
 *
 *             B---A====A----B
 *
 *             A---B====B----A
 *
 *       - 선분이 점에서 교차 : TRUE
 *
 *                B
 *                |
 *            A---+----A
 *                |
 *                B
 *
 *   4. 점 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Bp                     Bp
 *               |                      |
 *         A-----B------A  ==>   Ap----AB----An  
 *               |                      |
 *               Bn                     Bn
 *
 *   5. 점 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점에서 교차 : 점과 점의 관계로부터 유추
 *
 *                Bp
 *                |
 *         Ap----AB-----An
 *                |
 *                Bn
 *
 *   점과 점의 관계에서 다음과 같이 내부에 있다 하더라도
 *   선분이 Interia Ring 때문에 내부영역과 교차함을 보장할 수 없다.
 *   따라서, 구성된 Line이 Interior Ring내부에 포함되지 않음을 검사해야 함.
 *
 *      Ap  
 *       \     B
 *        \
 *         \   
 *          A------An              
 *
 *      Ap     
 *       \   I B B    I
 *        \  |     . ^ 
 *         \ | . ^
 *          AB-------An
 *
 ***********************************************************************/

IDE_RC stfRelation::saiTosae( iduMemory *              aQmxMem,
                              const stdPolygon2DType * aAreaInt,
                              const stdPolygon2DType * aAreaExt,
                              SChar *                  aResult )
{
    UInt i, j;
    UInt m, n;
    UInt x, y;
    
    idBool  sMeetOnPoint;
    SChar   sResult;
    
    //----------------------------
    // Internal Area 정보
    //----------------------------
    
    stdLinearRing2D * sIntRing;
    stdPoint2D      * sIntRingPt;
    stdPoint2D      * sIntRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sIntRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * sIntRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sIntRingCnt;     // Ring Count of a Polygon
    UInt              sIntRingSegCnt;  // Segment Count of a Ring

    stdPoint2D        sIntSomePt;
    
    //----------------------------
    // External Area 정보
    //----------------------------
    
    stdLinearRing2D * sExtRing;
    stdPoint2D      * sExtRingPt;
    stdPoint2D      * sExtRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sExtRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * sExtRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sExtRingCnt;     // Ring Count of a Polygon
    UInt              sExtRingSegCnt;  // Segment Count of a Ring
    idBool            sExtRingCCWise;  // Ring 이 시계 역방향인지의 여부

    //----------------------------
    // Ring과 Ring의 중복점인지 검사
    //----------------------------
    
    UInt              sCheckSegCnt;
    stdLinearRing2D * sCheckRing;
    stdPoint2D      * sCheckPt;
    
    //----------------------------------------
    // Parameter Validation
    //----------------------------------------

    IDE_DASSERT( aAreaInt != NULL );
    IDE_DASSERT( aAreaExt != NULL );
        
    //----------------------------------------
    // Initialization
    //----------------------------------------
    
    sResult = 'F';
    
    sIntRingCnt = STD_N_RINGS( aAreaInt );
    sExtRingCnt = STD_N_RINGS( aAreaExt );

    //----------------------------------------
    // 면과 면의 관계로부터 추출
    //----------------------------------------

    // AreaInt 내부의 어떤점이 AreaExt의 외부에 존재하는 지 검사
    IDE_TEST( stdUtils::getPointOnSurface2D( aQmxMem, aAreaInt, & sIntSomePt )
              != IDE_SUCCESS );
    
    if( spiTosae( &sIntSomePt, aAreaExt ) == '0' )
    {
        sResult = '2';
        IDE_RAISE( SAISAE2D_MAX_RESULT );
    }
    
    //----------------------------------------
    // 점과 면의 관계로부터 추출
    //----------------------------------------

    // AreaInt의 좌표가 AreaExt의 외부에 있는지 검사
    for ( i = 0, sIntRing = STD_FIRST_RN2D(aAreaInt);
          i < sIntRingCnt;
          i++, sIntRing = STD_NEXT_RN2D(sIntRing) )
    {
        sIntRingSegCnt = STD_N_POINTS(sIntRing) - 1;
        
        for ( j = 0, sIntRingPt = STD_FIRST_PT2D(sIntRing);
              j < sIntRingSegCnt;
              j++, sIntRingPt = STD_NEXT_PT2D(sIntRingPt) )
        {
            // 한점이 다른 Polygon의 외부에 존재하는지 판단
            if( spiTosae( sIntRingPt, aAreaExt ) == '0' )
            {
                sResult = '2';
                IDE_RAISE( SAISAE2D_MAX_RESULT );
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    // AreaExt의 내부링의 한 좌표가 AreaInt의 내부에 있는지 조사
    for ( m = 0, sExtRing = STD_FIRST_RN2D(aAreaExt);
          m < sExtRingCnt;
          m++, sExtRing = STD_NEXT_RN2D(sExtRing) )
    {
        if ( m == 0 )
        {
            // 외부링은 검사하지 않음
            continue;
        }
        
        sExtRingSegCnt = STD_N_POINTS(sExtRing) - 1;
        
        for ( n = 0, sExtRingPt = STD_FIRST_PT2D(sExtRing);
              n < sExtRingSegCnt;
              n++, sExtRingPt = STD_NEXT_PT2D(sExtRingPt) )
        {
            // 한점이 다른 Polygon의 외부에 존재하는지 판단
            if( spiTosai( sExtRingPt, aAreaInt ) == '0' )
            {
                sResult = '2';
                IDE_RAISE( SAISAE2D_MAX_RESULT );
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    
    //----------------------------------------
    // Internal Area와 External Area 의 관계 추출
    //----------------------------------------

    // Internal Area가 구성하는 Ring의 개수만큼 반복
    for ( i = 0, sIntRing = STD_FIRST_RN2D(aAreaInt);
          i < sIntRingCnt;
          i++, sIntRing = STD_NEXT_RN2D(sIntRing) )
    {
        sIntRingSegCnt = STD_N_POINTS(sIntRing) - 1;
        
        // Ring이 구성하는 Segment개수만큼 반복
        for ( j = 0, sIntRingPt = STD_FIRST_PT2D(sIntRing);
              j < sIntRingSegCnt;
              j++, sIntRingPt = STD_NEXT_PT2D(sIntRingPt) )
        {
            // External Area가 구성하는 Ring의 개수만큼 반복
            for ( m = 0, sExtRing = STD_FIRST_RN2D(aAreaExt);
                  m < sExtRingCnt;
                  m++, sExtRing = STD_NEXT_RN2D(sExtRing) )
            {
                sExtRingSegCnt = STD_N_POINTS(sExtRing) - 1;
                sExtRingCCWise = stdUtils::isCCW2D(sExtRing);
                
                // Ring이 구성하는 Segment개수만큼 반복
                for ( n = 0, sExtRingPt = STD_FIRST_PT2D(sExtRing);
                      n < sExtRingSegCnt;
                      n++, sExtRingPt = STD_NEXT_PT2D(sExtRingPt) )
                {
                    //----------------------------
                    // Internal Area Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------

                    sIntRingPrevPt =
                        stdUtils::findPrevPointInRing2D( sIntRingPt,
                                                         j,
                                                         sIntRingSegCnt,
                                                         NULL );
                    sIntRingCurrPt = sIntRingPt;
                    sIntRingNextPt =
                        stdUtils::findNextPointInRing2D( sIntRingPt,
                                                         j,
                                                         sIntRingSegCnt,
                                                         NULL );
                    
                    //----------------------------
                    // Exterior Area Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------
                
                    sExtRingPrevPt =
                        stdUtils::findPrevPointInRing2D( sExtRingPt,
                                                         n,
                                                         sExtRingSegCnt,
                                                         NULL );
                    sExtRingCurrPt = sExtRingPt;
                    sExtRingNextPt =
                        stdUtils::findNextPointInRing2D( sExtRingPt,
                                                         n,
                                                         sExtRingSegCnt,
                                                         NULL );

                    //------------------------------------
                    // 선분과 선분의 관계로부터 유추
                    //------------------------------------

                    // 선분이 점에서 교차한다면 TRUE
                    if( ( stdUtils::intersectI2D( sIntRingPt,
                                                  STD_NEXT_PT2D(sIntRingPt),
                                                  sExtRingPt,
                                                  STD_NEXT_PT2D(sExtRingPt) )
                          ==ID_TRUE ) )
                    {
                        // 선분이 교차하는 경우
                        sResult = '2';
                        IDE_RAISE( SAISAE2D_MAX_RESULT );
                    }
                    else
                    {
                        // 선분이 교차하지 않는 경우
                    }
                    
                    //----------------------------------------
                    // 다음 관계를 점과 점의 관계로 만들기 위한 조정
                    //  - Ring의 선분과 점과의 관계
                    //  - Ring의 점과 점의 관계
                    //----------------------------------------

                    sMeetOnPoint = ID_FALSE;
                    
                    //----------------------------
                    // Internal Area Ring 선분내에
                    // External Area Ring 의 점이 존재하는지 검사
                    //----------------------------
                    
                    if ( ( stdUtils::betweenI2D( sIntRingPt,
                                                 STD_NEXT_PT2D(sIntRingPt),
                                                 sExtRingPt )==ID_TRUE ) )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Bp                    Bp
                        //               |                     |
                        //         A-----B-----A  ==>   Ap----AB----An  
                        //               |                     |
                        //               Bn                    Bn
                        sMeetOnPoint = ID_TRUE;
                        
                        sIntRingPrevPt = sIntRingPt;
                        sIntRingCurrPt = sExtRingPt;
                        sIntRingNextPt = STD_NEXT_PT2D(sIntRingPt);
                    }
                
                    //----------------------------
                    // External Area Ring 선분내에
                    // Internal Area Ring의 점이 존재하는지 검사
                    //----------------------------

                    if ( stdUtils::betweenI2D( sExtRingPt,
                                               STD_NEXT_PT2D(sExtRingPt),
                                               sIntRingPt ) == ID_TRUE )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Ap                     Ap
                        //               |                      |
                        //         B-----A------B  ==>   Bp----BA----Bn  
                        //               |                      |
                        //               An                     An
                        sMeetOnPoint = ID_TRUE;
                        
                        sExtRingPrevPt = sExtRingPt;
                        sExtRingCurrPt = sIntRingPt;
                        sExtRingNextPt = STD_NEXT_PT2D(sExtRingPt);
                    }

                    //----------------------------
                    // 점과 점이 교차하는 지 검사
                    //----------------------------
                    
                    if ( stdUtils::isSamePoints2D( sIntRingPt,
                                                   sExtRingPt ) == ID_TRUE )
                    {
                        sMeetOnPoint = ID_TRUE;
                        
                        // 이미 구해진 값을 사용
                    }

                    //----------------------------
                    // Ring과 Ring이 겹치는 점인지 여부를 검사
                    //----------------------------
                    
                    if ( sMeetOnPoint == ID_TRUE )
                    {
                        // Ring과 Ring이 겹치는 점일 경우
                        // 내부에 존재하는 지를 판단할 수 없다.
                        // 다른 점에 의하여 판별 가능하다.
                        //
                        //              Pn
                        //       Ap       I
                        //        \   I  /  Pn
                        //         \  | /
                        //          \ |/
                        //           AIP-------An

                        // Internal Area의 점이 다른 Ring과 겹치는지 검사
                        for ( x = 0, sCheckRing = STD_FIRST_RN2D(aAreaInt);
                              x < sIntRingCnt;
                              x++, sCheckRing = STD_NEXT_RN2D(sCheckRing) )
                        {
                            if ( i == x )
                            {
                                // 자신의 Ring은 검사하지 않음
                                continue;
                            }
                            else
                            {
                                sCheckSegCnt = STD_N_POINTS(sCheckRing) - 1;
                                
                                for ( y=0, sCheckPt=STD_FIRST_PT2D(sCheckRing);
                                      y < sCheckSegCnt;
                                      y++, sCheckPt = STD_NEXT_PT2D(sCheckPt) )
                                {
                                    if ( stdUtils::between2D(
                                             sCheckPt,
                                             STD_NEXT_PT2D(sCheckPt),
                                             sIntRingCurrPt ) == ID_TRUE )
                                    {
                                        sMeetOnPoint = ID_FALSE;
                                        break;
                                    }
                                }
                                
                                if ( sMeetOnPoint != ID_TRUE )
                                {
                                    break;
                                }
                            }
                        }

                        if ( sMeetOnPoint != ID_TRUE )
                        {
                            continue;
                        }
                        
                        // External Area의 점이 다른 Ring과 겹치는지 검사
                        for ( x = 0, sCheckRing = STD_FIRST_RN2D(aAreaExt);
                              x < sExtRingCnt;
                              x++, sCheckRing = STD_NEXT_RN2D(sCheckRing) )
                        {
                            if ( m == x )
                            {
                                // 자신의 Ring은 검사하지 않음
                                continue;
                            }
                            else
                            {
                                sCheckSegCnt = STD_N_POINTS(sCheckRing) - 1;
                                
                                for ( y=0, sCheckPt=STD_FIRST_PT2D(sCheckRing);
                                      y < sCheckSegCnt;
                                      y++, sCheckPt = STD_NEXT_PT2D(sCheckPt) )
                                {
                                    if ( stdUtils::between2D(
                                             sCheckPt,
                                             STD_NEXT_PT2D(sCheckPt),
                                             sExtRingCurrPt ) == ID_TRUE )
                                    {
                                        sMeetOnPoint = ID_FALSE;
                                        break;
                                    }
                                }
                                
                                if ( sMeetOnPoint != ID_TRUE )
                                {
                                    break;
                                }
                            }
                        }
                    }
                    
                    //----------------------------------------
                    // 점과 점의 관계로부터 외부 교차의 판단
                    //----------------------------------------
                    
                    if ( sMeetOnPoint == ID_TRUE )
                    {
                        if ( hasRelLineSegRingSeg(
                                 (m == 0) ? ID_TRUE : ID_FALSE, // Exterior
                                 sExtRingCCWise,
                                 sExtRingPrevPt,
                                 sExtRingNextPt,
                                 sExtRingCurrPt,
                                 sIntRingPrevPt,
                                 sIntRingNextPt,
                                 STF_OUTSIDE_ANGLE_POS ) == ID_TRUE )
                        {
                            sResult = '2';
                            IDE_RAISE( SAISAE2D_MAX_RESULT );
                        }
                        else
                        {
                            // 교차 여부를 판단할 수 없음
                        }
                    }
                    else // sMeetOnPoint == ID_FALSE
                    {
                        // 검사 대상이 아님
                    }
                } // for n
            } // for m
        } // for j
    } // for i

    IDE_EXCEPTION_CONT(SAISAE2D_MAX_RESULT);
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}


SChar stfRelation::sabTosab( const stdPolygon2DType*    aObj1,
                             const stdPolygon2DType*    aObj2 )
{
    stdPoint2D*             sPt1, *sPt2;
    stdLinearRing2D*        sRing1, *sRing2;
    UInt                    i,j,k,l, sMax1, sMax2, sMaxR1, sMaxR2;
    SChar                   sResult = 0x00;
    SChar                   sTemp;

    sRing1 = STD_FIRST_RN2D(aObj1);
    sMaxR1 = STD_N_RINGS(aObj1);
    for( i = 0; i < sMaxR1; i++ )
    {
        sPt1 = STD_FIRST_PT2D(sRing1);
        sMax1 = STD_N_POINTS(sRing1)-1;
        for( j = 0; j < sMax1; j++ )
        {
            sRing2 = STD_FIRST_RN2D(aObj2);
            sMaxR2 = STD_N_RINGS(aObj2);
            for( k = 0; k < sMaxR2; k++ )
            {
                sPt2 = STD_FIRST_PT2D(sRing2);
                sMax2 = STD_N_POINTS(sRing2)-1;
                for( l = 0; l < sMax2; l++ )
                {
                    // Fix Bug-15432
                    sTemp = MidLineToMidLine(
                        sPt1, STD_NEXT_PT2D(sPt1), 
                        sPt2, STD_NEXT_PT2D(sPt2));

                    if( sTemp == '1' )
                    {
                         return '1';    // 나올 수 있는 최고 차원
                    }
                    else if( (sResult < '0') && (sTemp == '0') )
                    {
                        sResult = '0';
                    }

                    sPt2 = STD_NEXT_PT2D(sPt2);
                }
                sPt2 = STD_NEXT_PT2D(sPt2);
                sRing2 = (stdLinearRing2D*)sPt2;
            }
            sPt1 = STD_NEXT_PT2D(sPt1);
        }
        sPt1 = STD_NEXT_PT2D(sPt1);
        sRing1 = (stdLinearRing2D*)sPt1;
    }

    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    return sResult;
}

/***********************************************************************
 * Description:
 *
 *    SAB(Single Area Boundary)과
 *    SAE(Single Area External)의 DE-9IM 관계를 구한다.
 *
 * Implementation:
 *
 *   BUG-17043
 *
 *   Polygon Boundary를 구성하는 [점, 선분] 과
 *   Polygon External을 구성하는 [점, 선분, 면] 의 관계를 통해 구해낸다.
 *
 *   ================================
 *     Boundary .vs. External
 *   ================================
 *
 *   1. 선분 .vs. 면
 *
 *       - 선분과 선분, 점과 점의 관계로부터 유추
 *
 *   2. 점 .vs. 면 
 *       - 점이 Polygon의 내부에 존재 : 유추 불가
 *       - 점이 Polygon의 외부에 존재 : TRUE
 *
 *                A--------A
 *                |        |
 *                |        |    B
 *                |        |
 *                A--------A
 *                 
 *   3. 선분 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *
 *       - 선분이 선분으로 교차 : 점과 점의 관계로부터 유추 가능
 *
 *             B---A====B----A
 *
 *             B---A====A----B
 *
 *             A---B====B----A
 *
 *       - 선분이 점에서 교차 : TRUE
 *
 *                B
 *                |
 *            A---+----A
 *                |
 *                B
 *
 *   4. 점 .vs. 선분
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점이 선분에서 교차 : 점과 점의 관계로부터 유추 가능
 *
 *               Bp                     Bp
 *               |                      |
 *         A-----B------A  ==>   Ap----AB----An  
 *               |                      |
 *               Bn                     Bn
 *
 *   5. 점 .vs. 점
 *       - 교차하지 않는 경우 : 유추 불가
 *       - 점에서 교차 : 점과 점의 관계로부터 유추
 *
 *                Bp
 *                |
 *         Ap----AB-----An
 *                |
 *                Bn
 *
 ***********************************************************************/

SChar
stfRelation::sabTosae( const stdPolygon2DType * aAreaBnd,
                       const stdPolygon2DType * aAreaExt )
{
    UInt i, j;
    UInt m, n;
    
    idBool  sMeetOnPoint;
    SChar   sResult;
    
    //----------------------------
    // Area External 정보
    //----------------------------
    
    stdLinearRing2D * sExtRing;
    stdPoint2D      * sExtRingPt;
    stdPoint2D      * sExtRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sExtRingCurrPt;  // Ring Point의 이전 Point
    stdPoint2D      * sExtRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sExtRingCnt;     // Ring Count of a Polygon
    UInt              sExtRingSegCnt;  // Segment Count of a Ring
    idBool            sExtRingCCWise;  // Ring 이 시계 역방향인지의 여부

    //----------------------------
    // Area Boundary 정보
    //----------------------------
    
    stdLinearRing2D * sBndRing;
    stdPoint2D      * sBndRingPt;
    stdPoint2D      * sBndRingPrevPt;  // Ring Point의 이전 Point
    stdPoint2D      * sBndRingNextPt;  // Ring Point의 다음 Point
    
    UInt              sBndRingCnt;     // Ring Count of a Polygon
    UInt              sBndRingSegCnt;  // Segment Count of a Ring

    //----------------------------------------
    // Parameter Validation
    //----------------------------------------

    IDE_DASSERT( aAreaExt != NULL );
    IDE_DASSERT( aAreaBnd != NULL );
        
    //----------------------------------------
    // Initialization
    //----------------------------------------
    
    sResult = 'F';
    
    sExtRingCnt = STD_N_RINGS( aAreaExt );
    sBndRingCnt = STD_N_RINGS( aAreaBnd );

    //----------------------------------------
    // 점과 면의 관계로부터 추출
    //----------------------------------------

    // AreaBnd의 좌표가 AreaExt 외부에 포함되는 지 검사
    for ( i = 0, sBndRing = STD_FIRST_RN2D(aAreaBnd);
          i < sBndRingCnt;
          i++, sBndRing = STD_NEXT_RN2D(sBndRing) )
    {
        sBndRingSegCnt = STD_N_POINTS(sBndRing) - 1;
        
        for ( j = 0, sBndRingPt = STD_FIRST_PT2D(sBndRing);
              j < sBndRingSegCnt;
              j++, sBndRingPt = STD_NEXT_PT2D(sBndRingPt) )
        {
            // 한점이 다른 Polygon의 내부에 존재하는지 판단
            if( spiTosae( sBndRingPt, aAreaExt ) == '0' )
            {
                sResult = '1';
                IDE_RAISE( SABSAE2D_MAX_RESULT );
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    
    //----------------------------------------
    // 점과 선의 관계로부터 SAB .vs. SAE 의 관계 추출
    //----------------------------------------

    // AreaBnd가 구성하는 Ring의 개수만큼 반복
    for ( i = 0, sBndRing = STD_FIRST_RN2D(aAreaBnd);
          i < sBndRingCnt;
          i++, sBndRing = STD_NEXT_RN2D(sBndRing) )
    {
        sBndRingSegCnt = STD_N_POINTS(sBndRing) - 1;
        
        // Boundary의 Ring이 구성하는 Segment개수만큼 반복
        for ( j = 0, sBndRingPt = STD_FIRST_PT2D(sBndRing);
              j < sBndRingSegCnt;
              j++, sBndRingPt = STD_NEXT_PT2D(sBndRingPt) )
        {
            // AreaExt이 구성하는 Ring의 개수만큼 반복
            for ( m = 0, sExtRing = STD_FIRST_RN2D(aAreaExt);
                  m < sExtRingCnt;
                  m++, sExtRing = STD_NEXT_RN2D(sExtRing) )
            {
                sExtRingSegCnt = STD_N_POINTS(sExtRing) - 1;
                sExtRingCCWise = stdUtils::isCCW2D(sExtRing);
                
                // Ring이 구성하는 Segment개수만큼 반복
                for ( n = 0, sExtRingPt = STD_FIRST_PT2D(sExtRing);
                      n < sExtRingSegCnt;
                      n++, sExtRingPt = STD_NEXT_PT2D(sExtRingPt) )
                {
                    //----------------------------
                    // AreaExt Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------

                    sExtRingPrevPt =
                        stdUtils::findPrevPointInRing2D( sExtRingPt,
                                                         n,
                                                         sExtRingSegCnt,
                                                         NULL );
                    sExtRingCurrPt = sExtRingPt;
                    sExtRingNextPt =
                        stdUtils::findNextPointInRing2D( sExtRingPt,
                                                         n,
                                                         sExtRingSegCnt,
                                                         NULL );
                    
                    //----------------------------
                    // AreaBnd Ring의 현재점, 이전점, 이후점을 구함
                    //----------------------------
                
                    sBndRingPrevPt =
                        stdUtils::findPrevPointInRing2D( sBndRingPt,
                                                         j,
                                                         sBndRingSegCnt,
                                                         NULL );
                    sBndRingNextPt =
                        stdUtils::findNextPointInRing2D( sBndRingPt,
                                                         j,
                                                         sBndRingSegCnt,
                                                         NULL );

                    //------------------------------------
                    // 선분과 선분의 관계로부터 유추
                    //------------------------------------

                    // 선분이 점에서 교차한다면 TRUE
                    if( ( stdUtils::intersectI2D( sExtRingPt,
                                                  STD_NEXT_PT2D(sExtRingPt),
                                                  sBndRingPt,
                                                  STD_NEXT_PT2D(sBndRingPt) )
                          ==ID_TRUE ) )
                    {
                        // 선분이 교차하는 경우
                        sResult = '1';
                        IDE_RAISE( SABSAE2D_MAX_RESULT );
                    }
                    else
                    {
                        // 선분이 교차하지 않는 경우
                    }
                    
                    //----------------------------------------
                    // 다음 관계를 점과 점의 관계로 만들기 위한 조정
                    //  - Ring의 선분과 점과의 관계
                    //  - Ring의 점과 점의 관계
                    //----------------------------------------

                    sMeetOnPoint = ID_FALSE;
                    
                    //----------------------------
                    // Area Ring 선분내에 Boundary Ring의 점이 존재하는지 검사
                    //----------------------------
                    
                    if ( ( stdUtils::betweenI2D( sExtRingPt,
                                                 STD_NEXT_PT2D(sExtRingPt),
                                                 sBndRingPt )==ID_TRUE ) )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Bp                    Bp
                        //               |                     |
                        //         A-----B-----A  ==>   Ap----AB----An  
                        //               |                     |
                        //               Bn                    Bn
                        sMeetOnPoint = ID_TRUE;
                        
                        sExtRingPrevPt = sExtRingPt;
                        sExtRingCurrPt = sBndRingPt;
                        sExtRingNextPt = STD_NEXT_PT2D(sExtRingPt);
                    }
                
                    //----------------------------
                    // Boundary Ring 선분내에 Area Ring의 점이 존재하는지 검사
                    //----------------------------

                    if ( stdUtils::betweenI2D( sBndRingPt,
                                               STD_NEXT_PT2D(sBndRingPt),
                                               sExtRingPt ) == ID_TRUE )
                    {
                        // 점이 선분에서 교차 => 점과 점의 관계로 변경
                        //               Ap                     Ap
                        //               |                      |
                        //         B-----A------B  ==>   Bp----BA----Bn  
                        //               |                      |
                        //               An                     An
                        sMeetOnPoint = ID_TRUE;
                        
                        sBndRingPrevPt = sBndRingPt;
                        sBndRingNextPt = STD_NEXT_PT2D(sBndRingPt);
                    }

                    //----------------------------
                    // 점과 점이 교차하는 지 검사
                    //----------------------------
                    
                    if ( stdUtils::isSamePoints2D( sExtRingPt,
                                                   sBndRingPt ) == ID_TRUE )
                    {
                        sMeetOnPoint = ID_TRUE;
                        
                        // 이미 구해진 값을 사용
                    }

                    //----------------------------------------
                    // 점과 점의 관계로부터 외부 교차의 판단
                    //----------------------------------------
                    
                    if ( sMeetOnPoint == ID_TRUE )
                    {
                        if ( hasRelLineSegRingSeg(
                                 ( m == 0 ) ? ID_TRUE : ID_FALSE, // Exterior
                                 sExtRingCCWise,
                                 sExtRingPrevPt,
                                 sExtRingNextPt,
                                 sExtRingCurrPt,
                                 sBndRingPrevPt,
                                 sBndRingNextPt,
                                 STF_OUTSIDE_ANGLE_POS ) == ID_TRUE )
                        {
                            sResult = '1';
                            IDE_RAISE( SABSAE2D_MAX_RESULT );
                        }
                        else
                        {
                            // 교차 여부를 판단할 수 없음
                        }
                    }
                    else // sMeetOnPoint == ID_FALSE
                    {
                        // 검사 대상이 아님
                    }
                } // for n
            } // for m
        } // for j
    } // for i

    IDE_EXCEPTION_CONT(SABSAE2D_MAX_RESULT);
    
    return sResult;
}


// polygon vs multipoint
SChar stfRelation::saiTompi( const stdPolygon2DType*        aObj1,
                             const stdMultiPoint2DType*     aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;
    
    sPoint = STD_FIRST_POINT2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        if( spiTosai(&sPoint->mPoint, aObj1) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::sabTompi( const stdPolygon2DType*        aObj1,
                             const stdMultiPoint2DType*     aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;
    
    sPoint = STD_FIRST_POINT2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTosab(&sPoint->mPoint, aObj1) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::saeTompi( const stdPolygon2DType*        aObj1,
                             const stdMultiPoint2DType*     aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;
    
    sPoint = STD_FIRST_POINT2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTosae(&sPoint->mPoint, aObj1) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

// polygon vs multilinestring
SChar stfRelation::saiTomli( const stdPolygon2DType*                aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;
    
    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTosai(sLine, aObj1);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::saiTomlb( const stdPolygon2DType*                aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    
    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }
    
    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        if( slbTosai(sLine, aObj1) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::sabTomli( const stdPolygon2DType*                aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;
    
    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTosab(sLine, aObj1);

        if( sRet == '1' )
        {
            return '1';
        }


        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sabTomlb( const stdPolygon2DType*                aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    
    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    
    
    for( i = 0; i < sMax; i++ )
    {
        if( slbTosab(sLine, aObj1) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::sabTomle( const stdPolygon2DType*                aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdPoint2D*             sPt1;
    stdLinearRing2D*        sRing1;
    stdLineString2DType*    sLine2;
    UInt                    i, j, k, sMax, sMaxR, sMaxO;
    idBool                  sFound;

    sRing1 = STD_FIRST_RN2D(aObj1);
    sMaxR = STD_N_RINGS(aObj1);
    
    for( i = 0; i < sMaxR; i++ )
    {
        sMax = STD_N_POINTS(sRing1)-1;
        sPt1 = STD_FIRST_PT2D(sRing1);
        for( j = 0; j < sMax; j++ )
        {
            // Fix BUG-15516
            sFound = ID_FALSE;
            sLine2 = STD_FIRST_LINE2D(aObj2);
            sMaxO = STD_N_OBJECTS(aObj2);
            for( k = 0; k < sMaxO; k++ )
            {
                if( lineInLineString( 
                    sPt1, STD_NEXT_PT2D(sPt1), sLine2 ) == ID_TRUE )
                {
                    sFound = ID_TRUE;
                    break;
                }
                sLine2 = STD_NEXT_LINE2D(sLine2);
            }

            if( sFound == ID_FALSE )
            {
                return '1';
            }

            sPt1 = STD_NEXT_PT2D(sPt1);
        }
        sPt1 = STD_NEXT_PT2D(sPt1);
        sRing1 = (stdLinearRing2D*)sPt1;
    }

    return 'F';
}

SChar stfRelation::saeTomli( const stdPolygon2DType*                aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTosae(sLine, aObj1);

        if( sRet == '1' )
        {
            return '1';
        }


        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::saeTomlb( const stdPolygon2DType*                aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        if( slbTosae(sLine, aObj1) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

// polygon vs multipolygon
IDE_RC stfRelation::saiTomai( iduMemory*                         aQmxMem,
                              const stdPolygon2DType*            aObj1,
                              const stdMultiPolygon2DType*       aObj2,
                              SChar*                             aResult )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;
    
    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        IDE_TEST( saiTosai(aQmxMem, aObj1, sPoly, &sRet)
                  != IDE_SUCCESS );

        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }


        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
        
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::saiTomab( const stdPolygon2DType*            aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = saiTosab(aObj1, sPoly);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

/***********************************************************************
 *
 * Description :
 *
 *    단일 영역 객체의 내부 영역과 다중 영역 객체의 외부 영역과의 관계를 구함.
 *    sai(single area internal), mae(multi area external)
 *
 * Implementation :
 *
 *    표기법 : Ai (객체 A의 interior 영역)
 *
 *    BUG-16319
 *    단일 객체 내부 영역과 다중 객체 외부 영역과의 관계는 다음과 같은 식으로 표현
 *
 *    Si ^ ( A U B U ...U N )e
 *    <==>
 *    Si ^ ( Ae ^ Be ^ ... Ne )
 *
 ***********************************************************************/

IDE_RC stfRelation::saiTomae( iduMemory*                         aQmxMem,
                              const stdPolygon2DType*            aObj1,
                              const stdMultiPolygon2DType*       aObj2,
                              SChar*                             aResult )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet;
    SChar               sResult;

    //---------------------------------
    // Initialization
    //---------------------------------
    
    // 다중 객체 외부 영역중 모두 교차영역이 있어야 교차영역이 존재한다.
    sResult = '2';

    //---------------------------------
    // 모든 객체의 외부 영역과 교차영역이 존재하는 지를 판단
    //---------------------------------
    
    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    
    
    for( i = 0; i < sMax; i++ )
    {
        IDE_TEST( saiTosae(aQmxMem, aObj1, sPoly, &sRet )
                  != IDE_SUCCESS );

        if( sRet != '2' )
        {
            sResult = 'F';
            break;
        }
        else
        {
            // 교차 영역이 존재함
        }
        
        sPoly = STD_NEXT_POLY2D(sPoly);
    }

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::sabTomai( const stdPolygon2DType*            aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;
    
    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = saiTosab(sPoly, aObj1);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sabTomab( const stdPolygon2DType*            aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sabTosab(aObj1, sPoly);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

/***********************************************************************
 *
 * Description :
 *
 *    단일 영역 객체의 경계와 다중 영역 객체의 외부 영역과의 관계를 구함.
 *    sab(single area boundary), mae(multi area external)
 *
 * Implementation :
 *
 *    표기법 : Ai (객체 A의 interior 영역)
 *
 *    단일 객체 경계와 다중 객체 외부 영역과의 관계는 다음과 같은 식으로 표현
 *
 *    Sb ^ ( A U B U ...U N )e
 *    <==>
 *    Sb ^ ( Ae ^ Be ^ ... Ne )
 *
 ***********************************************************************/

SChar stfRelation::sabTomae( const stdPolygon2DType*            aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet;
    SChar               sResult;

    //---------------------------------
    // Initialization
    //---------------------------------
    
    // 다중 객체 외부 영역중 모두 교차영역이 있어야 교차라인이 존재한다.
    sResult = '1';

    //---------------------------------
    // 모든 객체의 외부 영역과 교차영역이 존재하는 지를 판단
    //---------------------------------
    
    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    

    for( i = 0; i < sMax; i++ )
    {
        sRet = sabTosae(aObj1, sPoly);

        if( sRet != '1' )
        {
            sResult = 'F';
            break;
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }

    return sResult;
}


IDE_RC stfRelation::saeTomai( iduMemory*                         aQmxMem,
                              const stdPolygon2DType*            aObj1,
                              const stdMultiPolygon2DType*       aObj2,
                              SChar*                             aResult )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    

    for( i = 0; i < sMax; i++ )
    {
        IDE_TEST( saiTosae(aQmxMem, sPoly, aObj1, &sRet)
                  != IDE_SUCCESS );

        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::saeTomab( const stdPolygon2DType*            aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPolygon2DType*   sPoly;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);    

    for( i = 0; i < sMax; i++ )
    {
        sRet = sabTosae(sPoly, aObj1);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

// polygon vs geometrycollection
IDE_RC stfRelation::saiTogci( iduMemory*                         aQmxMem,
                              const stdPolygon2DType*            aObj1,
                              const stdGeoCollection2DType*      aObj2,
                              SChar*                             aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
                sRet = spiTosai( &sGeom->point2D.mPoint, aObj1 );
                break;
            case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
                sRet = sliTosai( &sGeom->linestring2D, aObj1 );
                break;
            case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTosai( aQmxMem, aObj1, &sGeom->polygon2D, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
                sRet = saiTompi( aObj1, &sGeom->mpoint2D );
                break;
            case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = saiTomli( aObj1, &sGeom->mlinestring2D );
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTomai( aQmxMem, aObj1, &sGeom->mpolygon2D, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }
        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::saiTogcb( const stdPolygon2DType*            aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTosai( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saiTosab( aObj1, &sGeom->polygon2D );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = saiTomlb( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = saiTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

IDE_RC stfRelation::saiTogce( iduMemory*                         aQmxMem,
                              const stdPolygon2DType*            aObj1,
                              const stdGeoCollection2DType*      aObj2,
                              SChar*                             aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
            case STD_POINT_2D_EXT_TYPE:
            case STD_LINESTRING_2D_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
                sRet = '2';
                break;
            case STD_POLYGON_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTosae( aQmxMem, aObj1, &sGeom->polygon2D, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
            case STD_MULTIPOINT_2D_EXT_TYPE:
            case STD_MULTILINESTRING_2D_TYPE:
            case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = '2';
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTomae( aQmxMem, aObj1, &sGeom->mpolygon2D, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            sResult = sRet;
            break;
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::sabTogci( const stdPolygon2DType*            aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTosab( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTosab( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saiTosab( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = sabTompi( aObj1, &sGeom->mpoint2D );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sabTomli( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = sabTomai( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sabTogcb( const stdPolygon2DType*                aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTosab( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTosab( aObj1, &sGeom->polygon2D );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sabTomlb( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = sabTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::sabTogce( const stdPolygon2DType*            aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sleTosab( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTosae( aObj1, &sGeom->polygon2D );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = sabTomle( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = sabTomae( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

IDE_RC stfRelation::saeTogci( iduMemory*                         aQmxMem,
                              const stdPolygon2DType*            aObj1,
                              const stdGeoCollection2DType*      aObj2,
                              SChar*                             aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
                sRet = spiTosae( &sGeom->point2D.mPoint, aObj1 );
                break;
            case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
                sRet = sliTosae( &sGeom->linestring2D, aObj1 );
                break;
            case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTosae( aQmxMem, &sGeom->polygon2D, aObj1, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
                sRet = saeTompi( aObj1, &sGeom->mpoint2D );
                break;
            case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = saeTomli( aObj1, &sGeom->mlinestring2D );
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( saeTomai( aQmxMem, aObj1, &sGeom->mpolygon2D, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }
        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::saeTogcb( const stdPolygon2DType*            aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTosae( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTosae( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = saeTomlb( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = saeTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}
// multipoint vs multipoint
SChar stfRelation::mpiTompi( const stdMultiPoint2DType*        aObj1,
                             const stdMultiPoint2DType*        aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTompi(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::mpiTompe( const stdMultiPoint2DType*        aObj1,
                             const stdMultiPoint2DType*        aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTompe(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

// multipoint vs multilinestring
SChar stfRelation::mpiTomli( const stdMultiPoint2DType*             aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTomli(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::mpiTomlb( const stdMultiPoint2DType*             aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTomlb(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::mpiTomle( const stdMultiPoint2DType*             aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTomle(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::mpeTomlb( const stdMultiPoint2DType*             aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        if( slbTompe(sLine, aObj1) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

// multipoint vs multipolygon
SChar stfRelation::mpiTomai( const stdMultiPoint2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTomai(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::mpiTomab( const stdMultiPoint2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTomab(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

SChar stfRelation::mpiTomae( const stdMultiPoint2DType*         aObj1,
                             const stdMultiPolygon2DType*       aObj2 )
{
    stdPoint2DType* sPoint;
    UInt            i, sMax;

    sPoint = STD_FIRST_POINT2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( spiTomae(&sPoint->mPoint, aObj2) == '0' )
        {
            return '0';
        }
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    return 'F';
}

// multipoint vs geometrycollection
SChar stfRelation::mpiTogci( const stdMultiPoint2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            if(spiTompi( &sGeom->point2D.mPoint, aObj1 ) == '0')
            {
                return '0';
            }
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            if(sliTompi( &sGeom->linestring2D, aObj1 ) == '0')
            {
                return '0';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            if(saiTompi( &sGeom->polygon2D, aObj1 ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            if(mpiTompi( aObj1, &sGeom->mpoint2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            if(mpiTomli( aObj1, &sGeom->mlinestring2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            if(mpiTomai( aObj1, &sGeom->mpolygon2D ) == '0')
            {
                return '0';
            }
            break;
        default:
            return 'F';
        }
        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::mpiTogcb( const stdMultiPoint2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            if(slbTompi( &sGeom->linestring2D, aObj1 ) == '0')
            {
                return '0';
            }
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            if(sabTompi( &sGeom->polygon2D, aObj1 ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            if(mpiTomlb( aObj1, &sGeom->mlinestring2D ) == '0')
            {
                return '0';
            }
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            if(mpiTomab( aObj1, &sGeom->mpolygon2D ) == '0')
            {
                return '0';
            }
            break;
        default:
            return 'F';
        }
        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::mpiTogce( const stdMultiPoint2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = speTompi( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sleTompi( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saeTompi( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = mpiTompe( aObj1, &sGeom->mpoint2D );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mpiTomle( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mpiTomae( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mpeTogci( const stdMultiPoint2DType*         aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTompe( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            return '2';
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = mpiTompe( &sGeom->mpoint2D, aObj1 );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            return '2';
        default:
            return 'F';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mpeTogcb( const stdMultiPoint2DType*    /*    aObj1    */,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);
    
    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = '0';
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            return '1';
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = '0';
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            return '1';
        default:
            return 'F';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}
// multilinestring vs multilinestring
SChar stfRelation::mliTomli( const stdMultiLineString2DType*        aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;
    UInt                    i, sMax;

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTomli(sLine, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mliTomlb( const stdMultiLineString2DType*        aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( sliTomlb(sLine, aObj2) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::mliTomle( const stdMultiLineString2DType*        aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTomle(sLine, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mlbTomlb( const stdMultiLineString2DType*        aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }
    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj2) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( slbTomlb(sLine, aObj2) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::mlbTomle( const stdMultiLineString2DType*        aObj1,
                             const stdMultiLineString2DType*        aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( slbTomle(sLine, aObj2) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

// multilinestring vs multipolygon
SChar stfRelation::mliTomai( const stdMultiLineString2DType*        aObj1,
                             const stdMultiPolygon2DType*           aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTomai(sLine, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mliTomab( const stdMultiLineString2DType*        aObj1,
                             const stdMultiPolygon2DType*           aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTomab(sLine, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mliTomae( const stdMultiLineString2DType*        aObj1,
                             const stdMultiPolygon2DType*           aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        sRet = sliTomae(sLine, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sLine = STD_NEXT_LINE2D(sLine);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mlbTomai( const stdMultiLineString2DType*        aObj1,
                             const stdMultiPolygon2DType*           aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( slbTomai(sLine, aObj2) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::mlbTomab( const stdMultiLineString2DType*        aObj1,
                             const stdMultiPolygon2DType*           aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( slbTomab(sLine, aObj2) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::mlbTomae( const stdMultiLineString2DType*        aObj1,
                             const stdMultiPolygon2DType*           aObj2 )
{
    stdLineString2DType*    sLine;
    UInt                    i, sMax;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sLine = STD_FIRST_LINE2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        if( slbTomae(sLine, aObj2) == '0' )
        {
            return '0';
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    return 'F';
}

SChar stfRelation::mleTomab( const stdMultiLineString2DType*        aObj1,
                             const stdMultiPolygon2DType*           aObj2 )
{
    stdPolygon2DType*       sPoly;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj2);
    sMax = STD_N_OBJECTS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        sRet = sabTomle(sPoly, aObj1);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

// multilinestring vs geometrycollection
SChar stfRelation::mliTogci( const stdMultiLineString2DType*        aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTomli( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTomli( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saiTomli( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = mpiTomli( &sGeom->mpoint2D, aObj1 );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mliTomli( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mliTomai( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mliTogcb( const stdMultiLineString2DType*        aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTomli( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTomli( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mliTomlb( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mliTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mliTogce( const stdMultiLineString2DType*         aObj1,
                             const stdGeoCollection2DType*           aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sleTomli( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saeTomli( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mliTomle( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mliTomae( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mlbTogci( const stdMultiLineString2DType*        aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTomlb( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTomlb( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saiTomlb( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = mpiTomlb( &sGeom->mpoint2D, aObj1 );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mliTomlb( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mlbTomai( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }

        if( sRet == '0' )
        {
            return '0';
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::mlbTogcb( const stdMultiLineString2DType*        aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTomlb( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTomlb( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTomlb( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mlbTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }

        if( sRet == '0' )
        {
            return '0';
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    return 'F';
}

SChar stfRelation::mlbTogce( const stdMultiLineString2DType*     aObj1,
                             const stdGeoCollection2DType*       aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    if( stdUtils::isClosed2D((stdGeometryHeader*)aObj1) )
    {
        return 'F';
    }

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = '0';
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sleTomlb( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saeTomlb( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = '0';
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTomle( aObj1, &sGeom->mlinestring2D );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mlbTomae( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mleTogci( const stdMultiLineString2DType*        aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTomle( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTomle( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = '2';
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = mpiTomle( &sGeom->mpoint2D, aObj1 );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mliTomle( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = '2';
            break;
        default:
            return 'F';
        }

        if( sRet == '2' )
        {
            return '2';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mleTogcb( const stdMultiLineString2DType*        aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTomle( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTomle( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTomle( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mleTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

// multipolygon vs multipolygon
IDE_RC stfRelation::maiTomai( iduMemory*                          aQmxMem,
                              const stdMultiPolygon2DType*        aObj1,
                              const stdMultiPolygon2DType*        aObj2,
                              SChar*                              aResult )
{
    stdPolygon2DType*       sPoly;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        IDE_TEST( saiTomai(aQmxMem, sPoly, aObj2, &sRet)
                  != IDE_SUCCESS );

        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::maiTomab( const stdMultiPolygon2DType*        aObj1,
                             const stdMultiPolygon2DType*        aObj2 )
{
    stdPolygon2DType*       sPoly;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = saiTomab(sPoly, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

IDE_RC stfRelation::maiTomae( iduMemory*                          aQmxMem,
                              const stdMultiPolygon2DType*        aObj1,
                              const stdMultiPolygon2DType*        aObj2,
                              SChar*                              aResult )
{
    stdPolygon2DType*       sPoly;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);
    
    for( i = 0; i < sMax; i++ )
    {
        IDE_TEST( saiTomae(aQmxMem, sPoly, aObj2, &sRet)
                  != IDE_SUCCESS );

        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::mabTomab( const stdMultiPolygon2DType*        aObj1,
                             const stdMultiPolygon2DType*        aObj2 )
{
    stdPolygon2DType*       sPoly;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        sRet = sabTomab(sPoly, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mabTomae( const stdMultiPolygon2DType*        aObj1,
                             const stdMultiPolygon2DType*        aObj2 )
{
    stdPolygon2DType*       sPoly;
    UInt                    i, sMax;
    SChar                   sRet = 0x00;
    SChar                   sResult = 0x00;

    sPoly = STD_FIRST_POLY2D(aObj1);
    sMax = STD_N_OBJECTS(aObj1);
    
    for( i = 0; i < sMax; i++ )
    {
        sRet = sabTomae(sPoly, aObj2);

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}
// multipolygon vs geometrycollection
IDE_RC stfRelation::maiTogci( iduMemory*                             aQmxMem,
                              const stdMultiPolygon2DType*           aObj1,
                              const stdGeoCollection2DType*          aObj2,
                              SChar*                                 aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
            case STD_POINT_2D_EXT_TYPE:
                sRet = spiTomai( &sGeom->point2D.mPoint, aObj1 );
                break;
            case STD_LINESTRING_2D_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
                sRet = sliTomai( &sGeom->linestring2D, aObj1 );
                break;
            case STD_POLYGON_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTomai( aQmxMem, &sGeom->polygon2D, aObj1, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
            case STD_MULTIPOINT_2D_EXT_TYPE:
                sRet = mpiTomai( &sGeom->mpoint2D, aObj1 );
                break;
            case STD_MULTILINESTRING_2D_TYPE:
            case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = mliTomai( &sGeom->mlinestring2D, aObj1 );
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( maiTomai( aQmxMem, aObj1, &sGeom->mpolygon2D, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }

        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }


        sGeom = STD_NEXT_GEOM(sGeom);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::maiTogcb( const stdMultiPolygon2DType*       aObj1,
                             const stdGeoCollection2DType*      aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTomai( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTomai( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTomai( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = maiTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

IDE_RC stfRelation::maiTogce( iduMemory*                          aQmxMem,
                              const stdMultiPolygon2DType*        aObj1,
                              const stdGeoCollection2DType*       aObj2,
                              SChar*                              aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
            case STD_POINT_2D_EXT_TYPE:
            case STD_LINESTRING_2D_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
                sRet = '2';
                break;
            case STD_POLYGON_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saeTomai( aQmxMem, &sGeom->polygon2D, aObj1, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
            case STD_MULTIPOINT_2D_EXT_TYPE:
            case STD_MULTILINESTRING_2D_TYPE:
            case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = '2';
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( maiTomae( aQmxMem, aObj1, &sGeom->mpolygon2D, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            sResult = sRet;
            break;
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::mabTogci( const stdMultiPolygon2DType*           aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTomab( &sGeom->point2D.mPoint, aObj1 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTomab( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saiTomab( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = mpiTomab( &sGeom->mpoint2D, aObj1 );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mliTomab( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = maiTomab( &sGeom->mpolygon2D, aObj1 );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::mabTogcb( const stdMultiPolygon2DType*           aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTomab( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTomab( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTomab( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mabTomab( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}
SChar stfRelation::mabTogce( const stdMultiPolygon2DType*        aObj1,
                             const stdGeoCollection2DType*       aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sleTomab( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saeTomab( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = '1';
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mleTomab( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mabTomae( aObj1, &sGeom->mpolygon2D );
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

IDE_RC stfRelation::maeTogci( iduMemory*                             aQmxMem,
                              const stdMultiPolygon2DType*           aObj1,
                              const stdGeoCollection2DType*          aObj2,
                              SChar*                                 aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
            case STD_POINT_2D_EXT_TYPE:
                sRet = spiTomae( &sGeom->point2D.mPoint, aObj1 );
                break;
            case STD_LINESTRING_2D_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
                sRet = sliTomae( &sGeom->linestring2D, aObj1 );
                break;
            case STD_POLYGON_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTomae( aQmxMem, &sGeom->polygon2D, aObj1, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
            case STD_MULTIPOINT_2D_EXT_TYPE:
                sRet = mpiTomae( &sGeom->mpoint2D, aObj1 );
                break;
            case STD_MULTILINESTRING_2D_TYPE:
            case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = mliTomae( &sGeom->mlinestring2D, aObj1 );
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( maiTomae( aQmxMem, &sGeom->mpolygon2D, aObj1, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }

        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::maeTogcb( const stdMultiPolygon2DType*           aObj1,
                             const stdGeoCollection2DType*          aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj2);
    sMax = STD_N_GEOMS(aObj2);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTomae( &sGeom->linestring2D, aObj1 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTomae( &sGeom->polygon2D, aObj1 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTomae( &sGeom->mlinestring2D, aObj1 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mabTomae( &sGeom->mpolygon2D, aObj1 );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

// geometrycollection vs geometrycollection
IDE_RC stfRelation::gciTogci( iduMemory*                           aQmxMem,
                              const stdGeoCollection2DType*        aObj1,
                              const stdGeoCollection2DType*        aObj2,
                              SChar*                               aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj1);
    sMax = STD_N_GEOMS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
            case STD_POINT_2D_EXT_TYPE:
                sRet = spiTogci( &sGeom->point2D.mPoint, aObj2 );
                break;
            case STD_LINESTRING_2D_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
                sRet = sliTogci( &sGeom->linestring2D, aObj2 );
                break;
            case STD_POLYGON_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTogci( aQmxMem, &sGeom->polygon2D, aObj2, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
            case STD_MULTIPOINT_2D_EXT_TYPE:
                sRet = mpiTogci( &sGeom->mpoint2D, aObj2 );
                break;
            case STD_MULTILINESTRING_2D_TYPE:
            case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = mliTogci( &sGeom->mlinestring2D, aObj2 );
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( maiTogci( aQmxMem, &sGeom->mpolygon2D, aObj2, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }

        if( sRet == '2' )
        {
            sResult = sRet;
            break;
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::gciTogcb( const stdGeoCollection2DType*        aObj1,
                             const stdGeoCollection2DType*        aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj1);
    sMax = STD_N_GEOMS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            sRet = spiTogcb( &sGeom->point2D.mPoint, aObj2 );
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = sliTogcb( &sGeom->linestring2D, aObj2 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = saiTogcb( &sGeom->polygon2D, aObj2 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            sRet = mpiTogcb( &sGeom->mpoint2D, aObj2 );
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mliTogcb( &sGeom->mlinestring2D, aObj2 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = maiTogcb( &sGeom->mpolygon2D, aObj2 );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

IDE_RC stfRelation::gciTogce( iduMemory*                          aQmxMem,
                              const stdGeoCollection2DType*       aObj1,
                              const stdGeoCollection2DType*       aObj2,
                              SChar*                              aResult )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj1);
    sMax = STD_N_GEOMS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
            case STD_POINT_2D_TYPE:
            case STD_POINT_2D_EXT_TYPE:
                sRet = spiTogce( &sGeom->point2D.mPoint, aObj2 );
                break;
            case STD_LINESTRING_2D_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
                sRet = sliTogce( &sGeom->linestring2D, aObj2 );
                break;
            case STD_POLYGON_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( saiTogce( aQmxMem, &sGeom->polygon2D, aObj2, &sRet )
                          != IDE_SUCCESS );
                break;
            case STD_MULTIPOINT_2D_TYPE:
            case STD_MULTIPOINT_2D_EXT_TYPE:
                sRet = mpiTogce( &sGeom->mpoint2D, aObj2 );
                break;
            case STD_MULTILINESTRING_2D_TYPE:
            case STD_MULTILINESTRING_2D_EXT_TYPE:
                sRet = mliTogce( &sGeom->mlinestring2D, aObj2 );
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( maiTogce( aQmxMem, &sGeom->mpolygon2D, aObj2, &sRet )
                          != IDE_SUCCESS );
                break;
            default:
                sResult = 'F';
                IDE_RAISE( normal_exit );
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            sResult = sRet;
            break;
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        sResult = 'F';
    }

    IDE_EXCEPTION_CONT( normal_exit );
    
    *aResult = sResult;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

SChar stfRelation::gcbTogcb( const stdGeoCollection2DType*        aObj1,
                             const stdGeoCollection2DType*        aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj1);
    sMax = STD_N_GEOMS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTogcb( &sGeom->linestring2D, aObj2 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTogcb( &sGeom->polygon2D, aObj2 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTogcb( &sGeom->mlinestring2D, aObj2 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mabTogcb( &sGeom->mpolygon2D, aObj2 );
            break;
        default:
            return 'F';
        }

        if( sRet == '1' )
        {
            return '1';
        }

        // Fix BUG-36086
        if ( sRet != 'F' )
        {

            if ( sResult < sRet )
            {
                sResult = sRet;
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* Nothing To Do */
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}

SChar stfRelation::gcbTogce( const stdGeoCollection2DType*       aObj1,
                             const stdGeoCollection2DType*       aObj2 )
{
    stdGeometryType*    sGeom;
    UInt                i, sMax;
    SChar               sRet = 0x00;
    SChar               sResult = 0x00;

    sGeom = STD_FIRST_COLL2D(aObj1);
    sMax = STD_N_GEOMS(aObj1);

    for( i = 0; i < sMax; i++ )
    {
        switch( sGeom->header.mType )
        {
        case STD_POINT_2D_TYPE:
        case STD_POINT_2D_EXT_TYPE:
            break;
        case STD_LINESTRING_2D_TYPE:
        case STD_LINESTRING_2D_EXT_TYPE:
            sRet = slbTogce( &sGeom->linestring2D, aObj2 );
            break;
        case STD_POLYGON_2D_TYPE:
        case STD_POLYGON_2D_EXT_TYPE:
            sRet = sabTogce( &sGeom->polygon2D, aObj2 );
            break;
        case STD_MULTIPOINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
            break;
        case STD_MULTILINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            sRet = mlbTogce( &sGeom->mlinestring2D, aObj2 );
            break;
        case STD_MULTIPOLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            sRet = mabTogce( &sGeom->mpolygon2D, aObj2 );
            break;
        default:
            return 'F';
        }
        if( sRet == 'F' )     // 나올 수 있는 최저 차원
        {
            return 'F';
        }
        if( sResult == 0x00 )
        {
            sResult = sRet;
        }
        else if( sResult > sRet )
        {
            sResult = sRet;
        }

        sGeom = STD_NEXT_GEOM(sGeom);
    }
    if( sResult == 0x00 )
    {
        return 'F';
    }

    return sResult;
}
/***********************************************************************
 *
 * Description :
 *
 *    Line Segment와 Ring Segment가 (내부/외부) 교차가 존재하는 지 검사
 *
 * Implementation :
 *
 *    Line Segment와 Ring Segment가 한 점을 중심으로 교차할 때,
 *    둘 간의 원하는 교차 관계(내부/외부)가 존재하는 지 검사함.
 *
 *               Ap
 *               |
 *               |
 *        Lp-----M-----Ln
 *               |
 *               |
 *               An
 *
 ***********************************************************************/

idBool
stfRelation::hasRelLineSegRingSeg( idBool      aIsExtRing,
                                   idBool      aIsCCWiseRing,
                                   stdPoint2D* aRingPrevPt,
                                   stdPoint2D* aRingNextPt,
                                   stdPoint2D* aMeetPoint,
                                   stdPoint2D* aLinePrevPt,
                                   stdPoint2D* aLineNextPt,
                                   stfAnglePos aWantPos )
{
    idBool sResult;

    stfAnglePos  sRevPos;
    stfAnglePos  sLinePrevAnglePos;
    stfAnglePos  sLineNextAnglePos;

    SDouble      sTriangleArea;
    
    //----------------------------
    // Parameter Validation
    //----------------------------

    IDE_DASSERT( aRingPrevPt != NULL );
    IDE_DASSERT( aRingNextPt != NULL );
    IDE_DASSERT( aMeetPoint != NULL );
    IDE_DASSERT( (aWantPos == STF_INSIDE_ANGLE_POS) ||
                 (aWantPos == STF_OUTSIDE_ANGLE_POS) );

    //----------------------------
    // Initialization
    //----------------------------

    sResult = ID_FALSE;
    sRevPos = ( aWantPos == STF_INSIDE_ANGLE_POS ) ?
        STF_OUTSIDE_ANGLE_POS : STF_INSIDE_ANGLE_POS;
    
    //----------------------------
    // 판단을 위한 부가 정보 생성
    //----------------------------
    
    // Area > 0 : 시계반대방향으로 생성된 면
    // Area < 0 : 시계방향으로 생성된 면
    // Area = 0 : 세점이 일직선
    
    sTriangleArea = stdUtils::area2D( aRingPrevPt,
                                      aMeetPoint,
                                      aRingNextPt );
    
    // 라인이 링이 이루는 각에 포함되는 지 검사
    if ( aLinePrevPt != NULL )
    {
        sLinePrevAnglePos =
            stfRelation::wherePointInAngle( aRingPrevPt,
                                            aMeetPoint,
                                            aRingNextPt,
                                            aLinePrevPt );
    }
    else
    {
        sLinePrevAnglePos = STF_UNKNOWN_ANGLE_POS;
    }
    
    if ( aLineNextPt != NULL )
    {
        sLineNextAnglePos =
            stfRelation::wherePointInAngle( aRingPrevPt,
                                            aMeetPoint,
                                            aRingNextPt,
                                            aLineNextPt );
    }
    else
    {
        sLineNextAnglePos = STF_UNKNOWN_ANGLE_POS;
    }
    

    //----------------------------
    // Area 와 Angle을 이용한 교차 여부 판단
    //----------------------------

    if ( ( (aIsExtRing == ID_TRUE) && (aIsCCWiseRing == ID_TRUE )) ||
         ( (aIsExtRing != ID_TRUE) && (aIsCCWiseRing != ID_TRUE ) ) )
    {
        if ( sTriangleArea > 0 )
        {
            //             |         Area     In-->>--
            //      Area   An                 |
            //             |                  |
            //  ->>--Ap----A         Ip-->>---I
            //                       |
            
            if ( (sLinePrevAnglePos == aWantPos)||
                 (sLineNextAnglePos == aWantPos) )
            {
                sResult = ID_TRUE;
                IDE_RAISE( HAS_RELATION_MAX_RESULT );
            }
            else
            {
                // 교차 여부를 판단할 수 없음
            }
        }
        else if ( sTriangleArea < 0 )
        {
            //   Area     |
            //            |
            //       A----An
            //       |     
            //  ->>--Ap

            if ( (sLinePrevAnglePos == sRevPos)||
                 (sLineNextAnglePos == sRevPos) )
            {
                sResult = ID_TRUE;
                IDE_RAISE( HAS_RELATION_MAX_RESULT );
            }
            else
            {
                // 교차 여부를 판단할 수 없음
            }
                            
        }
        else // sTriangleArea == 0
        {
            // 직선인 경우
            //
            // A--->>--Ap---Ac---An-->>--A

            if ( (sLinePrevAnglePos == aWantPos)||
                 (sLineNextAnglePos == aWantPos) )
            {
                sResult = ID_TRUE;
                IDE_RAISE( HAS_RELATION_MAX_RESULT );
            }
            else
            {
                // 교차 여부를 판단할 수 없음
            }
        }
    }
    else // External Not CCW, Internal CCW
    {
        if ( sTriangleArea > 0 )
        {
            //  Area       |      Area   I--------Ip--<<--
            //       A-----Ap            |  
            //       |                   |   
            //  -<<--An                  In
            //                           
                            
            if ( (sLinePrevAnglePos == sRevPos)||
                 (sLineNextAnglePos == sRevPos) )
            {
                sResult = ID_TRUE;
                IDE_RAISE( HAS_RELATION_MAX_RESULT );
            }
            else
            {
                // 교차 여부를 판단할 수 없음
            }
        }
        else if ( sTriangleArea < 0 )
        {
            //             |   
            //      Area   Ap  
            //             |   
            //  -<<--An----A   
            //
            if ( (sLinePrevAnglePos == aWantPos)||
                 (sLineNextAnglePos == aWantPos) )
            {
                sResult = ID_TRUE;
                IDE_RAISE( HAS_RELATION_MAX_RESULT );
            }
            else
            {
                // 교차 여부를 판단할 수 없음
            }
        }
        else // sTriangleArea == 0
        {
            // 직선인 경우  Area
            //
            // A---<<--An---Ac---Ap--<<--A

            if ( (sLinePrevAnglePos == sRevPos)||
                 (sLineNextAnglePos == sRevPos) )
            {
                sResult = ID_TRUE;
                IDE_RAISE( HAS_RELATION_MAX_RESULT );
            }
            else
            {
                // 교차 여부를 판단할 수 없음
            }

        }
    }

    IDE_EXCEPTION_CONT( HAS_RELATION_MAX_RESULT );

    return sResult;
}

/***********************************************************************
 *
 * Description :
 *
 *    Point가 Angle상의 어디에 위치하는 지의 판단
 *
 * Implementation :
 *
 *     Angle상에서의 점의 위치
 *
 *                  An
 *        1         |                               1
 *                  |        
 *                  4     2              Ap---3->>--Am---4---An
 *                  |
 *      Ap---->>-3--Am                              2
 *
 *                2
 *
 *   1 : INSIDE
 *   2 : OUTSIDE
 *   3 : MIN
 *   4 : MAX
 *
 ***********************************************************************/

stfAnglePos
stfRelation::wherePointInAngle( stdPoint2D * aAnglePrevPt,
                                stdPoint2D * aAngleMiddPt,
                                stdPoint2D * aAngleNextPt,
                                stdPoint2D * aTestPt )
{
    stfAnglePos sResult;
        
    // Angle(Ap, Am, An)이 이루는 각과 면적
    SDouble sAngleArea;
    SDouble sAngleAngle;

    // Point(Ap, Am, P)가 이루는 각과 면적
    SDouble sPointArea;
    SDouble sPointAngle;

    // 일직선 상에 존재하는지 위해 사용
    SDouble sCorrArea;
    
    //--------------------------------------
    // Parameter Validation
    //--------------------------------------

    IDE_DASSERT( aAnglePrevPt != NULL );
    IDE_DASSERT( aAngleMiddPt != NULL );
    IDE_DASSERT( aAngleNextPt != NULL );
    IDE_DASSERT( aTestPt != NULL );
    
    //--------------------------------------
    // Initialization
    //--------------------------------------

    sResult = STF_UNKNOWN_ANGLE_POS;

    // 보정을 위해 일직선상에 존재하는 지 판단
    // 다음과 같이 대각선에 위치할때
    // (Ap,Am,An)과 (Ap,Am,P)가 다른 Angle값이 나올수 있다.
    //
    //                   An
    //                  +  
    //                 P
    //                +
    //        Ap-----Am

    // 일직선상이라면 Area는 보정되어 0 값이 나온다.
    sCorrArea = stdUtils::area2D( aAngleMiddPt,
                                  aAngleNextPt,
                                  aTestPt );
        
    //--------------------------------------
    // 면적과 각을 구한다.
    //--------------------------------------
    
    // Area > 0 : 시계반대방향으로 생성된 면
    // Area < 0 : 시계방향으로 생성된 면
    // Area = 0 : 세점이 일직선

    // 0 <= Angle <= 3.141592XXX
    
    sAngleArea = stdUtils::area2D( aAnglePrevPt,
                                   aAngleMiddPt,
                                   aAngleNextPt );
    sAngleAngle = stdUtils::getAngle2D( aAnglePrevPt,
                                        aAngleMiddPt,
                                        aAngleNextPt );

    sPointArea = stdUtils::area2D( aAnglePrevPt,
                                   aAngleMiddPt,
                                   aTestPt );
    
    sPointAngle = stdUtils::getAngle2D( aAnglePrevPt,
                                        aAngleMiddPt,
                                        aTestPt );

    //--------------------------------------
    // 면적과 각을 이용한 포함 여부 판별
    //--------------------------------------

    if ( sAngleArea > 0 )
    {
        //                 An
        //                 |
        //                 |
        //                 |
        //    Ap---->>-----Am
        
        if ( sPointArea > 0 )
        {
            if ( sCorrArea == 0 )
            {
                // sPointAngle == sAngleAngle 을 판단
                // Am-->P-->An 이 직선임
                sResult = STF_MAX_ANGLE_POS;
            }
            else
            {
                if ( sPointAngle > sAngleAngle )
                {
                    //                 An
                    //                 |       P
                    //                 |       
                    //                 |
                    //    Ap---->>-----Am
                    
                    sResult = STF_OUTSIDE_ANGLE_POS;
                }
                else // sPointAngle < sAngleAngle )
                {
                    sResult = STF_INSIDE_ANGLE_POS;
                }
            }
        }
        else if ( sPointArea < 0 )
        {
            //                 An
            //                 |    
            //                 |       
            //                 |
            //    Ap---->>-----Am
            //
            //                     P

            sResult = STF_OUTSIDE_ANGLE_POS;
        }
        else // sPointArea == 0
        {
            // 3.14보다 큰 것으로 비교한 이유
            // 0 또는 3.141592XXXX가 나와야 하지만,
            // 대각선으로 존재할때는 0에 가까운 아주 작은 수가 나온다.
            
            if ( sPointAngle > 3.14 )
            {
                //                 An
                //                 |    
                //                 |       
                //                 |
                //    Ap---->>-----Am        P
                
                sResult = STF_OUTSIDE_ANGLE_POS;
            }
            else
            {
                //                 An
                //                 |    
                //                 |       
                //                 |
                //    Ap---->>-P---Am
                
                sResult = STF_MIN_ANGLE_POS;
            }
        }
    }
    else if ( sAngleArea < 0 )
    {
        //     Am---->>-----An
        //     |
        //     |
        //     |
        //     Ap

        if ( sPointArea > 0 )
        {
            //
            //       Am---->>-----An
            // P     |
            //       |
            //       |
            //       Ap
            
            sResult = STF_OUTSIDE_ANGLE_POS;
        }
        else if ( sPointArea < 0 )
        {
            if ( sCorrArea == 0 )
            {
                // sPointAngle == sAngleAngle 을 판단
                // Am-->P-->An 이 직선임
                sResult = STF_MAX_ANGLE_POS;
            }
            else
            {
                if ( sPointAngle > sAngleAngle )
                {
                    //
                    //           P
                    //
                    //       Am---->>-----An
                    //       |
                    //       |
                    //       |
                    //       Ap
                    
                    sResult = STF_OUTSIDE_ANGLE_POS;
                }
                else
                {
                    //
                    //           
                    //
                    //       Am---->>-----An
                    //       |
                    //       |   P
                    //       |
                    //       Ap
                    
                    sResult = STF_INSIDE_ANGLE_POS;
                }
            }
        }
        else // sPointArea == 0
        {
            if ( sPointAngle > 3.14 )
            {
                //
                //       P    
                //
                //       Am---->>-----An
                //       |
                //       |
                //       |
                //       Ap
                
                sResult = STF_OUTSIDE_ANGLE_POS;
            }
            else
            {
                //
                //           
                //
                //       Am---->>-----An
                //       |
                //       P
                //       |
                //       Ap
                
                sResult = STF_MIN_ANGLE_POS;
            }
        }
    }
    else // sAngleArea == 0
    {
        //
        //     Ap-->>--Am-->>---An
        //
        
        if ( sPointArea > 0 )
        {
            //           P
            //
            //     Ap-->>--Am-->>---An
            //
            
            sResult = STF_INSIDE_ANGLE_POS;
        }
        else if ( sPointArea < 0 )
        {
            //
            //     Ap-->>--Am-->>---An
            //
            //           P
            
            sResult = STF_OUTSIDE_ANGLE_POS;
        }
        else // sPointArea == 0
        {
            if ( sPointAngle > 3.14 )
            {
                //
                //     Ap-->>---Am-->>P---An
                //
            
                sResult = STF_MAX_ANGLE_POS;
            }
            else
            {
                //
                //     Ap-->>-P--Am-->>---An
                //
                
                sResult = STF_MIN_ANGLE_POS;
            }
        }
    }

    return sResult;
}

IDE_RC stfRelation::relateAreaArea( iduMemory*             aQmxMem,
                                    const stdGeometryType* aObj1, 
                                    const stdGeometryType* aObj2,
                                    SChar*                 aPattern,
                                    mtdBooleanType*        aReturn )
{
    stdPolygon2DType*   sPoly1;
    stdPolygon2DType*   sPoly2;
    UInt                i, j , sMax1, sMax2;
    ULong               sTotalPoint      = 0;
    ULong               sTotalRing       = 0;
    Segment**           sIndexSeg;
    Segment**           sTempIndexSeg;    
    Segment**           sReuseSeg;   
    stdLinearRing2D*    sRing;
    UInt                sMaxR;
    UInt                sMaxP;
    UInt                sIndexSegTotal   = 0;
    iduPriorityQueue    sPQueue;
    idBool              sOverflow        = ID_FALSE;
    idBool              sUnderflow       = ID_FALSE;
    Segment*            sCurrNext;
    Segment*            sCurrPrev;
    Segment*            sCurrSeg;
    Segment*            sCmpSeg;
    ULong               sReuseSegCount   = 0;
    SInt                sIntersectStatus;
    SInt                sInterCount;
    stdPoint2D          sInterResult[4];
    PrimInterSeg*       sPrimInterSeg    = NULL;
    Segment**           sRingSegList;
    UInt                sRingCount       = 0;
    SChar               sResultMatrix[9] = { STF_INTERSECTS_DIM_NOT, STF_INTERSECTS_DIM_NOT, STF_INTERSECTS_DIM_NOT,
                                             STF_INTERSECTS_DIM_NOT, STF_INTERSECTS_DIM_NOT, STF_INTERSECTS_DIM_NOT,
                                             STF_INTERSECTS_DIM_NOT, STF_INTERSECTS_DIM_NOT, STF_INTERSECTS_DIM_2 };    
    stdRepPoint*        sRepPoints;
    SInt                sPointArea;
    SDouble             sCmpMinMaxX;
    SDouble             sCmpMaxMinX;

    sMax1 = stdUtils::getGeometryNum((stdGeometryHeader*)aObj1);
    sMax2 = stdUtils::getGeometryNum((stdGeometryHeader*)aObj2);

    sCmpMinMaxX = ( ((stdGeometryHeader*)aObj1)->mMbr.mMinX > ((stdGeometryHeader*)aObj2)->mMbr.mMinX )  ?
                    ((stdGeometryHeader*)aObj1)->mMbr.mMinX :
                    ((stdGeometryHeader*)aObj2)->mMbr.mMinX ;

    sCmpMaxMinX = ( ((stdGeometryHeader*)aObj1)->mMbr.mMaxX > ((stdGeometryHeader*)aObj2)->mMbr.mMaxX )  ?
                    ((stdGeometryHeader*)aObj2)->mMbr.mMaxX :
                    ((stdGeometryHeader*)aObj1)->mMbr.mMaxX ;

    /* 각 폴리곤의 대표 점을 저장할 배열 할당 */
    IDE_TEST( aQmxMem->alloc( (sMax1 + sMax2) * ID_SIZEOF(stdRepPoint),
                              (void**) & sRepPoints )
              != IDE_SUCCESS);

    sPoly1 = (stdPolygon2DType*)stdUtils::getFirstGeometry(
                                        (stdGeometryHeader*)aObj1);

    for( i = 0; i < sMax1; i++ ) 
    {
        IDE_TEST_RAISE( sPoly1 == NULL, err_invalid_object_mType );

        sRing       = STD_FIRST_RN2D(sPoly1);
        sMaxR       = STD_N_RINGS(sPoly1);
        sTotalRing += sMaxR;

        sRepPoints[i].mPoint = STD_FIRST_PT2D( sRing );

        for ( j = 0; j < sMaxR; j++)
        {
            sMaxP        = STD_N_POINTS(sRing);
            sTotalPoint += sMaxP;
            /* BUG-45528 링을 포함한 멀티폴리곤/폴리곤의 연산 시에 메모리 할당 오류가 있습니다. */
            sRing        = STD_NEXT_RN2D(sRing);
        }

        sPoly1 = (stdPolygon2DType*)stdUtils::getNextGeometry(
                                            (stdGeometryHeader*)sPoly1);
    }

    sPoly2 = (stdPolygon2DType*)stdUtils::getFirstGeometry(
                                        (stdGeometryHeader*)aObj2);

    for( i = 0; i < sMax2; i++ ) 
    {
        IDE_TEST_RAISE( sPoly2 == NULL, err_invalid_object_mType );

        sRing       = STD_FIRST_RN2D(sPoly2);
        sMaxR       = STD_N_RINGS(sPoly2);
        sTotalRing += sMaxR;
        
        sRepPoints[i+sMax1].mPoint = STD_FIRST_PT2D( sRing );
        
        for ( j = 0; j < sMaxR; j++ )
        {
            sMaxP        = STD_N_POINTS(sRing);
            sTotalPoint += sMaxP;
            /* BUG-45528 링을 포함한 멀티폴리곤/폴리곤의 연산 시에 메모리 할당 오류가 있습니다. */
            sRing        = STD_NEXT_RN2D(sRing);
        }
        
        sPoly2 = (stdPolygon2DType*)stdUtils::getNextGeometry(
                                            (stdGeometryHeader*)sPoly2);
    }

    /* BUG-33634 
     * 연산의 결과로 생성되는 폴리곤의 인덱스를 저장하기 위해서 
     * sIndexSeg의 크기를 수정함. 
     * sTempIndexSeg에 할당하는 부분을 삭제하고,
     * sRingSegList의 크기를 Ring의 개수에 맞게 수정함. */
    IDE_TEST( aQmxMem->alloc( 3 * sTotalPoint * ID_SIZEOF(Segment*),
                              (void**) & sIndexSeg )
              != IDE_SUCCESS);

    IDE_TEST( aQmxMem->alloc( sTotalRing * ID_SIZEOF(Segment*),
                              (void**) & sRingSegList )
              != IDE_SUCCESS);

    sPoly1 = (stdPolygon2DType*)stdUtils::getFirstGeometry(
                                        (stdGeometryHeader*)aObj1);
    
    for( i = 0; i < sMax1; i++ ) 
    {
        IDE_TEST_RAISE( sPoly1 == NULL, err_invalid_object_mType );

        j = sRingCount;
        
        IDE_TEST( stdUtils::classfyPolygonChain( aQmxMem,
                                                 sPoly1,
                                                 i, 
                                                 sIndexSeg,
                                                 &sIndexSegTotal,
                                                 sRingSegList,
                                                 &sRingCount,
                                                 ID_FALSE )  // For validation? 
                  != IDE_SUCCESS );

        sRepPoints[i].mIsValid = ( j == sRingCount ) ? ID_FALSE : ID_TRUE;

        sPoly1 = (stdPolygon2DType*)stdUtils::getNextGeometry(
                                            (stdGeometryHeader*)sPoly1);
    }

    sPoly2 = (stdPolygon2DType*)stdUtils::getFirstGeometry(
                                        (stdGeometryHeader*)aObj2);

    for( i = 0; i < sMax2; i++ ) 
    {
        IDE_TEST_RAISE( sPoly2 == NULL, err_invalid_object_mType );

        j = sRingCount;

        IDE_TEST( stdUtils::classfyPolygonChain( aQmxMem,
                                                 sPoly2,
                                                 i + sMax1, 
                                                 sIndexSeg,
                                                 &sIndexSegTotal,
                                                 sRingSegList,
                                                 &sRingCount,
                                                 ID_FALSE ) // For validation?
                  != IDE_SUCCESS );

        sRepPoints[i+sMax1].mIsValid = ( j == sRingCount ) ? ID_FALSE : ID_TRUE;
        
        sPoly2 = (stdPolygon2DType*)stdUtils::getNextGeometry(
                                            (stdGeometryHeader*)sPoly2);
    }

    /* 한점으로 이루어진 폴리곤들 끼리 먼저 비교를 수행 */
    for ( i = 0 ; i < sMax1 ; i++ )
    {
        if ( sRepPoints[i].mIsValid == ID_FALSE )
        {
            for ( j = sMax1 ; j < sMax1 + sMax2 ; j++ )
            {
                if ( sRepPoints[j].mIsValid == ID_FALSE )
                {
                    if ( stdUtils::isSamePoints2D( sRepPoints[i].mPoint, sRepPoints[j].mPoint ) 
                           == ID_TRUE )
                    {
                        setDE9MatrixValue( sResultMatrix, STF_INTER_INTER, STF_INTERSECTS_DIM_2 );
                        setDE9MatrixValue( sResultMatrix, STF_BOUND_BOUND, STF_INTERSECTS_DIM_1 );
                    }
                }
            }
        }
    }

    if ( sIndexSegTotal == 0 )
    {
        (void)checkRelateResult( aPattern, sResultMatrix, aReturn );
        IDE_RAISE( normal_exit );      
    }

    /* BUG-33436
     * sIndexSeg는 x좌표 순으로 정렬되어 있어야 한다. */
    iduHeapSort::sort( sIndexSeg, sIndexSegTotal, 
                       ID_SIZEOF(Segment*), cmpSegment );

    /* 각 대표점이 다른 폴리곤의 내부에 포함되는지 비교 수행 */
    for ( i = 0 ; i < sMax1; i++ )
    {
        sPointArea = stdUtils::isPointInside( sIndexSeg,
                                              sIndexSegTotal,
                                              sRepPoints[i].mPoint, 
                                              sMax1,
                                              sMax1 + sMax2 - 1 );

        switch ( sPointArea )
        {
        case ST_POINT_INSIDE:
            setDE9MatrixValue( sResultMatrix, STF_INTER_INTER,  STF_INTERSECTS_DIM_2 );
            setDE9MatrixValue( sResultMatrix, STF_BOUND_INTER,  STF_INTERSECTS_DIM_1 );
            setDE9MatrixValue( sResultMatrix, STF_EXTER_INTER,  STF_INTERSECTS_DIM_2 );
            break;
        case ST_POINT_ONBOUND:
            setDE9MatrixValue( sResultMatrix, STF_BOUND_BOUND,  STF_INTERSECTS_DIM_0 );
            break;
        case ST_POINT_OUTSIDE:
            setDE9MatrixValue( sResultMatrix, STF_INTER_EXTER,  STF_INTERSECTS_DIM_2 );
            break;
        default:
            IDE_DASSERT(0);
            break;
        }
    }

    for ( i = sMax1 ; i < sMax1 + sMax2; i++ )
    {
        sPointArea = stdUtils::isPointInside( sIndexSeg,
                                              sIndexSegTotal,
                                              sRepPoints[i].mPoint, 
                                              0,
                                              sMax1 - 1 );
        switch ( sPointArea )
        {
        case ST_POINT_INSIDE:
            setDE9MatrixValue( sResultMatrix, STF_INTER_INTER,  STF_INTERSECTS_DIM_2 );
            setDE9MatrixValue( sResultMatrix, STF_INTER_BOUND,  STF_INTERSECTS_DIM_1 );
            setDE9MatrixValue( sResultMatrix, STF_INTER_EXTER,  STF_INTERSECTS_DIM_2 );
            break;
        case ST_POINT_ONBOUND:
            setDE9MatrixValue( sResultMatrix, STF_BOUND_BOUND,  STF_INTERSECTS_DIM_0 );
            break;
        case ST_POINT_OUTSIDE:
            setDE9MatrixValue( sResultMatrix, STF_EXTER_INTER,  STF_INTERSECTS_DIM_2 );
            break;
        default:
            IDE_DASSERT(0);
            break;
        }
    }

    IDE_TEST_RAISE( checkRelateResult(aPattern, sResultMatrix, aReturn ) 
                    == ID_TRUE, normal_exit );

    IDE_TEST(sPQueue.initialize(aQmxMem,
                                sIndexSegTotal,
                                ID_SIZEOF(Segment*),
                                cmpSegment )
             != IDE_SUCCESS);
    
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Segment*) * sIndexSegTotal,
                              (void**)  &sReuseSeg )
              != IDE_SUCCESS);
    
    for ( i=0; i< sIndexSegTotal; i++ )
    {
        if ( ( sIndexSeg[i]->mParent->mEnd->mEnd.mX >= sCmpMinMaxX ) && 
             ( sIndexSeg[i]->mParent->mBegin->mStart.mX <= sCmpMaxMinX ) )
        {
            sPQueue.enqueue( &(sIndexSeg[i]), &sOverflow);
            IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
        }
        else
        {
            // Nothing to do 
        }
    }
    
    while(1)
    {
        sPQueue.dequeue( (void*) &sCurrSeg, &sUnderflow );

        if ( sUnderflow == ID_TRUE )
        {
            break;
        }

        sCurrNext = stdUtils::getNextSeg(sCurrSeg);
        sCurrPrev = stdUtils::getPrevSeg(sCurrSeg);

        sReuseSegCount = 0;
        sTempIndexSeg = sReuseSeg;  

        while(1)
        {      
            sPQueue.dequeue( (void*) &sCmpSeg, &sUnderflow );

            if ( sUnderflow == ID_TRUE )
            {
                break;
            }

            sReuseSeg[sReuseSegCount++] = sCmpSeg;
            
            if ( sCmpSeg->mStart.mX > sCurrSeg->mEnd.mX )
            {
                /* 잘 못 뽑은 놈은 재사용에 넣어야 한다. */
                break;                
            }
            
            do
            {
                /*
                  여기서 intersect와 방향에 대한 개념을 넣어 쳐낼수 있다.                  
                 */
                if ( ( sCurrNext != sCmpSeg ) && ( sCurrPrev != sCmpSeg ) && 
                     ( ( ( sCurrSeg->mParent->mPolygonNum < sMax1 ) && ( sCmpSeg->mParent->mPolygonNum  >= sMax1 ) ) ||
                       ( ( sCmpSeg->mParent->mPolygonNum  < sMax1 ) && ( sCurrSeg->mParent->mPolygonNum >= sMax1 ) ) )  )
                {
                    IDE_TEST( stdUtils::intersectCCW( sCurrSeg->mStart,
                                                      sCurrSeg->mEnd,
                                                      sCmpSeg->mStart,
                                                      sCmpSeg->mEnd,
                                                      &sIntersectStatus,
                                                      &sInterCount,
                                                      sInterResult)
                              != IDE_SUCCESS );

                    if ( sIntersectStatus != ST_NOT_INTERSECT )
                    {
                        switch( sIntersectStatus )
                        {
                            case ST_POINT_TOUCH:
                            case ST_TOUCH:
                                setDE9MatrixValue( sResultMatrix, STF_BOUND_BOUND,  STF_INTERSECTS_DIM_0 );
                                break;

                            case ST_INTERSECT:
                                setDE9MatrixValue( sResultMatrix, STF_INTER_INTER, STF_INTERSECTS_DIM_2 );
                                setDE9MatrixValue( sResultMatrix, STF_INTER_BOUND, STF_INTERSECTS_DIM_1 );
                                setDE9MatrixValue( sResultMatrix, STF_INTER_EXTER, STF_INTERSECTS_DIM_2 );
                                setDE9MatrixValue( sResultMatrix, STF_BOUND_INTER, STF_INTERSECTS_DIM_1 );
                                setDE9MatrixValue( sResultMatrix, STF_BOUND_BOUND, STF_INTERSECTS_DIM_0 );
                                setDE9MatrixValue( sResultMatrix, STF_BOUND_EXTER, STF_INTERSECTS_DIM_1 );
                                setDE9MatrixValue( sResultMatrix, STF_EXTER_INTER, STF_INTERSECTS_DIM_2 );
                                setDE9MatrixValue( sResultMatrix, STF_EXTER_BOUND, STF_INTERSECTS_DIM_1 );
                                break;

                            case ST_SHARE:
                                setDE9MatrixValue( sResultMatrix, STF_BOUND_BOUND,  STF_INTERSECTS_DIM_1 );
                                break;
                        }

                        IDE_TEST_RAISE( checkRelateResult(aPattern, sResultMatrix, aReturn ) 
                                        == ID_TRUE, normal_exit );
                       
                        switch( sIntersectStatus )
                        {
                        case ST_INTERSECT:
                        case ST_POINT_TOUCH:
                        case ST_TOUCH:
                            IDE_TEST( stdUtils::addPrimInterSeg( aQmxMem,
                                                                 &sPrimInterSeg,
                                                                 sCurrSeg,
                                                                 sCmpSeg,
                                                                 sIntersectStatus,
                                                                 sInterCount,
                                                                 sInterResult)
                                              != IDE_SUCCESS);                               
                            break;
                        case ST_SHARE:
                            IDE_TEST( stdUtils::addPrimInterSeg( aQmxMem,
                                                                 &sPrimInterSeg,
                                                                 sCurrSeg,
                                                                 sCmpSeg,
                                                                 sIntersectStatus,
                                                                 1,
                                                                 &sInterResult[0])
                                              != IDE_SUCCESS);                               

                            IDE_TEST( stdUtils::addPrimInterSeg( aQmxMem,
                                                                 &sPrimInterSeg,
                                                                 sCurrSeg,
                                                                 sCmpSeg,
                                                                 sIntersectStatus,
                                                                 1,
                                                                 &sInterResult[1])
                                              != IDE_SUCCESS);                               
                        }
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

    IDE_TEST( stdUtils::reassign( aQmxMem, sPrimInterSeg, ID_FALSE )
              != IDE_SUCCESS);

    stdPolyClip::adjustRingOrientation( sRingSegList,
                                        sRingCount,
                                        sIndexSeg,
                                        sIndexSegTotal,
                                        sMax1,
                                        sMax2 );

    IDE_TEST( stdPolyClip::labelingSegment( sRingSegList, 
                                            sRingCount, 
                                            sIndexSeg, 
                                            sIndexSegTotal,
                                            sMax1, 
                                            sMax2, 
                                            NULL )
              != IDE_SUCCESS );

    for ( i = 0 ; i < sIndexSegTotal ; i++ )
    {
        sCurrSeg = sIndexSeg[i];
        while ( sCurrSeg != NULL )
        {
            switch( sCurrSeg->mLabel )
            {

            case ST_SEG_LABEL_INSIDE:
                if( sCurrSeg->mParent->mPolygonNum < sMax1 )
                {
                    setDE9MatrixValue( sResultMatrix, STF_INTER_INTER,  STF_INTERSECTS_DIM_2 );
                    setDE9MatrixValue( sResultMatrix, STF_BOUND_INTER,  STF_INTERSECTS_DIM_1 );
                    setDE9MatrixValue( sResultMatrix, STF_EXTER_INTER,  STF_INTERSECTS_DIM_2 );
                }
                else
                {
                    setDE9MatrixValue( sResultMatrix, STF_INTER_INTER,  STF_INTERSECTS_DIM_2 );
                    setDE9MatrixValue( sResultMatrix, STF_INTER_BOUND,  STF_INTERSECTS_DIM_1 );
                    setDE9MatrixValue( sResultMatrix, STF_INTER_EXTER,  STF_INTERSECTS_DIM_2 );
                }
                break;
            
            case ST_SEG_LABEL_OUTSIDE:
                if( sCurrSeg->mParent->mPolygonNum < sMax1 )
                {
                    setDE9MatrixValue( sResultMatrix, STF_BOUND_EXTER,  STF_INTERSECTS_DIM_1 );
                    setDE9MatrixValue( sResultMatrix, STF_INTER_EXTER,  STF_INTERSECTS_DIM_2 );
                }
                else
                {
                    setDE9MatrixValue( sResultMatrix, STF_EXTER_BOUND,  STF_INTERSECTS_DIM_1 );
                    setDE9MatrixValue( sResultMatrix, STF_EXTER_INTER,  STF_INTERSECTS_DIM_2 );
                }
                break;

            case ST_SEG_LABEL_SHARED1:
                setDE9MatrixValue( sResultMatrix, STF_INTER_INTER,  STF_INTERSECTS_DIM_2 );
                setDE9MatrixValue( sResultMatrix, STF_EXTER_EXTER,  STF_INTERSECTS_DIM_2 );
                break;

            case ST_SEG_LABEL_SHARED2:
                setDE9MatrixValue( sResultMatrix, STF_EXTER_INTER,  STF_INTERSECTS_DIM_2 );
                setDE9MatrixValue( sResultMatrix, STF_INTER_EXTER,  STF_INTERSECTS_DIM_2 );
                break;

            default:
                IDE_DASSERT(0);
                break;
            }
            sCurrSeg = sCurrSeg->mNext;
        }
    }

    (void)checkRelateResult( aPattern, sResultMatrix, aReturn );

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_ABORT_ENQUEUE_ERROR )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stfRelation::RelateAreaArea",
                                  "enqueue overflow" ));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void stfRelation::setDE9MatrixValue( SChar* aMatrix, 
                                     SInt   aMatrixIndex, 
                                     SInt   aOrder )
{
    /* 이미 설정된 값보다 큰 경우에만 변경한다. */
    if ( aMatrix[aMatrixIndex] < aOrder )
    {
        aMatrix[aMatrixIndex] = aOrder;
    }
    else
    {
        // Nothing to do.
    }
}

idBool stfRelation::checkRelateResult( SChar*          aPattern, 
                                       SChar*          aResultMatrix, 
                                       mtdBooleanType* aResult )
{
    SInt   i;
    idBool sFastReturn = ID_FALSE;
    idBool sReturnValue = ID_TRUE;

    for ( i = 0 ; i < 9 ; i++ )
    {
        switch( aPattern[i] )
        {
        case 'F':
            if ( aResultMatrix[i] > STF_INTERSECTS_DIM_NOT )
            {
                sFastReturn  = ID_TRUE;
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_FALSE );
            }
            else
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_TRUE );
            }
            break;

        case '0':
            if ( aResultMatrix[i] > STF_INTERSECTS_DIM_0 )
            {
                sFastReturn  = ID_TRUE;
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_FALSE );
            }
            else if ( aResultMatrix[i] == STF_INTERSECTS_DIM_0 )
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_TRUE );
            }
            else
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_FALSE );
            }
            break;

        case '1':
            if ( aResultMatrix[i] > STF_INTERSECTS_DIM_1 )
            {
                sFastReturn  = ID_TRUE;
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_FALSE );
            }
            else if ( aResultMatrix[i] == STF_INTERSECTS_DIM_1 )
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_TRUE );
            }
            else
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_FALSE );
            }
            break;

        case '2':
            if ( aResultMatrix[i] == STF_INTERSECTS_DIM_2 )
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_TRUE );
            }
            else
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_FALSE );
            }
            break;

        case 'T':
            if ( aResultMatrix[i] > STF_INTERSECTS_DIM_NOT )
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_TRUE );
            }
            else
            {
                sReturnValue = ST_LOGICAL_AND( sReturnValue, ID_FALSE );
            }
            break;

        case '*': // don't care
            break;

        default: 
            IDE_DASSERT(0);
            break;
        }
    }

    *aResult = ( sReturnValue == ID_TRUE ) ? MTD_BOOLEAN_TRUE : MTD_BOOLEAN_FALSE;
    return sFastReturn;
}
