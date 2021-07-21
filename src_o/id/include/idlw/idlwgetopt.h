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
 
#ifndef _IDLW_GETOPT_H
#define _IDLW_GETOPT_H

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""


IDL_EXTERN_C char *optarg;
IDL_EXTERN_C int optind;
IDL_EXTERN_C int opterr;
IDL_EXTERN_C int optopt;
IDL_EXTERN_C int getopt(int nargc, char * const *nargv, const char *ostr);


#endif /* _IDLW_GETOPT_H */
