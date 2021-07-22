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
 * $Id: smrCompareLSN.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMR_COMPARE_LSN_H_
#define _O_SMR_COMPARE_LSN_H_ 1

#include <smDef.h>

class smrCompareLSN 
{
//For Operation
public:
    static idBool isGT(const smLSN*  aLSN1, 
                       const smLSN*  aLSN2);

    static idBool isGTE(const smLSN*  aLSN1, 
                        const smLSN*  aLSN2);

    static idBool isEQ(const smLSN*  aLSN1, 
                       const smLSN*  aLSN2);

    static idBool isLTE(const smLSN*  aLSN1, 
                        const smLSN*  aLSN2);

    static idBool isLT(const smLSN*  aLSN1, 
                       const smLSN*  aLSN2);

    static idBool isZero(const smLSN*  aLSN);
    
};

#endif /* _O_SMR_COMPARE_LSN_H_ */
