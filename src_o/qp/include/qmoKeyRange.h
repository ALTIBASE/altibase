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
 * $Id: qmoKeyRange.h 86786 2020-02-27 08:04:12Z donovan.seo $
 *
 * Description :
 *    Key Range 생성기
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_KEY_RANGE_H_
#define _O_QMO_KEY_RANGE_H_ 1

#include <qc.h>
#include <qtcDef.h>

class qmoKeyRange
{
public:

    // Key Range의 크기를 측정한다.
    static IDE_RC     estimateKeyRange( qcTemplate  * aTemplate,
                                        qtcNode     * aNode,
                                        UInt        * aRangeSize );
    
    // Key Range를 생성한다.
    static IDE_RC     makeKeyRange( qcTemplate  * aTemplate,
                                    qtcNode     * aNode,
                                    UInt          aKeyCount,
                                    mtcColumn   * aKeyColumn,
                                    UInt        * aKeyColsFlag,
                                    UInt          aCompareType,
                                    smiRange    * aRangeArea,
                                    UInt          aRangeAreaSize,
                                    smiRange   ** aRange,
                                    qtcNode    ** aFilter );

    // Key Filter를 생성한다.
    static IDE_RC     makeKeyFilter( qcTemplate  * aTemplate,
                                     qtcNode     * aNode,
                                     UInt          aKeyCount,
                                     mtcColumn   * aKeyColumn,
                                     UInt        * aKeyColsFlag,
                                     UInt          aCompareType,
                                     smiRange    * aRangeArea,
                                     UInt          aRangeAreaSize,
                                     smiRange   ** aRange,
                                     qtcNode    ** aFilter );

    // Indexable MIN, MIX 적용을 위한 Not Null Range 생성
    static IDE_RC     makeNotNullRange( void               * aPredicate,
                                        mtcColumn          * aKeyColumn,
                                        smiRange           * aRange );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC     makePartKeyRange(
        qcTemplate          * aTemplate,
        qtcNode             * aNode,
        UInt                  aPartKeyCount,
        mtcColumn           * aPartKeyColumns,
        UInt                * aPartKeyColsFlag,
        UInt                  aCompareType,
        smiRange            * aRangeArea,
        UInt                  aRangeAreaSize,
        smiRange           ** aRange );
    
private:

    // Key Range의 크기를 측정한다.
    static IDE_RC     estimateRange( qcTemplate  * aTemplate,
                                     qtcNode     * aNode,
                                     UInt        * aRangeCount,
                                     UInt        * aRangeSize );    

    // Key Range를  생성한다.
    static IDE_RC     makeRange( qcTemplate  * aTemplate,
                                 qtcNode     * aNode,
                                 UInt          aKeyCount,
                                 mtcColumn   * aKeyColumn,
                                 UInt        * aKeyColsFlag,
                                 idBool        aIsKeyRange,
                                 UInt          aCompareType,
                                 smiRange    * aRangeArea,
                                 UInt          aRangeAreaSize,
                                 smiRange   ** aRange,
                                 qtcNode    ** aFilter );

    // in subquer or subquery keyRange 인 경우
    // subquery를 수행한다.
    static IDE_RC     calculateSubqueryInRangeNode(
        qcTemplate   * aTemplate,
        qtcNode      * aNode,
        idBool       * aIsExistsValue );

    // 하나의 index column으로 추출된 노드들의 range 생성
    static IDE_RC     makeRange4AColumn( qcTemplate    * aTemplate,
                                         qtcNode       * aNode,
                                         mtcColumn     * aKeyColumn,
                                         UInt            aColumnIdx,
                                         UInt          * aKeyColsFlag,
                                         UInt          * aOffset,
                                         UChar         * aRangeStartPtr,
                                         UInt            aRangeAreaSize,
                                         UInt            aCompareType,
                                         smiRange     ** aRange );

    // range count를 얻는다.
    static UInt       getRangeCount( smiRange * aRange );   

    // AND merge를 위한 range count를 얻는다.
    static UInt       getAndRangeCount( smiRange * aRange1,
                                        smiRange * aRange2 );

    //
    static IDE_RC     copyRange( smiRange  * aRangeOrg,
                                 UInt      * aOffset,
                                 UChar     * aRangeStartPtr,
                                 UInt        aRangeAreaSize,
                                 smiRange ** aRangeNew );
};


#endif /*_O_QMO_KEY_RANGE_H_ */
