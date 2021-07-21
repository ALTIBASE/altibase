/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
#include <idl.h>
#include <idp.h>
#include <iduShmKeyFile.h>

/***********************************************************************
 * Description : Shared Memory를 생성한 XDB Daemon Process의 Startup 시간을
 *               $ALTIBASE_XDB_HOME/conf/xdbaltibase_shm.date에서 읽어온다.
 *               만약 파일이 없다면 현재 시간을 넘겨준다.
 *
 * aStartUpTime - [OUT] XDB Daemon Proces가 Startup한 시간.
 **********************************************************************/
IDE_RC iduShmKeyFile::getStartupInfo( struct timeval * aStartUpTime,
                                      UInt           * aPrevShmKey )
{
    SChar                 sShmKeyFileName[1024];
    iduFile               sShmKeyFile;
    iduStartupInfoOfXDB   sStartupInfo;
    struct timeval        sStartUpTime = idlOS::gettimeofday();
    UInt                  sPrevShmKey = 0;
    SInt                  sState    = 0;
    ULong                 sFileSize = 0;

    idlOS::snprintf( sShmKeyFileName,
                     1024,
                     "%s%sconf%s%s",
                     idlOS::getenv(IDP_HOME_ENV),
                     IDL_FILE_SEPARATORS,
                     IDL_FILE_SEPARATORS,
                     IDU_SHM_KEY_FILENAME );

    IDE_TEST( sShmKeyFile.initialize( IDU_MEM_ID_IDU,
                                      1, /* Max Open FD Count */
                                      IDU_FIO_STAT_OFF,
                                      IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sShmKeyFile.setFileName( sShmKeyFileName )
              != IDE_SUCCESS );

    while(1)
    {
        if( sShmKeyFile.exist() == ID_TRUE )
        {
            IDE_TEST( sShmKeyFile.open() != IDE_SUCCESS );
            sState = 2;

            IDE_TEST( sShmKeyFile.getFileSize( &sFileSize )
                      != IDE_SUCCESS );

            if ( sFileSize >= ID_SIZEOF( iduStartupInfoOfXDB ) )
            {
                IDE_TEST( sShmKeyFile.read( NULL /* idvSQL */,
                                            0,
                                            &sStartupInfo,
                                            ID_SIZEOF( iduStartupInfoOfXDB ) )
                          != IDE_SUCCESS );

                if ( idlOS::memcmp( &sStartupInfo.mStartupTime1,
                                    &sStartupInfo.mStartupTime2,
                                    ID_SIZEOF( struct timeval ) ) == 0 )
                {
                    sStartUpTime = sStartupInfo.mStartupTime1;
                    sPrevShmKey  = sStartupInfo.mShmDBKey;
                }
            }

        }

        if ( aStartUpTime != NULL )
        {
            *aStartUpTime = sStartUpTime;
        }
        if ( aPrevShmKey != NULL )
        {
            *aPrevShmKey = sPrevShmKey;
        }

        break;
    }

    if( sState != 0 )
    {
        sState = 1;
        IDE_TEST( sShmKeyFile.close() != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sShmKeyFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
    case 2:
        IDE_ASSERT( sShmKeyFile.close() == IDE_SUCCESS );
    case 1:
        IDE_ASSERT( sShmKeyFile.destroy() == IDE_SUCCESS );
    default:
        break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Shared Memory를 생성한 Daemon Process의 Startup 정보를
 *               $ALTIBASE_XDB_HOME/conf/xdbaltibase_shm.info에 설정한다.
 *               생성한 시간과 shmkey가 저장됨.
 *
 * aStartUpTime - [IN] XDB Daemon Proces가 Startup한 시간.
 **********************************************************************/
IDE_RC iduShmKeyFile::setStartupInfo( struct timeval * aStartUpTime )
{
    SChar                 sShmKeyFileName[1024];
    iduFile               sShmKeyFile;
    iduStartupInfoOfXDB   sStartupInfo;
    SInt                  sState = 0;
    
    idlOS::snprintf( sShmKeyFileName,
                     1024,
                     "%s%sconf%s%s",
                     idlOS::getenv(IDP_HOME_ENV),
                     IDL_FILE_SEPARATORS,
                     IDL_FILE_SEPARATORS,
                     IDU_SHM_KEY_FILENAME );

    IDE_TEST( sShmKeyFile.initialize( IDU_MEM_ID_IDU,
                                      1, /* Max Open FD Count */
                                      IDU_FIO_STAT_OFF,
                                      IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sShmKeyFile.setFileName( sShmKeyFileName )
              != IDE_SUCCESS );

    if( sShmKeyFile.exist() == ID_FALSE )
    {
        IDE_TEST( sShmKeyFile.create() != IDE_SUCCESS );
    }

    IDE_TEST( sShmKeyFile.open() != IDE_SUCCESS );
    sState = 2;

    idlOS::memcpy( &sStartupInfo.mStartupTime1,
                   aStartUpTime,
                   ID_SIZEOF( struct timeval ) );
  
    idlOS::memcpy( &sStartupInfo.mStartupTime2,
                   aStartUpTime,
                   ID_SIZEOF( struct timeval ) );

    sStartupInfo.mShmDBKey = iduProperty::getShmDBKey();
  
    IDE_TEST( sShmKeyFile.write( NULL /* idvSQL */,
                                 0,
                                 &sStartupInfo,
                                 ID_SIZEOF( iduStartupInfoOfXDB ) )
              != IDE_SUCCESS );

    IDE_TEST( sShmKeyFile.sync() != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sShmKeyFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sShmKeyFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
    case 2:
        IDE_ASSERT( sShmKeyFile.close() == IDE_SUCCESS );
    case 1:
        IDE_ASSERT( sShmKeyFile.destroy() == IDE_SUCCESS );
    default:
        break;
    }

    IDE_POP();

    return IDE_FAILURE;
}
