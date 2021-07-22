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
 * $Id: mtk.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtd.h>
#include <mtdTypes.h>
#include <mtk.h>
#include <mtc.h>
#include <mtlCollate.h>

smiCallBackFunc   mtk::mAllRangeFuncs[MTK_MAX_RANGE_FUNC_CNT];
mtkRangeFuncIndex mtk::mAllRangeFuncsIndex[MTK_MAX_RANGE_FUNC_CNT];
UInt              mtk::mAllRangeFuncCount = 0;
smiCallBackFunc   mtk::mInternalRangeFuncs[] = {
    mtk::rangeCallBackGE4Mtd,
    mtk::rangeCallBackGE4Stored,
    mtk::rangeCallBackLE4Mtd,
    mtk::rangeCallBackLE4Stored,
    mtk::rangeCallBackGT4Mtd,
    mtk::rangeCallBackGT4Stored,
    mtk::rangeCallBackLT4Mtd,
    mtk::rangeCallBackLT4Stored,
    mtk::rangeCallBackGE4IndexKey, /* PROJ-2433 */
    mtk::rangeCallBackLE4IndexKey, /* PROJ-2433 */
    mtk::rangeCallBackGT4IndexKey, /* PROJ-2433 */
    mtk::rangeCallBackLT4IndexKey, /* PROJ-2433 */
    mtk::rangeCallBack4Rid,
    NULL
};

mtdCompareFunc      mtk::mAllCompareFuncs[MTK_MAX_COMPARE_FUNC_CNT];
mtkCompareFuncIndex mtk::mAllCompareFuncsIndex[MTK_MAX_COMPARE_FUNC_CNT];
UInt                mtk::mAllCompareFuncCount = 0;
mtdCompareFunc      mtk::mInternalCompareFuncs[] = {
    mtk::compareMinimumLimit,
    mtk::compareMaximumLimit4Mtd,
    mtk::compareMaximumLimit4Stored,
    mtd::compareNumberGroupBigintMtdMtdAsc,
    mtd::compareNumberGroupBigintMtdMtdDesc,
    mtd::compareNumberGroupBigintStoredMtdAsc,
    mtd::compareNumberGroupBigintStoredMtdDesc,
    mtd::compareNumberGroupBigintStoredStoredAsc,
    mtd::compareNumberGroupBigintStoredStoredDesc,
    mtd::compareNumberGroupDoubleMtdMtdAsc,
    mtd::compareNumberGroupDoubleMtdMtdDesc,
    mtd::compareNumberGroupDoubleStoredMtdAsc,
    mtd::compareNumberGroupDoubleStoredMtdDesc,
    mtd::compareNumberGroupDoubleStoredStoredAsc,
    mtd::compareNumberGroupDoubleStoredStoredDesc,
    mtd::compareNumberGroupNumericMtdMtdAsc,
    mtd::compareNumberGroupNumericMtdMtdDesc,
    mtd::compareNumberGroupNumericStoredMtdAsc,
    mtd::compareNumberGroupNumericStoredMtdDesc,
    mtd::compareNumberGroupNumericStoredStoredAsc,
    mtd::compareNumberGroupNumericStoredStoredDesc,
    mtlCollate::mtlCharMS949collateMtdMtdAsc,
    mtlCollate::mtlCharMS949collateMtdMtdDesc,
    mtlCollate::mtlCharMS949collateStoredMtdAsc,
    mtlCollate::mtlCharMS949collateStoredMtdDesc,
    mtlCollate::mtlCharMS949collateStoredStoredAsc,
    mtlCollate::mtlCharMS949collateStoredStoredDesc,
    mtlCollate::mtlVarcharMS949collateMtdMtdAsc,
    mtlCollate::mtlVarcharMS949collateMtdMtdDesc,
    mtlCollate::mtlVarcharMS949collateStoredMtdAsc,
    mtlCollate::mtlVarcharMS949collateStoredMtdDesc,
    mtlCollate::mtlVarcharMS949collateStoredStoredAsc,
    mtlCollate::mtlVarcharMS949collateStoredStoredDesc,
    NULL
};

extern "C" SInt
compareRangeFunc( const void* aElem1, const void* aElem2 )
{
    smiCallBackFunc sElem1 = ((mtkRangeFuncIndex*)aElem1)->rangeFunc;
    smiCallBackFunc sElem2 = ((mtkRangeFuncIndex*)aElem2)->rangeFunc;
    
    if ( sElem1 > sElem2 )
    {
        return 1;
    }
    else if ( sElem1 < sElem2 )
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

extern "C" SInt
compareCompareFunc( const void* aElem1, const void* aElem2 )
{
    mtdCompareFunc sElem1 = ((mtkCompareFuncIndex*)aElem1)->compareFunc;
    mtdCompareFunc sElem2 = ((mtkCompareFuncIndex*)aElem2)->compareFunc;
    
    if ( sElem1 > sElem2 )
    {
        return 1;
    }
    else if ( sElem1 < sElem2 )
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

IDE_RC mtk::initialize( smiCallBackFunc ** aExtRangeFuncGroup,
                        UInt               aExtRangeFuncGroupCnt,
                        mtdCompareFunc  ** aExtCompareFuncGroup,
                        UInt               aExtCompareFuncGroupCnt )
{
    smiCallBackFunc  * sRangeFunc;
    mtdCompareFunc   * sCompareFunc;
    const mtdModule  * sModule;
    idBool             sExist;
    UInt               i;
    UInt               j;
    UInt               k;
    
    //---------------------------------------------------------
    // mAllRangeFuncs을 구성
    //---------------------------------------------------------

    for ( sRangeFunc = mInternalRangeFuncs;
          *sRangeFunc != NULL;
          sRangeFunc++ )
    {
        for ( k = 0, sExist = ID_FALSE; k < mAllRangeFuncCount; k++ )
        {
            if ( mAllRangeFuncs[k] == *sRangeFunc )
            {
                sExist = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sExist == ID_FALSE )
        {
            IDE_ASSERT_MSG( mAllRangeFuncCount < MTK_MAX_RANGE_FUNC_CNT,
                            "mAllRangeFuncCount : %"ID_UINT32_FMT"\n",
                            mAllRangeFuncCount );
        
            mAllRangeFuncs[mAllRangeFuncCount] = *sRangeFunc;
            mAllRangeFuncCount++;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    for ( i = 0; i < aExtRangeFuncGroupCnt; i++ )
    {
        for ( sRangeFunc = aExtRangeFuncGroup[i];
              *sRangeFunc != NULL;
              sRangeFunc++ )
        {
            for ( k = 0, sExist = ID_FALSE; k < mAllRangeFuncCount; k++ )
            {
                if ( mAllRangeFuncs[k] == *sRangeFunc )
                {
                    sExist = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( sExist == ID_FALSE )
            {
                IDE_ASSERT_MSG( mAllRangeFuncCount < MTK_MAX_RANGE_FUNC_CNT,
                                "mAllRangeFuncCount : %"ID_UINT32_FMT"\n",
                                mAllRangeFuncCount );
            
                mAllRangeFuncs[mAllRangeFuncCount] = *sRangeFunc;
                mAllRangeFuncCount++;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    //---------------------------------------------------------
    // mAllRangeFuncsIndex를 구성
    //---------------------------------------------------------

    for ( i = 0; i < mAllRangeFuncCount; i++ )
    {
        mAllRangeFuncsIndex[i].rangeFunc = mAllRangeFuncs[i];
        mAllRangeFuncsIndex[i].idx       = i;
    }
    
    if ( mAllRangeFuncCount > 1 )
    {
        idlOS::qsort( mAllRangeFuncsIndex,
                      mAllRangeFuncCount,
                      ID_SIZEOF(mtkRangeFuncIndex),
                      compareRangeFunc );
    }
    else
    {
        // Nothing to do.
    }
    
    //---------------------------------------------------------
    // mAllCompareFuncs을 구성
    //---------------------------------------------------------

    for ( sCompareFunc = mInternalCompareFuncs;
          *sCompareFunc != NULL;
          sCompareFunc++ )
    {
        for ( k = 0, sExist = ID_FALSE; k < mAllCompareFuncCount; k++ )
        {
            if ( mAllCompareFuncs[k] == *sCompareFunc )
            {
                sExist = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sExist == ID_FALSE )
        {
            IDE_ASSERT_MSG( mAllCompareFuncCount < MTK_MAX_COMPARE_FUNC_CNT,
                            "mAllCompareFuncCount : %"ID_UINT32_FMT"\n",
                            mAllCompareFuncCount );
            
            mAllCompareFuncs[mAllCompareFuncCount] = *sCompareFunc;
            mAllCompareFuncCount++;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    for ( i = 0; i < mtd::getNumberOfModules(); i++ )
    {
        IDE_TEST( mtd::moduleByNo( & sModule, i ) != IDE_SUCCESS );

        for ( j = 0; j < MTD_COMPARE_FUNC_MAX_CNT; j++ )
        {
            for ( k = 0, sExist = ID_FALSE; k < mAllCompareFuncCount; k++ )
            {
                if ( mAllCompareFuncs[k] == sModule->keyCompare[j][0] )
                {
                    sExist = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( sExist == ID_FALSE )
            {
                IDE_ASSERT_MSG( mAllCompareFuncCount < MTK_MAX_COMPARE_FUNC_CNT,
                                "mAllCompareFuncCount : %"ID_UINT32_FMT"\n",
                                mAllCompareFuncCount );
            
                mAllCompareFuncs[mAllCompareFuncCount] = sModule->keyCompare[j][0];
                mAllCompareFuncCount++;
            }
            else
            {
                // Nothing to do.
            }
            
            for ( k = 0, sExist = ID_FALSE; k < mAllCompareFuncCount; k++ )
            {
                if ( mAllCompareFuncs[k] == sModule->keyCompare[j][1] )
                {
                    sExist = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( sExist == ID_FALSE )
            {
                IDE_ASSERT_MSG( mAllCompareFuncCount < MTK_MAX_COMPARE_FUNC_CNT,
                                "mAllCompareFuncCount : %"ID_UINT32_FMT"\n",
                                mAllCompareFuncCount );
            
                mAllCompareFuncs[mAllCompareFuncCount] = sModule->keyCompare[j][1];
                mAllCompareFuncCount++;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    
    for ( i = 0; i < aExtCompareFuncGroupCnt; i++ )
    {
        for ( sCompareFunc = aExtCompareFuncGroup[i];
              *sCompareFunc != NULL;
              sCompareFunc++ )
        {
            for ( k = 0, sExist = ID_FALSE; k < mAllCompareFuncCount; k++ )
            {
                if ( mAllCompareFuncs[k] == *sCompareFunc )
                {
                    sExist = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
            
            if ( sExist == ID_FALSE )
            {
                IDE_ASSERT_MSG( mAllCompareFuncCount < MTK_MAX_COMPARE_FUNC_CNT,
                                "mAllCompareFuncCount : %"ID_UINT32_FMT"\n",
                                mAllCompareFuncCount );
            
                mAllCompareFuncs[mAllCompareFuncCount] = *sCompareFunc;
                mAllCompareFuncCount++;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    
    //---------------------------------------------------------
    // mAllCompareFuncsIndex를 구성
    //---------------------------------------------------------

    for ( i = 0; i < mAllCompareFuncCount; i++ )
    {
        mAllCompareFuncsIndex[i].compareFunc = mAllCompareFuncs[i];
        mAllCompareFuncsIndex[i].idx         = i;
    }
    
    if ( mAllCompareFuncCount > 1 )
    {
        idlOS::qsort( mAllCompareFuncsIndex,
                      mAllCompareFuncCount,
                      ID_SIZEOF(mtkCompareFuncIndex),
                      compareCompareFunc );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt mtk::rangeByFunc( smiCallBackFunc aRangeFunc )
{
    mtkRangeFuncIndex    sFindRangeFunc;
    mtkRangeFuncIndex  * sRangeFunc;
    UInt                 sId;

    if ( aRangeFunc != NULL )
    {
        sFindRangeFunc.rangeFunc = aRangeFunc;
        
        sRangeFunc = (mtkRangeFuncIndex*) idlOS::bsearch(
            (const void*) & sFindRangeFunc,
            (const void*) mAllRangeFuncsIndex,
            mAllRangeFuncCount,
            ID_SIZEOF(mtkRangeFuncIndex),
            compareRangeFunc );
        
        IDE_ASSERT( sRangeFunc != NULL );

        sId = sRangeFunc->idx;
    }
    else
    {
        sId = mAllRangeFuncCount;
    }
    
    return sId;
}

UInt mtk::compareByFunc( mtdCompareFunc aCompareFunc )
{
    mtkCompareFuncIndex    sFindCompareFunc;
    mtkCompareFuncIndex  * sCompareFunc;
    UInt                   sId;

    if ( aCompareFunc != NULL )
    {
        sFindCompareFunc.compareFunc = aCompareFunc;
        
        sCompareFunc = (mtkCompareFuncIndex*) idlOS::bsearch(
            (const void*) & sFindCompareFunc,
            (const void*) mAllCompareFuncsIndex,
            mAllCompareFuncCount,
            ID_SIZEOF(mtkCompareFuncIndex),
            compareCompareFunc );
    
        IDE_ASSERT( sCompareFunc != NULL );
    
        sId = sCompareFunc->idx;
    }
    else
    {
        sId = mAllCompareFuncCount;
    }

    return sId;
}

smiCallBackFunc mtk::rangeById( UInt aId )
{
    smiCallBackFunc  sFunc;
    
    IDE_ASSERT( aId <= mAllRangeFuncCount );

    if ( aId < mAllRangeFuncCount )
    {
        sFunc = mAllRangeFuncs[aId];
    }
    else
    {
        sFunc = NULL;
    }

    return sFunc;
}

mtdCompareFunc mtk::compareById( UInt aId )
{
    mtdCompareFunc  sFunc;
    
    IDE_ASSERT( aId <= mAllCompareFuncCount );

    if ( aId < mAllCompareFuncCount )
    {
        sFunc = mAllCompareFuncs[aId];
    }
    else
    {
        sFunc = NULL;
    }
    
    return sFunc;
}

SInt mtk::compareRange( const smiCallBack* aCallBack1,
                        const smiCallBack* aCallBack2 )
{
    mtkRangeCallBack* sCallBack1;
    mtkRangeCallBack* sCallBack2;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    mtdCompareFunc    sValueCompareFunc;
    UInt              sCompareType;
    UInt              sDirection;
    SInt              sResult;
    
    for( sCallBack1 = (mtkRangeCallBack*)aCallBack1->data,
             sCallBack2 = (mtkRangeCallBack*)aCallBack2->data;
         sCallBack1 != NULL && sCallBack2 != NULL;
         sCallBack1 = sCallBack1->next, sCallBack2 = sCallBack2->next )
    {
        if( sCallBack1->compare == compareMinimumLimit        ||
            sCallBack2->compare == compareMinimumLimit        ||
            sCallBack1->compare == compareMaximumLimit4Mtd    ||
            sCallBack1->compare == compareMaximumLimit4Stored ||
            sCallBack2->compare == compareMaximumLimit4Mtd    ||
            sCallBack2->compare == compareMaximumLimit4Stored)
        {
            if( ( sCallBack1->compare == compareMinimumLimit &&
                  sCallBack2->compare != compareMinimumLimit )
                ||
                ( sCallBack1->compare != compareMaximumLimit4Mtd &&
                  sCallBack2->compare == compareMaximumLimit4Mtd )
                ||
                ( sCallBack1->compare != compareMaximumLimit4Stored &&
                  sCallBack2->compare == compareMaximumLimit4Stored ) )
            {
                return -1;
            }
            if( ( sCallBack1->compare != compareMinimumLimit &&
                  sCallBack2->compare == compareMinimumLimit )
                ||
                ( sCallBack1->compare == compareMaximumLimit4Mtd &&
                  sCallBack2->compare != compareMaximumLimit4Mtd )
                ||
                ( sCallBack1->compare == compareMaximumLimit4Stored &&
                  sCallBack2->compare != compareMaximumLimit4Stored ) )
            {
                return 1;
            }
        }
        else
        {
            sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;

            if( ( ( sCallBack1->flag & MTK_COMPARE_SAMEGROUP_MASK )
                  == MTK_COMPARE_SAMEGROUP_TRUE )
                ||
                ( ( sCallBack2->flag & MTK_COMPARE_SAMEGROUP_MASK )
                  == MTK_COMPARE_SAMEGROUP_TRUE ) )
            {
                // PROJ-1364
                // 동일계열내 서로 다른 data type간 index를
                // 사용할 수 있으므로 value가 서로 다른 data type
                // 일 수 있다.
                // 이때는 서로 다른 data type간 비교함수를 사용해야 한다.

                sDirection =
                    (sCallBack1->flag & MTK_COMPARE_DIRECTION_MASK);

                sValueCompareFunc =
                    mtd::findCompareFunc( &(sCallBack1->valueDesc),
                                          &(sCallBack2->valueDesc),
                                          sCompareType,
                                          sDirection );

            }
            else
            {
                sDirection = (sCallBack1->flag & MTK_COMPARE_DIRECTION_MASK);

                sValueCompareFunc =
                    sCallBack1->valueDesc.module->keyCompare[sCompareType][sDirection];
            }

            //------------------------------------------------------------
            // value간 compare
            //------------------------------------------------------------
            sValueInfo1.column = &(sCallBack1->valueDesc);
            sValueInfo1.value  = sCallBack1->value;
            sValueInfo1.length = 0;
            sValueInfo1.flag   = MTD_OFFSET_USE;

            sValueInfo2.column = &(sCallBack2->valueDesc);
            sValueInfo2.value  = sCallBack2->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            if ( ( sResult = sValueCompareFunc( &sValueInfo1,
                                                &sValueInfo2 ) )
                != 0 )
            {
                return sResult;
            }
        }
    }
    
    if( sCallBack1 != NULL )
    {
        if ( ( aCallBack2->callback == rangeCallBackGE4Mtd ) ||
             ( aCallBack2->callback == rangeCallBackLT4Mtd ) ||
             ( aCallBack2->callback == rangeCallBackGE4Stored ) ||
             ( aCallBack2->callback == rangeCallBackLT4Stored ) ||
             ( aCallBack2->callback == rangeCallBackGE4IndexKey ) ||
             ( aCallBack2->callback == rangeCallBackLT4IndexKey ) )
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    
    if( sCallBack2 != NULL )
    {
        if ( ( aCallBack1->callback == rangeCallBackGE4Mtd ) ||
             ( aCallBack2->callback == rangeCallBackLT4Mtd ) ||
             ( aCallBack1->callback == rangeCallBackGE4Stored ) ||
             ( aCallBack2->callback == rangeCallBackLT4Stored ) ||
             ( aCallBack1->callback == rangeCallBackGE4IndexKey ) ||
             ( aCallBack2->callback == rangeCallBackLT4IndexKey ) )
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }

    if( aCallBack1->callback != aCallBack2->callback )
    {
        if ( ( aCallBack1->callback == rangeCallBackLT4Mtd ) ||
             ( aCallBack1->callback == rangeCallBackLT4Stored ) ||
             ( aCallBack1->callback == rangeCallBackLT4IndexKey ) )
        {
            return -1;
        }
        else
        {
            /* nothing to do */
        }

        if ( ( aCallBack2->callback == rangeCallBackLT4Mtd ) ||
             ( aCallBack2->callback == rangeCallBackLT4Stored ) ||
             ( aCallBack2->callback == rangeCallBackLT4IndexKey ) )
        {
            return 1;
        }
        else
        {
            /* nothing to do */
        }

        if ( ( aCallBack1->callback == rangeCallBackGT4Mtd ) ||
             ( aCallBack1->callback == rangeCallBackGT4Stored ) ||
             ( aCallBack1->callback == rangeCallBackGT4IndexKey ) )
        {
            return 1;
        }
        else
        {
            /* nothing to do */
        }

        if ( ( aCallBack2->callback == rangeCallBackGT4Mtd ) ||
             ( aCallBack2->callback == rangeCallBackGT4Stored ) ||
             ( aCallBack2->callback == rangeCallBackGT4IndexKey ) )
        {
            return -1;
        }
        else
        {
            /* nothing to do */
        }

        if ( ( aCallBack1->callback == rangeCallBackGE4Mtd ) ||
             ( aCallBack1->callback == rangeCallBackGE4Stored ) ||
             ( aCallBack1->callback == rangeCallBackGE4IndexKey ) )
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    return 0;
}


extern "C" SInt
compareMinimum( const void * aRange1,
                const void * aRange2 )
{
    return mtk::compareRange( &((*(smiRange **)aRange1)->minimum),
                              &((*(smiRange **)aRange2)->minimum) );
}


IDE_RC mtk::estimateRangeDefault( mtcNode*,
                                  mtcTemplate*,
                                  UInt,
                                  UInt*    aSize )
{
    *aSize = ID_SIZEOF(smiRange) + ( ID_SIZEOF(mtkRangeCallBack) << 1 );
    
    return IDE_SUCCESS;
}

IDE_RC mtk::estimateRangeDefaultLike( mtcNode*,
                                      mtcTemplate*,
                                      UInt,
                                      UInt*    aSize )
{
    *aSize = ID_SIZEOF(smiRange)
        + ( ID_SIZEOF(mtkRangeCallBack)
            + ID_SIZEOF(mtcColumn)
            + MTC_LIKE_KEY_SIZE ) * 2;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::estimateRangeDefaultLike4Echar( mtcNode*,
                                            mtcTemplate* aTemplate,
                                            UInt,
                                            UInt*        aSize )
{
    UInt  sEcharKeySize;
    
    IDE_TEST( mtc::getLikeEcharKeySize( aTemplate,
                                        NULL,
                                        & sEcharKeySize )
              != IDE_SUCCESS );
    
    *aSize = ID_SIZEOF(smiRange)
        + ( ID_SIZEOF(mtkRangeCallBack)
            + ID_SIZEOF(mtcColumn)
            + sEcharKeySize ) * 2;
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtk::estimateRangeNA( mtcNode*,
                             mtcTemplate*,
                             UInt,
                             UInt* )
{
    IDE_SET(ideSetErrorCode(mtERR_FATAL_NOT_APPLICABLE));
    
    return IDE_FAILURE;
}

IDE_RC mtk::extractRangeNA( mtcNode*,
                            mtcTemplate*,
                            mtkRangeInfo *,
                            smiRange* )
{
    IDE_SET(ideSetErrorCode(mtERR_FATAL_NOT_APPLICABLE));
    
    return IDE_FAILURE;
}

IDE_RC mtk::extractNotNullRange( mtcNode*,
                                 mtcTemplate*,
                                 mtkRangeInfo * aInfo,
                                 smiRange*    aRange )
{
    //mtcNode*          sIndexNode;
    mtkRangeCallBack* sMinimumCallBack;
    mtkRangeCallBack* sMaximumCallBack;
    
    // for( sIndexNode  = aNode->arguments;
    //     sIndexNode != NULL && aInfo->argument != 0;
    //     sIndexNode  = sIndexNode->next, aInfo->argument-- );
    //IDE_TEST_RAISE( sIndexNode == NULL, ERR_INVALID_FUNCTION_ARGUMENT );
    
    sMinimumCallBack = (mtkRangeCallBack*)( aRange + 1 );
    sMaximumCallBack = sMinimumCallBack + 1;
    
    aRange->prev                 = NULL;
    aRange->next                 = NULL;

    if ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ||
         aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL )
    {
        aRange->minimum.callback     = mtk::rangeCallBackGE4Mtd;
        aRange->minimum.data         = sMinimumCallBack;
        aRange->maximum.callback     = mtk::rangeCallBackLT4Mtd;
        aRange->maximum.data         = sMaximumCallBack;
    }
    else
    {
        if ( ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
             ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
        {
            aRange->minimum.callback     = mtk::rangeCallBackGE4Stored;
            aRange->minimum.data         = sMinimumCallBack;
            aRange->maximum.callback     = mtk::rangeCallBackLT4Stored;
            aRange->maximum.data         = sMaximumCallBack;
        }
        else
        {
            /* PROJ-2433 Direct Key Index */
            aRange->minimum.callback     = mtk::rangeCallBackGE4IndexKey;
            aRange->minimum.data         = sMinimumCallBack;
            aRange->maximum.callback     = mtk::rangeCallBackLT4IndexKey;
            aRange->maximum.data         = sMaximumCallBack;
        }
    }
            
    sMinimumCallBack->next       = NULL;
    sMinimumCallBack->columnIdx  = aInfo->columnIdx;
    sMinimumCallBack->compare    = mtk::compareMinimumLimit;
    //sMinimumCallBack->columnDesc = NULL;
    //sMinimumCallBack->valueDesc  = NULL;
    sMinimumCallBack->value      = NULL;
    
    sMaximumCallBack->next       = NULL;
    sMaximumCallBack->columnIdx  = aInfo->columnIdx;
    sMaximumCallBack->columnDesc = *aInfo->column;

    if ( ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_INDEX_KEY_FIXED_MTDVAL ) || /* PROJ-2433 */
         ( aInfo->compValueType == MTD_COMPARE_INDEX_KEY_MTDVAL ) )
    {
        sMaximumCallBack->compare    = mtk::compareMaximumLimit4Mtd;
    }
    else
    {
        sMaximumCallBack->compare    = mtk::compareMaximumLimit4Stored;
    }
    
    //sMaximumCallBack->valueDesc  = NULL;
    sMaximumCallBack->value      = NULL;
        
    return IDE_SUCCESS;
    
    //IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    //IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    //IDE_EXCEPTION_END;
    
    //return IDE_FAILURE;
}

SInt mtk::compareMinimumLimit( mtdValueInfo * /* aValueInfo1 */,
                               mtdValueInfo * /* aValueInfo2 */ )
{
    return 1;
}

SInt mtk::compareMaximumLimit4Mtd( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * /* aValueInfo2 */ )
{
    const void* sValue = mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                              aValueInfo1->value,
                                              aValueInfo1->flag,
                                              aValueInfo1->column->module->staticNull );

    return (aValueInfo1->column->module->isNull( aValueInfo1->column,
                                                 sValue )
            != ID_TRUE) ? -1 : 0 ;
}

SInt mtk::compareMaximumLimit4Stored( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * /* aValueInfo2 */ )
{
    UInt            sLength;
    const void  *   sRealValue;

    /* PROJ-2429 Dictionary based data compress for on-disk DB
     * Null value대신 null value가 저장된 OID값을 가지고있어
     * maximum check가 제대로 되지 않음
     */
    if ( ( aValueInfo1->column->column.flag & SMI_COLUMN_COMPRESSION_MASK )
         != SMI_COLUMN_COMPRESSION_TRUE )
    {
        if ( ( aValueInfo1->column->module->flag & MTD_VARIABLE_LENGTH_TYPE_MASK )
             == MTD_VARIABLE_LENGTH_TYPE_TRUE )
        {
            return ( aValueInfo1->length != 0 ) ? -1 : 0;
        }
        else
        {
            return ( idlOS::memcmp( aValueInfo1->value,
                                    aValueInfo1->column->module->staticNull,
                                    aValueInfo1->column->column.size )
                     != 0 ) ? -1 : 0;
        }
    }
    else
    {
        sRealValue = mtc::getCompressionColumn( aValueInfo1->value,
                                                (smiColumn*)aValueInfo1->column,
                                                ID_FALSE, // aUseColumnOffset
                                                &sLength );

        return (aValueInfo1->column->module->isNull( NULL, sRealValue )
                == ID_FALSE) ? -1 : 0;
    }
}

IDE_RC mtk::rangeCallBackGE4Mtd( idBool      * aResult,
                                 const void  * aColVal,
                                 void        *, /* aDirectKey */
                                 UInt         , /* aDirectKeyPartialSize */
                                 const scGRID,
                                 void        * aData )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key Col들의 greater & equal range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    sOrder = 0;

    for ( sData = (mtkRangeCallBack*)aData ;
          ( sData != NULL ) && ( sOrder == 0 ) ;
          sData = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aColVal;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = ( sOrder >= 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackGE4Stored( idBool      * aResult,
                                    const void  * aColVal,
                                    void        *, /* aDirectKey */
                                    UInt         , /* aDirectKeyPartialSize */
                                    const scGRID,
                                    void        * aData )
{
/***********************************************************************
 *
 * Description : Stored 타입의 Key Col들의 greater & equal range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    const smiValue  * sColVal;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    sColVal = (const smiValue*)aColVal;
    
    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = sColVal[ sData->columnIdx ].value;
        sValueInfo1.length = sColVal[ sData->columnIdx ].length;
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = sOrder >= 0 ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackLE4Mtd( idBool      * aResult,
                                 const void  * aColVal,
                                 void        *, /* aDirectKey */
                                 UInt         , /* aDirectKeyPartialSize */
                                 const scGRID,
                                 void        * aData )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key Col들의 less & equal range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    sOrder = 0;

    for ( sData = (mtkRangeCallBack*)aData ;
          ( sData != NULL ) && ( sOrder == 0 ) ;
          sData  = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aColVal;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = ( sOrder <= 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackLE4Stored( idBool      * aResult,
                                    const void  * aColVal,
                                    void        *, /* aDirectKey */
                                    UInt         , /* aDirectKeyPartialSize */
                                    const scGRID,
                                    void        * aData )
{
/***********************************************************************
 *
 * Description : Stored 타입의 Key Col들의 less & equal range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    const smiValue  * sColVal;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    sColVal = (const smiValue*)aColVal;
    
    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = sColVal[ sData->columnIdx ].value;
        sValueInfo1.length = sColVal[ sData->columnIdx ].length;
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0;
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = sOrder <= 0 ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackGT4Mtd( idBool      * aResult,
                                 const void  * aColVal,
                                 void        *, /* aDirectKey */
                                 UInt         , /* aDirectKeyPartialSize */
                                 const scGRID,
                                 void        * aData )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key Col들의 greater than range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aColVal;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;
        
        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = ( sOrder > 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackGT4Stored( idBool      * aResult,
                                    const void  * aColVal,
                                    void        *, /* aDirectKey */
                                    UInt         , /* aDirectKeyPartialSize */
                                    const scGRID,
                                    void        * aData )
{
/***********************************************************************
 *
 * Description : Stored 타입의 Key Col들의 greater than range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    const smiValue  * sColVal;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    sColVal = (const smiValue*)aColVal;

    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = sColVal[ sData->columnIdx ].value;
        sValueInfo1.length = sColVal[ sData->columnIdx ].length;
        sValueInfo1.flag   = MTD_OFFSET_USE;
        
        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0;
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = sOrder > 0 ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackLT4Mtd( idBool      * aResult,
                                 const void  * aColVal,
                                 void        *, /* aDirectKey */
                                 UInt         , /* aDirectKeyPartialSize */
                                 const scGRID,
                                 void        * aData )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key Col들의 less than range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = aColVal;
        sValueInfo1.length = 0; // do not use
        sValueInfo1.flag   = MTD_OFFSET_USE;
        
        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0; // do not use
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = ( sOrder < 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackLT4Stored( idBool      * aResult,
                                    const void  * aColVal,
                                    void        *, /* aDirectKey */
                                    UInt         , /* aDirectKeyPartialSize */
                                    const scGRID,
                                    void        * aData )
{
/***********************************************************************
 *
 * Description : Stored 타입의 Key Col들의 less than range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    const smiValue  * sColVal;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    sColVal = (const smiValue*)aColVal;
    
    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        sValueInfo1.column = &(sData->columnDesc);
        sValueInfo1.value  = sColVal[ sData->columnIdx ].value;
        sValueInfo1.length = sColVal[ sData->columnIdx ].length;
        sValueInfo1.flag   = MTD_OFFSET_USE;
        
        sValueInfo2.column = &(sData->valueDesc);
        sValueInfo2.value  = sData->value;
        sValueInfo2.length = 0;
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sOrder = sData->compare( &sValueInfo1,
                                 &sValueInfo2 );
    }
    
    *aResult = sOrder < 0 ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBack4Rid( idBool      * /* aResult */,
                               const void  * /* aColVal */,
                               void        *,/* aDirectKey */
                               UInt         ,/* aDirectKeyPartialSize */
                               const scGRID,
                               void        * /* aData */ )
{
    IDE_ASSERT(0);
    
    return IDE_SUCCESS;
}

/*
 * PROJ-2433
 * Direct key Index의 direct key와 mtd를 비교하는 range callback 함수
 * - index의 첫번째 컬럼만 direct key로 비교함
 * - partial direct key를 처리하는부분 추가
 */
IDE_RC mtk::rangeCallBackGE4IndexKey( idBool      * aResult,
                                      const void  * aColVal,
                                      void        * aDirectKey,
                                      UInt          aDirectKeyPartialSize,
                                      const scGRID,
                                      void        * aData )
{
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    sOrder = 0;

    for ( sData = (mtkRangeCallBack*)aData ;
          ( sData != NULL ) && ( sOrder == 0 ) ;
          sData = sData->next )
    {
        if ( aDirectKey != NULL )
        {
            /*
             * PROJ-2433 Direct Key Index
             * direct key가 NULL 이 아닌경우, 첫번째 컬럼은 direct key와 비교한다.
             *
             * - aDirectKeyPartialSize가 0 이 아닌경우,
             *   partial direct key 이므로 MTD_PARTIAL_KEY_ON 을 flag에 세팅한다.
             *
             * - partial direct key인 경우,
             *   그 결과가 정확한 값이 아니므로 두번째 컬럼 비교없이 바로 종료한다.
             */
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aDirectKey;
            sValueInfo1.length = aDirectKeyPartialSize;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            if ( aDirectKeyPartialSize != 0 )
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_ON;
            }
            else
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_OFF;
            }

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );

            if ( aDirectKeyPartialSize != 0 )
            {
                /* partial key 이면,
                 * 다음 컬럼의 비교는의미없다. 바로끝낸다 */
                break;
            }
            else
            {
                aDirectKey = NULL; /* direct key는 첫컬럼만 사용함 */
            }
        }
        else
        {
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aColVal;
            sValueInfo1.length = 0;
            sValueInfo1.flag   = MTD_OFFSET_USE;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );
        }
    }
    
    *aResult = ( sOrder >= 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackLE4IndexKey( idBool      * aResult,
                                      const void  * aColVal,
                                      void        * aDirectKey,
                                      UInt          aDirectKeyPartialSize,
                                      const scGRID,
                                      void        * aData )
{
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    sOrder = 0;

    for ( sData = (mtkRangeCallBack*)aData ;
          ( sData != NULL ) && ( sOrder == 0 ) ;
          sData  = sData->next )
    {
        if ( aDirectKey != NULL )
        {
            /*
             * PROJ-2433 Direct Key Index
             * direct key가 NULL 이 아닌경우, 첫번째 컬럼은 direct key와 비교한다.
             *
             * - aDirectKeyPartialSize가 0 이 아닌경우,
             *   partial direct key 이므로 MTD_PARTIAL_KEY_ON 을 flag에 세팅한다.
             *
             * - partial direct key인 경우,
             *   그 결과가 정확한 값이 아니므로 두번째 컬럼 비교없이 바로 종료한다.
             */
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aDirectKey;
            sValueInfo1.length = aDirectKeyPartialSize;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            if ( aDirectKeyPartialSize != 0 )
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_ON;
            }
            else
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_OFF;
            }

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );

            if ( aDirectKeyPartialSize != 0 )
            {
                /* partial key 이면,
                 * 다음 컬럼의 비교는의미없다. 바로끝낸다 */
                break;
            }
            else
            {
                aDirectKey = NULL; /* direct key는 첫번째 컬럼에서만 사용됨 */
            }
        }
        else
        {
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aColVal;
            sValueInfo1.length = 0;
            sValueInfo1.flag   = MTD_OFFSET_USE;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );
        }
    }
    
    *aResult = ( sOrder <= 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackGT4IndexKey( idBool      * aResult,
                                      const void  * aColVal,
                                      void        * aDirectKey,
                                      UInt          aDirectKeyPartialSize,
                                      const scGRID,
                                      void        * aData )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key Col들의 greater than range callback
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        if ( aDirectKey != NULL )
        {
            /*
             * PROJ-2433 Direct Key Index
             * direct key가 NULL 이 아닌경우, 첫번째 컬럼은 direct key와 비교한다.
             *
             * - aDirectKeyPartialSize가 0 이 아닌경우,
             *   partial direct key 이므로 MTD_PARTIAL_KEY_ON 을 flag에 세팅한다.
             *
             * - partial direct key인 경우,
             *   그 결과가 정확한 값이 아니므로 두번째 컬럼 비교없이 바로 종료한다.
             */
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aDirectKey;
            sValueInfo1.length = aDirectKeyPartialSize;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            if ( aDirectKeyPartialSize != 0 )
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_ON;
            }
            else
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_OFF;
            }

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );

            if ( aDirectKeyPartialSize != 0 )
            {
                /* PROJ-2433 Direct Key Index
                 * partial key 이면 정확한 비교가 될수없으므로
                 * 아래와 같이처리한다.
                 * 1. GT compare 함수라도, GE compare 랑 동일하게 처리한다.
                 *    즉, 두 값이 같은 경우도 ID_TRUE 로 처리한다.
                 *
                 * 2. composite key index인경우
                 *    다음 컬럼의 비교는의미없다. 바로끝낸다 */
                *aResult = ( sOrder >= 0 ) ? ID_TRUE : ID_FALSE;
                return IDE_SUCCESS;
            }
            else
            {
                aDirectKey = NULL; /* direct key는 한번만 사용됨 */
            }
        }
        else
        {
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aColVal;
            sValueInfo1.length = 0;
            sValueInfo1.flag   = MTD_OFFSET_USE;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );
        }
    }
    
    *aResult = ( sOrder > 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::rangeCallBackLT4IndexKey( idBool      * aResult,
                                      const void  * aColVal,
                                      void        * aDirectKey,
                                      UInt          aDirectKeyPartialSize,
                                      const scGRID,
                                      void        * aData )
{
    mtkRangeCallBack* sData;
    SInt              sOrder;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    for( sData  = (mtkRangeCallBack*)aData, sOrder = 0;
         sData != NULL && sOrder == 0;
         sData  = sData->next )
    {
        if ( aDirectKey != NULL )
        {
            /*
             * PROJ-2433 Direct Key Index
             * direct key가 NULL 이 아닌경우, 첫번째 컬럼은 direct key와 비교한다.
             *
             * - aDirectKeyPartialSize가 0 이 아닌경우,
             *   partial direct key 이므로 MTD_PARTIAL_KEY_ON 을 flag에 세팅한다.
             *
             * - partial direct key인 경우,
             *   그 결과가 정확한 값이 아니므로 두번째 컬럼 비교없이 바로 종료한다.
             */
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aDirectKey;
            sValueInfo1.length = aDirectKeyPartialSize;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            if ( aDirectKeyPartialSize != 0 )
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_ON;
            }
            else
            {
                sValueInfo1.flag &= ~MTD_PARTIAL_KEY_MASK;
                sValueInfo1.flag |= MTD_PARTIAL_KEY_OFF;
            }

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );

            if ( aDirectKeyPartialSize != 0 )
            {
                /* PROJ-2433 Direct Key Index
                 * partial key 이면 정확한 비교가 될수없으므로
                 * 아래와 같이처리한다.
                 * 1. LT compare 함수라도, LE compare 랑 동일하게 처리한다.
                 *    즉, 두 값이 같은 경우도 ID_TRUE 로 처리한다.
                 *
                 * 2. composite key index인경우
                 *    다음 컬럼의 비교는의미없다. 바로끝낸다 */
                *aResult = ( sOrder <= 0 ) ? ID_TRUE : ID_FALSE ;
                return IDE_SUCCESS;
            }
            else
            {
                aDirectKey = NULL; /* direct key는 한번만 사용됨 */
            }
        }
        else
        {
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aColVal;
            sValueInfo1.length = 0;
            sValueInfo1.flag   = MTD_OFFSET_USE;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.length = 0;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1,
                                     &sValueInfo2 );
        }
    }
    
    *aResult = ( sOrder < 0 ) ? ID_TRUE : ID_FALSE ;
    
    return IDE_SUCCESS;
}

IDE_RC mtk::addCompositeRange( smiRange* aComposite,
                               smiRange* aRange )
{
    mtkRangeCallBack * sData;

    //-----------------
    // minimum callback
    //-----------------
    if( ( aComposite->minimum.callback == rangeCallBackGE4Mtd ) ||
        ( aComposite->minimum.callback == rangeCallBackGE4Stored ) ||
        ( aComposite->minimum.callback == rangeCallBackGE4IndexKey ) ) /* PROJ-2433 */
    {
        aComposite->minimum.callback = aRange->minimum.callback;
        for( sData = (mtkRangeCallBack*)aComposite->minimum.data;
             sData->next != NULL;
             sData = sData->next ) ;
        sData->next = (mtkRangeCallBack*)aRange->minimum.data;
    }

    //-----------------    
    // maximum callback
    //-----------------

    if( ( aComposite->maximum.callback == rangeCallBackLE4Mtd ) ||
        ( aComposite->maximum.callback == rangeCallBackLE4Stored )  ||
        ( aComposite->maximum.callback == rangeCallBackLE4IndexKey ) ) /* PROJ-2433 */
    {
        aComposite->maximum.callback = aRange->maximum.callback;
        for( sData = (mtkRangeCallBack*)aComposite->maximum.data;
             sData->next != NULL;
             sData = sData->next ) ;
        sData->next = (mtkRangeCallBack*)aRange->maximum.data;
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtk::mergeAndRangeDefault( smiRange* aMerged,
                                  smiRange* aRange1,
                                  smiRange* aRange2 )
{
    static mtkRangeCallBack sRangeCallBack;
    smiRange* sRange;
    smiRange* sLast;
    
    sRangeCallBack.next       = NULL;
    sRangeCallBack.compare    = compareMinimumLimit;
    //sRangeCallBack.columnDesc = NULL;
    //sRangeCallBack.valueDesc  = NULL;
    sRangeCallBack.value      = NULL;
    sRangeCallBack.flag       = 0;
    
    sLast  = NULL;
    
    while( aRange1 != NULL && aRange2 != NULL )
    {
        if ( compareRange( &aRange1->minimum,
                           &aRange2->minimum ) > 0 )
        {
            sRange  = aRange1;
            aRange1 = aRange2;
            aRange2 = sRange;
        }
        
        if ( compareRange( &aRange1->maximum,
                           &aRange2->minimum ) > 0 )
        {
            aMerged->prev    = sLast;
            aMerged->next    = aMerged + 1;
            aMerged->minimum = aRange2->minimum;
            
            if ( compareRange( &aRange1->maximum,
                               &aRange2->maximum ) < 0 )
            {
                aMerged->maximum = aRange1->maximum;
                aRange1          = aRange1->next;
            }
            else
            {
                aMerged->maximum = aRange2->maximum;
                aRange2          = aRange2->next;
            }
            sLast            = aMerged;
            aMerged          = aMerged->next;
        }
        else
        {
            aRange1 = aRange1->next;
        }
    }
    
    if( sLast == NULL )
    {
        aMerged->prev             = NULL;
        aMerged->next             = NULL;
        aMerged->minimum.callback = mtk::rangeCallBackGT4Mtd;
        aMerged->minimum.data     = &sRangeCallBack;
        aMerged->maximum.callback = mtk::rangeCallBackLT4Mtd;
        aMerged->maximum.data     = &sRangeCallBack;
    }
    else
    {
        sLast->next = NULL;
    }

    return IDE_SUCCESS;
}

IDE_RC mtk::mergeOrRangeListDefault( smiRange  * aMerged,
                                     smiRange ** aRangeListArray,
                                     SInt        aRangeCount )
{
    SInt sIndex = 0;
    SInt i = 0;

    /* PROJ-2446 ONE SOURCE XDB인 경우 smiRange에서 statistics 가져오기
     * 위해 자료 구조에 추가 됨*/
    for( i = 0; i < aRangeCount; i++ )
    {
        aRangeListArray[i]->statistics = NULL;
    }

    idlOS::qsort( aRangeListArray,
                  aRangeCount,
                  ID_SIZEOF(smiRange *),
                  compareMinimum );

    aMerged->prev = NULL;

    aMerged->minimum = aRangeListArray[0]->minimum;
    aMerged->maximum = aRangeListArray[0]->maximum;

    for( sIndex = 1; sIndex < aRangeCount; sIndex++  )
    {
        if ( compareRange( &aMerged->maximum,
                           &(aRangeListArray[sIndex])->minimum ) >= 0 )
        {
            if ( compareRange( &aMerged->maximum,
                               &(aRangeListArray[sIndex])->maximum ) < 0 )
            {
                // merging...
                aMerged->maximum = aRangeListArray[sIndex]->maximum;         
            }
            else
            {
                // Nothing To Do
            }            
        }
        else
        {            
            aMerged->next    = aMerged + 1;
            aMerged          = aMerged->next;
            aMerged->prev    = aMerged - 1;
            aMerged->minimum = aRangeListArray[sIndex]->minimum;
            aMerged->maximum = aRangeListArray[sIndex]->maximum;
        }       
    }

    aMerged->next = NULL;

    return IDE_SUCCESS;
}


IDE_RC mtk::mergeOrRangeNotBetween( smiRange* aMerged,
                                    smiRange* aRange1,
                                    smiRange* aRange2 )
{
    smiRange* sRange;
    
    aMerged->prev = NULL;
    
    if ( compareRange( &aRange1->minimum, &aRange2->minimum ) < 0 )
    {
        sRange  = aRange1;
        aRange1 = aRange1->next;
    }
    else
    {
        sRange  = aRange2;
        aRange2 = aRange2->next;
    }
    
    aMerged->minimum = sRange->minimum;
    aMerged->maximum = sRange->maximum;
    
    while( aRange1 != NULL && aRange2 != NULL )
    {
        if ( compareRange( &aRange1->minimum, &aRange2->minimum ) < 0 )
        {
            sRange  = aRange1;
            aRange1 = aRange1->next;
        }
        else
        {
            sRange  = aRange2;
            aRange2 = aRange2->next;
        }
        if ( compareRange( &aMerged->maximum, &sRange->minimum ) < 0 )
        {
            aMerged->next    = aMerged + 1;
            aMerged          = aMerged->next;
            aMerged->prev    = aMerged - 1;
            aMerged->minimum = sRange->minimum;
            aMerged->maximum = sRange->maximum;
        }
        else
        {
            if ( compareRange( &aMerged->maximum, &sRange->maximum ) < 0 )
            {
                aMerged->maximum = sRange->maximum;
            }
        }
    }
    
    for( ; aRange1 != NULL; aRange1 = aRange1->next )
    {
        if ( compareRange( &aMerged->maximum, &aRange1->minimum ) < 0 )
        {
            aMerged->next    = aMerged + 1;
            aMerged          = aMerged->next;
            aMerged->prev    = aMerged - 1;
            aMerged->minimum = aRange1->minimum;
            aMerged->maximum = aRange1->maximum;
        }
        else
        {
            if ( compareRange( &aMerged->maximum, &aRange1->maximum ) < 0 )
            {
                aMerged->maximum = aRange1->maximum;
            }
        }
    }
    
    for( ; aRange2 != NULL; aRange2 = aRange2->next )
    {
        if ( compareRange( &aMerged->maximum, &aRange2->minimum ) < 0 )
        {
            aMerged->next    = aMerged + 1;
            aMerged          = aMerged->next;
            aMerged->prev    = aMerged - 1;
            aMerged->minimum = aRange2->minimum;
            aMerged->maximum = aRange2->maximum;
        }
        else
        {
            if ( compareRange( &aMerged->maximum, &aRange2->maximum ) < 0 )
            {
                aMerged->maximum = aRange2->maximum;
            }
        }        
    }
    
    aMerged->next = NULL;

    return IDE_SUCCESS;
}

IDE_RC mtk::mergeAndRangeNA( smiRange *,
                             smiRange *,
                             smiRange * )
{
    IDE_SET(ideSetErrorCode(mtERR_FATAL_NOT_APPLICABLE));
    
    return IDE_FAILURE;
}

IDE_RC mtk::mergeOrRangeListNA( smiRange *,
                                smiRange **,
                                SInt )
{
    IDE_SET(ideSetErrorCode(mtERR_FATAL_NOT_APPLICABLE));
    
    return IDE_FAILURE;
}
