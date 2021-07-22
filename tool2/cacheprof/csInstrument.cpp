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
 * $Id: csInstrument.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <cmplrs/atom.inst.h>

/* ------------------------------------------------
 *  csInterface의 Inteface List
 * 
 *  - csInit(int argc, long, long, long)
 *     - cache capacity  ( ...k)
 *     - cache line size (16, 32, 64, 128)
 *     - cache associate ways (1, 2, 4, 8)
 * 
 *  - csFinal()      : 종료화
 *
 *  - csReference(address, writeflag, filename, procname, line); 
 *
 *  - cs_cache_clear() : profile 내용을 지움. 
 *  - cs_cache_start() : profile 시작
 *  - cs_cache_stop()  : profile 중지
 *  - cs_cache_save()  : profile 저장
 *  - cs_cache_prefetch()  : prefetch simulating..
 *
 * ----------------------------------------------*/

void Instrument()
{
}

static const char *safe_proc_name(Proc *p)
{
    const char *	name;
    static char 	buf[1024];
    
    name = ProcName(p);
    if (name)
	return(name);
    sprintf(buf, "proc_at_0x%lx", ProcPC(p));
    return(buf);
}

unsigned InstrumentAll(int argc, char **argv)
{
    long  sProcCount = 0;
    Obj   *o;
    Proc  *p;
    Block *b;
    Inst  *i;
    unsigned long flags = THREAD_PTHREAD;
    /* ------------------------------------------------
     *  define Prototype of Interface
     * ----------------------------------------------*/
    
    AddCallProto("csInit(int, long, long, long)");
    AddCallProto("csFinal()");
    AddCallProto("csReference(VALUE, int, char*, char *, long)");
    AddCallProto("csClear()");
    AddCallProto("csStart()");
    AddCallProto("csStop()");
    AddCallProto("csSave(char *)");
    AddCallProto("csPrefetch(char *)");
    
    AddCallProgram(ProgramBefore,
                   "csInit",
                   argc,
                   atol(argv[1]),
                   atol(argv[2]),
                   atol(argv[3]));

    for (o = GetFirstObj(); o != NULL; o = GetNextObj(o))
    {
        if (BuildObj(o) != 0) return 1; // error
        
        {
            Proc *sProc;
            
            sProc = GetNamedProc("cs_cache_clear");
            if (sProc != NULL)
            {
                ReplaceProcedure(sProc, "csClear");
            }
            sProc = GetNamedProc("cs_cache_start");
            if (sProc != NULL)
            {
                ReplaceProcedure(sProc, "csStart");
            }
            sProc = GetNamedProc("cs_cache_stop");
            if (sProc != NULL)
            {
                ReplaceProcedure(sProc, "csStop");
            }
            sProc = GetNamedProc("cs_cache_save");
            if (sProc != NULL)
            {
                ReplaceProcedure(sProc, "csSave");
            }
            sProc = GetNamedProc("__cs_cache_prefetch");
            if (sProc != NULL)
            {
                ReplaceProcedure(sProc, "csPrefetch");
            }
        }
	if (GetObjInfo(o, ObjModifyHint) == OBJ_READONLY ||
	    ThreadExcludeObj(o, flags) != 0)
        {
	    continue; // skip analyze
        }
        for (p = GetFirstObjProc(o); p != NULL; p = GetNextProc(p))
	{
            if (ThreadExcludeProc(o, p, flags) != 0)
            {
                continue; // skip analyze
            }
            for (b = GetFirstBlock(p); b != NULL; b = GetNextBlock(b))
	    {
                for (i = GetFirstInst(b); i != NULL; i = GetNextInst(i))
		{
                    if (IsInstType(i, InstTypeLoad))
		    {
                        AddCallInst(i,
                                    InstBefore,
                                    "csReference", 
                                    EffAddrValue,
                                    0, // read flag
                                    ProcFileName(p),
                                    safe_proc_name(p),
                                    InstLineNo(i) 
                                    );
		    }
                    else
                    {
                        if (IsInstType(i, InstTypeStore))
                        {
                            AddCallInst(i,
                                        InstBefore,
                                        "csReference", 
                                        EffAddrValue,
                                        1, // write flag
                                        ProcFileName(p),
                                        safe_proc_name(p),
                                        InstLineNo(i) 
                                        );
                        }
                    }
		}
                sProcCount++;
	    }
	}
        WriteObj(o);
    }

    AddCallProgram(ProgramAfter, "csFinal");
    
    return 0;
}
