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
 * $Id: scpReq.h 39159 2010-04-27 01:47:47Z anapple $
 **********************************************************************/


#ifndef _O_SCP_REQ_H_
#define _O_SCP_REQ_H_ 1

#include <idl.h> /* for win32 porting */
#include <smiAPI.h>

class scpReqFunc
{
    public:

        /* smi */
        static void setEmergency( idBool aFlag )
        {
            smiSetEmergency( aFlag );
        };
};

#define smLayerCallback    scpReqFunc

#endif
