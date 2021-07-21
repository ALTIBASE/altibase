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
 * $Id:$
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smi.h>
#include <smiTableBackup.h>
#include <smr.h>

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        idlOS::printf("dumpTB table backup file name \n");

        return -1;
    }

    /* 프로퍼티 로드 */
    IDE_TEST( idp::initialize(NULL, NULL)
              != IDE_SUCCESS );

    IDE_TEST( iduProperty::load() != IDE_SUCCESS );
    
    IDE_TEST( smuProperty::load()
              != IDE_SUCCESS );
    
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST(iduCond::initializeStatic() != IDE_SUCCESS);

    idlOS::printf("Dump file: %s\n\n", argv[1]);
    
    IDE_TEST_RAISE(smiTableBackup::dump(argv[1]) != IDE_SUCCESS, 
                   err_table_backup_dump);

    idlOS::printf("\nDump complete.\n");


    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS);
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( idp::destroy() == IDE_SUCCESS );
    
    return 0;

    IDE_EXCEPTION(err_table_backup_dump);
    {
    }
    IDE_EXCEPTION_END;

    return -1;
}
