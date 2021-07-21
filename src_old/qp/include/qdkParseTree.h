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
 

#ifndef _O_QDK_PARSE_TREE_H_
#define _O_QDK_PARSE_TREE_H_ 1

#include <qc.h>

typedef struct qdkDatabaseLinkCreateParseTree
{
    qcParseTree             common;

    /* TRUE is public, FALSE is private */
    idBool                  publicFlag;
    qcNamePosition          name;
    
    idBool                  userPasswordExistFlag;
    qcNamePosition          userId;
    qcNamePosition          password;
 
    qcNamePosition          targetName;
 
} qdkDatabaseLinkCreateParseTree;

#define QDK_DATABASE_LINK_CREATE_PARSE_TREE_INIT( _dst_ )                   \
    {                                                                       \
        _dst_->publicFlag = ID_TRUE;                                        \
                                                                            \
        SET_EMPTY_POSITION( _dst_->name );                                  \
                                                                            \
        _dst_->userPasswordExistFlag = ID_FALSE;                            \
                                                                            \
        SET_EMPTY_POSITION( _dst_->userId );                                \
                                                                            \
        SET_EMPTY_POSITION( _dst_->password );                              \
                                                                            \
        SET_EMPTY_POSITION( _dst_->targetName );                            \
    }

typedef struct qdkRemoteDatabaseUserParseTree
{
    qcNamePosition          userId;
    
    qcNamePosition          password;
    
} qdkRemoteDatabaseUserParseTree;


typedef struct qdkDatabaseLinkDropParseTree
{
    qcParseTree             common;

    /* TRUE is public, FALSE is private */
    idBool                  publicFlag;
    
    qcNamePosition          name;
    
} qdkDatabaseLinkDropParseTree;

#define QDK_DATABASE_LINK_DROP_PARSE_TREE_INIT( _dst_ )                     \
    {                                                                       \
        _dst_->publicFlag = ID_TRUE;                                        \
                                                                            \
        SET_EMPTY_POSITION( _dst_->name );                                  \
                                                                            \
    }

enum qdkDatabaseLinkerControl
{
    QDK_DATABASE_LINKER_START,
    QDK_DATABASE_LINKER_STOP,
    QDK_DATABASE_LINKER_DUMP
};

typedef struct qdkDatabaseLinkAlterParseTree
{
    qcParseTree             common;
    
    enum qdkDatabaseLinkerControl control;
    
    idBool                  forceFlag;
    
} qdkDatabaseLinkAlterParseTree;

typedef struct qdkDatabaseLinkCloseParseTree
{
    qcParseTree             common;

    idBool                  allFlag;

    qcNamePosition          databaseLinkName;
    
} qdkDatabaseLinkCloseParseTree;

#endif /* _O_QDK_PARSE_TREE_H_ */
