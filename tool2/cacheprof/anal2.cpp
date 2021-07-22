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

extern "C" void PrintEnv(int argc, char *msg)
{
  int i;
  fprintf(stdout, "!! argc %d(%s) !! \n", argc, msg);

//   for (i = 0; i < argc; i++)
//     {
//       fprintf(stdout, "arg %d=%s\n", i, argv[i]);
//     }
}

extern "C" void Print(char *msg)
{
  fprintf(stdout, "end of profile.  bye. [%s] \n", msg);
  fflush(stdout);
}
