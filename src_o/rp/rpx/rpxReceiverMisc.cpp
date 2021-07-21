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
 * $Id: rpxReceiverMisc.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>

#include <qci.h>

#include <rpDef.h>
#include <rpuProperty.h>
#include <rpxReceiver.h>

//===================================================================
//
// Name:          isYou
//
// Return Value:  idBool
//
// Argument:
//
// Called By:     rpcExecutor.cpp
//
// Description:
//
//===================================================================
idBool rpxReceiver::isYou(const SChar* aRepName )
{
    return ( idlOS::strcmp( mMeta.mReplication.mRepName, aRepName ) == 0 )
        ? ID_TRUE : ID_FALSE;
}
