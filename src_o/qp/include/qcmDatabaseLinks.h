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
 

#ifndef _O_QCM_DATABASE_LINKS_H_
#define _O_QCM_DATABASE_LINKS_H_ 1

/* column index number of SYS_DATABASE_LINKS_ */
#define QCM_DATABASE_LINKS_USER_ID_COL_ORDER                (0)
#define QCM_DATABASE_LINKS_LINK_ID_COL_ORDER                (1)
#define QCM_DATABASE_LINKS_LINK_OID_COL_ORDER               (2)
#define QCM_DATABASE_LINKS_LINK_NAME_COL_ORDER              (3)
#define QCM_DATABASE_LINKS_USER_MODE_COL_ORDER              (4)
#define QCM_DATABASE_LINKS_REMOTE_USER_ID_COL_ORDER         (5)
#define QCM_DATABASE_LINKS_REMOTE_USER_PASSWORD_COL_ORDER   (6)
#define QCM_DATABASE_LINKS_LINK_TYPE_COL_ORDER              (7)
#define QCM_DATABASE_LINKS_TARGET_NAME_COL_ORDER            (8)

typedef struct qcmDatabaseLinksItem
{
    UInt     userID;
    UInt     linkID;
    smOID    linkOID;
    SChar    linkName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SInt     userMode;
    SChar    remoteUserID[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    UChar    remoteUserPassword[ QC_MAX_NAME_LEN + 1];
    SInt     linkType;
    SChar    targetName[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    struct qcmDatabaseLinksItem * next;
    
} qcmDatabaseLinksItem;

IDE_RC qcmDatabaseLinksInitializeMetaHandle( smiStatement * aQcStatement );

IDE_RC qcmGetNewDatabaseLinkId( qcStatement * aStatement,
                                UInt        * aDatabaseLinkId );

IDE_RC qcmDatabaseLinksInsertItem( qcStatement          * aQcStatement,
                                   qcmDatabaseLinksItem * aItem,
                                   idBool                 aPublicFlag );

IDE_RC qcmDatabaseLinksDeleteItem( qcStatement * aStatement,
                                   smOID         aLinkOID );
IDE_RC qcmDatabaseLinksDeleteItemByUserId( qcStatement * aStatement,
                                           UInt          aUserId );

IDE_RC qcmDatabaseLinksGetFirstItem( idvSQL * aStatistics, qcmDatabaseLinksItem ** aFirstItem );
IDE_RC qcmDatabaseLinksNextItem( qcmDatabaseLinksItem  * aCurrentItem,
                                 qcmDatabaseLinksItem ** aNextItem );
IDE_RC qcmDatabaseLinksFreeItems( qcmDatabaseLinksItem * aFirstItem );

IDE_RC qcmDatabaseLinksIsUserExisted( qcStatement * aStatement,
                                      UInt          aUserId,
                                      idBool      * aExistedFlag );

#endif /* _O_QCM_DATABASE_LINKS_ */
