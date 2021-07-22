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
 * $Id: qmvOrderBy.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QMV_ORDER_BY_H_
#define _Q_QMV_ORDER_BY_H_ 1

#include <qc.h>
#include <qtcDef.h>
#include <qmsParseTree.h>

class qmvOrderBy
{
public:
	// validate
	static IDE_RC validate(
        qcStatement     * aStatement);

    static IDE_RC transposePosValIntoTargetPtr(
        qmsSortColumns  * aSortColumn,
        qmsTarget       * aTarget );

    // PROJ-1413
    static IDE_RC disconnectConstantNode(
        qmsParseTree    * aParseTree );
    
private:
    static IDE_RC validateSortWithGroup(
        qcStatement     * aStatement);

    static IDE_RC validateSortWithoutGroup(
        qcStatement     * aStatement);

    static IDE_RC validateSortWithSet(
        qcStatement     * aStatement);

    /* PROJ-1715 */
    static IDE_RC searchSiblingColumn( qtcNode  * aExpression,
                                       qtcNode ** aColumn,
                                       idBool   * aFind );

};

#endif  // _Q_QMV_ORDER_BY_H_
