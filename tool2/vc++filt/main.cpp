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
 
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
    char buf[10000];
    char buf2[10000];

    while (fgets(buf, sizeof(buf), stdin)) {
	char *p = buf;

	char last = 0;
	while (*p) {
	    int l = strcspn(p, " \n\t()\"\'");

	    if (last)
		printf("%c", last);
	    last = p[l];
	    p[l] = 0;

	    buf2[0] = 0;
	    if (p[0] && !UnDecorateSymbolName(p, buf2, sizeof(buf2), UNDNAME_COMPLETE)) {
		printf("%s", p);
	    } else
		printf("%s", buf2);
	    p += l + 1;
	}
	if (last)
	    printf("%c", last);
    }
    return 0;
}

