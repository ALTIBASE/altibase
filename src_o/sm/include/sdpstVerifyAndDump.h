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
 * $Id: sdpstVerifyAndDump.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 자료구조 확인 및 출력과 관련된 STATIC
 * 인터페이스를 관리한다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_VERIFY_AND_DUMP_PAGE_H_
# define _O_SDPST_VERIFY_AND_DUMP_H_ 1

# include <sdpDef.h>

class sdpstVerifyAndDump
{
public:
    static void dump( scSpaceID    aSpaceID,
                      void       * /*aSegDesc*/,
                      idBool       /*aDisplayAll*/);

    static IDE_RC verify(idvSQL     * aStatistics,
                         scSpaceID    aSpaceID,
                         void       * /*aSegDesc*/,
                         UInt         /*aFlag*/,
                         idBool       /*aAllUsed*/,
                         scPageID     /*aUsedLimit*/);
};

#endif // _O_SDPST_VERIFY_AND_DUMP_H_
