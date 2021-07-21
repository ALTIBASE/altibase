/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id: act.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACT_PERF_H_)
#define _O_ACT_PERF_H_

/**
 * @dir core/src/act
 *
 * Altibase Core Unit Testing Framework
 */

#include <actBarrier.h>

/* ------------------------------------------------
 *  Framework Definition 
 * ----------------------------------------------*/

struct actPerfFrm ; /* forward declaration */

typedef struct actPerfFrmResult
{
    acp_time_t     mBeginTime; /* begin time of the test case */
    acp_time_t     mEndTime;   /*   end time of the test case */
    acp_double_t   mOPS;       /* operation per second */
} actPerfFrmResult;

typedef struct actPerfFrmObj
{
    struct actPerfFrm   *mFrm;  /* original framework desc info */
    void                *mData; /* user specific data */
    actPerfFrmResult     mResult;
    
} actPerfFrmObj;

typedef struct actPerfFrmThrObj
{
    acp_sint32_t  mNumber;
    actPerfFrmObj mObj;
} actPerfFrmThrObj;

typedef acp_rc_t actPerfFrmCallback(actPerfFrmThrObj *aObj);

typedef struct actPerfFrm
{
    acp_char_t          *mTitle;
    acp_uint32_t         mParallel;
    acp_uint64_t         mLoopCount;
    actPerfFrmCallback *mInit;
    actPerfFrmCallback *mBody;
    actPerfFrmCallback *mFinal;
} actPerfFrm;

#define ACT_PERF_FRM_SENTINEL { NULL, 0, 0, NULL, NULL, NULL }

/*
extern act_barrier_t gActBarrier;
*/

ACP_EXPORT act_barrier_t* actPerfGetBarrier(void);

ACP_EXPORT acp_rc_t actPerfFrameworkRun(acp_char_t *aLogFileName,
                                        actPerfFrm *aDesc);


#endif
