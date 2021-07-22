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
#include <idu.h>
#include <smuVersion.h>
#include <qcm.h>
#include <rp.h>
#include <cmuVersion.h>
#include <altipkg.h>

static SChar const *gHeader = "\
===============================================\n\
     ALTIBASE PACKAGE UTILITY VER 1.0\n\
===============================================\n";

static SChar const *gHelpMsg = "\
===============================================\n\
     Usage : altipkg [options]                 \n\
        -i : message information trace         \n\
        -q : quiet mode                        \n\
                                               \n\
        -d : do Distribution                   \n\
        -a : arguments from Makefile           \n\
        -r : Name of Root Package Directory    \n\
        -m : Name of Package Description File  \n\
                                               \n\
        -v : do Verification                   \n\
                                               \n\
        -e : ignore error (used in dist for developer) \n\
        -p : Distribution flag for patch       \n\
===============================================\n";

static SChar const *gReportHeader = ""\
"#===============================================================\n"
"#       Altibase Package REPORT FILE \n"
"#       Package Date : %s" 
"#       Product       Version %s %s %s \n"
"#       Binary DB     Version %s \n"
"#       Meta          Version %"ID_UINT32_FMT".%"ID_UINT32_FMT".%"ID_UINT32_FMT" \n"
"#       Cm   Protocol Version %"ID_UINT32_FMT".%"ID_UINT32_FMT".%"ID_UINT32_FMT" \n"
"#       Repl Protocol Version %"ID_UINT32_FMT".%"ID_UINT32_FMT".%"ID_UINT32_FMT"\n"
"#===============================================================\n";

SChar *gPkgArg[ALTI_PKG_ARG_MAX];
SChar *gPkgRootDir =    PKG_ROOT_DEFAULT_DIR_NAME;
SChar *gPkgMapFile =    PKG_MAP_DEFAULT_NAME;
SChar *gPkgReportFile = PKG_REPORT_FILE_NAME;
SChar *gPatchOrgDir   = NULL; /* patch compare original directory */ 
FILE  *gReportFP;
SChar  gReportFile[4096];


idBool gDebug       = ID_FALSE;
idBool gQuiet       = ID_FALSE;
idBool gErrorIgnore = ID_FALSE;
idBool gReport      = ID_TRUE; // default generation 

void makeDirectory(SChar *aDestDir, SChar *aPerm, idBool aIgnore);

/* ------------------------------------------------
 * Convertion from Makefile Macros to 
*  PFD (Package File Descriptor) Symbol
 * ----------------------------------------------*/

typedef struct strConv
{
    SChar *aBefore;
    SChar *aAfter;
}strConv;


static strConv gEditionConv[] =
{
    { (SChar *)"ENTERPRISE", (SChar *)"E"   } ,
    { (SChar *)"STANDARD",   (SChar *)"S"   } ,
    { (SChar *)"EMBEDDED",   (SChar *)"N"   } ,
    { (SChar *)"MOBILE",     (SChar *)"M"   } ,
    { (SChar *)"COMMUNITY",  (SChar *)"C"   } ,
    { (SChar *)"DISK",       (SChar *)"D"   } ,
    { (SChar *)"OPEN",       (SChar *)"O"   } ,
    { (SChar *)"*",          (SChar *)"*"   } ,
    { NULL, NULL }
};


static strConv gDevConv[] =
{
    { (SChar *)"SERVER", (SChar *)"S"   } ,
    { (SChar *)"CLIENT", (SChar *)"C"   } ,
    { (SChar *)"*",      (SChar *)"*"   } ,
    { NULL, NULL }
};


SChar * getConversion(strConv *aConv, SChar *aDefine)
{
    UInt i;

    for (i = 0; aConv[i].aBefore != NULL; i++)
    {
        debugMsg("Compare ??: <<[%s]>>  <<[%s][%s]>>\n", aDefine, aConv[i].aBefore, aConv[i].aAfter);
        if (idlOS::strcasecmp(aConv[i].aBefore, aDefine) == 0)
        {
            debugMsg("OK : <<[%s]>>  <<[%s][%s]>>\n", aDefine, aConv[i].aBefore, aConv[i].aAfter);
            return aConv[i].aAfter;
        }
    }
    altiPkgError("Conversion Error => [%s]", aDefine);

    return (SChar *)"";
}

SChar * argumentConversion(UInt aArgNum, SChar *aDefine)
{
    switch(aArgNum)
    {
        case ALTI_PKG_ARG_EDITION:
            return getConversion(gEditionConv, aDefine);
            
        case ALTI_PKG_ARG_DEVTYPE:
            return getConversion(gDevConv, aDefine);
            
            // conversion don't needed.
        default:
            if (aDefine == NULL)
            {
                return (SChar *)"";
            }
            else
            {
                return aDefine;
            }
            
    }
}

/* ------------------------------------------------
 *  Package Information String 
 * ----------------------------------------------*/

typedef struct pkgInfoArg
{
    altiPkgArgNum mNum;
    SChar const  *mName;
} pkgInfoArgList;

static pkgInfoArg gPkgInfoArg[] =
{
    {
        ALTI_PKG_ARG_CPU,
        "CPU_TYPE"
    } ,
    {
        ALTI_PKG_ARG_OS,
        "OS_TYPE"
    } ,
    {
        ALTI_PKG_ARG_MAJORVER,
        "OS_MAJORVER_VER"
    } ,
    { 
        ALTI_PKG_ARG_MINORVER,
        "OS_MINORVER_VER"
    } ,
    {
        ALTI_PKG_ARG_EDITION,
        "EDITION_TYPE"
    } ,
    {
        ALTI_PKG_ARG_DEVTYPE,
        "DEVTYPE"
    } ,
    {
        ALTI_PKG_ARG_BITTYPE,
        "BITTYPE"
    } ,
    {
        ALTI_PKG_ARG_COMPTYPE,
        "COMPTYPE"
    } ,
    {
        ALTI_PKG_ARG_EXEEXT,
        "EXEEXT"
    } ,
    {
        ALTI_PKG_ARG_OBJEXT,
        "OBJEXT"
    } ,
    {
        ALTI_PKG_ARG_LIBPRE,
        "LIBPRE"
    } ,
    {
        ALTI_PKG_ARG_LIBEXT,
        "LIBEXT"
    } ,
    {
        ALTI_PKG_ARG_SOEXT,
        "SOEXT"
    } ,
    {
        ALTI_PKG_ARG_END_ARG,
        NULL,
    }
};





int main(SInt argc, SChar *argv[])
{
    SInt          opr;
    UInt          sProcessed = 0;
    UInt          sArgNum = 0;
    altiPkgActNum sAction = ALTI_PKG_ACT_MAX;
    SChar        *sPkgRootDir = NULL;

    idlOS::fprintf(stdout, "%s", gHeader);
    
    while ( (opr = idlOS::getopt(argc, argv, "da:vr:m:iqep:")) != EOF) 
    {
        switch(opr)
        {
            case 'i':
                gDebug = ID_TRUE;
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);

                break;

            case 'q':
                gQuiet = ID_TRUE;
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);

                break;

            case 'v': // verification action 
                sProcessed++;
                sAction = ALTI_PKG_ACT_VERIFICATION;
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);
                break;
                
            case 'd': // distribution action 
                sProcessed++;
                sAction = ALTI_PKG_ACT_DISTRIBUTION;
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);
                break;
                
            case 'a':  // 정보 전달 
                sProcessed++;
                gPkgArg[sArgNum++] = (SChar *)optarg; 
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);
                break;

            case 'r': // 패키징 할 디렉토리
                sPkgRootDir = (SChar *)optarg;
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);
                break;
                
            case 'm': // 패키지 명세 화일명 
                gPkgMapFile  = (SChar *)optarg;
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);
                break;

            case 'e':
                gErrorIgnore = ID_TRUE;
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);
                break;
                
            case 'p':
                gPatchOrgDir= (SChar *)optarg;
                if (gPatchOrgDir == NULL)
                {
                    altiPkgError("Specify the patch base directory.");
                    idlOS::exit(-1);
                }
                
                debugMsg("test : %s:%d\n", __FILE__, __LINE__);
                break;
                
            default :
                altiPkgError("invalid argument");
                break;
        }
    }
    
    if (sProcessed == 0)
    {
        idlOS::printf(gHelpMsg);
        idlOS::fflush(stdout);
        idlOS::exit(0);
    }

    /* ------------------------------------------------
     *  Root Dir 처리
     *  Default일 경우 절대경로로 바꾸어서 처리하고,
     *  그렇지 않을 경우, 입력을 절대경로라고 가정함.
     * ----------------------------------------------*/
    if (sPkgRootDir == NULL)
    {
        
        SChar *sDevPath;
        SChar *sNewRoot;
        UInt   sSize;
        
        sDevPath = idlOS::getenv(ALTIBASE_ENV_PREFIX"DEV");

        if (sDevPath == NULL)
        {
            altiPkgError("No Environment [ALTIBASE_DEV]\n");
        }

        sSize = idlOS::strlen(gPkgRootDir) + idlOS::strlen(sDevPath) + 4;
        sNewRoot = (SChar *)idlOS::malloc(sSize);

        if (sNewRoot == NULL)
        {
            altiPkgError("Memory Allocation Error [size=%"ID_UINT32_FMT"] (errno=%"ID_UINT32_FMT")\n",
                         sSize, errno);
        }
        idlOS::memset(sNewRoot, 0, sSize);
        
        idlOS::sprintf(sNewRoot, "%s%s%s",
                       sDevPath,
                       IDL_FILE_SEPARATORS,
                       gPkgRootDir);

        gPkgRootDir = sNewRoot;
    }
    else
    {
        gPkgRootDir = sPkgRootDir;
    }
    {
      // gPkgRootDir에서 마지막 / or \ 제거
        ULong sLen;
        sLen = idlOS::strlen(gPkgRootDir) - 1;
        for ( ; gPkgRootDir[sLen] == IDL_FILE_SEPARATOR ; sLen--)
        {
            gPkgRootDir[sLen] = 0;
        }
    }

    /* ------------------------------------------------
     *  동작 수행 
     * ----------------------------------------------*/
    
    switch(sAction)
    {
        case ALTI_PKG_ACT_DISTRIBUTION:
        {
            UInt i;

            /* ------------------------------------------------
             *  1. 인자 적합성 검사.
             * ----------------------------------------------*/
            
            if (sArgNum != ALTI_PKG_ARG_END_ARG)
            {
                altiPkgError("distribution argument insufficient. %"ID_UINT32_FMT
                             "(should be %"ID_UINT32_FMT")\n", sArgNum, ALTI_PKG_ARG_MAX);
            }

            for (i = 0; i < ALTI_PKG_ARG_MAX; i++)
            {
                gPkgArg[i] = argumentConversion(i, gPkgArg[i]);
                debugMsg("ARGUMENT(%"ID_UINT32_FMT") (%s)\n", i, gPkgArg[i]);
                
            }

            /* ------------------------------------------------
             *  2. Root Package Directory 생성 
             * ----------------------------------------------*/
            makeDirectory(gPkgRootDir, (SChar *)"0755", gErrorIgnore); // force create 

            /* ------------------------------------------------
             *  3. Report 화일 준비 
             * ----------------------------------------------*/
            {
                time_t sCurTime;

                sCurTime = idlOS::time(NULL);
                
                idlOS::sprintf(gReportFile,
                               "%s%s%s",
                               gPkgRootDir,
                               IDL_FILE_SEPARATORS,
                               gPkgReportFile);
                
            
                changeFileSeparator(gReportFile);
                
                debugMsg("report file dir = [%s]\n", gReportFile);
                
                gReportFP = idlOS::fopen(gReportFile, "w+");
                
                if (gReportFP == NULL)
                {
                    altiPkgError("Can't create report File : %s\n", gReportFile);
                }
                
                reportMsg(gReportHeader, 
                          idlOS::asctime(idlOS::gmtime(&sCurTime)),
                          iduVersionString,
                          iduGetSystemInfoString(),
                          iduGetProductionTimeString(),
                          smVersionString,
                          QCM_META_MAJOR_VER,
                          QCM_META_MINOR_VER,
                          QCM_META_PATCH_VER,
                          CM_MAJOR_VERSION,
                          CM_MINOR_VERSION,
                          CM_PATCH_VERSION,
                          REPLICATION_MAJOR_VERSION,
                          REPLICATION_MINOR_VERSION,
                          REPLICATION_FIX_VERSION
                          );


                // Package Information 기록
                reportMsg("\n\n"
                          "# Package Information ( * = ALL )\n");

                for (i = 0; gPkgInfoArg[i].mName != NULL; i++)
                {
                    if ( i != (UInt)gPkgInfoArg[i].mNum)
                    {
                        altiPkgError("Pacakge Info Invalid : %"ID_UINT32_FMT
                                     " != %"ID_UINT32_FMT"\n", i, gPkgInfoArg[i].mNum);
                    }
                    reportMsg("# %s : %s \n", gPkgInfoArg[i].mName, gPkgArg[i]);
                }
                reportMsg("\n\n");
            }
            
            /* ------------------------------------------------
             *  4. Distribution 작업 수행!
             * ----------------------------------------------*/
            
            doDistribution(gQuiet);

            idlOS::fclose(gReportFP);
            
            break;
        }

        case ALTI_PKG_ACT_VERIFICATION:
            doVerification();
            break;
        default:
            altiPkgError("invalid action number.(%"ID_UINT32_FMT"\n", (UInt)sAction);
    }
    
    return 0;
}
