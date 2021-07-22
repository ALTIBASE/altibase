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
 
#include <idtBaseThread.h>
#include <ide.h>
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

#define EXIT_TASK_MAX 256

void killall()
{
    int task;
    int taskNumber;
    int taskList[EXIT_TASK_MAX];
    TASK_DESC taskData;
    taskNumber = ::taskIdListGet( taskList, sizeof(taskList)/sizeof(taskList[0]));
    for( task = 0; task < taskNumber; task++ )
    {
        if( ::taskInfoGet( taskList[task], &taskData ) == OK )
        {
            if( taskData.td_entry == (FUNCPTR)idtBaseThread::getTaskEntry() )
            {
                if( ::taskIdSelf() != taskList[task] )
                {
                    (void)::taskDeleteForce( taskList[task] );
                }
            }
        }
    }
}

static void 
pstackPrint(INSTR *caller, int func, int nargs, int *args)
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
    ideLog::logMessage(IDE_SERVER_0, "%s\n", buf );
    printf( "%s\n", buf );
}

int
pstack(WIND_TCB *pTcb)
{
    REG_SET regs;

    if (pTcb == NULL)
     return (ERROR);

    taskRegsGet ((int)pTcb, &regs);
    trcStack (&regs, (FUNCPTR) pstackPrint, (int)pTcb);

    return (OK);
}


class Pstack : public idtBaseThread
{
public:
    Pstack(WIND_TCB *aTcb) { pTcb = aTcb; };
    ~Pstack() {};
    void run();

private:
    WIND_TCB *pTcb;
};

void
Pstack::run()
{
    (void)pstack(pTcb);
}

void backtrace( WIND_TCB * aTcb )
{
    Pstack a(aTcb);
    a.start();
    idlOS::thr_join(a.getHandle(), NULL);

    killall();
}

extern "C" void test1()
{
    printf( "sizeof SLong: %d\n", sizeof(SLong) );
    printf( "sizeof ULong: %d\n", sizeof(ULong) );
    printf( "sizeof vSLong: %d\n", sizeof(vSLong) );
    printf( "sizeof vULong: %d\n", sizeof(vULong) );
}
