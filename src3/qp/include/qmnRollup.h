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
 * $Id: qmnRollup.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * PROJ-1353
 *
 ***********************************************************************/

#ifndef _O_QMN_ROLLUP_H_
#define _O_QMN_ROLLUP_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qmnAggregation.h>
#include <qmcSortTempTable.h>

#define QMND_ROLL_INIT_DONE_MASK           ( 0x00000001 )
#define QMND_ROLL_INIT_DONE_FALSE          ( 0x00000000 )
#define QMND_ROLL_INIT_DONE_TRUE           ( 0x00000001 )

#define QMND_ROLL_GROUP_MATCHED            ( 0x1 )
#define QMND_ROLL_GROUP_NOT_MATCHED        ( 0x0 )

#define QMND_ROLL_COLUMN_MACHED            ( 0x1 )
#define QMND_ROLL_COLUMN_NOT_MATCHED       ( 0x0 )

#define QMND_ROLL_COLUMN_CHECK_NEED        ( 0x1 )
#define QMND_ROLL_COLUMN_CHECK_NONE        ( 0x0 )

typedef struct qmnRollGrouping
{
    qmnGrouping   info;
    SInt          count;
} qmnRollGrouping;

typedef struct qmncROLL
{
    qmnPlan         plan;
    UInt            flag;
    UInt            planID;
    UShort          groupingFuncRowID;  /* Pseudo Column For Grouping Func Data RowID */
    UShort          depTupleID;

    idBool          preservedOrderMode;
    UShort          groupCount;
    UShort          aggrNodeCount;
    UShort          distNodeCount;
    UShort          myNodeCount;
    UShort          mtrNodeCount;
    SShort          partialRollup;

    qmcMtrNode    * myNode;
    qmcMtrNode    * mtrNode;
    qmcMtrNode    * aggrNode;
    qmcMtrNode    * distNode;
    qmcMtrNode    * valueTempNode;
    UChar        ** mtrGroup;
    UShort        * elementCount;

    UInt            mtrNodeOffset;
    UInt            myNodeOffset;
    UInt            aggrNodeOffset;
    UInt            sortNodeOffset;
    UInt            distNodePtrOffset;
    UInt            distNodeOffset;
    UInt            valueTempOffset;

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
} qmncROLL;

typedef struct qmndROLL
{
    qmndPlan           plan;
    doItFunc           doIt;
    UInt             * flag;

    mtcTuple         * mtrTuple;
    mtcTuple         * aggrTuple;
    mtcTuple         * valueTempTuple;
    mtcTuple         * depTuple;

    idBool             needCopy;
    idBool             isDataNone;
    UInt               depValue;
    UInt               mtrRowSize;
    UInt               myRowSize;
    UInt               aggrRowSize;
    UInt               valueTempRowSize;
    SInt               groupIndex;
    UChar            * groupStatus;

    qmdMtrNode       * mtrNode;
    qmdMtrNode       * myNode;
    qmdAggrNode      * aggrNode;
    qmdDistNode     ** distNode;
    qmcdSortTemp       sortMTR;
    qmdMtrNode       * sortNode;
    qmcdSortTemp     * valueTempMTR;
    qmdMtrNode       * valueTempNode;

    void             * nullRow;
    void             * myRow;
    void             * mtrRow;
    void            ** aggrRow;
    UChar            * compareResults;
    SLong            * groupingFuncPtr;
    qmnRollGrouping    groupingFuncData;

    /* PROJ-2462 Result Cache */
    qmndResultCache    resultData;
} qmndROLL;

class qmnROLL
{
public:
    static IDE_RC init( qcTemplate * aTemplate, qmnPlan * aPlan );

    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    static IDE_RC padNull( qcTemplate * aTemplate, qmnPlan * aPlan );

    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );

    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    static IDE_RC doItFirstSortMTR( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    qmcRowFlag * aFlag );

    static IDE_RC doItNextSortMTR( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan,
                                   qmcRowFlag * aFlag );

    static IDE_RC doItFirstValueTempMTR( qcTemplate * aTemplate,
                                         qmnPlan    * aPlan,
                                         qmcRowFlag * aFlag );

    static IDE_RC doItNextValueTempMTR( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag );

private:
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncROLL   * aCodePlan,
                             qmndROLL   * aDataPlan );

    static IDE_RC linkAggrNode( qmncROLL * aCodePlan, qmndROLL * aDataPlan );

    static IDE_RC initDistNode( qcTemplate * aTemplate,
                                qmncROLL   * aCodePlan,
                                qmndROLL   * aDataPlan );

    static IDE_RC clearDistNode( qmncROLL * aCodePlan,
                                 qmndROLL * aDataPlan,
                                 UInt       aGroupIndex );

    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncROLL   * aCodePlan,
                               qmndROLL   * aDataPlan );

    static IDE_RC allocMtrRow( qcTemplate * aTemplate,
                               qmncROLL   * aCodePlan,
                               qmndROLL   * aDataPlan );

    static IDE_RC initAggregation( qcTemplate * aTemplate,
                                   qmndROLL   * aDataPlan,
                                   UInt         aGroupIndex );

    static IDE_RC setDistMtrColumns( qcTemplate * aTemplate,
                                     qmncROLL   * aCodePlan,
                                     qmndROLL   * aDataPlan,
                                     UInt         aGroupIndex );
    static IDE_RC execAggregation( qcTemplate * aTemplate,
                                   qmncROLL   * aCodePlan,
                                   qmndROLL   * aDataPlan,
                                   UInt         aGroupIndex );

    static IDE_RC finiAggregation( qcTemplate * aTemplate,
                                   qmndROLL   * aDataPlan );

    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndROLL   * aDataPlan );

    static IDE_RC copyMtrRowToMyRow( qmndROLL * aDataPlan );

    static IDE_RC compareRows( qmndROLL * aDataPlan );

    static IDE_RC compareGroupsExecAggr( qcTemplate * aTemplate,
                                         qmncROLL   * aCodePlan,
                                         qmndROLL   * aDataPlan,
                                         idBool     * aAllMatched );
    static IDE_RC storeChild( qcTemplate * aTemplate,
                              qmncROLL   * aCodePlan,
                              qmndROLL   * aDataPlan );

    static IDE_RC setTupleMtrNode( qcTemplate * aTemplate,
                                   qmndROLL   * aDataPlan );

    static IDE_RC setTupleValueTempNode( qcTemplate * aTemplate,
                                         qmndROLL   * aDataPlan );
    
    static IDE_RC valueTempStore( qcTemplate * aTemplate, qmnPlan * aPlan );

    static IDE_RC checkDependency( qcTemplate * aTemplate,
                                   qmncROLL   * aCodePlan,
                                   qmndROLL   * aDataPlan,
                                   idBool     * aDependent );
};

#endif /* _O_QMN_ROLLUP_H_ */
