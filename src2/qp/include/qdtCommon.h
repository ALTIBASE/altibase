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
 * $Id: qdtCommon.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef  _O_QDT_COMMON_H_
#define  _O_QDT_COMMON_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

typedef struct qdtTBSAttrValueMap 
{
    SChar * mValueString;
    UInt    mValue;
} qcpTBSAttrValueMap ;

#define QCP_TBS_ATTR_MAX_VALUE_MAP_COUNT (2)

typedef struct qdtTBSAttrMap 
{
    SChar * mName;
    UInt    mMask;
    qcpTBSAttrValueMap mValueMap[QCP_TBS_ATTR_MAX_VALUE_MAP_COUNT+1];
} qdtTBSAttrMap;


class qdtCommon
{
public:
    static IDE_RC validateFilesSpec( qcStatement       * aStatement,
                                     smiTableSpaceType   aType,
                                     qdTBSFilesSpec    * aFilesSpec,
                                     ULong               aExtentSize,
                                     UInt              * aDataFileCount);

    // To Fix PR-10437
    // INDEX TABLESPACE tbs_name 에 대한 Validation
    static IDE_RC getAndValidateIndexTBS( qcStatement       * aStatement,
                                          scSpaceID           aTableTBSID,
                                          smiTableSpaceType   aTableTBSType,
                                          qcNamePosition      aIndexTBSName,
                                          UInt                aIndexOwnerID,
                                          scSpaceID         * aIndexTBSID,
                                          smiTableSpaceType * aIndexTBSType );
    
    static IDE_RC getAndValidateTBSOfIndexPartition( 
                                          qcStatement       * aStatement,
                                          scSpaceID           aTableTBSID,
                                          smiTableSpaceType   aTableTBSType,
                                          qcNamePosition      aIndexTBSName,
                                          UInt                aIndexOwnerID,
                                          scSpaceID         * aIndexTBSID,
                                          smiTableSpaceType * aIndexTBSType );

    // 여러 개의 Attribute Flag List의 Flag값을
    // Bitwise Or연산 하여 하나의 UInt 형의 Flag 값을 만든다
    static IDE_RC getTBSAttrFlagFromList(qdTBSAttrFlagList * aAttrFlagList,
                                         UInt              * aAttrFlag );

    // Tablespace의 Attribute Flag List에 대한 Validation수행
    static IDE_RC validateTBSAttrFlagList(qcStatement       * aStatement,
                                          qdTBSAttrFlagList * aAttrFlagList);

    // BUG-40728 동일 tbs type인지 검사한다.
    static idBool isSameTBSType( smiTableSpaceType  aTBSType1,
                                 smiTableSpaceType  aTBSType2 );
    
private:
    // Tablespace의 Attribute Flag List에서 동일한
    // Attribute List가 존재할 경우 에러처리
    static IDE_RC checkTBSAttrIsUnique(qcStatement       * aStatement,
                                       qdTBSAttrFlagList * aAttrFlagList);
    
};

#endif  //  _O_QDT_COMMON_H_
