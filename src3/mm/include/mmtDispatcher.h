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

#ifndef _O_MMT_DISPATCHER_H_
#define _O_MMT_DISPATCHER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idtBaseThread.h>
#include <cm.h>


class mmtDispatcher : public idtBaseThread
{
private:
    idBool             mRun;

    cmiDispatcherImpl  mDispatcherImpl;
    cmiDispatcher     *mDispatcher;

    cmiLink           *mLink[CMI_LINK_IMPL_MAX];
    UInt               mLinkCount;

public:
    mmtDispatcher();

    IDE_RC initialize(cmiDispatcherImpl aDispatcherImpl);
    IDE_RC finalize();

    void   run();
    void   stop();

    IDE_RC addLink(cmiLink *aLink);
};


#endif
