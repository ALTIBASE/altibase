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
 * $Id: qcphy.y 89925 2021-02-03 04:40:48Z ahra.cho $
 **********************************************************************/

%pure_parser

%{
#include <idl.h>
#include <mtcDef.h>
#include <qc.h>
#include <qtc.h>
#include <qcpManager.h>
#include <qmsParseTree.h>
#include <qmo.h>

#define PARAM       ((qcphx*)param)
#define STATEMENT   (PARAM->mStatement)
#define QTEXT       (PARAM->mStmtText)
#define MEMORY      (STATEMENT->myPlan->qmpMem)
#define HINTS       (PARAM->mHints)

#define SET_POSITION_IN_HINTS(_DESTINATION_, _SOURCE_)                  \
{                                                                       \
    (_DESTINATION_).stmtText = STATEMENT->myPlan->stmtText;         \
    (_DESTINATION_).offset   = (_SOURCE_).offset + PARAM->mTextOffset;  \
    (_DESTINATION_).size     = (_SOURCE_).size;                         \
}

/*BUGBUG_NT*/
#if defined(VC_WIN32)
# include <malloc.h>
//# define alloca _alloca
# define strcasecmp _stricmp
#endif
/*BUGBUG_NT ADD*/

#if defined(SYMBIAN)
void * alloca(unsigned int);
#endif
%}

%union {
    qmsHints              * hints;
    qcNamePosition          position;
    UInt                    uIntVal;
    qmoTableAccessType    * tableAccessType;
    qmsJoinMethodHints    * joinMethodHints;
    qmsTableAccessHints   * tableAccessHints;
    qmsHintTables         * hintTables;
    qmsHintIndices        * hintIndices;
}

%token E_ERROR

%token TR_ASC
%token TR_DESC

%token TR_FULL
%token TR_NO

%token TR_PARALLEL // PROJ-1665 Parallel Hint 추가 
%token TR_NOPARALLEL /* PROJ-1071 */
%token TR_NO_PARALLEL

%token TO_INDEX
%token TO_NO_INDEX
%token TO_INDEX_ASC
%token TO_INDEX_DESC

%token TI_NONQUOTED_IDENTIFIER
%token TI_QUOTED_IDENTIFIER
%token TL_INTEGER

%token TS_OPENING_PARENTHESIS
%token TS_CLOSING_PARENTHESIS
%token TS_COMMA
%token TS_PERIOD

%token TA_BUCKET
%token TA_COUNT
%token TA_PARTIAL /* BUG-39306 */

%token TA_ALTI_PARTIAL_FULL_SCAN /* BUG-48288 */
%token TA_ALTI_INDEX
%token TA_ALTI_INDEX_ASC
%token TA_ALTI_INDEX_DESC
%token TA_ALTI_NO_INDEX
%token TA_ALTI_PARALLEL

%{
#include <idl.h>
#include <ideErrorMgr.h>
#include <iduMemory.h>
#include <qcuError.h>
#include "qcphx.h"

#define YYPARSE_PARAM param
#define YYLEX_PARAM   param

extern int      qcphlex(YYSTYPE * lvalp, void * param );

static void     qcpherror(const char *);
%}


%%
hints_comment
    : all_hints
    {
        STATEMENT->myPlan->hints = $<hints>1;
        HINTS = $<hints>1;
    }
    ;

all_hints
    : all_hints hint
    {
        qmsJoinMethodHints * sJoinMethod;
        qmsTableAccessHints* sTableAccess;

        if ($<hints>1 == NULL && $<hints>2 == NULL)
        {
            $<hints>$ = NULL;
        }
        else if ($<hints>1 != NULL && $<hints>2 == NULL)
        {
            $<hints>$ = $<hints>1;
        }
        else if ($<hints>1 == NULL && $<hints>2 != NULL)
        {
            $<hints>$ = $<hints>2;
        }
        else
        {
            $<hints>$ = $<hints>1;

            if ($<hints>$->optGoalType == QMO_OPT_GOAL_TYPE_UNKNOWN)
            {
                $<hints>$->optGoalType = $<hints>2->optGoalType;
            }

            if ($<hints>$->firstRowsN == QMS_NOT_DEFINED_FIRST_ROWS_N)
            {
                $<hints>$->firstRowsN = $<hints>2->firstRowsN;
            }

            if ( $<hints>$->normalType == QMO_NORMAL_TYPE_NOT_DEFINED )
            {
                $<hints>$->normalType = $<hints>2->normalType;
            }

            if ( $<hints>$->materializeType == QMO_MATERIALIZE_TYPE_NOT_DEFINED )
            {
                $<hints>$->materializeType = $<hints>2->materializeType;
            }            

            if ($<hints>$->joinOrderType == QMO_JOIN_ORDER_TYPE_OPTIMIZED)
            {
                $<hints>$->joinOrderType = $<hints>2->joinOrderType;
            }

            if ($<hints>$->interResultType == QMO_INTER_RESULT_TYPE_NOT_DEFINED)
            {
                $<hints>$->interResultType = $<hints>2->interResultType;
            }

            if ($<hints>$->groupMethodType == QMO_GROUP_METHOD_TYPE_NOT_DEFINED)
            {
                $<hints>$->groupMethodType = $<hints>2->groupMethodType;
            }

            if ($<hints>$->distinctMethodType ==
                                        QMO_DISTINCT_METHOD_TYPE_NOT_DEFINED)
            {
                $<hints>$->distinctMethodType = $<hints>2->distinctMethodType;
            }

            if ($<hints>$->viewOptHint == NULL)
            {
                $<hints>$->viewOptHint = $<hints>2->viewOptHint;
            }
            else
            {
                if ($<hints>2->viewOptHint != NULL)
                {
                    $<hints>2->viewOptHint->next = $<hints>$->viewOptHint;
                    $<hints>$->viewOptHint = $<hints>2->viewOptHint;
                }
            }

            if  ($<hints>$->pushPredHint == NULL )
            {
                $<hints>$->pushPredHint = $<hints>2->pushPredHint;
            }
            else
            {
                if ($<hints>2->pushPredHint != NULL)
                {
                    $<hints>2->pushPredHint->next = $<hints>$->pushPredHint;
                    $<hints>$->pushPredHint = $<hints>2->pushPredHint;
                }
            }

            if ($<hints>$->joinMethod == NULL)
            {
                $<hints>$->joinMethod = $<hints>2->joinMethod;
            }
            else
            {
                if ($<hints>2->joinMethod != NULL)
                {
                    // To Fix PR-10496
                    // 사용자가 기술한 순서대로 Hint를 배치하여야 함.
                    // $<hints>2->joinMethod->next = $<hints>$->joinMethod;
                    // $<hints>$->joinMethod = $<hints>2->joinMethod;

                    for ( sJoinMethod = $<hints>1->joinMethod;
                          sJoinMethod->next != NULL;
                          sJoinMethod = sJoinMethod->next ) ;

                    sJoinMethod->next = $<hints>2->joinMethod;
                }
            }

            if ($<hints>$->tableAccess == NULL)
            {
                $<hints>$->tableAccess = $<hints>2->tableAccess;
            }
            else
            {
                if ($<hints>2->tableAccess != NULL)
                {
                    // To Fix PR-10496
                    // 사용자가 기술한 순서대로 Hint를 배치하여야 함.
                    // $<hints>2->tableAccess->next = $<hints>$->tableAccess;
                    // $<hints>$->tableAccess = $<hints>2->tableAccess;

                    for ( sTableAccess = $<hints>1->tableAccess;
                          sTableAccess->next != NULL;
                          sTableAccess = sTableAccess->next ) ;

                    sTableAccess->next = $<hints>2->tableAccess;
                }
            }

            if ( $<hints>$->leading == NULL )
            {
                $<hints>$->leading = $<hints>2->leading;
            }

            if ($<hints>$->hashBucketCnt == QMS_NOT_DEFINED_BUCKET_CNT)
            {
                $<hints>$->hashBucketCnt = $<hints>2->hashBucketCnt;
            }

            if ($<hints>$->groupBucketCnt == QMS_NOT_DEFINED_BUCKET_CNT)
            {
                $<hints>$->groupBucketCnt = $<hints>2->groupBucketCnt;
            }

            if ($<hints>$->setBucketCnt == QMS_NOT_DEFINED_BUCKET_CNT)
            {
                $<hints>$->setBucketCnt = $<hints>2->setBucketCnt;
            }

            // PROJ-1566
            if (( $<hints>$->directPathInsHintFlag & SMI_INSERT_METHOD_MASK )
                == SMI_INSERT_METHOD_NORMAL )
            {
                $<hints>$->directPathInsHintFlag =
                    $<hints>2->directPathInsHintFlag;
            }

            // PROJ-1665, PROJ-1071
            if ($<hints>$->parallelHint == NULL )
            {
                $<hints>$->parallelHint = $<hints>2->parallelHint;
            }
            else
            {
                if ($<hints>2->parallelHint != NULL)
                {
                    $<hints>2->parallelHint->next = $<hints>$->parallelHint;
                    $<hints>$->parallelHint = $<hints>2->parallelHint;
                }
            }       
            
            //PROJ-1583 large geometry
            if ($<hints>$->STObjBufSize == QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE)
            {
                $<hints>$->STObjBufSize = $<hints>2->STObjBufSize;
            }

            // PROJ-1404
            if ($<hints>$->transitivePredType == QMO_TRANSITIVE_PRED_TYPE_ENABLE)
            {
                $<hints>$->transitivePredType = $<hints>2->transitivePredType;
            }

            // PROJ-1413
            if  ($<hints>$->noMergeHint == NULL )
            {
                $<hints>$->noMergeHint = $<hints>2->noMergeHint;
            }
            else
            {
                if ($<hints>2->noMergeHint != NULL)
                {
                    $<hints>2->noMergeHint->next = $<hints>$->noMergeHint;
                    $<hints>$->noMergeHint = $<hints>2->noMergeHint;
                }
            }

            // PROJ-1436
            if ($<hints>$->noPlanCache == ID_FALSE)
            {
                $<hints>$->noPlanCache = $<hints>2->noPlanCache;
            }

            // PROJ-1436
            if ($<hints>$->keepPlan == ID_FALSE)
            {
                $<hints>$->keepPlan = $<hints>2->keepPlan;
            }

            if ($<hints>$->viewOptMtrType == QMO_VIEW_OPT_MATERIALIZE_NOT_DEFINED)
            {
                $<hints>$->viewOptMtrType = $<hints>2->viewOptMtrType;
            }

            if ($<hints>$->refreshMView == ID_FALSE)
            {
                $<hints>$->refreshMView = $<hints>2->refreshMView;
            }

            /* BUG-39600 Grouping Sets View Materialization Hint */
            if ($<hints>$->GBGSOptViewMtr == ID_FALSE)
            {
                $<hints>$->GBGSOptViewMtr = $<hints>2->GBGSOptViewMtr;
            }            

            if ($<hints>$->withoutRetry == ID_FALSE)
            {
                $<hints>$->withoutRetry = $<hints>2->withoutRetry;
            }

            // PROJ-2385 Separate Inverse Join Method Hint
            if ($<hints>$->inverseJoinOption == QMO_INVERSE_JOIN_METHOD_ALLOWED)
            {
                $<hints>$->inverseJoinOption = $<hints>2->inverseJoinOption;
            }
            
            // PROJ-2385 Unnesting Hint
            if ($<hints>$->semiJoinMethod == QMO_SEMI_JOIN_METHOD_NOT_DEFINED)
            {
                $<hints>$->semiJoinMethod = $<hints>2->semiJoinMethod;
            }

            if ($<hints>$->antiJoinMethod == QMO_ANTI_JOIN_METHOD_NOT_DEFINED)
            {
                $<hints>$->antiJoinMethod = $<hints>2->antiJoinMethod;
            }

            if ($<hints>$->subqueryUnnestType == QMO_SUBQUERY_UNNEST_TYPE_NOT_DEFINED)
            {
                $<hints>$->subqueryUnnestType = $<hints>2->subqueryUnnestType;
            }

            // PROJ-2462 Result Cache
            if ( $<hints>$->resultCacheType == QMO_RESULT_CACHE_NOT_DEFINED )
            {
                $<hints>$->resultCacheType = $<hints>2->resultCacheType;
            }
            // PROJ-2462 Result Cache
            if ( $<hints>$->topResultCache == ID_FALSE )
            {
                $<hints>$->topResultCache = $<hints>2->topResultCache;
            }

            /* BUG-39399 remove search key preserved table */
            if  ( $<hints>$->keyPreservedHint == NULL )
            {
                $<hints>$->keyPreservedHint = $<hints>2->keyPreservedHint;
            }
            else
            {
                if ( $<hints>2->keyPreservedHint != NULL )
                {
                    $<hints>2->keyPreservedHint->next = $<hints>$->keyPreservedHint;
                    $<hints>$->keyPreservedHint = $<hints>2->keyPreservedHint;
                }
            }

            // BUG-41615 simple query hint
            if ( $<hints>$->execFastHint == QMS_EXEC_FAST_NONE )
            {
                $<hints>$->execFastHint = $<hints>2->execFastHint;
            }

            // BUG-43493
            if ( $<hints>$->delayedExec == QMS_DELAY_NONE )
            {
                $<hints>$->delayedExec = $<hints>2->delayedExec;
            }

            // PROJ-2673
            if ( $<hints>$->disableInsTrigger == ID_TRUE )
            {
                $<hints>$->disableInsTrigger = $<hints>2->disableInsTrigger;
            }

            // BUG-46137
            if ( $<hints>$->planCacheKeep == ID_FALSE )
            {
                $<hints>$->planCacheKeep = $<hints>2->planCacheKeep;
            }

            /* PROJ-2632 */
            if ( $<hints>$->mSerialFilter == QMS_SERIAL_FILTER_NONE )
            {
                $<hints>$->mSerialFilter = $<hints>2->mSerialFilter;
            }

            /* BUG-48348 */
            if ( $<hints>$->partialCSE == ID_FALSE )
            {
                $<hints>$->partialCSE = $<hints>2->partialCSE;
            }
        }
    }
    | hint
    {
        $<hints>$ = $<hints>1;
    }
    ;

hint
    : hint_method
    {
        $<hints>$ = $<hints>1;
    }
    | hint_no_parameter
    {
        $<hints>$ = $<hints>1;
    }
    | hint_bucket_count
    {
        $<hints>$ = $<hints>1;
    }
    | hint_with_parameter
    {
        $<hints>$ = $<hints>1;
    }
    ;

// BUG-9020
TI_IDENTIFIER
    : TI_NONQUOTED_IDENTIFIER
    | TI_QUOTED_IDENTIFIER
      {
          $<position>$.stmtText = QTEXT;
          $<position>$.offset   = $<position>1.offset + 1;
          $<position>$.size     = $<position>1.size - 2;
      }
    ;

hint_method
    : TR_FULL
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
        // BUG-42389 full hint support
        /* FULL ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_FULLACCESS_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TR_FULL TI_NONQUOTED_IDENTIFIER
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* FULL SCAN ( table ) */
    {
        if (idlOS::strMatch(
              "SCAN", 4,
              QTEXT+$<position>2.offset, $<position>2.size) == 0)
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
            $<hints>$->tableAccess->accessType =
                        QMO_ACCESS_METHOD_TYPE_FULLACCESS_SCAN;
            $<hints>$->tableAccess->table      = $<hintTables>4;
            $<hints>$->tableAccess->indices    = NULL;
            $<hints>$->tableAccess->count      = 1;
            $<hints>$->tableAccess->id         = 1;
            $<hints>$->tableAccess->next       = NULL;
        }
        else
        { // syntax error

            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>2);

            IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
            YYABORT;
        }
    }
    | TA_PARTIAL TR_FULL TI_NONQUOTED_IDENTIFIER
        TS_OPENING_PARENTHESIS
        table_reference comma_integer_reference comma_integer_reference
        TS_CLOSING_PARENTHESIS
      /* PARTIAL FULL SCAN ( table, count, id ) */
    {
        if (idlOS::strMatch(
              "SCAN", 4,
              QTEXT+$<position>3.offset, $<position>3.size) == 0)
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
            $<hints>$->tableAccess->accessType =
                QMO_ACCESS_METHOD_TYPE_FULLACCESS_SCAN;
            $<hints>$->tableAccess->table      = $<hintTables>5;
            $<hints>$->tableAccess->indices    = NULL;
            $<hints>$->tableAccess->count      = $<uIntVal>6;
            $<hints>$->tableAccess->id         = $<uIntVal>7;
            $<hints>$->tableAccess->next       = NULL;
        }
        else
        { // syntax error

            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

            IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
            YYABORT;
        }
    }
    | TA_ALTI_PARTIAL_FULL_SCAN
        TS_OPENING_PARENTHESIS table_reference comma_integer_reference comma_integer_reference TS_CLOSING_PARENTHESIS
        /* BUG-48288 */
    {
        /* ALTI_PARTIAL_FULL_SCAN ( table, count, id ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType = QMO_ACCESS_METHOD_TYPE_FULLACCESS_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = $<uIntVal>4;
        $<hints>$->tableAccess->id         = $<uIntVal>5;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TR_NO TO_INDEX
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference
        TS_CLOSING_PARENTHESIS
      /* NO INDEX ( table, index, index, ... ) */
      /* NO INDEX ( table  index  index  ... ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>4;
        $<hints>$->tableAccess->indices    = $<hintIndices>5;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_NO_INDEX
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference
        TS_CLOSING_PARENTHESIS
      /* NO_INDEX ( table, index, index, ... ) */
      /* NO_INDEX ( table  index  index  ... ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TR_NO TO_INDEX
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* NO INDEX ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>4;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_NO_INDEX
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* NO_INDEX ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* INDEX ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEXACCESS_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX_ASC
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* INDEX_ASC ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX_DESC
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* INDEX_DESC ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX TR_ASC
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* INDEX ASC ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>4;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX TR_DESC
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
      /* INDEX DESC ( table ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>4;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference
        TS_CLOSING_PARENTHESIS
      /* INDEX ( table, index, index, ... ) */
      /* INDEX ( table  index  index  ... ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEXACCESS_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX TR_ASC
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference
        TS_CLOSING_PARENTHESIS
      /* INDEX ASC ( table, index, index, ... ) */
      /* INDEX ASC ( table  index  index  ... ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>4;
        $<hints>$->tableAccess->indices    = $<hintIndices>5;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX TR_DESC
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference
        TS_CLOSING_PARENTHESIS
      /* INDEX DESC ( table, index, index, ... ) */
      /* INDEX DESC ( table  index  index  ... ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>4;
        $<hints>$->tableAccess->indices    = $<hintIndices>5;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX_ASC
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference
        TS_CLOSING_PARENTHESIS
      /* INDEX_ASC ( table, index, index, ... ) */
      /* INDEX_ASC ( table  index  index  ... ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TO_INDEX_DESC
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference
        TS_CLOSING_PARENTHESIS
      /* INDEX_DESC ( table, index, index, ... ) */
      /* INDEX_DESC ( table  index  index  ... ) */
    {
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    /* BUG-48288 */
    | TA_ALTI_INDEX 
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_INDEX ( table ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEXACCESS_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TA_ALTI_INDEX 
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_INDEX ( table, index, index, ... ) */
        /* ALTI_INDEX ( table  index  index  ... ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType = QMO_ACCESS_METHOD_TYPE_INDEXACCESS_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TA_ALTI_NO_INDEX 
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_NO_INDEX ( table ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TA_ALTI_NO_INDEX 
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_NO_INDEX ( table, index, index, ... ) */
        /* ALTI_NO_INDEX ( table  index  index  ... ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType = QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TA_ALTI_INDEX_ASC 
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_INDEX_ASC ( table ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TA_ALTI_INDEX_ASC 
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_INDEX_ASC ( table, index, index, ... ) */
        /* ALTI_INDEX_ASC ( table  index  index  ... ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType = QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TA_ALTI_INDEX_DESC
        TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_INDEX_DESC ( table ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType =
                    QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = NULL;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }
    | TA_ALTI_INDEX_DESC
        TS_OPENING_PARENTHESIS table_reference comma_indices_reference TS_CLOSING_PARENTHESIS
    {
        /* ALTI_INDEX_DESC ( table, index, index, ... ) */
        /* ALTI_INDEX_DESC ( table  index  index  ... ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
        $<hints>$->tableAccess->accessType = QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN;
        $<hints>$->tableAccess->table      = $<hintTables>3;
        $<hints>$->tableAccess->indices    = $<hintIndices>4;
        $<hints>$->tableAccess->count      = 1;
        $<hints>$->tableAccess->id         = 1;
        $<hints>$->tableAccess->next       = NULL;
    }

    /*
     * PROJ-1071 Parallel Query
     *
     * PARALLEL(table_name, parallel_degree)
     * PARALLEL(table_name parallel_degree)
     * PARALLEL(parallel_degree)
     * NOPARALLEL(table_name)
     */
    | TR_PARALLEL
      TS_OPENING_PARENTHESIS
      table_reference comma_integer_reference
      TS_CLOSING_PARENTHESIS
    {
        /* PARALLEL ( table_name, parallel_degree ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);
        
        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = $<hintTables>3;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = $<uIntVal>4;
    }
    | TR_PARALLEL TS_OPENING_PARENTHESIS TL_INTEGER TS_CLOSING_PARENTHESIS
    {
        /* PROJ-1071 Parallel Query */
        /* PARALLEL( parallel_degree ) */

        SLong sParallelDegree;

        if (qtc::getBigint(QTEXT,
                           &sParallelDegree,
                           &$<position>3)
            != IDE_SUCCESS)
        {
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

            YYABORT;
        }

        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);
        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = NULL;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = (UInt)sParallelDegree;
    }
    /* BUG-48288 */
    | TA_ALTI_PARALLEL
      TS_OPENING_PARENTHESIS
          table_reference comma_integer_reference
      TS_CLOSING_PARENTHESIS
    {
        /* ALTI_PARALLEL ( table_name, parallel_degree ) */
        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);
        
        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = $<hintTables>3;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = $<uIntVal>4;
    }
    /* BUG-48288 */
    | TA_ALTI_PARALLEL TS_OPENING_PARENTHESIS TL_INTEGER TS_CLOSING_PARENTHESIS
    {
        /* ALTI_PARALLEL( parallel_degree ) */
        SLong sParallelDegree;

        if (qtc::getBigint(QTEXT,
                           &sParallelDegree,
                           &$<position>3)
            != IDE_SUCCESS)
        {
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

            YYABORT;
        }

        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);
        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = NULL;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = (UInt)sParallelDegree;
    }
    | TR_NOPARALLEL TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
    {
        /* NOPARALLEL ( table_name ) */

        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = $<hintTables>3;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = 1;
    }
    | TR_NO_PARALLEL TS_OPENING_PARENTHESIS table_reference TS_CLOSING_PARENTHESIS
    {
        /* NO_PARALLEL ( table_name ) */

        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = $<hintTables>3;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = 1;
    }
    | TR_NOPARALLEL
    {
        /* NOPARALLEL */

        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = NULL;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = 1;
    }
    | TR_NO_PARALLEL
    {
        /* NO_PARALLEL */

        QCP_STRUCT_ALLOC($<hints>$, qmsHints);
        QCP_SET_INIT_HINTS($<hints>$);

        QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

        $<hints>$->parallelHint->table          = NULL;
        $<hints>$->parallelHint->next           = NULL;
        $<hints>$->parallelHint->parallelDegree = 1;
    }
    | TI_NONQUOTED_IDENTIFIER
      TS_OPENING_PARENTHESIS
          table_reference_commalist
          opt_temp_table_count
      TS_CLOSING_PARENTHESIS
      /* [ALTI_]USE_NL ( table[,] table )                                  */
      /* [ALTI_]USE_FULL_NL ( table[,] table )                             */
      /* [ALTI_]USE_FULL_STORE_NL ( table[,] table )                       */
      /* [ALTI_]USE_INDEX_NL ( table[,] table )                            */
      /* [ALTI_]USE_ANTI ( table[,] table )                                */
      /* [ALTI_]USE_HASH ( table[,] table )                                */
      /* [ALTI_]USE_ONE_PASS_HASH ( table[,] table )                       */
      /* [ALTI_]USE_TWO_PASS_HASH ( table [,]table [[,]temp_table_count] ) */
      /* [ALTI_]USE_INVERSE_HASH ( table[,] table )                        */
      /* [ALTI_]USE_SORT ( table[,] table )                                */
      /* [ALTI_]USE_ONE_PASS_SORT ( table[,] table )                       */
      /* [ALTI_]USE_TWO_PASS_SORT ( table[,] table )                       */
      /* [ALTI_]USE_MERGE ( table[,] table )                               */
      /* [ALTI_]NO_USE_NL ( table[,] table )                               */
      /* [ALTI_]NO_USE_HASH ( table[,] table )                             */
      /* [ALTI_]NO_USE_SORT ( table[,] table )                             */
      /* [ALTI_]NO_USE_MERGE ( table[,] table )                            */
      /* [ALTI_]NO_PUSH_SELECT_VIEW ( view )                               */
      /* [ALTI_]PUSH_SELECT_VIEW ( view )                                  */
      /* [ALTI_]PUSH_PRED( view )                                          */ // PROJ-1495
      /* [ALTI_]NO_MERGE( view )                                           */ // PROJ-1413
      /* [ALTI_]KEY_PRESERVED_TABLE( table[,] table ... )                  */
      /* [ALTI_]LEADING( table[,] table ... )                              */ // BUG-42447
      /* ALTI_FULL ( table )                                               */
      /* ALTI_FULL_SCAN ( table )                                          */
      /* ALTI_NO_PARALLEL ( table )                                        */
      /* ALTI_NOPARALLEL  ( table )                                        */
    {
        qmsHintTables * sHintTable;
        SInt            sHintTableCnt = 0;

        for (sHintTable = $<hintTables>3;
             sHintTable != NULL;
             sHintTable = sHintTable->next)
        {
            sHintTableCnt++;
        }

        if ( ( idlOS::strMatch( "ALTI_NO_PUSH_SELECT_VIEW", 24, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
             ( idlOS::strMatch(      "NO_PUSH_SELECT_VIEW", 19, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( sHintTableCnt != 1 ||
                 $<uIntVal>4 != QMS_NOT_DEFINED_TEMP_TABLE_CNT )
            { // syntax error
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->viewOptHint, qmsViewOptHints);
            $<hints>$->viewOptHint->viewOptType = QMO_VIEW_OPT_TYPE_VMTR;
            $<hints>$->viewOptHint->table       = $<hintTables>3;
            $<hints>$->viewOptHint->next        = NULL;
        }
        else if ( ( idlOS::strMatch( "ALTI_PUSH_SELECT_VIEW", 21, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "PUSH_SELECT_VIEW", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( sHintTableCnt != 1 ||
                 $<uIntVal>4 != QMS_NOT_DEFINED_TEMP_TABLE_CNT )
            { // syntax error
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->viewOptHint, qmsViewOptHints);
            $<hints>$->viewOptHint->viewOptType = QMO_VIEW_OPT_TYPE_PUSH;
            $<hints>$->viewOptHint->table       = $<hintTables>3;
            $<hints>$->viewOptHint->next        = NULL;
        }
        else if ( ( idlOS::strMatch( "ALTI_PUSH_PRED", 14, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "PUSH_PRED",  9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( sHintTableCnt != 1 ||
                 $<uIntVal>4 != QMS_NOT_DEFINED_TEMP_TABLE_CNT )
            { // syntax error
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->pushPredHint, qmsPushPredHints);
            $<hints>$->pushPredHint->table       = $<hintTables>3;
            $<hints>$->pushPredHint->next        = NULL;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_NL", 11, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_NL",  6, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_NL;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_FULL_NL", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_FULL_NL", 11, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_FULL_NL;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_FULL_STORE_NL", 22, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_FULL_STORE_NL", 17, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_FULL_STORE_NL;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_INDEX_NL", 17, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_INDEX_NL", 12, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_INDEX;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_ANTI", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_ANTI",  8, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_ANTI;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_HASH", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_HASH",  8, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_HASH;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_ONE_PASS_HASH", 22, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_ONE_PASS_HASH", 17, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_ONE_PASS_HASH;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_TWO_PASS_HASH", 22, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_TWO_PASS_HASH", 17, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_TWO_PASS_HASH;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
            $<hints>$->joinMethod->tempTableCnt = $<uIntVal>4;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_INVERSE_HASH", 21, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_INVERSE_HASH", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_INVERSE_HASH;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_SORT", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_SORT",  8, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_SORT;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_ONE_PASS_SORT", 22, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_ONE_PASS_SORT", 17, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_ONE_PASS_SORT;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_TWO_PASS_SORT", 22, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_TWO_PASS_SORT", 17, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_TWO_PASS_SORT;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_USE_MERGE", 14, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_MERGE",  9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_MERGE;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_USE_NL", 14, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_USE_NL",  9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_NL;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
            $<hints>$->joinMethod->isNoUse    = ID_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_USE_HASH", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_USE_HASH", 11, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_HASH;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
            $<hints>$->joinMethod->isNoUse    = ID_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_USE_SORT", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_USE_SORT", 11, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_SORT;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
            $<hints>$->joinMethod->isNoUse    = ID_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_USE_MERGE", 17, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_USE_MERGE", 12, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->joinMethod, qmsJoinMethodHints);
            QCP_SET_INIT_JOIN_METHOD_HINTS($<hints>$->joinMethod);
            $<hints>$->joinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            $<hints>$->joinMethod->flag |= QMO_JOIN_METHOD_MERGE;
            $<hints>$->joinMethod->joinTables = $<hintTables>3;
            $<hints>$->joinMethod->isNoUse    = ID_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_MERGE", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_MERGE",  8, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( sHintTableCnt != 1 ||
                 $<uIntVal>4 != QMS_NOT_DEFINED_TEMP_TABLE_CNT )
            { // syntax error
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->noMergeHint, qmsNoMergeHints);
            $<hints>$->noMergeHint->table = $<hintTables>3;
            $<hints>$->noMergeHint->next  = NULL;
        }
        /* BUG-39399 remove search key preserved table */
        else if ( ( idlOS::strMatch( "ALTI_KEY_PRESERVED_TABLE", 24, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "KEY_PRESERVED_TABLE", 19, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( sHintTableCnt < 1 )
            { // syntax error
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode( qpERR_ABORT_QCP_SYNTAX, "" ) );
                YYABORT;
            }

            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            QCP_STRUCT_ALLOC( $<hints>$->keyPreservedHint, qmsKeyPreservedHints );
            $<hints>$->keyPreservedHint->table = $<hintTables>3;
            $<hints>$->keyPreservedHint->next  = NULL;
        }
        // BUG-42447 leading hint support
        else if ( ( idlOS::strMatch( "ALTI_LEADING", 12, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "LEADING",  7, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( sHintTableCnt < 1 )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }

            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->leading, qmsLeadingHints);
            QCP_SET_INIT_LEADING_HINTS($<hints>$->leading);
            $<hints>$->leading->mLeadingTables = $<hintTables>3;

            // ORDERED 힌트를 같이 적용한다.
            $<hints>$->joinOrderType = QMO_JOIN_ORDER_TYPE_ORDERED;
        }
        /* BUG-48288 */
        /* ALTI_FULL ( table ) */
        /* ALTI_FULL_SCAN ( table ) */
        else if ( ( idlOS::strMatch( "ALTI_FULL",       9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch( "ALTI_FULL_SCAN", 14, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( ( sHintTableCnt != 1 ) || ( $<uIntVal>4 != QMS_NOT_DEFINED_TEMP_TABLE_CNT ) )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>1);
                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->tableAccess, qmsTableAccessHints);
            $<hints>$->tableAccess->accessType = QMO_ACCESS_METHOD_TYPE_FULLACCESS_SCAN;
            $<hints>$->tableAccess->table      = $<hintTables>3;
            $<hints>$->tableAccess->indices    = NULL;
            $<hints>$->tableAccess->count      = 1;
            $<hints>$->tableAccess->id         = 1;
            $<hints>$->tableAccess->next       = NULL;
        }
        /* ALTI_NO_PARALLEL ( table ) */
        /* ALTI_NOPARALLEL  ( table ) */
        else if ( ( idlOS::strMatch( "ALTI_NO_PARALLEL", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch( "ALTI_NOPARALLEL",  15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            if ( ( sHintTableCnt != 1 ) || ( $<uIntVal>4 != QMS_NOT_DEFINED_TEMP_TABLE_CNT ) )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>1);
                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

            $<hints>$->parallelHint->table          = $<hintTables>3;
            $<hints>$->parallelHint->next           = NULL;
            $<hints>$->parallelHint->parallelDegree = 1;
        }
        else
        { // syntax error
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>1);

            IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
            YYABORT;
        }
    }
    ;

table_reference_commalist
    : table_reference_commalist TS_COMMA table_reference
      {
          qmsHintTables * sLast;

          for (sLast = $<hintTables>1;
               sLast->next != NULL;
               sLast = sLast->next) ;

          sLast->next = $<hintTables>3;

          $<hintTables>$ = $<hintTables>1;
      }
    | table_reference_commalist table_reference
      {
          qmsHintTables * sLast;

          for (sLast = $<hintTables>1;
               sLast->next != NULL;
               sLast = sLast->next) ;

          sLast->next = $<hintTables>2;

          $<hintTables>$ = $<hintTables>1;
      }
    | table_reference
      {
          $<hintTables>$ = $<hintTables>1;
      }
    ;

table_reference
    : TI_IDENTIFIER TS_PERIOD TI_IDENTIFIER
    {
        QCP_STRUCT_ALLOC($<hintTables>$, qmsHintTables);
        SET_POSITION_IN_HINTS($<hintTables>$->userName, $<position>1);
        SET_POSITION_IN_HINTS($<hintTables>$->tableName, $<position>3);
        $<hintTables>$->table     = NULL;
        $<hintTables>$->next      = NULL;
    }
    | TI_IDENTIFIER
    {
        QCP_STRUCT_ALLOC($<hintTables>$, qmsHintTables);
        SET_EMPTY_POSITION($<hintTables>$->userName);
        SET_POSITION_IN_HINTS($<hintTables>$->tableName, $<position>1);
        $<hintTables>$->table     = NULL;
        $<hintTables>$->next      = NULL;
    }
    ;

comma_indices_reference
    : comma_indices_reference comma_index_reference
    {
        qmsHintIndices * sHintIndices;

        sHintIndices = $<hintIndices>1;
        while (sHintIndices->next != NULL)
        {
            sHintIndices = sHintIndices->next;
        }
        sHintIndices->next = $<hintIndices>2;

        $<hintIndices>$ = $<hintIndices>1;
    }
    | comma_index_reference
    {
        $<hintIndices>$ = $<hintIndices>1;
    }
    ;

comma_index_reference
    : TS_COMMA TI_IDENTIFIER
    {
        QCP_STRUCT_ALLOC($<hintIndices>$, qmsHintIndices);
        SET_POSITION_IN_HINTS($<hintIndices>$->indexName, $<position>2);
        $<hintIndices>$->indexPtr = NULL;
        $<hintIndices>$->next     = NULL;
    }
    | TI_IDENTIFIER
    {
        QCP_STRUCT_ALLOC($<hintIndices>$, qmsHintIndices);
        SET_POSITION_IN_HINTS($<hintIndices>$->indexName, $<position>1);
        $<hintIndices>$->indexPtr = NULL;
        $<hintIndices>$->next     = NULL;
    }
    ;

comma_integer_reference
    : TS_COMMA TL_INTEGER
    {
        SLong  sValue;

        if( qtc::getBigint( QTEXT, &sValue, &$<position>2 ) != IDE_SUCCESS )
        {
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>2);

            YYABORT;
        }

        $<uIntVal>$ = sValue;
    }
    | TL_INTEGER
    {
        SLong  sValue;

        if( qtc::getBigint( QTEXT, &sValue, &$<position>1 ) != IDE_SUCCESS )
        {
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>1);

            YYABORT;
        }

        $<uIntVal>$ = sValue;
    }
    ;

opt_temp_table_count
    : /* empty */
    {
        $<uIntVal>$ = QMS_NOT_DEFINED_TEMP_TABLE_CNT;
    }
    | comma_integer_reference
    {
        $<uIntVal>$ = $<uIntVal>1;
    }
    ;

hint_no_parameter
    : TI_NONQUOTED_IDENTIFIER
      /* [ALTI_]COST                       */
      /* [ALTI_]RULE                       */
      /* [ALTI_]PUSH_PROJECTION            */
      /* [ALTI_]NO_PUSH_PROJECTION         */
      /* [ALTI_]CNF                        */
      /* [ALTI_]NO_EXPAND                  */
      /* [ALTI_]DNF                        */
      /* [ALTI_]USE_CONCAT                 */
      /* [ALTI_]ORDERED                    */
      /* [ALTI_]TEMP_TBS_MEMORY            */
      /* [ALTI_]TEMP_TBS_DISK              */
      /* [ALTI_]GROUP_HASH                 */
      /* [ALTI_]GROUP_SORT                 */
      /* [ALTI_]DISTINCT_HASH              */
      /* [ALTI_]DISTINCT_SORT              */
      /* [ALTI_]APPEND                     */
      /* [ALTI_]NO_TRANSITIVE_PRED         */
      /* [ALTI_]NO_PLAN_CACHE              */
      /* [ALTI_]KEEP_PLAN                  */
      /* [ALTI_]MATERIALIZE                */
      /* [ALTI_]NO_MATERIALIZE             */
      /* [ALTI_]REFRESH_MVIEW              */
      /* [ALTI_]GROUPING_SETS_MATERIALIZE  */
      /* [ALTI_]WITHOUT_RETRY              */
      /* [ALTI_]UNNEST                     */
      /* [ALTI_]NO_UNNEST                  */
      /* [ALTI_]NL_SJ                      */
      /* [ALTI_]HASH_SJ                    */
      /* [ALTI_]MERGE_SJ                   */
      /* [ALTI_]SORT_SJ                    */
      /* [ALTI_]NL_AJ                      */
      /* [ALTI_]HASH_AJ                    */
      /* [ALTI_]MERGE_AJ                   */
      /* [ALTI_]SORT_AJ                    */
      /* [ALTI_]INVERSE_JOIN               */
      /* [ALTI_]NO_INVERSE_JOIN            */
      /* [ALTI_]EXEC_FAST                  */
      /* [ALTI_]NO_EXEC_FAST               */
      /* [ALTI_]HIGH_PRECISION             */
      /* [ALTI_]HIGH_PERFORMANCE           */
      /* [ALTI_]HIGH_PERFORMANCE_LEVEL1    */
      /* [ALTI_]HIGH_PERFORMANCE_LEVEL2    */
      /* [ALTI_]RESULT_CACHE               */
      /* [ALTI_]NO_RESULT_CACHE            */
      /* [ALTI_]TOP_RESULT_CACHE           */
      /* [ALTI_]DELAY                      */
      /* [ALTI_]NO_DELAY                   */
      /* [ALTI_]NO_MERGE                   */
      /* [ALTI_]DISABLE_INSERT_TRIGGER     */
      /* [ALTI_]PLAN_CACHE_KEEP            */
      /* [ALTI_]SERIAL_FILTER              */
      /* [ALTI_]NO_SERIAL_FILTER           */
      /* ALTI_NO_PARALLEL                  */
      /* ALTI_NOPARALLEL                   */
      /* [ALTI_]PARTIAL_CSE */
    {
        if ( ( idlOS::strMatch( "ALTI_COST", 9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
             ( idlOS::strMatch(      "COST", 4, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->optGoalType = QMO_OPT_GOAL_TYPE_ALL_ROWS;
        }
        else if ( ( idlOS::strMatch( "ALTI_RULE", 9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "RULE", 4, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->optGoalType = QMO_OPT_GOAL_TYPE_RULE;
        }
        else if ( ( idlOS::strMatch( "ALTI_PUSH_PROJECTION", 20, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "PUSH_PROJECTION", 15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->materializeType = QMO_MATERIALIZE_TYPE_VALUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_PUSH_PROJECTION", 23, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_PUSH_PROJECTION", 18, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->materializeType = QMO_MATERIALIZE_TYPE_RID;
        }
        else if ( ( idlOS::strMatch( "ALTI_CNF", 8, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "CNF", 3, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch( "ALTI_NO_EXPAND", 14, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_EXPAND",  9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->normalType = QMO_NORMAL_TYPE_CNF;
        }
        else if ( ( idlOS::strMatch( "ALTI_DNF", 8, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "DNF", 3, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch( "ALTI_USE_CONCAT", 15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "USE_CONCAT", 10, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->normalType = QMO_NORMAL_TYPE_DNF;
        }
        else if ( ( idlOS::strMatch( "ALTI_ORDERED", 12, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "ORDERED",  7, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->joinOrderType = QMO_JOIN_ORDER_TYPE_ORDERED;
        }
        else if ( ( idlOS::strMatch( "ALTI_TEMP_TBS_MEMORY", 20, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "TEMP_TBS_MEMORY", 15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->interResultType = QMO_INTER_RESULT_TYPE_MEMORY;
        }
        else if ( ( idlOS::strMatch( "ALTI_TEMP_TBS_DISK", 18, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "TEMP_TBS_DISK", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->interResultType = QMO_INTER_RESULT_TYPE_DISK;
        }
        else if ( ( idlOS::strMatch( "ALTI_GROUP_HASH", 15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "GROUP_HASH", 10, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->groupMethodType = QMO_GROUP_METHOD_TYPE_HASH;
        }
        else if ( ( idlOS::strMatch( "ALTI_GROUP_SORT", 15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "GROUP_SORT", 10, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->groupMethodType = QMO_GROUP_METHOD_TYPE_SORT;
        }
        else if ( ( idlOS::strMatch( "ALTI_DISTINCT_HASH", 18, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "DISTINCT_HASH", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->distinctMethodType = QMO_DISTINCT_METHOD_TYPE_HASH;
        }
        else if ( ( idlOS::strMatch( "ALTI_DISTINCT_SORT", 18, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "DISTINCT_SORT", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->distinctMethodType = QMO_DISTINCT_METHOD_TYPE_SORT;
        }
        else if ( ( idlOS::strMatch( "ALTI_APPEND", 11, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "APPEND",  6, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->directPathInsHintFlag &= ~SMI_INSERT_METHOD_MASK;
            $<hints>$->directPathInsHintFlag |= SMI_INSERT_METHOD_APPEND;
        }
        // PROJ-1404
        else if ( ( idlOS::strMatch( "ALTI_NO_TRANSITIVE_PRED", 23, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_TRANSITIVE_PRED", 18, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->transitivePredType = QMO_TRANSITIVE_PRED_TYPE_DISABLE;
        }
        // PROJ-1436
        else if ( ( idlOS::strMatch( "ALTI_NO_PLAN_CACHE", 18, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_PLAN_CACHE", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->noPlanCache = ID_TRUE;
        }
        // PROJ-1436
        else if ( ( idlOS::strMatch( "ALTI_KEEP_PLAN", 14, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "KEEP_PLAN",  9, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->keepPlan = ID_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_MATERIALIZE", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "MATERIALIZE", 11, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->viewOptMtrType = QMO_VIEW_OPT_MATERIALIZE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_MATERIALIZE", 19, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_MATERIALIZE", 14, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->viewOptMtrType = QMO_VIEW_OPT_NO_MATERIALIZE;
        }
        /* PROJ-2211 Materialized View */
        else if ( ( idlOS::strMatch( "ALTI_REFRESH_MVIEW", 18, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "REFRESH_MVIEW", 13, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->refreshMView = ID_TRUE;
        }
        /* BUG-39600 Grouping Sets View Materialization Hint */
        else if ( ( idlOS::strMatch( "ALTI_GROUPING_SETS_MATERIALIZE", 30, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "GROUPING_SETS_MATERIALIZE", 25, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->GBGSOptViewMtr = ID_TRUE;
        }        
        // PROJ-1784 DML Without Retry
        else if ( ( idlOS::strMatch( "ALTI_WITHOUT_RETRY", 18, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "WITHOUT_RETRY", 13, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->withoutRetry = ID_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_UNNEST", 11, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "UNNEST",  6, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->subqueryUnnestType = QMO_SUBQUERY_UNNEST_TYPE_UNNEST;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_UNNEST", 14, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_UNNEST",  9, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->subqueryUnnestType = QMO_SUBQUERY_UNNEST_TYPE_NO_UNNEST;
        }
        else if ( ( idlOS::strMatch( "ALTI_NL_SJ", 10, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NL_SJ",  5, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->semiJoinMethod = QMO_SEMI_JOIN_METHOD_NL;
        }
        else if ( ( idlOS::strMatch( "ALTI_HASH_SJ", 12, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "HASH_SJ",  7, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->semiJoinMethod = QMO_SEMI_JOIN_METHOD_HASH;
        }
        else if ( ( idlOS::strMatch( "ALTI_MERGE_SJ", 13, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "MERGE_SJ",  8, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->semiJoinMethod = QMO_SEMI_JOIN_METHOD_MERGE;
        }
        else if ( ( idlOS::strMatch( "ALTI_SORT_SJ", 12, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "SORT_SJ",  7, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->semiJoinMethod = QMO_SEMI_JOIN_METHOD_SORT;
        }
        else if ( ( idlOS::strMatch( "ALTI_NL_AJ", 10, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NL_AJ",  5, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->antiJoinMethod = QMO_ANTI_JOIN_METHOD_NL;
        }
        else if ( ( idlOS::strMatch( "ALTI_HASH_AJ", 12, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "HASH_AJ",  7, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->antiJoinMethod = QMO_ANTI_JOIN_METHOD_HASH;
        }
        else if ( ( idlOS::strMatch( "ALTI_MERGE_AJ", 13, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "MERGE_AJ",  8, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->antiJoinMethod = QMO_ANTI_JOIN_METHOD_MERGE;
        }
        else if ( ( idlOS::strMatch( "ALTI_SORT_AJ", 12, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "SORT_AJ",  7, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->antiJoinMethod = QMO_ANTI_JOIN_METHOD_SORT;
        }
        else if ( ( idlOS::strMatch( "ALTI_INVERSE_JOIN", 17, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "INVERSE_JOIN", 12, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->inverseJoinOption = QMO_INVERSE_JOIN_METHOD_ONLY;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_INVERSE_JOIN", 20, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_INVERSE_JOIN", 15, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->inverseJoinOption = QMO_INVERSE_JOIN_METHOD_DENIED;
        }
        else if ( ( idlOS::strMatch( "ALTI_EXEC_FAST", 14, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "EXEC_FAST",  9, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->execFastHint = QMS_EXEC_FAST_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_EXEC_FAST", 17, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_EXEC_FAST", 12, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->execFastHint = QMS_EXEC_FAST_FALSE;
        }
        else if ( ( idlOS::strMatch( "ALTI_HIGH_PRECISION", 19, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "HIGH_PRECISION", 14, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );
            
            // BUG-41944
            // high precision hint는 parsing 즉시 동작해야 하는 hint로
            // template에 직접 등록한다.
            QC_SHARED_TMPLATE(STATEMENT)->tmplate.arithmeticOpMode =
                MTC_ARITHMETIC_OPERATION_PRECISION;
        }
        else if ( ( idlOS::strMatch( "ALTI_HIGH_PERFORMANCE", 21, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "HIGH_PERFORMANCE", 16, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch( "ALTI_HIGH_PERFORMANCE_LEVEL1", 28, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "HIGH_PERFORMANCE_LEVEL1", 23, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );
            
            // BUG-41944
            QC_SHARED_TMPLATE(STATEMENT)->tmplate.arithmeticOpMode =
                MTC_ARITHMETIC_OPERATION_PERFORMANCE_LEVEL1;
        }
        else if ( ( idlOS::strMatch( "ALTI_HIGH_PERFORMANCE_LEVEL2", 28, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "HIGH_PERFORMANCE_LEVEL2", 23, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );
            
            // BUG-41944
            QC_SHARED_TMPLATE(STATEMENT)->tmplate.arithmeticOpMode =
                MTC_ARITHMETIC_OPERATION_PERFORMANCE_LEVEL2;
        }
        else if ( ( idlOS::strMatch( "ALTI_RESULT_CACHE", 17, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "RESULT_CACHE", 12, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->resultCacheType = QMO_RESULT_CACHE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_RESULT_CACHE", 20, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_RESULT_CACHE", 15, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->resultCacheType = QMO_RESULT_CACHE_NO;
        }
        else if ( ( idlOS::strMatch( "ALTI_TOP_RESULT_CACHE", 21, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "TOP_RESULT_CACHE", 16, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->topResultCache = ID_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_DELAY", 10, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "DELAY",  5, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->delayedExec = QMS_DELAY_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_DELAY", 13, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_DELAY",  8, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->delayedExec = QMS_DELAY_FALSE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_MERGE", 13, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_MERGE",  8, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            QCP_STRUCT_ALLOC($<hints>$->noMergeHint, qmsNoMergeHints);
            $<hints>$->noMergeHint->table = NULL;
            $<hints>$->noMergeHint->next  = NULL;
        }
        else if ( ( idlOS::strMatch( "ALTI_DISABLE_INSERT_TRIGGER", 27, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "DISABLE_INSERT_TRIGGER", 22, QTEXT + $<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->disableInsTrigger = ID_TRUE;
        }
        // BUG-46137
        else if ( ( idlOS::strMatch( "ALTI_PLAN_CACHE_KEEP", 20, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "PLAN_CACHE_KEEP", 15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->planCacheKeep = ID_TRUE;
        }
        /* PROJ-2632 */
        else if ( ( idlOS::strMatch( "ALTI_SERIAL_FILTER", 18, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "SERIAL_FILTER", 13, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->mSerialFilter = QMS_SERIAL_FILTER_TRUE;
        }
        else if ( ( idlOS::strMatch( "ALTI_NO_SERIAL_FILTER", 21, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "NO_SERIAL_FILTER", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->mSerialFilter = QMS_SERIAL_FILTER_FALSE;
        }
        /* BUG-48348 */
        else if ( ( idlOS::strMatch( "ALTI_PARTIAL_CSE", 16, QTEXT+$<position>1.offset, $<position>1.size) == 0 ) ||
                  ( idlOS::strMatch(      "PARTIAL_CSE", 11, QTEXT+$<position>1.offset, $<position>1.size) == 0 ) )
        {
            QCP_STRUCT_ALLOC( $<hints>$, qmsHints );
            QCP_SET_INIT_HINTS( $<hints>$ );

            $<hints>$->partialCSE = ID_TRUE;
        }
        /* BUG-48288 */
        /* ALTI_NO_PARALLEL */
        /* ALTI_NOPARALLEL  */
        else if ( ( idlOS::strMatch( "ALTI_NO_PARALLEL", 16, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch( "ALTI_NOPARALLEL",  15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            QCP_STRUCT_ALLOC($<hints>$->parallelHint, qmsParallelHints);

            $<hints>$->parallelHint->table          = NULL;
            $<hints>$->parallelHint->next           = NULL;
            $<hints>$->parallelHint->parallelDegree = 1;
        }
        else
        { // syntax error

            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>1);

            IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
            YYABORT;
        }
    }
    ;

hint_bucket_count
    : TI_NONQUOTED_IDENTIFIER TA_BUCKET TA_COUNT
        TS_OPENING_PARENTHESIS TL_INTEGER TS_CLOSING_PARENTHESIS
        /* HASH BUCKET COUNT ( n ) */
        /* GROUP BUCKET COUNT ( n ) */
        /* SET BUCKET COUNT ( n ) */
    {
        SLong       sBucketCnt;

        if( qtc::getBigint( QTEXT, &sBucketCnt, &$<position>5 ) != IDE_SUCCESS )
        {
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>5);

            YYABORT;
        }

        if ( sBucketCnt <= 0 || sBucketCnt > QMS_MAX_BUCKET_CNT )
        {
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>5);

            YYABORT;
        }

        if (idlOS::strMatch(
              "HASH", 4,
              QTEXT+$<position>1.offset, $<position>1.size) == 0)
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->hashBucketCnt = sBucketCnt;
        }
        else if (idlOS::strMatch(
              "GROUP", 5,
              QTEXT+$<position>1.offset, $<position>1.size) == 0)
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->groupBucketCnt = sBucketCnt;
        }
        else if (idlOS::strMatch(
              "SET", 3,
              QTEXT+$<position>1.offset, $<position>1.size) == 0)
        {
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->setBucketCnt = sBucketCnt;
        }
        else
        { // syntax error

            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>1);

            IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
            YYABORT;
        }
    }
    ;

hint_with_parameter
    : TI_NONQUOTED_IDENTIFIER
        TS_OPENING_PARENTHESIS TL_INTEGER TS_CLOSING_PARENTHESIS
        /* [ALTI_]ST_OBJECT_BUFFER_SIZE   ( n ) */
        /* [ALTI_]ST_ALLOW_INVALID_OBJECT ( n ) */
        /* [ALTI_]FIRST_ROWS ( n )              */
        /* ALTI_HASH_BUCKET_COUNT  ( n )        */
        /* ALTI_GROUP_BUCKET_COUNT ( n )        */
        /* ALTI_SET_BUCKET_COUNT   ( n )        */
    {
        if ( ( idlOS::strMatch( "ALTI_ST_OBJECT_BUFFER_SIZE", 26, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
             ( idlOS::strMatch(      "ST_OBJECT_BUFFER_SIZE", 21, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            SLong sObjBufSize;

            if( qtc::getBigint( QTEXT, &sObjBufSize, &$<position>3 ) != IDE_SUCCESS )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }

            if ( (sObjBufSize < QMS_MIN_ST_OBJECT_BUFFER_SIZE ) || 
                 (sObjBufSize > QMS_MAX_ST_OBJECT_BUFFER_SIZE ) )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
                YYABORT;
            }

            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->STObjBufSize = (UInt)sObjBufSize;
        }

        //BUG-24361 ST_ALLOW_HINT
        else if ( ( idlOS::strMatch( "ALTI_ST_ALLOW_INVALID_OBJECT", 28, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "ST_ALLOW_INVALID_OBJECT", 23, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            // 호환성을 위해 남겨둔다.
            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);
        }
        else if ( ( idlOS::strMatch( "ALTI_FIRST_ROWS", 15, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) ||
                  ( idlOS::strMatch(      "FIRST_ROWS", 10, QTEXT+$<position>1.offset, $<position>1.size ) == 0 ) )
        {
            SLong       sFirstRowsN;

            if( qtc::getBigint( QTEXT, &sFirstRowsN, &$<position>3 ) != IDE_SUCCESS )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                YYABORT;
            }

            if ( (sFirstRowsN < QMS_MIN_FIRST_ROWS_N) ||
                 (sFirstRowsN > QMS_MAX_FIRST_ROWS_N) )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);

                YYABORT;
            }

            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->optGoalType = QMO_OPT_GOAL_TYPE_FIRST_ROWS_N;
            $<hints>$->firstRowsN  = sFirstRowsN;
        }
        /* BUG-48288 */
        /* ALTI_HASH_BUCKET_COUNT ( n ) */
        else if ( idlOS::strMatch( "ALTI_HASH_BUCKET_COUNT", 22, QTEXT+$<position>1.offset, $<position>1.size ) == 0 )
        {
            SLong       sBucketCnt;

            if ( qtc::getBigint( QTEXT, &sBucketCnt, &$<position>3 ) != IDE_SUCCESS )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);
                YYABORT;
            }

            if ( sBucketCnt <= 0 || sBucketCnt > QMS_MAX_BUCKET_CNT )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);
                YYABORT;
            }

            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->hashBucketCnt = sBucketCnt;
        }
        /* ALTI_GROUP_BUCKET_COUNT ( n ) */
        else if ( idlOS::strMatch( "ALTI_GROUP_BUCKET_COUNT", 23, QTEXT+$<position>1.offset, $<position>1.size ) == 0 )
        {
            SLong       sBucketCnt;

            if ( qtc::getBigint( QTEXT, &sBucketCnt, &$<position>3 ) != IDE_SUCCESS )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);
                YYABORT;
            }

            if ( sBucketCnt <= 0 || sBucketCnt > QMS_MAX_BUCKET_CNT )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);
                YYABORT;
            }

            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->groupBucketCnt = sBucketCnt;
        }
        /* ALTI_SET_BUCKET_COUNT ( n ) */
        else if ( idlOS::strMatch( "ALTI_SET_BUCKET_COUNT", 21, QTEXT+$<position>1.offset, $<position>1.size ) == 0 )
        {
            SLong       sBucketCnt;

            if ( qtc::getBigint( QTEXT, &sBucketCnt, &$<position>3 ) != IDE_SUCCESS )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);
                YYABORT;
            }

            if ( sBucketCnt <= 0 || sBucketCnt > QMS_MAX_BUCKET_CNT )
            {
                SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>3);
                YYABORT;
            }

            QCP_STRUCT_ALLOC($<hints>$, qmsHints);
            QCP_SET_INIT_HINTS($<hints>$);

            $<hints>$->setBucketCnt = sBucketCnt;
        }
        else
        {
            SET_POSITION_IN_HINTS(PARAM->mPosition, $<position>1);

            IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
            YYABORT;
        }
    }
    ;
%%

# undef yyFlexLexer
# define yyFlexLexer qcphFlexLexer

# include <FlexLexer.h>

#include "qcphl.h"

void qcpherror(const char* /*msg*/)
{
    IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, "hints syntax error"));
}
