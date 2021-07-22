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
 * $Id: mtf.h 85458 2019-05-16 07:13:00Z andrew.shin $
 **********************************************************************/

#ifndef _O_MTF_H_
# define _O_MTF_H_ 1

# include <mtcDef.h>
# include <mtdTypes.h>
# include <smi.h>

#define MTF_MEMPOOL_ELEMENT_CNT     (4)
#define MTF_KEEP_ORDERBY_COLUMN_MAX (32)
#define MTF_KEEP_ORDERBY_POS        (3)

typedef enum mtfKeepOrderMode
{
    MTF_KEEP_FIRST_ORDER_ASC = 0,
    MTF_KEEP_FIRST_ORDER_ASC_NULLS_LAST,
    MTF_KEEP_FIRST_ORDER_ASC_NULLS_FIRST,
    MTF_KEEP_FIRST_ORDER_DESC,
    MTF_KEEP_FIRST_ORDER_DESC_NULLS_LAST,
    MTF_KEEP_FIRST_ORDER_DESC_NULLS_FIRST,
    MTF_KEEP_LAST_ORDER_ASC,
    MTF_KEEP_LAST_ORDER_ASC_NULLS_LAST,
    MTF_KEEP_LAST_ORDER_ASC_NULLS_FIRST,
    MTF_KEEP_LAST_ORDER_DESC,
    MTF_KEEP_LAST_ORDER_DESC_NULLS_LAST,
    MTF_KEEP_LAST_ORDER_DESC_NULLS_FIRST
} mtfKeepOrderMode;

typedef enum mtfKeepAction
{
    MTF_KEEP_ACTION_INIT = 0,
    MTF_KEEP_ACTION_SKIP,
    MTF_KEEP_ACTION_AGGR
} mtfKeepAction;

typedef struct mtfKeepOrderData
{
    UInt     mCount;
    UInt     mOptions[MTF_KEEP_ORDERBY_COLUMN_MAX];
    UInt     mOffsets[MTF_KEEP_ORDERBY_COLUMN_MAX];
    UChar  * mData;
    idBool   mIsFirst;
} mtfKeepOrderData;

class mtf {
private:
    static const mtfModule*  mInternalModule[];
    static       mtfModule** mExternalModule;

    static const UInt comparisonGroup[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM];

    static IDE_RC alloc( void* aInfo,
                         UInt  aSize,
                         void** aMemPtr);

    static IDE_RC initConversionNodeForInitialize( mtcNode** aConversionNode,
                                                   mtcNode*  aNode,
                                                   void*     aInfo );

    static IDE_RC convertInternal( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   mtcTemplate* aTemplate );

    static IDE_RC initializeComparisonTable( void );

    static IDE_RC finalizeComparisonTable( void );

    // Conversion Cost를 변경 및 원복
    static IDE_RC changeConvertCost( void );
    static IDE_RC restoreConvertCost( void );

    // 원복하기 위해 저장하는 공간
    static ULong  saveCost[3][3];
    static const mtdModule*** comparisonTable;

    // PROJ-2527 WITHIN GROUP AGGR
    static iduMemPool mFuncMemoryPool;

public:

    static const mtdBooleanType andMatrix[3][3];

    static const mtdBooleanType orMatrix[3][3];

    static mtcNode* convertedNode( mtcNode*     aNode,
                                   mtcTemplate* aTemplate );

    static IDE_RC postfixCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

    static IDE_RC convertCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

    static IDE_RC convertLeftCalculate( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

    static IDE_RC initializeDefault( void );

    static IDE_RC finalizeDefault( void );

    static IDE_RC estimateNA( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack );

    static IDE_RC calculateNA( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

    static IDE_RC makeConversionNode( mtcNode**        aConversionNode,
                                      mtcNode*         aNode,
                                      mtcTemplate*     aTemplate,
                                      mtcStack*        aStack,
                                      mtcCallBack*     aCallBack,
                                      const mtvTable*  aTable );

    static IDE_RC makeConversionNodes( mtcNode*          aNode,
                                       mtcNode*          aArguments,
                                       mtcTemplate*      aTemplate,
                                       mtcStack*         aStack,
                                       mtcCallBack*      aCallBack,
                                       const mtdModule** aModule );

    static IDE_RC makeLeftConversionNodes( mtcNode*          aNode,
                                           mtcNode*          aArguments,
                                           mtcTemplate*      aTemplate,
                                           mtcStack*         aStack,
                                           mtcCallBack*      aCallBack,
                                           const mtdModule** aModule );

    static IDE_RC initializeComparisonTemplate(
        mtfSubModule**** aTable,
        mtfSubModule*    aGroupTable[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM],
        mtfSubModule*    aDefaultModule );

    static IDE_RC finalizeComparisonTemplate( mtfSubModule**** aTable );

    static IDE_RC initializeTemplate( mtfSubModule*** aTable,
                                      mtfSubModule*   aEstimates,
                                      mtfSubModule*   aDefaultModule );

    static IDE_RC finalizeTemplate( mtfSubModule*** aTable );

    static IDE_RC getCharFuncResultModule( const mtdModule** aResultModule,
                                           const mtdModule*  aArgumentModule );

    static IDE_RC getLobFuncResultModule( const mtdModule** aResultModule,
                                          const mtdModule*  aArgumentModule );

    static IDE_RC getLikeModule( const mtdModule** aResultModule,
                                 const mtdModule*  aSearchValueModule,
                                 const mtdModule*  aPatternModule);

    // PROJ-1579 NCHAR
    static IDE_RC getCharFuncCharResultModule( const mtdModule** aResultModule,
                                         const mtdModule*  aArgumentModule );

    static IDE_RC initialize( mtfModule ***  aExtFuncModuleGroup,
                              UInt           aGroupCnt );

    static IDE_RC finalize( void );

    static IDE_RC moduleByName( const mtfModule** aModule,
                                idBool*           aExist,
                                const void*       aName,
                                UInt              aLength );

    // To Fix BUG-12306 Filter가 필요한지 검사
    static IDE_RC checkNeedFilter( mtcTemplate * aTmplate,
                                   mtcNode     * aNode,
                                   idBool      * aNeedFilter );

    // PROJ-1075 module->no에 대응되는 comparisonModule을 반환.
    static IDE_RC getComparisonModule( const mtdModule** aModule,
                                       UInt              aNo1,
                                       UInt              aNo2 );

    // PROJ-1075 module->no에 대응되는 mtfSubModule을 반환.
    static IDE_RC getSubModule1Arg( const mtfSubModule** aModule,
                                    mtfSubModule**       aTable,
                                    UInt aNo );

    // PROJ-1075 2개의 module->no에 대응되는 mtfSubModule을 반환.
    static IDE_RC getSubModule2Args( const mtfSubModule** aModule,
                                     mtfSubModule***      aTable,
                                     UInt                 aNo1,
                                     UInt                 aNo2 );

    // BUG-15208, BUG-15212 동치 비교가 가능한 Data Type인지 검사
    static idBool isEquiValidType( const mtdModule * aModule );
    static idBool isGreaterLessValidType( const mtdModule * aModule );

    // fix for BUG-15930 , fix BUG-19639
    static mtlNCRet nextChar( const UChar      * aFence,
                              UChar           ** aCursor,
                              UChar           ** aCursorPrev,
                              const mtlModule  * aLanguage );

    static IDE_RC copyString( UChar            * aDstStr,
                              UInt               aDstStrSize,
                              UInt             * aDstStrCopySize,
                              UChar            * aSrcStr,
                              UInt               aSrcStrSize,
                              const mtlModule  * aLanguage );

    static IDE_RC truncIncompletedString( UChar            * aString,
                                          UInt               aSize,
                                          UInt             * aTruncatedSize,
                                          const mtlModule  * aLanguage );

    // PROJ-1579 NCHAR
    static IDE_RC makeUFromChar( const mtlModule  * aSourceCharSet,
                                 UChar            * aSourceIndex,
                                 UChar            * aSourceFence,
                                 UChar            * aResultValue,
                                 UChar            * aResultFence,
                                 UInt             * aCharLength );

    static IDE_RC checkNeedFilter4Like( mtcTemplate * aTmplate,
                                        mtcNode     * aNode,
                                        idBool      * aNeedFilter );

    // PROJ-2180
    static IDE_RC getQuantFuncCalcType( mtcNode *              aNode,
            					   	    mtcTemplate *          aTemplate,
            						    mtfQuantFuncCalcType * aCalcType );

    static IDE_RC allocQuantArgData( mtcNode *     aNode,
			  	  	  	  	         mtcTemplate * aTemplate,
			  	  	  	  	         mtcStack *    aStack,
			  	  	  	  	         mtcCallBack * aCallBack );

    static IDE_RC buildQuantArgData( mtcNode *     aNode,
    		                         mtcStack *    aStack,
    		                         SInt          aRemain,
    		                         void *        aInfo,
                                     mtcTemplate * aTemplate );

    static inline void * getArgDataPtr( mtcNode *     aNode,
                                        mtcTemplate * aTemplate )
    {
        void *     sArgData;
        mtcTuple * sTuple;

        sTuple    = &aTemplate->rows[aNode->table];
        sArgData  = ((UChar*)sTuple->row +
                             sTuple->columns[aNode->column + 1].column.offset);

    	return sArgData;
    }

    static inline void * getCalcInfoPtr( mtcNode *     aNode,
    		                             mtcTemplate * aTemplate )
    {
    	return aTemplate->rows[aNode->table].execute[aNode->column + 1].calculateInfo;
    }

    /* BUG-39237 adding sys_guid() function that returning byte(16) type. */
    static IDE_RC hex2Ascii( UChar aIn, UChar *aOut );

    // PROJ-2527 WITHIN GROUP AGGR
    static IDE_RC initializeFuncDataBasicInfo( mtfFuncDataBasicInfo  * aFuncDataBasicInfo,
                                               iduMemory             * aMemoryMgr );

    static IDE_RC allocFuncDataMemory( iduMemory ** aMemoryMgr );

    static void freeFuncDataMemory( iduMemory * aMemoryMgr );

    /* BUG-46892 */
    static IDE_RC checkFuncDataMemoryMaximum( mtcTemplate * aTemplate,
                                              iduMemory   * aMemoryMgr,
                                              ULong         aSize );

    static IDE_RC setKeepOrderData( mtcNode          * aNode,
                                    mtcStack         * aStack,
                                    iduMemory        * aMemory,
                                    UChar            * aDenseRankOpt,
                                    mtfKeepOrderData * aKeepOrderData,
                                    UInt               aOrderByStackPos );

    static void getKeepAction( mtcStack         * aStack,
                               mtfKeepOrderData * aKeepOrderData,
                               UInt             * aAction );
};

class mtfToCharInterface
{
public:
    static IDE_RC makeFormatInfo( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  UChar*       aFormat,
                                  UInt         aFormatLen,
                                  mtcCallBack* aCallBack );

   static IDE_RC mtfTo_charCalculateNumberFor2Args( mtcNode*     aNode,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                void*        aInfo,
                                                mtcTemplate* aTemplate );

    static IDE_RC mtfTo_charCalculateDateFor2Args( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );
};

#endif /* _O_MTF_H_ */

