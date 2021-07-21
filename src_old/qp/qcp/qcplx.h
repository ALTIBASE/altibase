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
 * $Id: qcplx.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _QCPLX_H_
#define _QCPLX_H_ 1

#include <qc.h>
#include <iduMemory.h>
#include <mtcDef.h>

class qcplLexer;

typedef struct qcplx
{
    idvSQL         * mStatistics; /* PROJ-2446 ONE SOURCE */
    qcplLexer      * mLexer;
    qcSession      * mSession;
    qcStatement    * mStatement;
    qcNamePosList  * mNcharList;
} qcplx;

#endif /* _QCPLX_H_ */
