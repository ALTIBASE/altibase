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
 * $Id: stfBasic.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체의 기본 속성 함수
 * 상세 구현을 필요로 하는 함수는 stdPrimitive.cpp를 실행한다.
 **********************************************************************/

#ifndef _O_STF_BASIC_H_
#define _O_STF_BASIC_H_        1

#include <idTypes.h>
#include <mtcDef.h>
#include <stdTypes.h>
#include <qtcDef.h>

class stfBasic
{
public:
/* Basic Functions ****************************************************/
    static IDE_RC dimension(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC geometryType(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC asText(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    /* PROJ-2422 SRID */
    static IDE_RC asEWKT( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

    static IDE_RC asBinary(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    /* PROJ-2422 SRID */
    static IDE_RC asEWKB( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

    static IDE_RC boundary(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC envelope(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC isEmpty(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC isSimple(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );
    // BUG-22924
    static IDE_RC isValid(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    idBool       aCheckSize, /* BUG-48051 */
                    mtcTemplate* aTemplate);

    // BUG-33576
    static IDE_RC isValidHeader(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate);

    /* PROJ-2422 SRID */
    // stm에 존재하는 SRS의 ID를 가져온다.
    static IDE_RC getSRID( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

    /* PROJ-2422 SRID */
    static SInt getSRID( stdGeometryHeader  * aObj );
    
    /* PROJ-2422 SRID */
    static IDE_RC setSRID( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );
    
    /* PROJ-2422 SRID */
    static IDE_RC setSRID( stdGeometryHeader  * aObj,
                           UInt                 aFence,
                           SInt                 aSRID );

    /* PROJ-2422 SRID */
    static IDE_RC setSRID( stdGeometryHeader  * aDstObj,
                           UInt                 aDstFence,
                           stdGeometryType    * aSrcObj );
    
    /* PROJ-2422 SRID */
    static IDE_RC setSRID( stdGeometryHeader  * aDstObj,
                           UInt                 aDstFence,
                           stdGeometryHeader  * aSrcObj );

    /* BUG-47816 ST_Transform 함수 지원 */
    static IDE_RC getProj4TextFromSRID ( qcStatement      * aStatement, 
                                         mtdIntegerType   * aSRID, 
                                         mtdCharType     ** aProj4Text );
    
/**************************************************************************/
    static IDE_RC getDimension(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet );

    static IDE_RC getText(
                    stdGeometryHeader*  aObj,              
                    UChar*              aBuf,
                    UInt                aMaxSize,
                    UInt*               aOffset,
                    IDE_RC*             aReturn);
                    
    /* PROJ-2422 SRID */
    static IDE_RC getEWKT( stdGeometryHeader*  aObj,              
                           UChar*              aBuf,
                           UInt                aMaxSize,
                           UInt*               aOffset,
                           IDE_RC*             aReturn );
                    
    static IDE_RC getBinary(
                    stdGeometryHeader*  aObj,              
                    UChar*              aBuf,
                    UInt                aMaxSize,
                    UInt*               aOffset );  // Fix BUG-15834

    /* PROJ-2422 SRID */
    static IDE_RC getEWKB( stdGeometryHeader*  aObj,              
                           UChar*              aBuf,
                           UInt                aMaxSize,
                           UInt*               aOffset );

    static IDE_RC getBoundary(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet,
                    UInt                aFence );  // Fix Bug-25110
    
    static IDE_RC getEnvelope(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet );

    static IDE_RC testSimple(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet );

    /* BUG-45645 ST_Reverse, ST_MakeEnvelope 함수 지원 */
    static IDE_RC getRectangle( mtcTemplate       * aTemplate,
                                mtdDoubleType       aX1,
                                mtdDoubleType       aY1,
                                mtdDoubleType       aX2,
                                mtdDoubleType       aY2,
                                stdGeometryHeader * aRet );

    static IDE_RC changeExtTypeToBasic( stdGeometryHeader  * aObj );    
};

#endif /* _O_STF_BASIC_H_ */


