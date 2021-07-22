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
 * $Id: checkSleep.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ***********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idtBaseThread.h>
#include <iduSema.h>
#include <smDef.h>

static PDL_Time_Value mySleep;
#define LOOP  (300000)

struct timespec myTime = { 0, 50 * 1000 };

int main(int argc, char **argv)
{
  int i;

  mySleep.initialize(0, 50);

  idlOS::printf("SD_RID_BIT_SIZE         = %"ID_UINT64_FMT"\n",(ULong)SD_RID_BIT_SIZE         );
  idlOS::printf("SD_PAGE_BIT_SIZE        = %"ID_UINT64_FMT"\n",(ULong)SD_PAGE_BIT_SIZE        );
  idlOS::printf("SD_SPACE_BIT_SIZE       = %"ID_UINT64_FMT"\n",(ULong)SD_SPACE_BIT_SIZE       );
  idlOS::printf("SD_SPACE_BIT_SHIFT      = %"ID_UINT64_FMT"\n",(ULong)SD_SPACE_BIT_SHIFT      );
  idlOS::printf("SD_OFFSET_BIT_SIZE      = %"ID_UINT64_FMT"\n",(ULong)SD_OFFSET_BIT_SIZE      );
  idlOS::printf("SD_PAGE_SIZE            = %"ID_UINT64_FMT"\n",(ULong)SD_PAGE_SIZE            );
  idlOS::printf("SD_MAX_PAGE_COUNT       = %"ID_UINT64_FMT"\n",(ULong)SD_MAX_PAGE_COUNT       );
  idlOS::printf("SC_MAX_SPACE_COUNT      = %"ID_UINT64_FMT"\n",(ULong)SD_MAX_SPACE_COUNT      );
  idlOS::printf("SD_OFFSET_MASK        = %"ID_xINT64_FMT"\n",(ULong)SD_OFFSET_MASK        );
  idlOS::printf("SD_PAGE_MASK          = %"ID_xINT64_FMT"\n",(ULong)SD_PAGE_MASK          );
  idlOS::printf("SD_SPACE_MASK         = %"ID_xINT64_FMT"\n",(ULong)SD_SPACE_MASK         );

}
