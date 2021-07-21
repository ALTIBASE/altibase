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

#ifndef _O_CM_H_
#define _O_CM_H_ 1

#include <ide.h>
#include <idl.h>
#include <idu.h>


/*
 * 외부 모듈에서 CM모듈을 사용하기 위해 필요한 HEADER 파일들
 *
 * 외부 모듈에서 CM모듈을 사용할 때는 이 HEADER 파일만 #include 하면 됨
 */

#include <cmErrorCode.h>
#include <cmuIpcPath.h>
#include <cmuVersion.h>
#include <cmtDef.h>
#include <cmtDateTime.h>
#include <cmtInterval.h>
#include <cmtNumeric.h>
#include <cmtVariable.h>
#include <cmtVariablePrivate.h>
#include <cmtLobLocator.h>
#include <cmtBit.h>
#include <cmtNibble.h>
#include <cmtAny.h>
#include <cmtCollection.h>
#include <cmbBlock.h>
#include <cmbPool.h>
#include <cmmSession.h>
#include <cmpHeader.h>
#include <cmnDef.h>
#include <cmnLink.h>
#include <cmnLinkPrivate.h>
#include <cmnLinkPeer.h>
#include <cmpDefBASE.h>
#include <cmpArgBASE.h>
#include <cmpDefDB.h>
#include <cmpArgDB.h>
#include <cmpDefRP.h>
#include <cmpArgRP.h>
#include <cmpDefDK.h>
#include <cmpArgDK.h>
#include <cmpDef.h>
#include <cmi.h>
//fix BUG-17947.
#include <cmpModule.h>
#include <cmxXid.h>
#include <cmuProperty.h>


#endif
