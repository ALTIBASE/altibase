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
 * $Id: altidump.c 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

/*
 * we wouldn't follow altibase coding convention.
 * because, this app have to use bfd with gcc. so, dont use idlOS::..etc.
 * by gamestar 2003/1/7
 */

#include <string.h>
#ifdef POWERPC_LINUX || ifdef POWERPC64_LINUX 
#include <locale>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <bfd.h>
#include <idConfig.h>

#if ( (IBM_AIX) && (OS_MAJORVER == 5) && (OS_MINORVER == 3) )
int _nl_msg_cat_cntr = 0;
#endif

/*#define DUMP_ALL */

#define MAX_LOG_STACK  2048
#define valueof(x) (long long)(((asymbol*)x)->section->vma + ((asymbol*)x)->value)

char gLogFile[1024];
char gBinFile[1024];

typedef struct
{
    long long mAddress; /* -1 is EOF */
    char     *mMsg;
} StackArray;

StackArray gStackArray[MAX_LOG_STACK];

asymbol **gSymbolArray;
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
        "   altidump altibase_boot.log [altibase] \n"
        "   => if you omit [altibase], altidump will use ${ALTIBASE_HOME}/bin/altibase \n";

    printf(sUsage);
}

void getBinFromEnv()
{
    char *sEnv;
    unsigned int sSize;


    sEnv = getenv(ALTIBASE_ENV_PREFIX"HOME");
    if (sEnv == NULL)
    {
        outError("can't get ALTIBASE_HOME environment variable");
    }
    // bug-33941: codesonar: buffer overrun
    sSize = sizeof(gBinFile);
    snprintf(gBinFile, sSize-1, "%s/bin/altibase", sEnv);
    gBinFile[sSize-1] = '\0';
}

void
bfd_nonfatal (CONST char *string)
{
    CONST char *errmsg = bfd_errmsg (bfd_get_error ());

    if (string)
        fprintf (stderr, "%s: %s: %s\n", "sample", string, errmsg);
    else
        fprintf (stderr, "%s: %s\n", "sample", errmsg);
}

void
bfd_fatal (CONST char *string)
{
    bfd_nonfatal (string);
    exit (1);
}

int compare(const void *aSym1, const void *aSym2)
{
    asymbol *sSym1;
    asymbol *sSym2;

    sSym1 = *(asymbol **)aSym1;
    sSym2 = *(asymbol **)aSym2;
    
    if (valueof (sSym1) != valueof (sSym2))
        return valueof (sSym1) < valueof (sSym2) ? -1 : 1;
    
    return 0;
}

void materializeSymbol(bfd *aBFD)
{
    long      sSize;
    long      i;
    long      j;
    asymbol **sFuncSymbol;
    

    sSize = bfd_get_symtab_upper_bound(aBFD);
    if (sSize <= 0)
    {
        outError("size of symbol is invalid");
    }

    gSymbolArray = (asymbol **)malloc(sSize);
    if (gSymbolArray == NULL)
    {
        fprintf(stderr, "Memory allocation error.(%s:%d)\n", __FILE__, __LINE__);
        exit(-1);
    }
    sFuncSymbol  = (asymbol **)malloc(sSize);
    if (sFuncSymbol == NULL)
    {
        fprintf(stderr, "Memory allocation error.(%s:%d)\n", __FILE__, __LINE__);
        exit(-1);
    }
    
    gCountOfSymbol = bfd_canonicalize_symtab(aBFD, gSymbolArray);


    if (gCountOfSymbol < 0)
    {
        outError("count of symbol is invalid");
    }

    
    /* filtering of symbol : just only for function symbol*/
    j = 0; 
    for (i = 0; i < gCountOfSymbol; i++)
    {
        if (gSymbolArray[i]->flags & BSF_FUNCTION)
        {
            sFuncSymbol[j++] = gSymbolArray[i];
        }
    }

    free(gSymbolArray);
    gSymbolArray   = sFuncSymbol;
    gCountOfSymbol = j;
    
    /*
      sBFD->arch_info->bits_per_address
      */
    
    qsort((void *)gSymbolArray, gCountOfSymbol, sizeof(asymbol *), compare); 
    
    printf("            => [ Compiled Bit = %d,  Symbol Count = %d ]\n",
           (int)bfd_arch_bits_per_address(aBFD), 
           (int)gCountOfSymbol);
    
}

void readSymFromBin(char *aBinFile)
{
    bfd   *sBFD;
    char  *sTarget = NULL;
/*    char  *sTest2 = "powerpc-ibm-aix5.1.0.0"; */
#if defined(IBM_AIX)
    char  *sTest2 = "powerpc-ibm-aix";
#endif

#if defined(X86_64_DARWIN)
    char *sTest2 = "mach-o-x86-64";
#endif
    char **matching;


    char **tlist;
    
    bfd_init();
#if defined(IBM_AIX)
    if (! bfd_set_default_target (sTest2))  
    {
        printf("error in default target\n");
        exit(1);
    }
#endif

#if defined(X86_64_DARWIN)
    tlist = bfd_target_list();
    if (! bfd_set_default_target (sTest2))  
    {
        printf("error in default target\n");
        exit(1);
    }

    for (; *tlist != NULL; tlist++)
    {
        printf("%s\n", *tlist);
    }
#endif
        
    sBFD = bfd_openr (aBinFile, sTarget);
    if (sBFD == NULL)
    {
        bfd_fatal("bfd_openr error");
    }


    if (bfd_check_format_matches (sBFD, bfd_object, &matching) == 0)
    {
        bfd_fatal("this is not an object format.");
    }

    if ( (bfd_get_file_flags (sBFD) & HAS_SYMS) == 0) /* no symbol */
    {
        bfd_fatal("no symbol exist.");
    }
    
    materializeSymbol(sBFD);
    
}

void readCallstackFromLog(char *gLogFile)
{
    FILE *sFP;
    int   sOffset;
    int   i;
    char sAddr[32];
    char sBuf[2048];
    long long sCallerValue;

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
        memset(sBuf,  0, 2048);
        memset(sAddr, 0, 32);
        if (fgets(sBuf, 2048, sFP) == NULL)
        {
            break;
        }

        if (strncmp(sBuf, "Caller(", 7) != 0)
        {
            if (strncmp(sBuf, "BEGIN-STACK-[CRASH]", 19) == 0)
            {
                gStackArray[sOffset].mAddress = 0;
                gStackArray[sOffset++].mMsg = "\n== Crash Stack Dump ==\n";
            }
            if (strncmp(sBuf, "BEGIN-STACK-[NORMAL]", 19) == 0)
            {
                gStackArray[sOffset].mAddress = 0;
                gStackArray[sOffset++].mMsg = "\n== Normal Stack Dump ==\n";
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
        // bug-33941: codesonar: uninitialized variable
        sCallerValue = 0;
        sAddr[32-1] = '\0';
        sscanf(sAddr, "%llx", &sCallerValue);
#if (IBM_AIX)
        
    /* address recalculation :
       ex ) real : 0x100xxxxxx  =>  text : 0x10xxxxxx
    */
#if  ( (OS_MAJORVER == 5) && (OS_MINORVER == 3) )
       /* do nothing */
       
#else       
        sCallerValue = (sCallerValue & 0x0FFFFFFF) | (0x10000000LL);
#endif

        
#endif
        if (strstr(sBuf, "CORE") != NULL)
        {
            gStackArray[sOffset].mMsg = "Core Position !!\n";
        }
        
        gStackArray[sOffset++].mAddress = sCallerValue;

    }

}

asymbol* findSymbolByAddress(long long aAddr)
{
    int i;
    asymbol *sSymbol;

    for (i = 1; i < gCountOfSymbol; i++)
    {
        sSymbol = gSymbolArray[i];

        if ( aAddr < valueof(sSymbol))
        {
            return gSymbolArray[i - 1];
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

void dumpStackSymbol()
{
    int i;
    asymbol *sSymbol;
#if defined(DUMP_ALL)
    for (i = 0; ; i++)
    {
        if (gStackArray[i].mAddress == -1) break;
        fprintf(stderr, "%llx \n", gStackArray[i].mAddress);
    }
    
    for (i = 0; i < gCountOfSymbol; i++)
    {
        if (gSymbolArray[i]->flags & BSF_FUNCTION)
        {
            printf("%08llx:name = %s flag=%lx\n",
                   (long long)valueof(gSymbolArray[i]),
                   gSymbolArray[i]->name,
                   gSymbolArray[i]->flags);
        }
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
                       (long long)gStackArray[i].mAddress);
                continue;
            }
            printf("0x%08llx : %s \n",
                   (long long)gStackArray[i].mAddress, sSymbol->name);
            
            if (gStackArray[i].mMsg != NULL) // Header
            {
                printf("    %s \n", gStackArray[i].mMsg);
            }
        }
    }
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
        strcpy(gBinFile, argv[2]);
    }

    printf("Dumping Info :\n"
           " Log File : %s\n"
           " Bin File : %s\n", gLogFile, gBinFile);

    
    readCallstackFromLog(gLogFile);
    readSymFromBin(gBinFile);

    dumpStackSymbol();

    // fix BUG-25529 [CodeSonar::MissingReturn]
    return 0;
}
