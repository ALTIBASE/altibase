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
 * $Id: qdcLibrary.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <qcg.h>
#include <qcuSqlSourceInfo.h>
#include <qdcLibrary.h>
#include <qdpPrivilege.h>
#include <qlParseTree.h>
#include <qcmLibrary.h>
#include <qcmProc.h>
#include <qcmPkg.h>
#include <qcmUser.h>
#include <qdpRole.h>

IDE_RC qdcLibrary::validateCreate( qcStatement * aStatement )
{
    qsOID            sProcID;
    idBool           sExist = ID_FALSE;
    qcuSqlSourceInfo sqlInfo;
    
    qlParseTree * sParseTree = (qlParseTree *)( aStatement->myPlan->parseTree );

    if( qdbCommon::containDollarInName( &(sParseTree->libraryNamePos) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->libraryNamePos) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    // check already existing object
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->libraryNamePos,
                                QS_OBJECT_MAX,
                                &(sParseTree->userID),
                                &sProcID,
                                &sExist )
              != IDE_SUCCESS );

    if( sExist == ID_TRUE )
    {
        if( sParseTree->replace == ID_TRUE )
        {
            /* Nothing to do */
        }
        else
        {
            IDE_RAISE( ERR_EXIST_OBJECT_NAME );
        }
    }
    else
    {
        if( sParseTree->replace == ID_TRUE )
        {
            sParseTree->replace = ID_FALSE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    
    // check grant
    IDE_TEST( qdpRole::checkDDLCreateLibraryPriv( aStatement,
                                                  sParseTree->userID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXIST_OBJECT_NAME );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_EXIST_OBJECT_NAME ) );
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC qdcLibrary::executeCreate( qcStatement * aStatement )
{
    qlParseTree * sParseTree;

    sParseTree = (qlParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( qcmLibrary::addMetaInfo( aStatement,
                                       sParseTree->userID,
                                       sParseTree->libraryName,
                                       sParseTree->fileSpec,
                                       sParseTree->replace )
              != IDE_SUCCESS )

    if( sParseTree->replace == ID_TRUE )
    {
        // related PSM
        IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                      aStatement,
                      sParseTree->userID,
                      (SChar *)(sParseTree->libraryNamePos.stmtText + sParseTree->libraryNamePos.offset),
                      sParseTree->libraryNamePos.size,
                      QS_LIBRARY )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcLibrary::validateDrop( qcStatement * aStatement )
{
    idBool            sExist;
    qcuSqlSourceInfo  sqlInfo;
    qlDropParseTree * sParseTree = (qlDropParseTree *)aStatement->myPlan->parseTree;
    qcmLibraryInfo    sLibraryInfo;
    
    if( QC_IS_NULL_NAME( sParseTree->userNamePos ) == ID_TRUE )
    {
        sParseTree->userID = QCG_GET_SESSION_USER_ID( aStatement );
    }
    else
    {
        IDE_TEST( qcmUser::getUserID( aStatement, 
                                      sParseTree->userNamePos, 
                                      &sParseTree->userID )
                  != IDE_SUCCESS);
    }

    // check already existing object
    IDE_TEST( qcmLibrary::getLibrary( aStatement,
                                      sParseTree->userID,
                                      (SChar*)(sParseTree->libraryNamePos.stmtText +
                                               sParseTree->libraryNamePos.offset),
                                      sParseTree->libraryNamePos.size,
                                      &sLibraryInfo,
                                      &sExist ) != IDE_SUCCESS ) 

    if( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->libraryNamePos );

        IDE_RAISE( ERR_NOT_EXISTS );
    }
    else
    {
        // check grant
        IDE_TEST( qdpRole::checkDDLDropLibraryPriv( aStatement,
                                                    sParseTree->userID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXISTS );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcLibrary::executeDrop( qcStatement * aStatement )
{
    qlDropParseTree * sParseTree = (qlDropParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( qcmLibrary::delMetaInfo( aStatement,
                                       sParseTree->userID,
                                       sParseTree->libraryName ) != IDE_SUCCESS ); 

    // related PSM
    IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                  aStatement,
                  sParseTree->userID,
                  (SChar *)(sParseTree->libraryNamePos.stmtText + sParseTree->libraryNamePos.offset),
                  sParseTree->libraryNamePos.size,
                  QS_LIBRARY )
              != IDE_SUCCESS );

    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                  aStatement,
                  sParseTree->userID,
                  (SChar *)(sParseTree->libraryNamePos.stmtText + sParseTree->libraryNamePos.offset),
                  sParseTree->libraryNamePos.size,
                  QS_LIBRARY )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcLibrary::validateAlter( qcStatement * aStatement )
{
    idBool            sExist;
    qcuSqlSourceInfo  sqlInfo;
    qlAlterParseTree * sParseTree = (qlAlterParseTree *)aStatement->myPlan->parseTree;
    qcmLibraryInfo     sLibraryInfo;
    
    if( QC_IS_NULL_NAME( sParseTree->userNamePos ) == ID_TRUE )
    {
        sParseTree->userID = QCG_GET_SESSION_USER_ID( aStatement );
    }
    else
    {
        IDE_TEST( qcmUser::getUserID( aStatement, 
                                      sParseTree->userNamePos, 
                                      &sParseTree->userID )
                  != IDE_SUCCESS);
    }

    // check already existing object
    IDE_TEST( qcmLibrary::getLibrary( aStatement,
                                      sParseTree->userID,
                                      (SChar*)(sParseTree->libraryNamePos.stmtText +
                                               sParseTree->libraryNamePos.offset),
                                      sParseTree->libraryNamePos.size,
                                      &sLibraryInfo,
                                      &sExist ) != IDE_SUCCESS ) 

    if( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->libraryNamePos );

        IDE_RAISE( ERR_NOT_EXISTS );
    }
    else
    {
        // check grant
        IDE_TEST( qdpRole::checkDDLAlterLibraryPriv( aStatement,
                                                     sParseTree->userID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXISTS );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcLibrary::executeAlter( qcStatement * /* aStatement */ )
{
    /* Nothing to do */

    return IDE_SUCCESS;
}
