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
 

/*********************************************************************** */
/* $Id: qdq.h 91512 2021-08-21 07:50:50Z emlee $ */
/**********************************************************************/
#ifndef _O_QDQ_H_
#define _O_QDQ_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

/* qd Queue -- enqueue/dequeue */
class qdq
{
public:
    static IDE_RC executeCreateQueue( qcStatement * aStatement );
    static IDE_RC executeDropQueue( qcStatement * aStatement );

    static IDE_RC executeDequeue( qcStatement * aStatement, idBool *aIsTimeOut);
    static IDE_RC executeEnqueue( qcStatement * aStatement);

    static IDE_RC wakeupDequeue(qcStatement * aStatement);
    static IDE_RC waitForEnqueue(qcStatement * aStatement, idBool *aIsTimeOut);

    // BUG-22346 : Queue TableÀÇ compact Áö¿ø 
    static IDE_RC validateAlterCompactQueue( qcStatement * aStatement);
    static IDE_RC executeCompactQueue( qcStatement * aStatement);
    
    static IDE_RC executeStart( qcStatement * aStatement);
    static IDE_RC executeStop( qcStatement * aStatement);

    /* BUG-45921 */
    static IDE_RC validateAlterQueueSequence( qcStatement * aStatement );
    static IDE_RC executeAlterQueueSequence( qcStatement * aStatement );
    static IDE_RC checkQueueSequenceInfo( qcStatement      * aStatement,
                                          UInt               aUserID,
                                          qcNamePosition     aQueueName,
                                          qcmSequenceInfo  * aQueueSequenceInfo,
                                          void            ** aQueueSequenceHandle );
    static IDE_RC updateQueueSequenceFromMeta( qcStatement    * aStatement,
                                               UInt             aUserID,
                                               qcNamePosition   aQueueNamePos,
                                               UInt             aQueueSequenceID,
                                               smOID            aQueueSequenceOID,
                                               SInt             aColumnCount,
                                               UInt             aParallelDegree );

    /* BUG-49063 DELETE ON/OFF */
    static IDE_RC validateAlterAllowDelete( qcStatement * aStatement );
};

#endif // _O_QDQ_H_
