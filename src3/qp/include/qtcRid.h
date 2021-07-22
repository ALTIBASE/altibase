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
 * $Id: qtcRid.h 48872 2011-10-06 01:58:27Z kwsong $
 **********************************************************************/

#ifndef _O_QTCRID_H_
#define _O_QTCRID_H_ 1

#include <qtc.h>

/*
 * ------------------------------------------------------------------
 * PROJ-1789 PROWID
 *
 * qtc 중 RID 관련 처리는 qtcRid.cpp, qtcRid.h 에 따로 모았다.
 * ------------------------------------------------------------------
 */


extern mtfModule  gQtcRidModule;
extern mtcColumn  gQtcRidColumn;
extern mtcExecute gQtcRidExecute;

IDE_RC qtcRidMakeColumn(qcStatement*    aStatement,
                        qtcNode**       aNode,
                        qcNamePosition* aUserPosition,
                        qcNamePosition* aTablePosition,
                        qcNamePosition* aColumnPosition);

#define QTC_IS_RID_COLUMN(aNode) \
    (((aNode)->node.module != &gQtcRidModule) ? ID_FALSE : ID_TRUE)

#endif /* _O_QTCRID_H_ */

