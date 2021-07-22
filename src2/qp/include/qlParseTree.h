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
 * $Id: qlParseTree.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QL_PARSE_TREE_H_
#define _O_QL_PARSE_TREE_H_ 1

#include <qc.h>
#include <qtcDef.h>

typedef struct qlParseTree
{
    qcParseTree      common;
    qcNamePosition   userNamePos;
    qcNamePosition   libraryNamePos;
    SChar          * libraryName;
    SChar          * fileSpec;
    idBool           replace;
    UInt             userID;
} qlParseTree;

typedef struct qlAlterParseTree
{
    qcParseTree      common;
    qcNamePosition   userNamePos;
    qcNamePosition   libraryNamePos;
    SChar          * libraryName;
    UInt             userID;
} qlAlterParseTree;

typedef struct qlDropParseTree
{
    qcParseTree      common;
    qcNamePosition   userNamePos;
    qcNamePosition   libraryNamePos;
    SChar          * libraryName;
    UInt             userID;
} qlDropParseTree;

#endif /* _O_QL_PARSE_TREE_H_ */
