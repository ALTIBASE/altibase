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
 
/*
 * create by gamestar 2000/8/4
 * - config-minimal.h를 자동으로 include 하기 위해 생성.
 *   porting시에 위 화일을 include하기 위해 별도로 소스 수정이 필요 없음
 *   각 플랫폼에 따른 헤더화일은 config-lankage.h와 링크되도록 함. 
 */

#ifndef _PDL_O_CONFIG_H_
#define _PDL_O_CONFIG_H_
#ifndef _WIN32
#include "config-altibase.h"
#endif
#include "config-minimal.h"
#include "config-linkage.h"

#endif
