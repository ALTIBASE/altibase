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
 * $Id: smiMain.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMI_MAIN_H_
#define _O_SMI_MAIN_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smiDef.h>

/* SM에서 사용할 콜백함수들 */
extern smiGlobalCallBackList gSmiGlobalCallBackList;

/* MEM_MAX_DB_SIZE property값이 올바른지 검사한다. */
IDE_RC smiCheckMemMaxDBSize( void );

// 사용자가 지정한 데이터베이스 크기를 토대로 
// 실제로 생성할 데이터베이스 크기를 계산한다.
IDE_RC smiCalculateDBSize( scPageID   aUserDbCreatePageCount,
                           scPageID * aDbCreatePageCount );

// 데이터베이스가 생성할 수 있는 최대 Page수를 계산
scPageID smiGetMaxDBPageCount( void );

// 데이터베이스가 생성할 수 있는 최소 Page수를 계산
scPageID smiGetMinDBPageCount( void );

// 하나의 데이터베이스 파일이 지니는 Page의 수를 리턴한다 
IDE_RC smiGetTBSFilePageCount(scSpaceID aSpaceID, scPageID * aDBFilePageCount);

// 데이터베이스를 생성한다.
IDE_RC smiCreateDB(SChar        * aDBName,
                   scPageID       aCreatePageCount,
                   SChar        * aDBCharSet,
                   SChar        * aNationalCharSet,
                   smiArchiveMode aArchiveMode = SMI_LOG_NOARCHIVE);

idBool smiRuntimeSelectiveLoadingSupport();

/* for A4 Startup Phase */

/*
 *  이 함수는 여러번 호출될 수 있음.
 *  증가하는 방향으로 호출되며, 감소할 수는 없다. 
 */

IDE_RC smiCreateDBCoreInit(smiGlobalCallBackList *   /*aCallBack*/);

IDE_RC smiCreateDBMetaInit(smiGlobalCallBackList*    /*aCallBack*/);

IDE_RC smiCreateDBCoreShutdown(smiGlobalCallBackList *   /*aCallBack*/);

IDE_RC smiCreateDBMetaShutdown(smiGlobalCallBackList*    /*aCallBack*/);

IDE_RC smiSmUtilInit(smiGlobalCallBackList*    aCallBack);

IDE_RC smiSmUtilShutdown(smiGlobalCallBackList*    aCallBack);


IDE_RC smiStartup(smiStartupPhase        aPhase,
                  UInt                   aActionFlag,
                  smiGlobalCallBackList* aCallBack = NULL);

IDE_RC smiSetRecStartupEnd();
/*
 * 현재의 Startup Phase를 돌려받는다. 
 */
smiStartupPhase smiGetStartupPhase();

#endif
 
