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
 * $Id: altidump-win.c 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

/*
 * we wouldn't follow altibase coding convention.
 * because, this app have to use bfd with gcc. so, dont use idlOS::..etc.
 * by gamestar 2003/1/7
 */

#include <idConfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

#define MAX_LOG_STACK  2048
#define valueof(x) (long long)(((asymbol*)x)->section->vma + ((asymbol*)x)->value)

char gLogFile[1024];
char gMapFile[1024];

#define DBG(a)  fprintf(stderr, "DBG:[%s] ==> [%s][%d]\n", #a, __FILE__, __LINE__); fflush(stderr);
#define DBG2(a)  fprintf(stderr, "DBG:[%s] ==> [%s][%d]\n", a, __FILE__, __LINE__); fflush(stderr);

typedef struct
{
    long mAddress; /* -1 is EOF */
    char     *mMsg;
} StackArray;

typedef struct
{
    long      mAddress; /* -1 is EOF */
    char     *mFuncName;
} SymbolArray;

StackArray gStackArray[MAX_LOG_STACK];

SymbolArray **gSymbolArray;
long      gCountOfSymbol;

void outError(char *message)
{
    printf("ERROR : %s \n", message);
    exit(-1);
}

void outUsage()
{
    char *sUsage =
        "Usage : \n"
        "   altidump boot.log [altibase.exe.map] \n"
        "   => if you omit [altibase.exe.map], altidump will use ${ALTIBASE_HOME}/bin/altibase.exe.map \n";

    printf(sUsage);
}

int compare(const void *aSym1, const void *aSym2)
{
    SymbolArray *sSym1;
    SymbolArray *sSym2;

    sSym1 = *(SymbolArray **)aSym1;
    sSym2 = *(SymbolArray **)aSym2;
    
    if (sSym1->mAddress != sSym2->mAddress)
        return sSym1->mAddress < sSym2->mAddress ? -1 : 1;
    
    return 0;
}


void getBinFromEnv()
{
    char *sEnv;

    sEnv = getenv(ALTIBASE_ENV_PREFIX"HOME");
    if (sEnv == NULL)
    {
        outError("can't get ALTIBASE_HOME environment variable");
    }
    sprintf(gMapFile, "%s\\bin\\altibase.exe.map", sEnv);
}

#define MAX_SYMBOL  (1024 * 1024)

void readSymFromMap(char *aMapFile)
{
    FILE *sFP;
    int   sOffset;
    int   i;
    int   sState = 0;

    gSymbolArray = (SymbolArray **)malloc(MAX_SYMBOL * sizeof(SymbolArray *));
    if (gSymbolArray == NULL)
    {
        fprintf(stderr, "Memory allocation error.(%s:%d)\n", __FILE__, __LINE__);
        exit(-1);
    }
    memset(gSymbolArray, 0, MAX_SYMBOL * sizeof(SymbolArray *));
/*     sFuncSymbol  = (asymbol **)malloc(sSize); */
    
/*     printf("            => [ Compiled Bit = %d,  Symbol Count = %d ]\n", */
/*            (int)aBFD->arch_info->bits_per_address,  */
/*            (int)gCountOfSymbol); */
    

//     for (i = 0; i < MAX_SYMBOL; i++)
//     {
//         gSymbolArray[i]->mAddress = -1;
//         gSymbolArray[i]->mFuncName= NULL;
//     }
        
    sFP = fopen(gMapFile, "r");
    if (sFP == NULL)
    {
        fprintf(stderr, "Error in fopen(%s) \n", gMapFile);
        exit(-1);
    }

    /* ------------------------------------------------
     *  1. Find Symbol Section 
     * ----------------------------------------------*/

    sOffset = 0;
    while(feof(sFP) == 0)
    {
        char sAddr[32];
        char sBuf[1024 * 4];
        long sCallerValue;
        memset(sBuf,  0, sizeof(sBuf));
        memset(sAddr, 0, sizeof(sAddr));
        if (fgets(sBuf, sizeof(sBuf) - 1, sFP) == NULL)
        {
            break;
        }
        if (strncmp(sBuf, "  Address", 9) == 0)
        {
            sState = 1;
            break;
        }
    }
    
    if (sState != 1) /* found section */
      {
        fprintf(stderr, "can't find Address Section..map file corrupted???");
        exit(-1);
      }
    /* ------------------------------------------------
     *  2. Collecting Symbol 
     *    section           symbol                     address  type  object
     *  0001:00000030 _main 00401030 f   altibase.obj
     * ----------------------------------------------*/

    while(feof(sFP) == 0)
    {
        char sAddr[32];
        char sBuf[1024 * 4];
        long long sCallerValue;
        
        char *ptr;
        int symlen = 0;
        int i;

        memset(sBuf,  0, sizeof(sBuf));
        memset(sAddr, 0, sizeof(sAddr));
        if (fgets(sBuf, sizeof(sBuf) - 1, sFP) == NULL)
        {
            break;
        }
        /* 1. check section */
        if (strncmp(sBuf, " 0001:", 6) != 0)
        {
          /* end of find */
          /*
          if (strncmp(sBuf, " 0002:", 6) == 0)
            {
              sState = 2;
              break;
            }
          */
          continue;
        }
        ptr = sBuf + 6;
        /* 2. get symbol */
        
        while( (*ptr++) != ' '); /* skip ascii */
        while( (*ptr++) == ' '); /* skip space */
        while( (*ptr++) != ' ') symlen++; /* calc symbol size */
        symlen++;
        /* get real symbol */
        gSymbolArray[gCountOfSymbol] = (SymbolArray *)malloc(sizeof(SymbolArray ));
        if (gSymbolArray[gCountOfSymbol] == NULL)
        {
            fprintf(stderr, "Memory allocation error.(%s:%d)\n", __FILE__, __LINE__);
            exit(-1);
        }
        gSymbolArray[gCountOfSymbol]->mFuncName = (char *)malloc(symlen + 10);
        if (gSymbolArray[gCountOfSymbol]->mFuncName == NULL)
        {
            fprintf(stderr, "Memory allocation error.(%s:%d)\n", __FILE__, __LINE__);
            exit(-1);
        }
        memset(gSymbolArray[gCountOfSymbol]->mFuncName, 0, symlen + 10);
        strncpy(gSymbolArray[gCountOfSymbol]->mFuncName, ptr - symlen - 1, symlen);

        while( (*ptr++) == ' '); /* skip space */
        for (i = 0; *ptr != ' '; i++, ptr++)
        {
            sAddr[i] = *ptr;
        }
        //sCallerValue = strtoll(sAddr, NULL, 16);
        sscanf(sAddr, "%llx", &sCallerValue);
        /* analyze Caller Address */

        gSymbolArray[gCountOfSymbol]->mAddress = sCallerValue;
        gCountOfSymbol++;
        if (gCountOfSymbol == MAX_SYMBOL)
        {
            fprintf(stderr, "symbol max overflowed..(MAX=%d)\n", MAX_SYMBOL);
            exit(-1);
        }
    }
    fprintf(stderr, "Total Symbol Count = %d \n", gCountOfSymbol);

    qsort((void *)gSymbolArray, gCountOfSymbol, sizeof(SymbolArray *), compare); 
}

void readCallstackFromLog(char *aLogFile)
{
    FILE *sFP;
    int   sOffset;
    int   i;
    int   line=1;

    for (i = 0; i < MAX_LOG_STACK; i++)
    {
        gStackArray[i].mAddress = -1;
        gStackArray[i].mMsg     = NULL;
    }
        
    sFP = fopen(gLogFile, "r");
    if (sFP == NULL)
    {
        fprintf(stderr, "Error in fopen(%s) \n", gLogFile);
        exit(-1);
    }

    sOffset = 0;
    while(feof(sFP) == 0)
    {
        char sAddr[32];
        char sBuf[2048];
        long long sCallerValue;
        memset(sBuf,  0, 2048);
        memset(sAddr, 0, 32);


        if (sOffset == MAX_LOG_STACK)
          {
            fprintf(stderr, "ERROR : max log stack overflowed[MAX=%d]\n", 
                    MAX_LOG_STACK);
            exit(-1);
          }
        if (fgets(sBuf, 2048, sFP) == NULL)
        {
            break;
        }
        
        line++;
        if (strncmp(sBuf, "Caller(", 6) != 0)
        {
            if (strstr(sBuf, "BEGIN-STACK-[CRASH]", 19) != NULL)
            {
                gStackArray[sOffset].mAddress = 0;
                gStackArray[sOffset++].mMsg = "\n== Crash Stack Dump ==\n";
                fprintf(stderr, "(%d) [%s] 0x%08X\n", sOffset, gStackArray[sOffset].mMsg, sCallerValue); fflush(stderr);
            }
            if (strstr(sBuf, "BEGIN-STACK-[NORMAL]", 19) != NULL)
            {
                gStackArray[sOffset].mAddress = 0;
                gStackArray[sOffset++].mMsg = "\n== Normal Stack Dump ==\n";
                fprintf(stderr, "(%d) [%s] 0x%08X\n", sOffset, gStackArray[sOffset].mMsg, sCallerValue); fflush(stderr);
            }
            continue;
        }
        /* analyze Caller Address */

        for (i = 0; ; i++)
        {
            char sSeed;
            sSeed = sBuf[i + 7];
            if ( isxdigit(sSeed) == 0)
            {
                break;
            }
            sAddr[i] = sSeed; 
        }

        //sCallerValue = strtoll(sAddr, NULL, 16);
        sscanf(sAddr, "%llx", &sCallerValue);
        
        if (strstr(sBuf, "CORE") != NULL)
        {
            gStackArray[sOffset].mMsg = "Core Position !!\n";
        }
        
        gStackArray[sOffset++].mAddress = sCallerValue;
        fprintf(stderr, "(%d) [%s] 0x%08X\n", sOffset, gStackArray[sOffset].mMsg, sCallerValue);
    }
}

char* findSymbolByAddress(long  aAddr)
{
    int i;
    char *sSymbol;

    for (i = 1; i < gCountOfSymbol; i++)
    {
        if ( aAddr < gSymbolArray[i]->mAddress)
        {
            return gSymbolArray[i - 1]->mFuncName;
        }
    }
    return NULL;
    /*
    int sPos;
    
    sPos    = gCountOfSymbol / 2;
    sSymbol = gSymbolArray[sPos];

    if ( valueof(sSymbol) > aAddr
    */
    
}
/*#define DUMP_ALL*/
void dumpStackSymbol()
{
    int i;
    char *sSymbol;

    fprintf(stderr, "\n\nCALLER List \n");
    for (i = 0; ; i++)
    {
        if (gStackArray[i].mAddress == -1) break;
        fprintf(stderr, "(%d)  0x%X \n", i, gStackArray[i].mAddress);
        fflush(stderr);
    }

#if defined(DUMP_ALL)
    fprintf(stderr, "\n\nSymbol List \n");
    for (i = 0; i < gCountOfSymbol; i++)
    {
      fprintf(stderr, "%08lx:name = %s\n",
             ( long)gSymbolArray[i]->mAddress,
             gSymbolArray[i]->mFuncName);
    }
#endif    
    printf("\n===============================================================\n");
    printf(  "                          DUMP OF STACK                        \n");
    printf(  "===============================================================\n");
    /*
                             mAddress      |   mMsg
            Exit   :           -1              Don't Care
            Header :            0              Not Null
            Normal :           Value( 0)       NULL
            Core   :           Value           Not NULL (Signal)
     */
    for (i = 0; ; i++)
    {
        if (gStackArray[i].mAddress == -1) // Exit
        {
            break;
        }
        else
        {
            if (gStackArray[i].mAddress == 0)
            {
                if (gStackArray[i].mMsg != NULL) // Header 
                {
                    printf("%s", gStackArray[i].mMsg);
                    continue;
                }
            }
            sSymbol = findSymbolByAddress(gStackArray[i].mAddress);
            
            if (sSymbol == NULL)
            {
                printf("0x%08llx : ?????????????????? \n",
                       ( long)gStackArray[i].mAddress);
                continue;
            }
            printf("0x%08llx : %s \n",
                   ( long)gStackArray[i].mAddress, sSymbol);
            
            if (gStackArray[i].mMsg != NULL) // Header
            {
                printf("    %s \n", gStackArray[i].mMsg);
            }
        }
    }
    fflush(stdout);
}                    

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        outUsage();
        exit(0);
    }
    
    strcpy(gLogFile, argv[1]);
    
    if (argc == 2)
    {
        getBinFromEnv();
    }
    else
    {
        strcpy(gMapFile, argv[2]);
    }

    printf("Dumping Info :\n"
           " Log File : %s\n"
           " Map File : %s\n", gLogFile, gMapFile);
    fflush(stdout);
    
    readCallstackFromLog(gLogFile);
    readSymFromMap(gMapFile);

    dumpStackSymbol();

    // BUG-32698 Codesonar warnings at UX&UL module on Window 32bit
    return 0;
}
