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
 * $Id$ sdiFailoverTypeStorage.cpp
 **********************************************************************/

#include <idl.h>
#include <sdi.h>
#include <sdl.h>
#include <sdiFailoverTypeStorage.h>

void sdiFailoverTypeStorage::initialize()
{
    clearReports();
}

void sdiFailoverTypeStorage::finalize()
{
    /* Nothing to do */
}

inline void sdiFailoverTypeStorage::clearReports()
{
    SInt sIndex;

    mStorage.mMaxIndex = -1;

    for ( sIndex = 0; sIndex < SDI_NODE_MAX_COUNT; ++sIndex )
    {
        mStorage.mNodeId[ sIndex ] = SDI_CLIENT_NODE_ID_NONE;
    }

    for ( sIndex = 0; sIndex < SDI_NODE_MAX_COUNT; ++sIndex )
    {
        mStorage.mDestination[ sIndex ] = SDI_FAILOVER_NOT_USED;
    }
}

IDE_RC sdiFailoverTypeStorage::setClientConnectionStatus( UInt aNodeId, UChar aDestination )
{
    SInt                sIndex;
    sdiFailOverTarget   sFailoverTarget;

    sFailoverTarget = (sdiFailOverTarget)aDestination;

    IDE_TEST_RAISE( ( sFailoverTarget != SDI_FAILOVER_ACTIVE_ONLY ) &&
                    ( sFailoverTarget != SDI_FAILOVER_ALTERNATE_ONLY ),
                    ERR_NOT_SUPPORT_DESTINATION_VALUE );

    for ( sIndex = 0; sIndex < SDI_NODE_MAX_COUNT; ++sIndex )
    {
        if ( mStorage.mDestination[ sIndex ] != SDI_FAILOVER_NOT_USED )
        {
            /* setted values */
            if ( mStorage.mNodeId[ sIndex ] == aNodeId )
            {
                break;
            }
        }
        else
        {
            /* not setted value. use this index. */
            break;
        }
    }

    IDU_FIT_POINT_RAISE( "sdiFailoverTypeStorage::setClientConnectionStatus::storageOverlfow",
                         ERR_STORAGE_OVERFLOW );
    IDE_TEST_RAISE( sIndex >= SDI_NODE_MAX_COUNT,
                    ERR_STORAGE_OVERFLOW );

    if ( mStorage.mDestination[ sIndex ] != SDI_FAILOVER_NOT_USED )
    {
        IDE_DASSERT( mStorage.mNodeId[ sIndex ] == aNodeId );

        mStorage.mDestination[ sIndex ] = (UChar)sFailoverTarget;
    }
    else
    {
        IDE_DASSERT( mStorage.mNodeId[ sIndex ] == SDI_CLIENT_NODE_ID_NONE );

        mStorage.mNodeId     [ sIndex ] = aNodeId;
        mStorage.mDestination[ sIndex ] = (UChar)sFailoverTarget;
    }

    if ( sIndex > mStorage.mMaxIndex )
    {
        mStorage.mMaxIndex = sIndex;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_DESTINATION_VALUE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdiFailoverTypeStorage::setClientConnectionStatus",
                                  "Invalid destination values" ) );
        ideLog::log( IDE_SD_5, "[SHARD_FAILOVER_ERROR] [%s] ERR-%"ID_XINT32_FMT" %s\n"
                               "     Invalid value = %"ID_INT32_FMT"\n",
                               "sdiFailoverTypeStorage::setClientConnectionStatus",
                               E_ERROR_CODE( ideGetErrorCode() ),
                               ideGetErrorMsg(),
                               sFailoverTarget );
        IDE_DASSERT( 0 );
    }
    IDE_EXCEPTION( ERR_STORAGE_OVERFLOW );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdiFailoverTypeStorage::setClientConnectionStatus",
                                  "Destination storage overflow" ) );
        ideLog::log( IDE_SD_5, "[SHARD_FAILOVER_ERROR] [%s] ERR-%"ID_XINT32_FMT" %s\n"
                               "     NODE_ID = %"ID_INT32_FMT", DST = %"ID_INT32_FMT"\n",
                               "sdiFailoverTypeStorage::setClientConnectionStatus",
                               E_ERROR_CODE( ideGetErrorCode() ),
                               ideGetErrorMsg(),
                               aNodeId,
                               aDestination );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

sdiFailOverTarget sdiFailoverTypeStorage::getClientConnectionStatus( UInt aNodeId )
{
    SInt                sIndex;
    SInt                sMaxIndex = mStorage.mMaxIndex;
    sdiFailOverTarget   sFailoverTarget = SDI_FAILOVER_ACTIVE_ONLY;

    for ( sIndex = 0;
          ( sIndex < SDI_NODE_MAX_COUNT ) && ( sIndex <= sMaxIndex ); 
          ++sIndex )
    {
        if ( mStorage.mDestination[ sIndex ] != SDI_FAILOVER_NOT_USED )
        {
            /* setted values */
            if ( mStorage.mNodeId[ sIndex ] == aNodeId )
            {
                sFailoverTarget = (sdiFailOverTarget)mStorage.mDestination[ sIndex ];
                break;
            }
        }
    }

    return sFailoverTarget;
}
