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
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <altipkg.h>

void doVerification()
{
    /* ------------------------------------------------
     *  - report open
     *  - existance check
     *  - filesize check 
     * ----------------------------------------------*/
    
    FILE *sReportFP;
    SChar sBuffer[4096];
    SChar sSize[4096];
    SChar sPkgBin[256];
#if defined(VC_WIN32)    
    idlOS::sprintf(sPkgBin, "bin%saltipkg.exe", IDL_FILE_SEPARATORS);
#else
    idlOS::sprintf(sPkgBin, "bin%saltipkg", IDL_FILE_SEPARATORS);
#endif    
    sReportFP = idlOS::fopen(gPkgReportFile, "r");

    if (sReportFP == NULL)
    {
        idlOS::fprintf(stderr, "There is no report file(%s)\n", gPkgReportFile);
        idlOS::fflush(stderr);
        idlOS::exit(-1);
    }

    while(!feof(sReportFP))
    {
        ULong sFileSize;
        
        idlOS::memset(sBuffer, 0, ID_SIZEOF(sBuffer));
        idlOS::memset(sSize, 0, ID_SIZEOF(sSize));
        
        if (idlOS::fgets(sBuffer, ID_SIZEOF(sBuffer), sReportFP) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }
        eraseWhiteSpace(sBuffer);

        if (sBuffer[0] == '#' || sBuffer[0] == 0)
        {
            continue;
        }
        
        /* ------------------------------------------------
         * 1. 화일명일 경우 
         * ----------------------------------------------*/
        idlOS::fprintf(stdout, "verificatoin => %s ", sBuffer);
        
        /* ------------------------------------------------
         *  2. 화일크기 
         * ----------------------------------------------*/
        if (idlOS::fgets(sSize, ID_SIZEOF(sSize), sReportFP) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }
        eraseWhiteSpace(sBuffer);

        // altibase package file의 경우 무시함.
        if (idlOS::strcmp(sPkgBin, sBuffer) == 0)
        {
            idlOS::fprintf(stdout, " ..ok \n");
            continue; // size check skip;
        }
        /* ------------------------------------------------
         *  3. Verification 수행 
         * ----------------------------------------------*/

        // - Existence test 
        if (idlOS::access(sBuffer, R_OK | W_OK | F_OK) == 0)
        {
            idlOS::fprintf(stdout, " ..ok \n");
        }
        else
        {
            idlOS::fprintf(stderr, " --> Error ! (missing? or no permission) \n\n", sBuffer);
            idlOS::fflush(stderr);
            idlOS::exit(-1);
        }

        // - filesize test

        sFileSize = idlOS::strtol(sSize, NULL, 10);

        if (sFileSize != 0) // not a directory
        {
            debugMsg("Size Check (%s) (%"ID_UINT64_FMT")(%"ID_UINT64_FMT")\n",
                     sBuffer, sFileSize, getFileSize(sBuffer));
            
            if (sFileSize != getFileSize(sBuffer))
            {
                idlOS::fprintf(stdout, " --> File Size Error. (org -> %"ID_UINT64_FMT
                               " : now -> %"ID_UINT64_FMT");  \n\n",
                               sFileSize,
                               getFileSize(sBuffer),
                               sBuffer);
            }
        }
    }
}
