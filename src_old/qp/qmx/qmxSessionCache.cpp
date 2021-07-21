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
 * $Id: qmxSessionCache.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <qmxSessionCache.h>

IDE_RC qmxSessionCache::clearSequence( )
{
#define IDE_FN "void qmxSessionCache::clearSequence"

    qcSessionSeqCaches   * sNode;
    qcSessionSeqCaches   * sNext;

    for (sNode = mSequences_;
         sNode != NULL;
         sNode = sNext)
    {
        sNext = sNode->next;

        IDE_TEST(iduMemMgr::free(sNode)
             != IDE_SUCCESS);
        sNode = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}
