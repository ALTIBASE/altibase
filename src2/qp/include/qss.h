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
 * $Id: qss.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QSS_H_
#define _Q_QSS_H_  1

#include <iduMemory.h>
#include <idsAltiWrap.h>
#include <qc.h>
#include <qcg.h>

/* isql에서 받을 수 있는 text max length */
#define QSS_STMT_MAX_LENGTH (65536)

class qss
{
public:
    /* decrypt a encrypted text and
       set plain text to qcSharedPlan->stmtText */
    static IDE_RC doDecryption( qcStatement    * aStatement,
                                qcNamePosition   aBody );

private:
    /* Set texts for qcSharedPlan */
    static IDE_RC setStmtTexts( qcStatement    * aStatement,
                                SChar          * aPlainText,
                                SInt             aPlainTextLen );
};

#endif /* _Q_QSS_H_ */
