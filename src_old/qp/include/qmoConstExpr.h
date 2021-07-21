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
 * $Id: qmoConstExpr.h 23857 2008-03-19 02:36:53Z sungminee $
 *
 * Description :
 *     PROJ-1413 constant exprssion의 상수 변환을 위한 자료 구조 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_CONST_EXPR_H_
#define _O_QMO_CONST_EXPR_H_ 1

#include <qc.h>
#include <qmsParseTree.h>

class qmoConstExpr
{
public:

    //------------------------------------------
    // constant exprssion의 상수 변환
    //------------------------------------------
    
    static IDE_RC  processConstExpr( qcStatement  * aStatement,
                                     qmsSFWGH     * aSFWGH );

    static IDE_RC  processConstExprForOrderBy( qcStatement  * aStatement,
                                               qmsParseTree * aParseTree );
    
private:

    static IDE_RC  processNode( qtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );
};

#endif /* _O_QMO_CONST_EXPR_H_ */
