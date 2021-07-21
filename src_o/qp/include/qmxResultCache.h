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
 * $Id: qmxResultCache.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QMX_RESULT_CACHE_H_
#define _Q_QMX_RESULT_CACHE_H_ 1

#include <qc.h>
#include <qci.h>
#include <qmc.h>
#include <mtc.h>
#include <qmnDef.h>

// RESULT Cache.dataFlag
#define QMX_RESULT_CACHE_INIT_DONE_MASK   (0x00000001)
#define QMX_RESULT_CACHE_INIT_DONE_FALSE  (0x00000000)
#define QMX_RESULT_CACHE_INIT_DONE_TRUE   (0x00000001)

// RESULT Cache.dataFlag
#define QMX_RESULT_CACHE_STORED_MASK   (0x00000002)
#define QMX_RESULT_CACHE_STORED_FALSE  (0x00000000)
#define QMX_RESULT_CACHE_STORED_TRUE   (0x00000002)

// RESULT Cache.dataFlag
#define QMX_RESULT_CACHE_USE_PRESERVED_ORDER_MASK  (0x00000004)
#define QMX_RESULT_CACHE_USE_PRESERVED_ORDER_FALSE (0x00000000)
#define QMX_RESULT_CACHE_USE_PRESERVED_ORDER_TRUE  (0x00000004)

class qmxResultCache
{
private:
    static void checkResultCacheCommitMode( qcStatement * aStatement );

public:
    static void setUpResultCache( qcStatement * aStatement );

    static IDE_RC initResultCache( qcTemplate      * aTemplate,
                                   qcComponentInfo * aInfo,
                                   qmndResultCache * aResultData );

    static void allocResultCacheData( qcTemplate    * aTemplate,
                                      iduVarMemList * aMemory );

    static void checkResultCacheMax( qcTemplate * aTemplate );

    static void destroyResultCache( qcTemplate * aTemplate );

    static IDE_RC checkExecuteMemoryMax( qcTemplate * aTemplate,
                                         UInt         aMemoryIdx );
};

#endif /* _Q_QMX_RESULT_CACHE_H_ */

