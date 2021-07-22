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
 *
 * Description :
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_INNER_JOIN_PUSH_DOWN_H_
#define _O_QMO_INNER_JOIN_PUSH_DOWN_H_ 1

#include <qc.h>
#include <qtc.h>
#include <qmsParseTree.h>

class qmoInnerJoinPushDown
{
public:

    static IDE_RC doTransform( qcStatement  * aStatement,
                               qmsQuerySet  * aQuerySet );

private:
    static IDE_RC pushInnerJoin( qcStatement * aStatement,
                                 qmsQuerySet * aQuerySet );
};

#endif
