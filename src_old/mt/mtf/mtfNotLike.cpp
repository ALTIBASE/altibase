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
 * $Id: mtfNotLike.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtuProperty.h>

extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdEchar;
extern mtdModule mtdEvarchar;
extern mtdModule mtdBinary;
extern mtdModule mtdClob;

extern mtfModule mtfLike;

extern mtfModule mtfNotLike;

extern mtlModule mtlAscii;

extern IDE_RC mtfLikeCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateNormal( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateNormalFast( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateEqualFast( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateIsNotNullFast( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateLengthFast( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateOnePercentFast( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateMB( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateMBNormal( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateMBNormalFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateLengthMBFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4XlobLocator( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4XlobLocatorNormal( mtcNode*     aNode,
                                                  mtcStack*    aStack,
                                                  SInt         aRemain,
                                                  void*        aInfo,
                                                  mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4XlobLocatorNormalFast( mtcNode*     aNode,
                                                      mtcStack*    aStack,
                                                      SInt         aRemain,
                                                      void*        aInfo,
                                                      mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateEqual4XlobLocatorFast( mtcNode*     aNode,
                                                     mtcStack*    aStack,
                                                     SInt         aRemain,
                                                     void*        aInfo,
                                                     mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateIsNotNull4XlobLocatorFast( mtcNode*     aNode,
                                                         mtcStack*    aStack,
                                                         SInt         aRemain,
                                                         void*        aInfo,
                                                         mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateLength4XlobLocatorFast( mtcNode*     aNode,
                                                      mtcStack*    aStack,
                                                      SInt         aRemain,
                                                      void*        aInfo,
                                                      mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateOnePercent4XlobLocatorFast( mtcNode*     aNode,
                                                          mtcStack*    aStack,
                                                          SInt         aRemain,
                                                          void*        aInfo,
                                                          mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4XlobLocatorMB( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4XlobLocatorMBNormal( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4XlobLocatorMBNormalFast( mtcNode*     aNode,
                                                        mtcStack*    aStack,
                                                        SInt         aRemain,
                                                        void*        aInfo,
                                                        mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateLength4XlobLocatorMBFast( mtcNode*     aNode,
                                                        mtcStack*    aStack,
                                                        SInt         aRemain,
                                                        void*        aInfo,
                                                        mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4Echar( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4EcharNormal( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4EcharNormalFast( mtcNode*     aNode,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                void*        aInfo,
                                                mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateEqual4EcharFast( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateIsNotNull4EcharFast( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateLength4EcharFast( mtcNode*     aNode,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                void*        aInfo,
                                                mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateOnePercent4EcharFast( mtcNode*     aNode,
                                                    mtcStack*    aStack,
                                                    SInt         aRemain,
                                                    void*        aInfo,
                                                    mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4EcharMB( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4EcharMBNormal( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculate4EcharMBNormalFast( mtcNode*     aNode,
                                                  mtcStack*    aStack,
                                                  SInt         aRemain,
                                                  void*        aInfo,
                                                  mtcTemplate* aTemplate );

extern IDE_RC mtfLikeCalculateLength4EcharMBFast( mtcNode*     aNode,
                                                  mtcStack*    aStack,
                                                  SInt         aRemain,
                                                  void*        aInfo,
                                                  mtcTemplate* aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculate4ClobValue( mtcNode     * aNode,
                                          mtcStack    * aStack,
                                          SInt          aRemain,
                                          void        * aInfo,
                                          mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculate4ClobValueMB( mtcNode     * aNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * aInfo,
                                            mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculate4ClobValueNormal( mtcNode     * aNode,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                void        * aInfo,
                                                mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculate4ClobValueMBNormal( mtcNode     * aNode,
                                                  mtcStack    * aStack,
                                                  SInt          aRemain,
                                                  void        * aInfo,
                                                  mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculate4ClobValueNormalFast( mtcNode     * aNode,
                                                    mtcStack    * aStack,
                                                    SInt          aRemain,
                                                    void        * aInfo,
                                                    mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculateEqual4ClobValueFast( mtcNode     * aNode,
                                                   mtcStack    * aStack,
                                                   SInt          aRemain,
                                                   void        * aInfo,
                                                   mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculateIsNotNull4ClobValueFast( mtcNode     * aNode,
                                                       mtcStack    * aStack,
                                                       SInt          aRemain,
                                                       void        * aInfo,
                                                       mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculateLength4ClobValueFast( mtcNode     * aNode,
                                                    mtcStack    * aStack,
                                                    SInt          aRemain,
                                                    void        * aInfo,
                                                    mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculateOnePercent4ClobValueFast( mtcNode     * aNode,
                                                        mtcStack    * aStack,
                                                        SInt          aRemain,
                                                        void        * aInfo,
                                                        mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculate4ClobValueMBNormalFast( mtcNode     * aNode,
                                                      mtcStack    * aStack,
                                                      SInt          aRemain,
                                                      void        * aInfo,
                                                      mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
extern IDE_RC mtfLikeCalculateLength4ClobValueMBFast( mtcNode     * aNode,
                                                      mtcStack    * aStack,
                                                      SInt          aRemain,
                                                      void        * aInfo,
                                                      mtcTemplate * aTemplate );

extern IDE_RC mtfLikeFormatInfo( mtcNode            * aNode,
                                 mtcTemplate        * aTemplate,
                                 mtcStack           * aStack,
                                 mtcLikeFormatInfo ** aFormatInfo,
                                 UShort             * aFormatLen,
                                 mtcCallBack        * aCallBack );

static mtcName mtfNotLikeFunctionName[1] = {
    { NULL, 8, (void*)"NOT LIKE" }
};

static IDE_RC mtfNotLikeEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

// PROJ-1755
static IDE_RC mtfNotLikeEstimateCharFast( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

// PROJ-1755
static IDE_RC mtfNotLikeEstimateBitFast( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

// PROJ-1755
static IDE_RC mtfNotLikeEstimateXlobLocatorFast( mtcNode*     aNode,
                                                 mtcTemplate* aTemplate,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 mtcCallBack* aCallBack );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfNotLikeEstimateClobValueFast( mtcNode     * aNode,
                                               mtcTemplate * aTemplate,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               mtcCallBack * aCallBack );

// PROJ-2002 Column Security
static IDE_RC mtfNotLikeEstimateEcharFast( mtcNode*     aNode,
                                           mtcTemplate* aTemplate,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           mtcCallBack* aCallBack );

mtfModule mtfNotLike = {
    3|MTC_NODE_OPERATOR_NOT_RANGED|
        MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_FILTER_NEED|
        MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    0.95,  // TODO : default selectivity 
    mtfNotLikeFunctionName,
    &mtfLike,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNotLikeEstimate
};

static IDE_RC mtfNotLikeCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateNormal( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateNormalFast( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateEqualFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateIsNullFast( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateLengthFast( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateOnePercentFast( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateMB( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateMBNormal( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateMBNormalFast( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateLengthMBFast( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4XlobLocator( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4XlobLocatorNormal( mtcNode*     aNode,
                                                     mtcStack*    aStack,
                                                     SInt         aRemain,
                                                     void*        aInfo,
                                                     mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4XlobLocatorNormalFast( mtcNode*     aNode,
                                                         mtcStack*    aStack,
                                                         SInt         aRemain,
                                                         void*        aInfo,
                                                         mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateEqual4XlobLocatorFast( mtcNode*     aNode,
                                                        mtcStack*    aStack,
                                                        SInt         aRemain,
                                                        void*        aInfo,
                                                        mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateIsNull4XlobLocatorFast( mtcNode*     aNode,
                                                         mtcStack*    aStack,
                                                         SInt         aRemain,
                                                         void*        aInfo,
                                                         mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateLength4XlobLocatorFast( mtcNode*     aNode,
                                                         mtcStack*    aStack,
                                                         SInt         aRemain,
                                                         void*        aInfo,
                                                         mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateOnePercent4XlobLocatorFast( mtcNode*     aNode,
                                                             mtcStack*    aStack,
                                                             SInt         aRemain,
                                                             void*        aInfo,
                                                             mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4XlobLocatorMB( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4XlobLocatorMBNormal( mtcNode*     aNode,
                                                       mtcStack*    aStack,
                                                       SInt         aRemain,
                                                       void*        aInfo,
                                                       mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4XlobLocatorMBNormalFast( mtcNode*     aNode,
                                                           mtcStack*    aStack,
                                                           SInt         aRemain,
                                                           void*        aInfo,
                                                           mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateLength4XlobLocatorMBFast( mtcNode*     aNode,
                                                           mtcStack*    aStack,
                                                           SInt         aRemain,
                                                           void*        aInfo,
                                                           mtcTemplate* aTemplate );

// PROJ-2002 Column Security
static IDE_RC mtfNotLikeCalculate4Echar( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4EcharNormal( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4EcharNormalFast( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateEqual4EcharFast( mtcNode*     aNode,
                                                  mtcStack*    aStack,
                                                  SInt         aRemain,
                                                  void*        aInfo,
                                                  mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateIsNull4EcharFast( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateLength4EcharFast( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateOnePercent4EcharFast( mtcNode*     aNode,
                                                       mtcStack*    aStack,
                                                       SInt         aRemain,
                                                       void*        aInfo,
                                                       mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4EcharMB( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4EcharMBNormal( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculate4EcharMBNormalFast( mtcNode*     aNode,
                                                     mtcStack*    aStack,
                                                     SInt         aRemain,
                                                     void*        aInfo,
                                                     mtcTemplate* aTemplate );

static IDE_RC mtfNotLikeCalculateLength4EcharMBFast( mtcNode*     aNode,
                                                     mtcStack*    aStack,
                                                     SInt         aRemain,
                                                     void*        aInfo,
                                                     mtcTemplate* aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfNotLikeCalculate4ClobValue( mtcNode     * aNode,
                                             mtcStack    * aStack,
                                             SInt          aRemain,
                                             void        * aInfo,
                                             mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfNotLikeCalculate4ClobValueMB( mtcNode     * aNode,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               void        * aInfo,
                                               mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfNotLikeCalculate4ClobValueNormal( mtcNode     * aNode,
                                                   mtcStack    * aStack,
                                                   SInt          aRemain,
                                                   void        * aInfo,
                                                   mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfNotLikeCalculate4ClobValueMBNormal( mtcNode     * aNode,
                                                     mtcStack    * aStack,
                                                     SInt          aRemain,
                                                     void        * aInfo,
                                                     mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC mtfNotLikeCalculate4ClobValueNormalFast( mtcNode     * aNode,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                void        * aInfo,
                                                mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC mtfNotLikeCalculateEqual4ClobValueFast( mtcNode     * aNode,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               void        * aInfo,
                                               mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC mtfNotLikeCalculateIsNull4ClobValueFast( mtcNode     * aNode,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                void        * aInfo,
                                                mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC mtfNotLikeCalculateLength4ClobValueFast( mtcNode     * aNode,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                void        * aInfo,
                                                mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC mtfNotLikeCalculateOnePercent4ClobValueFast( mtcNode     * aNode,
                                                    mtcStack    * aStack,
                                                    SInt          aRemain,
                                                    void        * aInfo,
                                                    mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC mtfNotLikeCalculate4ClobValueMBNormalFast( mtcNode     * aNode,
                                                  mtcStack    * aStack,
                                                  SInt          aRemain,
                                                  void        * aInfo,
                                                  mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC mtfNotLikeCalculateLength4ClobValueMBFast( mtcNode     * aNode,
                                                  mtcStack    * aStack,
                                                  SInt          aRemain,
                                                  void        * aInfo,
                                                  mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteMB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculateMB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculateNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteMBNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculateMBNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// BUG-16276
const mtcExecute mtfExecute4XlobLocator = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4XlobLocator,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecute4XlobLocatorMB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4XlobLocatorMB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// BUG-16276
const mtcExecute mtfExecute4XlobLocatorNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4XlobLocatorNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecute4XlobLocatorMBNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4XlobLocatorMBNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// PROJ-2002 Column Security
const mtcExecute mtfExecute4Echar = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4Echar,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecute4EcharMB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4EcharMB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// PROJ-2002 Column Security
const mtcExecute mtfExecute4EcharNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4EcharNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecute4EcharMBNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4EcharMBNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecute4ClobValue = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4ClobValue,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecute4ClobValueMB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4ClobValueMB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecute4ClobValueNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4ClobValueNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecute4ClobValueMBNormal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotLikeCalculate4ClobValueMBNormal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNotLikeEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack )
{
    extern mtdModule     mtdBoolean;

    mtcNode            * sNode;
    mtcNode            * sFormatNode;
    mtcNode            * sEscapeNode;

    const mtdModule    * sModules[3];
    mtcColumn          * sIndexColumn;
    SInt                 sModuleId;
    UInt                 sPrecision;    
    mtcLikeFormatInfo  * sFormatInfo;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sNode = aNode->arguments;
    sFormatNode = sNode->next;
    sEscapeNode = sFormatNode->next;

    //IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // BUG-40992 FATAL when using _prowid
    // 인자의 경우 mtcStack 의 column 값을 이용하면 된다.
    sIndexColumn     = aStack[1].column;

    // BUG-22611
    // switch-case에 UInt 형으로 음수값이 두번 이상 오면 서버 비정상 종료
    // ex )  case MTD_BIT_ID: ==> (UInt)-7 
    //       case MTD_VARBIT_ID: ==> (UInt)-8
    // 따라서 SInt 형으로 타입 캐스팅 하도록 수정함 
    sModuleId = (SInt)sIndexColumn->module->id;

    switch ( sModuleId )
    {
        case MTD_CLOB_LOCATOR_ID:
        {
            if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK ) == MTC_NODE_REESTIMATE_FALSE )
            {
                sModules[0] = &mtdVarchar;
                sModules[1] = sModules[0];
                sModules[2] = sModules[0];

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments->next,
                                                    aTemplate,
                                                    aStack + 2,
                                                    aCallBack,
                                                    sModules + 1 )
                          != IDE_SUCCESS );

                if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE)
                {
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4XlobLocator;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4XlobLocatorMB;
                    }
                }
                else
                {
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4XlobLocatorNormal;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4XlobLocatorMBNormal;
                    }                    
                }          
            }
            else
            {
                // PROJ-1755
                // format string에 따른 최적 함수를 연결한다.
                IDE_TEST( mtfNotLikeEstimateXlobLocatorFast( aNode,
                                                             aTemplate,
                                                             aStack,
                                                             aRemain,
                                                             aCallBack )
                          != IDE_SUCCESS );
            }
            break;
        }

        case MTD_CLOB_ID:
        {
            if ( aTemplate->isBaseTable( aTemplate, aNode->arguments->table ) == ID_TRUE )
            {
                if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK ) == MTC_NODE_REESTIMATE_FALSE )
                {
                    IDE_TEST( mtf::getLobFuncResultModule( &sModules[0],
                                                           aStack[1].column->module )
                              != IDE_SUCCESS );
                    sModules[1] = &mtdVarchar;
                    sModules[2] = sModules[1];

                    IDE_TEST( mtf::makeConversionNodes( aNode,
                                                        aNode->arguments,
                                                        aTemplate,
                                                        aStack + 1,
                                                        aCallBack,
                                                        sModules )
                              != IDE_SUCCESS );

                    if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE)
                    {
                        if( sIndexColumn->language == &mtlAscii )
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4XlobLocator;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4XlobLocatorMB;
                        }
                    }
                    else
                    {
                        if( sIndexColumn->language == &mtlAscii )
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4XlobLocatorNormal;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4XlobLocatorMBNormal;
                        }
                    }
                }
                else
                {
                    // PROJ-1755
                    // format string에 따른 최적 함수를 연결한다.
                    IDE_TEST( mtfNotLikeEstimateXlobLocatorFast( aNode,
                                                                 aTemplate,
                                                                 aStack,
                                                                 aRemain,
                                                                 aCallBack )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                if ( (aNode->lflag & MTC_NODE_REESTIMATE_MASK) == MTC_NODE_REESTIMATE_FALSE )
                {
                    sModules[0] = &mtdClob;
                    sModules[1] = &mtdVarchar;
                    sModules[2] = &mtdVarchar;

                    IDE_TEST( mtf::makeConversionNodes( aNode,
                                                        aNode->arguments,
                                                        aTemplate,
                                                        aStack + 1,
                                                        aCallBack,
                                                        sModules )
                              != IDE_SUCCESS );

                    if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE )
                    {
                        if ( sIndexColumn->language == &mtlAscii )
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4ClobValue;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4ClobValueMB;
                        }
                    }
                    else
                    {
                        if ( sIndexColumn->language == &mtlAscii )
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4ClobValueNormal;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column] =
                                mtfExecute4ClobValueMBNormal;
                        }
                    }
                }
                else
                {
                    // PROJ-1755
                    // format string에 따른 최적 함수를 연결한다.
                    IDE_TEST( mtfNotLikeEstimateClobValueFast( aNode,
                                                               aTemplate,
                                                               aStack,
                                                               aRemain,
                                                               aCallBack )
                              != IDE_SUCCESS );
                }
            }
            break;
        }

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
        {
            if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK ) == MTC_NODE_REESTIMATE_FALSE )
            {
                IDE_TEST( mtf::getLikeModule( &sModules[0],
                                              aStack[1].column->module,
                                              aStack[2].column->module )
                          != IDE_SUCCESS );
                sModules[1] = sModules[0];
                sModules[2] = sModules[0];

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
                
                if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE)
                {
                    aTemplate->rows[aNode->table].execute[aNode->column] =
                        mtfExecute;
                }
                else
                {
                    aTemplate->rows[aNode->table].execute[aNode->column] =
                        mtfExecuteNormal;
                }      
            }
            else
            {
                // PROJ-1755
                // format string에 따른 최적 함수를 연결한다.
                IDE_TEST( mtfNotLikeEstimateBitFast( aNode,
                                                     aTemplate,
                                                     aStack,
                                                     aRemain,
                                                     aCallBack )
                          != IDE_SUCCESS );
            }
            break;
        }

        case MTD_ECHAR_ID:
        case MTD_EVARCHAR_ID:
        {
            if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK ) == MTC_NODE_REESTIMATE_FALSE )
            {
                IDE_TEST( mtf::getLikeModule( &sModules[0],
                                              aStack[1].column->module,
                                              aStack[2].column->module )
                          != IDE_SUCCESS );
                sModules[1] = sModules[0];

                // escape 문자는 원본 타입으로 변환한다.
                if ( sModules[0] == &mtdEchar )
                {
                    sModules[2] = &mtdChar;
                }
                else
                {
                    sModules[2] = &mtdVarchar;
                }

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );

                if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE)
                {
                    //fix for BUG-15930
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4Echar;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4EcharMB;
                    }
                }
                else
                {
                    //fix for BUG-15930
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4EcharNormal;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute4EcharMBNormal;
                    }  
                }
            }            
            else
            {
                // format string에 따른 최적 함수를 연결한다.
                IDE_TEST( mtfNotLikeEstimateEcharFast( aNode,
                                                       aTemplate,
                                                       aStack,
                                                       aRemain,
                                                       aCallBack )
                          != IDE_SUCCESS );
            }
            break;
        }

        default:
        {
            if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK ) == MTC_NODE_REESTIMATE_FALSE )
            {
                IDE_TEST( mtf::getLikeModule( &sModules[0],
                                              aStack[1].column->module,
                                              aStack[2].column->module )
                          != IDE_SUCCESS );
                sModules[1] = sModules[0];
                sModules[2] = sModules[0];

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );

                if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE)
                {
                    //fix for BUG-15930
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecute;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecuteMB;
                    }
                }                
                else
                {
                    //fix for BUG-15930
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecuteNormal;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column] =
                            mtfExecuteMBNormal;
                    }
                } 
            }
            else
            {
                // PROJ-1755
                // format string에 따른 최적 함수를 연결한다.
                IDE_TEST( mtfNotLikeEstimateCharFast( aNode,
                                                      aTemplate,
                                                      aStack,
                                                      aRemain,
                                                      aCallBack )
                          != IDE_SUCCESS );
            }
            break;
        }
    }

    if ( aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo == NULL)
    {
        sPrecision = MTC_LIKE_PATTERN_MAX_SIZE;        
    }
    else
    {
        // BUG-37057 패턴의 길이는 문자열의 길이를 넘어갈수 없다.
        sFormatInfo = (mtcLikeFormatInfo*)
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo;

        sPrecision = sFormatInfo->patternSize;
    }
       
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     sPrecision * ID_SIZEOF(mtcLikeBlockInfo),
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdBinary,
                                     1,
                                     sPrecision * ID_SIZEOF(UChar),
                                     0 )
              != IDE_SUCCESS );
 
    // PROJ-1755
    if( ( MTC_NODE_IS_DEFINED_VALUE( sFormatNode ) == ID_TRUE )
        &&
        ( ( ( aTemplate->rows[sFormatNode->table].lflag & MTC_TUPLE_TYPE_MASK )
            == MTC_TUPLE_TYPE_CONSTANT ) ||
          ( ( aTemplate->rows[sFormatNode->table].lflag & MTC_TUPLE_TYPE_MASK )
            == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
    {
        if( sEscapeNode == NULL )
        {
            // format이 상수이고 esacpe문자가 없는 경우

            aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
            aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
        }
        else
        {
            if( ( MTC_NODE_IS_DEFINED_VALUE( sEscapeNode ) == ID_TRUE )
                &&
                ( ( ( aTemplate->rows[sEscapeNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                    == MTC_TUPLE_TYPE_CONSTANT ) ||
                  ( ( aTemplate->rows[sEscapeNode->table].lflag & MTC_TUPLE_TYPE_MASK )
                    == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
            {
                // format이 상수이고 escape문자도 상수인 경우

                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            }
            else
            {
                // format이 상수이고 escape문자가 변수인 경우

                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
        }
            
        // BUG-38070 undef type으로 re-estimate하지 않는다.
        if ( ( aTemplate->variableRow != ID_USHORT_MAX ) &&
             ( ( aNode->lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) )
        {
            if ( aTemplate->rows[aTemplate->variableRow].
                 columns->module->id == MTD_UNDEF_ID )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // format이 변수인 경우

        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeEstimateCharFast( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt,
                                   mtcCallBack* aCallBack )
{
    mtcColumn         * sIndexColumn;
    mtcLikeFormatInfo * sFormatInfo;
    UShort              sFormatLen;    

    // BUG-40992 FATAL when using _prowid
    // 인자의 경우 mtcStack 의 column 값을 이용하면 된다.
    sIndexColumn     = aStack[1].column;

    if ( mtfLikeFormatInfo( aNode, aTemplate, aStack, & sFormatInfo, & sFormatLen, aCallBack ) == IDE_SUCCESS )
    {
        if ( sFormatInfo != NULL )
        {
            switch ( sFormatInfo->type )
            {
                case MTC_FORMAT_NORMAL:
                {
                    // search_value = format pattern
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateEqualFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER:
                {
                    // character_length( search_value ) = underCnt
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLengthFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLengthMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_PERCENT:
                {
                    // search_value is null
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateIsNullFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_NORMAL_ONE_PERCENT:
                {
                    // search_value = [head]%[tail]
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateOnePercentFast;                           
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER_PERCENT:
                {
                    // character_length( search_value ) >= underCnt
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLengthFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLengthMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_NORMAL_MANY_PERCENT:
                case MTC_FORMAT_NORMAL_UNDER:
                case MTC_FORMAT_ALL:
                {
                    if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE )
                    {
                        if ( sIndexColumn->language == &mtlAscii )
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculateNormalFast; 
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculateMBNormalFast;
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                    }
                    else
                    {
                        // nothing to do
                    }                    
                    break;
                }
                case MTC_FORMAT_NULL:
                {
                    // Nothing to do.
                    break;
                }
                default:
                {
                    ideLog::log( IDE_ERR_0, 
                                 "sFormatInfo->type : %u\n",
                                 sFormatInfo->type );

                    IDE_ASSERT( 0 );
                    break;
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
        
    return IDE_SUCCESS;
}

IDE_RC mtfNotLikeEstimateBitFast( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt,
                                  mtcCallBack* aCallBack )
{
    mtcLikeFormatInfo * sFormatInfo;
    UShort              sFormatLen;    

    if ( mtfLikeFormatInfo( aNode, aTemplate, aStack, & sFormatInfo, & sFormatLen, aCallBack ) == IDE_SUCCESS )
    {
        if ( sFormatInfo != NULL )
        {
            switch ( sFormatInfo->type )
            {
                case MTC_FORMAT_NORMAL:
                {
                    // search_value = format pattern
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateEqualFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER:
                {
                    // character_length( search_value ) = underCnt
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateLengthFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_PERCENT:
                {
                    // search_value is null
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateIsNullFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_NORMAL_ONE_PERCENT:
                {
                    // search_value = [head]%[tail]
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateOnePercentFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER_PERCENT:
                {
                    // character_length( search_value ) >= underCnt
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateLengthFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_NORMAL_MANY_PERCENT:
                case MTC_FORMAT_NORMAL_UNDER:
                case MTC_FORMAT_ALL:
                {
                    if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE )
                    {                        
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateNormalFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        // nothing to do
                    }
                
                    break;
                }
                case MTC_FORMAT_NULL:
                {
                    // Nothing to do.
                    break;
                }
                default:
                {
                    ideLog::log( IDE_ERR_0, 
                                 "sFormatInfo->type : %u\n",
                                 sFormatInfo->type );

                    IDE_ASSERT( 0 );
                    break;
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
        
    return IDE_SUCCESS;
}

IDE_RC mtfNotLikeEstimateXlobLocatorFast( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt,
                                          mtcCallBack* aCallBack )
{
    mtcColumn         * sIndexColumn;
    mtcLikeFormatInfo * sFormatInfo;
    UShort              sFormatLen;    

    // BUG-40992 FATAL when using _prowid
    // 인자의 경우 mtcStack 의 column 값을 이용하면 된다.
    sIndexColumn     = aStack[1].column;

    if ( mtfLikeFormatInfo( aNode, aTemplate, aStack, & sFormatInfo, & sFormatLen, aCallBack ) == IDE_SUCCESS )
    {   
        if ( sFormatInfo != NULL )
        {
            switch ( sFormatInfo->type )
            {
                case MTC_FORMAT_NORMAL:
                {
                    // search_value = format pattern
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateEqual4XlobLocatorFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER:
                {
                    // character_length( search_value ) = underCnt
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4XlobLocatorFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4XlobLocatorMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_PERCENT:
                {
                    // search_value is null
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateIsNull4XlobLocatorFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_NORMAL_ONE_PERCENT:
                {
                    // search_value = [head]%[tail]
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateOnePercent4XlobLocatorFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER_PERCENT:
                {
                    // character_length( search_value ) >= underCnt
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4XlobLocatorFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4XlobLocatorMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_NORMAL_MANY_PERCENT:
                case MTC_FORMAT_NORMAL_UNDER:
                case MTC_FORMAT_ALL:
                {
                    if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE )
                    {
                        if ( sIndexColumn->language == &mtlAscii )
                        {                        
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculate4XlobLocatorNormalFast;
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculate4XlobLocatorMBNormalFast;
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                    }
                    else
                    {
                        // nothing to do 
                    }
                    
                    break;
                }
                case MTC_FORMAT_NULL:
                {
                    // Nothing to do.
                    break;
                }
                default:
                {
                    ideLog::log( IDE_ERR_0, 
                                 "sFormatInfo->type : %u\n",
                                 sFormatInfo->type );

                    IDE_ASSERT( 0 );
                    break;
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfNotLikeEstimateEcharFast( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt,
                                    mtcCallBack* aCallBack )
{
    mtcColumn         * sIndexColumn;
    mtcLikeFormatInfo * sFormatInfo;
    UShort              sFormatLen;    

    // BUG-40992 FATAL when using _prowid
    // 인자의 경우 mtcStack 의 column 값을 이용하면 된다.
    sIndexColumn     = aStack[1].column;

    if ( mtfLikeFormatInfo( aNode, aTemplate, aStack, & sFormatInfo, & sFormatLen, aCallBack ) == IDE_SUCCESS )
    {
        if ( sFormatInfo != NULL )
        {
            switch ( sFormatInfo->type )
            {
                case MTC_FORMAT_NORMAL:
                {
                    // search_value = format pattern
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateEqual4EcharFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER:
                {
                    // character_length( search_value ) = underCnt
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4EcharFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4EcharMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_PERCENT:
                {
                    // search_value is null
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateIsNull4EcharFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_NORMAL_ONE_PERCENT:
                {
                    // search_value = [head]%[tail]
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateOnePercent4EcharFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER_PERCENT:
                {
                    // character_length( search_value ) >= underCnt
                    if( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4EcharFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4EcharMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_NORMAL_MANY_PERCENT:
                case MTC_FORMAT_NORMAL_UNDER:
                case MTC_FORMAT_ALL:
                {
                    if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE )
                    {                        
                        if ( sIndexColumn->language == &mtlAscii )
                        {                        
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculate4EcharNormalFast;
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculate4EcharMBNormalFast;
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                    }
                    else
                    {
                        // nothing to do
                    }
                    
                    break;
                }
                case MTC_FORMAT_NULL:
                {
                    // Nothing to do.
                    break;
                }
                default:
                {
                    ideLog::log( IDE_ERR_0, 
                                 "sFormatInfo->type : %u\n",
                                 sFormatInfo->type );

                    IDE_ASSERT( 0 );
                    break;
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfNotLikeCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateNormal( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateNormalFast( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateEqualFast( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateEqualFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateIsNullFast( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateIsNotNullFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLengthFast( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateLengthFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateOnePercentFast( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateOnePercentFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
    
IDE_RC mtfNotLikeCalculateMB( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateMB( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateMBNormal( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateMBNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateMBNormalFast( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateMBNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLengthMBFast( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateLengthMBFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4XlobLocator( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4XlobLocator( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4XlobLocatorNormal( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4XlobLocatorNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4XlobLocatorNormalFast( mtcNode*     aNode,
                                                  mtcStack*    aStack,
                                                  SInt         aRemain,
                                                  void*        aInfo,
                                                  mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4XlobLocatorNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateEqual4XlobLocatorFast( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateEqual4XlobLocatorFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateIsNull4XlobLocatorFast( mtcNode*     aNode,
                                                  mtcStack*    aStack,
                                                  SInt         aRemain,
                                                  void*        aInfo,
                                                  mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateIsNotNull4XlobLocatorFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLength4XlobLocatorFast( mtcNode*     aNode,
                                                  mtcStack*    aStack,
                                                  SInt         aRemain,
                                                  void*        aInfo,
                                                  mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateLength4XlobLocatorFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateOnePercent4XlobLocatorFast( mtcNode*     aNode,
                                                      mtcStack*    aStack,
                                                      SInt         aRemain,
                                                      void*        aInfo,
                                                      mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateOnePercent4XlobLocatorFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4XlobLocatorMB( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4XlobLocatorMB( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4XlobLocatorMBNormal( mtcNode*     aNode,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                void*        aInfo,
                                                mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4XlobLocatorMBNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4XlobLocatorMBNormalFast( mtcNode*     aNode,
                                                    mtcStack*    aStack,
                                                    SInt         aRemain,
                                                    void*        aInfo,
                                                    mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4XlobLocatorMBNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLength4XlobLocatorMBFast( mtcNode*     aNode,
                                                    mtcStack*    aStack,
                                                    SInt         aRemain,
                                                    void*        aInfo,
                                                    mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateLength4XlobLocatorMBFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
    
IDE_RC mtfNotLikeCalculate4Echar( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4Echar( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4EcharNormal( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4EcharNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4EcharNormalFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4EcharNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateEqual4EcharFast( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateEqual4EcharFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateIsNull4EcharFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateIsNotNull4EcharFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLength4EcharFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateLength4EcharFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateOnePercent4EcharFast( mtcNode*     aNode,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                void*        aInfo,
                                                mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateOnePercent4EcharFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4EcharMB( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4EcharMB( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4EcharMBNormal( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4EcharMBNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4EcharMBNormalFast( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculate4EcharMBNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLength4EcharMBFast( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate )
{
    IDE_TEST( mtfLikeCalculateLength4EcharMBFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4ClobValue( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculate4ClobValue( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4ClobValueMB( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculate4ClobValueMB( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4ClobValueNormal( mtcNode     * aNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * aInfo,
                                            mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculate4ClobValueNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4ClobValueMBNormal( mtcNode     * aNode,
                                              mtcStack    * aStack,
                                              SInt          aRemain,
                                              void        * aInfo,
                                              mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculate4ClobValueMBNormal( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeEstimateClobValueFast( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          /* aRemain */,
                                        mtcCallBack * aCallBack )
{
    mtcColumn         * sIndexColumn;
    mtcLikeFormatInfo * sFormatInfo;
    UShort              sFormatLen;

    // BUG-40992 FATAL when using _prowid
    // 인자의 경우 mtcStack 의 column 값을 이용하면 된다.
    sIndexColumn     = aStack[1].column;

    if ( mtfLikeFormatInfo( aNode, aTemplate, aStack, & sFormatInfo, & sFormatLen, aCallBack ) == IDE_SUCCESS )
    {
        if ( sFormatInfo != NULL )
        {
            switch ( sFormatInfo->type )
            {
                case MTC_FORMAT_NORMAL:
                {
                    // search_value = format pattern
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateEqual4ClobValueFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER:
                {
                    // character_length( search_value ) = underCnt
                    if ( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4ClobValueFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4ClobValueMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_PERCENT:
                {
                    // search_value is null
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateIsNull4ClobValueFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_NORMAL_ONE_PERCENT:
                {
                    // search_value = [head]%[tail]
                    aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                        mtfNotLikeCalculateOnePercent4ClobValueFast;
                    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                        sFormatInfo;
                    break;
                }
                case MTC_FORMAT_UNDER_PERCENT:
                {
                    // character_length( search_value ) >= underCnt
                    if ( sIndexColumn->language == &mtlAscii )
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4ClobValueFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    else
                    {
                        aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                            mtfNotLikeCalculateLength4ClobValueMBFast;
                        aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                            sFormatInfo;
                    }
                    break;
                }
                case MTC_FORMAT_NORMAL_MANY_PERCENT:
                case MTC_FORMAT_NORMAL_UNDER:
                case MTC_FORMAT_ALL:
                {
                    if ( MTU_LIKE_OP_USE_MODULE == MTU_LIKE_USE_NEW_MODULE )
                    {
                        if ( sIndexColumn->language == &mtlAscii )
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculate4ClobValueNormalFast;
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                        else
                        {
                            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                                mtfNotLikeCalculate4ClobValueMBNormalFast;
                            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                sFormatInfo;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;
                }
                case MTC_FORMAT_NULL:
                {
                    /* Nothing to do */
                    break;
                }
                default:
                {
                    ideLog::log( IDE_ERR_0,
                                 "sFormatInfo->type : %u\n",
                                 sFormatInfo->type );

                    IDE_ASSERT( 0 );
                    /* break; */
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;
}

IDE_RC mtfNotLikeCalculate4ClobValueNormalFast( mtcNode     * aNode,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                void        * aInfo,
                                                mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculate4ClobValueNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateEqual4ClobValueFast( mtcNode     * aNode,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               void        * aInfo,
                                               mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculateEqual4ClobValueFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateIsNull4ClobValueFast( mtcNode     * aNode,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                void        * aInfo,
                                                mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculateIsNotNull4ClobValueFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLength4ClobValueFast( mtcNode     * aNode,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                void        * aInfo,
                                                mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculateLength4ClobValueFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateOnePercent4ClobValueFast( mtcNode     * aNode,
                                                    mtcStack    * aStack,
                                                    SInt          aRemain,
                                                    void        * aInfo,
                                                    mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculateOnePercent4ClobValueFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculate4ClobValueMBNormalFast( mtcNode     * aNode,
                                                  mtcStack    * aStack,
                                                  SInt          aRemain,
                                                  void        * aInfo,
                                                  mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculate4ClobValueMBNormalFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotLikeCalculateLength4ClobValueMBFast( mtcNode     * aNode,
                                                  mtcStack    * aStack,
                                                  SInt          aRemain,
                                                  void        * aInfo,
                                                  mtcTemplate * aTemplate )
{
    IDE_TEST( mtfLikeCalculateLength4ClobValueMBFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType *)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType *)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
