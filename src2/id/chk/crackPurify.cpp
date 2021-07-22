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

void patchIt(UChar *);
idBool findCrackRegion(UChar *aMem);

UChar crackInfo[] =
{
    0x68, 0xd1, 0x86, 0x2b, 0x34, 0xe8, 0x60, 0x2b, 0x60, 0x2b,
    0x60, 0x2b, 0x60, 0xe8, 0x34, 0xd3, 0x8f, 0x2b, 0x68, 0x20,
    0x0b, 0x2b, 0x48, 0x23, 0xe8, 0x37, 0x2b, 0x23, 0x37, 0x48,
    0x08, 0xe8, 0x48, 0xe8, 0x68, 0x2b, 0x23, 0x37, 0xe8, 0x48,
    0xe8, 0xe8, 0x34
};



SInt main(SInt argc, SChar **argv)
{
    struct stat   sStat;
    PDL_HANDLE sFp;
    UChar *sMem;
    UChar *sCur;
    ULong i, j;
    idBool   sFound = ID_FALSE;
    
    
    if (argc < 2)
    {
        idlOS::fprintf(stderr, "crackPurify filname\n");
        idlOS::exit(0);
    }


    sFp = idlOS::open(argv[1], O_RDWR);
    if (sFp == IDL_INVALID_HANDLE)
    {
        printf("error in open %d\n", errno);
        idlOS::exit(-1);
        
    }
    
    assert(idlOS::fstat(sFp, &sStat) == 0);

    sMem = (UChar *)idlOS::mmap(0,
                                sStat.st_size,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED ,
                                sFp, 0);
    printf("Mmap Ok.\n");

    printf("Find Region Ok.\n");

    for (i = 0; i < sStat.st_size; i += 4)
    {
        if (findCrackRegion(sMem + i) == ID_TRUE)
        {
            patchIt(sMem + i);
            idlOS::close(sFp);
            idlOS::printf("Success of Patch.\n");
            idlOS::exit(0);
            
        }
    }
    idlOS::printf("ERROR : Can't Find Patch Region.\n");
    idlOS::exit(-1);
}

idBool findCrackRegion(UChar *aMem)
{
    ULong j;
    
    for (j = 0; j < sizeof(crackInfo); j ++)
    {
        if ( *(aMem + j * 4) != crackInfo[j])
        {
            return ID_FALSE;
        }
    }

    return ID_TRUE;
}

void patchIt(UChar *aMem)
{
    ULong j;
    
    for (j = 0; j < sizeof(crackInfo); j ++)
    {
        *(aMem + j * 4 + 0) = 0x08;
        *(aMem + j * 4 + 1) = 0x00;
        *(aMem + j * 4 + 2) = 0x02;
        *(aMem + j * 4 + 3) = 0x40;
    }
}

