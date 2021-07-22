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
 * $Id: sctTBSAlter.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SCT_TBS_ALTER_H_
#define _O_SCT_TBS_ALTER_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <smmDef.h>
#include <smu.h>

/*
   Disk/Memory/Volatile Tablespace에 공통적으로 적용되는 Alter구문의 구현
 */
class sctTBSAlter
{
public :
   // 생성자 (아무것도 안함)
    sctTBSAlter() ;

    // Tablespace Attrbute Flag의 변경 ( ex> ALTER TABLESPACE LOG COMPRESS ON )
    static IDE_RC alterTBSAttrFlag( void      * aTrans,
                                    scSpaceID   aTableSpaceID,
                                    UInt        aAttrFlagMask, 
                                    UInt        aAttrFlagNewValue );
    
private :
    
    // Tablespace Attrbute Flag의 변경에 대한 에러처리 
    static IDE_RC checkErrorOnAttrFlag( sctTableSpaceNode * aSpaceNode,
                                        UInt        aCurrentAttrFlag,
                                        UInt        aAttrFlagMask, 
                                        UInt        aAttrFlagNewValue);
    
    
};

#endif /* _O_SCT_TBS_ALTER_H_ */
