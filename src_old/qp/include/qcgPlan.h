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
 * $Id: qcgPlan.h 82691 2018-04-03 06:25:10Z andrew.shin $
 *
 * Description :
 *     PROJ-1436 SQL Plan Cache를 위한 자료 구조 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QCG_PLAN_H_
#define _O_QCG_PLAN_H_ 1

#include <qci.h>
#include <qc.h>
#include <qs.h>

//////////////////////////////////////////////////////
// BUG-38148 프로퍼티 추가작업을 단순화 시켜야 합니다.
//////////////////////////////////////////////////////
// qcgPlan::isMatchedPlanProperty only
#define QCG_MATCHED_PLAN_PROPERTY( aRef, aValue, aSetValue )            \
{                                                                       \
    ++sMatchedCount;                                                    \
    if ( (aPlanProperty->aRef) == ID_TRUE )                             \
    {                                                                   \
        if ( (sProperty->aRef) == ID_FALSE )                            \
        {                                                               \
            (sProperty->aRef)   = ID_TRUE;                              \
            (sProperty->aValue) = (aSetValue);                          \
        }                                                               \
        else                                                            \
        {                                                               \
        }                                                               \
                                                                        \
        if ( (aPlanProperty->aValue) != (sProperty->aValue) )           \
        {                                                               \
            sIsMatched = ID_FALSE;                                      \
            IDE_CONT( NOT_MATCHED );                                    \
        }                                                               \
        else                                                            \
        {                                                               \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
    }                                                                   \
}
// qcgPlan::rebuildPlanProperty only
#define QCG_REBUILD_PLAN_PROPERTY( aRef, aValue, aSetValue )            \
{                                                                       \
    ++sRebuildCount;                                                    \
    if ( (aPlanProperty->aRef) == ID_TRUE )                             \
    {                                                                   \
        (aPlanProperty->aValue) = (aSetValue);                          \
    }                                                                   \
    else                                                                \
    {                                                                   \
    }                                                                   \
}
// qcgPlan::registerPlanProperty only
#define QCG_REGISTER_PLAN_PROPERTY( aRef, aValue, aSetValue )           \
{                                                                       \
    if ( (sProperty->aRef) == ID_FALSE )                                \
    {                                                                   \
        (sProperty->aRef)   = ID_TRUE;                                  \
        (sProperty->aValue) = (aSetValue);                              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        IDE_DASSERT( (sProperty->aValue) == (aSetValue) );              \
    }                                                                   \
}

//---------------------------------------------------
// [qcgPlanObject]
// plan 생성에 관련한 모든 database 객체
//---------------------------------------------------

// table, view, queue, dblink
typedef struct qcgEnvTableList
{
    void            * tableHandle;
    smSCN             tableSCN;

    /* BUG-45893 */
    idBool            mIsDBAUser;

    qcgEnvTableList * next;
} qcgEnvTableList;

typedef struct qcgEnvSequenceList
{
    void               * sequenceHandle;
    smSCN                sequenceSCN; /* Slot 재활용 여부를 확인하기 위한 SCN */
    qcgEnvSequenceList * next;
} qcgEnvSequenceList;

// procedure, function, typeset
typedef struct qcgEnvProcList
{
    void            * procHandle;
    smSCN             procSCN; /* Slot 재활용 여부를 확인하기 위한 SCN */
    qsOID             procID;
    qsProcParseTree * planTree;
    UInt              modifyCount;
    qcgEnvProcList  * next;
} qcgEnvProcList;

typedef struct qcgEnvSynonymList
{
    // synonym 정보
    SChar         userName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar         objectName[QC_MAX_OBJECT_NAME_LEN + 1];
    idBool        isPublicSynonym;

    // synonym이 가르키는 최종 객체의 정보
    // (table, view, queue, dblink, sequence, proc 중 하나임)
    void        * tableHandle;
    void        * sequenceHandle;
    void        * objectHandle;
    qsOID         objectID;
    smSCN         objectSCN; /* Slot 재활용 여부를 확인하기 위한 SCN */
    
    qcgEnvSynonymList  * next;
    
} qcgEnvSynonymList;

typedef struct qcgEnvRemoteTableColumnList
{
    SChar  columnName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt   dataTypeId;
    SInt   precision;
    SInt   scale;

    qcgEnvRemoteTableColumnList * next;
    
} qcgEnvRemoteTableColumnList;

// PROJ-1073 Package
typedef struct qcgEnvPkgList
{
    void           * pkgHandle;
    smSCN            pkgSCN; /* Slot 재활용 여부를 확인하기 위한 SCN */
    qsOID            pkgID;
    qsPkgParseTree * planTree;           // BUG-37250
    UInt             modifyCount;
    qcgEnvPkgList  * next;
} qcgEnvPkgList;

typedef struct qcgPlanObject
{
    qcgEnvTableList       * tableList;
    qcgEnvSequenceList    * sequenceList;
    qcgEnvProcList        * procList;
    qcgEnvSynonymList     * synonymList;
    qcgEnvPkgList         * pkgList;             // PROJ-1073 Package
} qcgPlanObject;

//---------------------------------------------------
// [qcgPlanPrivilege]
// plan 생성에 관련한 객체들에 대한 필요한 권한
//---------------------------------------------------

typedef struct qcgEnvTablePrivList
{
    // 대상 객체
    void                 * tableHandle;
    UInt                   tableOwnerID;
    SChar                  tableName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    UInt                   privilegeCount;
    struct qcmPrivilege  * privilegeInfo;  // cached meta의 복사본
    
    // 필요한 권한 (reference, insert, update, delete)
    UInt                   privilegeID;

    qcgEnvTablePrivList  * next;
} qcgEnvTablePrivList;

typedef struct qcgEnvSequencePrivList
{
    // 대상 객체
    UInt   sequenceOwnerID;
    UInt   sequenceID;  // meta table ID
    SChar  name[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    // 필요한 권한 (시퀀스에 대한 권한은 오직 select 뿐이다.)
    //UInt  privilegeID; 
    
    qcgEnvSequencePrivList * next;
} qcgEnvSequencePrivList;

typedef struct qcgEnvProcPrivList
{
    // 대상 객체
    qsOID       procID;
    
    // 필요한 권한 (proc에 대한 권한은 오직 execute 뿐이다.)
    //UInt        privilegeID;
    
    qcgEnvProcPrivList * next;
} qcgEnvProcPrivList;

typedef struct qcgPlanPrivilege
{
    qcgEnvTablePrivList    * tableList;
    qcgEnvSequencePrivList * sequenceList;
    
    // procPrivList는 procList와 동일하고 추가로 기록할 정보가
    // 없으므로 기록하지 않는다.
    //qcgEnvProcPrivList     * procList;
} qcgPlanPrivilege;

//---------------------------------------------------
// [qcgPlanEnv]
// plan 생성에 관련한 모든 기록 (Plan Environment)
//---------------------------------------------------

typedef struct qcgPlanEnv
{
    // plan 생성에 관련한 모든 property
    qcPlanProperty         planProperty;

    // plan 생성에 관련한 모든 객체
    qcgPlanObject          planObject;

    // plan 생성에 관련한 모든 권한
    qcgPlanPrivilege       planPrivilege;

    // plan을 생성한 user
    UInt                   planUserID;

    // PROJ-2163 : plan 생성에 관련한 바인드 정보
    qcPlanBindInfo         planBindInfo;

    // PROJ-2223 audit
    qciAuditInfo           auditInfo;
    
} qcgPlanEnv;


class qcgPlan
{
public:

    //-------------------------------------------------
    // plan environment 생성 함수
    //-------------------------------------------------

    // plan environment를 생성하고 초기화한다.
    static IDE_RC allocAndInitPlanEnv( qcStatement * aStatement );

    // plan property를 초기화한다.
    static void   initPlanProperty( qcPlanProperty * aProperty );

    // 간접 참조 시작을 기록한다.
    static void   startIndirectRefFlag( qcStatement * aStatement,
                                        idBool      * aIndirectRef );

    // 간접 참조 종료를 기록한다.
    static void   endIndirectRefFlag( qcStatement * aStatement,
                                      idBool      * aIndirectRef );

    // plan이 property를 참조함을 기록한다.
    static void   registerPlanProperty( qcStatement        * aStatement,
                                        qcPlanPropertyKind   aPropertyID );

    // plan이 table 객체를 참조함을 기록한다.
    static IDE_RC registerPlanTable( qcStatement  * aStatement,
                                     void         * aTableHandle,
                                     smSCN          aTableSCN,
                                     UInt           aTableOwnerID, /* BUG-45893 */
                                     SChar        * aTableName );  /* BUG-45893 */

    // plan이 sequence 객체를 참조함을 기록한다.
    static IDE_RC registerPlanSequence( qcStatement * aStatement,
                                        void        * aSequenceHandle );

    // plan이 proc 객체 참조함을 기록한다.
    static IDE_RC registerPlanProc( qcStatement     * aStatement,
                                    qsxProcPlanList * aProcPlanList );

    // plan이 table과 sequence 객체의 synonym 객체를 참조함을 기록한다.
    static IDE_RC registerPlanSynonym( qcStatement           * aStatement,
                                       struct qcmSynonymInfo * aSynonymInfo,
                                       qcNamePosition          aUserName,
                                       qcNamePosition          aObjectName,
                                       void                  * aTableHandle,
                                       void                  * aSequenceHandle );

    // plan이 proc 객체의 synonymn 객체를 참조함을 기록한다.
    static IDE_RC registerPlanProcSynonym( qcStatement          * aStatement,
                                           struct qsSynonymList * aObjectSynonymList );

    // plan이 table에 대한 필요한 권한을 기록한다.
    static IDE_RC registerPlanPrivTable( qcStatement         * aStatement,
                                         UInt                  aPrivilegeID,
                                         struct qcmTableInfo * aTableInfo );

    // plan이 sequence 객체에 대한 필요한 권한을 기록한다.
    static IDE_RC registerPlanPrivSequence( qcStatement            * aStatement,
                                            struct qcmSequenceInfo * aSequenceInfo );

    // Plan 이 참조하는 바인드 정보를 기록한다.
    static void registerPlanBindInfo( qcStatement  * aStatement );

    // PROJ-1073 Package
    // plan이 pkg 객체 참조함을 기록한다.
    static IDE_RC registerPlanPkg( qcStatement     * aStatement,
                                   qsxProcPlanList * aPlanList );

    //-------------------------------------------------
    // plan validation 함수
    //-------------------------------------------------

    // soft-prepare 과정에서 올바른 plan을 검색하기 위해 사용한다.
    static IDE_RC isMatchedPlanProperty( qcStatement    * aStatement,
                                         qcPlanProperty * aPlanProperty,
                                         idBool         * aIsMatched );

    // PROJ-2163 : soft-prepare 과정에서 올바른 plan을 검색하기 위해 사용한다.
    static IDE_RC isMatchedPlanBindInfo( qcStatement    * aStatement,
                                         qcPlanBindInfo * aBindInfo,
                                         idBool         * aIsMatched );

    // rebuild시 반영되는 property environment를 갱신한다.
    static IDE_RC rebuildPlanProperty( qcStatement    * aStatement,
                                       qcPlanProperty * aPlanProperty );

    // plan 생성시의 table 객체에 대한 validation을 수행한다.
    static IDE_RC validatePlanTable( qcgEnvTableList * aTableList,
                                     UInt              aUserID,    /* BUG-45893 */
                                     idBool          * aIsValid );

    // plan 생성시의 sequence 객체에 대한 validation을 수행한다.
    static IDE_RC validatePlanSequence( qcgEnvSequenceList * aSequenceList,
                                        idBool             * aIsValid );

    // plan 생성시의 proc 객체에 대한 validation을 수행한다.
    // proc을 검사하면서 latch를 획득한다.
    static IDE_RC validatePlanProc( qcgEnvProcList * aProcList,
                                    idBool         * aIsValid );

    // plan 생성시의 synonym 객체에 대한 validation을 수행한다.
    static IDE_RC validatePlanSynonym( qcStatement       * aStatement,
                                       qcgEnvSynonymList * aSynonymList,
                                       idBool            * aIsValid );

    // PROJ-1073 Package
    // plan 생성시의 package 객체에 대한 validation을 수행한다.
    // package을 검사하면서 latch를 획득한다.
    static IDE_RC validatePlanPkg( qcgEnvPkgList  * aPkgList,
                                   idBool         * aIsValid );

    //-------------------------------------------------
    // privilege 검사 함수
    //-------------------------------------------------

    // plan 생성시의 table 객체에 대한 필요한 권한을 검사한다.
    static IDE_RC checkPlanPrivTable(
        qcStatement                         * aStatement,
        qcgEnvTablePrivList                 * aTableList,
        qciGetSmiStatement4PrepareCallback    aGetSmiStmt4PrepareCallback,
        void                                * aGetSmiStmt4PrepareContext );

    // plan 생성시의 sequence 객체에 대한 필요한 권한을 검사한다.
    static IDE_RC checkPlanPrivSequence(
        qcStatement                        * aStatement,
        qcgEnvSequencePrivList             * aSequenceList,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );

    // plan 생성시의 proc 객체에 대한 필요한 권한을 검사한다.
    static IDE_RC checkPlanPrivProc(
        qcStatement                         * aStatement,
        qcgEnvProcList                      * aProcList,
        qciGetSmiStatement4PrepareCallback    aGetSmiStmt4PrepareCallback,
        void                                * aGetSmiStmt4PrepareContext );

    // BUG-37251 check privilege for package
    // plan 생성시의 package 객체에 대한 필요한 권한을 검사한다.
    static IDE_RC checkPlanPrivPkg(
        qcStatement                        * aStatement,
        qcgEnvPkgList                      * aPkgList,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );
    
    // qci::checkPrivilege에서 필요한 경우에만 smiStmt를 생성한다.
    static IDE_RC setSmiStmtCallback(
        qcStatement                        * aStatement,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );
    
    //-------------------------------------------------
    // plan cache 내부적으로 사용하기 위한 함수
    //-------------------------------------------------

    // template을 재사용하기 위한 초기화를 수행한다.
    static void   initPrepTemplate4Reuse( qcTemplate * aTemplate );

    // 원본 template에서 free 했던 tuple row를 재생성한다.
    static IDE_RC allocUnusedTupleRow( qcSharedPlan * aSharedPlan );

    // 원본 template에서 free 가능한 tuple row를 해제한다.
    static IDE_RC freeUnusedTupleRow( qcSharedPlan * aSharedPlan );

private:

    //-------------------------------------------------
    // plan validation 함수
    //-------------------------------------------------
    
    // plan 생성시의 table 객체의 synonym 객체에 대한 validation을 수행한다.
    static IDE_RC validatePlanSynonymTable( qcStatement       * aStatement,
                                            qcgEnvSynonymList * aSynonym,
                                            idBool            * aIsValid );
    
    // plan 생성시의 sequence 객체의 synonym 객체에 대한 validation을 수행한다.
    static IDE_RC validatePlanSynonymSequence( qcStatement       * aStatement,
                                               qcgEnvSynonymList * aSynonym,
                                               idBool            * aIsValid );
    
    // plan 생성시의 proc 객체의 synonym 객체에 대한 validation을 수행한다.
    static IDE_RC validatePlanSynonymProc( qcStatement       * aStatement,
                                           qcgEnvSynonymList * aSynonym,
                                           idBool            * aIsValid );
};

#endif /* _O_QCG_PLAN_H_ */
