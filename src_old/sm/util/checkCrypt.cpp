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
 * $Id: checkCrypt.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#include <idl.h>
#if !defined(VC_WINCE)
#include <crypt.h>
#else /* VC_WINCE */
#include <winnt.h>
#include <Wincrypt.h>
#endif /* !VC_WINCE */
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idtBaseThread.h>
#include <iduSema.h>

char *userpw = "gaga1234";
char *salt   = "ab";

/*
   code from here 
   http://www.codeproject.com/cpp/crypt_routine.asp
*/

#define OUT_KEY_LEN       (11)
#define SALT_KEY_LEN      (2)
#define MK_VISIBLE_CHAR(a)  (gVisibleChar[ (UInt)(a) % 256])


SChar*  Crypt(SChar *inp, SChar* key, SChar *aResult)
{
    static SChar gVisibleChar[256] = 
        {
/*   0    1    2    3    4    5    6    7    8    9    10   11  12   13   14   15   */
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
    '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 
    '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '.', '1',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1',
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
    '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1',
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1',
    '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
        };

    //we will consider size of sbox 256 bytes
    //(extra byte are only to prevent any mishep just in case)
    SChar Sbox[257], Sbox2[257];
    UInt i, j, t, x;

    SChar temp , k;
    i = j = k = t =  x = 0;
    temp = 0;

    //always initialize the arrays with zero
    idlOS::memset(Sbox,  0, ID_SIZEOF(Sbox));
    idlOS::memset(Sbox2, 0, ID_SIZEOF(Sbox2));

    //initialize sbox i
    for(i = 0; i < 256; i++)
    {
        Sbox[i] = (SChar)i;
    }

    if (key == NULL) // generate salt key
    {
        SChar sSalt1;
        SChar sSalt2;
        
        idlOS::srand(idlOS::time(NULL));

        sSalt1 = MK_VISIBLE_CHAR(idlOS::rand());
        sSalt2 = MK_VISIBLE_CHAR(idlOS::rand());

        for(i = 0; i < 256 ; i += 2)
        {
            Sbox2[i]     = sSalt1;
            Sbox2[i + 1] = sSalt2;
        }
    }
    else
    {
        if (idlOS::strlen(key) < SALT_KEY_LEN)  // skip key & generate salt key
        {
            SChar sSalt1;
            SChar sSalt2;
            
            idlOS::srand(idlOS::time(NULL));
            
            sSalt1 = MK_VISIBLE_CHAR(idlOS::rand());
            sSalt2 = MK_VISIBLE_CHAR(idlOS::rand());
            
            for(i = 0; i < 256 ; i += 2)
            {
                Sbox2[i]     = sSalt1;
                Sbox2[i + 1] = sSalt2;
            }
        }
        else
        {
            
            //initialize the sbox2 with user key
            j = 0;
            for(i = 0; i < 256U ; i++)
            {
                if(j == SALT_KEY_LEN)
                {
                    j = 0;
                }
                Sbox2[i] = key[j++];
            }   
        } 
    }

    j = 0 ; //Initialize j
    //scramble sbox1 with sbox2
    for(i = 0; i < 256; i++)
    {
        j = (j + (UInt) Sbox[i] + (UInt) Sbox2[i]) % 256 ;
        temp =  Sbox[i];                    
        Sbox[i] = Sbox[j];
        Sbox[j] =  temp;
    }

    i = j = 0;
    for(x = 0; x < OUT_KEY_LEN; x++)
    {
        //increment i
        i = (i + 1U) % 256;
        //increment j
        j = (j + (UInt) Sbox[i]) % 256U;

        //Scramble SBox #1 further so encryption routine will
        //will repeat itself at great interval
        temp = Sbox[i];
        Sbox[i] = Sbox[j] ;
        Sbox[j] = temp;

        //Get ready to create pseudo random  byte for encryption key
        t = ((UInt) Sbox[i] + (UInt) Sbox[j]) %  256;

        //get the random byte
        k = Sbox[t];

        //xor with the data and done
        aResult[x + 2] = MK_VISIBLE_CHAR(inp[x] ^ k);
        // make visible character (from 33 to 126) by gamestar
    }    
    aResult[x + 2] = 0;
    aResult[0] = Sbox2[0];
    aResult[1] = Sbox2[1];

    return aResult;
}

#ifdef NOTDEF
//if we have to supply a key we have to use crypt as follows
SChar Data[] = "abcdefghijklmnopqrstuvwxyz"
SChar Key[] = "Strong key";

//Encrypt
Crypt(Data, _tcslen(Data), Key, _tcslen(Data));

//Decrypt
Crypt(Data, _tcslen(Data), Key, _tcslen(Data));

#endif


int main(int argc, char **argv)
{
    char *result;
    char *result2;
    struct crypt_data sCryptData1;
    struct crypt_data sCryptData2;

    idlOS::memset(&sCryptData1, 0, ID_SIZEOF(crypt_data));
    idlOS::memset(&sCryptData2, 0, ID_SIZEOF(crypt_data));

    result = crypt_r( userpw, salt, &sCryptData1 );

    idlOS::printf("encrypt : org = %s salt=%s crypt=%s\n", userpw, salt, result);

    result2 = crypt_r( userpw, result, &sCryptData2);

    idlOS::printf("encrypt : org = %s salt=%2s crypt=%s\n", userpw, result, result2);

    if (idlOS::strcmp(result, result2) == 0)
    {
        idlOS::printf("same ok!\n");
    }
    else
    {
        idlOS::printf("different!!! what's wrong?\n");
    }

    //==================================

    char dest[20];
    char dest2[20];

    idlOS::memset(dest, 0, 20);
    idlOS::memset(dest2, 0, 20);

    result = Crypt( userpw, NULL, dest);

    idlOS::printf("encrypt : org = %s salt=%s crypt=%s\n", userpw, salt, result);


    result2 = Crypt( userpw, result, dest2);

    idlOS::printf("encrypt : org = %s salt=%2s crypt=%s\n", userpw, result, result2);

    if (idlOS::strcmp(result, result2) == 0)
    {
        idlOS::printf("same ok!\n");
    }
    else
    {
        idlOS::printf("different!!! what's wrong?\n");
    }
}
