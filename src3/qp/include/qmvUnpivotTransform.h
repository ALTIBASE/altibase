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
 * $Id: qmvUnpivotTransform.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QMV_UNPIVOT_TRANSFORM_H_
#define _Q_QMV_UNPIVOT_TRANSFORM_H_ 1

#include <qc.h>
#include <qmsParseTree.h>

class qmvUnpivotTransform
{
public:

    static IDE_RC doTransform( qcStatement * aStatement,
                               qmsSFWGH    * aSFWGH,
                               qmsTableRef * aTableRef );
 
    static IDE_RC expandUnpivotTarget( qcStatement * aStatement,
                                       qmsSFWGH    * aSFWGH );

private:

    static IDE_RC checkUnpivotSyntax( qmsUnpivot * aUnpivot );

    static IDE_RC makeViewForUnpivot( qcStatement * aStatement,
                                      qmsTableRef * aTableRef );

    static IDE_RC makeArgsForDecodeOffset( qcStatement         * aStatement,
                                           qtcNode             * aNode,
                                           qmsUnpivotInColInfo * aUnpivotIn,
                                           idBool                aIsValueColumn,
                                           ULong                 aOrder );

    static IDE_RC makeUnpivotTarget( qcStatement * aStatement,
                                     qmsSFWGH    * aSFWGH,
                                     qmsTableRef * aTableRef );

    static IDE_RC makeLoopClause( qcStatement         * aStatement,
                                  qmsParseTree        * aParseTree,
                                  qmsUnpivotInColInfo * aInColInfo );

    static IDE_RC makeNotNullPredicate( qcStatement * aStatement,
                                        qmsSFWGH    * aSFWGH,
                                        qmsTableRef * aTableRef );

};

#endif /* _Q_QMV_UNPIVOT_TRANSFORM_H_ */
