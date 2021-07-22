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
 * $Id: qcuTemporaryObj.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/
#ifndef _O_QCU_TEMPORARYOBJ_H_
#define _O_QCU_TEMPORARYOBJ_H_  1

#include <qc.h>
#include <qcmTableInfo.h>
#include <qsxEnv.h>

#define QCU_TEMPORARY_TABLE_INIT_SIZE  10

typedef struct qcuSessionTempObj
{
    UInt        baseTableID;
    SInt        sessionTableCount;
} qcuSessionTempObj ;

typedef struct qcuSessionTempObjMgr
{
    qcuSessionTempObj * tables;
    SInt                tableCount;
    SInt                tableAllocCount;
    iduMutex            mutex;
} qcuSessionTempObjMgr ;

class qcuTemporaryObj
{
public:
    static void initTemporaryObj( qcTemporaryObjInfo * aTemporaryObj );

    static void finalizeTemporaryObj( idvSQL             * aStatistics,
                                      qcTemporaryObjInfo * aTemporaryObj );

    static void dropAllTempTables( idvSQL             * aStatistics,
                                   qcTemporaryObjInfo * aTemporaryObj,
                                   qcmTemporaryType     aTemporaryType );

    static IDE_RC createAndGetTempTable( qcStatement    * aStatement,
                                         qcmTableInfo   * aTableInfo,
                                         const void    ** aTempTableHandle );

    static void getTempTableHandle( qcStatement    * aStatement,
                                    qcmTableInfo   * aTableInfo,
                                    const void    ** aTableHandle,
                                    smSCN          * aTableSCN );

    static const void * getTempIndexHandle( qcStatement    * aStatement,
                                            qcmTableInfo   * aTableInfo,
                                            UInt             aIndexID );

    static idBool isTemporaryTable( qcmTableInfo * aTableInfo );

    static IDE_RC truncateTempTable( qcStatement     * aStatement,
                                     UInt              aTableID,
                                     qcmTemporaryType  aTemporaryType );

    static idBool existSessionTable( qcmTableInfo  * aTableInfo );

    static IDE_RC initializeStatic();

    static void finilizeStatic();

private:
    static void initTempTables( qcTempTables * aTempTables );

    static IDE_RC dropTempTable( idvSQL        * aStatistics,
                                 qcTempTable   * aTempTable,
                                 idBool          aIsDDL );

    static IDE_RC createTempTable( qcStatement  * aStatement,
                                   qcmTableInfo * aTableInfo,
                                   qcTempTable  * aTempTable );

    static IDE_RC expandTempTableInfo( void   ** aTempTableInfo,
                                       UInt      aObjectSize,
                                       SInt   *  aAllocCount );

    static IDE_RC expandTempTables( qcTempTables *  aTempTables );

    static IDE_RC expandSessionTempObj();

    static qcTempTable * getTempTable( qcTemporaryObjInfo  * aTemporaryObj,
                                       UInt                  aTableID,
                                       qcmTemporaryType      aTemporaryType );

    static qcTempTables * getTempTables( qcTemporaryObjInfo  * aTemporaryObj,
                                         qcmTemporaryType      aTemporaryType );

    static IDE_RC createTempIndices( idvSQL               * aStatistics,
                                     smiStatement         * aStatement,
                                     qcmTableInfo         * aTableInfo,
                                     qcTempTable          * aTempTable,
                                     scSpaceID              aTableTBSID );

    static inline IDE_RC incSessionTableCount( UInt  aTableID );
    static inline void   decSessionTableCount( UInt  aTableID );

    static inline SInt getSessionTableCount( UInt  aTableID );

    static inline qcuSessionTempObj * getSessionTempObj( UInt  aTableID );

    static IDE_RC allocSessionTempObj( UInt                 aBaseTableID,
                                       qcuSessionTempObj ** aSessionTempObj );

    static inline qcTemporaryObjInfo * getTemporaryObj( qcStatement  * aStatement )
    {
        IDE_ASSERT( aStatement != NULL );
        IDE_ASSERT( aStatement->spxEnv != NULL );
        IDE_ASSERT( aStatement->spxEnv->mSession != NULL );

        return &(aStatement->session->mQPSpecific.mTemporaryObj);
    };

    static inline void lock();

    static inline void unlock();
};
#endif // _O_QCU_TEMPORARYOBJ_H_
