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
 

/*****************************************************************************
 * $Id: accs.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <idl.h>
#include <accs.h>

accsPropertyMgr accsg_property;
accsSymbolMgr accsg_symbol;
accsMgr accsg_mgr;

SChar   accsRoot[256];
SChar   ConfFile[256];
SChar   dirBuffer[256];

/*
 * 옵션 :
 * -s 화일명 : symbol table을 생성함.
 */

const SChar HelpMsg[] =
"accs [-s -h filename] \n"
"      -h [filename] :  processing File Name \n"
"      -s : generate symbol table to stdout \n";

int main(SInt argc, SChar* argv[])
{
    SInt    i;
    SInt    opr;
    SInt    option_count = 0;
    SChar   OptionFlag   = ' ';
    SChar  *filename = NULL;
    
    idBool  symbolGen = ID_FALSE;

    /* --------------------
     * [0] 환경변수 존재 검사 
     * -------------------*/
    SChar *envhome = idlOS::getenv(ENV_ACCS_HOME);
    if ( !(envhome && idlOS::strlen(envhome) > 0))
    {
        idlOS::printf("please make environment variable for [%s]\n",
                      ENV_ACCS_HOME);
        idlOS::exit(0);
    }
    
    idlOS::strncpy(accsRoot,  envhome, 255);
    idlOS::strncpy(ConfFile, DEFAULT_CONF_FILE, 255);
    
    while ( (opr = idlOS::getopt(argc, argv, "sh:d:f:")) != EOF)
    {
        switch(opr)
        {
        case 'd':  // 홈 디렉토리 명시
            idlOS::strncpy(accsRoot, optarg, 255);
            break;
        case 'f':  // Conf 화일 명시
            idlOS::strncpy(ConfFile, optarg, 255);
             break;
        case 's':
            symbolGen = ID_TRUE;
            option_count++;
            break;
        case 'h':
            option_count++;
            filename = optarg;
            break;
        }
    }

    if (filename == NULL)
    {
        idlOS::printf(HelpMsg);
        idlOS::exit(0);
    }

    idlOS::sprintf(dirBuffer, "%s%cconf%c%s", accsRoot, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, ConfFile);
    
//     idlOS::printf("filename is %s\n", filename);
//     idlOS::printf("ConfFile is %s\n", dirBuffer);

    idlOS::printf("=========== PROPERTY LOADING ========== \n");
    accsg_property.setPropertyFile(dirBuffer);
    IDE_TEST_RAISE(accsg_property.doIt() != IDE_SUCCESS, doIt_error);

    accsg_property.dump();

    for (i = 0; i < accsg_property.getPreloadCount(); i++)
    {
        if (i == 0)
        {
            idlOS::printf("=========== SYMBOL TABLE LOADING ========== \n");
        }
        idlOS::memset(dirBuffer, 0, 256);
        idlOS::sprintf(dirBuffer, "%s/symbol/%s", accsRoot, accsg_property.getPreloadItem(i));
        
        idlOS::printf("open & reading.. %s\n", dirBuffer);
        IDE_TEST_RAISE(accsg_symbol.loadSymbolFile(dirBuffer) != IDE_SUCCESS,
                       load_symbol_error);
    }
    
    accsg_symbol.dump();
    
    accsg_mgr.setAccsMode(symbolGen == ID_TRUE ? ACCS_MODE_SYMBOL_GEN : ACCS_MODE_CHECK);
    
    IDE_TEST_RAISE(accsg_mgr.doIt(filename) != IDE_SUCCESS, init_error);
    

    return 0;

    IDE_EXCEPTION(init_error);
    {
        idlOS::printf("[ERROR] ACCS INFO class initialization  \n");
    }
    IDE_EXCEPTION(doIt_error);
    {
        idlOS::printf("[ERROR] property loading\n");
    }
    IDE_EXCEPTION(load_symbol_error);
    {
        idlOS::printf("[ERROR] load symbol table file [%s]\n", dirBuffer);
    }
    IDE_EXCEPTION_END;
    return -1;
}
