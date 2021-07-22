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
 
 
/*****************************************************************************
 * $Id: stuProperty.cpp 29639 2008-11-27 05:38:24Z htkim $
 ****************************************************************************/
#include <idl.h>
#include <ide.h>
#include <idp.h>
#include <stuProperty.h>

UInt     stuProperty::mUseClipperLibrary;
SDouble  stuProperty::mClipTolerance;
SDouble  stuProperty::mClipToleranceSq;
UInt     stuProperty::mGeometryValidationEnable;

static SDouble clipTolerance[STU_MAX_CLIP_TOLERANCE] = {
    (SDouble) 0.0001,          // 1e-4
    (SDouble) 0.00001,         // 1e-5
    (SDouble) 0.000001,        // 1e-6
    (SDouble) 0.0000001,       // 1e-7
    (SDouble) 0.00000001,      // 1e-8
    (SDouble) 0.000000001,     // 1e-9
    (SDouble) 0.0000000001,    // 1e-10
    (SDouble) 0.00000000001,   // 1e-11
    (SDouble) 0.000000000001   // 1e-12
};

IDE_RC stuProperty::load()
{
    UInt  sTolerance;
    UInt  sAllowLevel;
    
    IDE_ASSERT( idp::read("ST_USE_CLIPPER_LIBRARY",
                          &mUseClipperLibrary) == IDE_SUCCESS );

    IDE_ASSERT( idp::read("ST_CLIP_TOLERANCE",
                          &sTolerance) == IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*) "ST_CLIP_TOLERANCE",
                  stuProperty::changeST_CLIP_TOLERANCE )
              != IDE_SUCCESS );
    if ( sTolerance < STU_MAX_CLIP_TOLERANCE )
    {
        mClipTolerance   = clipTolerance[sTolerance];
    }
    else
    {
        mClipTolerance   = clipTolerance[STU_DEFAULT_CLIP_TOLERANCE];
    }
    mClipToleranceSq = mClipTolerance * mClipTolerance;

    IDE_ASSERT( idp::read("ST_ALLOW_INVALID_OBJECT",
                          &sAllowLevel) == IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*) "ST_ALLOW_INVALID_OBJECT",
                  stuProperty::changeST_ALLOW_INVALID_OBJECT )
              != IDE_SUCCESS );
    if ( sAllowLevel == 0 )
    {
        mGeometryValidationEnable = STU_VALIDATION_ENABLE_TRUE;
    }
    else
    {
        mGeometryValidationEnable = STU_VALIDATION_ENABLE_FALSE;
    }
    
    IDE_ASSERT( idp::read("ST_GEOMETRY_VALIDATION_ENABLE",
                          &mGeometryValidationEnable) == IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*) "ST_GEOMETRY_VALIDATION_ENABLE",
                  stuProperty::changeST_GEOMETRY_VALIDATION_ENABLE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC stuProperty::changeST_CLIP_TOLERANCE( idvSQL * /* aStatistics */,
                                             SChar  * /* aName */,
                                             void   * /* aOldValue */,
                                             void   * aNewValue,
                                             void   * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    ST_CLIP_TOLERANCE를 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    UInt  sTolerance;
    
    idlOS::memcpy( & sTolerance,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    if ( sTolerance < STU_MAX_CLIP_TOLERANCE )
    {
        mClipTolerance = clipTolerance[sTolerance];
    }
    else
    {
        mClipTolerance = clipTolerance[STU_DEFAULT_CLIP_TOLERANCE];
    }

    mClipToleranceSq = mClipTolerance * mClipTolerance;
    
    return IDE_SUCCESS;
}

IDE_RC stuProperty::changeST_ALLOW_INVALID_OBJECT( idvSQL * /* aStatistics */,  
                                                   SChar  * /* aName */,
                                                   void   * /* aOldValue */,
                                                   void   * aNewValue,
                                                   void   * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    ST_ALLOW_INVALID_OBJECT를 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    UInt  sAllowLevel;
    
    idlOS::memcpy( & sAllowLevel,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    if ( sAllowLevel == 0 )
    {
        mGeometryValidationEnable = STU_VALIDATION_ENABLE_TRUE;
    }
    else
    {
        mGeometryValidationEnable = STU_VALIDATION_ENABLE_FALSE;
    }
    
    return IDE_SUCCESS;
}

IDE_RC stuProperty::changeST_GEOMETRY_VALIDATION_ENABLE( idvSQL * /* aStatistics */,
                                                         SChar  * /* aName */,
                                                         void   * /* aOldValue */,
                                                         void   * aNewValue,
                                                         void   * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    changeST_GEOMETRY_VALIDATION_ENABLE를 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & mGeometryValidationEnable,
                   aNewValue,
                   ID_SIZEOF(UInt) );
    
    return IDE_SUCCESS;
}
