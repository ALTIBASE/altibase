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

#ifndef _O_MMD_LINK_MANAGER_H_
#define _O_MMD_LINK_MANAGER_H_ 1

#include <idl.h>
#include <qci.h>
#include <lki.h>


class mmdLinkManager
{

public:
    static qciLinkCallback mCallback;

public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC startupLinker(void *aArg);
    static IDE_RC shutdownLinker(void *aArg);
    static IDE_RC closeLink(void *aArg);
};


#endif
