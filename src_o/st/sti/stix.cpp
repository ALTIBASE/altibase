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
 * $Id: stix.cpp 17938 2006-09-05 00:54:03Z leekmo $
 **********************************************************************/

#include <smnManager.h>
#include <mtc.h>
#include <stnmrModule.h>
#include <stm.h>
#include <stnIndexTypes.h>
#include <stix.h>
#include <qci.h>
#include <sdrUpdate.h>
#include <stndrUpdate.h>

mtdModule * stdModules[STI_MAX_DATA_TYPE_MODULES];
mtvModule * stvModules[STI_MAX_CONVERSION_MODULES];

extern mtvModule stvNull2Geometry;
extern mtvModule stvBinary2Geometry;
extern mtvModule stvGeometry2Binary;
extern mtvModule stvUndef2Geometry;

extern mtdModule stdGeometry;

qcmExternModule  gStiModule;

IDE_RC
stix::addExtSM_Recovery( void )
{
    // << PROJ-1591: Disk Spatial Index
    sdrUpdate::appendExternalRefNTAUndoFunction( SDR_STNDR_INSERT_KEY,
                                                 stndrUpdate::undo_SDR_STNDR_INSERT_KEY );
    sdrUpdate::appendExternalRefNTAUndoFunction( SDR_STNDR_DELETE_KEY_WITH_NTA,
                                                 stndrUpdate::undo_SDR_STNDR_DELETE_KEY_WITH_NTA );

    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_INSERT_INDEX_KEY,
                                           stndrUpdate::redo_SDR_STNDR_INSERT_INDEX_KEY );
    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_UPDATE_INDEX_KEY,
                                           stndrUpdate::redo_SDR_STNDR_UPDATE_INDEX_KEY );
    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_FREE_INDEX_KEY,
                                           stndrUpdate::redo_SDR_STNDR_FREE_INDEX_KEY );
    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_INSERT_KEY,
                                           stndrUpdate::redo_SDR_STNDR_INSERT_KEY );
    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_DELETE_KEY_WITH_NTA,
                                           stndrUpdate::redo_SDR_STNDR_DELETE_KEY_WITH_NTA );
    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_FREE_KEYS,
                                           stndrUpdate::redo_SDR_STNDR_FREE_KEYS );
    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_COMPACT_INDEX_PAGE,
                                           stndrUpdate::redo_SDR_STNDR_COMPACT_INDEX_PAGE );
    sdrUpdate::appendExternalRedoFunction( SDR_STNDR_KEY_STAMPING,
                                           stndrUpdate::redo_SDR_STNDR_KEY_STAMPING );
    
    // >> PORJ-1591

    return IDE_SUCCESS;
}

IDE_RC
stix::addExtSM_Index( void )
{
    IDE_TEST( smnManager::appendIndexType( & gStmnRtreeIndex )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
stix::addExtMT_Module( void )
{
    stdModules[0] = &stdGeometry;
    stdModules[1] = NULL;
    
    stvModules[0] = &stvNull2Geometry;

    // To Fix PR-15398, PR-15722
    stvModules[1] = &stvBinary2Geometry;
    stvModules[2] = &stvGeometry2Binary;
    stvModules[3] = &stvUndef2Geometry;
    
    stvModules[4] = NULL;
    
    IDE_TEST( mtc::addExtTypeModule( stdModules ) != IDE_SUCCESS );

    IDE_TEST( mtc::addExtCvtModule(  stvModules ) != IDE_SUCCESS );

    IDE_TEST( mtc::addExtFuncModule( stfModules ) != IDE_SUCCESS );

    IDE_TEST( mtc::addExtRangeFunc( stkRangeFuncs ) != IDE_SUCCESS );

    IDE_TEST( mtc::addExtCompareFunc( stkCompareFuncs ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
stix::addExtQP_Callback( void )
{
    gStiModule.mInitGlobalHandle = stm::initializeGlobalVariables;
    gStiModule.mInitMetaHandle = stm::initMetaHandles;

    IDE_TEST( qciMisc::addExternModuleCallback( & gStiModule ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

