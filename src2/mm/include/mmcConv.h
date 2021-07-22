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

#ifndef _O_MMC_CONV_H_
#define _O_MMC_CONV_H_ 1

#include <idl.h>
#include <cm.h>
#include <qci.h>

class mmcSession;

const SChar * getMtTypeName( const UInt aMtTypeName);

typedef IDE_RC (*mmcConvFuncFromCM)(qciBindParam *aTargetType,
                                    void         *aTarget,
                                    UInt          aTargetSize,
                                    cmtAny       *aSource,
                                    void         *aTemplate,
                                    mmcSession   *aSession);


class mmcConv
{
private:
    static mmcConvFuncFromCM mConvFromCM[CMT_ID_MAX];

public:
    static IDE_RC convertFromCM(qciBindParam *aTargetType,
                                void         *aTarget,
                                UInt          aTargetSize,
                                cmtAny       *aSource,
                                void         *aTemplate,
                                mmcSession   *aSession);
    
    static IDE_RC convertFromMT(cmiProtocolContext *aProtocolContext,
                                cmtAny             *aTarget,
                                void               *aSource,
                                UInt                aSourceType,
                                mmcSession         *aSession);
    
    static IDE_RC buildAnyFromMT(cmtAny             *aTarget,
                                 void               *aSource,
                                 UInt                aSourceType,
                                 mmcSession         *aSession);
};


inline IDE_RC mmcConv::convertFromCM(qciBindParam *aTargetType,
                                     void         *aTarget,
                                     UInt          aTargetSize,
                                     cmtAny       *aSource,
                                     void         *aTemplate,
                                     mmcSession   *aSession)
{
    return mConvFromCM[cmtAnyGetType(aSource)](aTargetType,
                                               aTarget,
                                               aTargetSize,
                                               aSource,
                                               aTemplate,
                                               aSession);
}


#endif
