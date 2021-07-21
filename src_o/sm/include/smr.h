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
 * $Id: smr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMR_H_
#define _O_SMR_H_ 1

#include <smrDef.h>
#include <smriDef.h>
#include <smrRecoveryMgr.h>
#include <smrLogMgr.h>
#include <smrBackupMgr.h>
#include <smrLogFile.h>
#include <smrLogFileMgr.h>
#include <smrLFThread.h>
#include <smrLogAnchorMgr.h>
#include <smrUTransQueue.h>
#include <smrCompareLSN.h>
#include <smrDirtyPageList.h>
#include <smrDPListMgr.h>
#include <smrUpdate.h>
#include <smrChkptThread.h>
#include <smrArchThread.h>
#include <smriChangeTrackingMgr.h>
#include <smriBackupInfoMgr.h>
#include <smrLogMultiplexThread.h>
#include <smrArchMultiplexThread.h>
#include <smrUCSNChkThread.h>   /* BUG-35392 */
#include <smrRedoLSNMgr.h>
#include <smrLogHeadI.h>
#include <smrLogFileDump.h> /* PROJ-1923 */
#include <smrRemoteLogMgr.h>
#include <smrLogComp.h>
#include <smrFT.h>
#include <smrPreReadLFileThread.h>


#endif /* _O_SMR_H_ */
