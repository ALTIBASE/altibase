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

#if !defined(_O_IDU_SHM_KEY_FILE_H_)
#define _O_IDU_SHM_KEY_FILE_H_ 1

#include <iduFile.h>

/* Shared Memory를 생성한 XDB Deamon Process의 Startup시간을
 * 가지는 파일의 이름 */
#define IDU_SHM_KEY_FILENAME PRODUCT_PREFIX"altibase_shm.info"

/* IDU_SHM_KEY_FILENAME에 기록된 데이타 구조로서
 * XDB Deamon Process의 Startup시간을 의미하고 같은 값을 두번
 * 기록한 이유는 파일이 Valid한지를 Check하기 위해서 이다. */
typedef struct iduStartupInfoOfXDB
{
    /* XDB Deamon Process의 Startup시간 */
    struct timeval mStartupTime1;
    /* mStartupTime1과 동일한 값을 가진다. */
    struct timeval mStartupTime2;
    /* 생성할 때의 SHM_DB_KEY를 갖는다. */
    UInt           mShmDBKey;
} iduStartupInfoOfXDB;

class iduShmKeyFile
{
public:
    static IDE_RC getStartupInfo( struct timeval * aStartUpTime,
                                  UInt           * aPrevShmKey );
    static IDE_RC setStartupInfo( struct timeval * aStartUpTime );
};

#endif /* _O_IDU_SHM_KEY_FILE_H_ */
