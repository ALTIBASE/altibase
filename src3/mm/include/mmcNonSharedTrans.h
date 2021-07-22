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

#ifndef _O_MMC_NON_SHARED_TRANS_H_
#define _O_MMC_NON_SHARED_TRANS_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>

class mmcTransObj;
class mmcSession;

class mmcNonSharedTrans
{
public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC allocTrans( mmcTransObj ** aTrans );
    static IDE_RC freeTrans( mmcTransObj ** aTrans );

private:
    static void initTransInfo( mmcTransObj * aTransObj );
    static void finiTransInfo( mmcTransObj * aTransObj );

private:
    static iduMemPool mPool;
};

#endif  // _O_MMC_NON_SHARED_TRANS_H_
