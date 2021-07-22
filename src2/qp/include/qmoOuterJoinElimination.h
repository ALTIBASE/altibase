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
 * $Id: qmoOuterJoinElimination.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_OUTER_JOIN_ELIMINATION_H_
#define _O_QMO_OUTER_JOIN_ELIMINATION_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmoDependency.h>

class qmoOuterJoinElimination
{
public:

    static IDE_RC doTransform( qcStatement  * aStatement,
                               qmsQuerySet  * aQuerySet,
                               idBool       * aChanged );

private:

    static IDE_RC doTransformQuerySet( qcStatement * aStatement,
                                       qmsQuerySet * aQuerySet,
                                       idBool      * aChanged );

    static IDE_RC doTransformFrom( qcStatement * aStatement,
                                   qmsSFWGH    * aSFWGH,
                                   qmsFrom     * aFrom,
                                   qcDepInfo   * aFindDependencies,
                                   idBool      * aChanged );

    static IDE_RC   addWhereDep( mtcNode     * aNode,
                                 qcDepInfo   * aFindDependencies );

    static IDE_RC addOnConditionDep( qmsFrom     * aFrom,
                                     qtcNode     * aNode,
                                     qcDepInfo   * aFindDependencies );

    static void removeDep( mtcNode     * aNode,
                           qcDepInfo   * aFindDependencies );
};

#endif
