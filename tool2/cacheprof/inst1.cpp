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
 
#include <stdio.h>
#include <cmplrs/atom.inst.h>

unsigned InstrumentAll(int argc, char **argv)
{
  Obj   *o;
  Proc  *p;
  Block *b;
  Inst  *i;

  AddCallProto("Reference(VALUE, char*, long)");
  AddCallProto("Print(char *)");
  AddCallProto("PrintEnv(int, char*)");

  AddCallProgram(ProgramBefore, "PrintEnv", argc, argv[1]);
  AddCallProgram(ProgramBefore, "Print", "Prepare..\n");

  for (o = GetFirstObj(); o != NULL; o = GetNextObj(o))
    {
      if (BuildObj(o) != 0) return 1; // error 
      for (p = GetFirstProc(); p != NULL; p = GetNextProc(p))
	{
	  for (b = GetFirstBlock(p); b != NULL; b = GetNextBlock(b))
	    {
	      for (i = GetFirstInst(b); i != NULL; i = GetNextInst(i))
		{
		  if (IsInstType(i, InstTypeLoad))
		    {
		      AddCallInst(i, InstBefore, "Reference", 
				  EffAddrValue, 
				  ProcName(p), InstLineNo(i) 
				  );
		    }
		}
	    }
	}
      WriteObj(o);
    }
  AddCallProgram(ProgramAfter, "Print", "Done..\n");

  return 0;
}
