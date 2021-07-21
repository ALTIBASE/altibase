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
 * $Id: qmg.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     모든 Graph 관련 Header를 통합함.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_H_
#define _O_QMG_H_ 1

#include <qmgDef.h>
#include <qmgSelection.h>
#include <qmgProjection.h>
#include <qmgDistinction.h>
#include <qmgGrouping.h>
#include <qmgSorting.h>
#include <qmgJoin.h>
#include <qmgLeftOuter.h>
#include <qmgFullOuter.h>
#include <qmgSet.h>
#include <qmgHierarchy.h>
#include <qmgDnf.h>
#include <qmgPartition.h> // PROJ-1502 PARTITIONED DISK TABLE
#include <qmgCounting.h>  // PROJ-1405 ROWNUM
#include <qmgWindowing.h>
#include <qmgInsert.h>
#include <qmgMultiInsert.h>
#include <qmgUpdate.h>
#include <qmgDelete.h>
#include <qmgMove.h>
#include <qmgMerge.h>
#include <qmgShardSelect.h>
#include <qmgShardDML.h>
#include <qmgShardInsert.h>

#endif /* _O_QMG_H_ */
