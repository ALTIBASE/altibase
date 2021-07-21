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
 * $Id: sctTBSUpdate.h 21425 2007-04-18 02:30:36Z kmkim $
 **********************************************************************/

#ifndef _O_SCT_TBS_UPDATE_H_
#define _O_SCT_TBS_UPDATE_H_ 1

#include <sct.h>

/**
   Disk/Memory/Volatile Tablespace에 모두 공통적으로 적용되는
   Alter Tablespace구문에 대한 Redo/Undo함수 구현

   Ex> Alter Tablespace Log Compress ON/OFF
 */
class sctTBSUpdate
{
//For Operation
public:
    /* Update type:  SMR_PHYSICAL */

    // Tablespace의 Attribute Flag변경에 대한 로그의 Redo수행
    static IDE_RC redo_SCT_UPDATE_ALTER_ATTR_FLAG(
                       idvSQL*              aStatistics, 
                       void*                aTrans,
                       smLSN                aCurLSN,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               /* aIsRestart */ );


    // Tablespace의 Attribute Flag변경에 대한 로그의 Undo수행
    static IDE_RC undo_SCT_UPDATE_ALTER_ATTR_FLAG(
                       idvSQL*              aStatistics, 
                       void*                aTrans ,
                       smLSN                aCurLSN,
                       scSpaceID            aSpaceID,
                       UInt                 /*aFileID*/,
                       UInt                 aValueSize,
                       SChar*               aValuePtr,
                       idBool               aIsRestart );
    
private:
    // Tablespace의 Attribute Flag변경에 대한 로그를 분석한다.
    static IDE_RC getAlterAttrFlagImage( UInt       aValueSize,
                                         SChar    * aValuePtr,
                                         UInt     * aAttrFlag );
};

#endif
