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
 *
 * $Id: sdpstItBMP.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Internal Bitmap 페이지 관련
 * STATIC 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <sdr.h>
# include <sdpReq.h>

# include <sdpstBMP.h>
# include <sdpstItBMP.h>
# include <sdpstLfBMP.h>
# include <sdpstRtBMP.h>
# include <sdpstAllocPage.h>
# include <sdpstDPath.h>
# include <sdpstSH.h>
# include <sdpstStackMgr.h>


sdpstBMPOps sdpstItBMP::mItBMPOps =
{
    (sdpstGetBMPHdrFunc)sdpstBMP::getHdrPtr,
    (sdpstGetSlotOrPageCntFunc)NULL,
    (sdpstGetDistInDepthFunc)sdpstStackMgr::getDistInItDepth,
    (sdpstIsCandidateChildFunc)sdpstItBMP::isCandidateLfBMP,
    (sdpstGetStartSlotOrPBSNoFunc)sdpstItBMP::getStartSlotNo,
    (sdpstGetChildCountFunc)sdpstItBMP::getSlotCnt,
    (sdpstSetCandidateArrayFunc)sdpstItBMP::setCandidateArray,
    (sdpstGetFstFreeSlotOrPBSNoFunc)sdpstBMP::getFstFreeSlotNo,
    (sdpstGetMaxCandidateCntFunc)smuProperty::getTmsCandidateLfBMPCnt,
    (sdpstUpdateBMPUntilHWMFunc)sdpstDPath::updateBMPUntilHWMInRtAndItBMP,
};

