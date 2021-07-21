/** 
 *  Copyright (c) 1999~2020, Altibase Corp. and/or its affiliates. All rights reserved.
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
#include <ide.h>
#include <idp.h>
#include <idu.h>

#include <dumpcommon.h>

void printCRC()
{
    UInt    sTotal = 0;
    idBool  sRet   = ID_FALSE;

    sRet = getSimpleCRC(&sTotal);

    if( sRet == ID_TRUE )
    {
        idlOS::printf("#define VALID_CRC %u\n",sTotal );
    }
}

SInt main(void)
{
    printCRC();
    
    return 0;
}
