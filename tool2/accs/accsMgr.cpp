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
#include <accsMgr.h>


accsMgr::accsMgr()
{
    curLine_ = 1;
    curCol_  = 1;
}

SInt accsMgr::doIt(SChar *filename)
{
    FILE *fp;

    fp = idlOS::fopen(filename, "r");
    IDE_TEST_RAISE(fp == NULL, fopen_error);
    setLexStdin(fp);

    outFp_ = idlOS::fopen(filename, "r");

    outOneLine(); // 첫번째 라인을 출력
    IDE_TEST_RAISE(doParse() != IDE_SUCCESS, get_parse_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(fopen_error);
    {
        idlOS::printf("open file error[%s]\n", filename);
    }
    IDE_EXCEPTION(get_parse_error);
    {
        idlOS::printf("parse error in parser\n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void accsMgr::setAccsMode(ACCS_MODE mode)
{
    mode_ = mode;
}

void accsMgr::addLine(SInt adder)
{
    curLine_ += adder;
    tokenCol_ = curCol_;
    curCol_   = 1;
    outOneLine();
}

void accsMgr::addColumn(SInt adder)
{
    tokenCol_ = curCol_;
    curCol_  += adder;
}


void accsMgr::outOneLine()
{
    SChar buffer[512];

    idlOS::memset(buffer, 0, 512);
    idlOS::fgets(buffer, 512, outFp_);
    idlOS::fprintf(stdout,"%d: %s", curLine_, buffer);
}

// -----------------------------------------------------------------------------
extern SChar *CPP_text;
void accsMgr::dump(const SChar *msg)
{
    SInt i;
    printf("[%3d:%3d]", curLine_, tokenCol_);
    for (i = 0; i < tokenCol_; i++) printf(" ");
    printf("%s<--%s\n", CPP_text, msg);
}
