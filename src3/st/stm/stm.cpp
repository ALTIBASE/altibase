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
 * $Id: stm.cpp 16545 2006-06-07 01:06:51Z laufer $
 **********************************************************************/

#include <stm.h>

/* BUG-47809 USER_SRS meta handle */
const void * gStmUserSrs;
const void * gStmUserSrsIndex[STM_MAX_META_INDICES];

IDE_RC
stm::initializeGlobalVariables( void )
{
    SInt              i;

    gStmUserSrs = NULL;

    for (i = 0; i < STM_MAX_META_INDICES; i ++)
    {
        gStmUserSrsIndex[i] = NULL;
    }

    return IDE_SUCCESS;
}

IDE_RC
stm::initMetaHandles( smiStatement * aSmiStmt )
{
    /**************************************************************
                        Get Table Handles
    **************************************************************/
    /* BUG-47809 USER_SRS meta handle */
    IDE_TEST( qcm::getMetaTable( STM_USER_SRS,
                                 &gStmUserSrs,
                                 aSmiStmt ) != IDE_SUCCESS );
    
    /**************************************************************
                        Get Index Handles
    **************************************************************/
    /* BUG-47809 USER_SRS meta handle */
    IDE_TEST( qcm::getMetaIndex( gStmUserSrsIndex,
                            gStmUserSrs ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stm::makeAndSetStmColumnInfo( void * /* aSmiStmt */,
                                     void * /* aTableInfo */,
                                     UInt /* aIndex */)
{
    return IDE_SUCCESS;

    // IDE_EXCEPTION_END;

    // return IDE_FAILURE;
}                                     
