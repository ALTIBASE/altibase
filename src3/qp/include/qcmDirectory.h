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
 * $Id: qcmDirectory.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 *     [PROJ-1371] Directories
 *
 *     Directory를 위한 메타 관리
 *
 **********************************************************************/

#ifndef _O_QCM_DIRECTORY_H_
#define _O_QCM_DIRECTORY_H_

#include <qcm.h>

typedef struct qcmDirectoryInfo
{
    smOID     directoryID;
    UInt      userID;
    SChar     directoryName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar     directoryPath[QCM_MAX_DEFAULT_VAL + 1];
} qcmDirectoryInfo;

class qcmDirectory
{
public:
    static IDE_RC getDirectory( qcStatement      * aStatement,
                                SChar            * aDirectoryName,
                                SInt               aDirectoryNameLen,
                                qcmDirectoryInfo * aDirectoryInfo,
                                idBool           * aExist );
    
    static IDE_RC addMetaInfo( qcStatement      * aStatement,
                               UInt               aUserID,
                               SChar            * aDirectoryName,
                               SChar            * aDirectoryPath,
                               idBool             aReplace );
    
    static IDE_RC delMetaInfoByDirectoryName( qcStatement * aStatement,
                                              SChar       * aDirectoryName );
};

IDE_RC qcmSetDirectory( idvSQL           * aStatistics,
                        const void       * aRow,
                        qcmDirectoryInfo * aDirectoryInfo);

#endif // _O_QCM_DIRECTORY_H_
