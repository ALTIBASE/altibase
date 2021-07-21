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

#ifndef _O_CM_ALL_H_
#define _O_CM_ALL_H_ 1

#include <ide.h>
#include <idl.h>
#include <idp.h>
#include <idu.h>

/*
 * CM모듈의 모든 HEADER 파일들
 *
 * CM모듈내에서는 이 HEADER 파일만 include 하면 됨
 */

#include <cmErrorCode.h>
#include <cmuIpcPath.h>
#include <cmuIPCDAPath.h>
#include <cmuVersion.h>
#include <cmuProperty.h>
#include <cmtDef.h>
#include <cmtDateTime.h>
#include <cmtInterval.h>
#include <cmtNumeric.h>
#include <cmtVariable.h>
#include <cmtVariablePrivate.h>
#include <cmtBit.h>
#include <cmtNibble.h>
#include <cmtLobLocator.h>
#include <cmtAny.h>
#include <cmtCollection.h>
#include <cmtAnyPrivate.h>
#include <cmbBlock.h>
#include <cmbBlockMarshal.h>
#include <cmbPool.h>
#include <cmbPoolPrivate.h>
#include <cmbShm.h>
#include <cmbShmIPCDA.h>
#include <cmmSession.h>
#include <cmmSessionPrivate.h>
#include <cmpHeader.h>
#include <cmnDef.h>
#include <cmnSock.h>
#include <cmnLink.h>
#include <cmnLinkListen.h>
#include <cmnLinkPrivate.h>
#include <cmnLinkPeer.h>
#include <cmnDispatcher.h>
#include <cmnDispatcherPrivate.h>
#include <cmpDefBASE.h>
#include <cmpArgBASE.h>
#include <cmpDefDB.h>
#include <cmpArgDB.h>
#include <cmpDefRP.h>
#include <cmpArgRP.h>
#include <cmpDefDK.h>
#include <cmpArgDK.h>
#include <cmpMarshal.h>
#include <cmpDef.h>
#include <cmpModule.h>
#include <cmi.h>
#include <cmxXid.h>
#include <cmnOpenssl.h>
#include <cmnIB.h>
#include <cmnSockIB.h>


#endif
