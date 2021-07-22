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
 * $Id: qmvGBGSTransform.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 *
 * PROJ-2415 Grouping Sets Clause
 * 
 * Grouping Sets가 생성 할 Sub Group을 각각의 Query Set으로 만들고
 * 이를 UNION ALL 로 병합하여 논리적으로 동등한 ParseTree를 생성한다.
 * 
 * SELECT /+Hint+/ TOP 3 i1, i2, i3 x, SUM( i4 )
 *   INTO :a, :b, :c, :d
 *   FROM t1
 *  WHERE i1 >= 0
 * GROUP BY GROUPING SETS( i1, i2, i3, () )
 * HAVING GROUPING_ID( i1, i2, i3 ) >= 0;
 * 
 * ========================= The Above Query Would Be Transformed As Below ==========================
 *        
 * SELECT /+Hint+/ TOP 3 *  -> INTO Clause 와 TOP, ORDER BY를 처리하기 위해 원본 Query Set을 Simple View 형태로 변형한다.
 *   INTO :a, :b, :c, :d
 *   FROM (                 -> GROUPING SETS를 포함한 QuerySet을 각 GROUP 별로 복제&변형된 Query Sets에 대한
 *                             UNION ALL Set Query로 변형하여 Simple View 형태로 변형된 원본 Query Set의 FROM에 Inline View로 연결
 *                             
 *            SELECT i1, NULL, NULL x, SUM( i4 )         
 *              FROM ( SELECT * FROM t1 WHERE i1 >= 0 ) -> SCAN COUNT를 줄이기 위해 SELECT, FROM, WHERE, HIERARCHY를
 *            GROUP BY i1                                  InlineView로 변형
 *            HAVING GROUPING_ID( i1, NULL, NULL ) >= 0;
 *                       UNION ALL
 *            SELECT NULL, i2, NULL x, SUM( i4 )             
 *              FROM ( SELECT * FROM t1 WHERE i1 >= 0 ) -> SameViewRef를 통해 상위의 View를 바라보도록 설정
 *            GROUP BY i2                              
 *            HAVING GROUPING_ID( NULL, i2, NULL ) >= 0;
 *                       UNION ALL
 *            SELECT NULL, NULL, i3 x, SUM( i4 )
 *              FROM ( SELECT * FROM t1 WHERE i1 >= 0 ) -> SameViewRef를 통해 상위의 View를 바라보도록 설정
 *            GROUP BY i3                               
 *            HAVING GROUPING_ID( NULL, NULL, i3 ) >= 0;
 *            
 *        );
 *
 * * GROUPING SETS의 인자가 하나 일 경우 GROUP BY 와 동일하기 때문에 GROUPING SETS 만 제거한다.
 *
 ***********************************************************************/

#include <idl.h>
#include <qcg.h>
#include <qtc.h>
#include <qmvQTC.h>
#include <qmvOrderBy.h>
#include <qmvGBGSTransform.h>
#include <qmvQuerySet.h>
#include <qmv.h>
#include <qmo.h>
#include <qmx.h>
#include <qcpManager.h>
#include <qmoViewMerging.h>

IDE_RC qmvGBGSTransform::searchGroupingSets( qmsQuerySet       * aQuerySet,
                                             qmsConcatElement ** aGroupingSets,
                                             qmsConcatElement ** aPrevGroup )
{
/***********************************************************************
 *
 * Description :
 *    GROUP BY 절에서 GROUPING SETS Type의 Group과
 *    그 Group을 Next로 가지는 Previous Group을 리턴한다. 
 *
 * Implementation :
 *
 ***********************************************************************/
    qmsConcatElement * sGroup;
    qmsConcatElement * sPrevGroup = NULL;
    idBool             sIsFound   = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::searchGroupingSets::__FT__" );

    *aGroupingSets = NULL;
    *aPrevGroup    = NULL;

    sGroup = aQuerySet->SFWGH->group;

    for ( sGroup  = aQuerySet->SFWGH->group;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        switch ( sGroup->type )
        {
            case QMS_GROUPBY_GROUPING_SETS :
                *aGroupingSets = sGroup;
                *aPrevGroup = sPrevGroup;
                /* fall through */ 
            case QMS_GROUPBY_ROLLUP :
            case QMS_GROUPBY_CUBE :
                if ( sIsFound == ID_FALSE )
                {
                    sIsFound = ID_TRUE;
                }
                else
                {
                    IDE_RAISE( ERR_NOT_SUPPORT_COMPLICATE_GROUP_EXT );
                }
                break;
            case QMS_GROUPBY_NORMAL :
            case QMS_GROUPBY_NULL:
                if ( sIsFound == ID_FALSE )
                {
                    sPrevGroup = sGroup;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            default :
                IDE_RAISE( ERR_UNEXPECTED_GROUP_BY_TYPE );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_COMPLICATE_GROUP_EXT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_SUPPORT_COMPLICATE_GROUP_EXT ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_GROUP_BY_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::searchGroupingSets",
                                  "Unsupported groupby type." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::copyPartialQuerySet( qcStatement          * aStatement,
                                              qmsConcatElement     * aGroupingSets,
                                              qmsQuerySet          * aSrcQuerySet,
                                              qmvGBGSQuerySetList ** aDstQuerySetList )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Sets의 Arguments 수 만큼 QuerySet을 생성하여
 *     List( qmvGBGSQuerySetList )형태로 연결한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmsConcatElement    * sGroupingSetsArgs;
    qmvGBGSQuerySetList * sQuerySetList = NULL;
    qmvGBGSQuerySetList * sPrevQuerySetList = NULL;
    qmsSFWGH            * sSFWGH;
    qmsFrom             * sFrom;    
    SInt                  sStartOffset = 0;
    SInt                  sSize = 0;
    qcStatement           sStatement;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::copyPartialQuerySet::__FT__" );

    QC_SET_STATEMENT( ( &sStatement ), aStatement, NULL );

    // SrcQuerySet의 Query Statement에 서 SELECT~GROUP~(HAVING)까지의
    // Partial Statement의 Offset과 Size를 구한다.    
    IDE_TEST( getPosForPartialQuerySet( aSrcQuerySet,
                                        & sStartOffset,
                                        & sSize )
              != IDE_SUCCESS );

    for ( sGroupingSetsArgs  = aGroupingSets->arguments;
          sGroupingSetsArgs != NULL;
          sGroupingSetsArgs  = sGroupingSetsArgs->next )
    {
        // FIT TEST
        IDU_FIT_POINT( "qmvGBGSTransform::copyPartialQuerySet::STRUCT_ALLOC::ERR_MEM",
                       idERR_ABORT_InsufficientMemory);
        
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmvGBGSQuerySetList,
                                &sQuerySetList ) != IDE_SUCCESS);

        if ( sGroupingSetsArgs == aGroupingSets->arguments )
        {
            // First Loop
            sPrevQuerySetList  = sQuerySetList;
            *aDstQuerySetList  = sPrevQuerySetList;
        }
        else
        {
            if ( sPrevQuerySetList != NULL )
            {
                sPrevQuerySetList->next = sQuerySetList;
                sPrevQuerySetList = sPrevQuerySetList->next;
            }
            else
            {
                // Compile Warning 때문에 추가
                IDE_RAISE( ERR_INVALID_POINTER );
            }            
        }

        sStatement.myPlan->stmtText = aSrcQuerySet->startPos.stmtText;
        sStatement.myPlan->stmtTextLen = idlOS::strlen( aSrcQuerySet->startPos.stmtText );
        
        // SrcQuerySet의 Statement Text를 PartialParsing해서 parseTree를 복제한다.    
        IDE_TEST( qcpManager::parsePartialForQuerySet( &sStatement,
                                                       aSrcQuerySet->startPos.stmtText,
                                                       sStartOffset,
                                                       sSize )
                  != IDE_SUCCESS );

        sQuerySetList->querySet = ( ( qmsParseTree* )sStatement.myPlan->parseTree )->querySet;
        
        sSFWGH = sQuerySetList->querySet->SFWGH;
        // Grouping Sets Transformed MASK 
        sSFWGH->lflag &= ~QMV_SFWGH_GBGS_TRANSFORM_MASK;
        sSFWGH->lflag |= QMV_SFWGH_GBGS_TRANSFORM_BOTTOM;

        // Bottom View는 Simple View Merging하지 않는다.
        for ( sFrom  = sQuerySetList->querySet->SFWGH->from;
              sFrom != NULL;
              sFrom  = sFrom->next )
        {        
            IDE_TEST( setNoMerge( sFrom ) 
                      != IDE_SUCCESS );
        }           

        // DISTINCT TOP INTO Initialize
        sSFWGH->selectType = QMS_ALL;
        sSFWGH->top = NULL;
        sSFWGH->intoVariables = NULL;
    }
   
    if ( sQuerySetList != NULL )
    { 
        sQuerySetList->next = NULL;    
    }
    else
    {
        IDE_RAISE( ERR_UNEXPECTED_ERROR );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::copyPartialQuerySet",
                                  "Unexpected Error" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_POINTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::copyPartialQuerySet",
                                  "Invalid Pointer" ) );
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::getPosForPartialQuerySet( qmsQuerySet * aQuerySet,
                                                   SInt        * aStartOffset,
                                                   SInt        * aSize )
{
/***********************************************************************
 *
 * Description :
 *     Query Set에서 SELECT - FROM - WHERE - HIERARCHY - GROUPBY - (HAVING)의
 *     부분 Statement를 Parsing하기 위해 SELECT의 시작 Offset과
 *     GROUPBY( HAVING이 있을경우 HAVING )의 LastNode Offset+Size를 구한다.
 *
 * Implementation :
 * 
 ***********************************************************************/
    SInt               sStartOffset = QC_POS_EMPTY_OFFSET; // -1
    SInt               sEndOffset   = QC_POS_EMPTY_OFFSET; // -1
    qmsConcatElement * sGroup;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::getPosForPartialQuerySet::__FT__" );

    // Get Start Offset of SELECT    
    sStartOffset = aQuerySet->startPos.offset;

    // Get End Offset of HAVING or GROUP BY 
    if ( aQuerySet->SFWGH->having != NULL )
    {
        sEndOffset = aQuerySet->SFWGH->having->position.offset
            + aQuerySet->SFWGH->having->position.size;
    }
    else
    {        
        for ( sGroup = aQuerySet->SFWGH->group;
              sGroup->next != NULL;
              sGroup = sGroup->next )
        {
            // GROUP BY의 마지막 Element를 구한다.
        };

        sEndOffset = sGroup->position.offset + sGroup->position.size;
    }

    // 뒤의 1 Byte를 확인하여 Double Quotation이 있으면 포함한다.
    if ( *( aQuerySet->startPos.stmtText + sEndOffset ) == '"' )
    {
        sEndOffset++;
    }
    else
    {
        /* Nothing to do */
    }    
    
    // Invalid Offset
    IDE_TEST_RAISE( ( sStartOffset == QC_POS_EMPTY_OFFSET ) ||
                    ( sEndOffset   == QC_POS_EMPTY_OFFSET ),
                    ERR_INVALID_OFFSET );
    
    //Set Return Variables    
    *aStartOffset = sStartOffset;
    *aSize        = sEndOffset - *aStartOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_OFFSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::getPosForPartialQuerySet",
                                  "Invalid Offset" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::getPosForOrderBy( qmsSortColumns * aOrderBy,
                                           SInt           * aStartOffset,
                                           SInt           * aSize )
{
/***********************************************************************
 *
 * Description :
 *     ORDER BY 의 sortColumn을 Partial Parsing 하기 위해
 *     First Node의 Offset과 Last Node Offset + Size를 구한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    SInt               sStartOffset = QC_POS_EMPTY_OFFSET; // -1
    SInt               sEndOffset   = QC_POS_EMPTY_OFFSET; // -1
    qmsSortColumns   * sOrderBy     = aOrderBy;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::getPosForOrderBy::__FT__" );

    // Get Start Offset of The First Node Of ORDER BY    
    sStartOffset = sOrderBy->sortColumn->position.offset;

    // 앞의 1 Byte를 확인하여 Double Quotation이 있으면 포함한다.
    if ( sStartOffset > 0 )
    {
        if ( *( sOrderBy->sortColumn->position.stmtText + sStartOffset - 1 ) == '"' )
        {
            sStartOffset--;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    for ( ; sOrderBy->next != NULL; sOrderBy  = sOrderBy->next )
    {
        // OrderBy의 마지막 Node를 구한다.
    };

    // Get End Offset of The Last Node Of ORDER BY
    sEndOffset = sOrderBy->sortColumn->position.offset +
        sOrderBy->sortColumn->position.size;
    
    // 뒤의 1 Byte를 확인하여 Double Quotation이 있으면 포함한다.
    if ( *( sOrderBy->sortColumn->position.stmtText + sEndOffset ) == '"' )
    {
        sEndOffset++;
    }
    else
    {
        /* Nothing to do */
    }

    // Invalid Offset
    IDE_TEST_RAISE( ( sStartOffset == QC_POS_EMPTY_OFFSET ) ||
                    ( sEndOffset   == QC_POS_EMPTY_OFFSET ),
                    ERR_INVALID_OFFSET );

    // Set Return Variables
    *aStartOffset = sStartOffset;
    *aSize = sEndOffset - sStartOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_OFFSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::getPosForOrderBy",
                                  "Invalid Offset" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::modifyGroupBy( qcStatement         * aStatement,
                                        qmvGBGSQuerySetList * aQuerySetList )
{
/***********************************************************************
 *
 * Description :
 *     GROUP BY 절에서 QMS_GROUPBY_GROUPING_SETS Type의 Group Element 와
 *     그 Grouping Sets Type Element 를 Next로 가지는 Prev Element를 찾고
 *     Grouping Sets Clause를 Group By Clause로 변형한다.
 *     
 * Implementation :
 *
 ***********************************************************************/
    qmvGBGSQuerySetList * sQuerySetList;
    qmsConcatElement    * sGroup = NULL;
    qmsConcatElement    * sPrevGroup = NULL;    
    SInt                  sNullCheck = 0;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::modifyGroupBy::__FT__" );

    for ( sQuerySetList  = aQuerySetList;
          sQuerySetList != NULL;
          sQuerySetList  = sQuerySetList->next, sNullCheck++ )
    {
        // Grouping Sets Type의 Group Element와 Previous Group Element의 포인터를 얻는다.   
        IDE_TEST( searchGroupingSets( sQuerySetList->querySet, &sGroup, &sPrevGroup )
                  != IDE_SUCCESS );

        // GROUP BY 절을 변형한다.
        if ( sGroup != NULL )
        {
            IDE_TEST( modifyGroupingSets( aStatement,
                                          sQuerySetList->querySet->SFWGH,
                                          sPrevGroup,
                                          sGroup,
                                          sGroup->next,
                                          sNullCheck )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE( ERR_UNEXPECTED_ERROR ); 
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::modifyGroupBy",
                                  "Unexpected Error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::modifyGroupingSets( qcStatement      * aStatement,
                                             qmsSFWGH         * aSFWGH,
                                             qmsConcatElement * aPrev,
                                             qmsConcatElement * aElement,
                                             qmsConcatElement * aNext,
                                             SInt               aNullCheck )
{
/***********************************************************************
 *
 * Description :
 *     GROUP BY절에서 Grouping sets를 제거하고, Grouping Sets 의 Argument들을
 *     GroupingSets가 있던 자리에 연결한다.
 *     
 *     Grouping sets의 arguments중 자신의 Query Set에 해당하는 Group이 아니면,
 *     - QMS_GROUPBY_NORMAL Type일 경우 QMS_GROUPBY_NULL로 변경한다.
 *     - QMS_GROUPBY_ROLLUP 또는 QMS_GROUPBY_CUBE 일 경우 ROLLUP/CUBE Group을 제거하고,
 *       ROLLUP/CUBE의 Argument들을 ROLLUP/CUBE가 있었던 자리에 연결한다.  
 *
 *     Grouping Sets의 인자로 올 수 있는 Group Type은 다음과 같다.
 *
 *     1) QMS_GROUPBY_NORMAL
 *                           1-1) Normal Group ( Normal Expression )
 *                           1-2) Concatenated Group( LIST )
 *     2) QMS_GROUPBY_ROLLUP 
 *     3) QMS_GROUPBY_CUBE
 *
 *     ROLLUP과 CUBE의 인자로 올 수 있는 Group Type은 다음과 같다.
 *
 *     1) QMS_GROUPBY_NORMAL
 *                           1-1) Normal Group ( Normal Expression )
 *                           1-2) Concatenated Group( LIST )
 *
 *     Example)
 *     
 *     GROUP BY GROUPING SETS( i1, ROLLUP( i2, i3 ), ( i4, i5 ) ), i6
 *     
 *     위의 Grouping Sets를 포함한 Query Set은 아래와 같이 3개의 Query Set으로
 *     Transform 된다.
 *       
 *     QUERY SET 1 : GROUP BY i1, i2(NULL), i3(NULL), i4(NULL), i5(NULL), i6
 *     UNION ALL
 *     QUERY SET 2 : GROUP BY i1(NULL), ROLLUP( i2, i3 ), i4(NULL), i5(NULL), i6
 *     UNION ALL
 *     QUERY SET 3 : GROUP BY i1(NULL), i2(NULL), i3(NULL), i4, i5, i6
 *
 *     * (NULL) 은 QMS_GROUPBY_NULL Type의 Group Element를 의미한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmsConcatElement * sElement;
    qmsConcatElement * sPrev;
    SInt               sNullIndex;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::modifyGroupingSets::__FT__" );

    if ( aPrev == NULL )
    {
        aSFWGH->group = aElement->arguments;
        sPrev = NULL;
    }
    else
    {
        aPrev->next = aElement->arguments;
        sPrev = aPrev;
    }

    for ( sElement  = aElement->arguments, sNullIndex = 0;
          sElement != NULL; 
          sElement  = sElement->next, sNullIndex++ )
    {
        // 복제된 querySetList에서 자신의 querySet순번( aNullCheck )과
        // Grouping Sets Argument의 순번(sNullIndex)이 같은 Group만 유효하다.
        if ( aNullCheck == sNullIndex )
        {   // 유효한 Group일 경우
            
            if ( sElement->type == QMS_GROUPBY_NORMAL )
            {
                if ( ( sElement->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK ) ==
                     MTC_NODE_OPERATOR_LIST )
                {
                    // LIST 일 경우 처리
                    IDE_TEST( modifyList( aStatement,
                                          aSFWGH,
                                          &sPrev,
                                          sElement,
                                          ( sElement->next == NULL ) ? aNext : sElement->next,
                                          QMS_GROUPBY_NORMAL )
                              != IDE_SUCCESS );
                }
                else
                {
                    sPrev = sElement;
                }
            }
            else
            {
                sPrev = sElement; 
            }
        }
        else
        {   // 유효하지 않은 Group일 경우            
            switch ( sElement->type )
            {
                case QMS_GROUPBY_ROLLUP :
                case QMS_GROUPBY_CUBE :
                    // ROLLUP, CUBE 일 경우 처리
                    IDE_TEST( modifyRollupCube( aStatement,
                                                aSFWGH,
                                                &sPrev,
                                                sElement,
                                                ( sElement->next == NULL ) ? aNext : sElement->next )
                              != IDE_SUCCESS );
                    break;  
                case QMS_GROUPBY_NORMAL :
                    if ( ( sElement->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK ) ==
                         MTC_NODE_OPERATOR_LIST )
                    {
                        // LIST 일 경우 처리
                        IDE_TEST( modifyList( aStatement,
                                              aSFWGH,
                                              &sPrev,
                                              sElement,
                                              ( sElement->next == NULL ) ? aNext : sElement->next,
                                              QMS_GROUPBY_NULL )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        sElement->type = QMS_GROUPBY_NULL;
                        sPrev = sElement;
                    }
                    break;
                default :
                    IDE_RAISE( ERR_NOR_SUPPORT_COMPLICATE_GROUP_EXT );
                    break;   
            }
        }
    }

    if ( sPrev != NULL )
    {
        if ( sPrev->next == NULL )
        {
            sPrev->next = aNext;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */

    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOR_SUPPORT_COMPLICATE_GROUP_EXT );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_SUPPORT_COMPLICATE_GROUP_EXT));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
IDE_RC qmvGBGSTransform::modifyList( qcStatement        * aStatement,
                                     qmsSFWGH           * aSFWGH,
                                     qmsConcatElement  ** aPrev,
                                     qmsConcatElement   * aElement,
                                     qmsConcatElement   * aNext,
                                     qmsGroupByType       aType )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Sets의 arguments로 List 형태( Concatenated Group )가 오거나
 *     Grouping Sets의 arguments중 Rollup 또는 Cube 안에 List 형태의 Group Element가 있을 경우
 *     LIST 를 제거하고 별도의 qmsConcatElement를 생성해서 LIST가 있던 자리에 연결한다.
 *
 *     GROUP BY GROUPING SETS( i1, ( i2, i3 ), ROLLUP( i4, ( i5, i6 ) ) )
 *
 *     GROUP 1 :  i1, i2(NULL), i3(NULL), i4(NULL), i5(NULL), i6(NULL)
 *     GROUP 2 :  i1(NULL), i2, i3, i4(NULL), i5(NULL), i6(NULL)
 *     GROUP 3 :  i1(NULL), i2(NULL), i3(NULL), ROLLUP( i4, ( i5, i6 ) )
 *
 *     * NULL 은 QMS_GROUPBY_NULL Type의 Group Element를 의미한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qtcNode          * sNode;
    qtcNode          * sPrevNode = NULL;
    qmsConcatElement * sElement;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::modifyList::__FT__" );

    for ( sNode  = ( qtcNode* )aElement->arithmeticOrList->node.arguments;
          sNode != NULL;
          sNode  = ( qtcNode* )sNode->node.next )
    {
        // LIST를 제거하고, LIST의 arguments를 Group Element 로 연결한다.

        // FIT TEST CODE
        IDU_FIT_POINT( "qmvGBGSTransform::modifyList::STRUCT_ALLOC::ERR_MEM",
                       idERR_ABORT_InsufficientMemory);

        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsConcatElement,
                                &sElement ) != IDE_SUCCESS);

        sElement->type             = aType;
        sElement->arithmeticOrList = sNode;
        sElement->arguments        = NULL;
        sElement->next             = NULL;

        if ( *aPrev == NULL )
        {
            aSFWGH->group = sElement;
            *aPrev = sElement;
        }
        else
        {
            ( *aPrev )->next = sElement;
            *aPrev = ( *aPrev )->next;
        }

        /* BUG-45143 partitioned table에 대한 grouping set 질의 수행시 오류
         * group by groupings sets ( ( a, b ) ) --> group by a, b
         * 위와 같이 변환시 List를 group by 구문으로 변환시  List의 a Node의 Next가 b
         * 로 지정되어 있는데 group by 로 나뉠경우 Next를 끊어 주어야한다.
         */
        if ( sPrevNode != NULL )
        {
            sPrevNode->node.next = NULL;
        }
        else
        {
            /* Nothing to do */
        }
        sPrevNode = sNode;
    }

    if ( *aPrev != NULL )
    {
        if ( ( *aPrev )->next == NULL )
        {
            ( *aPrev )->next = aNext;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
IDE_RC qmvGBGSTransform::modifyRollupCube( qcStatement       * aStatement,
                                           qmsSFWGH          * aSFWGH,
                                           qmsConcatElement ** aPrev,
                                           qmsConcatElement  * aElement,
                                           qmsConcatElement  * aNext )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Sets의 arguments로 온 Rollup / Cube 를 제거한 후
 *     Rollup / Cube가 가진 argument들을 Rollup / Cube 가 있었던 자리에 연결하고,
 *     Type을 QMS_GROUPBY_NULL 로 변경한다.
 *
 *     GROUP BY GROUPING SETS( i1, ROLLUP( i2, i3 ), i4 )
 *
 *     GROUP 1 : i1, i2(NULL), i3(NULL), i4(NULL)
 *     GROUP 2 : i1(NULL), ROLLUP( i2, i3 ), i4(NULL)
 *     GROUP 3 : i1(NULL), i2(NULL), i3(NULL), i4
 *
 *     * NULL 은 QMS_GROUPBY_NULL Type의 Group Element를 의미한다.
 *     
 * Implementation :
 *
 ***********************************************************************/
    qmsConcatElement * sElement;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::modifyRollupCube::__FT__" );

    if ( *aPrev == NULL )
    {
        aSFWGH->group = aElement->arguments;
    } 
    else
    {
        ( *aPrev )->next = aElement->arguments;
    }

    for ( sElement  = aElement->arguments;
          sElement != NULL;
          sElement  = sElement->next )
    {
        // ROLLUP, CUBE의 Argument로 GROUPING SETS, ROLLUP, CUBE가 온 경우
        IDE_TEST_RAISE( sElement->type != QMS_GROUPBY_NORMAL, 
                        ERR_NOR_SUPPORT_COMPLICATE_GROUP_EXT );

        if ( ( sElement->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK ) ==
             MTC_NODE_OPERATOR_LIST )
        {
            // LIST 일 경우 처리
            IDE_TEST( modifyList( aStatement,
                                  aSFWGH,
                                  aPrev,
                                  sElement,
                                  ( sElement->next == NULL ) ? aNext : sElement->next,
                                  QMS_GROUPBY_NULL )
                      != IDE_SUCCESS );
        }
        else
        {
            sElement->type = QMS_GROUPBY_NULL;
            *aPrev         = sElement;
        }
    }

    if ( *aPrev != NULL )
    {
        if ( ( *aPrev )->next == NULL )
        {
            ( *aPrev )->next = aNext;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOR_SUPPORT_COMPLICATE_GROUP_EXT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_SUPPORT_COMPLICATE_GROUP_EXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::modifyFrom( qcStatement         * aStatement,
                                     qmsQuerySet         * aQuerySet,
                                     qmvGBGSQuerySetList * aQuerySetList )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Sets Transform시에 Group 별로 생성 된
 *     Query Set들의 동일한 수행부분( SELECT * - FROM - WHERE - HIERARCHY )을
 *     SameViewReferencing 처리해서 중복 수행을 피한다.
 *
 *     < BEFORE TRANSFORMATION >
 *
 *     SELECT i1, i2, i3
 *       FROM t1
 *      WHERE i1 >= 0 
 *     GROUP BY GROUPING SETS( i1, i2, i3 );
 *
 *     < AFTER TRANSFORMATION >
 *
 *     SELECT *
 *       FROM (
 *                SELECT i1, NULL, NULL            *Same View Referencing*
 *                  FROM ( SELECT * FROM t1 WHERE i1 >= 0 )<------
 *                GROUP BY i1                                  | |
 *                UNION ALL                                    | |
 *                SELECT NULL, i2, NULL                        | |
 *                  FROM ( SELECT * FROM t1 WHERE i1 >= 0 )----- |
 *                GROUP BY i2                                    |
 *                UNION ALL                                      |
 *                SELECT NULL, NULL, i3                          |
 *                  FROM ( SELECT * FROM t1 WHERE i1 >= 0 )-------
 *                GROUP BY i3
 *            );
 *
 * Implementation :
 * 
 ***********************************************************************/
    qmvGBGSQuerySetList * sQuerySetList;
    qcStatement           sStatement;    
    qmsParseTree        * sParseTree;
    qmsTarget           * sTarget;
    qmsFrom             * sFrom;
    SInt                  sStartOffset = 0;
    SInt                  sSize        = 0;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::modifyFrom::__FT__" );

    // Partial Statement Parsing
    IDE_TEST( getPosForPartialQuerySet( aQuerySet,
                                        & sStartOffset,
                                        & sSize )
              != IDE_SUCCESS );
        
    for ( sQuerySetList  = aQuerySetList;
          sQuerySetList != NULL;
          sQuerySetList  = sQuerySetList->next )
    {
        // 하위 inLineView로 옮겨질 내용 초기화
        sQuerySetList->querySet->SFWGH->where         = NULL;
        sQuerySetList->querySet->SFWGH->hierarchy     = NULL;
        
        // 기타내용 초기화
        sQuerySetList->querySet->SFWGH->intoVariables = NULL;
        sQuerySetList->querySet->SFWGH->top           = NULL;

        QC_SET_STATEMENT( ( &sStatement ), aStatement, NULL );

        sStatement.myPlan->stmtText    = aQuerySet->startPos.stmtText;
        sStatement.myPlan->stmtTextLen = idlOS::strlen( aQuerySet->startPos.stmtText );
        
        IDE_TEST( qcpManager::parsePartialForQuerySet( &sStatement,
                                                       aQuerySet->startPos.stmtText,
                                                       sStartOffset,
                                                       sSize )
                  != IDE_SUCCESS );

        // FIT TEST
        IDU_FIT_POINT( "qmvGBGSTransform::modifyFrom::STRUCT_ALLOC1::ERR_MEM",
                       idERR_ABORT_InsufficientMemory);
        
        // Asterisk Target Node 생성
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsTarget,
                                &sTarget ) != IDE_SUCCESS);

        QMS_TARGET_INIT( sTarget );

        // FIT TEST
        IDU_FIT_POINT( "qmvGBGSTransform::modifyFrom::STRUCT_ALLOC2::ERR_MEM",
                       idERR_ABORT_InsufficientMemory);

        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qtcNode,
                                &sTarget->targetColumn ) != IDE_SUCCESS);

        QTC_NODE_INIT( sTarget->targetColumn );

        // make expression        
        IDE_TEST( qtc::makeTargetColumn( sTarget->targetColumn, 0, 0 ) != IDE_SUCCESS );
        
        sTarget->flag &= ~QMS_TARGET_ASTERISK_MASK;
        sTarget->flag |=  QMS_TARGET_ASTERISK_MASK;

        // 복제된 QuerySet의 초기화
        sParseTree = ( qmsParseTree* )sStatement.myPlan->parseTree;
        
        sParseTree->querySet->SFWGH->target = sTarget;        
        sParseTree->querySet->SFWGH->group  = NULL;
        sParseTree->querySet->SFWGH->having = NULL;
        sParseTree->querySet->SFWGH->top    = NULL;
        sParseTree->querySet->SFWGH->intoVariables = NULL;
        
        // Grouping Sets Transformed MASK For Bottom Mtr View
        sParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_GBGS_TRANSFORM_MASK;
        sParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_GBGS_TRANSFORM_BOTTOM_MTR;

        // Partial Statement Parsing으로 생성된 QuerySet을 inLineView로 연결 
        IDE_TEST( makeInlineView( aStatement,
                                  sQuerySetList->querySet->SFWGH,
                                  sParseTree->querySet ) 
                  != IDE_SUCCESS );
       
        // Middle View의 Outer SFWGH는 Simple View Merging하지 않는다.
        sFrom = sQuerySetList->querySet->SFWGH->from;        
        IDE_TEST( setNoMerge( sFrom ) != IDE_SUCCESS );
        
        // Change Grouping Sets Transformed MASK From Bottom View To Middle View
        sQuerySetList->querySet->SFWGH->lflag &= ~QMV_SFWGH_GBGS_TRANSFORM_MASK;
        sQuerySetList->querySet->SFWGH->lflag |= QMV_SFWGH_GBGS_TRANSFORM_MIDDLE;        

        // Bottom View 는 Simple View Merging 하지 않는다.
        for ( sFrom  = sParseTree->querySet->SFWGH->from;
              sFrom != NULL;
              sFrom  = sFrom->next )
        {
            IDE_TEST( setNoMerge( sFrom ) 
                      != IDE_SUCCESS );        
        }

        // Same View Referencing
        if ( sQuerySetList->querySet != aQuerySetList->querySet )
        {
            sQuerySetList->querySet->SFWGH->from->tableRef->sameViewRef = 
                aQuerySetList->querySet->SFWGH->from->tableRef;
        }
        else
        {
            /* Nothing to do */
        } 
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::unionQuerySets( qcStatement          * aStatement,
                                         qmvGBGSQuerySetList  * aQuerySetList,
                                         qmsQuerySet         ** aRet )
{
/*********************************************************************** *
 * Description :
 *     Query Set List의 Query Set 들을
 *     순차적인 UNION ALL로 연결한다.
 *
 *     GROUP BY GROUPING SETS( i1, i2, i3, i4 )
 *
 *     GROUP BY i1                         UNION ALL
 *     UNION ALL                            /      \
 *     GROUP BY i2                 GROUP BY i1   UNION ALL
 *     UNION ALL                                  /      \
 *     GROUP BY i3                        GROUP BY i2   UNION ALL
 *     UNION ALL                                         /      \
 *     GROUP BY i4                               GROUP BY i3  GROUP BY i4
 *     
 * Implementation :
 *
 ***********************************************************************/

    qmvGBGSQuerySetList * sQuerySetList;
    qmsQuerySet         * sUnionAllQuerySet;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::unionQuerySets::__FT__" );

    sQuerySetList = aQuerySetList;

    if ( sQuerySetList->next != NULL )
    {
        // FIT TEST
        IDU_FIT_POINT( "qmvGBGSTransform::unionQuerySets::STRUCT_ALLOC::ERR_MEM",
                       idERR_ABORT_InsufficientMemory);
        
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsQuerySet,
                                & sUnionAllQuerySet ) != IDE_SUCCESS);

        QCP_SET_INIT_QMS_QUERY_SET( sUnionAllQuerySet );

        sUnionAllQuerySet->setOp = QMS_UNION_ALL;
        sUnionAllQuerySet->left  = sQuerySetList->querySet;

        // Recursive Call For right of UNION ALL
        IDE_TEST( unionQuerySets( aStatement,
                                  sQuerySetList->next,
                                  & sUnionAllQuerySet->right ) 
                  != IDE_SUCCESS );
        
        *aRet = sUnionAllQuerySet;        
    }
    else
    {
        // UNION ALL 의 최하위 querySet
        *aRet = sQuerySetList->querySet;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
};

IDE_RC qmvGBGSTransform::makeInlineView( qcStatement * aStatement,
                                         qmsSFWGH    * aParentSFWGH,
                                         qmsQuerySet * aChildQuerySet )
{
/***********************************************************************
 *
 * Description :
 *     ParentSFWGH에 ChildQuerySet을 inLineView로 연결한다.
 *        
 * Implementation :        
 *
 ***********************************************************************/
    qcStatement    * sStatement;
    qmsParseTree   * sParseTree;
    qmsFrom        * sFrom;
    qmsTableRef    * sTableRef;
    qcNamePosition   sNullPosition;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::makeInlineView::__FT__" );

    IDE_TEST_RAISE( aChildQuerySet == NULL, ERR_INVALID_ARGS );

    SET_EMPTY_POSITION( sNullPosition );

    // FIT TEST
    IDU_FIT_POINT( "qmvGBGSTransform::makeInlineView::STRUCT_ALLOC1::ERR_MEM",
                   idERR_ABORT_InsufficientMemory);
    
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qcStatement,
                            &sStatement ) != IDE_SUCCESS);

    // FIT TEST
    IDU_FIT_POINT( "qmvGBGSTransform::makeInlineView::STRUCT_ALLOC2::ERR_MEM",
                   idERR_ABORT_InsufficientMemory);    

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsParseTree,
                            &sParseTree ) != IDE_SUCCESS);

    QC_SET_INIT_PARSE_TREE( sParseTree, sNullPosition );
    
    // FIT TEST
    IDU_FIT_POINT( "qmvGBGSTransform::makeInlineView::STRUCT_ALLOC3::ERR_MEM",
                   idERR_ABORT_InsufficientMemory);    

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsFrom,
                            &sFrom ) != IDE_SUCCESS);

    QCP_SET_INIT_QMS_FROM( sFrom );

    SET_POSITION( sFrom->fromPosition, aParentSFWGH->from->fromPosition );

    // FIT TEST
    IDU_FIT_POINT( "qmvGBGSTransform::makeInlineView::STRUCT_ALLOC4::ERR_MEM", 
                   idERR_ABORT_InsufficientMemory);    

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsTableRef,
                            &sTableRef ) != IDE_SUCCESS);

    QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

    sParseTree->withClause         = NULL;
    sParseTree->querySet           = aChildQuerySet;
    sParseTree->orderBy            = NULL;
    sParseTree->limit              = NULL;
    sParseTree->loopNode           = NULL;
    sParseTree->forUpdate          = NULL;
    sParseTree->queue              = NULL;
    sParseTree->isTransformed      = ID_FALSE;
    sParseTree->isSiblings         = ID_FALSE;
    sParseTree->isView             = ID_TRUE;
    sParseTree->isShardView        = ID_FALSE;
    sParseTree->common.currValSeqs = NULL;
    sParseTree->common.nextValSeqs = NULL;
    sParseTree->common.parse       = qmv::parseSelect;
    sParseTree->common.validate    = qmv::validateSelect;
    sParseTree->common.optimize    = qmo::optimizeSelect;
    sParseTree->common.execute     = qmx::executeSelect;
    
    QC_SET_STATEMENT( sStatement, aStatement, sParseTree );
    sStatement->myPlan->parseTree->stmtKind = QCI_STMT_SELECT;    

    aParentSFWGH->from                  = sFrom;
    aParentSFWGH->from->tableRef        = sTableRef;
    aParentSFWGH->from->tableRef->view  = sStatement;
    aParentSFWGH->from->tableRef->pivot = NULL;
    aParentSFWGH->from->next            = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::makeInlineView",
                                  "Invalid Arguments" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::modifyOrgSFWGH( qcStatement * aStatement,
                                         qmsSFWGH    * aSFWGH )
{
/***********************************************************************
 *
 * Description :
 *     Query Set 의 Target을 Asterisk Node로 변경 하고
 *     HINT, TOP, INTO 를 제외한 하위 View로 옮겨진 SFWGH의 내용을 초기화 한다.

 * Implementation :
 *
 ***********************************************************************/
    qmsTarget * sTarget;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::modifyOrgSFWGH::__FT__" );

    // FIT TEST
    IDU_FIT_POINT( "qmvGBGSTransform::modifyOrgSFWGH::STRUCT_ALLOC1::ERR_MEM",
                   idERR_ABORT_InsufficientMemory);
    
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsTarget,
                            &sTarget ) != IDE_SUCCESS);

    QMS_TARGET_INIT( sTarget );

    // FIT TEST
    IDU_FIT_POINT( "qmvGBGSTransform::modifyOrgSFWGH::STRUCT_ALLOC2::ERR_MEM",
                   idERR_ABORT_InsufficientMemory);    

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qtcNode,
                            &sTarget->targetColumn ) != IDE_SUCCESS);

    QTC_NODE_INIT( sTarget->targetColumn );

    IDE_TEST( qtc::makeTargetColumn( sTarget->targetColumn,
                                     0,
                                     0 ) != IDE_SUCCESS );
    
    sTarget->flag &= ~QMS_TARGET_ASTERISK_MASK;
    sTarget->flag |=  QMS_TARGET_ASTERISK_MASK;
    
    aSFWGH->target    = sTarget;
    aSFWGH->where     = NULL;
    aSFWGH->hierarchy = NULL;
    aSFWGH->group     = NULL;
    aSFWGH->having    = NULL;
    
    // Grouping Sets Transformed MASK 
    aSFWGH->lflag &= ~QMV_SFWGH_GBGS_TRANSFORM_MASK;
    aSFWGH->lflag |= QMV_SFWGH_GBGS_TRANSFORM_TOP;

    // Org SFWGH는 Simple View Merging 하지 않는다.
    IDE_TEST( setNoMerge( aSFWGH->from ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::modifyOrderBy( qcStatement         * aStatement,
                                        qmvGBGSQuerySetList * aQuerySetList )
{
/***********************************************************************
 *
 * Description :
 *     OrderBy 절의 처리를 위해 Transform을 수행한다.
 *
 *               << Before Transformation >>     
 *                  SELECT a.i1, a.i2 X
 *                    FROM t1 a
 *                GROUP BY GROUPING SETS( a.i1, a.i2 )
 *                ORDER BY a.i1 DESC, X ASC
 *                LIMIT 3;
 *
 *     위의 Query는 GBGSTransform 과정을 거쳐 아래와 같이 Validation되었을 것 이다.
 *
 *               << After Transformation >>
 *                  SELECT i1, X
 *                    FROM (
 *                          SELECT a.i1, NULL X
 *                            FROM ( SELECT * FROM t1 a )
 *                        GROUP BY a.i1              
 *                          SELECT NULL, a.i2 X         
 *                            FROM ( SELECT * FROM t1 a )
 *                        GROUP BY a.i2              
 *                         ) 
 *                ORDER BY a.i1 DESC, X ASC
 *                LIMIT 3;
 *                         ^ 여기서 GroupingSets Transform으로 생성 된 inLineView 때문에
 *                           TableAliasName인 a를 찾을 수 없다.
 *
 *     이에대한 해결책으로 다음과 같이 Transform 한다.
 * 
 *     SELECT i1, X // 3. expandAllTarget시 추가 된 ORDER BY의 Node를 제외한 나머지 TargetList 만 가져온다.
 *       FROM (
 *                SELECT a.i1, NULL X, a.i1, X // 1. ORDER BY절의 Node들을 Target List에 추가한다.
 *                  FROM ( SELECT * FROM t1 a )
 *                GROUP BY a.i1                  
 *                SELECT NULL, a.i2 X, a.i1, X    
 *                  FROM ( SELECT * FROM t1 a )
 *              ORDER BY 3 DESC, 4 ASC, // 2. ORDER BY와 Limit을 inLineView 안으로 집어넣고 
 *              LIMIT 3                       ORDER BY의 Node를 추가한 Targe Node의 Position으로 변경한다.     
 *            ); 
 *
 *            
 *    * Org Query에 Set Operator가 존재한다면 OrderBy의 Transform을 수행하지 않는다.
 *    
 * Implementation :  
 *
 ***********************************************************************/
    extern mtdModule      mtdSmallint;

    qmsParseTree        * sOrgParseTree;
    qmsParseTree        * sTransformedParseTree;

    qmvGBGSQuerySetList * sQuerySetList;
    qmsSortColumns      * sSortList;
    qmsTarget           * sLastTarget;
    qmsTarget           * sNewTarget;
    qmsTarget           * sTarget;
    qtcNode             * sLastNode = NULL;
    qtcNode             * sNewNode[2];    
    qtcNode             * sNode;
    UInt                  sTargetPosition = 1;
    SInt                  sStartOffset = 0;
    SInt                  sSize = 0;
    
    /* Target에 추가되는 OrderBy Node의 개수는 기존 Target Node의 개수와 합쳐 9999999999 개를 넘지 못한다. */
    SChar                 sValue[ QMV_GBGS_TRANSFORM_MAX_TARGET_COUNT + 1 ];

    const mtcColumn*      sColumn;
    qcStatement           sStatement; 
    qcNamePosition        sPosition;
    idBool                sIsIndicator;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::modifyOrderBy::__FT__" );

    SET_EMPTY_POSITION( sPosition );
    QC_SET_STATEMENT( ( &sStatement ), aStatement, NULL );

    /* sOrgParseTree -> SELECT TOP 3 *
     *                    FROM (
     * sTransformedParseTree->     SELECT t1.i1, NULL, NULL
     *                               FROM (
     *                                        SELECT * FROM t1, t2...
     *                                    )
     *                           GROUP BY t1.i1
     *                           UNION ALL
     *                           ...
     *                         )
     */                         
    sOrgParseTree = ( qmsParseTree * )aStatement->myPlan->parseTree;

    // Org Statement에 SetOerator가 있을 경우 OrderBy의 Transfrom은 수행하지 않는다. 
    if ( ( sOrgParseTree->orderBy != NULL ) && 
         ( sOrgParseTree->querySet->setOp == QMS_NONE ) )
    {
        // OrderBy의 Node를 Partial Parsing 해서 Target에 추가한다.
        IDE_TEST( getPosForOrderBy( sOrgParseTree->orderBy,
                                    &sStartOffset,
                                    &sSize ) != IDE_SUCCESS );
        
        // QuerySetList의 QuerySet들에 Order By의 Node를 Target으로 추가한다.
        for ( sQuerySetList  = aQuerySetList;
              sQuerySetList != NULL;
              sQuerySetList  = sQuerySetList->next )
        {
            // Target의 마지막 Node와 Count를 구한다.
            for ( sLastTarget        = sQuerySetList->querySet->SFWGH->target, sTargetPosition = 1;
                  sLastTarget->next != NULL;
                  sLastTarget        = sLastTarget->next )
            {
                sTargetPosition++;
            }
            
            sStatement.myPlan->stmtText = sQuerySetList->querySet->startPos.stmtText;
            sStatement.myPlan->stmtTextLen = idlOS::strlen( sQuerySetList->querySet->startPos.stmtText );
            
            IDE_TEST( qcpManager::parsePartialForOrderBy( &sStatement,
                                                          sQuerySetList->querySet->startPos.stmtText,
                                                          sStartOffset,
                                                          sSize )
                      != IDE_SUCCESS );

            sNewTarget = ( ( qmsParseTree* )sStatement.myPlan->parseTree )->querySet->target;
            
            sLastTarget->next = sNewTarget;
            sLastTarget->targetColumn->node.next = &sNewTarget->targetColumn->node;

            // 추가한 Target Node중 valueMode이면서 SmallInt형인 Node( ORDER BY의 Position Node )를 제거한다.
            for ( sTarget  = sNewTarget;
                  sTarget != NULL;
                  sTarget  = sTarget->next )
            {
                sNode = sTarget->targetColumn;
                
                // Indicator인지 확인한다.
                if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement), sNode ) == ID_TRUE )
                {
                    sColumn = QTC_STMT_COLUMN( aStatement, sNode );

                    if ( sColumn->module == &mtdSmallint )
                    {
                        sIsIndicator = ID_TRUE;
                    }
                    else
                    {
                        sIsIndicator = ID_FALSE;
                    }
                }
                else
                {
                    sIsIndicator = ID_FALSE;
                }

                if ( sIsIndicator == ID_TRUE )
                {
                    // Indicator일 경우 Target에서 제외                     
                    sLastTarget->next = sTarget->next; 
                }
                else
                {
                    sLastTarget = sTarget;
                    sNode->lflag &= ~QTC_NODE_GBGS_ORDER_BY_NODE_MASK;
                    sNode->lflag |= QTC_NODE_GBGS_ORDER_BY_NODE_TRUE;
                }
            }
        }

        // ORDER BY 의 Node를 Target에 추가한 Node를 가리키는 Position으로 변경한다.
        for ( sSortList  = sOrgParseTree->orderBy;  
              sSortList != NULL;
              sSortList  = sSortList->next )
        {
            sNode = sSortList->sortColumn;
            
            // Indicator인지 확인한다.
            if ( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement), sNode ) == ID_TRUE )
            {
                sColumn = QTC_STMT_COLUMN( aStatement, sNode );

                if ( sColumn->module == &mtdSmallint )
                {
                    sIsIndicator = ID_TRUE;
                }
                else
                {
                    sIsIndicator = ID_FALSE;
                }
            }
            else
            {
                sIsIndicator = ID_FALSE;
            }

            if ( sIsIndicator == ID_FALSE ) 
            {
                sTargetPosition++;

                // OrderBy Node를 Indicator로 변경한다.
                idlOS::snprintf( sValue,
                                 QMV_GBGS_TRANSFORM_MAX_TARGET_COUNT + 1,
                                 "%"ID_UINT32_FMT,
                                 sTargetPosition );            

                IDE_TEST( qtc::makeValue( aStatement,
                                          sNewNode,
                                          ( const UChar* )"SMALLINT",
                                          8,
                                          &sPosition,
                                          ( const UChar* )sValue,
                                          idlOS::strlen( sValue ),
                                          MTC_COLUMN_NORMAL_LITERAL ) 
                          != IDE_SUCCESS );
                
                sNewNode[0]->lflag &= ~QTC_NODE_GBGS_ORDER_BY_NODE_MASK;
                sNewNode[0]->lflag |= QTC_NODE_GBGS_ORDER_BY_NODE_TRUE;
                    
                sSortList->sortColumn = sNewNode[0];
            }
            else
            {
                /* Nothing to do */
            }
            
            // mtcNode의 연결            
            if ( sSortList == sOrgParseTree->orderBy )
            {
                // First Loop
                sLastNode = sSortList->sortColumn;
            }
            else
            {
                if ( sLastNode != NULL )
                {
                    sLastNode->node.next = &( sSortList->sortColumn->node );
                    sLastNode = ( qtcNode * )sLastNode->node.next;
                }
                else
                {
                    // Compile Warning 때문에 추가
                    IDE_RAISE( ERR_INVALID_POINTER );
                }                
            }
        }        

        sTransformedParseTree = ( qmsParseTree * )
            sOrgParseTree->querySet->SFWGH->from->tableRef->view->myPlan->parseTree;
    
        /* Order By 와 Limit을 View안으로 밀어 넣는다. */
        sTransformedParseTree->orderBy = sOrgParseTree->orderBy;
        sTransformedParseTree->limit   = sOrgParseTree->limit;
        sOrgParseTree->orderBy   = NULL;
        sOrgParseTree->limit     = NULL;                        
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_POINTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::modifyOrderBy",
                                  "Invalid Pointer" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::removeNullGroupElements( qmsSFWGH * aSFWGH )
{
/***********************************************************************
 *
 * Description :
 *     Group By의 Group Elements 중 QMS_GROUPBY_NULL Type의 
 *     Element를 제거한다.
 *     
 * Implementation :
 *
 ***********************************************************************/

    qmsConcatElement * sGroup     = NULL;
    qmsConcatElement * sPrevGroup = NULL;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::removeNullGroupElements::__FT__" );

    for ( sGroup  = aSFWGH->group;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        if ( sGroup->type == QMS_GROUPBY_NULL )
        {
            if ( sPrevGroup == NULL )
            {
                aSFWGH->group = sGroup->next;
            }
            else
            {
                sPrevGroup->next = sGroup->next;
            }
        }
        else
        {
            sPrevGroup = sGroup;
        }
    }

    IDE_TEST_RAISE( aSFWGH->group == NULL,
                    ERR_GROUP_IS_NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GROUP_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvGBGSTransform::removeNullGroupElements",
                                  "Group is null." ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::setNoMerge( qmsFrom * aFrom )
{

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::setNoMerge::__FT__" );

    if ( aFrom->joinType != QMS_NO_JOIN )
    {
        IDE_TEST( setNoMerge( aFrom->left  ) != IDE_SUCCESS );
        IDE_TEST( setNoMerge( aFrom->right ) != IDE_SUCCESS );        
    }
    else
    {
        aFrom->tableRef->noMergeHint = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmvGBGSTransform::makeNullNode( qcStatement * aStatement,
                                       qtcNode     * aExpression )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Sets Transform 으로 QMS_GROUPBY_NULL Type으로 변형된
 *     Group Expression 과 Equivalent 한 Target 또는 Having의 Expression을
 *     인자로 받아 해당 Type의 Null Value로 변형 시킨다.
 *     
 * Implementation :
 *
 ***********************************************************************/    
    mtcColumn       * sColumn;
    qtcNode         * sNullNode[2];    
    qcNamePosition    sPosition;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::makeNullNode::__FT__" );

    sColumn = QTC_STMT_COLUMN( aStatement, aExpression );
    SET_EMPTY_POSITION( sPosition );    

    // Null Value Node 생성
    IDE_TEST( qtc::makeValue( aStatement,
                              sNullNode,
                              ( const UChar* )sColumn->module->names->string,
                              sColumn->module->names->length,
                              &sPosition,
                              ( const UChar* )"NULL",
                              0, // 모든 Type이 Size가 0일 때 NULL 값으로 생성된다.
                              MTC_COLUMN_NORMAL_LITERAL )
              != IDE_SUCCESS );
    
    IDE_TEST( qtc::estimate( sNullNode[0],
                             QC_SHARED_TMPLATE( aStatement ),
                             aStatement,
                             NULL,
                             NULL,
                             NULL )
              != IDE_SUCCESS );

    aExpression->lflag           = sNullNode[0]->lflag;
    aExpression->node.table      = sNullNode[0]->node.table;
    aExpression->node.column     = sNullNode[0]->node.column;    
    aExpression->node.baseTable  = sNullNode[0]->node.table;
    aExpression->node.baseColumn = sNullNode[0]->node.column;
    aExpression->node.module     = &qtc::valueModule;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::makeNullOrPassNode( qcStatement      * aStatement,
                                             qtcNode          * aExpression,
                                             qmsConcatElement * aGroup,
                                             idBool             aMakePassNode )
{
/***********************************************************************
 *
 * Description :
 *    Grouping Sets Transform으로 변형된 QMS_GROUPBY_NULL Type의 Group Element에 대응되는
 *    Target 및 Having의 Expression을 NullNode로 변형 시키거나 다른PassNode 를바라보 게한다.
 *
 *        SELECT i1, i2, i3
 *          FROM t1
 *      GROUP BY GROUPING SETS ( i1, i2, i3 ), i1;
 *
 *      위의 Query에서 i1 처럼 GROUPING SETS의 인자로 온 Group Expression이
 *      GROUPING SETS 밖에서 Partial Group으로 사용 될 경우 생성되는 GROUPING은 다음과 같다.
 *
 *          GROUP 1 - i1(QMS_GROUPBY_NORMAL), i2(QMS_GROUPBY_NULL), i3(QMS_GROUPBY_NULL), i1(QMS_GROUPBY_NORMAL)
 *          GROUP 2 - i1(QMS_GROUPBY_NULL), i2(QMS_GROUPBY_NORMAL), i3(QMS_GROUPBY_NULL), i1(QMS_GROUPBY_NORMAL)
 *          GROUP 3 - i1(QMS_GROUPBY_NULL), i2(QMS_GROUPBY_NULL), i3(QMS_GROUPBY_NORMAL), i1(QMS_GROUPBY_NORMAL)
 *
 *      위의 예제처럼 Partial Group은 GROUPING SETS에 분배되어 모든 GROUPING 에 포함 되는데,
 *
 *      1. Target 또는 Having 의 Group Key가 되는 Expression과 Equivalent한
 *         QMS_GROUPBY_NORMAL Type의 Group이 있다면 해당 Group을 PassNode 로 바라보게 설정하고,
 *      
 *      2. Equivalent한 QMS_GROUPBY_NORMAL Type의 Expression이 없다면,
 *         QMS_GROUPBY_NULL Type의 Group만 Equivalent 하면 해당 Expression을 NullNode로 변형한다.
 *         
 * Implementation :
 *
 ***********************************************************************/    
    qmsConcatElement * sGroup;
    qmsConcatElement * sSubGroup;
    qtcNode          * sListElement;
    qtcNode          * sAlterPassNode = NULL;
    qtcNode          * sPassNode = NULL;
    idBool             sIsNormalGroup = ID_FALSE;
    idBool             sIsNormalGroupTmp = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::makeNullOrPassNode::__FT__" );

    for ( sGroup  = aGroup;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        // 모든 Group Element를 검사
        if ( sGroup->type == QMS_GROUPBY_NORMAL )
        {
            IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                   sGroup->arithmeticOrList,
                                                   aExpression,
                                                   &sIsNormalGroupTmp )
                      != IDE_SUCCESS );

            if ( sIsNormalGroupTmp == ID_TRUE )
            {
                // QMS_GROUPBY_NORMAL Type의 Group을 찾은경우
                // PassNode를 세팅하고 break
                sIsNormalGroup = ID_TRUE;                
                sAlterPassNode = sGroup->arithmeticOrList;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else if ( ( sGroup->type == QMS_GROUPBY_ROLLUP ) ||
                  ( sGroup->type == QMS_GROUPBY_CUBE   ) )
        {
            for ( sSubGroup  = sGroup->arguments;
                  sSubGroup != NULL ;
                  sSubGroup = sSubGroup->next )
            {
                IDE_DASSERT( sSubGroup->arithmeticOrList != NULL );

                /* *********************************
                 * Concatenated Expression
                 * *********************************/
                if ( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    
                    for ( sListElement  = ( qtcNode* )sSubGroup->arithmeticOrList->node.arguments;
                          sListElement != NULL;  
                          sListElement  = ( qtcNode* )sListElement->node.next )
                    {
                        IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                               sListElement,
                                                               aExpression,
                                                               &sIsNormalGroupTmp )
                                  != IDE_SUCCESS );
                        
                        if ( sIsNormalGroupTmp == ID_TRUE )
                        {
                            sIsNormalGroup = ID_TRUE;
                            sAlterPassNode = sListElement;
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                }
                else
                {
                    IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                           sSubGroup->arithmeticOrList,
                                                           aExpression,
                                                           &sIsNormalGroupTmp )
                              != IDE_SUCCESS );
                    
                    if ( sIsNormalGroupTmp == ID_TRUE )
                    {
                        sIsNormalGroup = ID_TRUE;
                        sAlterPassNode  = sSubGroup->arithmeticOrList;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                if ( sIsNormalGroupTmp == ID_TRUE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sIsNormalGroup != ID_TRUE )
    {
        // Make Null Node        
        IDE_TEST( makeNullNode( aStatement, aExpression ) != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( sAlterPassNode != NULL );

        if ( aMakePassNode == ID_TRUE )
        {
            IDE_TEST( qtc::makePassNode( aStatement,
                                         aExpression,
                                         sAlterPassNode,
                                         &sPassNode )
                      != IDE_SUCCESS );

            IDE_DASSERT( aExpression == sPassNode );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvGBGSTransform::doTransform( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet )
{
    qmsConcatElement    * sGroupingSets = NULL;
    qmsConcatElement    * sPrevGroup = NULL;
    qmvGBGSQuerySetList * sQuerySetList = NULL;
    qmsQuerySet         * sUnionQuerySet = NULL;

    IDU_FIT_POINT_FATAL( "qmvGBGSTransform::doTransform::__FT__" );

    // GroupBy 절에서 Grouping Sets를 찾는다.    
    IDE_TEST( searchGroupingSets( aQuerySet, &sGroupingSets, &sPrevGroup ) 
              != IDE_SUCCESS );

    if ( sGroupingSets != NULL )
    {
        // Grouping Sets의 Argument가 하나 라면
        // Normal GroupBy와 동일 함으로, Grouping Sets만 제거한다.         
        if ( sGroupingSets->arguments->next == NULL )
        {
            // FIT TEST
            IDU_FIT_POINT( "qmvGBGSTransform::doTransform::STRUCT_ALLOC::ERR_MEM",
                           idERR_ABORT_InsufficientMemory);
            
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                    qmvGBGSQuerySetList,
                                    &sQuerySetList ) != IDE_SUCCESS);

            sQuerySetList->querySet = aQuerySet;
            sQuerySetList->next     = NULL;

            IDE_TEST( modifyGroupBy( aStatement,
                                     sQuerySetList )
                      != IDE_SUCCESS );
        }
        else
        {
            // 1. Query Statement의 Partial Parsing을 통해
            //    생성 될 Group의 수 만큼 QuerySet을 복제한다.
            IDE_TEST( copyPartialQuerySet( aStatement,
                                           sGroupingSets,
                                           aQuerySet,
                                           &sQuerySetList )
                      != IDE_SUCCESS );
            
            // 2. 복제 된 각 QuerySet의 GroupBy 자료구조를 변형한다.
            IDE_TEST( modifyGroupBy( aStatement, 
                                     sQuerySetList ) 
                      != IDE_SUCCESS );
            
            // ( 3. Same View Refrencing 처리를 위해
            //      복제 된 각 QuerySet별로 InLineView를 생성한다. )
            //
            // "GROUPING_SETS_MTR" Hint 가 있을 경우에만 수행
            if ( aQuerySet->SFWGH->hints != NULL )
            {   
                if ( aQuerySet->SFWGH->hints->GBGSOptViewMtr == ID_TRUE )
                {
                    IDE_TEST( modifyFrom( aStatement, 
                                          aQuerySet, 
                                          sQuerySetList ) 
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }            
            
            // 4. 복제 된 QuerySet들을 UNION ALL로 연결한다.
            IDE_TEST( unionQuerySets( aStatement,
                                      sQuerySetList,
                                      &sUnionQuerySet )
                      != IDE_SUCCESS );

            // 5. UNION ALL로 연결 한 QuerySet들을 원본 QuerySet의
            //    InlineView로 만든다.
            IDE_TEST( makeInlineView( aStatement,
                                      aQuerySet->SFWGH,
                                      sUnionQuerySet )
                      != IDE_SUCCESS );
            
            // 6. 원본 QuerySet의 Target을 Asterisk Node로 변형하고,
            //    HINT, DISTINCT, TOP, INTO, FROM 을 제외한
            //    나머지 내용을 초기화 한다.
            IDE_TEST( modifyOrgSFWGH( aStatement,
                                      aQuerySet->SFWGH )
                      != IDE_SUCCESS );

            // 7. OrderBy의 Node를 Target으로 복제하고
            //    OrderBy의 Node들을 복제된 Target Node의
            //    Position 으로 변경한다.
            IDE_TEST( modifyOrderBy( aStatement,
                                     sQuerySetList )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
};
