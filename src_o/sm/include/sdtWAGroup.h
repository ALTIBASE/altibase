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
 

/***********************************************************************
 * $Id
 *
 * Sort 및 Hash를 하는 공간으로, 최대한 동시성 처리 없이 동작하도록 구현된다.
 * 따라서 SortArea, HashArea를 아우르는 의미로 사용된다. 즉
 * SortArea, HashArea는WorkArea에 속한다.
 *
 * 원래 PrivateWorkArea를 생각했으나, 첫번째 글자인 P가 여러기지로 사용되기
 * 때문에(예-Page); WorkArea로 변경하였다.
 * 일반적으로 약자 및 Prefix로 WA를 사용하지만, 몇가지 예외가 있다.
 * WCB - BCB와 역할이 같기 때문에, 의미상 WACB보다 WCB로 수정하였다.
 * WPID - WAPID는 좀 길어서 WPID로 줄였다.
 *
 **********************************************************************/

#ifndef _O_SDT_WA_GROUP_H_
#define _O_SDT_WA_GROUP_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smiDef.h>




#endif //_O_SDT_WA_GROUP_H_
