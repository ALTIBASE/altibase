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
 

/***********************************************************************
 *
 * $Id: scpfModule.h $
 *
 **********************************************************************/

#ifndef _O_SCPF_MODULE_H_
#define _O_SCPF_MODULE_H_ 1

#include <scpDef.h>
#include <scpfDef.h>
#include <scpModule.h>

IDE_RC scpfDumpHeader( scpfHandle     * aHandle,
                       idBool           aDetail );

IDE_RC scpfDumpBlocks( idvSQL         * aStatistics,
                       scpfHandle     * aHandle, 
                       SLong            aFirstBlock,
                       SLong            aLastBlock,
                       idBool           aDetail );
 
IDE_RC scpfDumpRows( idvSQL         * aStatistics,
                     scpfHandle     * aHandle );

extern scpModule scpfModule;

#endif // _O_SCPF_MODULE_H_


