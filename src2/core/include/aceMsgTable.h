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
 
/*******************************************************************************
 * $Id: aceMsgTable.h 8661 2009-11-16 04:50:23Z djin $
 ******************************************************************************/

#if !defined(_O_ACE_MSG_TABLE_H_)
#define _O_ACE_MSG_TABLE_H_

#include <acpDl.h>
#include <acpStr.h>
#include <aceError.h>

#define ACE_MSG_DECLARE_GLOBAL_AREA                                 \
static ace_msg_table_t gMsgTableGlobals[ACE_PRODUCT_MAX + 1]
#define ACE_MSG_GLOBAL_AREA ((ace_msg_table_t**)(&gMsgTableGlobals))
#define ACE_MSG_SET_GLOBAL_AREA                                 \
    do                                                          \
    {                                                           \
        aceMsgTableSetTableArea(ACE_MSG_GLOBAL_AREA);           \
    } while(0)

typedef struct ace_msg_desc_t
{
    acp_sint32_t mID;           /* ID of Message */
    acp_sint32_t mErrMsgCount;  /* Error message Count */
    acp_sint32_t mLogMsgCount;  /* Log Messgae Count */
    acp_str_t    mName;         /* Message Name */
    acp_str_t    mAbbr;         /* Abbreviation */
} ace_msg_desc_t;

typedef struct ace_msg_table_t
{
    ace_msg_desc_t     mDesc;   /* Message Description */
    acp_dl_t           mDl;     /* Message Handler (maybe) */

    const acp_char_t **mErrMsgFormat;   /* Format String */
    const acp_char_t **mLogMsgFormat;   /* Format String */
} ace_msg_table_t;


typedef ace_msg_desc_t    *ace_msg_getdesc_func_t(void);
typedef const acp_char_t **ace_msg_getmsgs_func_t(void);


/* 
 * Get Language Name
 */
ACP_EXPORT acp_rc_t          aceMsgTableGetLanguageName(acp_str_t  *aCode,
                                                        acp_str_t **aName);

/*
 * Load Messages from aDirPath to aMsgTable
 */
ACP_EXPORT acp_rc_t          aceMsgTableLoad(
    ace_msg_table_t  *aMsgTable,
    acp_str_t        *aDirPath,
    acp_str_t        *aCustomCodePrefix,
    acp_str_t        *aLangCode,
    acp_str_t        *aCustomCodeSuffix);
/*
 * Free aMsgTable
 */
ACP_EXPORT void              aceMsgTableUnload(ace_msg_table_t *aMsgTable);

ACP_EXPORT const acp_char_t *aceMsgTableGetProduct(ace_msgid_t aMsgID);
ACP_EXPORT const acp_char_t *aceMsgTableGetErrMsgFormat(ace_msgid_t aMsgID);
ACP_EXPORT const acp_char_t *aceMsgTableGetLogMsgFormat(ace_msgid_t aMsgID);

ACP_EXPORT void aceMsgTableSetTableArea(ace_msg_table_t** aMsgTable);
ACP_EXPORT ace_msg_table_t** aceMsgTableGetTableArea(void);

#endif
