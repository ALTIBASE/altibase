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
 * $Id: makeLockDecision.cpp 86294 2019-10-21 04:20:49Z et16 $
 ***********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smErrorCode.h>
#include <smuUtility.h>
#include <sml.h>
#include <smi.h>
#include <smr.h>
#include <smm.h>
#include "callback.h"

SChar        gHomeDir[256];
SChar        gConfFile[256];
const SChar  gHelpMsg[] =
"Usage: Display Lock Decision Table\n";


int main(void /*int argc, char* argv[]*/)
{
    SInt         i;
    smlLockMode  s_lockMode;
    SChar        s_strLockMode[][100] = {"SML_NLOCK",
                                         "SML_SLOCK",
                                         "SML_XLOCK",
                                         "SML_ISLOCK",
                                         "SML_IXLOCK",
                                         "SML_SIXLOCK"};
    
    smuUtility::outputUtilityHeader("makeLockDecision");

    for(i = 0; i < 64; i++)
    {
        s_lockMode = SML_NLOCK;
        
        if ( smlLockMgr::isExistLockModeMask( i, SML_SLOCK ) == ID_TRUE )
        {
            s_lockMode = smlLockMgr::getConversionLockMode( s_lockMode, SML_SLOCK );
        }

        if ( smlLockMgr::isExistLockModeMask( i, SML_XLOCK ) == ID_TRUE )

        {
            s_lockMode = smlLockMgr::getConversionLockMode( s_lockMode, SML_XLOCK );
        }

        if ( smlLockMgr::isExistLockModeMask( i, SML_ISLOCK ) == ID_TRUE )

        {
            s_lockMode = smlLockMgr::getConversionLockMode( s_lockMode, SML_ISLOCK );
        }

        if ( smlLockMgr::isExistLockModeMask( i, SML_IXLOCK ) == ID_TRUE )

        {
            s_lockMode = smlLockMgr::getConversionLockMode( s_lockMode, SML_IXLOCK );
        }

        if ( smlLockMgr::isExistLockModeMask( i, SML_SIXLOCK ) == ID_TRUE )

        {
            s_lockMode = smlLockMgr::getConversionLockMode( s_lockMode, SML_SIXLOCK );
        }

        if(i % 4 == 0)
        {
            idlOS::printf("\n");
        }
        

        idlOS::printf("%s, ", s_strLockMode[s_lockMode]);
    }

    return IDE_SUCCESS;
}

