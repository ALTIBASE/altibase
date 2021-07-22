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
 * $Id: csProto.h 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#ifndef _O_CS_PROTO_H_
#define _O_CS_PROTO_H_

/* ------------------------------------------------
 *  아래의 함수는 사용자가 프로파일링을
 *  하기 위해 설정하는 사용자 코드 중에
 *  삽입되는 함수리스트이다.
 * ----------------------------------------------*/

extern "C"  void cs_cache_clear();
extern "C"  void cs_cache_start();
extern "C"  void cs_cache_stop();
extern "C"  void cs_cache_save(char *);
extern "C"  void __cs_cache_prefetch(void *);

#define cs_cache_prefetch(a) { cs_cache_stop(); __cs_cache_prefetch( (void *)a); cs_cache_start(); }

#endif
