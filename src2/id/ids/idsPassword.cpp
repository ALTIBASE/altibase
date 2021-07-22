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
 
#include <idsPassword.h>
#include <idsSHA512.h>
#include <ide.h>

#define ASCII2HEX(c) (((c) < 10) ? ('0' + (c)) : ('A' + ((c) - 10)))

void idsPassword::makeSalt( UChar * aByte )
{
    UChar  sValue;
    UInt   i;
    
    idlOS::srand((UInt) idlOS::time(NULL));

    for ( i = 0; i < IDS_SALT_LEN; i++ )
    {
        sValue = idlOS::rand() % 16;
    
        aByte[i] = ASCII2HEX( sValue );
    }
}

void idsPassword::crypt( SChar * aResult, SChar * aPassword, UInt aPasswordLen, SChar * aSalt )
{
    UChar  sBuffer[IDS_SALT_LEN + IDS_MAX_PASSWORD_LEN];
    UInt   sPasswordLen;

    if ( aPasswordLen > IDS_MAX_PASSWORD_LEN )
    {
        sPasswordLen = IDS_MAX_PASSWORD_LEN;
    }
    else
    {
        sPasswordLen = aPasswordLen;
    }

    /* step 1
     * buffer에 salt와 password를 붙여 복사한다.
     * [salt/user password]
     */
    if ( aSalt != NULL )
    {
        idlOS::memcpy( sBuffer, aSalt, IDS_SALT_LEN );
    }
    else
    {
        makeSalt( sBuffer );
    }

    idlOS::memcpy( sBuffer + IDS_SALT_LEN, aPassword, sPasswordLen );

    /* step 2
     * result에 salt를 붙이고 sha512로 암호화해서 완성한다.
     *
     * crypt([salt][user password])
     *              |
     *              \/
     * [salt][crypted string]
     */
    idlOS::memcpy( aResult, sBuffer, IDS_SALT_LEN );

    idsSHA512::digest( (UChar*)aResult + IDS_SALT_LEN, sBuffer, sPasswordLen + IDS_SALT_LEN );

    /* clean buffer */
    idlOS::memset( sBuffer, 0x00, ID_SIZEOF(sBuffer) );

    /* terminate null */
    aResult[IDS_MAX_PASSWORD_BUFFER_LEN] = '\0';
}
