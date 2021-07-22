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
 * $Id: qcuSqlSourceInfo.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _QCU_SQL_SOURCE_INFO_
#define _QCU_SQL_SOURCE_INFO_ 1

#include <qc.h>
#include <iduMemory.h>

struct qcNamePosition ;

class qcuSqlSourceInfo
{
protected :
    iduVarMemList       * mem_p;                         // fix PROJ-1596
    iduMemory           * mem_x;
    SChar               * sqlInfoBuf_;
    UInt                  sqlInfoBufSize;
    SChar               * beforeMessageBuf_;
    iduVarMemListStatus   memStatus_p;                   // fix PROJ-1596
    iduMemoryStatus       memStatus_x;
    qcStatement         * stmt_;
    SInt                  pos1_;
    SInt                  pos2_;
    iduMemoryClientIndex  index;                         // fix PROJ-1596
    SChar               * stmtText;
    SInt                  stmtTextLen;

public :
    qcuSqlSourceInfo();
    IDE_RC initWithBeforeMessage( iduMemory     * mem );
    IDE_RC initWithBeforeMessage( iduVarMemList * mem ); // fix PROJ-1596

    IDE_RC init(
        iduMemory * mem,
        SChar     * beforeMessage = NULL);
    IDE_RC init(                                         // fix PROJ-1596
        iduVarMemList * mem,
        SChar         * beforeMessage = NULL);
    IDE_RC fini();
    void setSourceInfo (
        qcStatement    * stmt,
        qcNamePosition * namePos1 );

    void setSourceInfo (
        qcStatement    * stmt,
        qcNamePosition * namePos1,
        qcNamePosition * namePos2 );

    SChar * getErrMessage ();
    SChar * getBeforeErrMessage();

    void    applySqlRevise();
};

#define IS_VALID_TOK(pos1, pos2) ( (pos1) > 0 && ((pos1) <= (pos2)) )

#endif
