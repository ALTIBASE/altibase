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
 * $Id: qdcJob.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * PROJ-1438 Job Scheduler
 *
 **********************************************************************/

#ifndef _O_QDC_JOB_H_
#define _O_QDC_JOB_H_ 1

#include <qc.h>
#include <qci.h>
#include <qdParseTree.h>

#define QDC_JOB_EXEC_MAX_LEN    (1000)
#define QDC_JOB_DATE_MAX_LEN    (256)
#define QDC_JOB_EXEC_QUERY_LEN  (QDC_JOB_EXEC_MAX_LEN + 6)

/* PROJ-1438 Job Scheduler */
typedef enum qdcJobIntervalType
{
    QDC_JOB_INTERVAL_TYPE_NONE = 0,
    QDC_JOB_INTERVAL_TYPE_YEAR,
    QDC_JOB_INTERVAL_TYPE_MONTH,
    QDC_JOB_INTERVAL_TYPE_DAY,
    QDC_JOB_INTERVAL_TYPE_HOUR,
    QDC_JOB_INTERVAL_TYPE_MINUTE,
    QDC_JOB_INTERVAL_TYPE_SECOND
} qdcJobIntervalType;

typedef struct qdcJobInterval
{
    SLong              number;
    qdcJobIntervalType type;
} qtcJobInterval;

class qdcJob
{
public:

    static IDE_RC validateCreateJob( qcStatement * aStatement );

    static IDE_RC validateAlterJob( qcStatement * aStatement );

    static IDE_RC validateDropJob( qcStatement * aStatement );

    static IDE_RC executeCreateJob( qcStatement * aStatement );

    static IDE_RC executeAlterJobExec( qcStatement * aStatement );

    static IDE_RC executeAlterJobStartEnd( qcStatement * aStatement );

    static IDE_RC executeAlterJobInterval( qcStatement * aStatement );

    static IDE_RC executeDropJob( qcStatement * aStatement );

    // BUG-41713 each job enable disable
    static IDE_RC executeAlterJobEnable( qcStatement * aStatement );

    // BUG-41713 each job enable disable
    static IDE_RC executeAlterJobComment( qcStatement * aStatement );
private:

    static IDE_RC makeExecTxt( SChar          * aExecTxt,
                               qcNamePosition * aExec );

    static void makeIntervalTypeTxt( SChar * aBufTxt,
                                     SInt    aIntervalType );
};

#endif /* _O_QDC_JOB_H_ */
