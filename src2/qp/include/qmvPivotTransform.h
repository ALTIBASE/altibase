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
 * $Id: qmvPivotTransform.h 89218 2020-11-12 01:36:24Z donovan.seo $
 **********************************************************************/

#ifndef _Q_QMV_PIVOT_TRANSFORM_H_
#define _Q_QMV_PIVOT_TRANSFORM_H_ 1

#include <qc.h>
#include <qmsParseTree.h>

class qmvPivotTransform
{
public:

    static IDE_RC doTransform( qcStatement * aStatement,
                               qmsSFWGH    * aSFWGH,
                               qmsTableRef * aTableRef );

    static IDE_RC expandPivotDummy( qcStatement   * aStatement,
                                    qmsSFWGH      * aSFWGH );
private:

    static IDE_RC createFirstAlias( qcStatement     * aStatement,
                                    qmsNamePosition * aAlias,
                                    UInt              aOrder );
    
    static IDE_RC createSecondAlias( qcStatement     * aStatement,
                                     qmsNamePosition * aAlias,
                                     qmsPivotAggr    * aAggr,
                                     qtcNode         * aValueNode );

    static IDE_RC checkPivotSyntax( qcStatement * aStatement,
                                    qmsPivot    * aPivot );

    static IDE_RC createPivotParseTree( qcStatement * aStatement,
                                        qmsTableRef * aTableRef );

    static IDE_RC searchPivotAggrColumn( qcStatement * aStatement,
                                         mtcNode     * aPrevMtc,
                                         mtcNode     * aMtcNode );

    static IDE_RC createDummyTarget( qcStatement * aStatement,
                                     qmsQuerySet * aQuerySet,
                                     qmsTableRef * aTableRef );

    static IDE_RC createPivotFirstTarget( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          qmsTableRef * aTableRef );

    static IDE_RC createPivotSecondTarget( qcStatement * aStatement,
                                           qmsSFWGH    * aSFWGH,
                                           qmsTableRef * aTableRef );

    static IDE_RC checkPivotTarget( qmsSFWGH    * aSFWGH );

    static IDE_RC processPivot( qcStatement * aStatement,
                                qmsFrom     * aFrom );
};

#endif /* _Q_QMV_PIVOT_TRANSFORM_H_ */
