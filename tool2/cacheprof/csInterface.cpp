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
 * $Id: csInterface.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <cmplrs/atom.inst.h>

#include <csSimul.h>

csSimul gSimul;

extern "C" void csInit(int argc, long capacity, long linesize, long ways)
{
    int i;
    if (argc != 4)
    {
        fprintf(stderr,
                "specify [capacity, line size, ways]\n"\
                "         capacity  : 1 4 8 16 32 64 128 256 512 1024 Kbytes ..\n"\
                "         line size : 8 16 32 64 128 ..\n"\
                "         ways      : 1 2 4 8 16 ..\n" );
        exit(0);
    }
    gSimul.initialize((Address)capacity * 1024, (Address)linesize, (Address)ways);
}

extern "C" void csFinal()
{
    gSimul.Dump(stderr);
    fflush(stderr);
}

extern "C" void csSave(char *aFileName)
{
    gSimul.Save(aFileName);
}


extern "C" void csReference(unsigned long aAddr,
                            int           aWriteFlag,
                            char         *aFileName,
                            char         *aProcName,
                            long          aLine)
{
    gSimul.refer(ThreadGetId(), (Address)aAddr, aWriteFlag, aFileName, aProcName, aLine);
}

extern "C" void csClear()
{
    gSimul.Clear();
}

extern "C" void csStart()
{
    gSimul.setStartFlag(ID_TRUE);
    gSimul.setTargetThread(ThreadGetId());
}

extern "C" void csStop()
{
    gSimul.setStartFlag(ID_FALSE);
}

extern "C" void csPrefetch(char *aAddress)
{
  gSimul.setStartFlag(ID_FALSE);
  gSimul.doRefer( (Address)aAddress);
  gSimul.setStartFlag(ID_TRUE);
}

