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
 
/* -*- C++ -*- */
// $Id: config-sunos5.10.h 80576 2017-07-21 07:07:02Z yoonhee.kim $

// The following configuration file is designed to work for SunOS 5.10
// (Solaris 10) platforms using the SunC++ 5.x (Sun Studio 8), or g++
// compilers.

#ifndef ACE_CONFIG_H

// ACE_CONFIG_H is defined by one of the following #included headers.

// #include the SunOS 5.9 config, then add any SunOS 5.10 updates below.

// FlexLexer.h에서 iostream을 include하는데,
// iostream에서 stdexcept를 include한다.
// stdexcept에서는
// _RWSTD_NO_EXCEPTIONS가 define 되어 있으면,
// _RWSTD_THROW_SPEC_NULL를 throw()로 정의하지 않는데,
// 때문에 stdexcept에서 에러가 발생한다.
// 이를 막기 위해 _RWSTD_NO_EXCEPTIONS를 undef 한다.
// _RWSTD_NO_EXCEPTIONS는 config-sunos5.5.h에서 1로 정의한다.
// PDL_HAS_EXCEPTIONS가 정의되어 있으면 _RWSTD_NO_EXCEPTIONS를 
// 정의하지 않는다. (config-sunos5.5.h에서...)
// by kumdory
//#undef _RWSTD_NO_EXCEPTIONS
#define PDL_HAS_EXCEPTIONS 1

// SES의 seshx.cpp에서 ifstream.h 대신 ifstream을 include하기 위해 정의함.
#define USE_STD_IOSTREAM   1

#include "config-sunos5.9.h"

#define ACE_HAS_SCANDIR

#endif /* ACE_CONFIG_H */


