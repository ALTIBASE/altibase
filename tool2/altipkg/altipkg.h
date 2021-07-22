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

#ifndef _O_ALTI_PKG_H_
# define __ALTI_PKG_H_ 1

#include <idl.h>

#define PKG_ROOT_DEFAULT_DIR_NAME (SChar *)"altipkgdir"
#define PKG_MAP_DEFAULT_NAME      (SChar *)"pkg.map"
#define PKG_REPORT_FILE_NAME      (SChar *)"report.txt"

typedef enum
{
    ALTI_PKG_ARG_CPU = 0,
    ALTI_PKG_ARG_OS,
    ALTI_PKG_ARG_MAJORVER,
    ALTI_PKG_ARG_MINORVER,
    ALTI_PKG_ARG_EDITION,
    ALTI_PKG_ARG_DEVTYPE,
    ALTI_PKG_ARG_BITTYPE,
    ALTI_PKG_ARG_COMPTYPE,

    ALTI_PKG_ARG_EXEEXT,
    ALTI_PKG_ARG_OBJEXT,
    ALTI_PKG_ARG_LIBPRE,
    ALTI_PKG_ARG_LIBEXT,
    ALTI_PKG_ARG_SOEXT,
    
    ALTI_PKG_ARG_END_ARG,
    
    ALTI_PKG_ARG_EMPTY_STRING,
    
    ALTI_PKG_ARG_MAX
    
} altiPkgArgNum;

typedef enum
{
    ALTI_PKG_ACT_DISTRIBUTION = 0,
    ALTI_PKG_ACT_VERIFICATION,
    
    ALTI_PKG_ACT_MAX
} altiPkgActNum;

/* ------------------------------------------------
 * Function Prototypes
 * ----------------------------------------------*/

void doDistribution(idBool aQuiet);

void doVerification();

/* ------------------------------------------------
 *  Library Functions
 * ----------------------------------------------*/

void  eraseWhiteSpace(SChar *aBuffer);
void  changeFileSeparator(SChar *aBuf);
ULong getFileSize(SChar *aFileName);
void  getFileStat(SChar *aFileName, PDL_stat *aStat);
void  altiPkgError(const char *format, ...);
void  reportMsg(const char *format, ...);
void  debugMsg(const char *format, ...);

/* ------------------------------------------------
 *  Distribution
 * ----------------------------------------------*/
void doDistribution(idBool aQuiet);


/* ------------------------------------------------
 *  Verification
 * ----------------------------------------------*/
void doVerification();

/* ------------------------------------------------
 *  Values
 * ----------------------------------------------*/

extern SChar *gPkgArg[ALTI_PKG_ARG_MAX];
extern SChar *gPkgRootDir;
extern SChar *gPkgMapFile;
extern SChar *gPkgReportFile;
extern SChar *gPatchOrgDir;
extern FILE  *gReportFP;
extern idBool gDebug;
extern idBool gReport;
extern idBool gQuiet;
extern idBool gErrorIgnore;
extern SChar  gReportFile[4096];

#endif
