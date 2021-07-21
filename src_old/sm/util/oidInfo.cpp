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
 * $Id: oidInfo.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#include <idl.h>
#include <smDef.h>

int main(int argc, char* argv[])
{
    smOID    s_OID;
    
    if(argc != 2)
    {
        idlOS::printf("Invalid Argument\n");
    }

    s_OID = atol(argv[1]);
    
    idlOS::printf("PID: %d, OFFSET: %d\n",
                  SM_MAKE_PID(s_OID), SM_MAKE_OFFSET(s_OID));
    return 1;
}

