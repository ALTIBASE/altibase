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
 
/*******************************************************************************
 * $Id: 
 ******************************************************************************/

#include <acp.h>
#include <acpCStr.h>
#include <acpSys.h>
#include <acv.h>
#include <aclLic.h>
#include <aclCrypt.h>

#define ACV_INSTALL_COPYRIGHT \
    "-----------------------------------------------------------------\n"\
    "  Altibase License Install Utility\n"\
    "  Release Version 1.0.0.0\n"\
    "  Built on " __DATE__ " / " __TIME__ "\n"\
    "  Copyright 2009, ALTIBASE Corporation or its subsidiaries.\n"\
    "  All Rights Reserved.\n"\
    "-----------------------------------------------------------------\n"

#define ACV_LIC_PATH "AltiLicense.dat"
#define ACV_LINE_LENGTH 128

#define ACV_INPUT_MAX 4096

acp_char_t* gTokens[10] = 
{
    "Licensing Provider : ",
    "Licensing Product : ",
    "Type of License : ",
    "License Key : ",
    "Expiration Date : ",
    "Expiration Network Traffic : ",
    "ID : ",
    "Checksum1 : ",
    "Checksum2 : ",
    "Information : "
};

/* acvInstall -g path
 * Generate license request file */
void acvGenerateReq(acp_char_t* aFilename)
{
    acp_std_file_t sFile;
    acp_rc_t sRC;
    acp_char_t sID[ACV_LINE_LENGTH + 1] = {0, };
    
    sRC = acpSysGetHardwareID(sID, sizeof(sID) - 1);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = acpStdOpen(&sFile,
                         aFilename,
                         ACP_STD_OPEN_WRITE_TRUNC);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpPrintf("OS : %s\n", ALTI_CFG_OS);
            (void)acpPrintf("ID : %s\n", sID);
            (void)acpPrintf(
                "------------------------------------------------------------\n"
                );

            (void)acpFprintf(&sFile, "OS : %s\n", ALTI_CFG_OS);
            (void)acpFprintf(&sFile, "ID : %s\n", sID);
            (void)acpStdClose(&sFile);

            (void)acpPrintf("License request file %s generated.\n", aFilename);
            (void)acpPrintf("Please e-mail it to license@altibase.com\n");
            (void)acpPrintf("Exiting!\n");
            acpProcExit(0);
        }
        else
        {
            (void)acpPrintf("Cannot generate License request file. Exiting!\n");
            acpProcExit(0);
        }
    }
    else
    {
        (void)acpPrintf("Cannot get hardware ID! Exiting!\n");
        acpProcExit(0);
    }
}

/* Print banner */
void acvPrintBanner(void)
{
    (void)acpPrintf(ACV_INSTALL_COPYRIGHT);
}

acp_uint8_t acvConvertHexToInt(acp_uint8_t aByte)
{
    if(aByte >= '0' && aByte <= '9')
    {
        return aByte - '0';
    }
    else if(aByte >= 'A' && aByte <= 'F')
    {
        return aByte - 'A' + 10;
    }
    else if(aByte >= 'a' && aByte <= 'f')
    {
        return aByte - 'a' + 10;
    }
    else
    {
        return 0;
    }
}

void acvConvertOneHex(acp_uint8_t* aHex, acp_uint8_t* aByte)
{
    *aByte = 
        acvConvertHexToInt(aHex[0]) * 16 +
        acvConvertHexToInt(aHex[1]);
}

/* Convert Hexadecimal String into Number */
void acvConvertHex(
    acp_uint8_t* aHex, acp_uint8_t* aByte,
    acp_uint32_t aLength)
{
    acp_uint32_t i;

    for(i = 0; i < aLength; i ++)
    {
        acvConvertOneHex(aHex + i * 2, aByte + i);
    }
}

/* Install aLic onto aLibPath */
void acvInstall(acp_uint8_t* aKey, acl_lic_data_t* aLic, acp_char_t* aLibPath)
{
    acl_lic_t    sLicFile;
    acp_bool_t   sIsInstalled;
    acp_rc_t     sRC;
    acp_sint32_t sValidity;

    (void)acpPrintf("Loading License...");
            
    /* Install License */
    sRC = aclLicLoadLicense(&sLicFile, aLibPath);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Fall through */
        (void)acpPrintf("Licensing %s\n", sLicFile.mName);
        sRC = aclLicCheckValidity(&sLicFile, &sValidity);

        switch(sValidity)
        {
        case ACL_LIC_FILENOTFOUND:
            (void)acpPrintf("Creating %s\n", sLicFile.mName);
            break;
        case ACL_LIC_INVALIDFILE: /*@fallthrough@*/
        case ACL_LIC_INVALIDKEY: /*@fallthrough@*/
        case ACL_LIC_IDNOTMATCH:
            (void)acpPrintf("Updating invalid license %s.\n", sLicFile.mName);
            break;

        case ACL_LIC_VALID:
            (void)acpPrintf("%s successfully read.\n", sLicFile.mName);
            sRC = aclLicIsInstalled(&sLicFile, &sIsInstalled);

            if(ACP_RC_NOT_SUCCESS(sRC))
            {
                (void)acpPrintf(
                    "Reinstalling %s is Licensed!\n",
                    sLicFile.mName);
            }
            else
            {
                (void)acpPrintf(
                    (ACP_TRUE == sIsInstalled) ?
                    "Reinstalling License %s with ID [%s]!\n" :
                    "Installing License with ID [%s]....\n",
                    sLicFile.mName, aLic->mID);
            }
            break;

        case ACL_LIC_UNKNOWN:
            (void)acpPrintf("Unknown Error : %d\n", (acp_sint32_t)sRC);
            acpProcExit(0);
            break;
        }
    }
    else
    {
        (void)acpPrintf("Loading %s Failed!\n", sLicFile.mName);

        if(ACP_RC_IS_ENOENT(sRC))
        {
            (void)acpPrintf("Creating %s\n", sLicFile.mName);
        }
        else
        {
            (void)acpPrintf("Unknown Error : %d\n", (acp_sint32_t)sRC);
            acpProcExit(0);
        }
    }

    sRC = aclLicInstall(&sLicFile, aKey, ACL_LIC_KEYLENGTH,
                        aLic, ACL_LIC_DATALENGTH);
    if(ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpPrintf("Cannot Install License to %s!\n",
                         sLicFile.mName);
        acpProcExit(0);
    }
    else
    {
        (void)acpPrintf("License Successfully Installed to %s!\n",
                        sLicFile.mName);
    }

    (void)acpPrintf("Verifying %s....\n", sLicFile.mName);

    sRC = aclLicLoadLicense(&sLicFile, aLibPath);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acp_bool_t sIsExpired;
        sRC = aclLicCheckValidity(&sLicFile, &sValidity);
        sRC = aclLicIsInstalled(&sLicFile, &sIsInstalled);
        sRC = aclLicIsExpired(&sLicFile, &sIsExpired);

        if((ACP_TRUE == sIsInstalled) && (ACP_FALSE == sIsExpired) &&
           (ACL_LIC_VALID == sValidity))
        {
            (void)acpPrintf("Done!\n");
        }
        else
        {
            (void)acpPrintf("Failed! Please Re-install license.\n");
        }
    }
    else
    {
        (void)acpPrintf("Failed! : %d\n", (acp_sint32_t)sRC);
    }
}

/* Parse licreq.dat file */
void acvParseDatFile(acp_std_file_t* aFile,
                     acp_uint8_t* aKey,
                     acl_lic_data_t* aLic)
{
    acp_char_t sLine[ACV_INPUT_MAX];
    acp_rc_t sRC;
    acp_bool_t sEOF;
    acp_uint8_t sLicInfo[ACL_LIC_DATALENGTH];
    acp_sint32_t sLen3 = (acp_sint32_t)acpCStrLen(gTokens[3], ACV_LINE_LENGTH);
    acp_sint32_t sLen9 = (acp_sint32_t)acpCStrLen(gTokens[9], ACV_LINE_LENGTH);

    do
    {
        /* Parse dat file */
        sRC = acpStdGetCString(aFile, sLine, ACV_INPUT_MAX);

        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            (void)acpPrintf("Error processing License File : %d.\n", sRC);
            acpProcExit(0);
        }
        else
        {
            /* Fall through */
        }

        /* Check EOF */
        sRC = acpStdIsEOF(aFile, &sEOF);
        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            (void)acpPrintf("Cannot Find End of License File. : %d\n", sRC);
            acpProcExit(0);
        }
        else
        {
            /* Fall through */
        }

        /* End of file reached */
        if(ACP_TRUE == sEOF)
        {
            break;
        }
        else
        {
            /* fall through */
        }

        /* Drop trailing enter */
        acvDropEnter(sLine, ACV_INPUT_MAX);
        /* Get token of line */
        if(0 == acpCStrCmp(sLine, gTokens[3], sLen3))
        {
            /* Key String : 32bytes long */
            acvConvertHex((acp_uint8_t*)sLine + sLen3, aKey, ACL_LIC_KEYLENGTH);
        }
        else if(0 == acpCStrCmp(sLine, gTokens[9], sLen9))
        {
            /* Encrypted Data String : 2048bytes long */
            acvConvertHex((acp_uint8_t*)sLine + sLen9, sLicInfo,
                          ACL_LIC_DATALENGTH);
            (void)aclCryptTEADecipher(sLicInfo, aKey, aLic, 
                                      ACL_LIC_DATALENGTH,
                                      ACL_LIC_KEYLENGTH);
        }
        else
        {
            /* Do nothing */
        }
    } while(ACP_RC_IS_SUCCESS(sRC));

    /* Close request file */
    (void)acpStdClose(aFile);
}

/* $ acvInstall path1 path2
 * read path1 and generate path2 */
void acvInstallWithFile(acp_char_t* aDatPath, acp_char_t* aLibPath)
{
    acp_rc_t sRC;
    acp_std_file_t sFile;

    acp_uint8_t sKey[ACL_LIC_KEYLENGTH];
    acp_bool_t sMatch = ACP_FALSE;
    acp_bool_t sMatch1;
    acp_bool_t sMatch2;
    
    acl_lic_data_t sLic;

    /* Open request file and parse it */
    (void)acpPrintf("Reading %s...\n", aDatPath);
    sRC = acpStdOpen(&sFile, aDatPath, "r");

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acp_char_t sID[ACV_LINE_LENGTH + 1] = {0, };
        
        acpMemSet(&sLic, 0, sizeof(sLic));
        sRC = acpSysGetHardwareID(sID, sizeof(sID) - 1);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            acpMemSet(sKey, 0, ACL_LIC_KEYLENGTH);
            acvParseDatFile(&sFile, sKey, &sLic);
        }
        else
        {
            (void)acpPrintf("Cannot get your machine ID. Exiting!\n");
            acpProcExit(0);
        }

        /* Key and Hardware ID matching */
        sMatch1 = (0 == acpMemCmp(sKey, sLic.mKey, ACL_LIC_KEYLENGTH))?
            ACP_TRUE : ACP_FALSE;
        if(ACP_RC_IS_SUCCESS(aclLicCheckHardwareID(&sLic, &sMatch2)))
        {
            /* Do nothing */
        }
        else
        {
            (void)acpPrintf("Cannot get your Hardware ID! Exiting...\n");
            acpProcExit(0);
        }

        sMatch = 
            ((ACP_TRUE == sMatch1) && 
             (ACP_TRUE == sMatch2))? ACP_TRUE : ACP_FALSE;

        if(ACP_TRUE == sMatch)
        {
            (void)acpPrintf("Authorized License!\n");
            acvInstall(sKey, &sLic, aLibPath);
        }
        else
        {
            (void)acpPrintf("Unauthorized License!\n");
        }
    }
    else
    {
        (void)acpPrintf("Cannot open %s.(%d) Exiting!\n", aDatPath, sRC);
        acpProcExit(0);
    }
}

/* acvInstall -s
 * Get every information with console */
void acvInstallWithStdin(void)
{
    acp_rc_t sRC;
    acp_char_t sLibPath[ACP_PATH_MAX_LENGTH + 1] = {0, };
    acp_char_t sLine[ACV_INPUT_MAX];

    acp_uint8_t sKey[ACL_LIC_KEYLENGTH];
    acp_uint8_t sEncLic[ACL_LIC_DATALENGTH];
    acl_lic_data_t sLic;

    acp_bool_t sMatch = ACP_FALSE;
    acp_bool_t sMatch1;
    acp_bool_t sMatch2;

    acp_char_t sID[ACV_LINE_LENGTH + 1] = {0, };
    

    sRC = acpSysGetHardwareID(sID, sizeof(sID) - 1);
    
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Fall through */
    }
    else
    {
        (void)acpPrintf("Cannot get your machine ID. Exiting!\n");
        acpProcExit(0);
    }

    /* Get Licensing Path from stdin */
    (void)acpPrintf("Enter the path to license : [" ACV_LIC_PATH "] ");
    sRC = acpStdGetCString(ACP_STD_IN, sLibPath, ACV_LINE_LENGTH);
            
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acvDropEnter(sLibPath, ACV_LINE_LENGTH);

        if(0 == acpCStrLen(sLibPath, 5))
        {
            (void)acpCStrCpy(sLibPath, sizeof(sLibPath), ACV_LIC_PATH, 32);
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        (void)acpPrintf("Unrecoverble Error!!\n");
        acpProcExit(0);
    }

    /* Get License Key and Information */
    (void)acpPrintf("Enter License Key : ");
    sRC = acpStdGetCString(ACP_STD_IN, sLine, ACV_LINE_LENGTH);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acvConvertHex((acp_uint8_t*)sLine, sKey,
                      (acp_uint32_t)ACL_LIC_KEYLENGTH);
    }
    else
    {
        (void)acpPrintf("Unrecoverble Error!!\n");
        acpProcExit(0);
    }

    /* Enter 2048 bytes long license information */
    (void)acpPrintf("Enter License Information : ");
    sRC = acpStdGetCString(ACP_STD_IN, sLine, ACV_INPUT_MAX);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        acvConvertHex((acp_uint8_t*)sLine, sEncLic,
                      (acp_uint32_t)ACL_LIC_DATALENGTH);
    }
    else
    {
        (void)acpPrintf("Unrecoverble Error!!\n");
        acpProcExit(0);
    }

    (void)aclCryptTEADecipher(sEncLic, sKey, &sLic,
                              ACL_LIC_DATALENGTH,
                              ACL_LIC_KEYLENGTH);
                              
    /* Key Integrity */
    sMatch1 = (0 == acpMemCmp(sKey, sLic.mKey, ACL_LIC_KEYLENGTH))?
        ACP_TRUE : ACP_FALSE;
    /* Hardware ID Integrity */
    if(ACP_RC_IS_SUCCESS(aclLicCheckHardwareID(&sLic, &sMatch2)))
    {
        /* Do nothing */
    }
    else
    {
        (void)acpPrintf("Cannot get your Hardware ID! Exiting...\n");
        acpProcExit(0);
    }
    sMatch = 
        ((ACP_TRUE == sMatch1) && 
         (ACP_TRUE == sMatch2))? ACP_TRUE : ACP_FALSE;

    if(ACP_TRUE == sMatch)
    {
        (void)acpPrintf("Authorized License!\n");
        acvInstall(sKey, &sLic, sLibPath);
    }
    else
    {
        (void)acpPrintf("Unauthorized License!\n");
        acpProcExit(0);
    }
}

/* acvInstall -d AltiLicense.dat
 * Verify License and Display Information */
void acvVerifyWithFile(acp_char_t* aLibPath)
{
    acl_lic_t sLicFile;
    acp_bool_t sIsInstalled;
    acp_rc_t sRC;

    const acp_size_t sProductLength = sizeof(sLicFile.mLicenseInfo.mProducts);
    const acp_size_t sDateLength = sizeof(sLicFile.mLicenseInfo.mExpireDate);
    acp_char_t sDate[sizeof(sLicFile.mLicenseInfo.mExpireDate)] = {0, };
    acp_char_t sProducts[sizeof(sLicFile.mLicenseInfo.mProducts) + 1] = {0, };
    acp_sint32_t sValidity = ACL_LIC_VALID;

    (void)acpPrintf("Verifying License...\n");
            
    /* Install License */
    sRC = aclLicLoadLicense(&sLicFile, aLibPath);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = aclLicCheckValidity(&sLicFile, &sValidity);
    }
    else
    {
        (void)acpPrintf("Unknown Error : %d\n", (acp_sint32_t)sRC);
        acpProcExit(0);
    }

    /* Fall through */
    switch(sValidity)
    {
    case ACL_LIC_VALID:
        /* Exit switch and continue */
        break;
    case ACL_LIC_FILENOTFOUND:
        (void)acpPrintf("Cannot find %s\n", sLicFile.mName);
        acpProcExit(0);
        break;
    case ACL_LIC_INVALIDFILE:
        (void)acpPrintf("%s is Not a License File\n", sLicFile.mName);
        acpProcExit(0);
        break;
    case ACL_LIC_INVALIDKEY:
        (void)acpPrintf("License in %s is not valid\n", sLicFile.mName);
        acpProcExit(0);
        break;
    case ACL_LIC_IDNOTMATCH:
        (void)acpPrintf(
            "License in %s is not for this machine\n",
            sLicFile.mName);
        acpProcExit(0);
        break;
    case ACL_LIC_UNKNOWN:
        (void)acpPrintf("Unknown Error : %d\n", (acp_sint32_t)sRC);
        acpProcExit(0);
        break;
    }

    sRC = aclLicIsInstalled(&sLicFile, &sIsInstalled);
    if(ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpPrintf("Cannot Know %s is Licensed!\n", sLicFile.mName);
        acpProcExit(0);
    }
    else
    {
        if(ACP_TRUE == sIsInstalled)
        {
            /* Print license information */
            (void)acpPrintf("License Version     : %d.%d.%d.%d\n",
                            sLicFile.mLicenseInfo.mVersionMajor,
                            sLicFile.mLicenseInfo.mVersionMinor,
                            sLicFile.mLicenseInfo.mVersionPatch,
                            sLicFile.mLicenseInfo.mVersionMisc
                           );
            (void)acpPrintf("License Installed   : YES\n");
            (void)acpPrintf("License Expired     : %s\n",
                            (0 == sLicFile.mLicenseInfo.mExpired)?
                            "NO" : "YES");
            (void)aclLicGetProducts(&sLicFile, sProducts, sProductLength - 1);
            (void)acpPrintf("Licensed Product    : %s\n", sProducts);
            (void)acpPrintf("Hardware ID         : %s\n",
                            sLicFile.mLicenseInfo.mID);
            (void)acpMemCpy(sDate,
                            (acp_char_t*)sLicFile.mLicenseInfo.mExpireDate,
                            sDateLength);
            (void)acpPrintf("License Expiry Date : %8s\n", sDate);
        }
        else
        {
            (void)acpPrintf("License Installed   : NO\n");
        }
    }

}

enum
{
    ACV_OPTION_HELP = 1,
    ACV_OPTION_DISPLAY = 2,
    ACV_OPTION_GENERATE = 3,
    ACV_OPTION_LICENSE = 4,
    ACV_OPTION_DATFILE = 5
};

static acp_opt_def_t gOptDef[] =
{
    {
        ACV_OPTION_HELP,
        ACP_OPT_ARG_NOTEXIST,
        'h', "help", NULL, "Help",
        "Print This Help Screen"
    },
    {
        ACV_OPTION_DISPLAY,
        ACP_OPT_ARG_REQUIRED,
        'd', "display", NULL, "Path",
        "License File Path to Display Information"
    },
    {
        ACV_OPTION_GENERATE,
        ACP_OPT_ARG_REQUIRED,
        'g', "generate", NULL, "Path",
        "Request File Path to Generate"
    },
    {
        ACV_OPTION_LICENSE,
        ACP_OPT_ARG_REQUIRED,
        'l', "auth", NULL, "Path",
        "Authorization File Provided by Manufacturer"
    },
    {
        ACV_OPTION_DATFILE,
        ACP_OPT_ARG_REQUIRED,
        't', "license", NULL, "Path",
        "License File to Generate"
    },
    ACP_OPT_SENTINEL
};

/* Print help screen */
void acvPrintUsage(void)
{
    acp_char_t sHelp[1024] = {0, };
    
    (void)acpOptHelp(gOptDef, NULL, sHelp, sizeof(sHelp) - 1);
    (void)acpPrintf("%s\n", sHelp);
    acpProcExit(0);
}

void acvParseCommandLine(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_opt_t    sOpt;
    acp_sint32_t sValue;
    acp_rc_t     sRC;

    acp_bool_t sLicenseEntered = ACP_FALSE;
    acp_bool_t sDatEntered = ACP_FALSE;

    acp_char_t *sArg = NULL;
    acp_char_t  sError[ACV_LINE_LENGTH * 2 + 1] = {0, };
    
    acp_char_t sLicense[ACV_LINE_LENGTH * 2 + 1] = {0, };
    acp_char_t sDat[ACV_LINE_LENGTH * 2 + 1] = {0, };
                       
    sRC = acpOptInit(&sOpt, aArgc, aArgv);

    if(ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpPrintf("Internal Error!");
        acpProcExit(0);
    }
    else
    {
        /* Do nothing */
    }                      
                           
    while(ACP_RC_IS_SUCCESS(sRC = acpOptGet(
                                &sOpt, gOptDef, NULL, &sValue,
                                &sArg, sError, sizeof(sError) - 1)))
    {
        switch(sValue) 
        {              
        /* Print Help */
        case ACV_OPTION_HELP:
            acvPrintUsage();
            acpProcExit(0);
            break;     
        /* Display License */
        case ACV_OPTION_DISPLAY:
        {
            acvVerifyWithFile(sArg);
            acpProcExit(0);
            break;
        }
        /* Generate Request */
        case ACV_OPTION_GENERATE:
        {
            acvGenerateReq(sArg);
            acpProcExit(0);
            break;
        }
        /* License File */
        case ACV_OPTION_LICENSE:
            sLicenseEntered = ACP_TRUE;
            (void)acpCStrCpy(sLicense, sizeof(sLicense),
                             sArg, ACV_LINE_LENGTH * 2);
            break;     
        /* Dat File */
        case ACV_OPTION_DATFILE:
            sDatEntered = ACP_TRUE;
            (void)acpCStrCpy(sDat, sizeof(sDat), sArg, ACV_LINE_LENGTH * 2);
            break;     
        }              
    }                  
                       
    if(ACP_TRUE == sLicenseEntered)
    {                  
        if(ACP_TRUE == sDatEntered)
        {              
            acvInstallWithFile(sLicense, sDat);
        }              
        else           
        {              
            (void)acpPrintf("You must enter both License and Dat Path\n");
            acvPrintUsage();
        }              
    }                  
    else               
    {                  
        if(ACP_TRUE == sDatEntered)
        {                  
            (void)acpPrintf("You must enter both License and Dat Path\n");
            acvPrintUsage();
        }
        else               
        {
            /* Do nothing */
        }
    }

    if(ACP_RC_IS_EOF(sRC))
    {
        /* Do nothing */
    }
    else
    {
        (void)acpPrintf("Error! : %s\n", sError);
        acvPrintUsage();
    }
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acvPrintBanner();

    if(1 == aArgc)
    {
        acvInstallWithStdin();
    }
    else
    {
        acvParseCommandLine(aArgc, aArgv);
    }

    return 0;
}
