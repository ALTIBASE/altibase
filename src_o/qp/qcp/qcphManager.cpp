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
 * $Id: qcphManager.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <qcuError.h>
#include <qcphManager.h>
#include "qcphx.h"

#if defined(BISON_POSTFIX_HPP)
#include "qcphy.hpp"
#else /* BISON_POSTFIX_CPP_H */
#include "qcphy.cpp.h"
#endif

#include "qcphl.h"

extern int qcphparse(void *param);

void qcphManager::parseIt (
qcStatement    * aStatement,
SInt             aOffset,
SChar          * aBuffer,
SInt             aBufferLength )
{

    qcphLexer      s_qcphLexer(aBuffer, aBufferLength);
    qcphx          s_qcphx;
    qcOffset     * sOffset;

    s_qcphx.mLexer              = &s_qcphLexer;
    s_qcphx.mLexer->mStatement  = aStatement;
    s_qcphx.mLexer->mTextOffset = aOffset;
    s_qcphx.mStatement          = aStatement;
    s_qcphx.mStmtText           = aBuffer;
    s_qcphx.mTextOffset         = aOffset;

    // BUG-43566 hint의 시작위치
    s_qcphx.mPosition.stmtText  = aBuffer;
    s_qcphx.mPosition.offset    = aOffset;
    s_qcphx.mPosition.size      = QC_POS_EMPTY_SIZE;

    if (qcphparse(&s_qcphx) != IDE_SUCCESS)
    {
        // BUG-43524 hint 구문이 오류가 났을때 알수 있으면 좋겠습니다.
        aStatement->myPlan->mPlanFlag  &= ~QC_PLAN_HINT_PARSE_MASK;
        aStatement->myPlan->mPlanFlag  |= QC_PLAN_HINT_PARSE_FAIL;

        // BUG-43566 hint 구문이 오류가 났을때 위치를 알수 있으면 좋겠습니다.
        sOffset = aStatement->myPlan->mHintOffset;

        if ( (QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcOffset),
                                             (void**)&aStatement->myPlan->mHintOffset )) == IDE_SUCCESS )
        {
            aStatement->myPlan->mHintOffset->mOffset = s_qcphx.mPosition.offset;
            aStatement->myPlan->mHintOffset->mLen    = s_qcphx.mPosition.size;
            aStatement->myPlan->mHintOffset->mNext   = sOffset;
        }
        else
        {
            aStatement->myPlan->mHintOffset = sOffset;
        }

        aStatement->myPlan->hints = NULL;
    }
    else
    {
        aStatement->myPlan->hints = s_qcphx.mHints;
    }
}
