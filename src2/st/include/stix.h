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
 

/******************************************************************************
 * $Id:  $
 *****************************************************************************/

#ifndef _O_STIX_H_
#define _O_STIX_H_ 1

#include <qcm.h>

//-----------------------------------------
// Extern Module Definition
//-----------------------------------------
#define STI_MAX_CONVERSION_MODULES    (16)
#define STI_MAX_DATA_TYPE_MODULES     (2)

extern mtdModule * stdModules[STI_MAX_DATA_TYPE_MODULES];
extern mtvModule * stvModules[STI_MAX_CONVERSION_MODULES];
extern mtfModule * stfModules[];
extern smiCallBackFunc stkRangeFuncs[];
extern mtdCompareFunc  stkCompareFuncs[];

extern qcmExternModule gStiModule;

class stix
{
private:

public:

    static IDE_RC addExtSM_Recovery ( void );

    static IDE_RC addExtSM_Index    ( void );

    static IDE_RC addExtMT_Module   ( void );

    static IDE_RC addExtQP_Callback ( void );

};

#endif /* _O_STIX_H_ */

