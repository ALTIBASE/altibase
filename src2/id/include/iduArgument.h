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
 
#ifndef _O_IDU_ARGUMENT_H_
# define _O_IDU_ARGUMENT_H_ 1

#include <idTypes.h>

typedef struct iduArgumentHandler iduArgumentHandler;

typedef IDE_RC (*iduInitFunc)( iduArgumentHandler* aHandler );
typedef IDE_RC (*iduArrangeFunc)( iduArgumentHandler* aHandler );
typedef IDE_RC (*iduFiniFunc)( iduArgumentHandler* aHandler );
typedef IDE_RC (*iduHandlerFunc)( iduArgumentHandler* aHandler,
                                  const UChar***      aArguments );

struct iduArgumentHandler {
    void*          info;
    iduInitFunc    init;
    iduArrangeFunc arrange;
    iduFiniFunc    fini;
    iduHandlerFunc handler;
};

class iduArgument {
 public:
    static IDE_RC initHandlers( iduArgumentHandler** aHandler );
    
    static IDE_RC arrangeHandlers( iduArgumentHandler** aHandler );
    
    static IDE_RC finiHandlers( iduArgumentHandler** aHandler );
    
    static IDE_RC parseArguments( iduArgumentHandler** aHandler,
                                  const UChar**        aArgument );
};

#endif /* _O_IDU_ARGUMENT_H_ */
