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
 * $Id: qmnPSCRD.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QMN_PSCRD_H_
#define _O_QMN_PSCRD_H_ 1

#include <qmnDef.h>

// qmndPSCRD.flag
// First Initialization Done
#define QMND_PSCRD_INIT_DONE_MASK  (0x00000001)
#define QMND_PSCRD_INIT_DONE_FALSE (0x00000000)
#define QMND_PSCRD_INIT_DONE_TRUE  (0x00000001)

typedef struct qmncPSCRD
{
    qmnPlan          plan;
    UInt             planID;

    qmsTableRef    * mTableRef;
    UShort           mTupleRowID;

    qmsNamePosition  mTableOwnerName;
    qmsNamePosition  mTableName;
    qmsNamePosition  mAliasName;

    void           * mTableHandle;
    smSCN            mTableSCN;

    qtcNode        * mConstantFilter;
} qmncPSCRD;

typedef struct qmnPSCRDChildren
{
    qmnPlan         * mPlan;
    UInt              mTID;
    qmnPSCRDChildren* mNext;
} qmndPSCRDChildInfo;

typedef struct qmndPSCRD
{
    qmndPlan          plan;
    doItFunc          doIt;
    UInt            * flag;

    void            * mOrgRow;
    void            * mNullRow;
    scGRID            mNullRID;

    UInt              mPRLQCnt;
    qmnPSCRDChildren* mChildrenPRLQArea;
    qmnPSCRDChildren* mCurPRLQ;
    qmnPSCRDChildren* mPrevPRLQ;
    idBool            mSerialRemain;

    UInt              mAccessCount;
} qmndPSCRD;

class qmnPSCRD
{
public:

    static IDE_RC init(qcTemplate * aTemplate,
                       qmnPlan    * aPlan);

    static IDE_RC doIt(qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag);

    static IDE_RC padNull(qcTemplate * aTemplate,
                          qmnPlan    * aPlan);

    static IDE_RC printPlan(qcTemplate   * aTemplate,
                            qmnPlan      * aPlan,
                            ULong          aDepth,
                            iduVarString * aString,
                            qmnDisplay     aMode);

private:

    static IDE_RC firstInit(qcTemplate* aTemplate,
                            qmncPSCRD * aCodePlan,
                            qmndPSCRD * aDataPlan);

    static IDE_RC doItFirst(qcTemplate* aTemplate,
                            qmnPlan   * aPlan,
                            qmcRowFlag* aFlag);

    static IDE_RC doItNext(qcTemplate* aTemplate,
                           qmnPlan   * aPlan,
                           qmcRowFlag* aFlag);

    static IDE_RC doItAllFalse(qcTemplate* aTemplate,
                               qmnPlan   * aPlan,
                               qmcRowFlag* aFlag);

    static void printAccessInfo(qmndPSCRD   * aDataPlan,
                                iduVarString* aString,
                                qmnDisplay    aMode);

    static void makeChildrenPRLQArea(qcTemplate* aTemplate,
                                     qmnPlan   * aPlan);
};

#endif /* _O_QMN_PSCRD_H_ */

