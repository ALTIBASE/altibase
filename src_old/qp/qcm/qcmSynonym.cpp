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
 * $Id: qcmSynonym.cpp 91010 2021-06-17 01:33:11Z hykim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <qcm.h>
#include <qcg.h>
#include <qsv.h>
#include <qcmUser.h>
#include <qcmProc.h>
#include <qcmPkg.h>
#include <qcmSynonym.h>
#include <qcgPlan.h>
#include <qcmLibrary.h> // PROJ-1685

const void * gQcmSynonyms;
const void * gQcmSynonymsIndex[ QCM_MAX_META_INDICES ];

IDE_RC qcmSynonym::getSynonym( qcStatement    *aStatement,
                               UInt            aUserID,
                               SChar          *aSynonymName,
                               SInt            aSynonymNameSize,
                               qcmSynonymInfo *aSynonymInfo,
                               idBool         *aExist)
{
    vSLong                sRowCount;
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    mtdIntegerType        sIntData;
    qcNameCharBuffer      sSynonymNameBuffer;
    mtdCharType         * sSynonymName = (mtdCharType *) &sSynonymNameBuffer;
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSceondMtcColumn;

    IDE_DASSERT(aStatement   != NULL);
    IDE_DASSERT(aSynonymName != NULL);
    IDE_DASSERT(aSynonymInfo != NULL);

    *aExist = ID_FALSE;

    // fix BUG-34347
    if( gQcmSynonyms == NULL )
    {
        /* Nothing to do */
    }
    else
    {
        if ( aSynonymNameSize > QC_MAX_OBJECT_NAME_LEN )
        {
            *aExist = ID_FALSE;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        qtc::setVarcharValue( sSynonymName,
                              NULL,
                              aSynonymName,
                              aSynonymNameSize);
        sIntData = (mtdIntegerType)aUserID;
 
        qtc::initializeMetaRange(&sRange,
                                 MTD_COMPARE_MTDVAL_MTDVAL); // meta memory table

        IDE_TEST( smiGetTableColumns( gQcmSynonyms,
                                      QCM_SYNONYMS_SYNONYM_OWNER_ID_COL_ORDER,
                                      (const smiColumn**)&sFirstMtcColumn )
                  != IDE_SUCCESS );

        if(QC_PUBLIC_USER_ID == aUserID)
        {
            // public synonym의 경우
            // user_id는 NULL이다.
            qtc::setMetaRangeIsNullColumn(
                &sFirstMetaRange,
                sFirstMtcColumn,
                0 ); //First columnIdx
        }
        else
        {
            qtc::setMetaRangeColumn(
                &sFirstMetaRange,
                (mtcColumn*)
                sFirstMtcColumn,
                &sIntData,
                SMI_COLUMN_ORDER_ASCENDING,
                0 ); //First columnIdx
        }
 
        qtc::addMetaRangeColumn(&sRange,
                                &sFirstMetaRange);

        IDE_TEST( smiGetTableColumns( gQcmSynonyms,
                                      QCM_SYNONYMS_SYNONYM_NAME_COL_ORDER,
                                      (const smiColumn**)&sSceondMtcColumn )
                  != IDE_SUCCESS );

        qtc::setMetaRangeColumn(
            &sSecondMetaRange,
            (mtcColumn*)
            sSceondMtcColumn,
            (const void*) sSynonymName,
            SMI_COLUMN_ORDER_ASCENDING,
            1 ); //Second columnIdx
 
        qtc::addMetaRangeColumn(&sRange,
                                &sSecondMetaRange);
 
        qtc::fixMetaRange(&sRange);
 
        IDE_TEST(
            qcm::selectRow(
                QC_SMI_STMT(aStatement),
                gQcmSynonyms,
                smiGetDefaultFilter(),
                & sRange,
                gQcmSynonymsIndex[QCM_SYNONYMS_USERID_SYNONYMNAME_IDX_ORDER],
                (qcmSetStructMemberFunc) qcmSetSynonym,
                (void*) aSynonymInfo,
                0,
                1,
                &sRowCount )
            != IDE_SUCCESS );
 
        if ( sRowCount == 0 )
        {
            *aExist = ID_FALSE;
        }
        else if (sRowCount == 1)
        {
            *aExist = ID_TRUE;
        }
        else // rowcount > 1
        {
            IDE_RAISE(ERR_META_CRASH);
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSetSynonym (
    idvSQL     * /* aStatistics */,
    const void * aRow,
    qcmSynonymInfo *aSynonymInfo)
{
    mtcColumn * sOwnerNameMtcColumn;
    mtcColumn * sNameMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmSynonyms,
                                  QCM_SYNONYMS_OBJECT_OWNER_NAME_COL_ORDER,
                                  (const smiColumn**)&sOwnerNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue(
        aRow,
        sOwnerNameMtcColumn,
        aSynonymInfo->objectOwnerName);

    IDE_TEST( smiGetTableColumns( gQcmSynonyms,
                                  QCM_SYNONYMS_OBJECT_NAME_COL_ORDER,
                                  (const smiColumn**)&sNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue(
        aRow,
        sNameMtcColumn,
        aSynonymInfo->objectName);

    aSynonymInfo->isSynonymName = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::getSynonymOwnerID( qcStatement * aStatement,
                                      UInt          aUserID,
                                      SChar       * aSynonymName,
                                      SInt          aSynonymNameSize,
                                      UInt        * aSynonymOwnerID,
                                      idBool      * aExist )
{
    vSLong               sRowCount = 0;
    smiRange             sRange;
    qtcMetaRangeColumn   sFirstMetaRange;
    qtcMetaRangeColumn   sSecondMetaRange;
    mtdIntegerType       sIntData;
    qcNameCharBuffer     sSynonymNameBuffer;
    mtdCharType        * sSynonymName = (mtdCharType *) &sSynonymNameBuffer;
    mtcColumn          * sFirstMtcColumn;
    mtcColumn          * sSceondMtcColumn;

    IDE_DASSERT(aStatement   != NULL);
    IDE_DASSERT(aSynonymName != NULL);

    *aExist = ID_FALSE;

    // fix BUG-34347
    if( gQcmSynonyms == NULL )
    {
        /* Nothing to do */
    }
    else
    { 
        qtc::setVarcharValue( sSynonymName,
                              NULL,
                              aSynonymName,
                              aSynonymNameSize);
        sIntData = (mtdIntegerType)aUserID;
 
        qtc::initializeMetaRange(&sRange,
                                 MTD_COMPARE_MTDVAL_MTDVAL); // meta memory table

        IDE_TEST( smiGetTableColumns( gQcmSynonyms,
                                      QCM_SYNONYMS_SYNONYM_OWNER_ID_COL_ORDER,
                                      (const smiColumn**)&sFirstMtcColumn )
                  != IDE_SUCCESS );
        if(QC_PUBLIC_USER_ID == aUserID)
        {
            // public synonym의 경우
            // user_id는 NULL이다.
            qtc::setMetaRangeIsNullColumn(
                &sFirstMetaRange,
                sFirstMtcColumn,
                0 ); //First columnIdx
        }
        else
        {
            qtc::setMetaRangeColumn(
                &sFirstMetaRange,
                sFirstMtcColumn,
                &sIntData,
                SMI_COLUMN_ORDER_ASCENDING,
                0 ); //First columnIdx
        }
 
        qtc::addMetaRangeColumn(&sRange,
                                &sFirstMetaRange);

        IDE_TEST( smiGetTableColumns( gQcmSynonyms,
                                      QCM_SYNONYMS_SYNONYM_NAME_COL_ORDER,
                                      (const smiColumn**)&sSceondMtcColumn )
                  != IDE_SUCCESS );

        qtc::setMetaRangeColumn(
            &sSecondMetaRange,
            sSceondMtcColumn,
            (const void*) sSynonymName,
            SMI_COLUMN_ORDER_ASCENDING,
            1 ); //Second columnIdx
 
        qtc::addMetaRangeColumn(&sRange,
                                &sSecondMetaRange);
 
        qtc::fixMetaRange(&sRange);
 
        IDE_TEST(
            qcm::selectRow(
                QC_SMI_STMT(aStatement),
                gQcmSynonyms,
                smiGetDefaultFilter(),
                & sRange,
                gQcmSynonymsIndex[QCM_SYNONYMS_USERID_SYNONYMNAME_IDX_ORDER],
                (qcmSetStructMemberFunc) qcmSetSynonymOwnerID,
                (void*) aSynonymOwnerID,
                0,
                1,
                &sRowCount )
            != IDE_SUCCESS );
 
        if ( sRowCount == 0 )
        {
            /* Nothing to do */
        }
        else if ( sRowCount == 1 )
        {
            *aExist = ID_TRUE;
        }
        else // rowcount > 1
        {
            IDE_RAISE(ERR_META_CRASH);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSetSynonymOwnerID(
    idvSQL     * /* aStatistics */,
    const void * aRow,
    UInt * aSynonymOwnerID)
{
    mtcColumn * sOwnerIDMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmSynonyms,
                                  QCM_SYNONYMS_SYNONYM_OWNER_ID_COL_ORDER,
                                  (const smiColumn**)&sOwnerIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        aRow,
        sOwnerIDMtcColumn,
        aSynonymOwnerID);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveTableViewQueue(
    qcStatement     * aStatement,
    qcNamePosition    aUserName,
    qcNamePosition    aObjectName,
    qcmTableInfo   ** aTableInfo,
    UInt            * aUserID,
    smSCN           * aSCN,
    idBool          * aExist,
    qcmSynonymInfo  * aSynonymInfo,
    void           ** aTableHandle)
{
/***********************************************************************
 *
 * Description : get table info with name
 *
 * Implementation :
 *     1. search qcmTableInfo
 *     2. search private synonym
 *     3. search public synonym
 *
 ***********************************************************************/
    idBool         sCheckPublic =  ID_FALSE;
    SChar          sSynonymOwnerName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt           sSynonymOwnerID;
    SChar          sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    iduList        sSynonymInfoList;
    UInt           sTmpUserID;
    qsOID          sTmpObjectID = QS_EMPTY_OID;
    idBool         sExistObject = ID_FALSE;

    *aExist = ID_FALSE;
    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;

    // BUG-20908
    // Synonym정보를 저장할 List 초기화
    // 오류 발생시 에러 메세지에 쓰도록 Synonym의 이름을 넣어준다.
    IDU_LIST_INIT_OBJ( &sSynonymInfoList, &aObjectName );

    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        // have to check private & public synonym.
        *aUserID = QCG_GET_SESSION_USER_ID(aStatement);
        
        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, aUserID )
                 != IDE_SUCCESS);
    }

    IDE_TEST_CONT( QC_IS_NULL_NAME( aObjectName ) == ID_TRUE,
                   NORMAL_EXIT );

    idlOS::memcpy( &sObjectName,
                   aObjectName.stmtText + aObjectName.offset,
                   aObjectName.size );
    sObjectName[aObjectName.size] = '\0';

    // PROJ-2083 DUAL Table
    if ( ( idlOS::strlen( sObjectName ) >= 2 ) &&
         ( ( idlOS::strMatch( sObjectName, 2, "X$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "D$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "S$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "V$", 2 ) == 0 ) ) )
    {
        if( qcmFixedTable::getTableInfo( aStatement,
                                         aObjectName,
                                         aTableInfo,
                                         aSCN,
                                         aTableHandle )
            == IDE_SUCCESS )
        {
            *aExist = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        if ( qcm::getTableInfo(aStatement,
                               *aUserID,
                               aObjectName,
                               aTableInfo,
                               aSCN,
                               aTableHandle) == IDE_SUCCESS )
        {
            //gettableinfo success.
            *aExist     = ID_TRUE;

            if ( sCheckPublic == ID_TRUE )
            {
                // SESSION_USER_ID를 이용했다면 environment를 기록한다.
                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_USER_ID );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    if( *aExist == ID_FALSE )
    {
        IDE_CLEAR();

        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);

        if( ID_TRUE == *aExist )
        {
            // BUG-25587
            IDE_TEST(getSynonymOwnerID(aStatement,
                                       *aUserID,
                                       aObjectName.stmtText + aObjectName.offset,
                                       aObjectName.size,
                                       &sSynonymOwnerID,
                                       aExist) != IDE_SUCCESS);

            IDE_DASSERT( ID_TRUE == *aExist );

            IDE_TEST( qcmUser::getUserName(aStatement,
                                           sSynonymOwnerID,
                                           sSynonymOwnerName)
                      != IDE_SUCCESS );

            IDE_TEST( addSynonymToRelatedObject(aStatement,
                                                sSynonymOwnerName,
                                                sObjectName,
                                                QS_SYNONYM)
                      != IDE_SUCCESS );

            // BUG-20908
            // Synonym의 Circularity Check를 위해 List에 추가한다.
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            &sSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(
                resolveTableViewQueueInternal(
                    aStatement,
                    aSynonymInfo,
                    aTableInfo,
                    aUserID,
                    aSCN,
                    aExist,
                    aTableHandle,
                    &sSynonymInfoList)
                != IDE_SUCCESS);
            
            if ( sCheckPublic == ID_TRUE )
            {
                // SESSION_USER_ID를 이용했다면 environment를 기록한다.
                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_USER_ID );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if (sCheckPublic == ID_FALSE)
            {
                *aExist = ID_FALSE;
            }
            else
            {
                /* BUG- 39066
                   동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            aUserName,
                                            aObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if ( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST(
                        getSynonym(
                            aStatement,
                            QC_PUBLIC_USER_ID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist)
                        != IDE_SUCCESS);

                    if( ID_TRUE == *aExist )
                    {
                        IDE_TEST( addSynonymToRelatedObject(aStatement,
                                                            NULL,
                                                            sObjectName,
                                                            QS_SYNONYM)
                                  != IDE_SUCCESS );

                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        // BUG-20908
                        // Synonym의 Circularity Check를 위해 List에 추가한다.
                        IDE_TEST( addToSynonymInfoList( aStatement,
                                                        &sSynonymInfoList,
                                                        aSynonymInfo )
                                  != IDE_SUCCESS );

                        // resolve public synonym
                        IDE_TEST(
                            resolveTableViewQueueInternal(
                                aStatement,
                                aSynonymInfo,
                                aTableInfo,
                                aUserID,
                                aSCN,
                                aExist,
                                aTableHandle,
                                &sSynonymInfoList) != IDE_SUCCESS);
                    }
                    else
                    {
                        // no public synonym
                        // do nothing
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveSequence(
    qcStatement     * aStatement,
    qcNamePosition    aUserName,
    qcNamePosition    aObjectName,
    qcmSequenceInfo * aSequenceInfo,
    UInt            * aUserID,
    idBool          * aExist,
    qcmSynonymInfo  * aSynonymInfo,
    void           ** aSequenceHandle)
{
/***********************************************************************
 *
 * Description :
 *     BUG-16980
 *     get sequence info with name
 *
 * Implementation :
 *     1. search sys_tables_
 *     2. search private synonym
 *     3. search public synonym
 *
 ***********************************************************************/
    idBool         sCheckPublic =  ID_FALSE;
    iduList        sSynonymInfoList;
    UInt           sTmpUserID;
    qsOID          sTmpObjectID = QS_EMPTY_OID;
    idBool         sExistObject = ID_FALSE;

    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;

    // BUG-20908
    // Synonym정보를 저장할 List 초기화
    // 오류 발생시 에러 메세지에 쓰도록 Synonym의 이름을 넣어준다.
    IDU_LIST_INIT_OBJ( &sSynonymInfoList, &aObjectName );

    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        // have to check private & public synonym.
        *aUserID = QCG_GET_SESSION_USER_ID(aStatement);
        
        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, aUserID )
                 != IDE_SUCCESS);
    }


    if ( qcm::getSequenceInfo(aStatement,
                              *aUserID,
                              (UChar*) aObjectName.stmtText + aObjectName.offset,
                              aObjectName.size,
                              aSequenceInfo,
                              aSequenceHandle) == IDE_SUCCESS )
    {
        *aExist = ID_TRUE;
        
        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist)
        {
            // BUG-20908
            // Synonym의 Circularity Check를 위해 List에 추가한다.
            addToSynonymInfoList( aStatement,
                                  &sSynonymInfoList,
                                  aSynonymInfo );

            // resolve private synonym
            IDE_TEST(
                resolveSequenceInternal(
                    aStatement,
                    aSynonymInfo,
                    aSequenceInfo,
                    aUserID,
                    aExist,
                    aSequenceHandle,
                    &sSynonymInfoList)
                != IDE_SUCCESS);
            
            if ( sCheckPublic == ID_TRUE )
            {
                // SESSION_USER_ID를 이용했다면 environment를 기록한다.
                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_USER_ID );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if (sCheckPublic == ID_FALSE)
            {
                *aExist = ID_FALSE;
            }
            else
            {
                /* BUG- 39066
                   동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            aUserName,
                                            aObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST(
                        getSynonym(
                            aStatement,
                            QC_PUBLIC_USER_ID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist)
                        != IDE_SUCCESS);
                    if( ID_TRUE == *aExist )
                    {
                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        // BUG-20908
                        // Synonym의 Circularity Check를 위해 List에 추가한다.
                        IDE_TEST( addToSynonymInfoList( aStatement,
                                                        &sSynonymInfoList,
                                                        aSynonymInfo )
                                 != IDE_SUCCESS );

                        // resolve public synonym
                        IDE_TEST(
                            resolveSequenceInternal(
                                aStatement,
                                aSynonymInfo,
                                aSequenceInfo,
                                aUserID,
                                aExist,
                                aSequenceHandle,
                                &sSynonymInfoList)
                            != IDE_SUCCESS);
                    }
                    else
                    {
                        // no public synonym
                        // do nothing
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolvePSM(
    qcStatement     * aStatement,
    qcNamePosition    aUserName,
    qcNamePosition    aObjectName,
    qsOID           * aProcID,
    UInt            * aUserID,
    idBool          * aExist,
    qcmSynonymInfo  * aSynonymInfo )
{
    idBool         sCheckPublic =  ID_FALSE;
    iduList        sSynonymInfoList;
    /* BUG-39048
       check pubilc synonym전에 동일한 이름을
       가진 객체가 있는지 확인 필요. */
    UInt           sTmpUserID;
    qsOID          sTmpObjectID = QS_EMPTY_OID;
    idBool         sExistObject = ID_FALSE;

    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;

    // BUG-20908
    // Synonym정보를 저장할 List 초기화
    // 오류 발생시 에러 메세지에 쓰도록 Synonym의 이름을 넣어준다.
    IDU_LIST_INIT_OBJ( &sSynonymInfoList, &aObjectName );

    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        // have to check private & public synonym.
        *aUserID = QCG_GET_SESSION_USER_ID(aStatement);
        
        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, aUserID )
                 != IDE_SUCCESS);
        sCheckPublic = ID_FALSE;
    }


    IDE_TEST(
        qcmProc::getProcExistWithEmptyByNamePtr(
            aStatement,
            *aUserID,
            (SChar*) (aObjectName.stmtText +
                      aObjectName.offset),
            aObjectName.size,
            aProcID)
        != IDE_SUCCESS);

    if( QS_EMPTY_OID != *aProcID )
    {
        *aExist = ID_TRUE;
        
        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist )
        {
            // BUG-20908
            // Synonym의 Circularity Check를 위해 List에 추가한다.
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            &sSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(resolvePSMInternal(aStatement,
                                        aSynonymInfo,
                                        aProcID,
                                        aUserID,
                                        aExist,
                                        &sSynonymInfoList)
                     != IDE_SUCCESS);
            
            if ( sCheckPublic == ID_TRUE )
            {
                // SESSION_USER_ID를 이용했다면 environment를 기록한다.
                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_USER_ID );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if (sCheckPublic == ID_FALSE)
            {
                *aExist = ID_FALSE;
                // PSM not found
            }
            else
            {
                /* BUG-39048
                   동일한 이름을 가진 객체가 있는 확인필요 */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            aUserName,
                                            aObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if ( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST(getSynonym(
                                 aStatement,
                                 QC_PUBLIC_USER_ID,
                                 aObjectName.stmtText + aObjectName.offset,
                                 aObjectName.size,
                                 aSynonymInfo,
                                 aExist)
                             != IDE_SUCCESS);

                    if( ID_TRUE == *aExist )
                    {
                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        // BUG-20908
                        // Synonym의 Circularity Check를 위해 List에 추가한다.
                        IDE_TEST( addToSynonymInfoList( aStatement,
                                                        &sSynonymInfoList,
                                                        aSynonymInfo )
                                  != IDE_SUCCESS );

                        // resolve public synonym
                        IDE_TEST(resolvePSMInternal(aStatement,
                                                    aSynonymInfo,
                                                    aProcID,
                                                    aUserID,
                                                    aExist,
                                                    &sSynonymInfoList)
                                 != IDE_SUCCESS);
                    }
                    else
                    {
                        // no public synonym
                        // do nothing
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveObject(
    qcStatement     * aStatement,
    qcNamePosition    aUserName,
    qcNamePosition    aObjectName,
    qcmSynonymInfo  * aSynonymInfo,
    idBool          * aExist,
    UInt            * aUserID,
    UInt            * aObjectType,
    void           ** aObjectHandle,
    UInt            * aObjectID )
{
/***********************************************************************
 *
 * Description :
 *     type을 알 수 없는 객체를 이름만으로 찾을 수 있다.
 *     BUGBUG - sequence에 대해서만 특별히 meta id를 반환한다.
 *
 * Implementation :
 *     1. search objects
 *     2. search private synonym
 *     3. search public synonym
 *
 ***********************************************************************/

    idBool              sCheckPublic =  ID_FALSE;
    SChar               sSynonymOwnerName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt                sSynonymOwnerID;
    SChar               sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    iduList             sSynonymInfoList;

    qsOID               sProcOID;
    qsOID               sPkgOID;

    smSCN               sSCN;
    qcmTableInfo      * sTableInfo;
    qcmSequenceInfo     sSequenceInfo;

    // PROJ-1685
    idBool              sExist = ID_FALSE;
    qcmLibraryInfo      sLibraryInfo;

    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;

    // BUG-20908
    // Synonym정보를 저장할 List 초기화
    // 오류 발생시 에러 메세지에 쓰도록 Synonym의 이름을 넣어준다.
    IDU_LIST_INIT_OBJ( &sSynonymInfoList, &aObjectName );

    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        // have to check private & public synonym.
        *aUserID = QCG_GET_SESSION_USER_ID(aStatement);
        
        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, aUserID )
                 != IDE_SUCCESS);
    }

    *aExist = ID_FALSE;

    idlOS::memcpy( &sObjectName,
                   aObjectName.stmtText + aObjectName.offset,
                   aObjectName.size );

    sObjectName[aObjectName.size] = '\0';

    // Table, View, Queue
    // PROJ-2083 DUAL Table
    if ( ( idlOS::strlen( sObjectName ) >= 2 ) &&
         ( ( idlOS::strMatch( sObjectName, 2, "X$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "D$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "S$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "V$", 2 ) == 0 ) ) )
    {
        if( qcmFixedTable::getTableInfo( aStatement,
                                         aObjectName,
                                         &sTableInfo,
                                         &sSCN,
                                         aObjectHandle )
            == IDE_SUCCESS)
        {
            *aObjectType    = QCM_OBJECT_TYPE_TABLE;
            *aExist         = ID_TRUE;

            IDE_CONT(NORMAL_EXIT);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        if ( qcm::getTableInfo(aStatement,
                               *aUserID,
                               aObjectName,
                               &sTableInfo,
                               &sSCN,
                               aObjectHandle) == IDE_SUCCESS )
        {
            *aObjectType    = QCM_OBJECT_TYPE_TABLE;
            *aExist         = ID_TRUE;

            if ( sCheckPublic == ID_TRUE )
            {
                // SESSION_USER_ID를 이용했다면 environment를 기록한다.
                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_USER_ID );
            }
            else
            {
                // Nothing to do.
            }

            IDE_CONT(NORMAL_EXIT);
        }
        else
        {
            // Nothing to do.
        }
    }

    // Sequence
    if ( qcm::getSequenceInfo(aStatement,
                              *aUserID,
                              (UChar*)aObjectName.stmtText + aObjectName.offset,
                              aObjectName.size,
                              &sSequenceInfo,
                              aObjectHandle) == IDE_SUCCESS )
    {
        *aObjectType    = QCM_OBJECT_TYPE_SEQUENCE;
        *aExist         = ID_TRUE;
        *aObjectID      = sSequenceInfo.sequenceID;

        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }

    // PSM
    IDE_TEST(
        qcmProc::getProcExistWithEmptyByName(
            aStatement,
            *aUserID,
            aObjectName,
            &sProcOID)
        != IDE_SUCCESS);

    if( QS_EMPTY_OID != sProcOID )
    {
        *aObjectType    = QCM_OBJECT_TYPE_PSM;
        *aObjectHandle  = ( void* )smiGetTable( sProcOID );
        *aExist         = ID_TRUE;

        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }
   
    // PROJ-1685
    IDE_TEST(
        qcmLibrary::getLibrary(
            aStatement,
            *aUserID,
            (SChar*)(aObjectName.stmtText +
                     aObjectName.offset),
            aObjectName.size,
            &sLibraryInfo,
            &sExist)
        != IDE_SUCCESS);

    if( sExist == ID_TRUE )
    {
        *aObjectType    = QCM_OBJECT_TYPE_LIBRARY;
        *aExist         = ID_TRUE;
        *aObjectID      = sLibraryInfo.libraryID;

        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }
 
    // PROJ-1073 Package
    IDE_TEST(
        qcmPkg::getPkgExistWithEmptyByName(
            aStatement,
            *aUserID,
            aObjectName,
            QS_PKG,
            &sPkgOID)
        != IDE_SUCCESS);

    if( QS_EMPTY_OID != sPkgOID )
    {
        *aObjectType    = QCM_OBJECT_TYPE_PACKAGE;
        *aObjectHandle  = ( void* )smiGetTable( sPkgOID );
        *aExist         = ID_TRUE;

        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }

    IDE_CLEAR();

    // check private synonym
    IDE_TEST(getSynonym(aStatement,
                        *aUserID,
                        aObjectName.stmtText + aObjectName.offset,
                        aObjectName.size,
                        aSynonymInfo,
                        aExist) != IDE_SUCCESS);
    if( ID_TRUE == *aExist )
    {
        // BUG-25587
        IDE_TEST(getSynonymOwnerID(aStatement,
                                   *aUserID,
                                   aObjectName.stmtText + aObjectName.offset,
                                   aObjectName.size,
                                   &sSynonymOwnerID,
                                   aExist) != IDE_SUCCESS);

        IDE_DASSERT( ID_TRUE == *aExist );

        IDE_TEST( qcmUser::getUserName(aStatement,
                                       sSynonymOwnerID,
                                       sSynonymOwnerName)
                  != IDE_SUCCESS );

        IDE_TEST( addSynonymToRelatedObject(aStatement,
                                            sSynonymOwnerName,
                                            sObjectName,
                                            QS_SYNONYM)
                  != IDE_SUCCESS );

        // BUG-20908
        // Synonym의 Circularity Check를 위해 List에 추가한다.
        IDE_TEST( addToSynonymInfoList( aStatement,
                                        &sSynonymInfoList,
                                        aSynonymInfo )
                  != IDE_SUCCESS );

        // resolve private synonym
        IDE_TEST(
            resolveObjectInternal(
                aStatement,
                aSynonymInfo,
                &sSynonymInfoList,
                aExist,
                aUserID,
                aObjectType,
                aObjectHandle,
                aObjectID )
            != IDE_SUCCESS);
        
        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        if (sCheckPublic == ID_FALSE)
        {
            *aExist = ID_FALSE;
        }
        else
        {
            // check public synonym
            IDE_TEST(
                getSynonym(
                    aStatement,
                    QC_PUBLIC_USER_ID,
                    aObjectName.stmtText + aObjectName.offset,
                    aObjectName.size,
                    aSynonymInfo,
                    aExist)
                != IDE_SUCCESS);
            
            if( ID_TRUE == *aExist )
            {
                aSynonymInfo->isPublicSynonym = ID_TRUE;
                
                // BUG-20908
                // Synonym의 Circularity Check를 위해 List에 추가한다.
                IDE_TEST( addToSynonymInfoList( aStatement,
                                                &sSynonymInfoList,
                                                aSynonymInfo )
                          != IDE_SUCCESS );

                // resolve public synonym
                IDE_TEST(
                    resolveObjectInternal(
                        aStatement,
                        aSynonymInfo,
                        &sSynonymInfoList,
                        aExist,
                        aUserID,
                        aObjectType,
                        aObjectHandle,
                        aObjectID )
                    != IDE_SUCCESS);
            }
            else
            {
                // no public synonym
                // do nothing
            }
        }
    }

    IDE_EXCEPTION_CONT(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// PROJ-1073 Package
IDE_RC qcmSynonym::resolvePkg( qcStatement    * aStatement,
                               qcNamePosition   aUserName,
                               qcNamePosition   aTableName,
                               qsOID          * aPkgID,
                               UInt           * aUserID,
                               idBool         * aExist,
                               qcmSynonymInfo * aSynonymInfo )
{
    idBool           sCheckPublic = ID_FALSE;
    iduList          sSynonymInfoList;
    qcNamePosition   sUserName;
    qcNamePosition   sObjectName;
    /* BUG-39048
       check pubilc synonym전에 동일한 이름을
       가진 객체가 있는지 확인 필요. */
    UInt             sTmpUserID;
    qsOID            sTmpObjectID  = QS_EMPTY_OID;
    idBool           sExistObject  = ID_FALSE;
    qsPkgParseTree * sPkgParseTree = NULL;
    qsOID            sPkgSpecOID   = QS_EMPTY_OID;

    aSynonymInfo->isSynonymName   = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;
    SET_EMPTY_POSITION( sUserName );
    SET_EMPTY_POSITION( sObjectName );
    sPkgParseTree = aStatement->spvEnv->createPkg;
    if ( aStatement->spvEnv->pkgPlanTree != NULL )
    {
        sPkgSpecOID = aStatement->spvEnv->pkgPlanTree->pkgOID;
    }
    else
    {
        // Nothing to do.
    }

    if ( QC_IS_NULL_NAME( aUserName ) == ID_TRUE )
    {
        if ( QC_IS_NULL_NAME( aTableName) != ID_TRUE )
        {
            sObjectName = aTableName;

            // have to check private & public synonym.
            *aUserID = QCG_GET_SESSION_USER_ID(aStatement);

            sCheckPublic = ID_TRUE;
        }
        else
        {
            *aExist = ID_FALSE;
            IDE_TEST_CONT( *aExist == ID_FALSE, skip_resolve_pkg );
        }
    }
    else
    {
        if ( qcmUser::getUserID( aStatement,
                                 aUserName,
                                 aUserID )
             != IDE_SUCCESS )
        {
            IDE_CLEAR();

            /* aUserName은 user name이 아니다. */
            sObjectName = aUserName;

            // have to check private & public synonym.
            *aUserID = QCG_GET_SESSION_USER_ID(aStatement);

            sCheckPublic = ID_TRUE;
        }
        else
        {
            sUserName    = aUserName;
            sObjectName  = aTableName;
            sCheckPublic = ID_FALSE;
        }
    }

    IDU_LIST_INIT_OBJ( &sSynonymInfoList, &sObjectName );

    IDE_TEST( qcmPkg::getPkgExistWithEmptyByNamePtr(
                          aStatement,
                          *aUserID,
                          ( SChar * )( sObjectName.stmtText +
                                       sObjectName.offset),
                          sObjectName.size,
                          QS_PKG,
                          aPkgID )
              != IDE_SUCCESS );

    if ( *aPkgID != QS_EMPTY_OID )
    {
        *aExist = ID_TRUE;

        if( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // check private synonym
        IDE_TEST( getSynonym( aStatement,
                              *aUserID,
                              sObjectName.stmtText + sObjectName.offset,
                              sObjectName.size,
                              aSynonymInfo,
                              aExist )
                  != IDE_SUCCESS );

        if( ID_TRUE == *aExist )
        {
            // BUG-20908
            // Synonym의 Circularity Check를 위해 List에 추가한다.
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            &sSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST( resolvePkgInternal( aStatement,
                                          aSynonymInfo,
                                          aPkgID,
                                          aUserID,
                                          aExist,
                                          &sSynonymInfoList )
                      != IDE_SUCCESS );

            if ( sCheckPublic == ID_TRUE )
            {
                // SESSION_USER_ID를 이용했다면 environment를 기록한다.
                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_USER_ID );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* private synonym이 존재하지 않는다. */
            if( sCheckPublic == ID_FALSE )
            {
                *aExist = ID_FALSE;
            }
            else
            {
                /* BUG-39048
                   동일한 이름을 가진 객체가 있는 확인필요. */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            sUserName,
                                            sObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if ( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST( getSynonym( aStatement,
                                          QC_PUBLIC_USER_ID,
                                          sObjectName.stmtText + sObjectName.offset,
                                          sObjectName.size,
                                          aSynonymInfo,
                                          aExist )
                              != IDE_SUCCESS );

                    if( ID_TRUE == *aExist )
                    {
                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        // BUG-20908
                        // Synonym의 Circularity Check를 위해 List에 추가한다.
                        IDE_TEST( addToSynonymInfoList( aStatement,
                                                        &sSynonymInfoList,
                                                        aSynonymInfo )
                                  != IDE_SUCCESS );

                        // resolve public synonym
                        IDE_TEST( resolvePkgInternal( aStatement,
                                                      aSynonymInfo,
                                                      aPkgID,
                                                      aUserID,
                                                      aExist,
                                                      &sSynonymInfoList )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // No public synonym.
                        // Nothing to do.
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }

    /* BUG-41847 package subprogram을 parameter의 default로 사용 시 resource busy 발생
       이는 meta를 조회하여 객체를 찾는데,
       replace일 때는 메타에 객체 정보가 존재하고,
       이로 인해 자기 자신 또는 동일한 package를 외부 객체로 판단하여
       latch를 잡기 때문에 발생하는 문제

       1. 생성 중인 객체가 package spec일 때
           a. 생성 중인 객체의 user id와 object id가 동일하면
              생성 중인 객체와 동일한 객체를 메타에서 찾은 것이다.
       2. 생성 중인 객체가 package body일 경우
           a. 생성 중인 객체의 spec과 user id 및 object id가 동일하면
              다른 package가 아닌 해당 객체의 spec을 찾은 것이다.
           b. 생성 중인 객체의 user id와 object id가 동일하면
              생성 중인 객체와 동일한 객체를 메타에서 찾은 것이다. */
    if ( sPkgParseTree != NULL )
    {
        if ( (sPkgParseTree->userID == *aUserID) &&
             ((sPkgParseTree->pkgOID == *aPkgID) ||
              (sPkgSpecOID == *aPkgID)) )
        {
            *aExist = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( skip_resolve_pkg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveTableViewQueueInternal(
    qcStatement    * aStatement,
    qcmSynonymInfo * aSynonymInfo,
    qcmTableInfo  ** aTableInfo,
    UInt           * aUserID,
    smSCN          * aSCN,
    idBool         * aExist,
    void          ** aTableHandle,
    iduList        * aSynonymInfoList)
{
    //BUG-20855
    //PUBLIC SYNONYM을 찾기 위한 무한 Recusive Call을 방지하기 위해 원본 Synonym 정보를 임시 저장한다.
    //임시 저장된 객체는 새로운 Public Synonym을 얻어왔을 때 비교를 위해 쓰여진다.
    qcmSynonymInfo   sSynonymInfo;
    SChar          * sObjectName;
    qcNamePosition   sUserNamePos;
    qcNamePosition   sObjectNamePos;
    UInt             sTmpUserID;
    qsOID            sTmpObjectID = QS_EMPTY_OID;
    idBool           sExistObject = ID_FALSE;

    sUserNamePos.stmtText = aSynonymInfo->objectOwnerName;
    sUserNamePos.offset   = 0;
    sUserNamePos.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectNamePos.stmtText = aSynonymInfo->objectName;
    sObjectNamePos.offset   = 0;
    sObjectNamePos.size     = idlOS::strlen(aSynonymInfo->objectName);

    *aExist = ID_FALSE;

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    sObjectName = aSynonymInfo->objectName;

    // PROJ-2083 DUAL Table
    if ( ( idlOS::strlen( sObjectName ) >= 2 ) &&
         ( ( idlOS::strMatch( sObjectName, 2, "X$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "D$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "S$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "V$", 2 ) == 0 ) ) )
    {
        if( qcmFixedTable::getTableInfo( aStatement,
                                         (UChar*)aSynonymInfo->objectName,
                                         idlOS::strlen(aSynonymInfo->objectName),
                                         aTableInfo,
                                         aSCN,
                                         aTableHandle )
            == IDE_SUCCESS )
        {
            *aExist     = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        if ( qcm::getTableInfo(aStatement,
                               *aUserID,
                               (UChar*)aSynonymInfo->objectName,
                               idlOS::strlen(aSynonymInfo->objectName),
                               aTableInfo,
                               aSCN,
                               aTableHandle) == IDE_SUCCESS )
        {
            //gettableinfo success.
            *aExist = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    if( *aExist == ID_FALSE )
    {
        IDE_CLEAR();

        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist )
        {
            // BUG-25587
            IDE_TEST( addSynonymToRelatedObject(aStatement,
                                                sSynonymInfo.objectOwnerName,
                                                sSynonymInfo.objectName,
                                                QS_SYNONYM)
                      != IDE_SUCCESS );

            // BUG-20908
            // test synonym is circular
            IDE_TEST( checkSynonymCircularity( aStatement,
                                               aSynonymInfoList,
                                               aSynonymInfo )
                      != IDE_SUCCESS );

            // add synonym to list for circular check
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            aSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(
                resolveTableViewQueueInternal(
                    aStatement,
                    aSynonymInfo,
                    aTableInfo,
                    aUserID,
                    aSCN,
                    aExist,
                    aTableHandle,
                    aSynonymInfoList)
                != IDE_SUCCESS);
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserNamePos,
                                        sObjectNamePos,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if( sExistObject == ID_FALSE )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);
                if( ID_TRUE == *aExist )
                {
                    // BUG-25587
                    IDE_TEST( addSynonymToRelatedObject(aStatement,
                                                        NULL,
                                                        sSynonymInfo.objectName,
                                                        QS_SYNONYM)
                              != IDE_SUCCESS );

                    // BUG-20908
                    // test synonym is circular
                    IDE_TEST( checkSynonymCircularity( aStatement,
                                                       aSynonymInfoList,
                                                       aSynonymInfo )
                              != IDE_SUCCESS );

                    // add synonym to list for circular check
                    IDE_TEST( addToSynonymInfoList( aStatement,
                                                    aSynonymInfoList,
                                                    aSynonymInfo )
                              != IDE_SUCCESS );

                    //BUG-20855
                    if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                            idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                            sSynonymInfo.objectOwnerName,
                                            idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                         ( idlOS::strMatch( aSynonymInfo->objectName,
                                            idlOS::strlen( aSynonymInfo->objectName ),
                                            sSynonymInfo.objectName,
                                            idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                    {
                        // no public synonym matched
                        *aExist = ID_FALSE;
                    }
                    else
                    {    
                        IDE_TEST(
                            resolveTableViewQueueInternal(
                                aStatement,
                                aSynonymInfo,
                                aTableInfo,
                                aUserID,
                                aSCN,
                                aExist,
                                aTableHandle,
                                aSynonymInfoList) != IDE_SUCCESS );
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveSequenceInternal(
    qcStatement     * aStatement,
    qcmSynonymInfo  * aSynonymInfo,
    qcmSequenceInfo * aSequenceInfo,
    UInt            * aUserID,
    idBool          * aExist,
    void           ** aSequenceHandle,
    iduList         * aSynonymInfoList)
{
    //BUG-20855
    //PUBLIC SYNONYM을 찾기 위한 무한 Recusive Call을 방지하기 위해 원본 Synonym 정보를 임시 저장한다.
    //임시 저장된 객체는 새로운 Public Synonym을 얻어왔을 때 비교를 위해 쓰여진다.
    qcmSynonymInfo sSynonymInfo;
    qcNamePosition sUserName;
    qcNamePosition sObjectName;
    UInt           sTmpUserID;
    qsOID          sTmpObjectID = QS_EMPTY_OID;
    idBool         sExistObject = ID_FALSE;

    sUserName.stmtText = aSynonymInfo->objectOwnerName;
    sUserName.offset   = 0;
    sUserName.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectName.stmtText = aSynonymInfo->objectName;
    sObjectName.offset   = 0;
    sObjectName.size     = idlOS::strlen(aSynonymInfo->objectName);

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    if ( qcm::getSequenceInfo(aStatement,
                              *aUserID,
                              (UChar*)aSynonymInfo->objectName,
                              idlOS::strlen(aSynonymInfo->objectName),
                              aSequenceInfo,
                              aSequenceHandle) == IDE_SUCCESS )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist )
        {
            // BUG-20908
            // test synonym is circular
            IDE_TEST( checkSynonymCircularity( aStatement,
                                               aSynonymInfoList,
                                               aSynonymInfo )
                      != IDE_SUCCESS );

            // add synonym to list for circular check
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            aSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(
                resolveSequenceInternal(
                    aStatement,
                    aSynonymInfo,
                    aSequenceInfo,
                    aUserID,
                    aExist,
                    aSequenceHandle,
                    aSynonymInfoList)
                != IDE_SUCCESS);
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserName,
                                        sObjectName,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if( sExistObject == ID_FALSE  )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);
                if( ID_TRUE == *aExist )
                {
                    // BUG-20908
                    // test synonym is circular
                    IDE_TEST( checkSynonymCircularity( aStatement,
                                                       aSynonymInfoList,
                                                       aSynonymInfo )
                              != IDE_SUCCESS );

                    // add synonym to list for circular check
                    IDE_TEST( addToSynonymInfoList( aStatement,
                                                    aSynonymInfoList,
                                                    aSynonymInfo )
                              != IDE_SUCCESS );

                    //BUG-20855
                    if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                            idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                            sSynonymInfo.objectOwnerName,
                                            idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                         ( idlOS::strMatch( aSynonymInfo->objectName,
                                            idlOS::strlen( aSynonymInfo->objectName ),
                                            sSynonymInfo.objectName,
                                            idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                    {
                        // no public synonym matched
                        *aExist = ID_FALSE;
                    }
                    else
                    {
                        IDE_TEST(
                            resolveSequenceInternal(
                                aStatement,
                                aSynonymInfo,
                                aSequenceInfo,
                                aUserID,
                                aExist,
                                aSequenceHandle,
                                aSynonymInfoList)
                            != IDE_SUCCESS );                        
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;

}

IDE_RC qcmSynonym::resolvePSMInternal(
    qcStatement    * aStatement,
    qcmSynonymInfo * aSynonymInfo,
    qsOID          * aProcID,
    UInt           * aUserID,
    idBool         * aExist,
    iduList        * aSynonymInfoList)
{

    //BUG-20855
    //PUBLIC SYNONYM을 찾기 위한 무한 Recusive Call을 방지하기 위해 원본 Synonym 정보를 임시 저장한다.
    //임시 저장된 객체는 새로운 Public Synonym을 얻어왔을 때 비교를 위해 쓰여진다.
    qcmSynonymInfo sSynonymInfo;
    qcNamePosition sUserName;
    qcNamePosition sObjectName;
    UInt           sTmpUserID;
    qsOID          sTmpObjectID = QS_EMPTY_OID;
    idBool         sExistObject = ID_FALSE;

    sUserName.stmtText = aSynonymInfo->objectOwnerName;
    sUserName.offset   = 0;
    sUserName.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectName.stmtText = aSynonymInfo->objectName;
    sObjectName.offset   = 0;
    sObjectName.size     = idlOS::strlen(aSynonymInfo->objectName);

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    IDE_TEST(
        qcmProc::getProcExistWithEmptyByNamePtr(
            aStatement,
            *aUserID,
            aSynonymInfo->objectName,
            idlOS::strlen(aSynonymInfo->objectName),
            aProcID)
        != IDE_SUCCESS);

    if( QS_EMPTY_OID != *aProcID  )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist)
        {
            // BUG-20908
            // test synonym is circular
            IDE_TEST( checkSynonymCircularity( aStatement,
                                               aSynonymInfoList,
                                               aSynonymInfo )
                      != IDE_SUCCESS );

            // add synonym to list for circular check
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            aSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(
                resolvePSMInternal(
                    aStatement,
                    aSynonymInfo,
                    aProcID,
                    aUserID,
                    aExist,
                    aSynonymInfoList)
                != IDE_SUCCESS);
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserName,
                                        sObjectName,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if( sExistObject == ID_FALSE  )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);
                if( ID_TRUE == *aExist)
                {
                    // BUG-20908
                    // test synonym is circular
                    IDE_TEST( checkSynonymCircularity( aStatement,
                                                       aSynonymInfoList,
                                                       aSynonymInfo )
                              != IDE_SUCCESS );

                    // add synonym to list for circular check
                    IDE_TEST( addToSynonymInfoList( aStatement,
                                                    aSynonymInfoList,
                                                    aSynonymInfo )
                              != IDE_SUCCESS );

                    //BUG-20855
                    if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                            idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                            sSynonymInfo.objectOwnerName,
                                            idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                         ( idlOS::strMatch( aSynonymInfo->objectName,
                                            idlOS::strlen( aSynonymInfo->objectName ),
                                            sSynonymInfo.objectName,
                                            idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                    {
                        // no public synonym matched
                        *aExist = ID_FALSE;
                    }
                    else
                    {
                        IDE_TEST(
                            resolvePSMInternal(
                                aStatement,
                                aSynonymInfo,
                                aProcID,
                                aUserID,
                                aExist,
                                aSynonymInfoList)
                            != IDE_SUCCESS );                        
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

// PROJ-1073 Package
IDE_RC qcmSynonym::resolvePkgInternal(
    qcStatement    * aStatement,
    qcmSynonymInfo * aSynonymInfo,
    qsOID          * aPkgID,
    UInt           * aUserID,
    idBool         * aExist,
    iduList        * aSynonymInfoList)
{
    //BUG-20855
    //PUBLIC SYNONYM을 찾기 위한 무한 Recusive Call을 방지하기 위해 원본 Synonym 정보를 임시 저장한다.
    //임시 저장된 객체는 새로운 Public Synonym을 얻어왔을 때 비교를 위해 쓰여진다.
    qcmSynonymInfo sSynonymInfo;
    qcNamePosition sUserName;
    qcNamePosition sObjectName;
    UInt           sTmpUserID;
    qsOID          sTmpObjectID = QS_EMPTY_OID;
    idBool         sExistObject = ID_FALSE;

    sUserName.stmtText = aSynonymInfo->objectOwnerName;
    sUserName.offset   = 0;
    sUserName.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectName.stmtText = aSynonymInfo->objectName;
    sObjectName.offset   = 0;
    sObjectName.size     = idlOS::strlen(aSynonymInfo->objectName);

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    IDE_TEST( qcmPkg::getPkgExistWithEmptyByNamePtr( aStatement,
                                                     *aUserID,
                                                     aSynonymInfo->objectName,
                                                     idlOS::strlen(aSynonymInfo->objectName),
                                                     QS_PKG,
                                                     aPkgID )
              != IDE_SUCCESS );
    if( QS_EMPTY_OID != *aPkgID  )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist)
        {
            // BUG-20908
            // test synonym is circular
            IDE_TEST( checkSynonymCircularity( aStatement,
                                               aSynonymInfoList,
                                               aSynonymInfo )
                      != IDE_SUCCESS );

            // add synonym to list for circular check
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            aSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(
                resolvePkgInternal(
                    aStatement,
                    aSynonymInfo,
                    aPkgID,
                    aUserID,
                    aExist,
                    aSynonymInfoList)
                != IDE_SUCCESS);
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserName,
                                        sObjectName,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if( sExistObject == ID_FALSE  )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);
                if( ID_TRUE == *aExist)
                {
                    // BUG-20908
                    // test synonym is circular
                    IDE_TEST( checkSynonymCircularity( aStatement,
                                                       aSynonymInfoList,
                                                       aSynonymInfo )
                              != IDE_SUCCESS );

                    // add synonym to list for circular check
                    IDE_TEST( addToSynonymInfoList( aStatement,
                                                    aSynonymInfoList,
                                                    aSynonymInfo )
                              != IDE_SUCCESS );

                    //BUG-20855
                    if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                            idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                            sSynonymInfo.objectOwnerName,
                                            idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                         ( idlOS::strMatch( aSynonymInfo->objectName,
                                            idlOS::strlen( aSynonymInfo->objectName ),
                                            sSynonymInfo.objectName,
                                            idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                    {
                        // no public synonym matched
                        *aExist = ID_FALSE;
                    }
                    else
                    {
                        IDE_TEST(
                            resolvePkgInternal(
                                aStatement,
                                aSynonymInfo,
                                aPkgID,
                                aUserID,
                                aExist,
                                aSynonymInfoList)
                            != IDE_SUCCESS );
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::cascadeRemove(
    qcStatement *aStatement,
    UInt         aUserID)
{
    SChar               * sSqlStr;
    vSLong sRowCnt;

    IDU_LIMITPOINT("qcmSynonym::cascadeRemove::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SYNONYMS_ WHERE "
                     "SYNONYM_OWNER_ID = INTEGER'%"ID_INT32_FMT"'", aUserID);
    IDE_TEST(qcg::runDMLforDDL(QC_SMI_STMT( aStatement ),
                               sSqlStr,
                               & sRowCnt ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveObjectInternal(
    qcStatement     * aStatement,
    qcmSynonymInfo  * aSynonymInfo,
    iduList         * aSynonymInfoList,
    idBool          * aExist,
    UInt            * aUserID,
    UInt            * aObjectType,
    void           ** aObjectHandle,
    UInt            * aObjectID )
{
    qcmSynonymInfo      sSynonymInfo;
    SChar             * sObjectName;
    SInt                sObjectNameLength;
    qcNamePosition      sObjectNamePos;      // PROJ-1073 Package

    qsOID               sProcOID;
    qsOID               sPkgOID;             // PROJ-1073 Package

    smSCN               sSCN;
    qcmTableInfo      * sTableInfo;
    qcmSequenceInfo     sSequenceInfo;

    // PROJ-1685
    idBool              sExist = ID_FALSE;
    qcmLibraryInfo      sLibraryInfo;

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    sObjectName = aSynonymInfo->objectName;
    sObjectNameLength = idlOS::strlen(aSynonymInfo->objectName);

    // PROJ-1073 Package
    sObjectNamePos.stmtText = sObjectName;
    sObjectNamePos.offset = 0;
    sObjectNamePos.size = sObjectNameLength;

    *aExist = ID_FALSE;

    // Table, View, Queue
    // PROJ-2083 DUAL Table
    if ( ( idlOS::strlen( sObjectName ) >= 2 ) &&
         ( ( idlOS::strMatch( sObjectName, 2, "X$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "D$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "S$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "V$", 2 ) == 0 ) ) )
    {
        if( qcmFixedTable::getTableInfo( aStatement,
                                         (UChar*) sObjectName,
                                         sObjectNameLength,
                                         &sTableInfo,
                                         &sSCN,
                                         aObjectHandle)
            == IDE_SUCCESS)
        {
            *aObjectType    = QCM_OBJECT_TYPE_TABLE;
            *aExist         = ID_TRUE;

            IDE_CONT(NORMAL_EXIT);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {        
        if ( qcm::getTableInfo(aStatement,
                               *aUserID,
                               (UChar*)sObjectName,
                               sObjectNameLength,
                               &sTableInfo,
                               &sSCN,
                               aObjectHandle) == IDE_SUCCESS )
        {
            *aObjectType    = QCM_OBJECT_TYPE_TABLE;
            *aExist         = ID_TRUE;

            IDE_CONT(NORMAL_EXIT);
        }
        else
        {
            // Nothing to do.
        }
    }    

    // Sequence
    if ( qcm::getSequenceInfo(aStatement,
                              *aUserID,
                              (UChar*)sObjectName,
                              sObjectNameLength,
                              &sSequenceInfo,
                              aObjectHandle) == IDE_SUCCESS )
    {
        *aObjectType    = QCM_OBJECT_TYPE_SEQUENCE;
        *aExist         = ID_TRUE;
        *aObjectID      = sSequenceInfo.sequenceID;

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }

    // PSM
    IDE_TEST(
        qcmProc::getProcExistWithEmptyByNamePtr(
            aStatement,
            *aUserID,
            sObjectName,
            sObjectNameLength,
            &sProcOID)
        != IDE_SUCCESS);

    if( QS_EMPTY_OID != sProcOID )
    {
        *aObjectType    = QCM_OBJECT_TYPE_PSM;
        *aObjectHandle  = ( void* )smiGetTable( sProcOID );
        *aExist         = ID_TRUE;
        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1685
    IDE_TEST(
        qcmLibrary::getLibrary(
            aStatement,
            *aUserID,
            sObjectName,
            sObjectNameLength,
            &sLibraryInfo,
            &sExist)
        != IDE_SUCCESS);

    if( sExist == ID_TRUE )
    {
        *aObjectType    = QCM_OBJECT_TYPE_LIBRARY;
        *aExist         = ID_TRUE;
        *aObjectID      = sLibraryInfo.libraryID;

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1073 Package
    IDE_TEST(
        qcmPkg::getPkgExistWithEmptyByName(
            aStatement,
            *aUserID,
            sObjectNamePos,
            QS_PKG,
            &sPkgOID)
        != IDE_SUCCESS);

    if( QS_EMPTY_OID != sPkgOID )
    {
        *aObjectType    = QCM_OBJECT_TYPE_PACKAGE;
        *aObjectHandle  = ( void* )smiGetTable( sPkgOID );
        *aExist         = ID_TRUE;

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        // Nothing to do.
    }

    IDE_CLEAR();

    // check private synonym
    IDE_TEST( getSynonym( aStatement,
                          *aUserID,
                          sObjectName,
                          sObjectNameLength,
                          aSynonymInfo,
                          aExist ) != IDE_SUCCESS );
    if( ID_TRUE == *aExist )
    {
        // BUG-25587
        IDE_TEST( addSynonymToRelatedObject(aStatement,
                                            sSynonymInfo.objectOwnerName,
                                            sSynonymInfo.objectName,
                                            QS_SYNONYM)
                  != IDE_SUCCESS );

        IDE_TEST( checkSynonymCircularity( aStatement,
                                           aSynonymInfoList,
                                           aSynonymInfo )
                  != IDE_SUCCESS );

        IDE_TEST( addToSynonymInfoList( aStatement,
                                        aSynonymInfoList,
                                        aSynonymInfo )
                  != IDE_SUCCESS );

        // resolve private synonym
        IDE_TEST( resolveObjectInternal( aStatement,
                                         aSynonymInfo,
                                         aSynonymInfoList,
                                         aExist,
                                         aUserID,
                                         aObjectType,
                                         aObjectHandle,
                                         aObjectID )
                  != IDE_SUCCESS);
    }
    else
    {
        // check public synonym
        IDE_TEST( getSynonym( aStatement,
                              QC_PUBLIC_USER_ID,
                              sObjectName,
                              sObjectNameLength,
                              aSynonymInfo,
                              aExist )
                  != IDE_SUCCESS);

        if( ID_TRUE == *aExist )
        {
            // BUG-25587
            IDE_TEST( addSynonymToRelatedObject( aStatement,
                                                 NULL,
                                                 sSynonymInfo.objectName,
                                                 QS_SYNONYM )
                      != IDE_SUCCESS );

            IDE_TEST( checkSynonymCircularity( aStatement,
                                               aSynonymInfoList,
                                               aSynonymInfo )
                      != IDE_SUCCESS );

            // BUG-20908
            // Synonym의 Circularity Check를 위해 List에 추가한다.
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            aSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            //BUG-20855
            if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                    idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                    sSynonymInfo.objectOwnerName,
                                    idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                 ( idlOS::strMatch( aSynonymInfo->objectName,
                                    idlOS::strlen( aSynonymInfo->objectName ),
                                    sSynonymInfo.objectName,
                                    idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
            {
                // no public synonym matched
                *aExist = ID_FALSE;
            }
            else
            {    
                IDE_TEST( resolveObjectInternal( aStatement,
                                                 aSynonymInfo,
                                                 aSynonymInfoList,
                                                 aExist,
                                                 aUserID,
                                                 aObjectType,
                                                 aObjectHandle,
                                                 aObjectID )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_EXCEPTION_CONT(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    *aExist = ID_FALSE;
    return IDE_FAILURE;
}

IDE_RC qcmSynonym::addSynonymToRelatedObject(
    qcStatement       * aStatement,
    SChar             * aOwnerName,
    SChar             * aObjectName,
    SInt                aObjectType)
{
    qsRelatedObjects   * sCurrObject;
    qsRelatedObjects   * sObject;
    iduVarMemList      * sMemory = NULL;

    // 1. exec proc
    if( aStatement->spvEnv->createProc == NULL )
    {
        sMemory = aStatement->myPlan->qmpMem;
    }
    else
    {
        // 2. 프로시저 생성시에 최초 PVO 하는 경우
        if( aStatement->spvEnv->createProc->procInfo == NULL )
        {
            sMemory = aStatement->myPlan->qmpMem;
        }
        else
        {
            // 3. 프로시저 생성시에 두번째로 PVO 하는 경우
            // qsxProcInfo의 relatedObjects에 할당되는 메모리는 qmsMem 이 사용
            // 되어야 한다.
            // 각 statement의 qmpMem이 사용되는 경우 해당 statement가 재빌드
            // 되면 이전에 할당한 메모리 공간을 모두 해제하기 때문이다.
            sMemory = aStatement->spvEnv->createProc->procInfo->qmsMem;
        }
    }

    IDU_FIT_POINT( "qcmSynonym::addSynonymToRelatedObject::alloc::sCurrObject",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC( sMemory,
                            qsRelatedObjects,
                            &sCurrObject ) != IDE_SUCCESS);

    // object type
    sCurrObject->userID  = 0;
    sCurrObject->objectID = 0;
    sCurrObject->objectType = aObjectType;
    sCurrObject->tableID = 0;

    //-------------------------------------
    // SYNONYM USER NAME
    //-------------------------------------

    if(aOwnerName == NULL)
    {
        // public synonym
        sCurrObject->userName.size = 0;

        IDU_FIT_POINT( "qcmSynonym::addSynonymToRelatedObject::alloc::publicName",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->alloc( sCurrObject->userName.size + 1,
                                  (void**)&(sCurrObject->userName.name) )
                  != IDE_SUCCESS);

        sCurrObject->userName.name[sCurrObject->userName.size] = '\0';

        sCurrObject->userID = QC_PUBLIC_USER_ID;
    }
    else
    {
        // private synonym
        sCurrObject->userName.size = idlOS::strlen( aOwnerName );

        IDU_FIT_POINT( "qcmSynonym::addSynonymToRelatedObject::alloc::privateName",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->alloc( sCurrObject->userName.size + 1,
                                  (void**)&(sCurrObject->userName.name) )
                  != IDE_SUCCESS);

        idlOS::memcpy( sCurrObject->userName.name,
                       aOwnerName,
                       sCurrObject->userName.size );

        sCurrObject->userName.name[sCurrObject->userName.size] = '\0';

        IDE_TEST(qcmUser::getUserID( aStatement,
                                     sCurrObject->userName.name,
                                     (UInt)(sCurrObject->userName.size),
                                     &(sCurrObject->userID) )
                 != IDE_SUCCESS);
    }

    //-------------------------------------
    // SYNONYM OBJECT NAME
    //-------------------------------------
    sCurrObject->objectName.size
        = idlOS::strlen( aObjectName );

    IDU_FIT_POINT( "qcmSynonym::addSynonymToRelatedObject::alloc::objectName",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( sMemory->alloc( sCurrObject->objectName.size + 1,
                              (void**)&(sCurrObject->objectName.name) )
              != IDE_SUCCESS);

    idlOS::memcpy( sCurrObject->objectName.name,
                   aObjectName,
                   sCurrObject->objectName.size );

    sCurrObject->objectName.name[sCurrObject->objectName.size] = '\0';

    // objectNamePos
    SET_EMPTY_POSITION( sCurrObject->objectNamePos );

    //-------------------------------------
    // ADD RELATED OBJECTS
    //-------------------------------------
    if (aStatement->spvEnv->relatedObjects == NULL)
    {
        sCurrObject->next = NULL;
        aStatement->spvEnv->relatedObjects = sCurrObject;
    }

    // search same object
    for (sObject = aStatement->spvEnv->relatedObjects;
         sObject != NULL;
         sObject = sObject->next)
    {
        if (sObject->objectType == sCurrObject->objectType &&
            idlOS::strMatch(sObject->userName.name,
                            sObject->userName.size,
                            sCurrObject->userName.name,
                            sCurrObject->userName.size) == 0 &&
            idlOS::strMatch(sObject->objectName.name,
                            sObject->objectName.size,
                            sCurrObject->objectName.name,
                            sCurrObject->objectName.size) == 0)
        {
            // found
            break;
        }
    }

    if (sObject == NULL)
    {
        sCurrObject->next = aStatement->spvEnv->relatedObjects;

        aStatement->spvEnv->relatedObjects = sCurrObject;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::checkSynonymCircularity(
    qcStatement         * aStatement,
    iduList             * aList,
    qcmSynonymInfo      * aSynonymInfo)
{
/***********************************************************************
 *
 * Description : check synonym has circualr reference (BUG-20908)
 *
 * Implementation :
 *     1. 파라메터로 받은 Synonym이 List에 있는지 확인한다.
 *     2. Synonym의 연속 Resolve가 64번 이상 반복되는지 검사한다.
 *
 ***********************************************************************/
    iduList              * sIterator;
    UInt                   sCnt = 0;
    qcuSqlSourceInfo       sqlInfo;

    IDE_DASSERT( aList != NULL );
    IDE_DASSERT( aSynonymInfo != NULL );

    // Synonym Info List 안에 동일한 객체를 참조하는 Synonym이 있는지 검사한다.
    IDU_LIST_ITERATE( aList, sIterator )
    {
        // Synonym resolve 단계가 64를 넘어설 경우 에러 처리한다.
        if ( ++sCnt > QCM_MAX_RESOLVE_SYNONYM )
        {
            // List Header에 들어있는 Synonym이름을 출력한다.
            sqlInfo.setSourceInfo( aStatement, (qcNamePosition*)(aList->mObj) );
            IDE_RAISE( ERR_SYNONYM_RESOLVE_OVERFLOW );
        }
        else
        {
            // Nothing to do.
        }

        // 동일한 객체를 가리키는 링크가 있는지 검사
        if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                ((qcmSynonymInfo*)sIterator->mObj)->objectOwnerName,
                                idlOS::strlen( ((qcmSynonymInfo*)sIterator->mObj)->objectOwnerName ) ) == 0 ) &&
             ( idlOS::strMatch( aSynonymInfo->objectName,
                                idlOS::strlen( aSynonymInfo->objectName ),
                                ((qcmSynonymInfo*)sIterator->mObj)->objectName,
                                idlOS::strlen( ((qcmSynonymInfo*)sIterator->mObj)->objectName ) ) == 0 ) )
        {
            // List Header에 들어있는 Synonym이름을 출력한다.
            sqlInfo.setSourceInfo( aStatement, (qcNamePosition*)(aList->mObj) );
            IDE_RAISE( ERR_CIRCULAR_SYNONYM );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CIRCULAR_SYNONYM);
    {
        (void) sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_CIRCULAR_SYNONYM,
                            sqlInfo.getErrMessage() ));
        (void) sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_SYNONYM_RESOLVE_OVERFLOW);
    {
        (void) sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_SYNONYM_RESOLVE_OVERFLOW,
                            QCM_MAX_RESOLVE_SYNONYM_STR,
                            sqlInfo.getErrMessage() ));
        (void) sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::addToSynonymInfoList(
    qcStatement         * aStatement,
    iduList             * aList,
    qcmSynonymInfo      * aSynonymInfo )
{
/***********************************************************************
 *
 * Description : add synonym to list for circular check (BUG-20908)
 *
 * Implementation :
 *     1. List Node와 들어갈 Object인 Synonym Info에 메모리를 할당한다.
 *     2. List에 추가한다.
 *
 ***********************************************************************/
    iduListNode *sNode = NULL;

    // Node 공간 할당
    IDU_FIT_POINT( "qcmSynonym::addToSynonymInfoList::alloc::sNode",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(iduList),
                                            (void**)&(sNode))
              != IDE_SUCCESS);

    // Synonym 공간 할당
    IDU_FIT_POINT( "qcmSynonym::addToSynonymInfoList::alloc::mObj",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmSynonymInfo),
                                            (void**)&(sNode->mObj))
              != IDE_SUCCESS);

    // Synonym 정보 복사
    *((qcmSynonymInfo*)sNode->mObj) = *aSynonymInfo;

    // List의 마지막에 Node를 추가한다.
    IDU_LIST_ADD_FIRST( aList, sNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1685
IDE_RC qcmSynonym::resolveLibrary(
    qcStatement     * aStatement,
    qcNamePosition    aUserName,
    qcNamePosition    aObjectName,
    qcmLibraryInfo  * aLibraryInfo,
    UInt            * aUserID,
    idBool          * aExist,
    qcmSynonymInfo  * aSynonymInfo )
{
    idBool  sExist       = ID_FALSE;
    idBool  sCheckPublic =  ID_FALSE;
    iduList sSynonymInfoList;
    UInt    sTmpUserID;
    qsOID   sTmpObjectID = QS_EMPTY_OID;
    idBool  sExistObject = ID_FALSE;

    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;

    // BUG-20908
    // Synonym정보를 저장할 List 초기화
    // 오류 발생시 에러 메세지에 쓰도록 Synonym의 이름을 넣어준다.
    IDU_LIST_INIT_OBJ( &sSynonymInfoList, &aObjectName );

    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        // have to check private & public synonym.
        *aUserID = QCG_GET_SESSION_USER_ID(aStatement);

        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, aUserID )
                 != IDE_SUCCESS);
        sCheckPublic = ID_FALSE;
    }


    IDE_TEST(
        qcmLibrary::getLibrary(
            aStatement,
            *aUserID,
            (SChar*) (aObjectName.stmtText +
                      aObjectName.offset),
            aObjectName.size,
            aLibraryInfo,
            &sExist)
        != IDE_SUCCESS);

    if( sExist == ID_TRUE )
    {
        *aExist = ID_TRUE;

        if ( sCheckPublic == ID_TRUE )
        {
            // SESSION_USER_ID를 이용했다면 environment를 기록한다.
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_USER_ID );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist )
        {
            // BUG-20908
            // Synonym의 Circularity Check를 위해 List에 추가한다.
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            &sSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(resolveLibraryInternal(aStatement,
                                            aSynonymInfo,
                                            aLibraryInfo,
                                            aUserID,
                                            aExist,
                                            &sSynonymInfoList)
                     != IDE_SUCCESS);

            if ( sCheckPublic == ID_TRUE )
            {
                // SESSION_USER_ID를 이용했다면 environment를 기록한다.
                qcgPlan::registerPlanProperty( aStatement,
                                               PLAN_PROPERTY_USER_ID );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if (sCheckPublic == ID_FALSE)
            {
                *aExist = ID_FALSE;
                // library not found
            }
            else
            {
                /* BUG- 39066
                   동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            aUserName,
                                            aObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST(getSynonym(
                            aStatement,
                            QC_PUBLIC_USER_ID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist)
                        != IDE_SUCCESS);
                    if( ID_TRUE == *aExist )
                    {
                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        // BUG-20908
                        // Synonym의 Circularity Check를 위해 List에 추가한다.
                        IDE_TEST( addToSynonymInfoList( aStatement,
                                                        &sSynonymInfoList,
                                                        aSynonymInfo )
                                  != IDE_SUCCESS );

                        // resolve public synonym
                        IDE_TEST(resolveLibraryInternal(aStatement,
                                                        aSynonymInfo,
                                                        aLibraryInfo,
                                                        aUserID,
                                                        aExist,
                                                        &sSynonymInfoList)
                                 != IDE_SUCCESS);
                    }
                    else
                    {
                        // no public synonym
                        // do nothing
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveLibraryInternal(
    qcStatement    * aStatement,
    qcmSynonymInfo * aSynonymInfo,
    qcmLibraryInfo * aLibraryInfo,
    UInt           * aUserID,
    idBool         * aExist,
    iduList        * aSynonymInfoList)
{
    idBool sExist = ID_FALSE;

    //BUG-20855
    //PUBLIC SYNONYM을 찾기 위한 무한 Recusive Call을 방지하기 위해 원본 Synonym 정보를 임시 저장한다.
    //임시 저장된 객체는 새로운 Public Synonym을 얻어왔을 때 비교를 위해 쓰여진다.
    qcmSynonymInfo sSynonymInfo;
    qcNamePosition sUserName;
    qcNamePosition sObjectName;
    UInt           sTmpUserID;
    qsOID          sTmpObjectID = QS_EMPTY_OID;
    idBool         sExistObject = ID_FALSE;

    sUserName.stmtText = aSynonymInfo->objectOwnerName;
    sUserName.offset   = 0;
    sUserName.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectName.stmtText = aSynonymInfo->objectName;
    sObjectName.offset   = 0;
    sObjectName.size     = idlOS::strlen(aSynonymInfo->objectName);

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    IDE_TEST(
        qcmLibrary::getLibrary(
            aStatement,
            *aUserID,
            aSynonymInfo->objectName,
            idlOS::strlen(aSynonymInfo->objectName),
            aLibraryInfo,
            &sExist)
        != IDE_SUCCESS);

    if( sExist == ID_TRUE )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);
        if( ID_TRUE == *aExist)
        {
            // BUG-20908
            // test synonym is circular
            IDE_TEST( checkSynonymCircularity( aStatement,
                                               aSynonymInfoList,
                                               aSynonymInfo )
                      != IDE_SUCCESS );

            // add synonym to list for circular check
            IDE_TEST( addToSynonymInfoList( aStatement,
                                            aSynonymInfoList,
                                            aSynonymInfo )
                      != IDE_SUCCESS );

            // resolve private synonym
            IDE_TEST(
                resolveLibraryInternal(
                    aStatement,
                    aSynonymInfo,
                    aLibraryInfo,
                    aUserID,
                    aExist,
                    aSynonymInfoList)
                != IDE_SUCCESS);
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserName,
                                        sObjectName,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if( sExistObject == ID_FALSE  )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);
                if( ID_TRUE == *aExist)
                {
                    // BUG-20908
                    // test synonym is circular
                    IDE_TEST( checkSynonymCircularity( aStatement,
                                                       aSynonymInfoList,
                                                       aSynonymInfo )
                              != IDE_SUCCESS );

                    // add synonym to list for circular check
                    IDE_TEST( addToSynonymInfoList( aStatement,
                                                    aSynonymInfoList,
                                                    aSynonymInfo )
                              != IDE_SUCCESS );

                    //BUG-20855
                    if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                            idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                            sSynonymInfo.objectOwnerName,
                                            idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                         ( idlOS::strMatch( aSynonymInfo->objectName,
                                            idlOS::strlen( aSynonymInfo->objectName ),
                                            sSynonymInfo.objectName,
                                            idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                    {
                        // no public synonym matched
                        *aExist = ID_FALSE;
                    }
                    else
                    {
                        IDE_TEST(
                            resolveLibraryInternal(
                                aStatement,
                                aSynonymInfo,
                                aLibraryInfo,
                                aUserID,
                                aExist,
                                aSynonymInfoList)
                            != IDE_SUCCESS );                        
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

/* BUG-48594 */
IDE_RC qcmSynonym::resolveTableViewQueue4PlanCache( qcStatement     * aStatement,
                                                    qcNamePosition    aUserName,
                                                    qcNamePosition    aObjectName,
                                                    idBool          * aExist,
                                                    qcmSynonymInfo  * aSynonymInfo,
                                                    void           ** aTableHandle )
{
/***********************************************************************
 *
 * Description : get table info with name
 *
 * Implementation :
 *     1. search qcmTableInfo
 *     2. search private synonym
 *     3. search public synonym
 *
 ***********************************************************************/
    idBool             sCheckPublic =  ID_FALSE;
    SChar              sSynonymOwnerName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt               sSynonymOwnerID;
    SChar              sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt               sTmpUserID;
    qsOID              sTmpObjectID = QS_EMPTY_OID;
    idBool             sExistObject = ID_FALSE;
    UInt               sUserID = 0;
    qcmTableInfo     * sTableInfo = NULL;
    smSCN              sSCN;
    qcmSynonymArr      sSynonymArr;

    *aExist = ID_FALSE;

    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;
    sSynonymArr.mCount = 0;

    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        // have to check private & public synonym.
        sUserID = QCG_GET_SESSION_USER_ID(aStatement);
        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, &sUserID)
                 != IDE_SUCCESS);
    }

    IDE_TEST_CONT( QC_IS_NULL_NAME( aObjectName ) == ID_TRUE,
                   NORMAL_EXIT );

    idlOS::memcpy( &sObjectName,
                   aObjectName.stmtText + aObjectName.offset,
                   aObjectName.size );
    sObjectName[aObjectName.size] = '\0';

    // PROJ-2083 DUAL Table
    if ( ( idlOS::strlen( sObjectName ) >= 2 ) &&
         ( ( idlOS::strMatch( sObjectName, 2, "X$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "D$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "S$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "V$", 2 ) == 0 ) ) )
    {
        if ( qcmFixedTable::getTableInfo( aStatement,
                                          aObjectName,
                                          &sTableInfo,
                                          &sSCN,
                                          aTableHandle )
            == IDE_SUCCESS )
        {
            *aExist = ID_TRUE;
        }
    }
    else
    {
        if ( qcm::getTableInfo( aStatement,
                                sUserID,
                                aObjectName,
                                &sTableInfo,
                                &sSCN,
                                aTableHandle ) == IDE_SUCCESS )
        {
            //gettableinfo success.
            *aExist     = ID_TRUE;
        }
    }

    if ( *aExist == ID_FALSE )
    {
        IDE_CLEAR();

        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            sUserID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);

        if ( ID_TRUE == *aExist )
        {
            IDE_TEST(getSynonymOwnerID(aStatement,
                                       sUserID,
                                       aObjectName.stmtText + aObjectName.offset,
                                       aObjectName.size,
                                       &sSynonymOwnerID,
                                       aExist) != IDE_SUCCESS);

            IDE_TEST( qcmUser::getUserName(aStatement,
                                           sSynonymOwnerID,
                                           sSynonymOwnerName)
                      != IDE_SUCCESS );

            if ( addToSynonymArr( &sSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
            {
                // BUG-25587
                // resolve private synonym
                IDE_TEST( resolveTableViewQueueInternal4PlanCache( aStatement,
                                                                   aSynonymInfo,
                                                                   &sTableInfo,
                                                                   &sUserID,
                                                                   &sSCN,
                                                                   aExist,
                                                                   aTableHandle,
                                                                   &sSynonymArr )
                          != IDE_SUCCESS);
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
        else
        {
            if ( sCheckPublic == ID_FALSE)
            {
                *aExist = ID_FALSE;
            }
            else
            {
                /* BUG- 39066
                   동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            aUserName,
                                            aObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if ( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST( getSynonym( aStatement,
                                          QC_PUBLIC_USER_ID,
                                          aObjectName.stmtText + aObjectName.offset,
                                          aObjectName.size,
                                          aSynonymInfo,
                                          aExist )
                              != IDE_SUCCESS);

                    if ( ID_TRUE == *aExist )
                    {
                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        if ( addToSynonymArr( &sSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
                        {
                            // resolve public synonym
                            IDE_TEST( resolveTableViewQueueInternal4PlanCache( aStatement,
                                                                               aSynonymInfo,
                                                                               &sTableInfo,
                                                                               &sUserID,
                                                                               &sSCN,
                                                                               aExist,
                                                                               aTableHandle,
                                                                               &sSynonymArr )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            *aExist = ID_FALSE;
                        }
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

/* BUG-48594 */
IDE_RC qcmSynonym::resolveTableViewQueueInternal4PlanCache( qcStatement      * aStatement,
                                                            qcmSynonymInfo   * aSynonymInfo,
                                                            qcmTableInfo    ** aTableInfo,
                                                            UInt             * aUserID,
                                                            smSCN            * aSCN,
                                                            idBool           * aExist,
                                                            void            ** aTableHandle,
                                                            qcmSynonymArr    * aSynonymArr )
{
    qcmSynonymInfo   sSynonymInfo;
    SChar          * sObjectName;
    qcNamePosition   sUserNamePos;
    qcNamePosition   sObjectNamePos;
    UInt             sTmpUserID;
    qsOID            sTmpObjectID = QS_EMPTY_OID;
    idBool           sExistObject = ID_FALSE;

    sUserNamePos.stmtText = aSynonymInfo->objectOwnerName;
    sUserNamePos.offset   = 0;
    sUserNamePos.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectNamePos.stmtText = aSynonymInfo->objectName;
    sObjectNamePos.offset   = 0;
    sObjectNamePos.size     = idlOS::strlen(aSynonymInfo->objectName);

    *aExist = ID_FALSE;

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    sObjectName = aSynonymInfo->objectName;

    // PROJ-2083 DUAL Table
    if ( ( idlOS::strlen( sObjectName ) >= 2 ) &&
         ( ( idlOS::strMatch( sObjectName, 2, "X$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "D$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "S$", 2 ) == 0 ) ||
           ( idlOS::strMatch( sObjectName, 2, "V$", 2 ) == 0 ) ) )
    {
        if ( qcmFixedTable::getTableInfo( aStatement,
                                          (UChar*)aSynonymInfo->objectName,
                                          idlOS::strlen(aSynonymInfo->objectName),
                                          aTableInfo,
                                          aSCN,
                                          aTableHandle )
             == IDE_SUCCESS )
        {
            *aExist     = ID_TRUE;
        }
    }
    else
    {
        if ( qcm::getTableInfo(aStatement,
                               *aUserID,
                               (UChar*)aSynonymInfo->objectName,
                               idlOS::strlen(aSynonymInfo->objectName),
                               aTableInfo,
                               aSCN,
                               aTableHandle) == IDE_SUCCESS )
        {
            //gettableinfo success.
            *aExist = ID_TRUE;
        }
    }

    if ( *aExist == ID_FALSE )
    {
        IDE_CLEAR();

        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);

        if ( ID_TRUE == *aExist )
        {
            if ( checkSynonymCircularityArr( aSynonymArr,
                                             aSynonymInfo )
                 == ID_TRUE )
            {
                *aExist = ID_FALSE;
            }
        }

        if ( ID_TRUE == *aExist )
        {
            if ( addToSynonymArr( aSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
            {
                // resolve private synonym
                IDE_TEST( resolveTableViewQueueInternal4PlanCache( aStatement,
                                                                   aSynonymInfo,
                                                                   aTableInfo,
                                                                   aUserID,
                                                                   aSCN,
                                                                   aExist,
                                                                   aTableHandle,
                                                                   aSynonymArr )
                          != IDE_SUCCESS );
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserNamePos,
                                        sObjectNamePos,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if ( sExistObject == ID_FALSE )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);

                if ( *aExist == ID_TRUE )
                {
                    if ( checkSynonymCircularityArr( aSynonymArr,
                                                     aSynonymInfo )
                         == ID_TRUE )
                    {
                        *aExist = ID_FALSE;
                    }
                }

                if ( ID_TRUE == *aExist )
                {
                    if ( addToSynonymArr( aSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
                    {
                        //BUG-20855
                        if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                                idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                                sSynonymInfo.objectOwnerName,
                                                idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                             ( idlOS::strMatch( aSynonymInfo->objectName,
                                                idlOS::strlen(aSynonymInfo->objectName),
                                                sSynonymInfo.objectName,
                                                idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                        {
                            // no public synonym matched
                            *aExist = ID_FALSE;
                        }
                        else
                        {
                            IDE_TEST( resolveTableViewQueueInternal4PlanCache( aStatement,
                                                                               aSynonymInfo,
                                                                               aTableInfo,
                                                                               aUserID,
                                                                               aSCN,
                                                                               aExist,
                                                                               aTableHandle,
                                                                               aSynonymArr )
                                      != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        *aExist = ID_FALSE;
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveSequence4PlanCache( qcStatement     * aStatement,
                                              qcNamePosition    aUserName,
                                              qcNamePosition    aObjectName,
                                              qcmSequenceInfo * aSequenceInfo,
                                              idBool          * aExist,
                                              qcmSynonymInfo  * aSynonymInfo,
                                              void           ** aSequenceHandle )
{
/***********************************************************************
 *
 * Description :
 *     BUG-16980
 *     get sequence info with name
 *
 * Implementation :
 *     1. search sys_tables_
 *     2. search private synonym
 *     3. search public synonym
 *
 ***********************************************************************/
    idBool           sCheckPublic =  ID_FALSE;
    UInt             sTmpUserID;
    qsOID            sTmpObjectID = QS_EMPTY_OID;
    idBool           sExistObject = ID_FALSE;
    UInt             sUserID = 0;
    qcmSynonymArr    sSynonymArr;

    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;
    sSynonymArr.mCount = 0;

    if ( QC_IS_NULL_NAME(aUserName) == ID_TRUE )
    {
        // have to check private & public synonym.
        sUserID = QCG_GET_SESSION_USER_ID(aStatement);
        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, &sUserID )
                 != IDE_SUCCESS);
    }

    if ( qcm::getSequenceInfo(aStatement,
                              sUserID,
                              (UChar*) aObjectName.stmtText + aObjectName.offset,
                              aObjectName.size,
                              aSequenceInfo,
                              aSequenceHandle) == IDE_SUCCESS )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            sUserID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);

        if ( ID_TRUE == *aExist)
        {
            if ( addToSynonymArr( &sSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
            {
                IDE_TEST( resolveSequenceInternal4PlanCache( aStatement,
                                                             aSynonymInfo,
                                                             aSequenceInfo,
                                                             &sUserID,
                                                             aExist,
                                                             aSequenceHandle,
                                                             &sSynonymArr )
                          != IDE_SUCCESS);
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
        else
        {
            if ( sCheckPublic == ID_FALSE )
            {
                *aExist = ID_FALSE;
            }
            else
            {
                /* BUG- 39066
                   동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            aUserName,
                                            aObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if ( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST( getSynonym( aStatement,
                                          QC_PUBLIC_USER_ID,
                                          aObjectName.stmtText + aObjectName.offset,
                                          aObjectName.size,
                                          aSynonymInfo,
                                          aExist)
                               != IDE_SUCCESS);

                    if ( ID_TRUE == *aExist )
                    {
                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        if ( addToSynonymArr( &sSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
                        {
                            // resolve public synonym
                            IDE_TEST( resolveSequenceInternal4PlanCache( aStatement,
                                                                         aSynonymInfo,
                                                                         aSequenceInfo,
                                                                         &sUserID,
                                                                         aExist,
                                                                         aSequenceHandle,
                                                                         &sSynonymArr )
                                      != IDE_SUCCESS);
                        }
                        else
                        {
                            *aExist = ID_FALSE;
                        }
                    }
                    else
                    {
                        // no public synonym
                        // do nothing
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::resolveSequenceInternal4PlanCache( qcStatement      * aStatement,
                                                      qcmSynonymInfo   * aSynonymInfo,
                                                      qcmSequenceInfo  * aSequenceInfo,
                                                      UInt             * aUserID,
                                                      idBool           * aExist,
                                                      void            ** aSequenceHandle,
                                                      qcmSynonymArr    * aSynonymArr )
{
    qcmSynonymInfo   sSynonymInfo;
    qcNamePosition   sUserName;
    qcNamePosition   sObjectName;
    UInt             sTmpUserID;
    qsOID            sTmpObjectID = QS_EMPTY_OID;
    idBool           sExistObject = ID_FALSE;

    sUserName.stmtText = aSynonymInfo->objectOwnerName;
    sUserName.offset   = 0;
    sUserName.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectName.stmtText = aSynonymInfo->objectName;
    sObjectName.offset   = 0;
    sObjectName.size     = idlOS::strlen(aSynonymInfo->objectName);

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    if ( qcm::getSequenceInfo(aStatement,
                              *aUserID,
                              (UChar*)aSynonymInfo->objectName,
                              idlOS::strlen(aSynonymInfo->objectName),
                              aSequenceInfo,
                              aSequenceHandle) == IDE_SUCCESS )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);

        if ( ID_TRUE == *aExist )
        {
            if ( checkSynonymCircularityArr( aSynonymArr,
                                             aSynonymInfo )
                 == ID_TRUE )
            {
                *aExist = ID_FALSE;
            }
        }

        if ( ID_TRUE == *aExist )
        {
            if ( addToSynonymArr( aSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
            {
                // resolve private synonym
                IDE_TEST( resolveSequenceInternal4PlanCache( aStatement,
                                                             aSynonymInfo,
                                                             aSequenceInfo,
                                                             aUserID,
                                                             aExist,
                                                             aSequenceHandle,
                                                             aSynonymArr )
                           != IDE_SUCCESS);
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserName,
                                        sObjectName,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if ( sExistObject == ID_FALSE  )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);

                if ( ID_TRUE == *aExist )
                {
                    if ( checkSynonymCircularityArr( aSynonymArr,
                                                     aSynonymInfo )
                         == ID_TRUE )
                    {
                        *aExist = ID_FALSE;
                    }
                }

                if ( ID_TRUE == *aExist )
                {
                    if ( addToSynonymArr( aSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
                    {
                        //BUG-20855
                        if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                                idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                                sSynonymInfo.objectOwnerName,
                                                idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                             ( idlOS::strMatch( aSynonymInfo->objectName,
                                                idlOS::strlen(aSynonymInfo->objectName),
                                                sSynonymInfo.objectName,
                                                idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                        {
                            // no public synonym matched
                            *aExist = ID_FALSE;
                        }
                        else
                        {

                            IDE_TEST( resolveSequenceInternal4PlanCache( aStatement,
                                                                         aSynonymInfo,
                                                                         aSequenceInfo,
                                                                         aUserID,
                                                                         aExist,
                                                                         aSequenceHandle,
                                                                         aSynonymArr )
                                      != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        *aExist = ID_FALSE;
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

/* BUG-48594 */
IDE_RC qcmSynonym::resolvePSM4PlanCache( qcStatement    * aStatement,
                                         qcNamePosition   aUserName,
                                         qcNamePosition   aObjectName,
                                         qsOID          * aProcID,
                                         idBool         * aExist,
                                         qcmSynonymInfo * aSynonymInfo )
{
    idBool           sCheckPublic =  ID_FALSE;
    UInt             sTmpUserID;
    UInt             sUserID;
    qsOID            sTmpObjectID = QS_EMPTY_OID;
    idBool           sExistObject = ID_FALSE;
    qcmSynonymArr    sSynonymArr;

    aSynonymInfo->isSynonymName = ID_FALSE;
    aSynonymInfo->isPublicSynonym = ID_FALSE;
    sSynonymArr.mCount = 0;

    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        // have to check private & public synonym.
        sUserID = QCG_GET_SESSION_USER_ID(aStatement);
        sCheckPublic = ID_TRUE;
    }
    else
    {
        // only check private synonym.
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, &sUserID )
                 != IDE_SUCCESS);
        sCheckPublic = ID_FALSE;
    }

    IDE_TEST( qcmProc::getProcExistWithEmptyByNamePtr( aStatement,
                                                       sUserID,
                                                       (SChar*) (aObjectName.stmtText +
                                                                 aObjectName.offset),
                                                       aObjectName.size,
                                                       aProcID )
              != IDE_SUCCESS);

    if ( QS_EMPTY_OID != *aProcID )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            sUserID,
                            aObjectName.stmtText + aObjectName.offset,
                            aObjectName.size,
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);

        if ( ID_TRUE == *aExist )
        {
            if ( addToSynonymArr( &sSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
            {
                // resolve private synonym
                IDE_TEST(resolvePSMInternal4PlanCache( aStatement,
                                                       aSynonymInfo,
                                                       aProcID,
                                                       &sUserID,
                                                       aExist,
                                                       &sSynonymArr )
                         != IDE_SUCCESS);
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
        else
        {
            if ( sCheckPublic == ID_FALSE )
            {
                *aExist = ID_FALSE;
                // PSM not found
            }
            else
            {
                /* BUG-39048
                   동일한 이름을 가진 객체가 있는 확인필요 */
                IDE_TEST( qcm::existObject( aStatement,
                                            ID_FALSE,
                                            aUserName,
                                            aObjectName,
                                            QS_OBJECT_MAX,
                                            &sTmpUserID,
                                            &sTmpObjectID,
                                            &sExistObject )
                          != IDE_SUCCESS );

                if ( sExistObject == ID_FALSE )
                {
                    // check public synonym
                    IDE_TEST(getSynonym( aStatement,
                                         QC_PUBLIC_USER_ID,
                                         aObjectName.stmtText + aObjectName.offset,
                                         aObjectName.size,
                                         aSynonymInfo,
                                         aExist)
                             != IDE_SUCCESS);

                    if ( ID_TRUE == *aExist )
                    {
                        aSynonymInfo->isPublicSynonym = ID_TRUE;

                        if ( addToSynonymArr( &sSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
                        {
                            // resolve public synonym
                            IDE_TEST(resolvePSMInternal4PlanCache( aStatement,
                                                                   aSynonymInfo,
                                                                   aProcID,
                                                                   &sUserID,
                                                                   aExist,
                                                                   &sSynonymArr )
                                     != IDE_SUCCESS);
                        }
                        else
                        {
                            *aExist = ID_FALSE;
                        }
                    }
                    else
                    {
                        // no public synonym
                        // do nothing
                    }
                }
                else
                {
                    *aExist = ID_FALSE;
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

/* BUG-48594 */
IDE_RC qcmSynonym::resolvePSMInternal4PlanCache( qcStatement      * aStatement,
                                                 qcmSynonymInfo   * aSynonymInfo,
                                                 qsOID            * aProcID,
                                                 UInt             * aUserID,
                                                 idBool           * aExist,
                                                 qcmSynonymArr    * aSynonymArr )
{
    qcmSynonymInfo   sSynonymInfo;
    qcNamePosition   sUserName;
    qcNamePosition   sObjectName;
    UInt             sTmpUserID;
    qsOID            sTmpObjectID = QS_EMPTY_OID;
    idBool           sExistObject = ID_FALSE;

    sUserName.stmtText = aSynonymInfo->objectOwnerName;
    sUserName.offset   = 0;
    sUserName.size     = idlOS::strlen( aSynonymInfo->objectOwnerName );

    sObjectName.stmtText = aSynonymInfo->objectName;
    sObjectName.offset   = 0;
    sObjectName.size     = idlOS::strlen(aSynonymInfo->objectName);

    idlOS::memcpy( &sSynonymInfo, aSynonymInfo, ID_SIZEOF(qcmSynonymInfo) );

    IDE_TEST(qcmUser::getUserID( aStatement,
                                 aSynonymInfo->objectOwnerName,
                                 idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                 aUserID )
             != IDE_SUCCESS);

    IDE_TEST( qcmProc::getProcExistWithEmptyByNamePtr( aStatement,
                                                       *aUserID,
                                                       aSynonymInfo->objectName,
                                                       idlOS::strlen(aSynonymInfo->objectName),
                                                       aProcID )
              != IDE_SUCCESS);

    if ( QS_EMPTY_OID != *aProcID )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_CLEAR();
        // check private synonym
        IDE_TEST(getSynonym(aStatement,
                            *aUserID,
                            aSynonymInfo->objectName,
                            idlOS::strlen(aSynonymInfo->objectName),
                            aSynonymInfo,
                            aExist) != IDE_SUCCESS);

        if ( ID_TRUE == *aExist )
        {
            if ( checkSynonymCircularityArr( aSynonymArr,
                                             aSynonymInfo )
                 == ID_TRUE )
            {
                *aExist = ID_FALSE;
            }
        }

        if ( ID_TRUE == *aExist )
        {
            if ( addToSynonymArr( aSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
            {
                // resolve private synonym
                IDE_TEST( resolvePSMInternal4PlanCache( aStatement,
                                                        aSynonymInfo,
                                                        aProcID,
                                                        aUserID,
                                                        aExist,
                                                        aSynonymArr )
                          != IDE_SUCCESS);
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
        else
        {
            /* BUG- 39066
               동일한 이름을 가진 객체가 존재하는지 public synonym 찾기 전에 체크 */
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sUserName,
                                        sObjectName,
                                        QS_OBJECT_MAX,
                                        &sTmpUserID,
                                        &sTmpObjectID,
                                        &sExistObject )
                      != IDE_SUCCESS );

            if ( sExistObject == ID_FALSE )
            {
                // check public synonym
                IDE_TEST(getSynonym(aStatement,
                                    QC_PUBLIC_USER_ID,
                                    aSynonymInfo->objectName,
                                    idlOS::strlen(aSynonymInfo->objectName),
                                    aSynonymInfo,
                                    aExist) != IDE_SUCCESS);

                if ( ID_TRUE == *aExist )
                {
                    if ( checkSynonymCircularityArr( aSynonymArr,
                                                     aSynonymInfo )
                         == ID_TRUE )
                    {
                         *aExist = ID_FALSE;
                    }
                }
                if ( ID_TRUE == *aExist )
                {
                    if ( addToSynonymArr( aSynonymArr, aSynonymInfo ) == IDE_SUCCESS )
                    {
                        //BUG-20855
                        if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                                idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                                sSynonymInfo.objectOwnerName,
                                                idlOS::strlen( sSynonymInfo.objectOwnerName ) ) == 0 ) &&
                             ( idlOS::strMatch( aSynonymInfo->objectName,
                                                idlOS::strlen(aSynonymInfo->objectName),
                                                sSynonymInfo.objectName,
                                                idlOS::strlen( sSynonymInfo.objectName ) ) == 0 ) )
                        {
                            // no public synonym matched
                            *aExist = ID_FALSE;
                        }
                        else
                        {
                            IDE_TEST( resolvePSMInternal4PlanCache( aStatement,
                                                                    aSynonymInfo,
                                                                    aProcID,
                                                                    aUserID,
                                                                    aExist,
                                                                    aSynonymArr )
                                      != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        *aExist = ID_FALSE;
                    }
                }
                else
                {
                    // no public synonym
                    // do nothing
                }
            }
            else
            {
                *aExist = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExist = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qcmSynonym::addToSynonymArr( qcmSynonymArr  * aSynonymArr,
                                    qcmSynonymInfo * aSynonymInfo )
{
    UInt sSize = 0;

    IDE_TEST_RAISE( aSynonymArr->mCount >= QCM_MAX_RESOLVE_SYNONYM, ERR_UNEXPECTED_MAX_RESOLVE );

    sSize = idlOS::strlen(aSynonymInfo->objectOwnerName);

    IDE_TEST_RAISE( sSize > QCM_MAX_SYNOM_NAME_LEN, ERR_UNEXPECT_INVALID_SIZE );

    if ( sSize > 0 )
    {
        idlOS::memcpy( aSynonymArr->mInfo[aSynonymArr->mCount].mObjectOwnerName,
                       aSynonymInfo->objectOwnerName,
                       sSize );
        aSynonymArr->mInfo[aSynonymArr->mCount].mObjectOwnerName[sSize] = '\0';
    }
    else
    {
        aSynonymArr->mInfo[aSynonymArr->mCount].mObjectOwnerName[0] = '\0';
    }

    sSize = idlOS::strlen(aSynonymInfo->objectName);

    IDE_TEST_RAISE( sSize > QCM_MAX_SYNOM_NAME_LEN, ERR_UNEXPECT_INVALID_SIZE );

    if ( sSize > 0 )
    {
        idlOS::memcpy( aSynonymArr->mInfo[aSynonymArr->mCount].mObjectName,
                       aSynonymInfo->objectName,
                       sSize );
        aSynonymArr->mInfo[aSynonymArr->mCount].mObjectName[sSize] = '\0';
    }
    else
    {
        aSynonymArr->mInfo[aSynonymArr->mCount].mObjectName[0] = '\0';
    }
    aSynonymArr->mCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_MAX_RESOLVE )
    {
        ideLog::log( IDE_QP_32, "qcmSynonym::addToSynonymArr over resolve synonym" );
    }
    IDE_EXCEPTION( ERR_UNEXPECT_INVALID_SIZE )
    {
        ideLog::log( IDE_QP_32, "qcmSynonym::addToSynonymArr over max synonym name len" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qcmSynonym::checkSynonymCircularityArr( qcmSynonymArr  * aSynonymArr,
                                               qcmSynonymInfo * aSynonymInfo )
{
    UInt              i;
    idBool            sCheck = ID_FALSE;

    for ( i = 0 ; i < aSynonymArr->mCount; i++ )
    {
        if ( ( i + 1 ) > QCM_MAX_RESOLVE_SYNONYM )
        {
            sCheck = ID_TRUE;
            break;
        }

        // 동일한 객체를 가리키는 링크가 있는지 검사
        if ( ( idlOS::strMatch( aSynonymInfo->objectOwnerName,
                                idlOS::strlen( aSynonymInfo->objectOwnerName ),
                                aSynonymArr->mInfo[i].mObjectOwnerName,
                                idlOS::strlen( aSynonymArr->mInfo[i].mObjectOwnerName ) ) == 0 ) &&
             ( idlOS::strMatch( aSynonymInfo->objectName,
                                idlOS::strlen( aSynonymInfo->objectName ),
                                aSynonymArr->mInfo[i].mObjectName,
                                idlOS::strlen( aSynonymArr->mInfo[i].mObjectName ) ) == 0 ) )
        {
            sCheck = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sCheck;
}

