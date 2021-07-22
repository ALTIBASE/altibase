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
 * $Id: qcmLibrary.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 **********************************************************************/

#ifndef _O_QCM_LIBRARY_H_
#define _O_QCM_LIBRARY_H_

#include <qcm.h>

class qcmLibrary
{
public:
    static IDE_RC getLibrary( qcStatement      * aStatement,
                              UInt               aUserID,
                              SChar            * aLibraryName,
                              SInt               aLibraryNameLen,
                              qcmLibraryInfo   * aLibraryInfo,
                              idBool           * aExist );
    
    static IDE_RC addMetaInfo( qcStatement      * aStatement,
                               UInt               aUserID,
                               SChar            * aLibraryName,
                               SChar            * aFileSpec,
                               idBool             aReplace );
    
    static IDE_RC delMetaInfo( qcStatement * aStatement,
                               UInt          aUserID,
                               SChar       * aLibraryName );
};

IDE_RC qcmSetLibrary( idvSQL           * aStatistics,
                      const void       * aRow,
                      qcmLibraryInfo   * aLibraryInfo);

#endif // _O_QCM_LIBRARY_H_
