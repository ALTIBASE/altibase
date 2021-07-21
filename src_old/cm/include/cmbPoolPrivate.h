/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _O_CMB_POOL_PRIVATE_H_
#define _O_CMB_POOL_PRIVATE_H_ 1


/*
 * Alloc Support Functions Implemented by Each Subclasses
 */

IDE_RC cmbPoolMapLOCAL(cmbPool *aPool);
UInt   cmbPoolSizeLOCAL();

IDE_RC cmbPoolMapIPC(cmbPool *aPool);
UInt   cmbPoolSizeIPC();

/*PROJ-2616*/
IDE_RC cmbPoolMapIPCDA(cmbPool *aPool);
UInt   cmbPoolSizeIPCDA();

//fix PROJ-1749
IDE_RC cmbPoolMapDA(cmbPool *aPool);
UInt   cmbPoolSizeDA();

#endif
