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
#include <ide.h>
#include <idp.h>
#include <idu.h>

#include <dumpcommon.h>

#if defined(ALTI_CFG_OS_LINUX)
# include <cxxabi.h>
#endif

#if defined(ALTI_CFG_OS_WINDOWS)
#define MAPNAME "altibase.exe.map"
#else
#define MAPNAME "altibase.map"
#endif

int compareFunction(const void* aF1, const void* aF2)
{
    functions* sF1 = (functions*)aF1;
    functions* sF2 = (functions*)aF2;

    if( sF1->mAddress < sF2->mAddress )
    {
        return -1;
    }
    else if( sF1->mAddress > sF2->mAddress )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

SInt main(SInt aArgc, SChar** aArgv)
{
    SInt    i;

    SChar*          sHomeDir;
    SChar*          sRet;
    SChar*          sTemp;
    size_t          sFunctionsSize = 512;
    SInt            sNoFunctions;

    ULong           sAddress;
    SChar*          sFunctionName;
    SInt            sStatus;
    SInt            sLineLength;

    functions*      sFunctions = NULL;
    FILE*           sFP;

    SChar           sPath[ID_MAX_FILE_NAME];
    SChar           sLine[MAXLEN];

    PDL_UNUSED_ARG(aArgc);
    PDL_UNUSED_ARG(aArgv);

    sHomeDir = idlOS::getenv("ALTIBASE_DEV");
    IDE_TEST_RAISE( sHomeDir == NULL, ENOHOME );

    idlOS::snprintf(sPath, ID_MAX_FILE_NAME,
                    "%s%saltibase_home%sbin%s%s",
                    sHomeDir,
                    IDL_FILE_SEPARATORS,
                    IDL_FILE_SEPARATORS,
                    IDL_FILE_SEPARATORS,
                    MAPNAME);

    sFunctions = (functions*)malloc(sizeof(functions) * sFunctionsSize);
    IDE_TEST_RAISE( sFunctions == NULL, ENOMEMORY );

    sFP = idlOS::fopen(sPath, "r");
    IDE_TEST_RAISE(sFP == NULL, EFILEOPEN);

    for( sNoFunctions = 0; feof(sFP) == 0; )
    {
        sRet = idlOS::fgets(sLine, MAXLEN, sFP);
        if( sRet == NULL )
        {
            break;
        }

        sLineLength = idlOS::strlen(sRet);

        if( sNoFunctions >= (ssize_t)sFunctionsSize )
        {
            sFunctionsSize *= 2;
            sFunctions = (functions*)realloc(sFunctions,
                                             sizeof(functions) * sFunctionsSize);

            IDE_TEST_RAISE( sFunctions == NULL, ENOMEMORY );
        }

        if( sRet == NULL )
        {
            continue;
        }
        else
        {
            /* fall through */
        }

        if( sRet[sLineLength - 1] == '\n' )
        {
            sRet[sLineLength - 1] = '\0';
        }

#if defined(ALTI_CFG_OS_LINUX)
        sAddress = idlOS::strtoul(sRet, &sTemp, 16);
        if(sAddress == 0)
        {
            continue;
        }
        else
        {
            /* fall through */
        }

        if ( idlOS::strncmp(sTemp, " <.", 3) == 0 )
        {
            /* CPU_POWERPC */
            sTemp += 3;
            sRet[sLineLength - 3] = '\0';
        }
        else if ( ( idlOS::strncmp(sTemp, " T ", 3) == 0 ) || 
                  ( idlOS::strncmp(sTemp, " t ", 3) == 0 ) )
        {
            sTemp += 3;
        }
        else
        {
            continue;
        }

        sFunctionName = abi::__cxa_demangle(sTemp, 0, 0, &sStatus);


        if ( sFunctionName == NULL )
        {
            sFunctionName = sTemp;
        }
        else
        {
            /* fall through */
        }

        sFunctions[sNoFunctions].mAddress = sAddress;
        idlOS::strcpy(sFunctions[sNoFunctions].mName, sFunctionName);
#elif defined(ALTI_CFG_OS_SOLARIS)
        {
            SChar*  sAddrStr;
            SChar*  sSymbolStr;
            SInt    sLen;
            sSymbolStr = idlOS::strstr(sRet, "FUNC");

            if(sSymbolStr == NULL)
            {
                continue;
            }
            else
            {
                /* fall through */
            }

            sAddrStr    = idlOS::strchr(sRet, '|');
            sSymbolStr  = idlOS::strrchr(sRet, '|');

            sAddress        = idlOS::strtoul(sAddrStr + 1, NULL, 16);
            sFunctionName   = sSymbolStr + 1;
        }

        sFunctions[sNoFunctions].mAddress = sAddress;
        idlOS::strcpy(sFunctions[sNoFunctions].mName, sFunctionName);
#elif defined(ALTI_CFG_OS_AIX)
        {
            SChar*  sAddrStr;
            SChar*  sSymbolStr;
            SChar*  sEndStr;
            SInt    sLen;

            if((sRet[0] != '.') ||
                    (idlOS::strncmp(sRet, "._", 2) == 0) ||
                    (idlOS::strncmp(sRet, ".bb", 3) == 0) ||
                    (idlOS::strncmp(sRet, ".bf", 3) == 0) ||
                    (idlOS::strncmp(sRet, ".bs", 3) == 0) ||
                    (idlOS::strncmp(sRet, ".eb", 3) == 0) ||
                    (idlOS::strncmp(sRet, ".ef", 3) == 0) ||
                    (idlOS::strncmp(sRet, ".es", 3) == 0) ||
                    (idlOS::strncmp(sRet, ".eb", 3) == 0))
            {
                continue;
            }
            else
            {
                /* fall through */
            }

            sAddrStr = idlOS::strstr(sRet, " T  0x");
            if(sAddrStr == NULL)
            {
                sAddrStr = idlOS::strstr(sRet, " t  0x");
            }
            else
            {
                /* fall through */
            }

            if(sAddrStr == NULL)
            {
                continue;
            }
            else
            {
                /* fall through */
            }

            *sAddrStr = '\0';
            sAddress  = idlOS::strtoul(sAddrStr + 6, &sEndStr, 16);

            sFunctionName = sRet;
        }

        sFunctions[sNoFunctions].mAddress = sAddress;
        idlOS::strcpy(sFunctions[sNoFunctions].mName, sFunctionName);
#elif defined(ALTI_CFG_OS_WINDOWS)
        {
            SChar   sTemp[MAXLEN];

            if(idlOS::strstr(sRet, " f ") == NULL)
            {
                continue;
            }

            sscanf(sRet, " %s %s %llX",
                   sTemp, sFunctions[sNoFunctions].mName,
                   &(sFunctions[sNoFunctions].mAddress));
        }
#endif

        sNoFunctions++;
    }

    qsort(sFunctions, sNoFunctions, sizeof(functions), compareFunction);

    idlOS::printf("SInt gNoFunctions = %d;\n\n", sNoFunctions);

    idlOS::printf("functions gFunctions[] = {\n");

    for( i = 0; i < sNoFunctions; i++ )
    {
        idlOS::printf("\t{ID_ULONG(0x%016llX), \"%s\"},\n",
                      sFunctions[i].mAddress,
                      sFunctions[i].mName);
    }

    idlOS::printf("\t{ID_ULONG_MAX, \"not found\"}\n");
    idlOS::printf("};\n");

    return 0;

    IDE_EXCEPTION(ENOHOME)
    {
        idlOS::fprintf(stderr, "No $ALTIBASE_DEV environment variable.\n");
    }
    IDE_EXCEPTION(ENOMEMORY)
    {
        idlOS::fprintf(stderr, "Not enough memory.\n");
    }
    IDE_EXCEPTION(EFILEOPEN)
    {
        idlOS::fprintf(stderr, "Could not open %s.\n", sPath);
    }

    IDE_EXCEPTION_END;
    if( sFunctions != NULL )
    {
        free(sFunctions);
    }

    return -1;
}

