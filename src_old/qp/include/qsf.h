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
 * $Id: qsf.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     extended function들의 모음
 *
 **********************************************************************/

#ifndef _O_QSF_H_
#define _O_QSF_H_ 1

#include <idl.h>
#include <mt.h>
#include <qs.h>

// BUG-42464 dbms_alert package
#define QSF_MAX_EVENT_NAME        (30)
#define QSF_MAX_EVENT_MESSAGE     (1800)

#define QSF_DEFAULT_POLL_INTERVAL (1)
#define QSF_MAX_POLL_INTERVAL     (30)

// BUG-40854
#define QSF_TX_MAX_BUFFER_SIZE (32767)

class qsf
{
public:

    static IDE_RC initialize();

    static IDE_RC finalize();

    static IDE_RC moduleByName( const mtfModule** aModule,
                                idBool*           aExist,
                                const void*       aName,
                                UInt              aLength );

    static IDE_RC makeFilePath( iduMemory   * aMem,
                                SChar      ** aFilePath,
                                SChar       * aPath,
                                mtdCharType * aFilename );

    static const mtfModule * mFuncModules[];

    static const mtfModule * extendedFunctionModules[];

    static IDE_RC getArg( mtcStack   * aStack,
                          UInt         aIdx,
                          idBool       aNotNull,
                          qsInOutType  aInOutType,
                          void      ** aRetPtr );
};

#endif /* _O_QSF_H_ */
