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
 * $Id$
 *
 * ST에서 사용하는 System Property에 대한 정의
 * A4에서 제공하는 Property 관리자를 이용하여 처리한다.
 * 
 **********************************************************************/

#ifndef _O_STU_PROPERTY_H
#define _O_STU_PROPERTY_H 1

#include <idl.h>
#include <idp.h>

#define STU_USE_CLIPPER_LIBRARY     ( stuProperty::mUseClipperLibrary )
#define STU_CLIP_TOLERANCE          ( stuProperty::mClipTolerance )
#define STU_CLIP_TOLERANCESQ        ( stuProperty::mClipToleranceSq )
#define STU_VALIDATION_ENABLE       ( stuProperty::mGeometryValidationEnable )

#define STU_USE_CLIPPER_GPC         ((UInt)0)
#define STU_USE_CLIPPER_ALTIBASE    ((UInt)1)

#define STU_MAX_CLIP_TOLERANCE      ((UInt)9)
#define STU_DEFAULT_CLIP_TOLERANCE  ((UInt)3)

#define STU_VALIDATION_ENABLE_TRUE  ((UInt)1)
#define STU_VALIDATION_ENABLE_FALSE ((UInt)0)

class stuProperty
{
public:
    static UInt    mUseClipperLibrary;
    static SDouble mClipTolerance;
    static SDouble mClipToleranceSq;
    static UInt    mGeometryValidationEnable;
    
public:
    static IDE_RC  load();

    static idBool  useGpcLib()
    {
        return (STU_USE_CLIPPER_LIBRARY == STU_USE_CLIPPER_GPC) ? ID_TRUE : ID_FALSE;
    }
    
    //----------------------------------------------
    // Writable Property를 위한 Call Back 함수들
    //----------------------------------------------
    
    static IDE_RC  changeST_CLIP_TOLERANCE( idvSQL * aStatistics,
                                            SChar  * /* aName */,
                                            void   * /* aOldValue */,
                                            void   * aNewValue,
                                            void   * /* aArg */);

    static IDE_RC  changeST_ALLOW_INVALID_OBJECT( idvSQL * aStatistics, 
                                                  SChar  * /* aName */,
                                                  void   * /* aOldValue */,
                                                  void   * aNewValue,
                                                  void   * /* aArg */);
    
    static IDE_RC  changeST_GEOMETRY_VALIDATION_ENABLE( idvSQL * aStatistics,
                                                        SChar  * /* aName */,
                                                        void   * /* aOldValue */,
                                                        void   * aNewValue,
                                                        void   * /* aArg */);
};

#endif /* _O_STU_PROPERTY_H */

