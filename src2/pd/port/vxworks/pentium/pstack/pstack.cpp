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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <envLib.h>
#include <unldLib.h>
#include <loadLib.h>
#include <ioLib.h>
#include <taskLib.h>
#include <symLib.h>
#include <memLib.h>
#include <trcLib.h>

extern SYMTAB_ID sysSymTbl;

static void 
xxxTracePrint(INSTR *caller, int func, int nargs, int *args)
{
    char buf [250];
    int ix;
    int len = 0;

    SYM_TYPE type;
    int value;
    char name[512];
    //char b[512];
    symFindByValue(sysSymTbl, (int)caller, name, &value, &type);
    //symFindByValue(sysSymTbl, (int)func, b, &value, &type);

    //len += sprintf (&buf [len], "%#10x: %#10x (", (int)caller, func);
    len += sprintf (&buf [len], "%#10x: %s", (int)caller, name);
    //for (ix = 0; ix < nargs; ix++)
    //{
    //    if (ix != 0)
    //        len += sprintf (&buf [len], ", ");
    //    len += sprintf (&buf [len], "%#x", args [ix]);
    //}

    //len += sprintf (&buf [len], ")\n");

    //ideLog(buf);
    printf( "%s\n", buf );
}

int
xxxTrace(WIND_TCB *pTcb)
{
    REG_SET regs;

    if (pTcb == NULL)
     return (ERROR);

    taskRegsGet ((int)pTcb, &regs);
    trcStack (&regs, (FUNCPTR) xxxTracePrint, (int)pTcb);

    return (OK);
}

void bt( int aTid )
{
    xxxTrace(::taskTcb(aTid));
}

void bt2( char *aName )
{
    xxxTrace(::taskTcb(taskNameToId(aName)));
}


void xTrace()
{
    xxxTrace(::taskTcb(::taskIdSelf()));
}
