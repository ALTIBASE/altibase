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

#include <acpConfig.h>
#include <acpPath.h>
#include <idl.h>
#include <altipkg.h>

void makeDirectory(SChar *aDestDir, SChar *aPerm);

/* ------------------------------------------------
 *  ******** DISTRIBUTION ********
 * ----------------------------------------------*/


// [로 시작하는 라인이면 1, 아니면, 0 이면 retry.
UInt getObjectHeader(SChar *aBuffer)
{
    /* ------------------------
     * [1] White Space 제거
     * ----------------------*/
    eraseWhiteSpace(aBuffer);

    /* ---------------------------------
     * [2] 내용이 없거나 주석이면 무시
     * -------------------------------*/
    SInt len = idlOS::strlen(aBuffer);
    if (len == 0 || aBuffer[0] == '#')
    {
        //debugMsg("Get Object FALSE : [%s]\n", aBuffer);
        return 0;
    }
    else
    {
        if (aBuffer[0] == '[')
        {
            //debugMsg("Get Object TRUE : [%s]\n", aBuffer);
            return 1;
        }
        else
        {
            altiPkgError("Not a Header [%s]\n", aBuffer);
        }
    }

    return 0;
}

/* ------------------------------------------------
 * [  ] 내부에 위치하는 임의의 스트링을 검사함.
 * 형식
 *   - [ member / member / member/ member ... ] 의 형태를 띄고 있음.
 *     [^member]는 member의 역을 의미.
 *   - member는 obj으로 이루어지고, obj간의 관계는 -로 분리
 *     즉, 4일 경우에는 obj-obj-obj-obj 형태
 *         1일 경우에는 obj 임.
 *  - 이 obj가 *일 경우 모든 케이스를 나타냄.
 * ----------------------------------------------*/

SChar * getObjectInMember(SChar *aMember, SChar *aObject, idBool *aEndFlag)
{
    SChar *sBuf = aMember;

    *aEndFlag = ID_FALSE;

    // Member 복사
    while(1)
    {
        switch(*sBuf)
        {
            case '/':
                sBuf++; // next position
                goto next_member;

            case ']':
                sBuf++; // next position
                *aEndFlag = ID_TRUE;
                goto next_member;

            default:
                *aObject++ = *sBuf;
                break;
        }
        sBuf++;
    }
  next_member:;

    return sBuf;
}

/* ------------------------------------------------
 * 멤버의 구성의 Object를 비교한다.
 * ----------------------------------------------*/

idBool compareObject(SChar *aArgObject, SChar *aMapObject)
{
    debugMsg("compareObject [%s] : [%s] ", aArgObject, aMapObject);

    if (*aArgObject == '*' || *aMapObject == '*')
    {
        debugMsg(" ==> TRUE \n");
        return ID_TRUE;
    }

    if (idlOS::strncmp(aArgObject, aMapObject, idlOS::strlen(aArgObject)) == 0)
    {
        debugMsg(" ==> TRUE \n");
        return ID_TRUE;
    }

    debugMsg(" ==> FALSE \n");
    return ID_FALSE;
}

idBool compareObjects(SChar *sObject, UInt aBegin, UInt aEnd)
{
    UInt i;
    SChar *sPtr = sObject;

    /* ------------------------------------------------
     *  Member가 2개 이상의 Object 일 경우에
     *  그 값이 *일 경우에는 무조건 TRUE
     *  그렇지 않으면, *를 object횟수만큼 돌기때문에
     *  문제가 생김.
     * ----------------------------------------------*/

    if (aBegin != aEnd)
    {
        if (*sPtr == '*' && idlOS::strlen(sPtr) == 1)
        {
            sPtr++;
            return ID_TRUE;
        }
    }

    for (i = aBegin; i <= aEnd; i++)
    {
        if (compareObject(gPkgArg[i], sPtr) != ID_TRUE)
        {
            return ID_FALSE;
        }
        if (i < aEnd)
        {
            // go next object aaa-bbb-ccc
            while(*sPtr != '-') sPtr++;
            sPtr++;
        }
    }

    return ID_TRUE;
}

idBool checkValidity(SChar **aPtr, UInt aBegin, UInt aEnd)
{
    SChar *sBuf = *aPtr;
    SChar  sObject[128];
    idBool sEndFlag = ID_FALSE;
    idBool sIsOk    = ID_FALSE;
    idBool sReverse = ID_FALSE;

    debugMsg("Begin Validity \n");

    if (*sBuf != '[')
    {
        altiPkgError("Not a Header [%s]\n", *aPtr);
    }
    sBuf++;

    if (*sBuf == '^')
    {
        sReverse = ID_TRUE;
        sBuf++;
    }

    do
    {
        idBool sResult;

        idlOS::memset(sObject, 0, ID_SIZEOF(sObject));

        sBuf = getObjectInMember(sBuf, sObject, &sEndFlag);

        debugMsg("    Member : [%s] \n", sObject);

        sResult = compareObjects(sObject, aBegin, aEnd);

        if (sReverse == ID_TRUE) // ^
        {   // reverse 모드일 경우에는 모두 AND 처리함. 하나라도 TRUE이면, return FALSE;
            if (sResult == ID_TRUE)
            {
                sIsOk = ID_FALSE;
                while (sEndFlag == ID_FALSE)
                {
                    sBuf = getObjectInMember(sBuf, sObject, &sEndFlag);
                }
                break;
            }
            else
            {
                sIsOk = ID_TRUE;
            }

        }
        else
        {   // reverse 모드가 아닐 경우에는 모두 OR 처리함. 하나라도 TRUE이면, return TRUE;
            if (sResult == ID_TRUE)
            {
                sIsOk = ID_TRUE;
                while (sEndFlag == ID_FALSE)
                {
                    sBuf = getObjectInMember(sBuf, sObject, &sEndFlag);
                }
                break;
            }
        }
    }
    while (sEndFlag == ID_FALSE);

    debugMsg("End Validity : => %s\n\n", (sIsOk == ID_TRUE ? "TRUE" : "FALSE"));

    *aPtr = sBuf;

    return sIsOk;
}

/* ------------------------------------------------
 *  Ext Action 설계
 * ----------------------------------------------*/

struct extAction;
typedef struct extAction
{
    SChar *mExtSym; // 스트링
    void (*mFunc)(SChar *aMsg,
                  SChar *aSrcDir,  // 액션 함수
                  SChar *aSrcFile,
                  SChar *aDestDir,
                  SChar *aDestFile,
                  SChar *aFilePerm,
                  idBool aSkipError,
                  extAction *aAction);
    altiPkgArgNum mPrefix; // 변환시 prefix 첨가
    altiPkgArgNum mPostfix; // 변환시 postfix 첨가

} extAction;

/* ------------------------------------------------
 * 디렉토리 생성 함수
 * ----------------------------------------------*/

mode_t getModeFromString(SChar *aFilePerm)
{
    UInt sMode;

    sMode = idlOS::strtol(aFilePerm, NULL, 8);

    return sMode;
}

void makeDirectory(SChar *aDestDir, SChar *aPerm, idBool aErrorIgnore)
{
    mode_t sMode;

    sMode = getModeFromString(aPerm);

    changeFileSeparator(aDestDir);

    if (idlOS::mkdir(aDestDir, sMode) != 0)
    {
        SChar const *sErrMsg = "Can't Make Directory [%s] (errno=%"ID_UINT32_FMT") : \n"
                         "Check : already exist? or have a permission?\n";

        if (aErrorIgnore == ID_TRUE)
        {
            if( gQuiet != ID_TRUE )
            {
                idlOS::fprintf(stderr, sErrMsg, aDestDir, errno);
            }
        }
        else
        {
            // stop here!!
            altiPkgError(sErrMsg, aDestDir, errno);
        }
    }
}

void actDirectory(SChar *aMsg,
                  SChar *aSrcDir,  // 액션 함수
                  SChar *aSrcFile,
                  SChar *aDestDir,
                  SChar */*aDestFile*/,
                  SChar *aFilePerm,
                  idBool aSkipError,
                  extAction */*aAction*/)
{
    SChar sDestBuf[2048];

    /* ------------------------------------------------
     *  Report 생성
     * ----------------------------------------------*/

    reportMsg("# %s\n", aMsg);

    idlOS::memset(sDestBuf, 0, ID_SIZEOF(sDestBuf));

    idlOS::sprintf(sDestBuf, "%s%s%s",
                   gPkgRootDir,
                   IDL_FILE_SEPARATORS,
                   aDestDir);

    makeDirectory(sDestBuf, aFilePerm, aSkipError);

    if( gQuiet != ID_TRUE )
    {
        idlOS::fprintf(stdout, "Distributing : From (%s/%s)\n"
                       "               To   (%s) ...",
                       aSrcDir, aSrcFile, sDestBuf);
    }

    /* ------------------------------------------------
     *  Report 생성
     * ----------------------------------------------*/
    {
        reportMsg("# Directory CREATED : %s \n", sDestBuf);

        changeFileSeparator(aDestDir);

        reportMsg("%s\n", aDestDir);
        reportMsg("0\n\n");

    }

    debugMsg("################### actDirectory [%s] \n", sDestBuf);
    if( gQuiet != ID_TRUE )
    {
        idlOS::fprintf(stdout, "Done.\n\n");
    }
}

/* ------------------------------------------------
 * prefix & postfix 처리
 * ----------------------------------------------*/

#define COPY_BUFFER_SIZE (4 * 1024)

idBool copyFile(SChar *aSrcBuf, SChar *aDestBuf, SChar *aFilePerm, idBool aSkipError)
{
    PDL_HANDLE sCreateFd = IDL_INVALID_HANDLE;
    PDL_HANDLE sSourceFd = IDL_INVALID_HANDLE;

    UChar     *sCopyBuf;
    ssize_t    sReadSize;
    ULong      sOffset;

    debugMsg("copyFile [%s] -> [%s]\n", aSrcBuf, aDestBuf);

    changeFileSeparator(aSrcBuf);
    changeFileSeparator(aDestBuf);

    if (idlOS::strcmp(aSrcBuf, aDestBuf) == 0)
    {
        /* ------------------------------------------------
         *  Same Path?? : ok skip it
         * ----------------------------------------------*/
        return ID_TRUE;
    }
    sCopyBuf = (UChar *)idlOS::malloc(COPY_BUFFER_SIZE);

    if (sCopyBuf == NULL)
    {
        altiPkgError("Memory Allocation Error [size=%"ID_UINT32_FMT"] (errno=%"ID_UINT32_FMT")\n",
                     COPY_BUFFER_SIZE, errno);
    }

    /* ------------------------------------------------
     *  1. Open/Create file
     * ----------------------------------------------*/

    sSourceFd = idlOS::open(aSrcBuf, O_RDONLY );

    if (sSourceFd == IDL_INVALID_HANDLE )
    {
        if (aSkipError == ID_TRUE)
        {
            reportMsg("# Copy Ignored : No File (%s)\n", aSrcBuf);
            goto copy_error;
        }
        else
        {
            altiPkgError("Can't Open File For Dist [%s] (errno=%"ID_UINT32_FMT")\n",
                         aSrcBuf, errno);
        }
    }

    sCreateFd = idlOS::creat( aDestBuf, getModeFromString(aFilePerm));

    if (sCreateFd == IDL_INVALID_HANDLE )
    {
        altiPkgError("Can't Create File For Dist [%s] (errno=%"ID_UINT32_FMT")\n",
                     aDestBuf, errno);
    }
    /* ------------------------------------------------
     * 2. Copy File
     * ----------------------------------------------*/

    sOffset = 0;

    while(1)
    {
        sReadSize = idlOS::pread(sSourceFd, sCopyBuf, COPY_BUFFER_SIZE, sOffset);

        if (sReadSize < 0)
        {
            altiPkgError("Error in Read File  [%s] (errno=%"ID_UINT32_FMT")\n",
                         aSrcBuf, errno);
        }
        if (idlOS::pwrite(sCreateFd, sCopyBuf, sReadSize, sOffset) != sReadSize)
        {
            altiPkgError("Error in Write File  [%s] (errno=%"ID_UINT32_FMT")\n",
                         aDestBuf, errno);
        }
        if(sReadSize < COPY_BUFFER_SIZE)
        {
            break;
        }

        sOffset += sReadSize;
    }

    /* ------------------------------------------------
     * 3. Close File
     * ----------------------------------------------*/

    (void)idlOS::close(sSourceFd);
    (void)idlOS::close(sCreateFd);

    return ID_TRUE;

  copy_error:
    return ID_FALSE;


}


/* ------------------------------------------------
 * Compare File
 *
 *  0. same file?
 *  1. content compare
 *  2. return value;
 * ----------------------------------------------*/

idBool compareFile(SChar *aSrcBuf, SChar *aDestBuf)
{
    PDL_HANDLE sDestFd = IDL_INVALID_HANDLE;
    PDL_HANDLE sSourceFd = IDL_INVALID_HANDLE;

    UChar     *sCopyBuf1;
    UChar     *sCopyBuf2;
    ssize_t    sReadSize1;
    ssize_t    sReadSize2;
    ULong      sOffset1;
    ULong      sOffset2;
    idBool     sIsSame = ID_FALSE;

    debugMsg("copyFile [%s] -> [%s]\n", aSrcBuf, aDestBuf);

    changeFileSeparator(aSrcBuf);
    changeFileSeparator(aDestBuf);

    /* ------------------------------------------------
     * 0. same file check!
     * ----------------------------------------------*/
    
    if (idlOS::strcmp(aSrcBuf, aDestBuf) == 0)
    {
        return ID_TRUE;
    }

    /* ------------------------------------------------
     *  1. Content compare
     * ----------------------------------------------*/

    sCopyBuf1 = (UChar *)idlOS::malloc(COPY_BUFFER_SIZE);
    sCopyBuf2 = (UChar *)idlOS::malloc(COPY_BUFFER_SIZE);

    if ( (sCopyBuf1 == NULL) || (sCopyBuf2 == NULL))
    {
        altiPkgError("Memory Allocation Error [size=%"ID_UINT32_FMT"] (errno=%"ID_UINT32_FMT")\n",
                     COPY_BUFFER_SIZE, errno);
    }

    sSourceFd = idlOS::open(aSrcBuf, O_RDONLY );

    if (sSourceFd == IDL_INVALID_HANDLE )
    {
        altiPkgError("Can't Open File For Dist [%s] (errno=%"ID_UINT32_FMT")\n",
                     aSrcBuf, errno);
    }
    
    sDestFd = idlOS::open(aDestBuf, O_RDONLY );

    if (sDestFd == IDL_INVALID_HANDLE )
    {
        altiPkgError("Can't Open File For Dist [%s] (errno=%"ID_UINT32_FMT")\n",
                     aDestBuf, errno);
    }

    sOffset1 = 0;
    sOffset2 = 0;

    while(1)
    {
        sReadSize1 = idlOS::pread(sSourceFd, sCopyBuf1, COPY_BUFFER_SIZE, sOffset1);
        sReadSize2 = idlOS::pread(sDestFd,   sCopyBuf2, COPY_BUFFER_SIZE, sOffset2);

        if ( (sReadSize1 < 0) || (sReadSize2 < 0))
        {
            altiPkgError("Error in Read File  [%s] (errno=%"ID_UINT32_FMT")\n",
                         aSrcBuf, errno);
        }

        if ( sReadSize1 != sReadSize2 )
        {
            break;
        }

        if (idlOS::memcmp(sCopyBuf1, sCopyBuf2, sReadSize1) != 0)
        {
            break;
        }
        
        if( (sReadSize1 < COPY_BUFFER_SIZE) || (sReadSize2 < COPY_BUFFER_SIZE) )
        {
            sIsSame = ID_TRUE;
            break;
        }

        sOffset1 += sReadSize1;
        sOffset2 += sReadSize2;
    }

    /* ------------------------------------------------
     * 3. Close File
     * ----------------------------------------------*/

    (void)idlOS::close(sSourceFd);
    (void)idlOS::close(sDestFd);

    idlOS::free(sCopyBuf1);
    idlOS::free(sCopyBuf2);

    return sIsSame;
}


void actExtension(SChar *aMsg,
                  SChar *aSrcDir,  // 액션 함수
                  SChar *aSrcFile,
                  SChar *aDestDir,
                  SChar *aDestFile,
                  SChar *aFilePerm,
                  idBool aSkipError,
                  extAction *aAction)
{
    SChar sSrcBuf[2048];
    SChar sDestBuf[2048];
    SChar sPatchBuf[2048];
    idBool sDoCopy = ID_FALSE;
    
    idlOS::memset(sSrcBuf, 0, ID_SIZEOF(sSrcBuf));
    idlOS::memset(sDestBuf, 0, ID_SIZEOF(sDestBuf));
    idlOS::memset(sPatchBuf, 0, ID_SIZEOF(sPatchBuf));

    /* ------------------------------------------------
     * Source File Path Gen
     * ----------------------------------------------*/
    if (aSrcDir[0] == '/')
    {
        // absolute path control
        idlOS::sprintf(sSrcBuf, "%s%s%s%s%s",
                       aSrcDir,
                       IDL_FILE_SEPARATORS,
                       gPkgArg[aAction->mPrefix],
                       aSrcFile,
                       gPkgArg[aAction->mPostfix]);
    }
    else
    {
        /*
         * Dereference $ALTIBASE_DEV.  This helps altipkg to detect if
         * two different path strings -- one using symlink and the
         * other, physical path -- are pointing at the same location.
         */
        SChar *sEnvPath = idlOS::getenv(ALTIBASE_ENV_PREFIX"DEV");
        SChar  sRealPath[ACP_PATH_MAX_LENGTH];
        idlOS::memset(sRealPath, '\0', ID_SIZEOF(sRealPath));
        if (!idlOS::realpath(sEnvPath, sRealPath))
        {
            altiPkgError("Cannot obtain absolute path: %s", sEnvPath);
        }
        else
        {
            /* do nothing */
        }

        // relative with $ALTIBASE_DEV
        idlOS::sprintf(sSrcBuf, "%s%s%s%s%s%s%s",
                       sRealPath,
                       IDL_FILE_SEPARATORS,
                       aSrcDir,
                       IDL_FILE_SEPARATORS,
                       gPkgArg[aAction->mPrefix],
                       aSrcFile,
                       gPkgArg[aAction->mPostfix]);
    }

    /* ------------------------------------------------
     * Target File Path Gen
     * ----------------------------------------------*/
    idlOS::sprintf(sDestBuf, "%s%s%s%s%s%s%s",
                   gPkgRootDir,
                   IDL_FILE_SEPARATORS,
                   aDestDir,
                   IDL_FILE_SEPARATORS,
                   gPkgArg[aAction->mPrefix],
                   aDestFile,
                   gPkgArg[aAction->mPostfix] );

    /* ------------------------------------------------
     *  Patch Compare
     *  1. patch file path generation
     *  2. compare source file to patch file
     *  3. If same, skip!
     *  4. not same, do copy!
     * ----------------------------------------------*/

    if (gPatchOrgDir != NULL)
    {

        if (aSrcDir[0] == '/')
        {
            // absolute path control
            idlOS::sprintf(sPatchBuf, "%s%s%s%s%s",
                           aSrcDir,
                           IDL_FILE_SEPARATORS,
                           gPkgArg[aAction->mPrefix],
                           aSrcFile,
                           gPkgArg[aAction->mPostfix]);
        }
        else
        {
            // relative with $ALTIBASE_DEV
            idlOS::sprintf(sPatchBuf, "%s%s%s%s%s%s%s",
                           gPatchOrgDir,
                           IDL_FILE_SEPARATORS,
                           aDestDir,
                           IDL_FILE_SEPARATORS,
                           gPkgArg[aAction->mPrefix],
                           aDestFile,
                           gPkgArg[aAction->mPostfix]);
        }

/*        
        fprintf(stderr, "source info : %s\n", sSrcBuf);
        fprintf(stderr, "patch info : %s\n", sPatchBuf);
        fprintf(stderr, "dest info : %s\n", sDestBuf);
*/
        if (compareFile(sSrcBuf, sPatchBuf) == ID_TRUE)
        {
            /* file is same. we don't have to distribute. */
            sDoCopy = ID_FALSE;
        }
        else
        {
            /* different : distribute */
            sDoCopy = ID_TRUE;
        }
    }
    else
    {
        sDoCopy = ID_TRUE;
    }
    
    
    /* ------------------------------------------------
     *  Copy it!
     * ----------------------------------------------*/
    if (sDoCopy == ID_TRUE)
    {
        
        /* ------------------------------------------------
         *  Report 생성
         * ----------------------------------------------*/
        
        reportMsg("# %s\n", aMsg);
        
        if( gQuiet != ID_TRUE )
        {
            idlOS::fprintf(stdout, "Distributing : From (%s)\n"
                           "               To   (%s) ...",
                           sSrcBuf, sDestBuf);
        }

        if (copyFile(sSrcBuf, sDestBuf, aFilePerm, aSkipError) == ID_TRUE)
        {
            /* ------------------------------------------------
             *  Report 생성
             * ----------------------------------------------*/
            {
                reportMsg("# COPYED  : From %s \n", sSrcBuf);
                reportMsg("#         : To   %s \n", sDestBuf);

                // 바뀐 부분만
                idlOS::sprintf(sDestBuf, "%s%s%s%s%s",
                               aDestDir,
                               IDL_FILE_SEPARATORS,
                               gPkgArg[aAction->mPrefix],
                               aDestFile,
                               gPkgArg[aAction->mPostfix] );

                changeFileSeparator(sDestBuf);

                reportMsg("%s\n", sDestBuf);
                if( gErrorIgnore == ID_TRUE )
                {
                    // nothing to do
                }
                else
                {
                    reportMsg("%"ID_UINT64_FMT"\n\n", getFileSize(sSrcBuf));
                }
            }
        }
        else
        {
            // message printed..in copyFile
        }
        if( gQuiet != ID_TRUE )
        {
            idlOS::fprintf(stdout, "Done.\n\n");
        }
    }
    else
    {
        // skip!
    }
}

extAction gAction[] =
{
    { (SChar *)"DIR", actDirectory, ALTI_PKG_ARG_EMPTY_STRING, ALTI_PKG_ARG_EMPTY_STRING },
    { (SChar *)"EXE", actExtension, ALTI_PKG_ARG_EMPTY_STRING, ALTI_PKG_ARG_EXEEXT  },
    { (SChar *)"OBJ", actExtension, ALTI_PKG_ARG_EMPTY_STRING, ALTI_PKG_ARG_OBJEXT  },
    { (SChar *)"ARL", actExtension, ALTI_PKG_ARG_LIBPRE,       ALTI_PKG_ARG_LIBEXT  },
    { (SChar *)"SHL", actExtension, ALTI_PKG_ARG_LIBPRE,       ALTI_PKG_ARG_SOEXT  },
    { (SChar *)"*",   actExtension, ALTI_PKG_ARG_EMPTY_STRING, ALTI_PKG_ARG_EMPTY_STRING  },

    { NULL, NULL, ALTI_PKG_ARG_EMPTY_STRING, ALTI_PKG_ARG_EMPTY_STRING }
};


idBool doDistFile(SChar *aOrgBuf,
                  SChar *aSrcDir,
                  SChar *aSrcFile,
                  SChar *aDestDir,
                  SChar *aDestFile,
                  SChar *aFilePerm,
                  SChar *aExtType)
{
    UInt i;
    idBool sFound  = ID_FALSE;
    idBool sIgnore = ID_FALSE;

    debugMsg("doDistFile(%s, %s, %s, %s, %s, %s)\n",
             aSrcDir, aSrcFile, aDestDir, aDestFile, aFilePerm, aExtType);

    /* ------------------------------------------------
     *  EXT에 따른 동작 설계
     *  - DIR : 디렉토리 생성
     *  - EXE, OBJ, ARL, SHL : Prefix 및 확장자 처리
     *  - ! 가 존재하면, 없더라도 무시하고, 진행.
     * ----------------------------------------------*/
    if ( *aExtType == '!')
    {
        aExtType++;
        sIgnore = ID_TRUE;
    }

    for (i = 0; gAction[i].mExtSym != NULL; i++)
    {
        if (idlOS::strcmp(aExtType, gAction[i].mExtSym) == 0)
        { //gErrorIgnore == ID_FALSE
            (*gAction[i].mFunc)(aOrgBuf,
                                aSrcDir,
                                aSrcFile,
                                aDestDir,
                                aDestFile,
                                aFilePerm,
                                ((gErrorIgnore == ID_TRUE || sIgnore == ID_TRUE) ? ID_TRUE : ID_FALSE),
                                &gAction[i]);
            sFound = ID_TRUE;
            break;
        }
    }
    if (sFound == ID_FALSE)
    {
        altiPkgError("Can't Find Ext Action [%s]\n", aExtType);
    }

    return ID_TRUE;
}

void doDistribution(idBool aQuiet)
{
    /* ------------------------------------------------
     *  - mapfile open
     *  - file read
     *  - property 처리 | true & false
     *  - true
     *    + EXT-action : file name generation
     *    + COPY action if needed.
     *    + info 화일 생성 처리
     *
     * ----------------------------------------------*/
    FILE *sMapFP;
    SChar sBuffer[4096];

    PDL_UNUSED_ARG( aQuiet );

    sMapFP = idlOS::fopen(gPkgMapFile, "r");

    if (sMapFP == NULL)
    {
        idlOS::printf("Can't open file %s\n", sMapFP);
        idlOS::exit(-1);
    }

    while(!feof(sMapFP))
    {
        idlOS::memset(sBuffer, 0, ID_SIZEOF(sBuffer));
        if (idlOS::fgets(sBuffer, ID_SIZEOF(sBuffer), sMapFP) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }

        /* ------------------------------------------------
         * 정식 헤더 [] 일 경우
         * ----------------------------------------------*/
        if (getObjectHeader(sBuffer) != 0)
        {
            UInt i;
            SChar sFilePerm[8];
            SChar sExtType[8];
            SChar sDistInfo[4][4096];
            SChar *sBufCur = sBuffer;

            idlOS::memset(sDistInfo, 0, ID_SIZEOF(sDistInfo));

            /* ------------------------------------------------
             *  1. Src/Dest 화일 정보 읽기
             * ----------------------------------------------*/
            for (i = 0; i < 4; i++)
            {
                if (fgets(sDistInfo[i], 4096, sMapFP) == NULL)
                {
                    altiPkgError("Can't read File Info =>%"ID_UINT32_FMT"\n", i);
                }
                eraseWhiteSpace(sDistInfo[i]);
                debugMsg("READ FILEINFO : [%"ID_UINT32_FMT"][%s]\n", i, sDistInfo[i]);
            }

            /* ------------------------------------------------
             *  2. True/False 체크
             *
             *   -  5개의 항목에 대해 수행
             *    type       arg      argcount
             * -------------------------------
             *   platform : 0,1,2,3     4
             *   edition  :   4         1
             *   devtype  :   5         1
             *   bittype  :   6         1
             *   comptype :   7         1
             *
             * ----------------------------------------------*/

            // Table로 처리할 수 있으나, 가독성을 염려하여 그냥 풀어서 구현함.
            // Platform Check
            if (checkValidity(&sBufCur, 0, 3) != ID_TRUE)
            {
                debugMsg("******** FAILURE ****** %s\n", sBuffer);
                continue;
            }

            // Edition Check
            if (checkValidity(&sBufCur, 4, 4) != ID_TRUE)
            {
                debugMsg("******** FAILURE ****** %s\n", sBuffer);
                continue;
            }

            // Devtype Check
            if (checkValidity(&sBufCur, 5, 5) != ID_TRUE)
            {
                debugMsg("******** FAILURE ****** %s\n", sBuffer);
                continue;
            }

            // BitType Check
            if (checkValidity(&sBufCur, 6, 6) != ID_TRUE)
            {
                debugMsg("******** FAILURE ****** %s\n", sBuffer);
                continue;
            }

            // Comptype Check
            if (checkValidity(&sBufCur, 7, 7) != ID_TRUE)
            {
                debugMsg("******** FAILURE ****** %s\n", sBuffer);
                continue;
            }

            debugMsg("******** OK VALIDITY SUCCESS ****** %s\n", sBuffer);
            /* ------------------------------------------------
             *  3. Mod 얻기
             * ----------------------------------------------*/
            {
                idBool sTmpEndFlag;

                idlOS::memset(sFilePerm, 0, ID_SIZEOF(sFilePerm));

                sBufCur = getObjectInMember(sBufCur + 1, sFilePerm, &sTmpEndFlag);
                debugMsg("Get Perm (%s) (%s)\n", sFilePerm, sBufCur);
            }


            /* ------------------------------------------------
             *  4. EXTTYPE 얻기
             * ----------------------------------------------*/
            {
                idBool sTmpEndFlag;

                idlOS::memset(sExtType, 0, ID_SIZEOF(sExtType));

                sBufCur = getObjectInMember(sBufCur + 1, sExtType, &sTmpEndFlag);
                debugMsg("Get Ext (%s) (%s)\n", sExtType, sBufCur);
            }

            if (doDistFile(sBuffer,
                           sDistInfo[0],
                           sDistInfo[1],
                           sDistInfo[2],
                           sDistInfo[3],
                           sFilePerm,
                           sExtType) != ID_TRUE)
            {
                altiPkgError("Copy Error \n");
            }
        }
    }
    idlOS::fprintf(stdout, "SUCCESS of Distribution. \n");
    idlOS::fprintf(stdout, "Check Report File.(%s) \n", gReportFile);

    // fix BUG-25556 : [codeSonar] fclose 추가.
    idlOS::fclose(sMapFP);
}
