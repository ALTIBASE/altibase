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
 * $Id: qmnCube.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * PROJ-1353
 ***********************************************************************/

#ifndef _O_QMN_CUBE_H_
#define _O_QMN_CUBE_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qmnAggregation.h>
#include <qmcSortTempTable.h>

#define QMND_CUBE_INIT_DONE_MASK           ( 0x00000001 )
#define QMND_CUBE_INIT_DONE_FALSE          ( 0x00000000 )
#define QMND_CUBE_INIT_DONE_TRUE           ( 0x00000001 )

/* cube group의 수행완료여부 */
#define QMND_CUBE_GROUP_DONE_MASK          ( 0x8000 )
#define QMND_CUBE_GROUP_DONE_FALSE         ( 0x0000 )
#define QMND_CUBE_GROUP_DONE_TRUE          ( 0x8000 )


typedef struct qmnCubeGrouping
{
    qmnGrouping   info;
    UInt        * subIndexMap;
    UShort      * groups;
} qmnCubeGrouping;

typedef struct qmncCUBE
{
    qmnPlan         plan;
    UInt            flag;
    UInt            planID;
    UShort          groupingFuncRowID;  /* Pseudo Column For Grouping Func Data RowID */
    UInt            groupCount;
    UShort          depTupleID;

    UShort          cubeCount;
    UShort          aggrNodeCount;
    UShort          distNodeCount;
    UShort          myNodeCount;
    UShort          mtrNodeCount;
    SShort          partialCube;
    UShort        * elementCount;

    qmcMtrNode    * myNode;
    qmcMtrNode    * mtrNode;
    qmcMtrNode    * aggrNode;
    qmcMtrNode    * distNode;
    qmcMtrNode    * valueTempNode;

    UInt            mtrNodeOffset;
    UInt            myNodeOffset;
    UInt            aggrNodeOffset;
    UInt            sortNodeOffset;
    UInt            distNodePtrOffset;
    UInt            distNodeOffset;
    UInt            valueTempOffset;

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
} qmncCUBE;

typedef struct qmndCUBE
{
    qmndPlan           plan;
    doItFunc           doIt;
    UInt             * flag;

    mtcTuple         * mtrTuple;
    mtcTuple         * aggrTuple;
    mtcTuple         * valueTempTuple;
    mtcTuple         * depTuple;

    UInt               depValue;
    idBool             needCopy;
    idBool             isDataNone;
    idBool             noStoreChild;
    UInt               mtrRowSize;
    UInt               myRowSize;
    UInt               aggrRowSize;
    UInt               valueTempRowSize;
    ULong              compareResults;
    SInt               subIndex;
    UInt               subStatus;
    UInt               subGroupCount;
    UInt               groupIndex;
    UInt             * subIndexToGroupIndex;
    UShort           * cubeGroups;

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
    void             * searchRow;
    SLong            * groupingFuncPtr;
    qmnCubeGrouping    groupingFuncData;

    /* PROJ-2462 Result Cache */
    qmndResultCache    resultData;
} qmndCUBE;

class qmnCUBE
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

    static IDE_RC doItFirstValueTemp( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan,
                                      qmcRowFlag * aFlag );

    static IDE_RC doItNextValueTemp( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan,
                                     qmcRowFlag * aFlag );

private:
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncCUBE   * aCodePlan,
                             qmndCUBE   * aDataPlan );

    static IDE_RC clearDistNode( qmncCUBE * aCodePlan,
                                 qmndCUBE * aDataPlan,
                                 UInt       aSubIndex );

    static IDE_RC setDistMtrColumns( qcTemplate * aTemplate,
                                     qmncCUBE   * aCodePlan,
                                     qmndCUBE   * aDataPlan,
                                     UInt         aSubIndex );

    static IDE_RC storeChild( qcTemplate * aTemplate,
                              qmncCUBE   * aCodePlan,
                              qmndCUBE   * aDataPlan );

    static IDE_RC linkAggrNode( qmncCUBE * aCodePlan, qmndCUBE * aDataPlan );

    static IDE_RC initDistNode( qcTemplate * aTemplate,
                                qmncCUBE   * aCodePlan,
                                qmndCUBE   * aDataPlan );

    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncCUBE   * aCodePlan,
                               qmndCUBE   * aDataPlan );

    static IDE_RC allocMtrRow( qcTemplate * aTemplate,
                               qmncCUBE   * aCodePlan,
                               qmndCUBE   * aDataPlan );

    static IDE_RC initAggregation( qcTemplate * aTemplate,
                                   qmndCUBE   * aDataPlan,
                                   UInt         aGroupIndex );

    static IDE_RC execAggregation( qcTemplate * aTemplate,
                                   qmncCUBE   * aCodePlan,
                                   qmndCUBE   * aDataPlan,
                                   UInt         aGroupIndex );

    static IDE_RC finiAggregation( qcTemplate * aTemplate,
                                   qmndCUBE   * aDataPlan );

    static IDE_RC setTupleMtrNode( qcTemplate * aTemplate,
                                   qmndCUBE   * aDataPlan );

    static IDE_RC setTupleValueTempNode( qcTemplate * aTemplate,
                                         qmndCUBE   * aDataPlan );
    
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndCUBE   * aDataPlan );

    static IDE_RC copyMtrRowToMyRow( qmndCUBE * aDataPlan );

    static IDE_RC makeGroups( qcTemplate * aTemplate,
                              qmncCUBE   * aCodePlan,
                              qmndCUBE   * aDataPlan );

    static void initGroups( qmncCUBE   * aCodePlan,
                            qmndCUBE   * aDataPlan );

    static UInt findRemainGroup( qmndCUBE * aDataPlan );

    static IDE_RC makeSortNodeAndSort( qmncCUBE * aCodePlan,
                                       qmndCUBE * aDataPlan );

    static void setSubGroups( qmncCUBE * aCodePlan, qmndCUBE * aDataPlan );

    static IDE_RC compareRows( qmndCUBE * aDataPlan );

    static IDE_RC compareGroupsExecAggr( qcTemplate * aTemplate,
                                         qmncCUBE   * aCodePlan,
                                         qmndCUBE   * aDataPlan,
                                         idBool     * aAllMatched );

    static void setColumnNull( qmncCUBE * aCodePlan,
                               qmndCUBE * aDataPlan,
                               SInt       aSubIndex );

    static IDE_RC valueTempStore( qcTemplate * aTemplate, qmnPlan * aPlan );

    static IDE_RC checkDependency( qcTemplate * aTemplate,
                                   qmncCUBE   * aCodePlan,
                                   qmndCUBE   * aDataPlan,
                                   idBool     * aDependent );
};

#endif /* _O_QMN_CUBE_H_ */
