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
 * $Id: qcmTrigger.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 *     [PROJ-1359] Trigger
 *
 *     Trigger를 위한 Meta 및 Cache 관리를 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QCM_TRIGGER_H_
#define _O_QCM_TRIGGER_H_  1

#include <qcm.h>

// 100개의 문자열을 저장할 때,
// 다음과 같은 문장이 존재하는 경우
#define QCM_TRIGGER_SUBSTRING_LEN                  (100)
#define QCM_TRIGGER_SUBSTRING_LEN_STR              "100"

class qcmTrigger
{
public:

    // Trigger OID 와 Table ID의 획득
    static IDE_RC getTriggerOID( qcStatement    * aStatement,
                                 UInt             aUserID,
                                 qcNamePosition   aTriggerName,
                                 smOID          * aTriggerOID,
                                 UInt           * aTableID,
                                 idBool         * aIsExist );

    // Trigger Cycle의 존재 여부 검사
    // aTableID의 Trigger가 접근하는 Table중 aCycleTableID가
    // 존재할 경우 Cycle이 존재함.
    static IDE_RC checkTriggerCycle( qcStatement * aStatement,
                                     UInt          aTableID,
                                     UInt          aCycleTableID );

    // Meta Table에 Trigger 정보를 등록한다.
    static IDE_RC addMetaInfo( qcStatement * aStatement,
                               void        * sTriggerHandle );

    // Meta Table에서 Trigger 정보를 삭제한다.
    static IDE_RC removeMetaInfo( qcStatement * aStatement,
                                  smOID         aTriggerOID );

    // Meta Table로부터 Table Cache의 Trigger 정보를 설정한다.
    static IDE_RC getTriggerMetaCache( smiStatement * aSmiStmt,
                                       UInt           aTableID,
                                       qcmTableInfo * aTableInfo );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC copyTriggerMetaInfo( qcStatement * aStatement,
                                       UInt          aToTableID,
                                       smOID         aToTriggerOID,
                                       SChar       * aToTriggerName,
                                       UInt          aFromTableID,
                                       smOID         aFromTriggerOID,
                                       SChar       * aDDLText,
                                       SInt          aDDLTextLength );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC updateTriggerStringsMetaInfo( qcStatement * aStatement,
                                                UInt          aTableID,
                                                smOID         aTriggerOID,
                                                SInt          aDDLTextLength );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC insertTriggerStringsMetaInfo( qcStatement * aStatement,
                                                UInt          aTableID,
                                                smOID         aTriggerOID,
                                                SChar       * aDDLText,
                                                SInt          aDDLTextLength );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC removeTriggerStringsMetaInfo( qcStatement * aStatement,
                                                smOID         aTriggerOID );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC renameTriggerMetaInfoForSwap( qcStatement    * aStatement,
                                                qcmTableInfo   * aSourceTableInfo,
                                                qcmTableInfo   * aTargetTableInfo,
                                                qcNamePosition   aNamesPrefix );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC swapTriggerDMLTablesMetaInfo( qcStatement * aStatement,
                                                UInt          aTargetTableID,
                                                UInt          aSourceTableID );

private:

};
#endif // _O_QCM_TRIGGER_H_
