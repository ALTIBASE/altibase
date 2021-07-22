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
 
#include <idl.h>
#include <idu.h>
#include <iduLatch.h>
#include <iduMutex.h>
#include <idtBaseThread.h>

iduMutex gLock;

class Server : public idtBaseThread
{
public:
  Server() {}
  ~Server() {}			

  void run();
};

void Server::run()
{
  char *log = "task 1\n";

  FILE * a = NULL;

  time_t timer;
  struct tm now;

  idlOS::time( &timer );
  idlOS::localtime_r( &timer, &now );

  IDE_TEST( gLock.lock() != IDE_SUCCESS );

  a = idlOS::fopen( "a.log", "a+" );

  if( a != NULL )
  {
    idlOS::fprintf( a,
                    "[%4"ID_UINT32_FMT
                    "/%02"ID_UINT32_FMT
                    "/%02"ID_UINT32_FMT
                    " %02"ID_UINT32_FMT
                    ":%02"ID_UINT32_FMT
                    ":%02"ID_UINT32_FMT"] ",
                    now.tm_year + 1900,
                    now.tm_mon + 1,
                    now.tm_mday,
                    now.tm_hour,
                    now.tm_min,
                    now.tm_sec);


    idlOS::fwrite( log, strlen(log), 1, a );
    idlOS::fclose(a);

    idlOS::printf( "i am task 1\n" );
  }

  IDE_TEST( gLock.unlock() != IDE_SUCCESS );

  return;

  IDE_EXCEPTION_END;

  printf( "run error\n" );

  return;
}

int serverTest(int argc, char **argv)
{
  Server a;

  IDE_TEST( iduMemMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
  IDE_TEST( iduMutexMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
  iduLatch::initializeStatic();

  IDE_TEST( gLock.initialize( "a", IDU_MUTEX_KIND_POSIX ) != IDE_SUCCESS );

  a.start();

  idlOS::thr_join( a.getHandle(), NULL );

  gLock.destroy();

  iduLatch::destroyStatic();
  IDE_TEST( iduMutexMgr::destroyStatic() != IDE_SUCCESS );
  IDE_TEST( iduMemMgr::destroyStatic() != IDE_SUCCESS );

  return 0;

  IDE_EXCEPTION_END;

  printf( "server error: %d\n", errno );

  return -1;
}


