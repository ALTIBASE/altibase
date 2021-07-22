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
 * $Id: qmoRownumPredToLimit.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_ROWNUM_PRED_TO_LIMIT_H_
#define _O_QMO_ROWNUM_PRED_TO_LIMIT_H_ 1

#include <qc.h>
#include <qmsParseTree.h>

class qmoRownumPredToLimit
{
public:

    static IDE_RC rownumPredToLimitTransform( qcStatement   * aStatement,
                                              qmsQuerySet   * aQuerySet,
                                              qmsTableRef   * aViewTableRef,
                                              qmoPredicate ** aPredicate );

private:

    static idBool isRownumPredToLimit( qcStatement  * aStatement,
                                       qmsTableRef  * aViewTableRef,
                                       qmoPredicate * aPredicate,
                                       UShort       * aPredPosition );

    static IDE_RC doRownumPredToLimit( qcStatement   * aStatement,
                                       qmsTableRef   * aViewTableRef,
                                       qmoPredicate ** aPredicate,
                                       UShort          aPredPosition );

    static IDE_RC makeLimit( qcStatement  * aStatement,
                             qmsTableRef  * aViewTableRef,
                             qtcNode      * aNode,
                             idBool       * aChanged );

    static idBool isViewRownumToLimit( qcStatement  * aStatement,
                                       qmsQuerySet  * aQuerySet,
                                       qmsTableRef  * aViewTableRef );

    static IDE_RC doViewRownumToLimit( qcStatement  * aStatement,
                                       qmsQuerySet  * aQuerySet,
                                       qmsTableRef  * aViewTableRef );
};

#endif
