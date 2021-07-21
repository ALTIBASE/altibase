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
 

/***********************************************************************
 * $Id: ideLogEntry.h 91200 2021-07-12 02:29:48Z hykim $
 **********************************************************************/

/***********************************************************************
 * NAME
 *  ideLogEntry.h
 *
 * DESCRIPTION
 *  This file defines ideLogEntry, used for outputing trace log
 *  entries.
 *
 * **********************************************************************/

#ifndef _O_IDELOGENTRY_H_
#define _O_IDELOGENTRY_H_

#include <acp.h>
#include <idTypes.h>

#define IDE_MESSAGE_SIZE (2048)

/* ------------------------------------------------
 *  SERVER
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_SERVER_0   1           /* always do */
#define IDE_TRC_SERVER_1   (iduProperty::getServerTrcFlag() & 0x00000001)
#define IDE_TRC_SERVER_2   (iduProperty::getServerTrcFlag() & 0x00000002)
#define IDE_TRC_SERVER_3   (iduProperty::getServerTrcFlag() & 0x00000004)
#define IDE_TRC_SERVER_4   (iduProperty::getServerTrcFlag() & 0x00000008)
#define IDE_TRC_SERVER_5   (iduProperty::getServerTrcFlag() & 0x00000010)
#define IDE_TRC_SERVER_6   (iduProperty::getServerTrcFlag() & 0x00000020)
#define IDE_TRC_SERVER_7   (iduProperty::getServerTrcFlag() & 0x00000040)
#define IDE_TRC_SERVER_8   (iduProperty::getServerTrcFlag() & 0x00000080)
#define IDE_TRC_SERVER_9   (iduProperty::getServerTrcFlag() & 0x00000100)
#define IDE_TRC_SERVER_10  (iduProperty::getServerTrcFlag() & 0x00000200)
#define IDE_TRC_SERVER_11  (iduProperty::getServerTrcFlag() & 0x00000400)
#define IDE_TRC_SERVER_12  (iduProperty::getServerTrcFlag() & 0x00000800)
#define IDE_TRC_SERVER_13  (iduProperty::getServerTrcFlag() & 0x00001000)
#define IDE_TRC_SERVER_14  (iduProperty::getServerTrcFlag() & 0x00002000)
#define IDE_TRC_SERVER_15  (iduProperty::getServerTrcFlag() & 0x00004000)
#define IDE_TRC_SERVER_16  (iduProperty::getServerTrcFlag() & 0x00008000)
#define IDE_TRC_SERVER_17  (iduProperty::getServerTrcFlag() & 0x00010000)
#define IDE_TRC_SERVER_18  (iduProperty::getServerTrcFlag() & 0x00020000)
#define IDE_TRC_SERVER_19  (iduProperty::getServerTrcFlag() & 0x00040000)
#define IDE_TRC_SERVER_20  (iduProperty::getServerTrcFlag() & 0x00080000)
#define IDE_TRC_SERVER_21  (iduProperty::getServerTrcFlag() & 0x00100000)
#define IDE_TRC_SERVER_22  (iduProperty::getServerTrcFlag() & 0x00200000)
#define IDE_TRC_SERVER_23  (iduProperty::getServerTrcFlag() & 0x00400000)
#define IDE_TRC_SERVER_24  (iduProperty::getServerTrcFlag() & 0x00800000)
#define IDE_TRC_SERVER_25  (iduProperty::getServerTrcFlag() & 0x01000000)
#define IDE_TRC_SERVER_26  (iduProperty::getServerTrcFlag() & 0x02000000)
#define IDE_TRC_SERVER_27  (iduProperty::getServerTrcFlag() & 0x04000000)
#define IDE_TRC_SERVER_28  (iduProperty::getServerTrcFlag() & 0x08000000)
#define IDE_TRC_SERVER_29  (iduProperty::getServerTrcFlag() & 0x10000000)
#define IDE_TRC_SERVER_30  (iduProperty::getServerTrcFlag() & 0x20000000)
#define IDE_TRC_SERVER_31  (iduProperty::getServerTrcFlag() & 0x40000000)
#define IDE_TRC_SERVER_32  (iduProperty::getServerTrcFlag() & 0x80000000)


#define IDE_SERVER_0    IDE_TRC_SERVER_0,  IDE_SERVER, 0
#define IDE_SERVER_1    IDE_TRC_SERVER_1,  IDE_SERVER, 1
#define IDE_SERVER_2    IDE_TRC_SERVER_2,  IDE_SERVER, 2
#define IDE_SERVER_3    IDE_TRC_SERVER_3,  IDE_SERVER, 3
#define IDE_SERVER_4    IDE_TRC_SERVER_4,  IDE_SERVER, 4
#define IDE_SERVER_5    IDE_TRC_SERVER_5,  IDE_SERVER, 5
#define IDE_SERVER_6    IDE_TRC_SERVER_6,  IDE_SERVER, 6
#define IDE_SERVER_7    IDE_TRC_SERVER_7,  IDE_SERVER, 7
#define IDE_SERVER_8    IDE_TRC_SERVER_8,  IDE_SERVER, 8
#define IDE_SERVER_9    IDE_TRC_SERVER_9,  IDE_SERVER, 9
#define IDE_SERVER_10   IDE_TRC_SERVER_10, IDE_SERVER, 10
#define IDE_SERVER_11   IDE_TRC_SERVER_11, IDE_SERVER, 11
#define IDE_SERVER_12   IDE_TRC_SERVER_12, IDE_SERVER, 12
#define IDE_SERVER_13   IDE_TRC_SERVER_13, IDE_SERVER, 13
#define IDE_SERVER_14   IDE_TRC_SERVER_14, IDE_SERVER, 14
#define IDE_SERVER_15   IDE_TRC_SERVER_15, IDE_SERVER, 15
#define IDE_SERVER_16   IDE_TRC_SERVER_16, IDE_SERVER, 16
#define IDE_SERVER_17   IDE_TRC_SERVER_17, IDE_SERVER, 17
#define IDE_SERVER_18   IDE_TRC_SERVER_18, IDE_SERVER, 18
#define IDE_SERVER_19   IDE_TRC_SERVER_19, IDE_SERVER, 19
#define IDE_SERVER_20   IDE_TRC_SERVER_20, IDE_SERVER, 20
#define IDE_SERVER_21   IDE_TRC_SERVER_21, IDE_SERVER, 21
#define IDE_SERVER_22   IDE_TRC_SERVER_22, IDE_SERVER, 22
#define IDE_SERVER_23   IDE_TRC_SERVER_23, IDE_SERVER, 23
#define IDE_SERVER_24   IDE_TRC_SERVER_24, IDE_SERVER, 24
#define IDE_SERVER_25   IDE_TRC_SERVER_25, IDE_SERVER, 25
#define IDE_SERVER_26   IDE_TRC_SERVER_26, IDE_SERVER, 26
#define IDE_SERVER_27   IDE_TRC_SERVER_27, IDE_SERVER, 27
#define IDE_SERVER_28   IDE_TRC_SERVER_28, IDE_SERVER, 28
#define IDE_SERVER_29   IDE_TRC_SERVER_29, IDE_SERVER, 29
#define IDE_SERVER_30   IDE_TRC_SERVER_30, IDE_SERVER, 30
#define IDE_SERVER_31   IDE_TRC_SERVER_31, IDE_SERVER, 31
#define IDE_SERVER_32   IDE_TRC_SERVER_32, IDE_SERVER, 32

/* ------------------------------------------------
 *  SM
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_SM_0   1           /* always do */
#define IDE_TRC_SM_1   (iduProperty::getSmTrcFlag() & 0x00000001)
#define IDE_TRC_SM_2   (iduProperty::getSmTrcFlag() & 0x00000002)
#define IDE_TRC_SM_3   (iduProperty::getSmTrcFlag() & 0x00000004)
#define IDE_TRC_SM_4   (iduProperty::getSmTrcFlag() & 0x00000008)
#define IDE_TRC_SM_5   (iduProperty::getSmTrcFlag() & 0x00000010)
#define IDE_TRC_SM_6   (iduProperty::getSmTrcFlag() & 0x00000020)
#define IDE_TRC_SM_7   (iduProperty::getSmTrcFlag() & 0x00000040)
#define IDE_TRC_SM_8   (iduProperty::getSmTrcFlag() & 0x00000080)
#define IDE_TRC_SM_9   (iduProperty::getSmTrcFlag() & 0x00000100)
#define IDE_TRC_SM_10  (iduProperty::getSmTrcFlag() & 0x00000200)
#define IDE_TRC_SM_11  (iduProperty::getSmTrcFlag() & 0x00000400)
#define IDE_TRC_SM_12  (iduProperty::getSmTrcFlag() & 0x00000800)
#define IDE_TRC_SM_13  (iduProperty::getSmTrcFlag() & 0x00001000)
#define IDE_TRC_SM_14  (iduProperty::getSmTrcFlag() & 0x00002000)
#define IDE_TRC_SM_15  (iduProperty::getSmTrcFlag() & 0x00004000)
#define IDE_TRC_SM_16  (iduProperty::getSmTrcFlag() & 0x00008000)
#define IDE_TRC_SM_17  (iduProperty::getSmTrcFlag() & 0x00010000)
#define IDE_TRC_SM_18  (iduProperty::getSmTrcFlag() & 0x00020000)
#define IDE_TRC_SM_19  (iduProperty::getSmTrcFlag() & 0x00040000)
#define IDE_TRC_SM_20  (iduProperty::getSmTrcFlag() & 0x00080000)
#define IDE_TRC_SM_21  (iduProperty::getSmTrcFlag() & 0x00100000)
#define IDE_TRC_SM_22  (iduProperty::getSmTrcFlag() & 0x00200000)
#define IDE_TRC_SM_23  (iduProperty::getSmTrcFlag() & 0x00400000)
#define IDE_TRC_SM_24  (iduProperty::getSmTrcFlag() & 0x00800000)
#define IDE_TRC_SM_25  (iduProperty::getSmTrcFlag() & 0x01000000)
#define IDE_TRC_SM_26  (iduProperty::getSmTrcFlag() & 0x02000000)
#define IDE_TRC_SM_27  (iduProperty::getSmTrcFlag() & 0x04000000)
#define IDE_TRC_SM_28  (iduProperty::getSmTrcFlag() & 0x08000000)
#define IDE_TRC_SM_29  (iduProperty::getSmTrcFlag() & 0x10000000)
#define IDE_TRC_SM_30  (iduProperty::getSmTrcFlag() & 0x20000000)
#define IDE_TRC_SM_31  (iduProperty::getSmTrcFlag() & 0x40000000)
#define IDE_TRC_SM_32  (iduProperty::getSmTrcFlag() & 0x80000000)


#define IDE_SM_0    IDE_TRC_SM_0,  IDE_SM, 0
#define IDE_SM_1    IDE_TRC_SM_1,  IDE_SM, 1
#define IDE_SM_2    IDE_TRC_SM_2,  IDE_SM, 2
#define IDE_SM_3    IDE_TRC_SM_3,  IDE_SM, 3
#define IDE_SM_4    IDE_TRC_SM_4,  IDE_SM, 4
#define IDE_SM_5    IDE_TRC_SM_5,  IDE_SM, 5
#define IDE_SM_6    IDE_TRC_SM_6,  IDE_SM, 6
#define IDE_SM_7    IDE_TRC_SM_7,  IDE_SM, 7
#define IDE_SM_8    IDE_TRC_SM_8,  IDE_SM, 8
#define IDE_SM_9    IDE_TRC_SM_9,  IDE_SM, 9
#define IDE_SM_10   IDE_TRC_SM_10, IDE_SM, 10
#define IDE_SM_11   IDE_TRC_SM_11, IDE_SM, 11
#define IDE_SM_12   IDE_TRC_SM_12, IDE_SM, 12
#define IDE_SM_13   IDE_TRC_SM_13, IDE_SM, 13
#define IDE_SM_14   IDE_TRC_SM_14, IDE_SM, 14
#define IDE_SM_15   IDE_TRC_SM_15, IDE_SM, 15
#define IDE_SM_16   IDE_TRC_SM_16, IDE_SM, 16
#define IDE_SM_17   IDE_TRC_SM_17, IDE_SM, 17
#define IDE_SM_18   IDE_TRC_SM_18, IDE_SM, 18
#define IDE_SM_19   IDE_TRC_SM_19, IDE_SM, 19
#define IDE_SM_20   IDE_TRC_SM_20, IDE_SM, 20
#define IDE_SM_21   IDE_TRC_SM_21, IDE_SM, 21
#define IDE_SM_22   IDE_TRC_SM_22, IDE_SM, 22
#define IDE_SM_23   IDE_TRC_SM_23, IDE_SM, 23
#define IDE_SM_24   IDE_TRC_SM_24, IDE_SM, 24
#define IDE_SM_25   IDE_TRC_SM_25, IDE_SM, 25
#define IDE_SM_26   IDE_TRC_SM_26, IDE_SM, 26
#define IDE_SM_27   IDE_TRC_SM_27, IDE_SM, 27
#define IDE_SM_28   IDE_TRC_SM_28, IDE_SM, 28
#define IDE_SM_29   IDE_TRC_SM_29, IDE_SM, 29
#define IDE_SM_30   IDE_TRC_SM_30, IDE_SM, 30
#define IDE_SM_31   IDE_TRC_SM_31, IDE_SM, 31
#define IDE_SM_32   IDE_TRC_SM_32, IDE_SM, 32

/* ------------------------------------------------
 *  RP
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_RP_0   1           /* always do */
#define IDE_TRC_RP_1   (iduProperty::getRpTrcFlag() & 0x00000001)
#define IDE_TRC_RP_2   (iduProperty::getRpTrcFlag() & 0x00000002)
#define IDE_TRC_RP_3   (iduProperty::getRpTrcFlag() & 0x00000004)
#define IDE_TRC_RP_4   (iduProperty::getRpTrcFlag() & 0x00000008)
#define IDE_TRC_RP_5   (iduProperty::getRpTrcFlag() & 0x00000010)
#define IDE_TRC_RP_6   (iduProperty::getRpTrcFlag() & 0x00000020)
#define IDE_TRC_RP_7   (iduProperty::getRpTrcFlag() & 0x00000040)
#define IDE_TRC_RP_8   (iduProperty::getRpTrcFlag() & 0x00000080)
#define IDE_TRC_RP_9   (iduProperty::getRpTrcFlag() & 0x00000100)
#define IDE_TRC_RP_10  (iduProperty::getRpTrcFlag() & 0x00000200)
#define IDE_TRC_RP_11  (iduProperty::getRpTrcFlag() & 0x00000400)
#define IDE_TRC_RP_12  (iduProperty::getRpTrcFlag() & 0x00000800)
#define IDE_TRC_RP_13  (iduProperty::getRpTrcFlag() & 0x00001000)
#define IDE_TRC_RP_14  (iduProperty::getRpTrcFlag() & 0x00002000)
#define IDE_TRC_RP_15  (iduProperty::getRpTrcFlag() & 0x00004000)
#define IDE_TRC_RP_16  (iduProperty::getRpTrcFlag() & 0x00008000)
#define IDE_TRC_RP_17  (iduProperty::getRpTrcFlag() & 0x00010000)
#define IDE_TRC_RP_18  (iduProperty::getRpTrcFlag() & 0x00020000)
#define IDE_TRC_RP_19  (iduProperty::getRpTrcFlag() & 0x00040000)
#define IDE_TRC_RP_20  (iduProperty::getRpTrcFlag() & 0x00080000)
#define IDE_TRC_RP_21  (iduProperty::getRpTrcFlag() & 0x00100000)
#define IDE_TRC_RP_22  (iduProperty::getRpTrcFlag() & 0x00200000)
#define IDE_TRC_RP_23  (iduProperty::getRpTrcFlag() & 0x00400000)
#define IDE_TRC_RP_24  (iduProperty::getRpTrcFlag() & 0x00800000)
#define IDE_TRC_RP_25  (iduProperty::getRpTrcFlag() & 0x01000000)
#define IDE_TRC_RP_26  (iduProperty::getRpTrcFlag() & 0x02000000)
#define IDE_TRC_RP_27  (iduProperty::getRpTrcFlag() & 0x04000000)
#define IDE_TRC_RP_28  (iduProperty::getRpTrcFlag() & 0x08000000)
#define IDE_TRC_RP_29  (iduProperty::getRpTrcFlag() & 0x10000000)
#define IDE_TRC_RP_30  (iduProperty::getRpTrcFlag() & 0x20000000)
#define IDE_TRC_RP_31  (iduProperty::getRpTrcFlag() & 0x40000000)
#define IDE_TRC_RP_32  (iduProperty::getRpTrcFlag() & 0x80000000)


#define IDE_RP_0    IDE_TRC_RP_0,  IDE_RP, 0
#define IDE_RP_1    IDE_TRC_RP_1,  IDE_RP, 1
#define IDE_RP_2    IDE_TRC_RP_2,  IDE_RP, 2
#define IDE_RP_3    IDE_TRC_RP_3,  IDE_RP, 3
#define IDE_RP_4    IDE_TRC_RP_4,  IDE_RP, 4
#define IDE_RP_5    IDE_TRC_RP_5,  IDE_RP, 5
#define IDE_RP_6    IDE_TRC_RP_6,  IDE_RP, 6
#define IDE_RP_7    IDE_TRC_RP_7,  IDE_RP, 7
#define IDE_RP_8    IDE_TRC_RP_8,  IDE_RP, 8
#define IDE_RP_9    IDE_TRC_RP_9,  IDE_RP, 9
#define IDE_RP_10   IDE_TRC_RP_10, IDE_RP, 10
#define IDE_RP_11   IDE_TRC_RP_11, IDE_RP, 11
#define IDE_RP_12   IDE_TRC_RP_12, IDE_RP, 12
#define IDE_RP_13   IDE_TRC_RP_13, IDE_RP, 13
#define IDE_RP_14   IDE_TRC_RP_14, IDE_RP, 14
#define IDE_RP_15   IDE_TRC_RP_15, IDE_RP, 15
#define IDE_RP_16   IDE_TRC_RP_16, IDE_RP, 16
#define IDE_RP_17   IDE_TRC_RP_17, IDE_RP, 17
#define IDE_RP_18   IDE_TRC_RP_18, IDE_RP, 18
#define IDE_RP_19   IDE_TRC_RP_19, IDE_RP, 19
#define IDE_RP_20   IDE_TRC_RP_20, IDE_RP, 20
#define IDE_RP_21   IDE_TRC_RP_21, IDE_RP, 21
#define IDE_RP_22   IDE_TRC_RP_22, IDE_RP, 22
#define IDE_RP_23   IDE_TRC_RP_23, IDE_RP, 23
#define IDE_RP_24   IDE_TRC_RP_24, IDE_RP, 24
#define IDE_RP_25   IDE_TRC_RP_25, IDE_RP, 25
#define IDE_RP_26   IDE_TRC_RP_26, IDE_RP, 26
#define IDE_RP_27   IDE_TRC_RP_27, IDE_RP, 27
#define IDE_RP_28   IDE_TRC_RP_28, IDE_RP, 28
#define IDE_RP_29   IDE_TRC_RP_29, IDE_RP, 29
#define IDE_RP_30   IDE_TRC_RP_30, IDE_RP, 30
#define IDE_RP_31   IDE_TRC_RP_31, IDE_RP, 31
#define IDE_RP_32   IDE_TRC_RP_32, IDE_RP, 32

/* ------------------------------------------------
 *  RP
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_RP_0   1           /* always do */
#define IDE_TRC_RP_1   (iduProperty::getRpTrcFlag() & 0x00000001)
#define IDE_TRC_RP_2   (iduProperty::getRpTrcFlag() & 0x00000002)
#define IDE_TRC_RP_3   (iduProperty::getRpTrcFlag() & 0x00000004)
#define IDE_TRC_RP_4   (iduProperty::getRpTrcFlag() & 0x00000008)
#define IDE_TRC_RP_5   (iduProperty::getRpTrcFlag() & 0x00000010)
#define IDE_TRC_RP_6   (iduProperty::getRpTrcFlag() & 0x00000020)
#define IDE_TRC_RP_7   (iduProperty::getRpTrcFlag() & 0x00000040)
#define IDE_TRC_RP_8   (iduProperty::getRpTrcFlag() & 0x00000080)
#define IDE_TRC_RP_9   (iduProperty::getRpTrcFlag() & 0x00000100)
#define IDE_TRC_RP_10  (iduProperty::getRpTrcFlag() & 0x00000200)
#define IDE_TRC_RP_11  (iduProperty::getRpTrcFlag() & 0x00000400)
#define IDE_TRC_RP_12  (iduProperty::getRpTrcFlag() & 0x00000800)
#define IDE_TRC_RP_13  (iduProperty::getRpTrcFlag() & 0x00001000)
#define IDE_TRC_RP_14  (iduProperty::getRpTrcFlag() & 0x00002000)
#define IDE_TRC_RP_15  (iduProperty::getRpTrcFlag() & 0x00004000)
#define IDE_TRC_RP_16  (iduProperty::getRpTrcFlag() & 0x00008000)
#define IDE_TRC_RP_17  (iduProperty::getRpTrcFlag() & 0x00010000)
#define IDE_TRC_RP_18  (iduProperty::getRpTrcFlag() & 0x00020000)
#define IDE_TRC_RP_19  (iduProperty::getRpTrcFlag() & 0x00040000)
#define IDE_TRC_RP_20  (iduProperty::getRpTrcFlag() & 0x00080000)
#define IDE_TRC_RP_21  (iduProperty::getRpTrcFlag() & 0x00100000)
#define IDE_TRC_RP_22  (iduProperty::getRpTrcFlag() & 0x00200000)
#define IDE_TRC_RP_23  (iduProperty::getRpTrcFlag() & 0x00400000)
#define IDE_TRC_RP_24  (iduProperty::getRpTrcFlag() & 0x00800000)
#define IDE_TRC_RP_25  (iduProperty::getRpTrcFlag() & 0x01000000)
#define IDE_TRC_RP_26  (iduProperty::getRpTrcFlag() & 0x02000000)
#define IDE_TRC_RP_27  (iduProperty::getRpTrcFlag() & 0x04000000)
#define IDE_TRC_RP_28  (iduProperty::getRpTrcFlag() & 0x08000000)
#define IDE_TRC_RP_29  (iduProperty::getRpTrcFlag() & 0x10000000)
#define IDE_TRC_RP_30  (iduProperty::getRpTrcFlag() & 0x20000000)
#define IDE_TRC_RP_31  (iduProperty::getRpTrcFlag() & 0x40000000)
#define IDE_TRC_RP_32  (iduProperty::getRpTrcFlag() & 0x80000000)

/* ------------------------------------------------
 *  RP_CONFLICT
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_RP_CONFLICT_0   1           /* always do */
#define IDE_TRC_RP_CONFLICT_1   (iduProperty::getRpConflictTrcFlag() & 0x00000001)
#define IDE_TRC_RP_CONFLICT_2   (iduProperty::getRpConflictTrcFlag() & 0x00000002)
#define IDE_TRC_RP_CONFLICT_3   (iduProperty::getRpConflictTrcFlag() & 0x00000004)
#define IDE_TRC_RP_CONFLICT_4   (iduProperty::getRpConflictTrcFlag() & 0x00000008)
#define IDE_TRC_RP_CONFLICT_5   (iduProperty::getRpConflictTrcFlag() & 0x00000010)
#define IDE_TRC_RP_CONFLICT_6   (iduProperty::getRpConflictTrcFlag() & 0x00000020)
#define IDE_TRC_RP_CONFLICT_7   (iduProperty::getRpConflictTrcFlag() & 0x00000040)
#define IDE_TRC_RP_CONFLICT_8   (iduProperty::getRpConflictTrcFlag() & 0x00000080)
#define IDE_TRC_RP_CONFLICT_9   (iduProperty::getRpConflictTrcFlag() & 0x00000100)
#define IDE_TRC_RP_CONFLICT_10  (iduProperty::getRpConflictTrcFlag() & 0x00000200)
#define IDE_TRC_RP_CONFLICT_11  (iduProperty::getRpConflictTrcFlag() & 0x00000400)
#define IDE_TRC_RP_CONFLICT_12  (iduProperty::getRpConflictTrcFlag() & 0x00000800)
#define IDE_TRC_RP_CONFLICT_13  (iduProperty::getRpConflictTrcFlag() & 0x00001000)
#define IDE_TRC_RP_CONFLICT_14  (iduProperty::getRpConflictTrcFlag() & 0x00002000)
#define IDE_TRC_RP_CONFLICT_15  (iduProperty::getRpConflictTrcFlag() & 0x00004000)
#define IDE_TRC_RP_CONFLICT_16  (iduProperty::getRpConflictTrcFlag() & 0x00008000)
#define IDE_TRC_RP_CONFLICT_17  (iduProperty::getRpConflictTrcFlag() & 0x00010000)
#define IDE_TRC_RP_CONFLICT_18  (iduProperty::getRpConflictTrcFlag() & 0x00020000)
#define IDE_TRC_RP_CONFLICT_19  (iduProperty::getRpConflictTrcFlag() & 0x00040000)
#define IDE_TRC_RP_CONFLICT_20  (iduProperty::getRpConflictTrcFlag() & 0x00080000)
#define IDE_TRC_RP_CONFLICT_21  (iduProperty::getRpConflictTrcFlag() & 0x00100000)
#define IDE_TRC_RP_CONFLICT_22  (iduProperty::getRpConflictTrcFlag() & 0x00200000)
#define IDE_TRC_RP_CONFLICT_23  (iduProperty::getRpConflictTrcFlag() & 0x00400000)
#define IDE_TRC_RP_CONFLICT_24  (iduProperty::getRpConflictTrcFlag() & 0x00800000)
#define IDE_TRC_RP_CONFLICT_25  (iduProperty::getRpConflictTrcFlag() & 0x01000000)
#define IDE_TRC_RP_CONFLICT_26  (iduProperty::getRpConflictTrcFlag() & 0x02000000)
#define IDE_TRC_RP_CONFLICT_27  (iduProperty::getRpConflictTrcFlag() & 0x04000000)
#define IDE_TRC_RP_CONFLICT_28  (iduProperty::getRpConflictTrcFlag() & 0x08000000)
#define IDE_TRC_RP_CONFLICT_29  (iduProperty::getRpConflictTrcFlag() & 0x10000000)
#define IDE_TRC_RP_CONFLICT_30  (iduProperty::getRpConflictTrcFlag() & 0x20000000)
#define IDE_TRC_RP_CONFLICT_31  (iduProperty::getRpConflictTrcFlag() & 0x40000000)
#define IDE_TRC_RP_CONFLICT_32  (iduProperty::getRpConflictTrcFlag() & 0x80000000)

#define IDE_RP_CONFLICT_0    IDE_TRC_RP_CONFLICT_0,  IDE_RP_CONFLICT, 0
#define IDE_RP_CONFLICT_1    IDE_TRC_RP_CONFLICT_1,  IDE_RP_CONFLICT, 1
#define IDE_RP_CONFLICT_2    IDE_TRC_RP_CONFLICT_2,  IDE_RP_CONFLICT, 2
#define IDE_RP_CONFLICT_3    IDE_TRC_RP_CONFLICT_3,  IDE_RP_CONFLICT, 3
#define IDE_RP_CONFLICT_4    IDE_TRC_RP_CONFLICT_4,  IDE_RP_CONFLICT, 4
#define IDE_RP_CONFLICT_5    IDE_TRC_RP_CONFLICT_5,  IDE_RP_CONFLICT, 5
#define IDE_RP_CONFLICT_6    IDE_TRC_RP_CONFLICT_6,  IDE_RP_CONFLICT, 6
#define IDE_RP_CONFLICT_7    IDE_TRC_RP_CONFLICT_7,  IDE_RP_CONFLICT, 7
#define IDE_RP_CONFLICT_8    IDE_TRC_RP_CONFLICT_8,  IDE_RP_CONFLICT, 8
#define IDE_RP_CONFLICT_9    IDE_TRC_RP_CONFLICT_9,  IDE_RP_CONFLICT, 9
#define IDE_RP_CONFLICT_10   IDE_TRC_RP_CONFLICT_10, IDE_RP_CONFLICT, 10
#define IDE_RP_CONFLICT_11   IDE_TRC_RP_CONFLICT_11, IDE_RP_CONFLICT, 11
#define IDE_RP_CONFLICT_12   IDE_TRC_RP_CONFLICT_12, IDE_RP_CONFLICT, 12
#define IDE_RP_CONFLICT_13   IDE_TRC_RP_CONFLICT_13, IDE_RP_CONFLICT, 13
#define IDE_RP_CONFLICT_14   IDE_TRC_RP_CONFLICT_14, IDE_RP_CONFLICT, 14
#define IDE_RP_CONFLICT_15   IDE_TRC_RP_CONFLICT_15, IDE_RP_CONFLICT, 15
#define IDE_RP_CONFLICT_16   IDE_TRC_RP_CONFLICT_16, IDE_RP_CONFLICT, 16
#define IDE_RP_CONFLICT_17   IDE_TRC_RP_CONFLICT_17, IDE_RP_CONFLICT, 17
#define IDE_RP_CONFLICT_18   IDE_TRC_RP_CONFLICT_18, IDE_RP_CONFLICT, 18
#define IDE_RP_CONFLICT_19   IDE_TRC_RP_CONFLICT_19, IDE_RP_CONFLICT, 19
#define IDE_RP_CONFLICT_20   IDE_TRC_RP_CONFLICT_20, IDE_RP_CONFLICT, 20
#define IDE_RP_CONFLICT_21   IDE_TRC_RP_CONFLICT_21, IDE_RP_CONFLICT, 21
#define IDE_RP_CONFLICT_22   IDE_TRC_RP_CONFLICT_22, IDE_RP_CONFLICT, 22
#define IDE_RP_CONFLICT_23   IDE_TRC_RP_CONFLICT_23, IDE_RP_CONFLICT, 23
#define IDE_RP_CONFLICT_24   IDE_TRC_RP_CONFLICT_24, IDE_RP_CONFLICT, 24
#define IDE_RP_CONFLICT_25   IDE_TRC_RP_CONFLICT_25, IDE_RP_CONFLICT, 25
#define IDE_RP_CONFLICT_26   IDE_TRC_RP_CONFLICT_26, IDE_RP_CONFLICT, 26
#define IDE_RP_CONFLICT_27   IDE_TRC_RP_CONFLICT_27, IDE_RP_CONFLICT, 27
#define IDE_RP_CONFLICT_28   IDE_TRC_RP_CONFLICT_28, IDE_RP_CONFLICT, 28
#define IDE_RP_CONFLICT_29   IDE_TRC_RP_CONFLICT_29, IDE_RP_CONFLICT, 29
#define IDE_RP_CONFLICT_30   IDE_TRC_RP_CONFLICT_30, IDE_RP_CONFLICT, 30
#define IDE_RP_CONFLICT_31   IDE_TRC_RP_CONFLICT_31, IDE_RP_CONFLICT, 31
#define IDE_RP_CONFLICT_32   IDE_TRC_RP_CONFLICT_32, IDE_RP_CONFLICT, 32

/* ------------------------------------------------
 *  QP
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_QP_0   1           /* always do */
#define IDE_TRC_QP_1   (iduProperty::getQpTrcFlag() & 0x00000001)
#define IDE_TRC_QP_2   (iduProperty::getQpTrcFlag() & 0x00000002)
#define IDE_TRC_QP_3   (iduProperty::getQpTrcFlag() & 0x00000004)
#define IDE_TRC_QP_4   (iduProperty::getQpTrcFlag() & 0x00000008)
#define IDE_TRC_QP_5   (iduProperty::getQpTrcFlag() & 0x00000010)
#define IDE_TRC_QP_6   (iduProperty::getQpTrcFlag() & 0x00000020)
#define IDE_TRC_QP_7   (iduProperty::getQpTrcFlag() & 0x00000040)
#define IDE_TRC_QP_8   (iduProperty::getQpTrcFlag() & 0x00000080)
#define IDE_TRC_QP_9   (iduProperty::getQpTrcFlag() & 0x00000100)
#define IDE_TRC_QP_10  (iduProperty::getQpTrcFlag() & 0x00000200)
#define IDE_TRC_QP_11  (iduProperty::getQpTrcFlag() & 0x00000400)
#define IDE_TRC_QP_12  (iduProperty::getQpTrcFlag() & 0x00000800)
#define IDE_TRC_QP_13  (iduProperty::getQpTrcFlag() & 0x00001000)
#define IDE_TRC_QP_14  (iduProperty::getQpTrcFlag() & 0x00002000)
#define IDE_TRC_QP_15  (iduProperty::getQpTrcFlag() & 0x00004000)
#define IDE_TRC_QP_16  (iduProperty::getQpTrcFlag() & 0x00008000)
#define IDE_TRC_QP_17  (iduProperty::getQpTrcFlag() & 0x00010000)
#define IDE_TRC_QP_18  (iduProperty::getQpTrcFlag() & 0x00020000)
#define IDE_TRC_QP_19  (iduProperty::getQpTrcFlag() & 0x00040000)
#define IDE_TRC_QP_20  (iduProperty::getQpTrcFlag() & 0x00080000)
#define IDE_TRC_QP_21  (iduProperty::getQpTrcFlag() & 0x00100000)
#define IDE_TRC_QP_22  (iduProperty::getQpTrcFlag() & 0x00200000)
#define IDE_TRC_QP_23  (iduProperty::getQpTrcFlag() & 0x00400000)
#define IDE_TRC_QP_24  (iduProperty::getQpTrcFlag() & 0x00800000)
#define IDE_TRC_QP_25  (iduProperty::getQpTrcFlag() & 0x01000000)
#define IDE_TRC_QP_26  (iduProperty::getQpTrcFlag() & 0x02000000)
#define IDE_TRC_QP_27  (iduProperty::getQpTrcFlag() & 0x04000000)
#define IDE_TRC_QP_28  (iduProperty::getQpTrcFlag() & 0x08000000)
#define IDE_TRC_QP_29  (iduProperty::getQpTrcFlag() & 0x10000000)
#define IDE_TRC_QP_30  (iduProperty::getQpTrcFlag() & 0x20000000)
#define IDE_TRC_QP_31  (iduProperty::getQpTrcFlag() & 0x40000000)
#define IDE_TRC_QP_32  (iduProperty::getQpTrcFlag() & 0x80000000)


#define IDE_QP_0    IDE_TRC_QP_0,  IDE_QP, 0
#define IDE_QP_1    IDE_TRC_QP_1,  IDE_QP, 1
#define IDE_QP_2    IDE_TRC_QP_2,  IDE_QP, 2
#define IDE_QP_3    IDE_TRC_QP_3,  IDE_QP, 3
#define IDE_QP_4    IDE_TRC_QP_4,  IDE_QP, 4
#define IDE_QP_5    IDE_TRC_QP_5,  IDE_QP, 5
#define IDE_QP_6    IDE_TRC_QP_6,  IDE_QP, 6
#define IDE_QP_7    IDE_TRC_QP_7,  IDE_QP, 7
#define IDE_QP_8    IDE_TRC_QP_8,  IDE_QP, 8
#define IDE_QP_9    IDE_TRC_QP_9,  IDE_QP, 9
#define IDE_QP_10   IDE_TRC_QP_10, IDE_QP, 10
#define IDE_QP_11   IDE_TRC_QP_11, IDE_QP, 11
#define IDE_QP_12   IDE_TRC_QP_12, IDE_QP, 12
#define IDE_QP_13   IDE_TRC_QP_13, IDE_QP, 13
#define IDE_QP_14   IDE_TRC_QP_14, IDE_QP, 14
#define IDE_QP_15   IDE_TRC_QP_15, IDE_QP, 15
#define IDE_QP_16   IDE_TRC_QP_16, IDE_QP, 16
#define IDE_QP_17   IDE_TRC_QP_17, IDE_QP, 17
#define IDE_QP_18   IDE_TRC_QP_18, IDE_QP, 18
#define IDE_QP_19   IDE_TRC_QP_19, IDE_QP, 19
#define IDE_QP_20   IDE_TRC_QP_20, IDE_QP, 20
#define IDE_QP_21   IDE_TRC_QP_21, IDE_QP, 21
#define IDE_QP_22   IDE_TRC_QP_22, IDE_QP, 22
#define IDE_QP_23   IDE_TRC_QP_23, IDE_QP, 23
#define IDE_QP_24   IDE_TRC_QP_24, IDE_QP, 24
#define IDE_QP_25   IDE_TRC_QP_25, IDE_QP, 25
#define IDE_QP_26   IDE_TRC_QP_26, IDE_QP, 26
#define IDE_QP_27   IDE_TRC_QP_27, IDE_QP, 27
#define IDE_QP_28   IDE_TRC_QP_28, IDE_QP, 28
#define IDE_QP_29   IDE_TRC_QP_29, IDE_QP, 29
#define IDE_QP_30   IDE_TRC_QP_30, IDE_QP, 30
#define IDE_QP_31   IDE_TRC_QP_31, IDE_QP, 31
#define IDE_QP_32   IDE_TRC_QP_32, IDE_QP, 32

/* ------------------------------------------------
 *  JOB
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_JOB_0   1           /* always do */
#define IDE_TRC_JOB_1   (iduProperty::getJobTrcFlag() & 0x00000001)
#define IDE_TRC_JOB_2   (iduProperty::getJobTrcFlag() & 0x00000002)
#define IDE_TRC_JOB_3   (iduProperty::getJobTrcFlag() & 0x00000004)
#define IDE_TRC_JOB_4   (iduProperty::getJobTrcFlag() & 0x00000008)
#define IDE_TRC_JOB_5   (iduProperty::getJobTrcFlag() & 0x00000010)
#define IDE_TRC_JOB_6   (iduProperty::getJobTrcFlag() & 0x00000020)
#define IDE_TRC_JOB_7   (iduProperty::getJobTrcFlag() & 0x00000040)
#define IDE_TRC_JOB_8   (iduProperty::getJobTrcFlag() & 0x00000080)
#define IDE_TRC_JOB_9   (iduProperty::getJobTrcFlag() & 0x00000100)
#define IDE_TRC_JOB_10  (iduProperty::getJobTrcFlag() & 0x00000200)
#define IDE_TRC_JOB_11  (iduProperty::getJobTrcFlag() & 0x00000400)
#define IDE_TRC_JOB_12  (iduProperty::getJobTrcFlag() & 0x00000800)
#define IDE_TRC_JOB_13  (iduProperty::getJobTrcFlag() & 0x00001000)
#define IDE_TRC_JOB_14  (iduProperty::getJobTrcFlag() & 0x00002000)
#define IDE_TRC_JOB_15  (iduProperty::getJobTrcFlag() & 0x00004000)
#define IDE_TRC_JOB_16  (iduProperty::getJobTrcFlag() & 0x00008000)
#define IDE_TRC_JOB_17  (iduProperty::getJobTrcFlag() & 0x00010000)
#define IDE_TRC_JOB_18  (iduProperty::getJobTrcFlag() & 0x00020000)
#define IDE_TRC_JOB_19  (iduProperty::getJobTrcFlag() & 0x00040000)
#define IDE_TRC_JOB_20  (iduProperty::getJobTrcFlag() & 0x00080000)
#define IDE_TRC_JOB_21  (iduProperty::getJobTrcFlag() & 0x00100000)
#define IDE_TRC_JOB_22  (iduProperty::getJobTrcFlag() & 0x00200000)
#define IDE_TRC_JOB_23  (iduProperty::getJobTrcFlag() & 0x00400000)
#define IDE_TRC_JOB_24  (iduProperty::getJobTrcFlag() & 0x00800000)
#define IDE_TRC_JOB_25  (iduProperty::getJobTrcFlag() & 0x01000000)
#define IDE_TRC_JOB_26  (iduProperty::getJobTrcFlag() & 0x02000000)
#define IDE_TRC_JOB_27  (iduProperty::getJobTrcFlag() & 0x04000000)
#define IDE_TRC_JOB_28  (iduProperty::getJobTrcFlag() & 0x08000000)
#define IDE_TRC_JOB_29  (iduProperty::getJobTrcFlag() & 0x10000000)
#define IDE_TRC_JOB_30  (iduProperty::getJobTrcFlag() & 0x20000000)
#define IDE_TRC_JOB_31  (iduProperty::getJobTrcFlag() & 0x40000000)
#define IDE_TRC_JOB_32  (iduProperty::getJobTrcFlag() & 0x80000000)


#define IDE_JOB_0    IDE_TRC_JOB_0,  IDE_JOB, 0
#define IDE_JOB_1    IDE_TRC_JOB_1,  IDE_JOB, 1
#define IDE_JOB_2    IDE_TRC_JOB_2,  IDE_JOB, 2
#define IDE_JOB_3    IDE_TRC_JOB_3,  IDE_JOB, 3
#define IDE_JOB_4    IDE_TRC_JOB_4,  IDE_JOB, 4
#define IDE_JOB_5    IDE_TRC_JOB_5,  IDE_JOB, 5
#define IDE_JOB_6    IDE_TRC_JOB_6,  IDE_JOB, 6
#define IDE_JOB_7    IDE_TRC_JOB_7,  IDE_JOB, 7
#define IDE_JOB_8    IDE_TRC_JOB_8,  IDE_JOB, 8
#define IDE_JOB_9    IDE_TRC_JOB_9,  IDE_JOB, 9
#define IDE_JOB_10   IDE_TRC_JOB_10, IDE_JOB, 10
#define IDE_JOB_11   IDE_TRC_JOB_11, IDE_JOB, 11
#define IDE_JOB_12   IDE_TRC_JOB_12, IDE_JOB, 12
#define IDE_JOB_13   IDE_TRC_JOB_13, IDE_JOB, 13
#define IDE_JOB_14   IDE_TRC_JOB_14, IDE_JOB, 14
#define IDE_JOB_15   IDE_TRC_JOB_15, IDE_JOB, 15
#define IDE_JOB_16   IDE_TRC_JOB_16, IDE_JOB, 16
#define IDE_JOB_17   IDE_TRC_JOB_17, IDE_JOB, 17
#define IDE_JOB_18   IDE_TRC_JOB_18, IDE_JOB, 18
#define IDE_JOB_19   IDE_TRC_JOB_19, IDE_JOB, 19
#define IDE_JOB_20   IDE_TRC_JOB_20, IDE_JOB, 20
#define IDE_JOB_21   IDE_TRC_JOB_21, IDE_JOB, 21
#define IDE_JOB_22   IDE_TRC_JOB_22, IDE_JOB, 22
#define IDE_JOB_23   IDE_TRC_JOB_23, IDE_JOB, 23
#define IDE_JOB_24   IDE_TRC_JOB_24, IDE_JOB, 24
#define IDE_JOB_25   IDE_TRC_JOB_25, IDE_JOB, 25
#define IDE_JOB_26   IDE_TRC_JOB_26, IDE_JOB, 26
#define IDE_JOB_27   IDE_TRC_JOB_27, IDE_JOB, 27
#define IDE_JOB_28   IDE_TRC_JOB_28, IDE_JOB, 28
#define IDE_JOB_29   IDE_TRC_JOB_29, IDE_JOB, 29
#define IDE_JOB_30   IDE_TRC_JOB_30, IDE_JOB, 30
#define IDE_JOB_31   IDE_TRC_JOB_31, IDE_JOB, 31
#define IDE_JOB_32   IDE_TRC_JOB_32, IDE_JOB, 32

/* ------------------------------------------------
 *  SD  BUG-46138
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 * ----------------------------------------------*/

#define IDE_TRC_SD_0   1           /* always do */
#define IDE_TRC_SD_1   (iduProperty::getSdTrcFlag() & 0x00000001)
#define IDE_TRC_SD_2   (iduProperty::getSdTrcFlag() & 0x00000002)
#define IDE_TRC_SD_3   (iduProperty::getSdTrcFlag() & 0x00000004)
#define IDE_TRC_SD_4   (iduProperty::getSdTrcFlag() & 0x00000008)
#define IDE_TRC_SD_5   (iduProperty::getSdTrcFlag() & 0x00000010)
#define IDE_TRC_SD_6   (iduProperty::getSdTrcFlag() & 0x00000020)
#define IDE_TRC_SD_7   (iduProperty::getSdTrcFlag() & 0x00000040)
#define IDE_TRC_SD_8   (iduProperty::getSdTrcFlag() & 0x00000080)
#define IDE_TRC_SD_9   (iduProperty::getSdTrcFlag() & 0x00000100)
#define IDE_TRC_SD_10  (iduProperty::getSdTrcFlag() & 0x00000200)
#define IDE_TRC_SD_11  (iduProperty::getSdTrcFlag() & 0x00000400)
#define IDE_TRC_SD_12  (iduProperty::getSdTrcFlag() & 0x00000800)
#define IDE_TRC_SD_13  (iduProperty::getSdTrcFlag() & 0x00001000)
#define IDE_TRC_SD_14  (iduProperty::getSdTrcFlag() & 0x00002000)
#define IDE_TRC_SD_15  (iduProperty::getSdTrcFlag() & 0x00004000)
#define IDE_TRC_SD_16  (iduProperty::getSdTrcFlag() & 0x00008000)
#define IDE_TRC_SD_17  (iduProperty::getSdTrcFlag() & 0x00010000)
#define IDE_TRC_SD_18  (iduProperty::getSdTrcFlag() & 0x00020000)
#define IDE_TRC_SD_19  (iduProperty::getSdTrcFlag() & 0x00040000)
#define IDE_TRC_SD_20  (iduProperty::getSdTrcFlag() & 0x00080000)
#define IDE_TRC_SD_21  (iduProperty::getSdTrcFlag() & 0x00100000)
#define IDE_TRC_SD_22  (iduProperty::getSdTrcFlag() & 0x00200000)
#define IDE_TRC_SD_23  (iduProperty::getSdTrcFlag() & 0x00400000)
#define IDE_TRC_SD_24  (iduProperty::getSdTrcFlag() & 0x00800000)
#define IDE_TRC_SD_25  (iduProperty::getSdTrcFlag() & 0x01000000)
#define IDE_TRC_SD_26  (iduProperty::getSdTrcFlag() & 0x02000000)
#define IDE_TRC_SD_27  (iduProperty::getSdTrcFlag() & 0x04000000)
#define IDE_TRC_SD_28  (iduProperty::getSdTrcFlag() & 0x08000000)
#define IDE_TRC_SD_29  (iduProperty::getSdTrcFlag() & 0x10000000)
#define IDE_TRC_SD_30  (iduProperty::getSdTrcFlag() & 0x20000000)
#define IDE_TRC_SD_31  (iduProperty::getSdTrcFlag() & 0x40000000)
#define IDE_TRC_SD_32  (iduProperty::getSdTrcFlag() & 0x80000000)


#define IDE_SD_0    IDE_TRC_SD_0,  IDE_SD, 0
/* error 성 로그는 1 ~ 16 을 사용합니다. */
#define IDE_SD_1    IDE_TRC_SD_1,  IDE_SD, 1    /* shard meta error logging */
#define IDE_SD_2    IDE_TRC_SD_2,  IDE_SD, 2    /* global transaction error */
#define IDE_SD_3    IDE_TRC_SD_3,  IDE_SD, 3    
#define IDE_SD_4    IDE_TRC_SD_4,  IDE_SD, 4
#define IDE_SD_5    IDE_TRC_SD_5,  IDE_SD, 5
#define IDE_SD_6    IDE_TRC_SD_6,  IDE_SD, 6
#define IDE_SD_7    IDE_TRC_SD_7,  IDE_SD, 7
#define IDE_SD_8    IDE_TRC_SD_8,  IDE_SD, 8
#define IDE_SD_9    IDE_TRC_SD_9,  IDE_SD, 9
#define IDE_SD_10   IDE_TRC_SD_10, IDE_SD, 10
#define IDE_SD_11   IDE_TRC_SD_11, IDE_SD, 11
#define IDE_SD_12   IDE_TRC_SD_12, IDE_SD, 12
#define IDE_SD_13   IDE_TRC_SD_13, IDE_SD, 13
#define IDE_SD_14   IDE_TRC_SD_14, IDE_SD, 14
#define IDE_SD_15   IDE_TRC_SD_15, IDE_SD, 15
#define IDE_SD_16   IDE_TRC_SD_16, IDE_SD, 16
/* trace 성 로그는 17 ~ 32 을 사용합니다. */
#define IDE_SD_17   IDE_TRC_SD_17, IDE_SD, 17   /* shard meta change trace */
#define IDE_SD_18   IDE_TRC_SD_18, IDE_SD, 18   /* PROJ-2733 SCN and DistTxInfo */
#define IDE_SD_19   IDE_TRC_SD_19, IDE_SD, 19   /* PROJ-2733 SM module trace log */
#define IDE_SD_20   IDE_TRC_SD_20, IDE_SD, 20
#define IDE_SD_21   IDE_TRC_SD_21, IDE_SD, 21
#define IDE_SD_22   IDE_TRC_SD_22, IDE_SD, 22
#define IDE_SD_23   IDE_TRC_SD_23, IDE_SD, 23
#define IDE_SD_24   IDE_TRC_SD_24, IDE_SD, 24
#define IDE_SD_25   IDE_TRC_SD_25, IDE_SD, 25
#define IDE_SD_26   IDE_TRC_SD_26, IDE_SD, 26
#define IDE_SD_27   IDE_TRC_SD_27, IDE_SD, 27
#define IDE_SD_28   IDE_TRC_SD_28, IDE_SD, 28
#define IDE_SD_29   IDE_TRC_SD_29, IDE_SD, 29
#define IDE_SD_30   IDE_TRC_SD_30, IDE_SD, 30
#define IDE_SD_31   IDE_TRC_SD_31, IDE_SD, 31
#define IDE_SD_32   IDE_TRC_SD_32, IDE_SD, 32

/* ------------------------------------------------
 *  DK(Database Link)
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_DK_0   1           /* always do */
#define IDE_TRC_DK_1   (iduProperty::getDkTrcFlag() & 0x00000001)
#define IDE_TRC_DK_2   (iduProperty::getDkTrcFlag() & 0x00000002)
#define IDE_TRC_DK_3   (iduProperty::getDkTrcFlag() & 0x00000004)
#define IDE_TRC_DK_4   (iduProperty::getDkTrcFlag() & 0x00000008)
#define IDE_TRC_DK_5   (iduProperty::getDkTrcFlag() & 0x00000010)
#define IDE_TRC_DK_6   (iduProperty::getDkTrcFlag() & 0x00000020)
#define IDE_TRC_DK_7   (iduProperty::getDkTrcFlag() & 0x00000040)
#define IDE_TRC_DK_8   (iduProperty::getDkTrcFlag() & 0x00000080)
#define IDE_TRC_DK_9   (iduProperty::getDkTrcFlag() & 0x00000100)
#define IDE_TRC_DK_10  (iduProperty::getDkTrcFlag() & 0x00000200)
#define IDE_TRC_DK_11  (iduProperty::getDkTrcFlag() & 0x00000400)
#define IDE_TRC_DK_12  (iduProperty::getDkTrcFlag() & 0x00000800)
#define IDE_TRC_DK_13  (iduProperty::getDkTrcFlag() & 0x00001000)
#define IDE_TRC_DK_14  (iduProperty::getDkTrcFlag() & 0x00002000)
#define IDE_TRC_DK_15  (iduProperty::getDkTrcFlag() & 0x00004000)
#define IDE_TRC_DK_16  (iduProperty::getDkTrcFlag() & 0x00008000)
#define IDE_TRC_DK_17  (iduProperty::getDkTrcFlag() & 0x00010000)
#define IDE_TRC_DK_18  (iduProperty::getDkTrcFlag() & 0x00020000)
#define IDE_TRC_DK_19  (iduProperty::getDkTrcFlag() & 0x00040000)
#define IDE_TRC_DK_20  (iduProperty::getDkTrcFlag() & 0x00080000)
#define IDE_TRC_DK_21  (iduProperty::getDkTrcFlag() & 0x00100000)
#define IDE_TRC_DK_22  (iduProperty::getDkTrcFlag() & 0x00200000)
#define IDE_TRC_DK_23  (iduProperty::getDkTrcFlag() & 0x00400000)
#define IDE_TRC_DK_24  (iduProperty::getDkTrcFlag() & 0x00800000)
#define IDE_TRC_DK_25  (iduProperty::getDkTrcFlag() & 0x01000000)
#define IDE_TRC_DK_26  (iduProperty::getDkTrcFlag() & 0x02000000)
#define IDE_TRC_DK_27  (iduProperty::getDkTrcFlag() & 0x04000000)
#define IDE_TRC_DK_28  (iduProperty::getDkTrcFlag() & 0x08000000)
#define IDE_TRC_DK_29  (iduProperty::getDkTrcFlag() & 0x10000000)
#define IDE_TRC_DK_30  (iduProperty::getDkTrcFlag() & 0x20000000)
#define IDE_TRC_DK_31  (iduProperty::getDkTrcFlag() & 0x40000000)
#define IDE_TRC_DK_32  (iduProperty::getDkTrcFlag() & 0x80000000)


#define IDE_DK_0    IDE_TRC_DK_0,  IDE_DK, 0
#define IDE_DK_1    IDE_TRC_DK_1,  IDE_DK, 1
#define IDE_DK_2    IDE_TRC_DK_2,  IDE_DK, 2
#define IDE_DK_3    IDE_TRC_DK_3,  IDE_DK, 3
#define IDE_DK_4    IDE_TRC_DK_4,  IDE_DK, 4
#define IDE_DK_5    IDE_TRC_DK_5,  IDE_DK, 5
#define IDE_DK_6    IDE_TRC_DK_6,  IDE_DK, 6
#define IDE_DK_7    IDE_TRC_DK_7,  IDE_DK, 7
#define IDE_DK_8    IDE_TRC_DK_8,  IDE_DK, 8
#define IDE_DK_9    IDE_TRC_DK_9,  IDE_DK, 9
#define IDE_DK_10   IDE_TRC_DK_10, IDE_DK, 10
#define IDE_DK_11   IDE_TRC_DK_11, IDE_DK, 11
#define IDE_DK_12   IDE_TRC_DK_12, IDE_DK, 12
#define IDE_DK_13   IDE_TRC_DK_13, IDE_DK, 13
#define IDE_DK_14   IDE_TRC_DK_14, IDE_DK, 14
#define IDE_DK_15   IDE_TRC_DK_15, IDE_DK, 15
#define IDE_DK_16   IDE_TRC_DK_16, IDE_DK, 16
#define IDE_DK_17   IDE_TRC_DK_17, IDE_DK, 17
#define IDE_DK_18   IDE_TRC_DK_18, IDE_DK, 18
#define IDE_DK_19   IDE_TRC_DK_19, IDE_DK, 19
#define IDE_DK_20   IDE_TRC_DK_20, IDE_DK, 20
#define IDE_DK_21   IDE_TRC_DK_21, IDE_DK, 21
#define IDE_DK_22   IDE_TRC_DK_22, IDE_DK, 22
#define IDE_DK_23   IDE_TRC_DK_23, IDE_DK, 23
#define IDE_DK_24   IDE_TRC_DK_24, IDE_DK, 24
#define IDE_DK_25   IDE_TRC_DK_25, IDE_DK, 25
#define IDE_DK_26   IDE_TRC_DK_26, IDE_DK, 26
#define IDE_DK_27   IDE_TRC_DK_27, IDE_DK, 27
#define IDE_DK_28   IDE_TRC_DK_28, IDE_DK, 28
#define IDE_DK_29   IDE_TRC_DK_29, IDE_DK, 29
#define IDE_DK_30   IDE_TRC_DK_30, IDE_DK, 30
#define IDE_DK_31   IDE_TRC_DK_31, IDE_DK, 31
#define IDE_DK_32   IDE_TRC_DK_32, IDE_DK, 32

// bug-24840 divide xa log
/* ------------------------------------------------
 *  XA
 *  Macro: IDE_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define IDE_TRC_XA_0   1           /* always do */
#define IDE_TRC_XA_1   (iduProperty::getXaTrcFlag() & 0x00000001)
#define IDE_TRC_XA_2   (iduProperty::getXaTrcFlag() & 0x00000002)
#define IDE_TRC_XA_3   (iduProperty::getXaTrcFlag() & 0x00000004)
#define IDE_TRC_XA_4   (iduProperty::getXaTrcFlag() & 0x00000008)
#define IDE_TRC_XA_5   (iduProperty::getXaTrcFlag() & 0x00000010)
#define IDE_TRC_XA_6   (iduProperty::getXaTrcFlag() & 0x00000020)
#define IDE_TRC_XA_7   (iduProperty::getXaTrcFlag() & 0x00000040)
#define IDE_TRC_XA_8   (iduProperty::getXaTrcFlag() & 0x00000080)

#define IDE_XA_0    IDE_TRC_XA_0,  IDE_XA, 0
#define IDE_XA_1    IDE_TRC_XA_1,  IDE_XA, 1
#define IDE_XA_2    IDE_TRC_XA_2,  IDE_XA, 2
#define IDE_XA_3    IDE_TRC_XA_3,  IDE_XA, 3
#define IDE_XA_4    IDE_TRC_XA_4,  IDE_XA, 4
#define IDE_XA_5    IDE_TRC_XA_5,  IDE_XA, 5
#define IDE_XA_6    IDE_TRC_XA_6,  IDE_XA, 6
#define IDE_XA_7    IDE_TRC_XA_7,  IDE_XA, 7
#define IDE_XA_8    IDE_TRC_XA_8,  IDE_XA, 8

/* BUG-28866 */
/* ------------------------------------------------
 *  MM
 * ----------------------------------------------*/

#define IDE_TRC_MM_0   1           /* always do */
#define IDE_TRC_MM_3   (iduProperty::getMmTrcFlag() & 0x00000004)
#define IDE_TRC_MM_4   (iduProperty::getMmTrcFlag() & 0x00000008)

#define IDE_MM_0    IDE_TRC_MM_0,  IDE_MM, 0
#define IDE_MM_3    IDE_TRC_MM_3,  IDE_MM, 3
#define IDE_MM_4    IDE_TRC_MM_4,  IDE_MM, 4

/* PROJ-2118 */
/* ------------------------------------------------
 *  DUMP
 * ----------------------------------------------*/

#define IDE_TRC_DUMP_0   1           /* always do */
#define IDE_DUMP_0    IDE_TRC_DUMP_0,  IDE_DUMP, 0

/* ------------------------------------------------
 *  ERROR
 * ----------------------------------------------*/

#define IDE_TRC_ERR_0   1           /* always do */
#define IDE_ERR_0    IDE_TRC_ERR_0,  IDE_ERR, 0


/* PROJ-2473 SNMP 지원 */
/* ------------------------------------------------
 *  SNMP
 * ----------------------------------------------*/

#define IDE_TRC_SNMP_0   1           /* always do */
#define IDE_TRC_SNMP_1   (iduProperty::getSNMPTrcFlag() & 0x00000001)
#define IDE_TRC_SNMP_2   (iduProperty::getSNMPTrcFlag() & 0x00000002)
#define IDE_TRC_SNMP_3   (iduProperty::getSNMPTrcFlag() & 0x00000004)
#define IDE_TRC_SNMP_4   (iduProperty::getSNMPTrcFlag() & 0x00000008)

#define IDE_SNMP_0    IDE_TRC_SNMP_0, IDE_SNMP, 0
#define IDE_SNMP_1    IDE_TRC_SNMP_1, IDE_SNMP, 1
#define IDE_SNMP_2    IDE_TRC_SNMP_2, IDE_SNMP, 2
#define IDE_SNMP_3    IDE_TRC_SNMP_3, IDE_SNMP, 3
#define IDE_SNMP_4    IDE_TRC_SNMP_4, IDE_SNMP, 4


/* BUG-41909 */
/* ------------------------------------------------
 *  CM
 * ----------------------------------------------*/

#define IDE_TRC_CM_0   1           /* always do */
#define IDE_TRC_CM_1   (iduProperty::getCmTrcFlag() & 0x00000001)
#define IDE_TRC_CM_2   (iduProperty::getCmTrcFlag() & 0x00000002)
#define IDE_TRC_CM_3   (iduProperty::getCmTrcFlag() & 0x00000004)
#define IDE_TRC_CM_4   (iduProperty::getCmTrcFlag() & 0x00000008)

#define IDE_CM_0    IDE_TRC_CM_0,  IDE_CM, 0
#define IDE_CM_1    IDE_TRC_CM_1,  IDE_CM, 1
#define IDE_CM_2    IDE_TRC_CM_2,  IDE_CM, 2
#define IDE_CM_3    IDE_TRC_CM_3,  IDE_CM, 3
#define IDE_CM_4    IDE_TRC_CM_4,  IDE_CM, 4

#define IDE_MISC_0  1, IDE_MISC, 0

/* BUG-45274 */
/* ------------------------------------------------
 *  LoadBalancer
 * ----------------------------------------------*/
#define IDE_TRC_LB_0   1           /* always do */
#define IDE_TRC_LB_1   (iduProperty::getLbTrcFlag() & 0x00000001)
#define IDE_TRC_LB_2   (iduProperty::getLbTrcFlag() & 0x00000002)

#define IDE_LB_0    IDE_TRC_LB_0, IDE_LB, 0
#define IDE_LB_1    IDE_TRC_LB_1, IDE_LB, 1
#define IDE_LB_2    IDE_TRC_LB_2, IDE_LB, 2

/* PROJ-2581 */
/* ------------------------------------------------
 *  FIT
 * ----------------------------------------------*/
#if defined(ALTIBASE_FIT_CHECK)

#define IDE_TRC_FIT_0    1          /* always do */
#define IDE_FIT_0    IDE_TRC_FIT_0, IDE_FIT, 0

#endif

typedef enum ideLogModule
{
    IDE_ERR = 0,
#if defined(ALTIBASE_FIT_CHECK)
    IDE_FIT, /* PROJ-2581 */
#endif
    IDE_SERVER,
    IDE_SM,
    IDE_RP,
    IDE_QP,
    IDE_JOB, /* BUG-49108 */
    IDE_SD, /* BUG-46138 */
    IDE_DK,
    IDE_XA,
    IDE_MM, /* BUG-28866 */
    IDE_RP_CONFLICT,
    IDE_DUMP,
    IDE_SNMP, /* PROJ-2473 */
    IDE_CM,   /* BUG-41909 */
    /* 
     * Add additional module logs BEFORE miscellanous trc log
     * PROJ-2595
     */
    IDE_MISC,
    IDE_LB,   /* BUG-45274 */
    IDE_LOG_MAX_MODULE
}ideLogModule; 

class ideMsgLog;

/* Helps outputing trace log entry */
class ideLogEntry
{
public:
    ideLogEntry( UInt aChkFlag,
                 ideLogModule aModule,
                 UInt aLevel,
                 acp_bool_t aNewLine = ACP_TRUE );
    ideLogEntry( ideMsgLog *aMsgLog,
                 UInt aChkFlag,
                 UInt aLevel,
                 acp_bool_t aNewLine = ACP_TRUE );
    ~ideLogEntry();

    static inline ULong getLogSerial(void)
    {
        return acpAtomicInc64(&mLogSerial);
    }

    /* write()를 호출하지 않을 경우 파일에 기록되지 않는다. */
    IDE_RC write()
    {
        return logClose();
    }

    IDE_RC append( const acp_char_t *aStr );
    IDE_RC appendArgs( const acp_char_t *aFormat, va_list aArgs );

    IDE_RC appendFormat( const acp_char_t *aFormat, ... )
    {
        IDE_RC  sRC;
        va_list ap;

        va_start (ap, aFormat);
        sRC = this->appendArgs(aFormat, ap);
        va_end (ap);

        return sRC;
    }
    
    IDE_RC dumpHex(const UChar*, const UInt, const UInt);

    void setTailless(acp_bool_t aIsTailless) { mIsTailless = aIsTailless; }

private:
    void   logOpen();
    IDE_RC logClose();

    IDE_RC appendForce( const acp_char_t *aStr );

private:
    ideMsgLog *mMsgLog;
    UInt       mChkFlag;
    UInt       mLevel;

    acp_char_t  mBuffer[IDE_MESSAGE_SIZE]; /* buffer to store message */
    acp_size_t  mLength; /* length of the message */

    acp_bool_t  mIsTailless;
    acp_bool_t  mNewLine;  /* whether to append new line */

    acp_char_t* mHexDump; /* hexadecimal dump */
    acp_size_t  mDumpLength;

private:
    static acp_uint64_t mLogSerial;

    friend class ideMsgLog;
};

#endif /* _O_IDELOGENTRY_H_ */
