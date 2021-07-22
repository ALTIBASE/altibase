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
 * $Id: accsMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#ifndef _O_ACCSINFO_H_
#define _O_ACCSINFO_H_ 1

#include <idl.h>


typedef enum
{
    ACCS_MODE_CHECK = 0,
    ACCS_MODE_SYMBOL_GEN /* 입력화일로 부터 출력 */ 
} ACCS_MODE;

class accsMgr
{
    // cursor info =====================================
    SInt       curLine_;
    SInt       curCol_;
    SInt       tokenCol_;
    ACCS_MODE  mode_;

    FILE      *outFp_; // 화면에 진행을 출력해 주기 위함

public:
    accsMgr();
    SInt doIt(SChar *);
    void setAccsMode(ACCS_MODE);

    // cursor info ======================================
    void addLine(SInt adder);
    void setLine(SInt linenum) { curLine_ = linenum; }
    void addColumn(SInt adder);
    void setColumn(SInt colnum) { curCol_ = colnum; }
    void dump(const SChar *msg);
    SInt getCurCol() { return curCol_; }
    SInt getTokenCol() { return tokenCol_; }
    SInt getCurLine() { return curLine_; }

    // conole output  ======================================
    void outOneLine();
    
private:
    void accsMgr::setLexStdin(FILE *fd);
    SInt doParse();           // call yacc(bison) api

};

#endif
