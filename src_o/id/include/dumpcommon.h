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
 
#define MAXLEN (2048)

typedef struct functions
{
    ULong   mAddress;
    SChar   mName[MAXLEN];
} functions;

/* BUG-47888 */
idBool getSimpleCRC(UInt * aOut)
{
    UInt    sTotal = 0;
    FILE  * sF = NULL;
    SChar   sPath[ID_MAX_FILE_NAME];
    UInt    sSize;
    UChar * sBuf;
    UInt    i;

    idlOS::snprintf(sPath, ID_SIZEOF(sPath), "%s%sbin%s%s",
                    idlOS::getenv(IDP_HOME_ENV),
                    IDL_FILE_SEPARATORS,
                    IDL_FILE_SEPARATORS,
                    "altibase");
    sF = idlOS::fopen(sPath, "r");
    if( sF == NULL )
    {
        idlOS::printf("Error : %s cannot be opened.\n",sPath);
        return ID_FALSE;
    }

    idlOS::fseek(sF, 0, SEEK_END); 
    sSize = idlOS::ftell(sF);      

    sBuf = (UChar*)idlOS::malloc(sSize);   
    if( sBuf == NULL )
    {
        idlOS::printf("Error : malloc failed.\n",sPath);
        return ID_FALSE;
    }

    idlOS::fseek(sF, 0, SEEK_SET);          
    (void)idlOS::fread(sBuf, sSize, 1, sF);    

    idlOS::fclose(sF);   

    for(i = 1; i < sSize ; i++)
    {
        sTotal += sBuf[i];
    }

    idlOS::free(sBuf);

    *aOut = sTotal;

    return ID_TRUE;
}
