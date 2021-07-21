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
 * $Id: qmxSessionCache.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QMX_SESSION_CACHE_H_
#define _O_QMX_SESSION_CACHE_H_ 1

#include <smiDef.h>

typedef struct qcSessionSeqCaches
{
    smOID                 sequenceOID;
    smSCN                 sequenceSCN; /* BUG-45315 */
    SLong                 currVal;

    qcSessionSeqCaches  * next;
} qcSessionSeqCaches;

class qmxSessionCache
{
public:
    IDE_RC clearSequence( );

    qcSessionSeqCaches   * mSequences_;
};

#endif /* _O_QMX_SESSION_CACHE_H_ */
