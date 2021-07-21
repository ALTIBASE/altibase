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
 * $Id: qcphx.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _QCPLX_H_
#define _QCPLX_H_ 1

#include <iduMemory.h>
#include <qc.h>
#include <qmsParseTree.h>

class qcphLexer;

typedef struct qcphx
{
    qcphLexer*      mLexer;
    qcStatement*    mStatement;
    SChar*          mStmtText;
    SInt            mTextOffset;
    UInt            mLanguage;
    qmsHints*       mHints;      // OUT
    qcNamePosition  mPosition;
} qcphx;

#endif /* _QCPLX_H_ */
