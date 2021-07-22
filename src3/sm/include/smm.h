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
 * $Id: smm.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMM_H_
#define _O_SMM_H_ 1

#include <smDef.h>
#include <smmDef.h>
#include <smmDatabase.h>
#include <smmExpandChunk.h>
#include <smmManager.h>
#include <smmSlotList.h>
#include <smmDirtyPageMgr.h>
#include <smmFixedMemoryMgr.h>
#include <smmUpdate.h>

#include <smmTBSStartupShutdown.h>
#include <smmTBSMultiPhase.h>
#include <smmTBSChkptPath.h>
#include <smmTBSAlterAutoExtend.h>
#include <smmTBSAlterChkptPath.h>
#include <smmTBSAlterDiscard.h>
#include <smmTBSCreate.h>
#include <smmTBSDrop.h>
#include <smmTBSMediaRecovery.h>

#endif
