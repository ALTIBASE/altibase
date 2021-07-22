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
 * $Id: $
 **********************************************************************/

#include <idl.h>
#include <mtdTypes.h>
#include <stk.h>

#include <mtd.h>
#include <stdUtils.h>
#include <stfRelation.h>

extern mtdModule stdGeometry;
extern mtdModule mtdDouble;

smiCallBackFunc stkRangeFuncs[] = {
    stk::rangeCallBack,
    NULL
};

mtdCompareFunc  stkCompareFuncs[] = {
    stfRelation::isMBRContains,
    stfRelation::isMBRIntersects,
    stfRelation::isMBRWithin,
    stfRelation::isMBREquals,
    NULL
};

IDE_RC stk::rangeCallBack( idBool     * aResult,
                           const void * aRow,
                           void       *, /* aDirectKey */
                           UInt        , /* aDirectKeyPartialSize */
                           const scGRID,
                           void       * aData )
{
    SInt               sResult;
    mtkRangeCallBack * sData;
    stdMBR           * sValue;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;
    

    // Fix BUG-15844 :  R-Tree내의 Row와 비교하려는 대상객체가
    //                  Null 또는 Empty이면 실재비교를 하지않고 ID_FALSE로 처리한다.
    sData  = (mtkRangeCallBack*)aData;
    sValue = (stdMBR*)sData->value;  // from key range.

    if( stdUtils::isNullMBR( sValue ) == ID_TRUE )
    {
        sResult = 0;
    }
    else
    {
        for( sData  = (mtkRangeCallBack*)aData, sResult = MTD_BOOLEAN_TRUE;
             sData != NULL && sResult == MTD_BOOLEAN_TRUE;
             sData  = sData->next )
        {
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aRow;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;  // from key range.
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            
            sResult = sData->compare( &sValueInfo1, &sValueInfo2 );
        }
    }
    

    if(sResult == 1) // TRUE
    {
        *aResult = ID_TRUE;
    }
    else
    {
        *aResult = ID_FALSE;
    }
    
    return IDE_SUCCESS; 
}

SInt stk::RelationDefault( const mtcColumn* /*aColumn1*/,
                           const void*      /*aRow1*/,
                           UInt             /*aFlag1*/,
                           const mtcColumn* /*aColumn2*/,
                           const void*      /*aRow2*/,
                           UInt             /*aFlag2*/,
                           const void*      /*aInfo*/ )
{
    return 1; // TRUE
}

IDE_RC stk::mergeAndRange( smiRange* aMerged,
                           smiRange* aRange1,
                           smiRange* aRange2 )
{
    mtkRangeCallBack  * sMaximumCallBack;
    stdMBR            * sMBR1;
    stdMBR            * sMBR2;
    stdMBR              sMergedMBR;

    sMBR1 = (stdMBR*)((mtkRangeCallBack*)aRange1->maximum.data)->value;
    sMBR2 = (stdMBR*)((mtkRangeCallBack*)aRange2->maximum.data)->value;

    if ( stdUtils::isMBRIntersects( sMBR1, sMBR2 ) == ID_TRUE )
    {
        stdUtils::mergeAndMBR( &sMergedMBR, sMBR1, sMBR2 );
    }
    else
    {
        mtdDouble.null( NULL, &sMergedMBR.mMinX );
        mtdDouble.null( NULL, &sMergedMBR.mMinY );
        mtdDouble.null( NULL, &sMergedMBR.mMaxX );
        mtdDouble.null( NULL, &sMergedMBR.mMaxY );
    }

    // 새로운 MBR을 사용하는 callback을 생성한다.
    aMerged->prev = NULL;
    aMerged->next = NULL;
    aMerged->minimum = aRange1->minimum;
    aMerged->maximum = aRange1->maximum;

    // 새로운 MBR을 aRange1의 value에 복사한다.
    sMaximumCallBack = (mtkRangeCallBack*)aMerged->maximum.data;
    *(stdMBR*)sMaximumCallBack->value = sMergedMBR;
    
    return IDE_SUCCESS;
}

IDE_RC stk::mergeOrRangeList( smiRange  * aMerged,
                              smiRange ** aRangeListArray,
                              SInt        aRangeCount )
{
#define IDE_FN "void stk::mergeOrRangeList"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    smiRange         * sMerged;
    smiRange         * sStandard;
    smiRange         * sLast;
    mtkRangeCallBack * sData1;
    mtkRangeCallBack * sData2;
    stdMBR           * sMBR1;
    stdMBR           * sMBR2;
    stdMBR             sMergedMBR;
    SInt               sIndex;
    
    sMerged = aMerged;
    sLast   = aMerged;
    
    aMerged->prev    = NULL;
    aMerged->minimum = aRangeListArray[0]->minimum;
    aMerged->maximum = aRangeListArray[0]->maximum;

    for( sIndex = 1; sIndex < aRangeCount; sIndex++  )
    {
        sMerged->next    = sMerged + 1;
        sMerged          = sMerged->next;
        sMerged->prev    = sMerged - 1;
        sMerged->minimum = aRangeListArray[sIndex]->minimum;
        sMerged->maximum = aRangeListArray[sIndex]->maximum;
    }

    sMerged->next = NULL;

    sStandard = aMerged;
    
    while ( sStandard != NULL )
    {
        sMerged = aMerged;

        //MBR get
        sData1 = (mtkRangeCallBack*)sStandard->maximum.data;
        sMBR1  = (stdMBR*)sData1->value;

        while ( sMerged != NULL )
        {
            //MBR get
            sData2 = (mtkRangeCallBack*)sMerged->maximum.data;
            sMBR2  = (stdMBR*)sData2->value;

            if ( sStandard != sMerged )
            {
                if ( stdUtils::isMBRIntersects(sMBR1, sMBR2) == ID_TRUE )
                {
                    // 둘중 앞의 노드랑 합친다.
                    // 합친 MBR 값을 얻는다.
                    stdUtils::mergeOrMBR( &sMergedMBR, sMBR1, sMBR2 );

                    if ( sStandard < sMerged )
                    {
                        if ( sMerged->prev != NULL )
                        {
                            sMerged->prev->next = sMerged->next;
                        }
                        if ( sMerged->next != NULL )
                        {
                            sMerged->next->prev = sMerged->prev;
                        }
                    }
                    else
                    {
                        //기준점이 뒤면
                        //기준점을 변경 시킨다.
                        if ( sStandard->prev != NULL )
                        {
                            sStandard->prev->next = sStandard->next;
                        }
                        if ( sStandard->next != NULL )
                        {
                            sStandard->next->prev = sStandard->prev;
                        }

                        sStandard = sMerged;

                        sData1 = (mtkRangeCallBack*)sStandard->maximum.data;
                        sMBR1  = (stdMBR*)sData1->value;
                    }

                    // Standard의 MBR 값을 변경한다.
                    *sMBR1 = sMergedMBR;

                    sMerged = aMerged;
                    continue;
                }
                else
                {
                    sMerged = sMerged->next;
                }
            }
            else
            {
                sMerged = sMerged->next;
            }
        }

        sLast = sStandard;
        sStandard = sStandard->next;
    }
    
    sLast->next = NULL;

    return IDE_SUCCESS;
    
#undef IDE_FN
}
