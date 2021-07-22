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
 
/******************************************************************************
 * $Id$
 *
 * Description : ORDER BY Elimination Transformation
 *
 * 용어 설명 :
 *
 *****************************************************************************/

#ifndef _Q_QMO_OBYE_TRANSFORM_H_
#define _Q_QMO_OBYE_TRANSFORM_H_ 1

#include <ide.h>
#include <qmsParseTree.h>

class qmoOBYETransform
{
public:

    static IDE_RC doTransform( qcStatement  * aStatement,
                               qmsParseTree * aParseTree );

private:
    static IDE_RC doTransformInternal( qcStatement * aStatement,
                                       qmsQuerySet * aQuerySet,
                                       qmsLimit    * aLimit,
                                       idBool        aIsSubQPred );

    static IDE_RC doTransform4FromTree( qcStatement * aStatement,
                                        qmsQuerySet * aQuerySet, 
                                        qmsFrom     * aFrom,
                                        idBool        aIsTransMode2 );

    static IDE_RC doTransform4Predicate( qcStatement * aStatement,
                                         qtcNode     * aNode );

    static idBool canTranMode2ForInlineView( qmsParseTree * aParseTree );
};

#endif  /* _Q_QMO_OBYE_TRANSFORM_H_ */
