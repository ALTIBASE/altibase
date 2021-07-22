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
 **********************************************************************/

#ifndef _Q_QSX_TEMPLATE_CACHE_H_
#define _Q_QSX_TEMPLATE_CACHE_H_ 1

#include <qc.h>

struct qsxProcInfo;

class qsxTemplateCache
{
public:
    IDE_RC build( qcStatement * aStatement,
                  qcTemplate  * aTemplate );

    IDE_RC destroy();

    IDE_RC get( qcTemplate  ** aTemplate,
                iduListNode ** aCacheNode );

    IDE_RC put( iduListNode * aCacheNode );

private:
    iduMemory   * mCacheMemory;
    iduList       mCache;
    iduLatch      mLock;
    UInt          mCount;
};

#endif
