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
 
/***********************************************************************
 * $Id$
 * $Log$
 * Revision 1.1  2002/12/17 05:43:23  jdlee
 * create
 *
 * Revision 1.1.1.1  2002/10/18 13:55:36  jdlee
 * create
 *
 * Revision 1.14  2002/02/07 05:11:04  jdlee
 * fix PR-1969
 *
 * Revision 1.13  2001/12/12 01:39:19  jdlee
 * fix
 *
 * Revision 1.12  2001/12/11 08:43:24  jdlee
 * fix
 *
 * Revision 1.11  2001/10/18 05:50:41  jdlee
 * fix PR-1570,1575
 *
 * Revision 1.10  2001/08/13 04:32:05  assam
 * PR/1411, PR/1412 수정
 *
 * Revision 1.9  2001/06/21 03:15:19  jdlee
 * fix PR-1217
 *
 * Revision 1.8  2001/05/14 11:15:08  jdlee
 * fix
 *
 * Revision 1.7  2000/10/31 13:10:27  assam
 * *** empty log message ***
 *
 * Revision 1.6  2000/10/13 10:30:48  assam
 * *** empty log message ***
 *
 * Revision 1.5  2000/10/11 10:04:03  sjkim
 * trace info added
 *
 * Revision 1.4  2000/09/26 10:44:26  jdlee
 * fix
 *
 * Revision 1.3  2000/08/30 05:38:05  sjkim
 * ida fix
 *
 * Revision 1.2  2000/06/15 05:53:09  bethy
 * idaTNumericType const value 재정의
 *
 * Revision 1.1.1.1  2000/06/07 11:44:06  jdlee
 * init
 *
 * Revision 1.1  2000/05/31 11:14:31  jdlee
 * change naming to id
 *
 * Revision 1.1  2000/05/31 07:33:56  sjkim
 * id 라이브러리 생성
 *
 * Revision 1.6  2000/04/19 11:48:26  assam
 * idaFloat,idaDouble,idaMod is fixed
 *
 * Revision 1.5  2000/03/24 01:55:50  assam
 * *** empty log message ***
 *
 * Revision 1.4  2000/03/10 07:24:07  assam
 * *** empty log message ***
 *
 * Revision 1.3  2000/02/24 07:56:40  assam
 * *** empty log message ***
 *
 * Revision 1.2  2000/02/24 01:10:38  assam
 * *** empty log message ***
 *
 * Revision 1.1  2000/02/17 03:17:31  assam
 * Migration from QP.
 *
 **********************************************************************/

#include <mtca.h>

const mtaVarcharType mtaVarcharNULL = {
    { 0, 0 } , { 0 }
};

const mtaNumericType mtaNumericNULL = {
    0,{0}
};

const mtaTNumericType mtaTNumericNULL = {
    0,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

const mtaTNumericType mtaTNumericZERO = {
    0x80,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

const mtaTNumericType mtaTNumericONE = {
    0x80+65,{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

const mtaTNumericType mtaTNumericTWELVE = {
    0x80+65,{12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
