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

#ifndef   _O_MMC_CONVFMMT_H_
#define   _O_MMC_CONVFMMT_H_  1

#include <idl.h>
#include <cm.h>
#include <mmcDef.h>

class mmcSession;

class mmcConvFromMT
{
public:
    static IDE_RC convert(cmiProtocolContext *aProtocolContext,
                          cmtAny             *aTarget,
                          void               *aSource,
                          UInt                aSourceType,
                          mmcSession         *aSession,
                          UInt                aLobSize);
    
    static IDE_RC buildAny(cmtAny             *aTarget,
                           void               *aSource,
                           UInt                aSourceType,
                           mmcSession         *aSession,
                           UInt                aLobSize);

    // PROJ-2256
    static IDE_RC checkDataRedundancy( mmcBaseRow    *aBaseRow,
                                       qciBindColumn *aMetaData,
                                       void          *aData,
                                       cmtAny        *aAny );
    // PROJ-2331
    static IDE_RC compareMtChar( mmcBaseRow    *aBaseRow,
                                 qciBindColumn *aMetaData,
                                 mtdCharType   *aData );
    // PROJ-2331
    static IDE_RC compareMtChar( mmcBaseRow    *aBaseRow,
                                 qciBindColumn *aMetaData,
                                 void          *aData,
                                 cmtAny        *aAny );
};
#endif
