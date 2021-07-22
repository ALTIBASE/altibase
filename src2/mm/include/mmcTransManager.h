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

#ifndef _O_MMC_TRANS_MANAGER_H_
#define _O_MMC_TRANS_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>

#include <mmcDef.h>
#include <sdi.h>

class mmcTransObj;
class mmcSession;

class mmcTransManager
{
public:
    static IDE_RC initializeManager();
    static IDE_RC finalizeManager();
    static IDE_RC allocTrans( mmcTransObj ** aTrans, mmcSession * aSession );
    static IDE_RC freeTrans( mmcTransObj ** aTrans, mmcSession * aSession );
};

#endif  // _O_MMC_TRANS_MANAGER_H_
