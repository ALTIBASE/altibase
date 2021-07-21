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
 * $Id: sctFT.h 15787 2006-05-19 02:26:17Z bskim $
 *
 * Description :
 *
 * 테이블스페이스 관리자
 *
 *
 **********************************************************************/

#ifndef _O_SCT_FT_H_
#define _O_SCT_FT_H_ 1

#include <sctDef.h>
#include <sdd.h>

class sctFT
{
public:

    static IDE_RC buildRecordForTableSpaceHeader(idvSQL              * /*aStatistics*/,
                                                 void                *aHeader,
                                                 void        * /* aDumpObj */,
                                                 iduFixedTableMemory *aMemory);

    static IDE_RC buildRecordForTABLESPACES(idvSQL              * /*aStatistics*/,
                                            void        *aHeader,
                                            void        * /* aDumpObj */,
                                            iduFixedTableMemory *aMemory);

private:
    // TBS State Bitset을 받아 State Name을 반환
    static void getTBSStateName( UInt aTBSStateBitset, UChar*  aTBSStateName );

    // Tablespace Header Performance View구조체의
    // Attribute Flag 필드들을 설정한다.
    static IDE_RC getSpaceHeaderAttrFlags(
                      sctTableSpaceNode * aSpaceNode,
                      sctTbsHeaderInfo  * aSpaceHeader);


    // Disk Tablespace의 정보를 Fix Table 구조체에 저장한다.
    static IDE_RC getDiskTBSInfo( sddTableSpaceNode * aDiskSpaceNode,
                                  sctTbsInfo        * aSpaceInfo );

    // Memory Tablespace의 정보를 Fix Table 구조체에 저장한다.
    static void getMemTBSInfo( smmTBSNode   * aMemSpaceNode,
                               sctTbsInfo   * aSpaceInfo );

    // Volatile Tablespace의 정보를 Fixed Table 구조체에 저장한다.
    static void getVolTBSInfo( svmTBSNode   * aVolSpaceNode,
                               sctTbsInfo   * aSpaceInfo );
};

#endif // _O_SCT_FT_H_


