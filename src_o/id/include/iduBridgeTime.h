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
 * $Id: iduBridgeTime.h 15123 2006-02-14 03:48:25Z qa $
 **********************************************************************/
#ifndef _O_IDU_BRIDGE_TIME_H_
# define _O_IDU_BRIDGE_TIME_H_ (1)

/* iduTime객체를 iduBridge를 통해 쓸때 idvTime대신 사용할 객체 */
typedef struct iduBridgeTime 
{
    /* idvTime 타입의 인스턴스가 저장될 공간 */
    ULong mIdvTime[4]; 
} iduBridgeTime ;

#endif /* _O_IDU_BRIDGE_TIME_H_ */
