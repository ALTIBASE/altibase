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

#include <mmm.h>
#include <sti.h>
#include <qci.h>
#include <dki.h>
#include <sdi.h>

static IDE_RC mmmPhaseActionInitMT(mmmPhase         /*aPhase*/,
                                   UInt             /*aOptionflag*/,
                                   mmmPhaseAction * /*aAction*/)
{
    mtcExtCallback sExtCallBack;

    sExtCallBack.getColumn               = smiGetVarColumn;
    sExtCallBack.openLobCursorWithRow    =
        (mtcOpenLobCursorWithRow)smiLob::openLobCursorWithRow;
    sExtCallBack.openLobCursorWithGRID   =
        (mtcOpenLobCursorWithGRID)smiLob::openLobCursorWithGRID;
    sExtCallBack.readLob                 = smiLob::read;
    sExtCallBack.getLobLengthWithLocator = smiLob::getLength;
    sExtCallBack.isNullLobColumnWithRow  = smiIsNullLobColumn;

    // PROJ-1579 NCHAR
    sExtCallBack.getDBCharSet            = smiGetDBCharSet;
    sExtCallBack.getNationalCharSet      = smiGetNationalCharSet;

    // PROJ-2264 Dictionary table
    sExtCallBack.getCompressionColumn    = smiGetCompressionColumn;

    // PROJ-2446
    sExtCallBack.getStatistics           = qciMisc::getStatistics;

    // Stored Procedure Function을 MT에 등록
    IDE_TEST( qciMisc::addExtFuncModule() != IDE_SUCCESS );

    IDE_TEST( qciMisc::addExtRangeFunc() != IDE_SUCCESS );

    IDE_TEST( qciMisc::addExtCompareFunc() != IDE_SUCCESS );
    
    // Spatio-Temporal 관련 DataType, Conversion, Function 등록
    IDE_TEST( sti::addExtMT_Module() != IDE_SUCCESS );

    /* PROJ-2638 shard native linker */
    IDE_TEST( sdi::addExtMT_Module() != IDE_SUCCESS );

    /* Database Link's Functions */
    IDE_TEST( dkiAddDatabaseLinkFunctions() != IDE_SUCCESS );
              
    // MT 초기화
    IDE_TEST( mtc::initialize( &sExtCallBack )
              != IDE_SUCCESS );

    // ST 초기화
    IDE_TEST( sti::initialize() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitMT =
{
    (SChar *)"Initialize MT Module",
    0,
    mmmPhaseActionInitMT
};

 
