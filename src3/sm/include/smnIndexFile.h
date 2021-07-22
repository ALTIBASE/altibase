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
 * $Id: smnIndexFile.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMN_INDEX_FILE_H_
#define _O_SMN_INDEX_FILE_H_ 1

#include <idu.h>
#include <smDef.h>

class smnIndexFile
{
public:
    IDE_RC initialize(smOID a_oidTable, UInt a_nOffset);
    IDE_RC destroy();

    IDE_RC create();
    IDE_RC write(SChar *a_pRow, idBool a_bFinal);
    IDE_RC read( scSpaceID    aSpaceID,
                 SChar      * aReadBuffer[],
                 UInt         aReadWantCnt,
                 UInt       * aReadCnt );
    
    IDE_RC dump( SChar * aName, idBool aChangeEndian );

    IDE_RC open();
    IDE_RC close();
            
    smnIndexFile();
    virtual ~smnIndexFile();

//For Member
public:
    UInt         m_cItemInBuffer;
    UInt         m_cLeftInBuffer;
    UInt         m_cMaxItemInBuffer;
    UInt         m_nOffset;
    UInt         m_cReadItemInBuffer;

    idBool       m_bEnd;
    
    iduFile      m_file;
    smOID*       mBufferPtr;
    smOID*       m_pAlignedBuffer;
};

#endif
