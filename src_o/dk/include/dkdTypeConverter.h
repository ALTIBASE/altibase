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
 * $id$
 **********************************************************************/

#ifndef _O_DKD_TYPE_CONVERTER_H_
#define _O_DKD_TYPE_CONVERTER_H_

#include <dkp.h>

typedef struct dkdTypeConverter dkdTypeConverter;

extern IDE_RC dkdTypeConverterCreate( dkpColumn * aColumnArray,
                                      UInt aColumnCount,
                                      dkdTypeConverter ** aConverter );
extern IDE_RC dkdTypeConverterDestroy( dkdTypeConverter * aConverter );

extern IDE_RC dkdTypeConverterGetOriginalMeta( dkdTypeConverter * aConverter,
                                               dkpColumn ** aMeta );

extern IDE_RC dkdTypeConverterGetConvertedMeta( dkdTypeConverter * aConverter,
                                                mtcColumn ** aMeta );
 
extern IDE_RC dkdTypeConverterGetRecordLength( dkdTypeConverter * aConverter,
                                               UInt * aRecordLength );

extern IDE_RC dkdTypeConverterGetColumnCount( dkdTypeConverter * aConverter,
                                              UInt * aColumnCount );

extern IDE_RC dkdTypeConverterConvertRecord( dkdTypeConverter * aConverter,
                                             void * aSrcRecord,
                                             void * aDestRecord );

extern IDE_RC dkdTypeConverterMakeNullRow( dkdTypeConverter * aConverter,
                                           void * aDestRecord );

#endif /* _O_DKD_TYPE_CONVERTER_H_ */
