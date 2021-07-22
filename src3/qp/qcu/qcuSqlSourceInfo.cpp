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
 * $Id: qcuSqlSourceInfo.cpp 89096 2020-10-29 23:09:57Z bethy $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuError.h>
#include <qcuSqlSourceInfo.h>
#include <qc.h>
#include <qsParseTree.h>
#include <qsvEnv.h>
#include <sdi.h>

qcuSqlSourceInfo::qcuSqlSourceInfo()
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mem_p             = NULL;
    mem_x             = NULL;
    sqlInfoBuf_       = NULL;
    sqlInfoBufSize    = 0;
    beforeMessageBuf_ = NULL;
    stmt_             = NULL;
    pos1_             = -1;
    pos2_             = -1;
    stmtText          = NULL;
    stmtTextLen       = 0;

#undef IDE_FN
}

IDE_RC
qcuSqlSourceInfo::initWithBeforeMessage( iduMemory * mem )
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return init( mem, sdi::getMultiErrorMsg() );

#undef IDE_FN
}

IDE_RC
qcuSqlSourceInfo::initWithBeforeMessage( iduVarMemList * mem )
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return init( mem, sdi::getMultiErrorMsg() );

#undef IDE_FN
}

IDE_RC
qcuSqlSourceInfo::init( iduVarMemList  * mem,
                        SChar          * beforeMessage /* = NULL */)
{
#define IDE_FN "qcuSqlSourceInfo::init(iduMemory * mem)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    index = IDU_MEM_QMP;

    if (sqlInfoBuf_ == NULL)
    {
        mem_p = mem;
        IDE_TEST(mem_p->getStatus( &memStatus_p ) != IDE_SUCCESS );

        sqlInfoBufSize = QCU_SQL_INFO_SIZE;
        // BUG-38018 replace cralloc -> alloc
        IDE_TEST(mem_p->alloc( sqlInfoBufSize + 32, (void**)&sqlInfoBuf_)
                 != IDE_SUCCESS);
        sqlInfoBuf_[0] = '\0';
    }
    else
    {
        sqlInfoBuf_[0] = '\0';
    }

    if (beforeMessage != NULL)
    {
        // BUG-38018 replace cralloc -> alloc
        IDE_TEST(mem_p->alloc( idlOS::strlen( beforeMessage) + 1, (void**)&beforeMessageBuf_ )
                 != IDE_SUCCESS);

        idlOS::strcpy( beforeMessageBuf_, beforeMessage );
    }
    else
    {
        beforeMessageBuf_ = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qcuSqlSourceInfo::init( iduMemory  * mem,
                        SChar      * beforeMessage /* = NULL */)
{
#define IDE_FN "qcuSqlSourceInfo::init(iduMemory * mem)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    index = IDU_MEM_QMX;

    if (sqlInfoBuf_ == NULL)
    {
        mem_x = mem;
        IDE_TEST(mem_x->getStatus( &memStatus_x ) != IDE_SUCCESS );

        sqlInfoBufSize = QCU_SQL_INFO_SIZE;
        // BUG-38018 replace cralloc -> alloc
        IDE_TEST(mem_x->alloc( sqlInfoBufSize + 32, (void**)&sqlInfoBuf_)
                 != IDE_SUCCESS);
        sqlInfoBuf_[0] = '\0';
    }
    else
    {
        sqlInfoBuf_[0] = '\0';
    }

    if (beforeMessage != NULL)
    {
        // BUG-38018 replace cralloc -> alloc
        IDE_TEST(mem_x->alloc( idlOS::strlen( beforeMessage) + 1, (void**)&beforeMessageBuf_ )
                 != IDE_SUCCESS);

        idlOS::strcpy( beforeMessageBuf_, beforeMessage );
    }
    else
    {
        beforeMessageBuf_ = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qcuSqlSourceInfo::fini()
{
#define IDE_FN "qcuSqlSourceInfo::fini()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( sqlInfoBuf_ != NULL )
    {
        if( index == IDU_MEM_QMP )
        {
            if ( mem_p != NULL )
            {
                IDE_TEST( mem_p->setStatus( &memStatus_p ) != IDE_SUCCESS );
                mem_p = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( mem_x != NULL )
            {
                IDE_TEST( mem_x->setStatus( &memStatus_x ) != IDE_SUCCESS );
                mem_x = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    sqlInfoBuf_       = NULL;
    sqlInfoBufSize    = 0;
    beforeMessageBuf_ = NULL;
    stmt_             = NULL;
    pos1_             = -1;
    pos2_             = -1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

void
qcuSqlSourceInfo::setSourceInfo ( qcStatement    * stmt,
                                  qcNamePosition * namePos )
{
#define IDE_FN "SInt qcuSqlSourceInfo::setSourceInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    stmt_        = stmt;

    if ( stmt_ != NULL )
    {
        pos1_        = namePos->offset;
        pos2_        = namePos->offset + namePos->size;

        stmtText     = stmt_->myPlan->stmtText;
        stmtTextLen  = stmt_->myPlan->stmtTextLen;

        (void)applySqlRevise();

        // BUG-44856
        if ( stmt->spxEnv != NULL )
        {
            stmt->spxEnv->mSqlInfo = *namePos;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

#undef IDE_FN
}


void
qcuSqlSourceInfo::setSourceInfo ( qcStatement   * stmt,
                                  qcNamePosition * namePos1,
                                  qcNamePosition * namePos2 )
{
#define IDE_FN "SInt qcuSqlSourceInfo::setSourceInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SInt      tok1_pos1;
    SInt      tok1_pos2;
    SInt      tok2_pos1;
    SInt      tok2_pos2;

    stmt_        = stmt;

    if ( stmt_ != NULL )
    {
        tok1_pos1 = namePos1->offset;
        tok1_pos2 = namePos1->offset + namePos1->size ;
        tok2_pos1 = namePos2->offset;
        tok2_pos2 = namePos2->offset + namePos2->size ;
    
        if (IS_VALID_TOK(tok1_pos1, tok1_pos2))
        {
            pos1_ = tok1_pos1;
        }
        else
        {
            pos1_ = tok2_pos1;
        }

        pos2_ = tok2_pos2;

        stmtText     = stmt_->myPlan->stmtText;
        stmtTextLen  = stmt_->myPlan->stmtTextLen;

        (void)applySqlRevise();

        // BUG-44856
        if ( stmt->spxEnv != NULL )
        {
            stmt->spxEnv->mSqlInfo = *namePos1;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

#undef IDE_FN
}

SChar *
qcuSqlSourceInfo::getErrMessage ()
{
#define IDE_FN "qcuSqlSourceInfo::getErrMessage"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar               * sqlBuf = NULL;
    SInt                  lineCount = 1;
    SInt                  lineStartPos = 0;
    SInt                  lineCharCount;
    SInt                  sStrLen;
    SInt                  i;
    SInt                  bufRemain  = sqlInfoBufSize;
    SChar               * o_buf;
    qcNamePosition      * sNamePos   = NULL;
    qsProcParseTree     * sParseTree = NULL;

    IDE_TEST_RAISE(stmt_ == NULL, ERR_STMT_IS_NULL);

    IDE_TEST_RAISE(sqlInfoBuf_ == NULL, ERR_BUT_NOT_INITIALIZED);
    o_buf = sqlInfoBuf_;

    *o_buf++ = '\n';
    bufRemain--;

    *o_buf = '\0';

    // BUG-42322
    if ( ( QCU_PSM_SHOW_ERROR_STACK == 0 ) && 
         ( stmt_->calledByPSMFlag == ID_TRUE ) )
    {
        return (SChar *)"";
    }

    if (stmt_->myPlan->parseTree != NULL )
    {
        if (stmt_->spvEnv->createProc != NULL )
        {
            sParseTree = stmt_->spvEnv->createProc;
        }
        else
        {
            if( stmt_->spxEnv != NULL )
            {
                sParseTree = stmt_->spxEnv->mProcPlan;
            }
        }

        if ( sParseTree != NULL )
        {
            // BUG-43981
            // In PSM_name -> In User_name.ObjectName
            if ( sParseTree->objectNameInfo.name != NULL )
            {
                sStrLen = idlOS::snprintf( o_buf,
                                           bufRemain,
                                           "\nIn %s %s",
                                           sParseTree->objectNameInfo.objectType,
                                           sParseTree->objectNameInfo.name );
                o_buf += sStrLen;

                *o_buf++ = '\n';
                *o_buf   = '\0';
                bufRemain -= (sStrLen + 1);
            }
            else
            {
                sNamePos = &(sParseTree->procNamePos);

                sStrLen = idlOS::snprintf( o_buf, bufRemain, "\nIn " );
                o_buf += sStrLen;

                idlOS::memcpy( o_buf,
                               sNamePos->stmtText + sNamePos->offset,
                               sNamePos->size );
                o_buf += sNamePos->size;

                *o_buf++ = '\n';
                *o_buf   = '\0';
                bufRemain -= (sStrLen + sNamePos->size + 1);
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    if (pos2_ < pos1_)
    {
        return sqlInfoBuf_;
    }

    if( index == IDU_MEM_QMP )
    {
        // BUG-38018 replace cralloc -> alloc
        IDE_TEST_RAISE(mem_p->alloc( stmtTextLen + 16, (void**)&sqlBuf)
                       != IDE_SUCCESS, ERR_NO_MEM);
    }
    else
    {
        // BUG-38018 replace cralloc -> alloc
        IDE_TEST_RAISE(mem_x->alloc( stmtTextLen + 16, (void**)&sqlBuf)
                       != IDE_SUCCESS, ERR_NO_MEM);
    }

    idlOS::memcpy( sqlBuf,
                   stmtText,
                   stmtTextLen );
    sqlBuf[ stmtTextLen ] = '\0';

    if ( sqlBuf [ stmtTextLen - 1 ] != '\n' )
    {
        idlOS::strcat (sqlBuf, "\n ");
    }
    else
    {
        idlOS::strcat (sqlBuf, " ");
    }

    for (i=0; sqlBuf[i] != '\0'; i++)
    {
        if (i != 0)
        {
            if (sqlBuf[i-1] == '\n')
            {
                if ( (pos1_ < i && pos1_ >= lineStartPos) || // pos1 is in line
                     (pos2_ < i && pos2_ >= lineStartPos) || // pos2 is in line
                     // line is between pos1 and pos2
                     (lineStartPos > pos1_ && lineStartPos < pos2_) )
                {
                    IDE_TEST_RAISE
                        (
                            bufRemain -
                            ( (i - lineStartPos) * 2 +  // count of line string
                                                        // plus token indicator(^).
                            16 )                        // count of additional info
                            < 0,
                            ERR_OUT_OF_BUFFER
                        );

                    sStrLen = idlOS::snprintf( o_buf,
                                               bufRemain,
                                               "%04"ID_INT32_FMT" : ",
                                               lineCount );
                    bufRemain -= sStrLen;

                    o_buf = o_buf + idlOS::strlen(o_buf);

                    lineCharCount = i - lineStartPos;

                    idlOS::strncpy(o_buf,
                                   &sqlBuf[ lineStartPos ],
                                   lineCharCount);
                    o_buf[lineCharCount] = '\0';

                    bufRemain -= idlOS::strlen(o_buf);
                    o_buf = o_buf + idlOS::strlen(o_buf);

                    // pos1 is in the line?
                    if (lineStartPos <= pos1_ &&
                        pos1_ < i)
                    {
                        bufRemain -= idlOS::snprintf( o_buf,
                                                      bufRemain,
                                                      "%*s",
                                                      pos1_ - lineStartPos +
                                                      sStrLen,
                                                      "^" );
                        o_buf = o_buf + idlOS::strlen(o_buf);

                        // pos1 and pos2 is in the line?
                        if (lineStartPos <= pos2_ &&
                            pos2_ < i)
                        {
                            if (pos2_ > pos1_)
                            {
                                bufRemain -= idlOS::snprintf( o_buf,
                                                              bufRemain,
                                                              "%*s",
                                                              pos2_ - pos1_,
                                                              "^" );
                                o_buf = o_buf + idlOS::strlen(o_buf);
                            }
                        }

                        *o_buf++ = '\n';
                        *o_buf   = '\0';
                        bufRemain--;
                    }
                    else
                    {
                        // only pos2 is in the line?
                        if (lineStartPos <= pos2_ &&
                            pos2_ < i)
                        {
                            bufRemain -= idlOS::snprintf( o_buf,
                                                          bufRemain,
                                                          "%*s",
                                                          pos2_ - lineStartPos
                                                          + sStrLen,
                                                          "^" );
                            o_buf = o_buf + idlOS::strlen(o_buf);

                            *o_buf++ = '\n';
                            *o_buf   = '\0';
                            bufRemain--;
                        }
                    }
                }

                lineCount++;
                lineStartPos = i;
            }
        }
    }

    // BUG-38883 print error position
    (void)ideSetHasErrorPosition();

    return sqlInfoBuf_;

    IDE_EXCEPTION(ERR_BUT_NOT_INITIALIZED);
    {
        return (SChar*)
            "\nERROR (Error formatter) : buffer is not initialized\n";
    }
    IDE_EXCEPTION(ERR_OUT_OF_BUFFER);
    {
        return (SChar*)"\nERROR (Error formatter) : out of SQL text buffer\n";
    }
    IDE_EXCEPTION(ERR_STMT_IS_NULL);
    {
        return (SChar*)
            "\nERROR (Error formatter) : setSourceInfo is not called\n";
    }
    IDE_EXCEPTION(ERR_NO_MEM);
    {
        return (SChar*)
            "\nERROR (Error formatter) : memory allocation failed\n";
    }
    IDE_EXCEPTION_END;

    return (SChar*)"\nERROR : unknown error\n";

#undef IDE_FN
}

SChar *
qcuSqlSourceInfo::getBeforeErrMessage()
{
#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return (beforeMessageBuf_ == NULL) ? (SChar*)"(null)" : beforeMessageBuf_;

#undef IDE_FN
}

void
qcuSqlSourceInfo::applySqlRevise()
{
    if( stmt_->spvEnv != NULL )
    {
        if( stmt_->spvEnv->sql != NULL )
        {
            if( stmtText != stmt_->spvEnv->sql )
            {
                stmtText    = stmt_->spvEnv->sql;
                stmtTextLen = stmt_->spvEnv->sqlSize;
            }
        }
    }
}
