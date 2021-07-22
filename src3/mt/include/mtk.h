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
 * $Id: mtk.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_MTK_H_
# define _O_MTK_H_ 1

# include <mtcDef.h>
# include <mtd.h>

#define MTK_MAX_RANGE_FUNC_CNT        (128)
#define MTK_MAX_COMPARE_FUNC_CNT      (128 + MTD_MAX_DATATYPE_CNT * MTD_COMPARE_FUNC_MAX_CNT * 2)

class mtk {
private:
    static smiCallBackFunc     mAllRangeFuncs[MTK_MAX_RANGE_FUNC_CNT];
    static mtkRangeFuncIndex   mAllRangeFuncsIndex[MTK_MAX_RANGE_FUNC_CNT];
    static UInt                mAllRangeFuncCount;
    static smiCallBackFunc     mInternalRangeFuncs[];
    
    static mtdCompareFunc      mAllCompareFuncs[MTK_MAX_COMPARE_FUNC_CNT];
    static mtkCompareFuncIndex mAllCompareFuncsIndex[MTK_MAX_COMPARE_FUNC_CNT];
    static UInt                mAllCompareFuncCount;
    static mtdCompareFunc      mInternalCompareFuncs[];

public:

    static IDE_RC initialize( smiCallBackFunc ** aExtRangeFuncGroup,
                              UInt               aExtRangeFuncGroupCnt,
                              mtdCompareFunc  ** aExtCompareFuncGroup,
                              UInt               aExtCompareFuncGroupCnt );

    static UInt rangeByFunc( smiCallBackFunc aRangeFunc );
    static UInt compareByFunc( mtdCompareFunc aRangeFunc );
    
    static smiCallBackFunc rangeById( UInt aId );
    static mtdCompareFunc compareById( UInt aId );
    
    static SInt compareRange( const smiCallBack * aCallBack1,
                              const smiCallBack * aCallBack2 );

    static IDE_RC estimateRangeDefault( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        UInt         aArgument,
                                        UInt*        aSize );

    static IDE_RC estimateRangeDefaultLike( mtcNode*     aNode,
                                            mtcTemplate* aTemplate,
                                            UInt         aArgument,
                                            UInt*        aSize );

    // PROJ-2002 Column Security
    static IDE_RC estimateRangeDefaultLike4Echar( mtcNode*     aNode,
                                                  mtcTemplate* aTemplate,
                                                  UInt         aArgument,
                                                  UInt*        aSize );

    static IDE_RC estimateRangeNA( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   UInt         aArgument,
                                   UInt*        aSize );

    static IDE_RC extractRangeNA( mtcNode*       aNode,
                                  mtcTemplate*   aTemplate,
                                  mtkRangeInfo * aInfo,
                                  smiRange*      aRange );

    static IDE_RC extractNotNullRange( mtcNode*       aNode,
                                       mtcTemplate*   aTemplate,
                                       mtkRangeInfo * aInfo,
                                       smiRange*      aRange );

    static SInt compareMinimumLimit( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

    static SInt compareMaximumLimit4Mtd( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

    static SInt compareMaximumLimit4Stored( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

    static IDE_RC rangeCallBackGE4Mtd( idBool     * aResult,
                                       const void * aColVal,
                                       void       *, /* aDirectKey */
                                       UInt        , /* aDirectKeyPartialSize */
                                       const scGRID,
                                       void       * aData );
    
    static IDE_RC rangeCallBackGE4Stored( idBool     * aResult,
                                          const void * aColVal,
                                          void       *, /* aDirectKey */
                                          UInt        , /* aDirectKeyPartialSize */
                                          const scGRID,
                                          void       * aData );

    static IDE_RC rangeCallBackLE4Mtd( idBool     * aResult,
                                       const void * aColVal,
                                       void       *, /* aDirectKey */
                                       UInt        , /* aDirectKeyPartialSize */
                                       const scGRID,
                                       void       * aData );

    static IDE_RC rangeCallBackLE4Stored( idBool     * aResult,
                                          const void * aColVal,
                                          void       *, /* aDirectKey */
                                          UInt        , /* aDirectKeyPartialSize */
                                          const scGRID,
                                          void       * aData );

    static IDE_RC rangeCallBackGT4Mtd( idBool     * aResult,
                                       const void * aColVal,
                                       void       *, /* aDirectKey */
                                       UInt        , /* aDirectKeyPartialSize */
                                       const scGRID,
                                       void       * aData );

    static IDE_RC rangeCallBackGT4Stored( idBool     * aResult,
                                          const void * aColVal,
                                          void       *, /* aDirectKey */
                                          UInt        , /* aDirectKeyPartialSize */
                                          const scGRID,
                                          void       * aData );

    
    static IDE_RC rangeCallBackLT4Mtd( idBool     * aResult,
                                       const void * aColVal,
                                       void       *, /* aDirectKey */
                                       UInt        , /* aDirectKeyPartialSize */
                                       const scGRID,
                                       void       * aData );

    static IDE_RC rangeCallBackLT4Stored( idBool     * aResult,
                                          const void * aColVal,
                                          void       *, /* aDirectKey */
                                          UInt        , /* aDirectKeyPartialSize */
                                          const scGRID,
                                          void       * aData );

    /* PROJ-2433 */
    static IDE_RC rangeCallBackGE4IndexKey( idBool     * aResult,
                                            const void * aColVal,
                                            void       * aDirectKey,
                                            UInt         aDirectKeyPartialSize,
                                            const scGRID,
                                            void       * aData );

    static IDE_RC rangeCallBackLE4IndexKey( idBool     * aResult,
                                            const void * aColVal,
                                            void       * aDirectKey,
                                            UInt         aDirectKeyPartialSize,
                                            const scGRID,
                                            void       * aData );

    static IDE_RC rangeCallBackGT4IndexKey( idBool     * aResult,
                                            const void * aColVal,
                                            void       * aDirectKey,
                                            UInt         aDirectKeyPartialSize,
                                            const scGRID,
                                            void       * aData );
    
    static IDE_RC rangeCallBackLT4IndexKey( idBool     * aResult,
                                            const void * aColVal,
                                            void       * aDirectKey,
                                            UInt         aDirectKeyPartialSize,
                                            const scGRID,
                                            void       * aData );
    /* PROJ-2433 end */

    static IDE_RC rangeCallBack4Rid( idBool     * aResult,
                                     const void * aColVal,
                                     void       *, /* aDirectKey */
                                     UInt        , /* aDirectKeyPartialSize */
                                     const scGRID,
                                     void       * aData );

    static IDE_RC addCompositeRange( smiRange* aComposite,
                                     smiRange* aRange );

    static IDE_RC mergeAndRangeDefault( smiRange* aMerged,
                                        smiRange* aRange1,
                                        smiRange* aRange2 );

    static IDE_RC mergeOrRangeListDefault( smiRange  * aMerged,
                                           smiRange ** aRangeListArray,
                                           SInt        aRangeCount );
    
    static IDE_RC mergeOrRangeNotBetween( smiRange* aMerged,
                                          smiRange* aRange1,
                                          smiRange* aRange2 );
    
    static IDE_RC mergeAndRangeNA( smiRange* aMerged,
                                   smiRange* aRange1,
                                   smiRange* aRange2 );

    static IDE_RC mergeOrRangeListNA( smiRange  * aMerged,
                                      smiRange ** aRangeList,
                                      SInt        aRangeCount );
};

#endif /* _O_MTK_H_ */
