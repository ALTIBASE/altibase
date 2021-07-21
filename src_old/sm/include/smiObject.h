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
 * $Id: smiObject.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMI_OBJECT_H_
# define _O_SMI_OBJECT_H_ 1

# include <smiDef.h>

class smiObject
{
public:
    static IDE_RC createObject( smiStatement    * aStatement,
                                const void      * aInfo,
                                UInt              aInfoSize,
                                const void      * aTempInfo,
                                smiObjectType     aObjectType,
                                const void     ** aTable );

    static IDE_RC dropObject(smiStatement    *aStatement,
                             const void      *aTable);

    static void  getObjectInfo( const void  * aTable,
                                void       ** aObjectInfo );
    static void  getObjectInfoSize( const void* aTable, UInt *aInfoSize );
    static IDE_RC setObjectInfo( smiStatement    *aStatement,
                                 const void      *aTable,
                                 void            *aInfo,
                                 UInt             aInfoSize);

    static IDE_RC getObjectTempInfo( const void    * aTable,
                                     void         ** aRuntimeInfo );
    static IDE_RC setObjectTempInfo( const void     * aTable,
                                     void           * aTempInfo );
};

#endif /* _O_SMI_OBJECT_H_ */
