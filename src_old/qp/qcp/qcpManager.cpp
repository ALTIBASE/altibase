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
 * $Id: qcpManager.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <qdc.h>
#include <qcuError.h>
#include <qcpManager.h>
#include <qdParseTree.h>
#include <qdnTrigger.h>
#include <qmmParseTree.h>
#include <qmsParseTree.h>
#include <qriParseTree.h>
#include <qsParseTree.h>
#include <qspDef.h>
#include <qlParseTree.h> // PROJ-1685
#include <qdkParseTree.h>
#include <qdk.h>
#include <qdcAudit.h>
#include <qdcJob.h>
#include "qcplx.h"

#if defined(BISON_POSTFIX_HPP)
#include "qcply.hpp"
#else /* BISON_POSTFIX_CPP_H */
#include "qcply.cpp.h"
#endif

#include "qcpll.h"

extern int qcplparse(void *param);

IDE_RC qcpManager::parseIt( qcStatement * aStatement )
{
    qcplLexer  s_qcplLexer( aStatement->myPlan->stmtText,
                            aStatement->myPlan->stmtTextLen,
                            0,
                            aStatement->myPlan->stmtTextLen,
                            0 );

    IDU_FIT_POINT_FATAL( "qcpManager::parseIt::__FT__" );

    IDE_TEST( parseInternal( aStatement, & s_qcplLexer )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcpManager::parsePartialForSubquery( qcStatement * aStatement,
                                            SChar       * aText,
                                            SInt          aStart,
                                            SInt          aSize )
{
    SInt sTextLen = idlOS::strlen( aText );

    qcplLexer  s_qcplLexer( aText,
                            sTextLen,
                            aStart,
                            aSize,
                            TR_RETURN );

    IDU_FIT_POINT_FATAL( "qcpManager::parsePartialForSubquery::__FT__" );

    IDE_TEST_RAISE( ( aStart < 0 ) ||
                    ( aSize < 0 ) ||
                    ( aStart >= sTextLen ) ||
                    ( aSize > sTextLen ) ||
                    ( aStart + aSize > sTextLen ),
                    ERR_INVALID_ARGUMENT );

    IDE_TEST( parseInternal( aStatement, & s_qcplLexer )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcpManager::parsePartialForSubQueryNode",
                                  "Invalid argument" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcpManager::parsePartialForQuerySet( qcStatement * aStatement,
                                            SChar       * aText,
                                            SInt          aStart,
                                            SInt          aSize )
{
    SInt sTextLen = idlOS::strlen( aText );

    qcplLexer  s_qcplLexer( aText,
                            sTextLen,
                            aStart,
                            aSize,
                            TR_MODIFY );

    IDU_FIT_POINT_FATAL( "qcpManager::parsePartialForQuerySet::__FT__" );

    IDE_TEST_RAISE( ( aStart < 0 ) ||
                    ( aSize < 0 ) ||
                    ( aStart >= sTextLen ) ||
                    ( aSize > sTextLen ) ||
                    ( aStart + aSize > sTextLen ),
                    ERR_INVALID_ARGUMENT );

    IDE_TEST( parseInternal( aStatement, & s_qcplLexer )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcpManager::parsePartialForQuerySet",
                                  "Invalid argument" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcpManager::parsePartialForOrderBy( qcStatement * aStatement,
                                           SChar       * aText,
                                           SInt          aStart,
                                           SInt          aSize )
{
    SInt sTextLen = idlOS::strlen( aText );

    qcplLexer  s_qcplLexer( aText,
                            sTextLen,
                            aStart,
                            aSize,
                            TR_BACKUP );

    IDU_FIT_POINT_FATAL( "qcpManager::parsePartialForOrderBy::__FT__" );

    IDE_TEST_RAISE( ( aStart < 0 ) ||
                    ( aSize < 0 ) ||
                    ( aStart >= sTextLen ) ||
                    ( aSize > sTextLen ) ||
                    ( aStart + aSize > sTextLen ),
                    ERR_INVALID_ARGUMENT );

    IDE_TEST( parseInternal( aStatement, & s_qcplLexer )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcpManager::parsePartialForOrderBy",
                                  "Invalid argument" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcpManager::parsePartialForAnalyze( qcStatement * aStatement,
                                           SChar       * aText,
                                           SInt          aStart,
                                           SInt          aSize )
{
    SInt sTextLen = idlOS::strlen( aText );

    qcplLexer  s_qcplLexer( aText,
                            sTextLen,
                            aStart,
                            aSize,
                            TA_SHARD );

    IDU_FIT_POINT_FATAL( "qcpManager::parsePartialForAnalyze::__FT__" );

    IDE_TEST_RAISE( ( aStart < 0 ) ||
                    ( aSize < 0 ) ||
                    ( aStart >= sTextLen ) ||
                    ( aSize > sTextLen ) ||
                    ( aStart + aSize > sTextLen ),
                    ERR_INVALID_ARGUMENT );

    IDE_TEST( parseInternal( aStatement, & s_qcplLexer )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    aStatement->myPlan->parseTree->stmtShard = QC_STMT_SHARD_NONE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcpManager::parsePartialForAnalyze",
                                  "Invalid argument" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-7219 */
IDE_RC qcpManager::parsePartialForWhere( qcStatement * aStatement,
                                         SChar       * aText,
                                         SInt          aStart,
                                         SInt          aSize )
{
    SInt sTextLen = idlOS::strlen( aText );

    qcplLexer s_qcplLexer( aText,
                           sTextLen,
                           aStart,
                           aSize,
                           TR_WHERE );

    IDU_FIT_POINT_FATAL( "qcpManager::parsePartialForWhere::__FT__" );

    IDE_TEST_RAISE( ( aStart < 0 ) ||
                    ( aSize < 0 ) ||
                    ( aStart >= sTextLen ) ||
                    ( aSize > sTextLen ) ||
                    ( aStart + aSize > sTextLen ),
                    ERR_INVALID_ARGUMENT );

    IDE_TEST( parseInternal( aStatement, & s_qcplLexer )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcpManager::parsePartialForWhere",
                                  "Invalid argument" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcpManager::parseInternal( qcStatement * aStatement,
                                  qcplLexer   * aLexer )
{
    qcplx          s_qcplx;
    qcTemplate   * sTemplate;

    IDU_FIT_POINT_FATAL( "qcpManager::parseInternal::__FT__" );

    IDE_DASSERT( aStatement->myPlan == & aStatement->privatePlan );

    // set members of qcplx
    s_qcplx.mLexer              = aLexer;
    s_qcplx.mLexer->mStatement  = aStatement;
    s_qcplx.mSession            = aStatement->session;
    s_qcplx.mStatement          = aStatement;
    s_qcplx.mNcharList          = NULL;

    /* PROJ-2446 ONE SOURCE */
    s_qcplx.mStatistics         = aStatement->mStatistics;

    aStatement->myPlan->hints = NULL;

    // Parser makes aStatement->parseTree
    IDE_TEST( qcplparse( & s_qcplx ) != IDE_SUCCESS );

    if ( aStatement->myPlan->parseTree == NULL )
    {
        /* PROJ-2550 PSM Encryption
           aStatement->myPlan->stmtText가 encrypted text라면,
           첫번째 parsing을 통해, plain text를 얻어,
           해당 text로 다시 parsing하여 parse tree를 생성한다.
           aStatement->myPlan->stmtText는
           첫번째 parsing 단계에서 plain text로 교체된다. */
        IDE_TEST( parseIt( aStatement ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE( aStatement->myPlan->parseTree == NULL, ERR_PARSER );

        aStatement->myPlan->parseTree->stmt = aStatement;

        sTemplate = QC_SHARED_TMPLATE(aStatement);

        if ( sTemplate->insOrUptStmtCount > 0 )
        {
            if( sTemplate->insOrUptRowValueCount == NULL &&
                sTemplate->insOrUptRow == NULL )
            {
                IDE_TEST( QC_QMP_MEM( aStatement )->cralloc( ID_SIZEOF( UInt ) * sTemplate->insOrUptStmtCount,
                                                             (void**)&( sTemplate->insOrUptRowValueCount ) )
                          != IDE_SUCCESS );

                IDE_TEST( QC_QMP_MEM(aStatement)->cralloc( ID_SIZEOF( smiValue* ) * sTemplate->insOrUptStmtCount,
                                                           (void**)&( sTemplate->insOrUptRow ) )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
                // To Fix BUG-12887
                // insert나 update에서 default value가 있는 function을 호출하면
                // default value를 parsing하기 때문에, 여기서 또 할당받아서
                // 기존의 값들은 전부 날라가게 된다.
                // Ex) insert into t1 values( func1(1));
                //     func1의 argument는 v1 integer, v2 integer default 100
                // 기존 template의 할당되어 있는 공간을 유지하기 위해
                // null이 아니라면 할당받도록 수정.
            }
        }

        // do not allocate cursorInfo, although sTemplate->procCurCount > 0
        // it is allocated in the qsx::cloneTemplate again.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_PARSER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_SYNTAX, ""));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
