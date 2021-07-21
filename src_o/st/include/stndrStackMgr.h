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
 * $Id: stndrStackMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#ifndef _O_STNDR_STACK_MGR_H_
#define _O_STNDR_STACK_MGR_H_   1


#define STNDR_STACK_DEPTH_EMPTY     (-1)
#define STNDR_STACK_DEFAULT_SIZE    (256)


class stndrStackMgr
{
public:
    static IDE_RC initialize( stndrStack * aStack, SInt aStackSize = STNDR_STACK_DEFAULT_SIZE );
    static IDE_RC destroy( stndrStack * aStack );

    static void clear( stndrStack * aStack );

    static IDE_RC push( stndrStack      * aStack,
                        stndrStackSlot  * aItem );

    static IDE_RC push( stndrStack  * aStack,
                        scPageID      aNodePID,
                        ULong         aSmoNo,
                        SShort        aKeySeq );

    static stndrStackSlot pop( stndrStack * aStack );

    static void copy( stndrStack * aDstStack,
                      stndrStack * aSrcStack );

    static SInt getDepth( stndrStack * aStack );
    static SInt getStackSize( stndrStack * aStack );
};



#endif  /* _O_STNDR_STACK_MGR_H_ */
