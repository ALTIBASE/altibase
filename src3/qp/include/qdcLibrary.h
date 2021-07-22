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
 * $Id: qdcLibrary.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 **********************************************************************/

#ifndef _O_QDC_LIBRARY_H_
#define _O_QDC_LIBRARY_H_

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

class qdcLibrary
{
public:
    // create
    static IDE_RC validateCreate( qcStatement * aStatement );
    static IDE_RC executeCreate( qcStatement * aStatement );
    
    // drop
    static IDE_RC validateDrop( qcStatement * aStatement );
    static IDE_RC executeDrop( qcStatement * aStatement );

    // alter 
    static IDE_RC validateAlter( qcStatement * aStatement );
    static IDE_RC executeAlter( qcStatement * aStatement );
};

#endif // _O_QDC_LIBRARY_H_
