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
 
#include <idl.h>
#include <ideErrorMgr.h>
#include <accsPropertyMgr.h>

accsPropertyMgr::accsPropertyMgr()
{
    idlOS::memset(Filename_, 0, ACCS_CONF_FILE_LEN);
    PreloadCount_ = 0;
    PrefixCount_  = 0;
    idlOS::memset(PreloadList_[0], 0, sizeof(PreloadList_));
    idlOS::memset(PrefixList_[0], 0, sizeof(PrefixList_));
    TabSize_      = 0;
    LineWidth_    = 0;
    ColStart_     = 0;
    
}

void accsPropertyMgr::setPropertyFile(SChar *file)
{
    idlOS::strcpy(Filename_, file);
}

SInt accsPropertyMgr::doIt()
{
    FILE *fp;

    fp = idlOS::fopen(Filename_, "r");
    IDE_TEST_RAISE(fp == NULL, fopen_error);

    setLexStdin(fp);

    IDE_TEST_RAISE(doParse() != IDE_SUCCESS, get_property_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(fopen_error);
    {
        idlOS::printf("open property file error[%s]\n", Filename_);
    }
    IDE_EXCEPTION(get_property_error);
    {
        idlOS::printf("get property error in parser\n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SInt accsPropertyMgr::addPreloadItem(SChar *name)
{
    SInt i;
    for (i = 0; i < ACCS_MAX_PRELOAD_LIST; i++)
    {
        if (strcmp(PreloadList_[i], name) == 0)
        {
            return IDE_SUCCESS; // already inserted
        }
        if (PreloadList_[i][0] == 0)
        {
            idlOS::strcpy(PreloadList_[i], name);
            PreloadCount_ = i + 1;
            return IDE_SUCCESS;
        }
    }
    return IDE_FAILURE;
}

SInt accsPropertyMgr::addPrefixItem (SChar *name)
{
    SInt i;
    for (i = 0; i < ACCS_MAX_PREFIX_LIST; i++)
    {
        if (strcmp(PrefixList_[i], name) == 0)
        {
            return IDE_SUCCESS; // already inserted
        }
        if (PrefixList_[i][0] == 0)
        {
            idlOS::strcpy(PrefixList_[i], name);
            PrefixCount_ = i + 1;
            return IDE_SUCCESS;
        }
    }
    return IDE_FAILURE;
}

SChar *accsPropertyMgr::getPreloadItem(SInt number)
{
    if (number < PreloadCount_)
    {
        return PreloadList_[number];
    }
    return NULL;
}

SChar *accsPropertyMgr::getPrefixItem(SInt number)
{
    if (number < PrefixCount_)
    {
        return PrefixList_[number];
    }
    return NULL;
}

void accsPropertyMgr::dump()
{
    SInt i;
    idlOS::printf("================== DUMP OF ACCS PROPERTY ===========================\n");
    idlOS::printf("PreloadCount_ %d\n",  getPreloadCount());
    for (i = 0; i < getPreloadCount(); i++)
    {
        idlOS::printf("[%s] ", getPreloadItem(i));
    }
    idlOS::printf("\n");
    
    idlOS::printf("PrefixCount_ %d\n",   getPrefixCount() );
    for (i = 0; i < getPrefixCount(); i++)
    {
        idlOS::printf("[%s] ", getPrefixItem(i));
    }
    idlOS::printf("\n");
    
    idlOS::printf("TabSize_ %d\n",       getTabSize()     );
    idlOS::printf("LineWidth_ %d\n",     getLineWidth()   );
    idlOS::printf("ColStart_ %d\n",      getColStart()    );
    idlOS::printf("================== END OF ACCS PROPERTY ===========================\n");
}

