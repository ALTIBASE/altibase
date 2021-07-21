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
 
#include <idl.h>

int main()
{
    SChar *platform = OS_TARGET " " OS_MAJORVER " "OS_MINORVER;
    SChar *bit =
        "ID_SINT_FMT(ID_INT32_FMT)="ID_INT32_FMT
        "ID_UINT_FMT(ID_UINT32_FMT)="ID_UINT32_FMT
        
        "ID_SLONG_FMT(ID_INT64_FMT)="ID_INT64_FMT
        "ID_ULONG_FMT(ID_UINT64_FMT)="ID_UINT64_FMT
        
        "ID_vSLONG_FMT(ID_vSLONG_FMT)="ID_vSLONG_FMT
        "ID_vULONG_FMT(ID_vULONG_FMT)="ID_vULONG_FMT
        
        "ID_xUINT_FMT(ID_xINT32_FMT)="ID_xINT32_FMT
        "ID_XUINT_FMT(ID_XINT32_FMT)="ID_XINT32_FMT
        
        "ID_xULONG_FMT(ID_xINT64_FMT)="ID_xINT64_FMT
        "ID_XULONG_FMT(ID_XINT64_FMT)="ID_XINT64_FMT
        
        "ID_xvULONG_FMT(ID_vxULONG_FMT)="ID_vxULONG_FMT
        "ID_XvULONG_FMT(ID_vXULONG_FMT)="ID_vXULONG_FMT
        
        "ID_PTR_FMT(ID_POINTER_FMT)="ID_POINTER_FMT
        
        "ID_xPTR_FMT(ID_xPOINTER_FMT)="ID_xPOINTER_FMT
        "ID_XPTR_FMT(ID_XPOINTER_FMT)="ID_XPOINTER_FMT
        ;
    
#if defined(COMPILE_64BIT)
    fprintf(stderr, "64 BIT!!");
#else
    fprintf(stderr, "32 BIT!!");
#endif
    
    return 0;
}

