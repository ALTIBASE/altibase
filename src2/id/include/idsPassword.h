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
 
#ifndef _O_IDSPASSWORD_H_
#define _O_IDSPASSWORD_H_

#include <idl.h>

#define IDS_MAX_PASSWORD_BUFFER_LEN  (4 + 128)   /* salt + sha512 digest size */
#define IDS_MAX_PASSWORD_LEN         (40)
#define IDS_SALT_LEN                 (4)

class idsPassword
{
private:
    /* requires aByte[4] */
    static void makeSalt( UChar * aByte );

public:
    /* requires aResult[132+1] */
    static void crypt( SChar * aResult, SChar * aPassword, UInt aPasswordLen, SChar * aSalt );
};

#endif /* _O_IDSPASSWORD_H_ */
