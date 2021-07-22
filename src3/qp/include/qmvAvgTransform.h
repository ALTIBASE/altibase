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
 * $Id: qmvAvgTransform.h 82075 2018-01-17 06:39:52Z jina.kim $
 * PROJ-2361
 ***********************************************************************/

#ifndef _Q_QMV_AVGTRANSFORM_H_
#define _Q_QMV_AVGTRANSFORM_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qtcDef.h>

class qmvAvgTransform
{
public:
    static IDE_RC
        doTransform( qcStatement * aStatement,
                     qmsQuerySet * aQuerySet,
                     qmsSFWGH    * aSFWGH );

private:
    static IDE_RC
        transformAvg2SumDivCount( qcStatement  * aStatement,
                                  qmsQuerySet  * aQuerySet,
                                  qtcNode      * aNode,
                                  qtcNode     ** aResultNode,
                                  idBool       * aNeedEstimate );

    static IDE_RC
        checkTransform( qcStatement * aStatement,
                        qmsSFWGH    * aSFWGH,
                        qtcNode     * aAvgNode,
                        idBool      * aCheckTransform );

    static IDE_RC
        isExistViewColumn( qcStatement * aStatement,
                           qtcNode     * aAvgNode,
                           idBool      * aIsExistViewColumn );

    static IDE_RC
        isExistSameArgSum( qcStatement * aStatement,
                           qtcNode     * aNode,
                           qtcNode     * aAvgNode,
                           idBool      * aIsExist );
};
#endif /* _Q_QMV_AVGTRANSFORM_H_ */
