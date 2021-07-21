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

#include <idu.h>
#include <mmuOS.h>
/* fix BUG-30485  Heapmin System function should be called in aix 6.x  */
#if defined(IBM_AIX) && (OS_MAJORVER >= 5)
#include <extension.h> /* for use of _heapmin() function */
#endif /* defined(IBM_AIX) && (OS_MAJORVER >=  5) */

SInt mmuOS::heapmin()
{
    SChar sBuffer[1024];

    // fix BUG-37960
    IDE_TEST_RAISE( iduMemMgr::shrinkAllAllocators() != IDE_SUCCESS,
                    shrink_allocator_error );

/* BUG-48047 ALTIBASE_HEAPMIN */
/* it must match IDU_ALTIBASE_VERSION_STRING */
#if defined(IBM_AIX) && (OS_MAJORVER >= 5) && defined(ALTIBASE_HEAPMIN)
    /* BUG-32748 compile error of MM module at AIX platfom */
    #ifdef _heapmin
    #undef _heapmin
    #endif
    (void)::_heapmin();
#elif defined(OS_LINUX_KERNEL)
    (void)::malloc_trim(0);
#endif

    return 0;

    IDE_EXCEPTION( shrink_allocator_error )
    {
        idlOS::snprintf( sBuffer,
                         ID_SIZEOF(sBuffer),
                         " ** shrinkAllAllocators() ** ERROR. but, proceeding.. (errno=%d)",
                         errno );

        IDE_WARNING(IDE_SERVER_3, sBuffer );
    }
    IDE_EXCEPTION_END;

    return 1;
}

