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
 * $Id: aciMsgLog.h 11319 2010-06-23 02:39:42Z djin $
 **********************************************************************/

/***********************************************************************
 * NAME
 *  ideMsgLog.h
 *
 * DESCRIPTION
 *  This file defines Error Log.
 *   
 * **********************************************************************/

#ifndef _O_ACIMSGLOG_H_
#define _O_ACIMSGLOG_H_ 1

#include <acp.h>

ACP_EXTERN_C_BEGIN

#define ACI_MSGLOG_FUNC(body)     ;
#define ACI_MSGLOG_ERROR(a, body) ;

/* ----------------------------------------------------------------------
 *  BootLog Function (altibase_boot.log)
 * ----------------------------------------------------------------------*/

#define ACI_MSGLOG_TYPE_MSG       0x00000001
#define ACI_MSGLOG_TYPE_RAW       0x00000002
#define ACI_MSGLOG_TYPE_THR       0x00000004
#define ACI_MSGLOG_TYPE_TIMESTAMP 0x00000008
#define ACI_MSGLOG_TYPE_ERROR     0x00000010

typedef enum aci_log_module_t
{
    ACI_SERVER = 0,
    ACI_SM,
    ACI_RP,
    ACI_QP,
    ACI_JOB, /* BUG-49108 */
    ACI_SD, /* BUG-46138 */
    ACI_DL,
    ACI_LK,
    ACI_XA,
    ACI_LOG_MAX_MODULE
} aci_log_module_t;

/* ------------------------------------------------
 *  SERVER 
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 * 
 * ----------------------------------------------*/

#define ACI_TRC_SERVER_0   1           /* always do */
#define ACI_TRC_SERVER_1   (iduProperty::getServerTrcFlag() & 0x00000001)
#define ACI_TRC_SERVER_2   (iduProperty::getServerTrcFlag() & 0x00000002)
#define ACI_TRC_SERVER_3   (iduProperty::getServerTrcFlag() & 0x00000004)
#define ACI_TRC_SERVER_4   (iduProperty::getServerTrcFlag() & 0x00000008)
#define ACI_TRC_SERVER_5   (iduProperty::getServerTrcFlag() & 0x00000010)
#define ACI_TRC_SERVER_6   (iduProperty::getServerTrcFlag() & 0x00000020)
#define ACI_TRC_SERVER_7   (iduProperty::getServerTrcFlag() & 0x00000040)
#define ACI_TRC_SERVER_8   (iduProperty::getServerTrcFlag() & 0x00000080)
#define ACI_TRC_SERVER_9   (iduProperty::getServerTrcFlag() & 0x00000100)
#define ACI_TRC_SERVER_10  (iduProperty::getServerTrcFlag() & 0x00000200)
#define ACI_TRC_SERVER_11  (iduProperty::getServerTrcFlag() & 0x00000400)
#define ACI_TRC_SERVER_12  (iduProperty::getServerTrcFlag() & 0x00000800)
#define ACI_TRC_SERVER_13  (iduProperty::getServerTrcFlag() & 0x00001000)
#define ACI_TRC_SERVER_14  (iduProperty::getServerTrcFlag() & 0x00002000)
#define ACI_TRC_SERVER_15  (iduProperty::getServerTrcFlag() & 0x00004000)
#define ACI_TRC_SERVER_16  (iduProperty::getServerTrcFlag() & 0x00008000)
#define ACI_TRC_SERVER_17  (iduProperty::getServerTrcFlag() & 0x00010000)
#define ACI_TRC_SERVER_18  (iduProperty::getServerTrcFlag() & 0x00020000)
#define ACI_TRC_SERVER_19  (iduProperty::getServerTrcFlag() & 0x00040000)
#define ACI_TRC_SERVER_20  (iduProperty::getServerTrcFlag() & 0x00080000)
#define ACI_TRC_SERVER_21  (iduProperty::getServerTrcFlag() & 0x00100000)
#define ACI_TRC_SERVER_22  (iduProperty::getServerTrcFlag() & 0x00200000)
#define ACI_TRC_SERVER_23  (iduProperty::getServerTrcFlag() & 0x00400000)
#define ACI_TRC_SERVER_24  (iduProperty::getServerTrcFlag() & 0x00800000)
#define ACI_TRC_SERVER_25  (iduProperty::getServerTrcFlag() & 0x01000000)
#define ACI_TRC_SERVER_26  (iduProperty::getServerTrcFlag() & 0x02000000)
#define ACI_TRC_SERVER_27  (iduProperty::getServerTrcFlag() & 0x04000000)
#define ACI_TRC_SERVER_28  (iduProperty::getServerTrcFlag() & 0x08000000)
#define ACI_TRC_SERVER_29  (iduProperty::getServerTrcFlag() & 0x10000000)
#define ACI_TRC_SERVER_30  (iduProperty::getServerTrcFlag() & 0x20000000)
#define ACI_TRC_SERVER_31  (iduProperty::getServerTrcFlag() & 0x40000000)
#define ACI_TRC_SERVER_32  (iduProperty::getServerTrcFlag() & 0x80000000)


#define ACI_SERVER_0    ACI_TRC_SERVER_0,  ACI_SERVER, 0
#define ACI_SERVER_1    ACI_TRC_SERVER_1,  ACI_SERVER, 1
#define ACI_SERVER_2    ACI_TRC_SERVER_2,  ACI_SERVER, 2 
#define ACI_SERVER_3    ACI_TRC_SERVER_3,  ACI_SERVER, 3 
#define ACI_SERVER_4    ACI_TRC_SERVER_4,  ACI_SERVER, 4 
#define ACI_SERVER_5    ACI_TRC_SERVER_5,  ACI_SERVER, 5 
#define ACI_SERVER_6    ACI_TRC_SERVER_6,  ACI_SERVER, 6 
#define ACI_SERVER_7    ACI_TRC_SERVER_7,  ACI_SERVER, 7 
#define ACI_SERVER_8    ACI_TRC_SERVER_8,  ACI_SERVER, 8 
#define ACI_SERVER_9    ACI_TRC_SERVER_9,  ACI_SERVER, 9 
#define ACI_SERVER_10   ACI_TRC_SERVER_10, ACI_SERVER, 10 
#define ACI_SERVER_11   ACI_TRC_SERVER_11, ACI_SERVER, 11 
#define ACI_SERVER_12   ACI_TRC_SERVER_12, ACI_SERVER, 12 
#define ACI_SERVER_13   ACI_TRC_SERVER_13, ACI_SERVER, 13 
#define ACI_SERVER_14   ACI_TRC_SERVER_14, ACI_SERVER, 14 
#define ACI_SERVER_15   ACI_TRC_SERVER_15, ACI_SERVER, 15 
#define ACI_SERVER_16   ACI_TRC_SERVER_16, ACI_SERVER, 16 
#define ACI_SERVER_17   ACI_TRC_SERVER_17, ACI_SERVER, 17 
#define ACI_SERVER_18   ACI_TRC_SERVER_18, ACI_SERVER, 18 
#define ACI_SERVER_19   ACI_TRC_SERVER_19, ACI_SERVER, 19
#define ACI_SERVER_20   ACI_TRC_SERVER_20, ACI_SERVER, 20 
#define ACI_SERVER_21   ACI_TRC_SERVER_21, ACI_SERVER, 21
#define ACI_SERVER_22   ACI_TRC_SERVER_22, ACI_SERVER, 22
#define ACI_SERVER_23   ACI_TRC_SERVER_23, ACI_SERVER, 23
#define ACI_SERVER_24   ACI_TRC_SERVER_24, ACI_SERVER, 24
#define ACI_SERVER_25   ACI_TRC_SERVER_25, ACI_SERVER, 25
#define ACI_SERVER_26   ACI_TRC_SERVER_26, ACI_SERVER, 26
#define ACI_SERVER_27   ACI_TRC_SERVER_27, ACI_SERVER, 27
#define ACI_SERVER_28   ACI_TRC_SERVER_28, ACI_SERVER, 28
#define ACI_SERVER_29   ACI_TRC_SERVER_29, ACI_SERVER, 29
#define ACI_SERVER_30   ACI_TRC_SERVER_30, ACI_SERVER, 30
#define ACI_SERVER_31   ACI_TRC_SERVER_31, ACI_SERVER, 31
#define ACI_SERVER_32   ACI_TRC_SERVER_32, ACI_SERVER, 32

/* ------------------------------------------------
 *  SM 
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 * 
 * ----------------------------------------------*/

#define ACI_TRC_SM_0   1           /* always do */
#define ACI_TRC_SM_1   (iduProperty::getSmTrcFlag() & 0x00000001)
#define ACI_TRC_SM_2   (iduProperty::getSmTrcFlag() & 0x00000002)
#define ACI_TRC_SM_3   (iduProperty::getSmTrcFlag() & 0x00000004)
#define ACI_TRC_SM_4   (iduProperty::getSmTrcFlag() & 0x00000008)
#define ACI_TRC_SM_5   (iduProperty::getSmTrcFlag() & 0x00000010)
#define ACI_TRC_SM_6   (iduProperty::getSmTrcFlag() & 0x00000020)
#define ACI_TRC_SM_7   (iduProperty::getSmTrcFlag() & 0x00000040)
#define ACI_TRC_SM_8   (iduProperty::getSmTrcFlag() & 0x00000080)
#define ACI_TRC_SM_9   (iduProperty::getSmTrcFlag() & 0x00000100)
#define ACI_TRC_SM_10  (iduProperty::getSmTrcFlag() & 0x00000200)
#define ACI_TRC_SM_11  (iduProperty::getSmTrcFlag() & 0x00000400)
#define ACI_TRC_SM_12  (iduProperty::getSmTrcFlag() & 0x00000800)
#define ACI_TRC_SM_13  (iduProperty::getSmTrcFlag() & 0x00001000)
#define ACI_TRC_SM_14  (iduProperty::getSmTrcFlag() & 0x00002000)
#define ACI_TRC_SM_15  (iduProperty::getSmTrcFlag() & 0x00004000)
#define ACI_TRC_SM_16  (iduProperty::getSmTrcFlag() & 0x00008000)
#define ACI_TRC_SM_17  (iduProperty::getSmTrcFlag() & 0x00010000)
#define ACI_TRC_SM_18  (iduProperty::getSmTrcFlag() & 0x00020000)
#define ACI_TRC_SM_19  (iduProperty::getSmTrcFlag() & 0x00040000)
#define ACI_TRC_SM_20  (iduProperty::getSmTrcFlag() & 0x00080000)
#define ACI_TRC_SM_21  (iduProperty::getSmTrcFlag() & 0x00100000)
#define ACI_TRC_SM_22  (iduProperty::getSmTrcFlag() & 0x00200000)
#define ACI_TRC_SM_23  (iduProperty::getSmTrcFlag() & 0x00400000)
#define ACI_TRC_SM_24  (iduProperty::getSmTrcFlag() & 0x00800000)
#define ACI_TRC_SM_25  (iduProperty::getSmTrcFlag() & 0x01000000)
#define ACI_TRC_SM_26  (iduProperty::getSmTrcFlag() & 0x02000000)
#define ACI_TRC_SM_27  (iduProperty::getSmTrcFlag() & 0x04000000)
#define ACI_TRC_SM_28  (iduProperty::getSmTrcFlag() & 0x08000000)
#define ACI_TRC_SM_29  (iduProperty::getSmTrcFlag() & 0x10000000)
#define ACI_TRC_SM_30  (iduProperty::getSmTrcFlag() & 0x20000000)
#define ACI_TRC_SM_31  (iduProperty::getSmTrcFlag() & 0x40000000)
#define ACI_TRC_SM_32  (iduProperty::getSmTrcFlag() & 0x80000000)


#define ACI_SM_0    ACI_TRC_SM_0,  ACI_SM, 0
#define ACI_SM_1    ACI_TRC_SM_1,  ACI_SM, 1
#define ACI_SM_2    ACI_TRC_SM_2,  ACI_SM, 2 
#define ACI_SM_3    ACI_TRC_SM_3,  ACI_SM, 3 
#define ACI_SM_4    ACI_TRC_SM_4,  ACI_SM, 4 
#define ACI_SM_5    ACI_TRC_SM_5,  ACI_SM, 5 
#define ACI_SM_6    ACI_TRC_SM_6,  ACI_SM, 6 
#define ACI_SM_7    ACI_TRC_SM_7,  ACI_SM, 7 
#define ACI_SM_8    ACI_TRC_SM_8,  ACI_SM, 8 
#define ACI_SM_9    ACI_TRC_SM_9,  ACI_SM, 9 
#define ACI_SM_10   ACI_TRC_SM_10, ACI_SM, 10 
#define ACI_SM_11   ACI_TRC_SM_11, ACI_SM, 11 
#define ACI_SM_12   ACI_TRC_SM_12, ACI_SM, 12 
#define ACI_SM_13   ACI_TRC_SM_13, ACI_SM, 13 
#define ACI_SM_14   ACI_TRC_SM_14, ACI_SM, 14 
#define ACI_SM_15   ACI_TRC_SM_15, ACI_SM, 15 
#define ACI_SM_16   ACI_TRC_SM_16, ACI_SM, 16 
#define ACI_SM_17   ACI_TRC_SM_17, ACI_SM, 17 
#define ACI_SM_18   ACI_TRC_SM_18, ACI_SM, 18 
#define ACI_SM_19   ACI_TRC_SM_19, ACI_SM, 19
#define ACI_SM_20   ACI_TRC_SM_20, ACI_SM, 20 
#define ACI_SM_21   ACI_TRC_SM_21, ACI_SM, 21
#define ACI_SM_22   ACI_TRC_SM_22, ACI_SM, 22
#define ACI_SM_23   ACI_TRC_SM_23, ACI_SM, 23
#define ACI_SM_24   ACI_TRC_SM_24, ACI_SM, 24
#define ACI_SM_25   ACI_TRC_SM_25, ACI_SM, 25
#define ACI_SM_26   ACI_TRC_SM_26, ACI_SM, 26
#define ACI_SM_27   ACI_TRC_SM_27, ACI_SM, 27
#define ACI_SM_28   ACI_TRC_SM_28, ACI_SM, 28
#define ACI_SM_29   ACI_TRC_SM_29, ACI_SM, 29
#define ACI_SM_30   ACI_TRC_SM_30, ACI_SM, 30
#define ACI_SM_31   ACI_TRC_SM_31, ACI_SM, 31
#define ACI_SM_32   ACI_TRC_SM_32, ACI_SM, 32

/* ------------------------------------------------
 *  RP 
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 * 
 * ----------------------------------------------*/

#define ACI_TRC_RP_0   1           /* always do */
#define ACI_TRC_RP_1   (iduProperty::getRpTrcFlag() & 0x00000001)
#define ACI_TRC_RP_2   (iduProperty::getRpTrcFlag() & 0x00000002)
#define ACI_TRC_RP_3   (iduProperty::getRpTrcFlag() & 0x00000004)
#define ACI_TRC_RP_4   (iduProperty::getRpTrcFlag() & 0x00000008)
#define ACI_TRC_RP_5   (iduProperty::getRpTrcFlag() & 0x00000010)
#define ACI_TRC_RP_6   (iduProperty::getRpTrcFlag() & 0x00000020)
#define ACI_TRC_RP_7   (iduProperty::getRpTrcFlag() & 0x00000040)
#define ACI_TRC_RP_8   (iduProperty::getRpTrcFlag() & 0x00000080)
#define ACI_TRC_RP_9   (iduProperty::getRpTrcFlag() & 0x00000100)
#define ACI_TRC_RP_10  (iduProperty::getRpTrcFlag() & 0x00000200)
#define ACI_TRC_RP_11  (iduProperty::getRpTrcFlag() & 0x00000400)
#define ACI_TRC_RP_12  (iduProperty::getRpTrcFlag() & 0x00000800)
#define ACI_TRC_RP_13  (iduProperty::getRpTrcFlag() & 0x00001000)
#define ACI_TRC_RP_14  (iduProperty::getRpTrcFlag() & 0x00002000)
#define ACI_TRC_RP_15  (iduProperty::getRpTrcFlag() & 0x00004000)
#define ACI_TRC_RP_16  (iduProperty::getRpTrcFlag() & 0x00008000)
#define ACI_TRC_RP_17  (iduProperty::getRpTrcFlag() & 0x00010000)
#define ACI_TRC_RP_18  (iduProperty::getRpTrcFlag() & 0x00020000)
#define ACI_TRC_RP_19  (iduProperty::getRpTrcFlag() & 0x00040000)
#define ACI_TRC_RP_20  (iduProperty::getRpTrcFlag() & 0x00080000)
#define ACI_TRC_RP_21  (iduProperty::getRpTrcFlag() & 0x00100000)
#define ACI_TRC_RP_22  (iduProperty::getRpTrcFlag() & 0x00200000)
#define ACI_TRC_RP_23  (iduProperty::getRpTrcFlag() & 0x00400000)
#define ACI_TRC_RP_24  (iduProperty::getRpTrcFlag() & 0x00800000)
#define ACI_TRC_RP_25  (iduProperty::getRpTrcFlag() & 0x01000000)
#define ACI_TRC_RP_26  (iduProperty::getRpTrcFlag() & 0x02000000)
#define ACI_TRC_RP_27  (iduProperty::getRpTrcFlag() & 0x04000000)
#define ACI_TRC_RP_28  (iduProperty::getRpTrcFlag() & 0x08000000)
#define ACI_TRC_RP_29  (iduProperty::getRpTrcFlag() & 0x10000000)
#define ACI_TRC_RP_30  (iduProperty::getRpTrcFlag() & 0x20000000)
#define ACI_TRC_RP_31  (iduProperty::getRpTrcFlag() & 0x40000000)
#define ACI_TRC_RP_32  (iduProperty::getRpTrcFlag() & 0x80000000)


#define ACI_RP_0    ACI_TRC_RP_0,  ACI_RP, 0
#define ACI_RP_1    ACI_TRC_RP_1,  ACI_RP, 1
#define ACI_RP_2    ACI_TRC_RP_2,  ACI_RP, 2 
#define ACI_RP_3    ACI_TRC_RP_3,  ACI_RP, 3 
#define ACI_RP_4    ACI_TRC_RP_4,  ACI_RP, 4 
#define ACI_RP_5    ACI_TRC_RP_5,  ACI_RP, 5 
#define ACI_RP_6    ACI_TRC_RP_6,  ACI_RP, 6 
#define ACI_RP_7    ACI_TRC_RP_7,  ACI_RP, 7 
#define ACI_RP_8    ACI_TRC_RP_8,  ACI_RP, 8 
#define ACI_RP_9    ACI_TRC_RP_9,  ACI_RP, 9 
#define ACI_RP_10   ACI_TRC_RP_10, ACI_RP, 10 
#define ACI_RP_11   ACI_TRC_RP_11, ACI_RP, 11 
#define ACI_RP_12   ACI_TRC_RP_12, ACI_RP, 12 
#define ACI_RP_13   ACI_TRC_RP_13, ACI_RP, 13 
#define ACI_RP_14   ACI_TRC_RP_14, ACI_RP, 14 
#define ACI_RP_15   ACI_TRC_RP_15, ACI_RP, 15 
#define ACI_RP_16   ACI_TRC_RP_16, ACI_RP, 16 
#define ACI_RP_17   ACI_TRC_RP_17, ACI_RP, 17 
#define ACI_RP_18   ACI_TRC_RP_18, ACI_RP, 18 
#define ACI_RP_19   ACI_TRC_RP_19, ACI_RP, 19
#define ACI_RP_20   ACI_TRC_RP_20, ACI_RP, 20 
#define ACI_RP_21   ACI_TRC_RP_21, ACI_RP, 21
#define ACI_RP_22   ACI_TRC_RP_22, ACI_RP, 22
#define ACI_RP_23   ACI_TRC_RP_23, ACI_RP, 23
#define ACI_RP_24   ACI_TRC_RP_24, ACI_RP, 24
#define ACI_RP_25   ACI_TRC_RP_25, ACI_RP, 25
#define ACI_RP_26   ACI_TRC_RP_26, ACI_RP, 26
#define ACI_RP_27   ACI_TRC_RP_27, ACI_RP, 27
#define ACI_RP_28   ACI_TRC_RP_28, ACI_RP, 28
#define ACI_RP_29   ACI_TRC_RP_29, ACI_RP, 29
#define ACI_RP_30   ACI_TRC_RP_30, ACI_RP, 30
#define ACI_RP_31   ACI_TRC_RP_31, ACI_RP, 31
#define ACI_RP_32   ACI_TRC_RP_32, ACI_RP, 32

/* ------------------------------------------------
 *  QP 
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 * 
 * ----------------------------------------------*/

#define ACI_TRC_QP_0   1           /* always do */
#define ACI_TRC_QP_1   (iduProperty::getQpTrcFlag() & 0x00000001)
#define ACI_TRC_QP_2   (iduProperty::getQpTrcFlag() & 0x00000002)
#define ACI_TRC_QP_3   (iduProperty::getQpTrcFlag() & 0x00000004)
#define ACI_TRC_QP_4   (iduProperty::getQpTrcFlag() & 0x00000008)
#define ACI_TRC_QP_5   (iduProperty::getQpTrcFlag() & 0x00000010)
#define ACI_TRC_QP_6   (iduProperty::getQpTrcFlag() & 0x00000020)
#define ACI_TRC_QP_7   (iduProperty::getQpTrcFlag() & 0x00000040)
#define ACI_TRC_QP_8   (iduProperty::getQpTrcFlag() & 0x00000080)
#define ACI_TRC_QP_9   (iduProperty::getQpTrcFlag() & 0x00000100)
#define ACI_TRC_QP_10  (iduProperty::getQpTrcFlag() & 0x00000200)
#define ACI_TRC_QP_11  (iduProperty::getQpTrcFlag() & 0x00000400)
#define ACI_TRC_QP_12  (iduProperty::getQpTrcFlag() & 0x00000800)
#define ACI_TRC_QP_13  (iduProperty::getQpTrcFlag() & 0x00001000)
#define ACI_TRC_QP_14  (iduProperty::getQpTrcFlag() & 0x00002000)
#define ACI_TRC_QP_15  (iduProperty::getQpTrcFlag() & 0x00004000)
#define ACI_TRC_QP_16  (iduProperty::getQpTrcFlag() & 0x00008000)
#define ACI_TRC_QP_17  (iduProperty::getQpTrcFlag() & 0x00010000)
#define ACI_TRC_QP_18  (iduProperty::getQpTrcFlag() & 0x00020000)
#define ACI_TRC_QP_19  (iduProperty::getQpTrcFlag() & 0x00040000)
#define ACI_TRC_QP_20  (iduProperty::getQpTrcFlag() & 0x00080000)
#define ACI_TRC_QP_21  (iduProperty::getQpTrcFlag() & 0x00100000)
#define ACI_TRC_QP_22  (iduProperty::getQpTrcFlag() & 0x00200000)
#define ACI_TRC_QP_23  (iduProperty::getQpTrcFlag() & 0x00400000)
#define ACI_TRC_QP_24  (iduProperty::getQpTrcFlag() & 0x00800000)
#define ACI_TRC_QP_25  (iduProperty::getQpTrcFlag() & 0x01000000)
#define ACI_TRC_QP_26  (iduProperty::getQpTrcFlag() & 0x02000000)
#define ACI_TRC_QP_27  (iduProperty::getQpTrcFlag() & 0x04000000)
#define ACI_TRC_QP_28  (iduProperty::getQpTrcFlag() & 0x08000000)
#define ACI_TRC_QP_29  (iduProperty::getQpTrcFlag() & 0x10000000)
#define ACI_TRC_QP_30  (iduProperty::getQpTrcFlag() & 0x20000000)
#define ACI_TRC_QP_31  (iduProperty::getQpTrcFlag() & 0x40000000)
#define ACI_TRC_QP_32  (iduProperty::getQpTrcFlag() & 0x80000000)


#define ACI_QP_0    ACI_TRC_QP_0,  ACI_QP, 0
#define ACI_QP_1    ACI_TRC_QP_1,  ACI_QP, 1
#define ACI_QP_2    ACI_TRC_QP_2,  ACI_QP, 2 
#define ACI_QP_3    ACI_TRC_QP_3,  ACI_QP, 3 
#define ACI_QP_4    ACI_TRC_QP_4,  ACI_QP, 4 
#define ACI_QP_5    ACI_TRC_QP_5,  ACI_QP, 5 
#define ACI_QP_6    ACI_TRC_QP_6,  ACI_QP, 6 
#define ACI_QP_7    ACI_TRC_QP_7,  ACI_QP, 7 
#define ACI_QP_8    ACI_TRC_QP_8,  ACI_QP, 8 
#define ACI_QP_9    ACI_TRC_QP_9,  ACI_QP, 9 
#define ACI_QP_10   ACI_TRC_QP_10, ACI_QP, 10 
#define ACI_QP_11   ACI_TRC_QP_11, ACI_QP, 11 
#define ACI_QP_12   ACI_TRC_QP_12, ACI_QP, 12 
#define ACI_QP_13   ACI_TRC_QP_13, ACI_QP, 13 
#define ACI_QP_14   ACI_TRC_QP_14, ACI_QP, 14 
#define ACI_QP_15   ACI_TRC_QP_15, ACI_QP, 15 
#define ACI_QP_16   ACI_TRC_QP_16, ACI_QP, 16 
#define ACI_QP_17   ACI_TRC_QP_17, ACI_QP, 17 
#define ACI_QP_18   ACI_TRC_QP_18, ACI_QP, 18 
#define ACI_QP_19   ACI_TRC_QP_19, ACI_QP, 19
#define ACI_QP_20   ACI_TRC_QP_20, ACI_QP, 20 
#define ACI_QP_21   ACI_TRC_QP_21, ACI_QP, 21
#define ACI_QP_22   ACI_TRC_QP_22, ACI_QP, 22
#define ACI_QP_23   ACI_TRC_QP_23, ACI_QP, 23
#define ACI_QP_24   ACI_TRC_QP_24, ACI_QP, 24
#define ACI_QP_25   ACI_TRC_QP_25, ACI_QP, 25
#define ACI_QP_26   ACI_TRC_QP_26, ACI_QP, 26
#define ACI_QP_27   ACI_TRC_QP_27, ACI_QP, 27
#define ACI_QP_28   ACI_TRC_QP_28, ACI_QP, 28
#define ACI_QP_29   ACI_TRC_QP_29, ACI_QP, 29
#define ACI_QP_30   ACI_TRC_QP_30, ACI_QP, 30
#define ACI_QP_31   ACI_TRC_QP_31, ACI_QP, 31
#define ACI_QP_32   ACI_TRC_QP_32, ACI_QP, 32

/* ------------------------------------------------
 *  JOB
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 * 
 * ----------------------------------------------*/

#define ACI_TRC_JOB_0   1           /* always do */
#define ACI_TRC_JOB_1   (iduProperty::getJobTrcFlag() & 0x00000001)
#define ACI_TRC_JOB_2   (iduProperty::getJobTrcFlag() & 0x00000002)
#define ACI_TRC_JOB_3   (iduProperty::getJobTrcFlag() & 0x00000004)
#define ACI_TRC_JOB_4   (iduProperty::getJobTrcFlag() & 0x00000008)
#define ACI_TRC_JOB_5   (iduProperty::getJobTrcFlag() & 0x00000010)
#define ACI_TRC_JOB_6   (iduProperty::getJobTrcFlag() & 0x00000020)
#define ACI_TRC_JOB_7   (iduProperty::getJobTrcFlag() & 0x00000040)
#define ACI_TRC_JOB_8   (iduProperty::getJobTrcFlag() & 0x00000080)
#define ACI_TRC_JOB_9   (iduProperty::getJobTrcFlag() & 0x00000100)
#define ACI_TRC_JOB_10  (iduProperty::getJobTrcFlag() & 0x00000200)
#define ACI_TRC_JOB_11  (iduProperty::getJobTrcFlag() & 0x00000400)
#define ACI_TRC_JOB_12  (iduProperty::getJobTrcFlag() & 0x00000800)
#define ACI_TRC_JOB_13  (iduProperty::getJobTrcFlag() & 0x00001000)
#define ACI_TRC_JOB_14  (iduProperty::getJobTrcFlag() & 0x00002000)
#define ACI_TRC_JOB_15  (iduProperty::getJobTrcFlag() & 0x00004000)
#define ACI_TRC_JOB_16  (iduProperty::getJobTrcFlag() & 0x00008000)
#define ACI_TRC_JOB_17  (iduProperty::getJobTrcFlag() & 0x00010000)
#define ACI_TRC_JOB_18  (iduProperty::getJobTrcFlag() & 0x00020000)
#define ACI_TRC_JOB_19  (iduProperty::getJobTrcFlag() & 0x00040000)
#define ACI_TRC_JOB_20  (iduProperty::getJobTrcFlag() & 0x00080000)
#define ACI_TRC_JOB_21  (iduProperty::getJobTrcFlag() & 0x00100000)
#define ACI_TRC_JOB_22  (iduProperty::getJobTrcFlag() & 0x00200000)
#define ACI_TRC_JOB_23  (iduProperty::getJobTrcFlag() & 0x00400000)
#define ACI_TRC_JOB_24  (iduProperty::getJobTrcFlag() & 0x00800000)
#define ACI_TRC_JOB_25  (iduProperty::getJobTrcFlag() & 0x01000000)
#define ACI_TRC_JOB_26  (iduProperty::getJobTrcFlag() & 0x02000000)
#define ACI_TRC_JOB_27  (iduProperty::getJobTrcFlag() & 0x04000000)
#define ACI_TRC_JOB_28  (iduProperty::getJobTrcFlag() & 0x08000000)
#define ACI_TRC_JOB_29  (iduProperty::getJobTrcFlag() & 0x10000000)
#define ACI_TRC_JOB_30  (iduProperty::getJobTrcFlag() & 0x20000000)
#define ACI_TRC_JOB_31  (iduProperty::getJobTrcFlag() & 0x40000000)
#define ACI_TRC_JOB_32  (iduProperty::getJobTrcFlag() & 0x80000000)


#define ACI_JOB_0    ACI_TRC_JOB_0,  ACI_JOB, 0
#define ACI_JOB_1    ACI_TRC_JOB_1,  ACI_JOB, 1
#define ACI_JOB_2    ACI_TRC_JOB_2,  ACI_JOB, 2 
#define ACI_JOB_3    ACI_TRC_JOB_3,  ACI_JOB, 3 
#define ACI_JOB_4    ACI_TRC_JOB_4,  ACI_JOB, 4 
#define ACI_JOB_5    ACI_TRC_JOB_5,  ACI_JOB, 5 
#define ACI_JOB_6    ACI_TRC_JOB_6,  ACI_JOB, 6 
#define ACI_JOB_7    ACI_TRC_JOB_7,  ACI_JOB, 7 
#define ACI_JOB_8    ACI_TRC_JOB_8,  ACI_JOB, 8 
#define ACI_JOB_9    ACI_TRC_JOB_9,  ACI_JOB, 9 
#define ACI_JOB_10   ACI_TRC_JOB_10, ACI_JOB, 10 
#define ACI_JOB_11   ACI_TRC_JOB_11, ACI_JOB, 11 
#define ACI_JOB_12   ACI_TRC_JOB_12, ACI_JOB, 12 
#define ACI_JOB_13   ACI_TRC_JOB_13, ACI_JOB, 13 
#define ACI_JOB_14   ACI_TRC_JOB_14, ACI_JOB, 14 
#define ACI_JOB_15   ACI_TRC_JOB_15, ACI_JOB, 15 
#define ACI_JOB_16   ACI_TRC_JOB_16, ACI_JOB, 16 
#define ACI_JOB_17   ACI_TRC_JOB_17, ACI_JOB, 17 
#define ACI_JOB_18   ACI_TRC_JOB_18, ACI_JOB, 18 
#define ACI_JOB_19   ACI_TRC_JOB_19, ACI_JOB, 19
#define ACI_JOB_20   ACI_TRC_JOB_20, ACI_JOB, 20 
#define ACI_JOB_21   ACI_TRC_JOB_21, ACI_JOB, 21
#define ACI_JOB_22   ACI_TRC_JOB_22, ACI_JOB, 22
#define ACI_JOB_23   ACI_TRC_JOB_23, ACI_JOB, 23
#define ACI_JOB_24   ACI_TRC_JOB_24, ACI_JOB, 24
#define ACI_JOB_25   ACI_TRC_JOB_25, ACI_JOB, 25
#define ACI_JOB_26   ACI_TRC_JOB_26, ACI_JOB, 26
#define ACI_JOB_27   ACI_TRC_JOB_27, ACI_JOB, 27
#define ACI_JOB_28   ACI_TRC_JOB_28, ACI_JOB, 28
#define ACI_JOB_29   ACI_TRC_JOB_29, ACI_JOB, 29
#define ACI_JOB_30   ACI_TRC_JOB_30, ACI_JOB, 30
#define ACI_JOB_31   ACI_TRC_JOB_31, ACI_JOB, 31
#define ACI_JOB_32   ACI_TRC_JOB_32, ACI_JOB, 32

/* ------------------------------------------------
 *  SD  BUG-46138 
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 * ----------------------------------------------*/

#define ACI_TRC_SD_0   1           /* always do */
#define ACI_TRC_SD_1   (iduProperty::getSdTrcFlag() & 0x00000001)
#define ACI_TRC_SD_2   (iduProperty::getSdTrcFlag() & 0x00000002)
#define ACI_TRC_SD_3   (iduProperty::getSdTrcFlag() & 0x00000004)
#define ACI_TRC_SD_4   (iduProperty::getSdTrcFlag() & 0x00000008)
#define ACI_TRC_SD_5   (iduProperty::getSdTrcFlag() & 0x00000010)
#define ACI_TRC_SD_6   (iduProperty::getSdTrcFlag() & 0x00000020)
#define ACI_TRC_SD_7   (iduProperty::getSdTrcFlag() & 0x00000040)
#define ACI_TRC_SD_8   (iduProperty::getSdTrcFlag() & 0x00000080)
#define ACI_TRC_SD_9   (iduProperty::getSdTrcFlag() & 0x00000100)
#define ACI_TRC_SD_10  (iduProperty::getSdTrcFlag() & 0x00000200)
#define ACI_TRC_SD_11  (iduProperty::getSdTrcFlag() & 0x00000400)
#define ACI_TRC_SD_12  (iduProperty::getSdTrcFlag() & 0x00000800)
#define ACI_TRC_SD_13  (iduProperty::getSdTrcFlag() & 0x00001000)
#define ACI_TRC_SD_14  (iduProperty::getSdTrcFlag() & 0x00002000)
#define ACI_TRC_SD_15  (iduProperty::getSdTrcFlag() & 0x00004000)
#define ACI_TRC_SD_16  (iduProperty::getSdTrcFlag() & 0x00008000)
#define ACI_TRC_SD_17  (iduProperty::getSdTrcFlag() & 0x00010000)
#define ACI_TRC_SD_18  (iduProperty::getSdTrcFlag() & 0x00020000)
#define ACI_TRC_SD_19  (iduProperty::getSdTrcFlag() & 0x00040000)
#define ACI_TRC_SD_20  (iduProperty::getSdTrcFlag() & 0x00080000)
#define ACI_TRC_SD_21  (iduProperty::getSdTrcFlag() & 0x00100000)
#define ACI_TRC_SD_22  (iduProperty::getSdTrcFlag() & 0x00200000)
#define ACI_TRC_SD_23  (iduProperty::getSdTrcFlag() & 0x00400000)
#define ACI_TRC_SD_24  (iduProperty::getSdTrcFlag() & 0x00800000)
#define ACI_TRC_SD_25  (iduProperty::getSdTrcFlag() & 0x01000000)
#define ACI_TRC_SD_26  (iduProperty::getSdTrcFlag() & 0x02000000)
#define ACI_TRC_SD_27  (iduProperty::getSdTrcFlag() & 0x04000000)
#define ACI_TRC_SD_28  (iduProperty::getSdTrcFlag() & 0x08000000)
#define ACI_TRC_SD_29  (iduProperty::getSdTrcFlag() & 0x10000000)
#define ACI_TRC_SD_30  (iduProperty::getSdTrcFlag() & 0x20000000)
#define ACI_TRC_SD_31  (iduProperty::getSdTrcFlag() & 0x40000000)
#define ACI_TRC_SD_32  (iduProperty::getSdTrcFlag() & 0x80000000)


#define ACI_SD_0    ACI_TRC_SD_0,  ACI_SD, 0
#define ACI_SD_1    ACI_TRC_SD_1,  ACI_SD, 1
#define ACI_SD_2    ACI_TRC_SD_2,  ACI_SD, 2 
#define ACI_SD_3    ACI_TRC_SD_3,  ACI_SD, 3 
#define ACI_SD_4    ACI_TRC_SD_4,  ACI_SD, 4 
#define ACI_SD_5    ACI_TRC_SD_5,  ACI_SD, 5 
#define ACI_SD_6    ACI_TRC_SD_6,  ACI_SD, 6 
#define ACI_SD_7    ACI_TRC_SD_7,  ACI_SD, 7 
#define ACI_SD_8    ACI_TRC_SD_8,  ACI_SD, 8 
#define ACI_SD_9    ACI_TRC_SD_9,  ACI_SD, 9 
#define ACI_SD_10   ACI_TRC_SD_10, ACI_SD, 10 
#define ACI_SD_11   ACI_TRC_SD_11, ACI_SD, 11 
#define ACI_SD_12   ACI_TRC_SD_12, ACI_SD, 12 
#define ACI_SD_13   ACI_TRC_SD_13, ACI_SD, 13 
#define ACI_SD_14   ACI_TRC_SD_14, ACI_SD, 14 
#define ACI_SD_15   ACI_TRC_SD_15, ACI_SD, 15 
#define ACI_SD_16   ACI_TRC_SD_16, ACI_SD, 16 
#define ACI_SD_17   ACI_TRC_SD_17, ACI_SD, 17 
#define ACI_SD_18   ACI_TRC_SD_18, ACI_SD, 18 
#define ACI_SD_19   ACI_TRC_SD_19, ACI_SD, 19
#define ACI_SD_20   ACI_TRC_SD_20, ACI_SD, 20 
#define ACI_SD_21   ACI_TRC_SD_21, ACI_SD, 21
#define ACI_SD_22   ACI_TRC_SD_22, ACI_SD, 22
#define ACI_SD_23   ACI_TRC_SD_23, ACI_SD, 23
#define ACI_SD_24   ACI_TRC_SD_24, ACI_SD, 24
#define ACI_SD_25   ACI_TRC_SD_25, ACI_SD, 25
#define ACI_SD_26   ACI_TRC_SD_26, ACI_SD, 26
#define ACI_SD_27   ACI_TRC_SD_27, ACI_SD, 27
#define ACI_SD_28   ACI_TRC_SD_28, ACI_SD, 28
#define ACI_SD_29   ACI_TRC_SD_29, ACI_SD, 29
#define ACI_SD_30   ACI_TRC_SD_30, ACI_SD, 30
#define ACI_SD_31   ACI_TRC_SD_31, ACI_SD, 31
#define ACI_SD_32   ACI_TRC_SD_32, ACI_SD, 32

/* ------------------------------------------------
 *  DL(Database Link:Server side)
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define ACI_TRC_DL_0   1           /* always do */
#define ACI_TRC_DL_1   (iduProperty::getDlTrcFlag() & 0x00000001)
#define ACI_TRC_DL_2   (iduProperty::getDlTrcFlag() & 0x00000002)
#define ACI_TRC_DL_3   (iduProperty::getDlTrcFlag() & 0x00000004)
#define ACI_TRC_DL_4   (iduProperty::getDlTrcFlag() & 0x00000008)
#define ACI_TRC_DL_5   (iduProperty::getDlTrcFlag() & 0x00000010)
#define ACI_TRC_DL_6   (iduProperty::getDlTrcFlag() & 0x00000020)
#define ACI_TRC_DL_7   (iduProperty::getDlTrcFlag() & 0x00000040)
#define ACI_TRC_DL_8   (iduProperty::getDlTrcFlag() & 0x00000080)
#define ACI_TRC_DL_9   (iduProperty::getDlTrcFlag() & 0x00000100)
#define ACI_TRC_DL_10  (iduProperty::getDlTrcFlag() & 0x00000200)
#define ACI_TRC_DL_11  (iduProperty::getDlTrcFlag() & 0x00000400)
#define ACI_TRC_DL_12  (iduProperty::getDlTrcFlag() & 0x00000800)
#define ACI_TRC_DL_13  (iduProperty::getDlTrcFlag() & 0x00001000)
#define ACI_TRC_DL_14  (iduProperty::getDlTrcFlag() & 0x00002000)
#define ACI_TRC_DL_15  (iduProperty::getDlTrcFlag() & 0x00004000)
#define ACI_TRC_DL_16  (iduProperty::getDlTrcFlag() & 0x00008000)
#define ACI_TRC_DL_17  (iduProperty::getDlTrcFlag() & 0x00010000)
#define ACI_TRC_DL_18  (iduProperty::getDlTrcFlag() & 0x00020000)
#define ACI_TRC_DL_19  (iduProperty::getDlTrcFlag() & 0x00040000)
#define ACI_TRC_DL_20  (iduProperty::getDlTrcFlag() & 0x00080000)
#define ACI_TRC_DL_21  (iduProperty::getDlTrcFlag() & 0x00100000)
#define ACI_TRC_DL_22  (iduProperty::getDlTrcFlag() & 0x00200000)
#define ACI_TRC_DL_23  (iduProperty::getDlTrcFlag() & 0x00400000)
#define ACI_TRC_DL_24  (iduProperty::getDlTrcFlag() & 0x00800000)
#define ACI_TRC_DL_25  (iduProperty::getDlTrcFlag() & 0x01000000)
#define ACI_TRC_DL_26  (iduProperty::getDlTrcFlag() & 0x02000000)
#define ACI_TRC_DL_27  (iduProperty::getDlTrcFlag() & 0x04000000)
#define ACI_TRC_DL_28  (iduProperty::getDlTrcFlag() & 0x08000000)
#define ACI_TRC_DL_29  (iduProperty::getDlTrcFlag() & 0x10000000)
#define ACI_TRC_DL_30  (iduProperty::getDlTrcFlag() & 0x20000000)
#define ACI_TRC_DL_31  (iduProperty::getDlTrcFlag() & 0x40000000)
#define ACI_TRC_DL_32  (iduProperty::getDlTrcFlag() & 0x80000000)

#define ACI_DL_0    ACI_TRC_DL_0,  ACI_DL, 0
#define ACI_DL_1    ACI_TRC_DL_1,  ACI_DL, 1
#define ACI_DL_2    ACI_TRC_DL_2,  ACI_DL, 2 
#define ACI_DL_3    ACI_TRC_DL_3,  ACI_DL, 3 
#define ACI_DL_4    ACI_TRC_DL_4,  ACI_DL, 4 
#define ACI_DL_5    ACI_TRC_DL_5,  ACI_DL, 5 
#define ACI_DL_6    ACI_TRC_DL_6,  ACI_DL, 6 
#define ACI_DL_7    ACI_TRC_DL_7,  ACI_DL, 7 
#define ACI_DL_8    ACI_TRC_DL_8,  ACI_DL, 8 
#define ACI_DL_9    ACI_TRC_DL_9,  ACI_DL, 9 
#define ACI_DL_10   ACI_TRC_DL_10, ACI_DL, 10 
#define ACI_DL_11   ACI_TRC_DL_11, ACI_DL, 11 
#define ACI_DL_12   ACI_TRC_DL_12, ACI_DL, 12 
#define ACI_DL_13   ACI_TRC_DL_13, ACI_DL, 13 
#define ACI_DL_14   ACI_TRC_DL_14, ACI_DL, 14 
#define ACI_DL_15   ACI_TRC_DL_15, ACI_DL, 15 
#define ACI_DL_16   ACI_TRC_DL_16, ACI_DL, 16 
#define ACI_DL_17   ACI_TRC_DL_17, ACI_DL, 17 
#define ACI_DL_18   ACI_TRC_DL_18, ACI_DL, 18 
#define ACI_DL_19   ACI_TRC_DL_19, ACI_DL, 19
#define ACI_DL_20   ACI_TRC_DL_20, ACI_DL, 20 
#define ACI_DL_21   ACI_TRC_DL_21, ACI_DL, 21
#define ACI_DL_22   ACI_TRC_DL_22, ACI_DL, 22
#define ACI_DL_23   ACI_TRC_DL_23, ACI_DL, 23
#define ACI_DL_24   ACI_TRC_DL_24, ACI_DL, 24
#define ACI_DL_25   ACI_TRC_DL_25, ACI_DL, 25
#define ACI_DL_26   ACI_TRC_DL_26, ACI_DL, 26
#define ACI_DL_27   ACI_TRC_DL_27, ACI_DL, 27
#define ACI_DL_28   ACI_TRC_DL_28, ACI_DL, 28
#define ACI_DL_29   ACI_TRC_DL_29, ACI_DL, 29
#define ACI_DL_30   ACI_TRC_DL_30, ACI_DL, 30
#define ACI_DL_31   ACI_TRC_DL_31, ACI_DL, 31
#define ACI_DL_32   ACI_TRC_DL_32, ACI_DL, 32

/* ------------------------------------------------
 *  LK(Database Link:Linker side)
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define ACI_TRC_LK_0   1           /* always do */
#define ACI_TRC_LK_1   (iduProperty::getLkTrcFlag() & 0x00000001)
#define ACI_TRC_LK_2   (iduProperty::getLkTrcFlag() & 0x00000002)
#define ACI_TRC_LK_3   (iduProperty::getLkTrcFlag() & 0x00000004)
#define ACI_TRC_LK_4   (iduProperty::getLkTrcFlag() & 0x00000008)
#define ACI_TRC_LK_5   (iduProperty::getLkTrcFlag() & 0x00000010)
#define ACI_TRC_LK_6   (iduProperty::getLkTrcFlag() & 0x00000020)
#define ACI_TRC_LK_7   (iduProperty::getLkTrcFlag() & 0x00000040)
#define ACI_TRC_LK_8   (iduProperty::getLkTrcFlag() & 0x00000080)
#define ACI_TRC_LK_9   (iduProperty::getLkTrcFlag() & 0x00000100)
#define ACI_TRC_LK_10  (iduProperty::getLkTrcFlag() & 0x00000200)
#define ACI_TRC_LK_11  (iduProperty::getLkTrcFlag() & 0x00000400)
#define ACI_TRC_LK_12  (iduProperty::getLkTrcFlag() & 0x00000800)
#define ACI_TRC_LK_13  (iduProperty::getLkTrcFlag() & 0x00001000)
#define ACI_TRC_LK_14  (iduProperty::getLkTrcFlag() & 0x00002000)
#define ACI_TRC_LK_15  (iduProperty::getLkTrcFlag() & 0x00004000)
#define ACI_TRC_LK_16  (iduProperty::getLkTrcFlag() & 0x00008000)
#define ACI_TRC_LK_17  (iduProperty::getLkTrcFlag() & 0x00010000)
#define ACI_TRC_LK_18  (iduProperty::getLkTrcFlag() & 0x00020000)
#define ACI_TRC_LK_19  (iduProperty::getLkTrcFlag() & 0x00040000)
#define ACI_TRC_LK_20  (iduProperty::getLkTrcFlag() & 0x00080000)
#define ACI_TRC_LK_21  (iduProperty::getLkTrcFlag() & 0x00100000)
#define ACI_TRC_LK_22  (iduProperty::getLkTrcFlag() & 0x00200000)
#define ACI_TRC_LK_23  (iduProperty::getLkTrcFlag() & 0x00400000)
#define ACI_TRC_LK_24  (iduProperty::getLkTrcFlag() & 0x00800000)
#define ACI_TRC_LK_25  (iduProperty::getLkTrcFlag() & 0x01000000)
#define ACI_TRC_LK_26  (iduProperty::getLkTrcFlag() & 0x02000000)
#define ACI_TRC_LK_27  (iduProperty::getLkTrcFlag() & 0x04000000)
#define ACI_TRC_LK_28  (iduProperty::getLkTrcFlag() & 0x08000000)
#define ACI_TRC_LK_29  (iduProperty::getLkTrcFlag() & 0x10000000)
#define ACI_TRC_LK_30  (iduProperty::getLkTrcFlag() & 0x20000000)
#define ACI_TRC_LK_31  (iduProperty::getLkTrcFlag() & 0x40000000)
#define ACI_TRC_LK_32  (iduProperty::getLkTrcFlag() & 0x80000000)


#define ACI_LK_0    ACI_TRC_LK_0,  ACI_LK, 0
#define ACI_LK_1    ACI_TRC_LK_1,  ACI_LK, 1
#define ACI_LK_2    ACI_TRC_LK_2,  ACI_LK, 2 
#define ACI_LK_3    ACI_TRC_LK_3,  ACI_LK, 3 
#define ACI_LK_4    ACI_TRC_LK_4,  ACI_LK, 4 
#define ACI_LK_5    ACI_TRC_LK_5,  ACI_LK, 5 
#define ACI_LK_6    ACI_TRC_LK_6,  ACI_LK, 6 
#define ACI_LK_7    ACI_TRC_LK_7,  ACI_LK, 7 
#define ACI_LK_8    ACI_TRC_LK_8,  ACI_LK, 8 
#define ACI_LK_9    ACI_TRC_LK_9,  ACI_LK, 9 
#define ACI_LK_10   ACI_TRC_LK_10, ACI_LK, 10 
#define ACI_LK_11   ACI_TRC_LK_11, ACI_LK, 11 
#define ACI_LK_12   ACI_TRC_LK_12, ACI_LK, 12 
#define ACI_LK_13   ACI_TRC_LK_13, ACI_LK, 13 
#define ACI_LK_14   ACI_TRC_LK_14, ACI_LK, 14 
#define ACI_LK_15   ACI_TRC_LK_15, ACI_LK, 15 
#define ACI_LK_16   ACI_TRC_LK_16, ACI_LK, 16 
#define ACI_LK_17   ACI_TRC_LK_17, ACI_LK, 17 
#define ACI_LK_18   ACI_TRC_LK_18, ACI_LK, 18 
#define ACI_LK_19   ACI_TRC_LK_19, ACI_LK, 19
#define ACI_LK_20   ACI_TRC_LK_20, ACI_LK, 20 
#define ACI_LK_21   ACI_TRC_LK_21, ACI_LK, 21
#define ACI_LK_22   ACI_TRC_LK_22, ACI_LK, 22
#define ACI_LK_23   ACI_TRC_LK_23, ACI_LK, 23
#define ACI_LK_24   ACI_TRC_LK_24, ACI_LK, 24
#define ACI_LK_25   ACI_TRC_LK_25, ACI_LK, 25
#define ACI_LK_26   ACI_TRC_LK_26, ACI_LK, 26
#define ACI_LK_27   ACI_TRC_LK_27, ACI_LK, 27
#define ACI_LK_28   ACI_TRC_LK_28, ACI_LK, 28
#define ACI_LK_29   ACI_TRC_LK_29, ACI_LK, 29
#define ACI_LK_30   ACI_TRC_LK_30, ACI_LK, 30
#define ACI_LK_31   ACI_TRC_LK_31, ACI_LK, 31
#define ACI_LK_32   ACI_TRC_LK_32, ACI_LK, 32

/* bug-24840 divide xa log */
/* ------------------------------------------------
 *  XA
 *  Macro: ACI_*_LEV   ==> DO_FLAG,MODULE,LEVEL
 *
 * ----------------------------------------------*/

#define ACI_TRC_XA_0   1           /* always do */
#define ACI_TRC_XA_1   (iduProperty::getXaTrcFlag() & 0x00000001)
#define ACI_TRC_XA_2   (iduProperty::getXaTrcFlag() & 0x00000002)
#define ACI_TRC_XA_3   (iduProperty::getXaTrcFlag() & 0x00000004)
#define ACI_TRC_XA_4   (iduProperty::getXaTrcFlag() & 0x00000008)
#define ACI_TRC_XA_5   (iduProperty::getXaTrcFlag() & 0x00000010)
#define ACI_TRC_XA_6   (iduProperty::getXaTrcFlag() & 0x00000020)
#define ACI_TRC_XA_7   (iduProperty::getXaTrcFlag() & 0x00000040)
#define ACI_TRC_XA_8   (iduProperty::getXaTrcFlag() & 0x00000080)

#define ACI_XA_0    ACI_TRC_XA_0,  ACI_XA, 0
#define ACI_XA_1    ACI_TRC_XA_1,  ACI_XA, 1
#define ACI_XA_2    ACI_TRC_XA_2,  ACI_XA, 2 
#define ACI_XA_3    ACI_TRC_XA_3,  ACI_XA, 3 
#define ACI_XA_4    ACI_TRC_XA_4,  ACI_XA, 4 
#define ACI_XA_5    ACI_TRC_XA_5,  ACI_XA, 5 
#define ACI_XA_6    ACI_TRC_XA_6,  ACI_XA, 6 
#define ACI_XA_7    ACI_TRC_XA_7,  ACI_XA, 7 
#define ACI_XA_8    ACI_TRC_XA_8,  ACI_XA, 8 

/* ------------------------------------------------
 *  aci_log_module_t 로부터 Module 번호와 레벨을 얻는다.
 * ----------------------------------------------*/

#define ACI_GET_TRC_MODULE(a)  ((a) >> 32)
#define ACI_GET_TRC_LEVEL(a)   ((a) & ID_ULONG(0x00000000FFFFFFFF))


/* ---------------------------------------------
 * TASK-4007 [SM]PBT를 위한 기능 추가
 * 
 * Hexa code로 Dump해주는 기능 추가 
 * --------------------------------------------- */

/*   SRC는 덤프 대상의 크기, DEST는 덤프되어 저장되는
 * 버퍼의 크기입니다. SRC 크기가 다를 경우에는 LIMIT
 * 까지 출력하고, DEST 크기가 다를 경우에는 아예 출력
 * 하지 않습니다.
 *   SRC 크기가 다를 경우는 어차피 값을 읽는데 문제가
 * 없지만, DEST크기가 다를 경우에는 출력 대상이 되는
 * 버퍼를 신뢰할 수 없기 때문입니다.
 */
#define ACI_DUMP_SRC_LIMIT  ( 64*1024)
#define ACI_DUMP_DEST_LIMIT (256*1024)

/* Format flag */

/* 개행 공백 등을 통해 BinaryBody를 어떻게 구분해줄지를 설정합니다. */
#define ACI_DUMP_FORMAT_PIECE_MASK       (0x00000001) 
#define ACI_DUMP_FORMAT_PIECE_SINGLE     (0x00000000) /* 단일한 한 조각으로 출력합니다. */
#define ACI_DUMP_FORMAT_PIECE_4BYTE      (0x00000001) /* 4Byte단위로 구분합니다.*/

/* 절대주소 또는 상대주소를 출력해줍니다. */
#define ACI_DUMP_FORMAT_ADDR_MASK        (0x00000006)
#define ACI_DUMP_FORMAT_ADDR_NONE        (0x00000000) /* 주소를 출력하지 않습니다. */
#define ACI_DUMP_FORMAT_ADDR_ABSOLUTE    (0x00000002) /* 절대주소를 출력합니다.     */
#define ACI_DUMP_FORMAT_ADDR_RELATIVE    (0x00000004) /* 상대주소를 출력합니다.     */
#define ACI_DUMP_FORMAT_ADDR_BOTH        (0x00000006) /* 절대 상대 모두 출력합니다. */

/* Binary로 데이터를 출력해줍니다. */
#define ACI_DUMP_FORMAT_BODY_MASK        (0x00000008)
#define ACI_DUMP_FORMAT_BODY_NONE        (0x00000000) /* Binary데이터를 출력하지 않습니다. */
#define ACI_DUMP_FORMAT_BODY_HEX         (0x00000008) /* 16진수로 출력합니다. */

/* Character로 데이터를 출력해줍니다. */
#define ACI_DUMP_FORMAT_CHAR_MASK        (0x00000010)
#define ACI_DUMP_FORMAT_CHAR_NONE        (0x00000000) /* Char데이터를 출력하지 않습니다. */
#define ACI_DUMP_FORMAT_CHAR_ASCII       (0x00000010) /* 공백(32) ~ 126 사이의 값들을 \
                                                       * Ascii로 출력해줍니다. */

#define ACI_DUMP_FORMAT_BINARY         ( ACI_DUMP_FORMAT_PIECE_SINGLE | \
                                         ACI_DUMP_FORMAT_ADDR_NONE |    \
                                         ACI_DUMP_FORMAT_BODY_HEX |     \
                                         ACI_DUMP_FORMAT_CHAR_NONE )

#define ACI_DUMP_FORMAT_NORMAL         ( ACI_DUMP_FORMAT_PIECE_4BYTE  |  \
                                         ACI_DUMP_FORMAT_ADDR_RELATIVE | \
                                         ACI_DUMP_FORMAT_BODY_HEX |      \
                                         ACI_DUMP_FORMAT_CHAR_ASCII )

#define ACI_DUMP_FORMAT_VALUEONLY      ( ACI_DUMP_FORMAT_PIECE_SINGLE | \
                                         ACI_DUMP_FORMAT_ADDR_NONE |    \
                                         ACI_DUMP_FORMAT_BODY_HEX |     \
                                         ACI_DUMP_FORMAT_CHAR_ASCII )

#define ACI_DUMP_FORMAT_FULL           ( ACI_DUMP_FORMAT_PIECE_4BYTE  |  \
                                         ACI_DUMP_FORMAT_ADDR_BOTH |     \
                                         ACI_DUMP_FORMAT_BODY_HEX |      \
                                         ACI_DUMP_FORMAT_CHAR_ASCII )


#define ACI_BOOT_LOG_FILE   (acp_char_t *)"altibase_boot.log"

#define ACI_MSGLOG_BODY            idg_MsgLog.logBody




/*
 * BUGBUG: CLIENT does not use log
 * These codes are activated when do poting Altibase server.
 */
#if 0
typedef struct aci_msg_log_t
{
    acp_std_file_t               *mFP;                /* Normal Error Log File Pointer */
    acp_thr_mutex_t  mMutex;             /* Error Log File Mutex */
    acp_sint32_t                mDoLogFlag;         /* Message Logging ? */
    acp_char_t               mFileName[1024];    /* File Name */
    acp_offset_t           mSize;              /* file size */
    acp_uint32_t                mMaxNumber;         /* loop file number */
    acp_uint32_t                mCurNumber;         /* Replace될 화일번호 */
    acp_uint32_t                mInitialized;       /* 초기화 유무 : idBool을 쓰지 않은  */
                                            /* 것은 정적영역으로 0 초기화으로  */
                                            /* 되기 때문에 의미가 틀려짐. */
} aci_msg_log_t;


typedef struct aci_log_t
{
    aci_msg_log_t    mLogObj [ACI_LOG_MAX_MODULE];
    const acp_char_t *mMsgModuleName[];

} aci_log_t;

#endif

ACP_EXTERN_C_END


#endif /* _O_IDEMSGLOG_H_ */
