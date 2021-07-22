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

#include <mmcTransManager.h>
#include <mmcTrans.h>
#include <mmcSession.h>
#include <mmcSharedTrans.h>
#include <mmcNonSharedTrans.h>
#include <sdi.h>


IDE_RC mmcTransManager::initializeManager()
{

    IDE_TEST( mmcSharedTrans::initialize()
              != IDE_SUCCESS );

    IDE_TEST( mmcNonSharedTrans::initialize()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTransManager::finalizeManager()
{
    IDE_TEST( mmcSharedTrans::finalize()
              != IDE_SUCCESS );

    IDE_TEST( mmcNonSharedTrans::finalize()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTransManager::allocTrans( mmcTransObj ** aTrans, mmcSession * aSession )
{
    idBool sIsSharable = ID_FALSE;

    *aTrans = NULL;

    if ( aSession != NULL )
    {
        if ( aSession->isShareableTrans() == ID_TRUE )
        {
            sIsSharable = ID_TRUE;
        }
    }

    if ( sIsSharable == ID_TRUE )
    {
        IDE_TEST( mmcSharedTrans::allocTrans( aTrans, aSession )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mmcNonSharedTrans::allocTrans( aTrans )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTransManager::freeTrans( mmcTransObj ** aTrans, mmcSession * aSession )
{
    if ( mmcTrans::isShareableTrans( *aTrans ) == ID_TRUE )
    {
        IDE_TEST( mmcSharedTrans::freeTrans( aTrans, aSession ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mmcNonSharedTrans::freeTrans( aTrans ) != IDE_SUCCESS );
    }

    *aTrans = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

