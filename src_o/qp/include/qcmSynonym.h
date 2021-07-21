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
 * $Id: qcmSynonym.h 90130 2021-03-05 02:10:09Z donovan.seo $
 **********************************************************************/

#ifndef _O_QCM_SYNONYMS_H_
#define _O_QCM_SYNONYMS_H_ 1

#include <qc.h>
#include <qcm.h>
#include <qtc.h>

/***********************************************************************
 * qcmSynonym::resolveObject에서 사용할 객체의 type 정의
***********************************************************************/
#define QCM_OBJECT_TYPE_TABLE                   (0)
#define QCM_OBJECT_TYPE_SEQUENCE                (1)
#define QCM_OBJECT_TYPE_PSM                     (2)
#define QCM_OBJECT_TYPE_LINK                    (3)
#define QCM_OBJECT_TYPE_LIBRARY                 (4) // PROJ-1685
#define QCM_OBJECT_TYPE_PACKAGE                 (5) // PROJ-1073 Package
#define QCM_MAX_SYNOM_NAME_LEN                  (45)

typedef struct qcmSynonymInfo
{
    SChar   objectOwnerName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   objectName[QC_MAX_OBJECT_NAME_LEN + 1];
    idBool  isSynonymName;
    idBool  isPublicSynonym;
} qcmSynonymInfo;

typedef struct qcmSynonymInfo2
{
    SChar   mObjectOwnerName[QCM_MAX_SYNOM_NAME_LEN + 1];
    SChar   mObjectName[QCM_MAX_SYNOM_NAME_LEN + 1];
} qcmSynonymInfo2;

/* BUG-48594
 * Synonym이 plan cache에 등록된 경우 순환구조를 확인하기 위한 구조체
 */
typedef struct qcmSynonymArr
{
    qcmSynonymInfo2  mInfo[QCM_MAX_RESOLVE_SYNONYM];
    UShort           mCount;
} qcmSynonymArr;

class qcmSynonym
{
private:
    static IDE_RC resolveTableViewQueueInternal(
        qcStatement    * aStatement,
        qcmSynonymInfo * aSynonymInfo,
        qcmTableInfo  ** aTableInfo,
        UInt           * aUserID,
        smSCN          * aSCN,
        idBool         * aExist,
        void          ** aTableHandle,
        iduList        * aSynonymInfoList);

    /* BUG-48594 */
    static IDE_RC resolveTableViewQueueInternal4PlanCache( qcStatement      * aStatement,
                                                           qcmSynonymInfo   * aSynonymInfo,
                                                           qcmTableInfo    ** aTableInfo,
                                                           UInt             * aUserID,
                                                           smSCN            * aSCN,
                                                           idBool           * aExist,
                                                           void            ** aTableHandle,
                                                           qcmSynonymArr    * aSynonymArr );

    static IDE_RC resolveSequenceInternal(
        qcStatement     * aStatement,
        qcmSynonymInfo  * aSynonymInfo,
        qcmSequenceInfo * aSequenceInfo,
        UInt            * aUserID,
        idBool          * aExist,
        void           ** aSequenceHandle,
        iduList         * aSynonymInfoList);

    /* BUG-48594 */
    static IDE_RC resolveSequenceInternal4PlanCache( qcStatement      * aStatement,
                                                     qcmSynonymInfo   * aSynonymInfo,
                                                     qcmSequenceInfo  * aSequenceInfo,
                                                     UInt             * aUserID,
                                                     idBool           * aExist,
                                                     void            ** aSequenceHandle,
                                                     qcmSynonymArr    * aSynonymArr );

    static IDE_RC resolvePSMInternal(
        qcStatement    * aStatement,
        qcmSynonymInfo * aSynonymInfo,
        qsOID          * aProcID,
        UInt           * aUserID,
        idBool         * aExist,
        iduList        * aSynonymInfoList);

    /* BUG-48594 */
    static IDE_RC resolvePSMInternal4PlanCache( qcStatement      * aStatement,
                                                qcmSynonymInfo   * aSynonymInfo,
                                                qsOID            * aProcID,
                                                UInt             * aUserID,
                                                idBool           * aExist,
                                                qcmSynonymArr    * aSynonymArr );

    static IDE_RC resolveObjectInternal(
        qcStatement     * aStatement,
        qcmSynonymInfo  * aSynonymInfo,
        iduList         * aSynonymInfoList,
        idBool          * aExist,
        UInt            * aUserID,
        UInt            * aObjectType,
        void           ** aObjectHandle,
        UInt            * aObjectID );

    // PROJ-1685
    static IDE_RC resolveLibraryInternal(
        qcStatement    * aStatement,
        qcmSynonymInfo * aSynonymInfo,
        qcmLibraryInfo * aLibraryInfo,
        UInt           * aUserID,
        idBool         * aExist,
        iduList        * aSynonymInfoList);

    static IDE_RC checkSynonymCircularity(
        qcStatement    * aStatement,
        iduList        * aList,
        qcmSynonymInfo * aSynonymInfo );

    static idBool checkSynonymCircularityArr( qcmSynonymArr  * aSynonymArr,
                                              qcmSynonymInfo * aSynonymInfo );

    static IDE_RC addToSynonymInfoList(
        qcStatement    * aStatement,
        iduList        * aList,
        qcmSynonymInfo * aSynonymInfo );

    static IDE_RC addToSynonymArr( qcmSynonymArr  * aSynonymArr,
                                   qcmSynonymInfo * aSynonymInfo );

    // PROJ-1073 Package
    static IDE_RC resolvePkgInternal(
        qcStatement    * aStatement,
        qcmSynonymInfo * aSynonymInfo,
        qsOID          * aPkgID,
        UInt           * aUserID,
        idBool         * aExist,
        iduList        * aSynonymInfoList);

public:
    // get schema_nane and object_name using userid and synonymname
    static IDE_RC getSynonym(
        qcStatement      * aStatement,
        UInt               aUserID,
        SChar            * aSynonymName,
        SInt               aSynonymNameSize,
        qcmSynonymInfo   * aSynonymInfo,
        idBool           * aExist);

    static IDE_RC getSynonymOwnerID(
        qcStatement     *aStatement,
        UInt            aUserID,
        SChar          *aSynonymName,
        SInt            aSynonymNameSize,
        UInt           *aSynonymOwnerID,
        idBool         *aExist);

    static IDE_RC resolveTableViewQueue(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        qcNamePosition    aObjectName,
        qcmTableInfo   ** aTableInfo,
        UInt            * aUserID,
        smSCN           * aSCN,
        idBool          * aExist,
        qcmSynonymInfo  * aSynonymInfo,
        void           ** aTableHandle);

    /* BUG-48594 */
    static IDE_RC resolveTableViewQueue4PlanCache( qcStatement     * aStatement,
                                                   qcNamePosition    aUserName,
                                                   qcNamePosition    aObjectName,
                                                   idBool          * aExist,
                                                   qcmSynonymInfo  * aSynonymInfo,
                                                   void           ** aTableHandle );

    static IDE_RC resolveSequence(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        qcNamePosition    aObjectName,
        qcmSequenceInfo * aSequenceInfo,
        UInt            * aUserID,
        idBool          * aExist,
        qcmSynonymInfo  * aSynonymInfo,
        void           ** aSequenceHandle);

    /* BUG-48594 */
    static IDE_RC resolveSequence4PlanCache( qcStatement     * aStatement,
                                             qcNamePosition    aUserName,
                                             qcNamePosition    aObjectName,
                                             qcmSequenceInfo * aSequenceInfo,
                                             idBool          * aExist,
                                             qcmSynonymInfo  * aSynonymInfo,
                                             void           ** aSequenceHandle);

    static IDE_RC resolvePSM(
        qcStatement    * aStatement,
        qcNamePosition   aUserName,
        qcNamePosition   aObjectName,
        qsOID          * aProcID,
        UInt           * aUserID,
        idBool         * aExist,
        qcmSynonymInfo * aSynonymInfo);

    /* BUG-48594 */
    static IDE_RC resolvePSM4PlanCache( qcStatement    * aStatement,
                                        qcNamePosition   aUserName,
                                        qcNamePosition   aObjectName,
                                        qsOID          * aProcID,
                                        idBool         * aExist,
                                        qcmSynonymInfo * aSynonymInfo );

    static IDE_RC resolveObject(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        qcNamePosition    aObjectName,
        qcmSynonymInfo  * aSynonymInfo,
        idBool          * aExist,
        UInt            * aUserID,
        UInt            * aObjectType,
        void           ** aObjectHandle,
        UInt            * aObjectID );

    // PROJ-1685
    static IDE_RC resolveLibrary(
        qcStatement    * aStatement,
        qcNamePosition   aUserName,
        qcNamePosition   aObjectName,
        qcmLibraryInfo * aLibraryInfo,
        UInt           * aUserID,
        idBool         * aExist,
        qcmSynonymInfo * aSynonymInfo);

    // PROJ-1073 Package
    static IDE_RC resolvePkg( 
        qcStatement    * aStatement,
        qcNamePosition   aUserName,
        qcNamePosition   atableName,
        qsOID          * aPkgID,
        UInt           * aUserID,
        idBool         * aExist,
        qcmSynonymInfo * aSynonymInfo );

    // delete all
    static IDE_RC cascadeRemove(
        qcStatement    * aStatement,
        UInt             aUserID);

    static IDE_RC addSynonymToRelatedObject(
        qcStatement       * aStatement,
        SChar             * aOwnerName,
        SChar             * aObjectName,
        SInt                aObjectType);
};

IDE_RC qcmSetSynonym(
    idvSQL * aStatistics,
    const void * aRow,
    qcmSynonymInfo *aSynonymInfo);

IDE_RC qcmSetSynonymOwnerID(
    idvSQL * aStatistics,
    const void * aRow,
    UInt * aSynonymOwnerID);

#endif /* _O_QCM_SYNONYMS_H_ */
