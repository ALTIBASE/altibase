/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _O_ULN_CONVERSION_H_
#define _O_ULN_CONVERSION_H_ 1

#include <ulnConfig.h>

#if ULN_USE_GETDATA_EXTENSIONS

typedef struct ulnValue ulnValue;

struct ulnValue
{
    UChar *mData;       /* Data Buffer Pointer              */
    SInt  *mIndPtr;     /* Values Length/Indicator Pointer  */
    SInt   mLength;     /* Maximum Buffer size of it        */

    UShort mIndex;      /* ColumnNumber/ParamNumber of Value*/
};

IDE_RC ulnConvertValue(ulnFnContext *aContext,
                       ulnValue           *aSource,
                       ulnValue           *aTarget,
                       idBool              aIsGetData);

#endif  /* #if ULN_USE_GETDATA_EXTENSIONS */

#endif  /* _O_ULN_CONVERSION_H_ */

