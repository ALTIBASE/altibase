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
 * $Id$
 **********************************************************************/

#ifndef _Q_QMV_TABLE_FUNC_TRANSFORM_H_
#define _Q_QMV_TABLE_FUNC_TRANSFORM_H_ 1

#include <qc.h>
#include <qmsParseTree.h>

class qmvTableFuncTransform
{
public:

    static IDE_RC doTransform( qcStatement * aStatement,
                               qmsTableRef * aTableRef );

    static IDE_RC expandTarget( qcStatement * aStatement,
                                qmsSFWGH    * aSFWGH );

private:

    static IDE_RC createParseTree( qcStatement * aStatement,
                                   qmsTableRef * aTableRef );
};

#endif /* _Q_QMV_PIVOT_TRANSFORM_H_ */
