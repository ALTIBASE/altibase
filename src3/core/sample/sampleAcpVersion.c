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
 * $Id: sampleAcpFile.c 9116 2009-12-10 02:03:59Z gurugio $
 ******************************************************************************/


#include <acpVersion.h>
#include <acpPrintf.h>
#include <acpInit.h>
#include <acpDir.h>
#include <acpMem.h>


acp_sint32_t main(void)
{
    (void)acpPrintf("Altibase Core version %d.%d.%d\n",
                    acpVersionMajor(),
                    acpVersionMinor(),
                    acpVersionPatch());

    return 0;
}
