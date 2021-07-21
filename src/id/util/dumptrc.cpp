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

#include "alticrc.txt"

#define MAXFILES (IDE_LOG_MAX_MODULE * 2)

#define MAXLINES        (128)
#define MAXLINELENGTH   (512)
#define DEFAULTLINES    (10)

#define   MAXHOURDIFF (+24)
#define   MINHOURDIFF (-24)

/* BUG-47888 */
#define DUMPTRC_INVALID_MESSAGE "\nWarning!!  --------------------------------------------\n"\
                                "Altibase version doesn't match this dumptrc.\n"        \
                                "Output of dumptrc might be wrong!!\n"                       \
                                "--------------------------------------------------------\n" 

typedef struct 
{
    SInt    mIndex;
    SLong   mOffset;
    SChar   mLine[MAXLINELENGTH];
} bracketPosition;

SChar gNoFile = 0;
SInt  gIndex[MAXFILES] = {0, };
SChar gPath[MAXFILES][ID_MAX_FILE_NAME];

SInt  gLines = DEFAULTLINES;
SInt  gLineIndex = 0;
SChar gLine[MAXFILES][MAXLINELENGTH];
SInt  gHeap[MAXFILES];
SInt  gRead;
SInt  gPrinted = 0;
SInt  gHourDiff = 0;

idBool  gPrintCrash     = ID_FALSE;
idBool  gCrashDetected  = ID_FALSE;
idBool  gPrintSymbol    = ID_FALSE;
idBool  gPrintTime      = ID_FALSE;
time_t  gTime;
idBool  gTimePrinted    = ID_FALSE;

FILE*   gFP[MAXFILES] = {NULL, };
SChar*  gRet[MAXFILES];

PDL_Time_Value  gDates[MAXFILES];
bracketPosition gBracketPosition[MAXLINES];

void printversion(void)
{
    idlOS::printf("Altibase trace log dump utility.\n");
    idlOS::printf("version %s %s %s\n",
            iduVersionString,
            iduGetSystemInfoString(),
            iduGetProductionTimeString());
}

void printusage(void)
{
    idlOS::printf("Usage : dumptrc [options]\n");
    idlOS::printf(" Option list\n");
    idlOS::printf(" -a : prints all file.\n");
    idlOS::printf(" -i filename : prints only \"filename\" file.\n");
    idlOS::printf("\t -i option can be used repeatedly"
                  " to specify multiple trc logs.\n");
    idlOS::printf(" -e filename : prints except \"filename\" file.\n");
    idlOS::printf("\t -e option can be used repeatedly"
                  " to specify multiple trc logs.\n");
    idlOS::printf(" -c : traces call stack and"
                  " prints trc log before the callstack\n");
    idlOS::printf("\t When there are multiple callstacks,"
                  " prints all stack trace\n");
    idlOS::printf("\t The addresses will be translated "
                  " into symbols.\n");
    idlOS::printf("\t -c can be compined with -a, -e, or -i.\n");
    idlOS::printf("\t If not compined with -a, -e, or -i,"
                  " prints only call stack trace.\n");
    idlOS::printf(" -s : print only stack address symbol.\n"
                  "\t -s should be used with -c\n");
    idlOS::printf(" -t DATETIME : prints the logs of DATETIME.\n");
    idlOS::printf("\t The format of DATETIME is YYYYMMDDHHmmSS.\n"
                  "\t e.g.) 20150915103000\n");
    idlOS::printf(" -f : follows trc logs and prints all.\n");
    idlOS::printf("\t -f can be comined with -a, -e, or -i.\n"
                  "\t Retries are not possible.(like \"tail -F\")\n");
    idlOS::printf(" -n [1~%d] : speficies the number of lines to be printed.\n",
                  MAXLINES - 1);
    idlOS::printf("\t If not specified, 10 lines will be printed by default.\n");
    idlOS::printf(" -H [%d ~ %d] : This option sets the time difference between time zones. If this option is not set, it is assumed that the timezone of the current system is the same as the timezone of the trc log.",
                  MINHOURDIFF,MAXHOURDIFF );
    idlOS::printf("\t If not specified,the timezone of altibase server is the same as current system.\n");
    idlOS::printf(" -p path : speficies the path of trc logs.\n");
    idlOS::printf("\t If not specified, $ALTIBASE_HOME/trc by default.\n");
    idlOS::printf(" -h : prints this help screen.\n");
    idlOS::printf("\t Without any option, dumptrc will"
                  " print this help screen and exit.\n");
    idlOS::printf(" -x : show callstacks even if altibase doesn't match dumptrc\n");
    idlOS::printf(" -v : prints version info.\n");
    idlOS::printf(" Filenames are as follows, not case-sensitive;\n");
    idlOS::printf("\t ERROR       : altibase_error.log\n");
    idlOS::printf("\t SERVER      : altibase_boot.log\n");
    idlOS::printf("\t SM          : altibase_sm.log\n");
    idlOS::printf("\t RP          : altibase_rp.log\n");
    idlOS::printf("\t QP          : altibase_qp.log\n");
    idlOS::printf("\t JOB         : altibase_job.log\n");
    idlOS::printf("\t SD          : altibase_sd.log\n");    /* BUG-46138 */
    idlOS::printf("\t DK          : altibase_dk.log\n");
    idlOS::printf("\t XA          : altibase_xa.log\n");
    idlOS::printf("\t MM          : altibase_mm.log\n");
    idlOS::printf("\t RP_CONFLICT : altibase_rp_conflict.log\n");
    idlOS::printf("\t DUMP        : altibase_dump.log\n");
    idlOS::printf("\t SNMP        : altibase_snmp.log\n");
    idlOS::printf("\t CM          : altibase_cm.log\n");
    idlOS::printf("\t MISC        : altibase_misc.log\n");
}

void makeTimeFromHeader(SChar* aHeader, time_t* aTime, SLong* aSerial)
{
    struct tm   sTime;
    SChar*      sTerm;
    SChar       sLine[MAXLINELENGTH];

    if( idlOS::strncmp(aHeader, "[0x", 3) == 0 )
    {
        *aTime = idlOS::strtoul(aHeader + 3, &sTerm, 16);
        *aTime += (gHourDiff * 60 * 60);

        *aSerial = idlOS::strtoul(sTerm + 1, &sTerm, 16);

        idlOS::memcpy(&sTime, localtime(aTime), sizeof(struct tm));

        idlOS::snprintf(
            sLine, MAXLINELENGTH,
            "[%4"ID_UINT32_FMT
            "/%02"ID_UINT32_FMT
            "/%02"ID_UINT32_FMT
            " %02"ID_UINT32_FMT
            ":%02"ID_UINT32_FMT
            ":%02"ID_UINT32_FMT
            " %llX%s",
            sTime.tm_year + 1900,
            sTime.tm_mon  + 1,
            sTime.tm_mday,
            sTime.tm_hour,
            sTime.tm_min,
            sTime.tm_sec,
            *aSerial,
            sTerm);

        idlOS::strcpy(aHeader, sLine);

        gCrashDetected = ID_TRUE;
    }
    else
    {
        sTime.tm_year   = idlOS::strtoul(aHeader + 1, &sTerm, 10) - 1900;
        sTime.tm_mon    = idlOS::strtoul(sTerm + 1, &sTerm, 10) - 1;
        sTime.tm_mday   = idlOS::strtoul(sTerm + 1, &sTerm, 10);

        sTime.tm_hour   = idlOS::strtoul(sTerm + 1, &sTerm, 10);
        sTime.tm_min    = idlOS::strtoul(sTerm + 1, &sTerm, 10);
        sTime.tm_sec    = idlOS::strtoul(sTerm + 1, &sTerm, 10);

        *aTime = idlOS::mktime(&sTime);
        *aSerial = idlOS::strtoul(sTerm + 1, NULL, 16);
    }
}

SInt compareHeader(SChar* aLhs, SChar* aRhs, time_t* aTime)
{
    time_t  sLTime;
    SLong   sLSerial;
    time_t  sRTime;
    SLong   sRSerial;

    if( (aLhs == NULL) && (aRhs == NULL) )
    {
        return 0;
    }
    else if( aLhs == NULL )
    {
        makeTimeFromHeader(aRhs, aTime, &sRSerial);
        return 1;
    }
    else if( aRhs == NULL )
    {
        makeTimeFromHeader(aLhs, aTime, &sLSerial);
        return -1;
    }
    else
    {
        makeTimeFromHeader(aLhs, &sLTime, &sLSerial);
        makeTimeFromHeader(aRhs, &sRTime, &sRSerial);

        if( sLTime > sRTime )
        {
            *aTime = sRTime;
            return 1;
        }
        else if( sLTime < sRTime )
        {
            *aTime = sLTime;
            return -1;
        }
        else
        {
            if( sLSerial > sRSerial )
            {
                *aTime = sRTime;
                return 1;
            }
            else
            {
                *aTime = sLTime;
                return -1;
            }
        }
    }
}

SInt findOldestHeader(SChar** aRet, time_t* aTime)
{
    SInt    i;
    SInt    sMin = 0;
    SLong   sSerial;

    for( i = 0; i < gNoFile; i++ )
    {
        if( aRet[i] != NULL )
        {
            break;
        }
        else
        {
            /* continue */
        }
    }

    if( i == gNoFile )
    {
        return -1;
    }

    if( gNoFile == 1 )
    {
        makeTimeFromHeader(aRet[0], aTime, &sSerial);
    }
    else
    {
        for( i = 1; i < gNoFile; i++ )
        {
            if( compareHeader(aRet[sMin], aRet[i], aTime) > 0 )
            {
                sMin = i;
            }
        }
    }

    return sMin;
}

idBool checkHeaderValidity(SChar** aLine)
{
    IDE_ASSERT(*aLine != NULL);

    /*BUG-45982*/
    if( idlOS::strncmp( *gLine, IDU_DUMPSTACKS_PREFIX, 
                        IDU_DUMPSTACKS_PREFIX_LEN ) == 0 )
    {
        idlOS::printf(IDU_DUMPSTACKS_PREFIX);
        *aLine += IDU_DUMPSTACKS_PREFIX_LEN;
    }

    if( (*aLine)[0] != '[' ) return ID_FALSE;
    if( isdigit((SInt)((UChar)(*aLine)[1])) == 0 ) return ID_FALSE;

    return ID_TRUE;
}

#include "map.ic"

void printMap( void )
{
    SInt    i = 0;

    idlOS::printf("Total Function Count : %d\n", gNoFunctions );
    idlOS::printf("Print Function Start\n" );

    for ( i = 0; i < gNoFunctions; i++ )
    {
        idlOS::printf( "0x%016x %s\n", gFunctions[i].mAddress, gFunctions[i].mName );
    }

    idlOS::printf("Print Function End\n" );
}

SInt findFunction(ULong aAddress)
{
    SInt sHeader;
    SInt sTailer;
    SInt sMid;

    sHeader = 0;
    sTailer = gNoFunctions;

    while(1)
    {
        sMid = (sHeader + sTailer) / 2;

        if( (gFunctions[sMid    ].mAddress <= aAddress) &&
            (gFunctions[sMid + 1].mAddress >= aAddress) )
        {
            /* Gotcha! */
            break;
        }
        else
        {
            if( gFunctions[sMid].mAddress > aAddress )
            {
                sTailer = sMid;
            }
            else
            {
                sHeader = sMid;
            }
        }

        if( sMid == 0 )
        {
            sMid = -1;
            break;
        }
    }

    if( sMid == (gNoFunctions - 1) )
    {
        sMid = -1;
    }
    return sMid;
}

void printLines(void)
{
    SInt        i;
    SInt        sStart;
    SInt        sIndex;
    ULong       sAddress;
    SInt        sLineIndex;
    SInt        sFuncIndex;
    SChar*      gRet;
    SChar*      sTemp;

    if( gRead < gLines )
    {
        sStart = 0;
    }
    else
    {
        sStart = (gLineIndex + MAXLINES - gLines) % MAXLINES;
    }

    for( i = sStart; (i % MAXLINES) != gLineIndex; i++ )
    {
        sLineIndex = i % MAXLINES;
        sIndex = gBracketPosition[sLineIndex].mIndex;

        idlOS::fputs(gBracketPosition[sLineIndex].mLine, stdout);
        idlOS::fseek(gFP[sIndex], gBracketPosition[sLineIndex].mOffset, SEEK_SET);
        gPrinted++;
        
        while(1)
        {
            gRet = idlOS::fgets(gLine[sIndex], MAXLINELENGTH, gFP[sIndex]);
            if( gRet == NULL ||
                checkHeaderValidity(&gRet) == ID_TRUE )
            {
                break;
            }
            else
            {
                if( (gPrintCrash == ID_TRUE) &&
                    (gPrintSymbol == ID_FALSE) &&
                    (idlOS::strncmp(gRet, "Caller", 6) == 0) )
                {
                    gRet[idlOS::strlen(gRet) - 1] = '\0';
                    sTemp = idlOS::strstr(gRet, " ") + 1;
                
                    /*
                     * BUG-48433 
                     * when not found, strstr is supposed to return NULL.
                     * but,it returns 1 in 1.98 platform
                     * I don't think it's worth digging into this problem :(
                     * anyway, it works.
                     */
                    if( sTemp < (SChar*)0xff ) //error
                    {
                        idlOS::printf("[dumptrc:wrong format.omit this line.]\n");
                        continue;
                    }
#if defined(ALTI_CFG_OS_WINDOWS)
                    sscanf(sTemp, "%llX", &sAddress);
#else
                    sAddress = idlOS::strtoul(sTemp, NULL, 16);
#endif
                    sFuncIndex = findFunction(sAddress);

                    if( sFuncIndex == -1 )
                    {
                        idlOS::printf("%s\t=> not found\n", gRet);
                    }
                    else
                    {
                        idlOS::printf("%s\t=> %s\n",
                                      gRet,
                                      gFunctions[sFuncIndex].mName);
                    }
                }
                else
                {
                    idlOS::fputs(gRet, stdout);
                }
            }
        }
    }
}

void traceFile(void)
{
    SInt    i;
    SInt    j;
    SInt    sIndex;
    SInt    sCrashCount = 0;
    idBool  sTimePrinted = ID_FALSE;
    time_t  sTime;

    gRead   = 0;

    for( i = 0, j = 0; i < gNoFile; i++ )
    {
        gFP[j] = idlOS::fopen(gPath[i], "r");
        if( gFP[j] == NULL )
        {
            idlOS::printf("Warning : %s cannot be opened. Excluding file from dump.\n",
                          gPath[i]);
        }
        else
        {
            do
            {
                gRet[j] = idlOS::fgets(gLine[j], MAXLINELENGTH, gFP[j]);
            } while(gRet[j] != NULL &&
                    checkHeaderValidity(&gRet[j]) == ID_FALSE);

            j++;
        }
    }

    gNoFile = j;
    gLineIndex = 0;

    while(1)
    {
        sIndex = findOldestHeader(gRet, &sTime);
        
        if( sIndex == -1 )
        {
            break;
        }
        else
        {
            /* fall through */
            gRead++;
        }

        gBracketPosition[gLineIndex].mIndex = sIndex;
        gBracketPosition[gLineIndex].mOffset = ftell(gFP[sIndex]);
        idlOS::strcpy(gBracketPosition[gLineIndex].mLine, gRet[sIndex]);
        gLineIndex = (gLineIndex + 1) % MAXLINES;

        if( (gPrintCrash == ID_TRUE) &&
            (sIndex == 0)&&
            (gCrashDetected == ID_TRUE) )
        {
            idlOS::printf("=================================================\n");
            idlOS::printf("= Callstack Information %d\n", sCrashCount);
            idlOS::printf("=================================================\n\n");
            printLines();
            idlOS::printf("\n\n\n");
            sCrashCount++;

            gCrashDetected = ID_FALSE;
        }
        else if( (gPrintTime == ID_TRUE) && (sTime == gTime) )
        {
            if( sTimePrinted == ID_FALSE )
            {
                printLines();
                sTimePrinted = ID_TRUE;
            }
            else
            {
                /* fall through */
                idlOS::fputs(gRet[sIndex], stdout);
                gPrinted++;
            }

            gRet[sIndex] = idlOS::fgets(gLine[sIndex],
                                        MAXLINELENGTH,
                                        gFP[sIndex]);

            while(gRet[sIndex] != NULL &&
                  checkHeaderValidity(&gRet[sIndex]) == ID_FALSE)
            {
                idlOS::fputs(gLine[sIndex], stdout);
                gRet[sIndex] = idlOS::fgets(gLine[sIndex],
                                            MAXLINELENGTH,
                                            gFP[sIndex]);
            }
        }
        else
        {
            gRet[sIndex] = idlOS::fgets(gLine[sIndex],
                                        MAXLINELENGTH,
                                        gFP[sIndex]);

            while(gRet[sIndex] != NULL &&
                  checkHeaderValidity(&gRet[sIndex]) == ID_FALSE)
            {
                gRet[sIndex] = idlOS::fgets(gLine[sIndex],
                                            MAXLINELENGTH,
                                            gFP[sIndex]);
            }
        }
    }

    if( (gPrintCrash == ID_FALSE) && (gPrintTime == ID_FALSE) )
    {
        printLines();
    }
}

void followFile(void)
{
    SInt    i;
    SInt    sIndex;
    SInt    sLength;
    time_t  sTime;

    while(1)
    {
        sIndex = findOldestHeader(gRet, &sTime);

        if( sIndex == -1 )
        {
            for( i = 0; i < gNoFile; i++ )
            {
                do
                {
                    gRet[i] = idlOS::fgets(gLine[i], MAXLINELENGTH, gFP[i]);
                    if( gRet[i] != NULL )
                    {
                        sLength = idlOS::strnlen(gRet[i], MAXLINELENGTH);
                        if( gRet[i][sLength - 1] != '\n' )
                        {
                            idlOS::fseek(gFP[i], -sLength, SEEK_CUR);
                            gRet[i] = NULL;
                        }
                        else
                        {
                            /* fall through */
                        }
                    }
                    else
                    {
                        /* fall through */
                    }
                } while(gRet[i] != NULL && gRet[i][0] != '[');
            }

            idlOS::sleep(1);
            continue;
        }
        else
        {
            /* fall through */
        }

        idlOS::fputs(gLine[sIndex], stdout);
        while(1)
        {
            gRet[sIndex] = idlOS::fgets(gLine[sIndex], MAXLINELENGTH, gFP[sIndex]);
            if( (gRet[sIndex] == NULL) || (gRet[sIndex][0] == '[') )
            {
                break;
            }
            else
            {
                idlOS::fputs(gLine[sIndex], stdout);
                idlOS::fflush(stdout);
            }
        }
    }
}

SInt getNameIndex(SChar* aName)
{
    SInt i;
    for( i = 0; i < IDE_LOG_MAX_MODULE; i++ )
    {
        if( idlOS::strcasecmp(aName, ideLog::mMsgModuleName[i]) == 0 )
        {
            return i;
        }
    }

    return -1;
}

/* BUG-47888 */
idBool validDump()
{
    UInt    sTotal = 0;
    UChar   sRet = ID_FALSE;

    sRet = getSimpleCRC(&sTotal);
    
    if( (sRet == ID_TRUE) && (sTotal == VALID_CRC) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

SInt main(SInt aArgc, SChar** aArgv)
{
    SInt    i;
    SInt    j;
    SInt    sIndex;

    UInt    sOptCount        = 0;
    SInt    sOptVal;
    
    idBool  sPrintUsage     = ID_FALSE;
    idBool  sPrintAll       = ID_FALSE;
    idBool  sPrintOnly      = ID_FALSE;
    idBool  sPrintExcept    = ID_FALSE;
    idBool  sPrintFollow    = ID_FALSE;
    idBool  sPathSet        = ID_FALSE;
    idBool  sLineSet        = ID_FALSE;
    idBool  sVersion        = ID_FALSE;
    idBool  sIsPrintMap     = ID_FALSE;
    idBool  sCallstackForce = ID_FALSE;

    struct tm   sTime;

    SChar*  sDir;
    SChar*  sFileName;
    SChar   sPath[ID_MAX_FILE_NAME];
    SChar   sTemp[512];

    idBool  sInclude[IDE_LOG_MAX_MODULE];
    idBool  sExclude[IDE_LOG_MAX_MODULE];
    
    idBool  sCorrectCRC = validDump();

    for( i = 0; i < IDE_LOG_MAX_MODULE; i++ )
    {
        sInclude[i] = ID_FALSE;
        sExclude[i] = ID_FALSE;
    }

    while((sOptVal = idlOS::getopt(aArgc, aArgv, "hai:e:csft:p:n:vH:mx")) != -1)
    {
        sOptCount++;

        switch(sOptVal)
        {
        case 'h':
            sPrintUsage = ID_TRUE;
            break;
        case 'a':
            sPrintAll = ID_TRUE;
            break;
        case 'i':
            sIndex = getNameIndex(optarg);
            if( sIndex == -1 )
            {
                idlOS::printf("Invalid trc file name : %s.\n", optarg);
                printusage();
                idlOS::exit(-1);
            }
            sInclude[sIndex] = ID_TRUE;
            sPrintOnly = ID_TRUE;
            break;
        case 'e':
            sIndex = getNameIndex(optarg);
            if( sIndex == -1 )
            {
                idlOS::printf("Invalid trc file name : %s.\n", optarg);
                printusage();
                idlOS::exit(-1);
            }
            sExclude[sIndex] = ID_TRUE;
            sPrintExcept = ID_TRUE;
            break;
        case 'c':
            gPrintCrash = ID_TRUE;
            break;
        case 's':
            gPrintSymbol = ID_TRUE;
            break;
        case 'f':
            sPrintFollow = ID_TRUE;
            break;
        case 't':
            gPrintTime = ID_TRUE;

            idlOS::strncpy(sTemp, optarg, 4);
            sTemp[4] = '\0';
            sTime.tm_year = (int)idlOS::strtol(sTemp, NULL, 10) - 1900;

            idlOS::strncpy(sTemp, optarg + 4, 2);
            sTemp[2] = '\0';
            sTime.tm_mon = (int)idlOS::strtol(sTemp, NULL, 10) - 1;

            idlOS::strncpy(sTemp, optarg + 6, 2);
            sTemp[2] = '\0';
            sTime.tm_mday = (int)idlOS::strtol(sTemp, NULL, 10);

            idlOS::strncpy(sTemp, optarg + 8, 2);
            sTemp[2] = '\0';
            sTime.tm_hour = (int)idlOS::strtol(sTemp, NULL, 10);

            idlOS::strncpy(sTemp, optarg + 10, 2);
            sTemp[2] = '\0';
            sTime.tm_min = (int)idlOS::strtol(sTemp, NULL, 10);

            idlOS::strncpy(sTemp, optarg + 12, 2);
            sTemp[2] = '\0';
            sTime.tm_sec = (int)idlOS::strtol(sTemp, NULL, 10);

            gTime = idlOS::mktime(&sTime);

            if( gTime == -1 )
            {
                idlOS::printf("Time format is invalid\n");
                printusage();
                idlOS::exit(-1);
            }

            break;
        case 'p':
            idlOS::strcpy(sPath, optarg);
            idlOS::printf("Path : %s\n", optarg);
            sPathSet = ID_TRUE;
            break;

        case 'n':
            gLines = idlOS::strtol(optarg, NULL, 10);
            if( (gLines == 0) || (gLines > MAXLINES - 1) )
            {
                idlOS::printf("-n option needs 1~%d lines\n", MAXLINES - 1);
                printusage();
                idlOS::exit(-1);
            }
            sLineSet = ID_TRUE;
            break;
        /*BUG-45423 The time of error trc log differs from the target platform.*/
        case 'H':
            gHourDiff = idlOS::strtol(optarg, NULL, 10);
            if( (gHourDiff > MAXHOURDIFF) || (gHourDiff < MINHOURDIFF))
            {
                idlOS::printf("The value of the -H option is available from -24 to 24.\n",
                               MINHOURDIFF,MAXHOURDIFF );
                printusage();
                idlOS::exit(-1);
            }
            break;
        case 'v':
            sVersion = ID_TRUE;
            break;
        case 'm':
            sIsPrintMap = ID_TRUE;
            break;
        case 'x':
            sCallstackForce = ID_TRUE;
            break;
        default:
            break;
        }
    }

    /* BUG-47888 */
    if( (sCorrectCRC == ID_FALSE) && (sCallstackForce == ID_FALSE) )
    {
        idlOS::printf( DUMPTRC_INVALID_MESSAGE );
        idlOS::exit(0);
    }

    if( (sOptCount == 0) || (sPrintUsage == ID_TRUE) )
    {
        printusage();
        idlOS::exit(0);
    }

    if( sVersion == ID_TRUE )
    {
        printversion();
        idlOS::exit(0);
    }

    if( gPrintCrash == ID_TRUE )
    {
        if( ((sPrintAll == ID_TRUE) || (sPrintExcept == ID_TRUE)) &&
            (sExclude[0] == ID_TRUE) )
        {
            idlOS::printf("Error : altibase_error.log should not be excluded "
                          "to print call stack.\n");
            printusage();
            idlOS::exit(0);
        }
        if( (sPrintOnly == ID_TRUE) && (sInclude[0] == ID_FALSE) )
        {
            idlOS::printf("Error : altibase_error.log should not be excluded "
                          "to print call stack.\n");
            printusage();
            idlOS::exit(0);
        }

        if( (sLineSet == ID_FALSE) && (sPrintAll == ID_FALSE) &&
            (sPrintOnly == ID_FALSE) && (sPrintExcept == ID_FALSE) )
        {
            gLines = 1;
            sPrintOnly = ID_TRUE;
            sInclude[0] = ID_TRUE;
        }
    }

    if( (gPrintSymbol == ID_TRUE) && (gPrintCrash == ID_FALSE) )
    {
        idlOS::printf("Error : -s should be used with -c.\n");
        printusage();
        idlOS::exit(0);
    }

    if( ((sPrintAll == ID_TRUE) && (sPrintOnly == ID_TRUE)) ||
        ((sPrintExcept == ID_TRUE) && (sPrintOnly == ID_TRUE)) )
    {
        idlOS::printf("Error : The options cannot be combined.\n");
        printusage();
        idlOS::exit(0);
    }

    if ( sIsPrintMap == ID_TRUE )
    {
        printMap();
        idlOS::exit(0);
    }
    else
    {
        /* do nothing */
    }

    if( gPrintTime == ID_TRUE )
    {
        if( sPrintFollow == ID_TRUE )
        {
            idlOS::printf("Error : The options cannot be combined.\n");
            printusage();
            idlOS::exit(0);
        }

        if( (sPrintOnly == ID_FALSE) && (sPrintExcept == ID_FALSE) )
        {
            sPrintOnly = ID_TRUE;

            for( i = 0; i < IDE_LOG_MAX_MODULE; i++ )
            {
                sInclude[i] = ID_TRUE;
            }
        }

        if( sLineSet == ID_FALSE )
        {
            gLines = 1;
        }
    }

    IDE_TEST_RAISE(idp::initialize(NULL, NULL) != IDE_SUCCESS, property_error);
    IDE_TEST_RAISE(iduProperty::load() != IDE_SUCCESS, property_error);

    if( sPathSet == ID_FALSE )
    {
        IDE_TEST_RAISE(idp::readPtr( "SERVER_MSGLOG_DIR",
                                     (void **)&sDir)
                       != IDE_SUCCESS, property_error );
        idlOS::strcpy(sPath, sDir);
    }

    for( i = 0, j = 0; i < IDE_LOG_MAX_MODULE; i++ )
    {
        if( ((sPrintExcept == ID_TRUE) && (sExclude[i] == ID_TRUE)) ||
            ((sPrintOnly   == ID_TRUE) && (sInclude[i] == ID_FALSE)) )
        {
            continue;
        }
        else
        {
            /* fall through */
        }

        idlOS::snprintf(sTemp,
                        ID_SIZEOF(sTemp),
                        "%s_MSGLOG_FILE", ideLog::mMsgModuleName[i]);
        IDE_TEST_RAISE(idp::readPtr(sTemp,
                                    (void **)&sFileName)
                       != IDE_SUCCESS, property_error);
        idlOS::snprintf(gPath[j], ID_MAX_FILE_NAME,
                        "%s%s%s",
                        sPath,
                        IDL_FILE_SEPARATORS,
                        sFileName);
    
        j++;
    }

    gNoFile = j;

    traceFile();
    if( sPrintFollow == ID_TRUE )
    {
        followFile();
    }
    else
    {
        idlOS::printf("%d %s printed.\n", gPrinted,
                      gPrinted > 1? "logs":"log");
    }

    return 0;

    IDE_EXCEPTION(property_error)
    {
        idlOS::printf("Error : Loading property failed.\n");
        idlOS::printf("It is essential to print trace logs.\n");
    }

    IDE_EXCEPTION_END;
    return -1;
}
