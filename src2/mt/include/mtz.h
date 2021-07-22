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
 

/***********************************************************************
 * $Id: mtz.h 55070 2012-08-16 23:16:52Z sparrow $
 **********************************************************************/

#ifndef _MTZ_DBTIMEZONE_H_
#define _MTZ_DBTIMEZONE_H_ 1

#include <idl.h>
#include <mtc.h>

typedef struct mtzTimezoneNamesTable
{
    const SChar   * mName;       //ex Asia/Seoul, KST ..
    const SChar   * mUTCOffset;  //ex +09:00
    SLong           mSecond;     //ex +9*60*60
} mtzTimezoneNamesTable;

class mtz
{
public:
    static IDE_RC   initialize( void );
    static IDE_RC   finalize( void );
    static IDE_RC   buildSecondOffset( void );

    static IDE_RC   getTimezoneSecondAndString( SChar * aTimezoneString,
                                                SLong * aOutTimezoneSecond,
                                                SChar * aOutTimezoneString );

    static UInt     getTimezoneElementCount( void );
    static const SChar  * getTimezoneName( UInt idx );
    static const SChar  * getTimezoneUTCOffset( UInt idx );
    static SLong    getTimezoneSecondWithIndex( UInt idx );

private:
    static IDE_RC   timezoneStr2Second( const SChar * aTimezoneStr,
                                        SLong * aOutTimezoneSecond );

};

#endif /* _MTZ_DBTIMEZONE_H_ */
