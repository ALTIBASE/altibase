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
 * $Id: aceMsgTable.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

/**
 * @example sampleAceMsg.msg
 */

#include <acpDl.h>
#include <acpStr.h>
#include <aceError.h>
#include <aceMsgTable.h>


#define ACE_MSG_TABLE_RETURN_IF_FAIL(aExpr)     \
    do                                          \
    {                                           \
        acp_rc_t sExprRC_MACRO_LOCAL_VAR = aExpr;               \
                                                \
        if (ACP_RC_NOT_SUCCESS(sExprRC_MACRO_LOCAL_VAR))        \
        {                                       \
            return sExprRC_MACRO_LOCAL_VAR;                     \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)


/*
 * QUALIFIED LANGUAGE CODE
 *
 * language code is based on an ISO-639 standard with some exceptions
 *
 * CM : Tranditioanl Chinese (ZH in ISO-639)
 * CN : Simplified Chinese (ZH in ISO-639)
 * DA : Danish
 * DE : German
 * EN : English
 * ES : Spanish
 * FI : Finnish
 * FR : French
 * IT : Italian
 * JA : Japanese
 * KO : Korean
 * NL : Dutch
 * NO : Norwegian
 * PT : Portuguese
 * RU : Russian
 * SV : Swedish
 */

typedef struct aceMsgLangDesc
{
    acp_str_t mCode;
    acp_str_t mName;
} aceMsgLangDesc;


static aceMsgLangDesc gAceMsgLang[] =
{
    {
        ACP_STR_CONST("CM"),
        ACP_STR_CONST("Tranditioanl Chinese")
    },
    {
        ACP_STR_CONST("CN"),
        ACP_STR_CONST("Simplified Chinese")
    },
    {
        ACP_STR_CONST("DA"),
        ACP_STR_CONST("Danish")
    },
    {
        ACP_STR_CONST("DE"),
        ACP_STR_CONST("German")
    },
    {
        ACP_STR_CONST("EN"),
        ACP_STR_CONST("English")
    },
    {
        ACP_STR_CONST("ES"),
        ACP_STR_CONST("Spanish")
    },
    {
        ACP_STR_CONST("FI"),
        ACP_STR_CONST("Finnish")
    },
    {
        ACP_STR_CONST("FR"),
        ACP_STR_CONST("French")
    },
    {
        ACP_STR_CONST("IT"),
        ACP_STR_CONST("Italian")
    },
    {
        ACP_STR_CONST("JA"),
        ACP_STR_CONST("Japanese")
    },
    {
        ACP_STR_CONST("KO"),
        ACP_STR_CONST("Korean")
    },
    {
        ACP_STR_CONST("NL"),
        ACP_STR_CONST("Dutch")
    },
    {
        ACP_STR_CONST("NO"),
        ACP_STR_CONST("Norwegian")
    },
    {
        ACP_STR_CONST("PT"),
        ACP_STR_CONST("Portuguese")
    },
    {
        ACP_STR_CONST("RU"),
        ACP_STR_CONST("Russian")
    },
    {
        ACP_STR_CONST("SV"),
        ACP_STR_CONST("Swedish")
    },
    {
        ACP_STR_CONST(""),
        ACP_STR_CONST("")
    }
};


static ace_msg_table_t **gAceMsgTable = NULL;


ACP_INLINE acp_rc_t aceMsgTableMakeNames(acp_str_t       *aFileName,
                                         acp_str_t       *aGetDescFuncName,
                                         acp_str_t       *aGetErrMsgsFuncName,
                                         acp_str_t       *aGetLogMsgsFuncName,
                                         ace_msg_table_t *aMsgTable,
                                         acp_str_t       *aCustomCodePrefix,
                                         acp_str_t       *aLangCode,
                                         acp_str_t       *aCustomCodeSuffix)
{
    acp_str_t sPrefix = ACP_STR_CONST("msg");
    acp_str_t sSuffix = ACP_STR_CONST("");

    if(NULL == aCustomCodePrefix)
    {
        aCustomCodePrefix = &sPrefix;
    }
    else
    {
        /* Do nothing */
    }

    if(NULL == aCustomCodeSuffix)
    {
        aCustomCodeSuffix = &sSuffix;
    }
    else
    {
        /* Do nothing */
    }

    ACE_MSG_TABLE_RETURN_IF_FAIL(acpStrCpyFormat(aFileName,
                                                 "%S%S%d%S.mdl",
                                                 aCustomCodePrefix,
                                                 aLangCode,
                                                 aMsgTable->mDesc.mID,
                                                 aCustomCodeSuffix
                                                 ));

    ACE_MSG_TABLE_RETURN_IF_FAIL(acpStrCpyFormat(aGetDescFuncName,
                                                 "msgGetDesc%S%d",
                                                 aLangCode,
                                                 aMsgTable->mDesc.mID));

    ACE_MSG_TABLE_RETURN_IF_FAIL(acpStrCpyFormat(aGetErrMsgsFuncName,
                                                 "msgGetErrMsgs%S%d",
                                                 aLangCode,
                                                 aMsgTable->mDesc.mID));

    ACE_MSG_TABLE_RETURN_IF_FAIL(acpStrCpyFormat(aGetLogMsgsFuncName,
                                                 "msgGetLogMsgs%S%d",
                                                 aLangCode,
                                                 aMsgTable->mDesc.mID));

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t aceMsgTableCheck(ace_msg_table_t *aMsgTable,
                                     ace_msg_desc_t  *aMsgDesc)
{
    if ((aMsgDesc->mID          != aMsgTable->mDesc.mID) ||
        (aMsgDesc->mErrMsgCount != aMsgTable->mDesc.mErrMsgCount) ||
        (aMsgDesc->mLogMsgCount != aMsgTable->mDesc.mLogMsgCount) ||
        (acpStrEqual(&aMsgDesc->mName,
                     &aMsgTable->mDesc.mName,
                     0) != ACP_TRUE) ||
        (acpStrEqual(&aMsgDesc->mAbbr,
                     &aMsgTable->mDesc.mAbbr,
                     0) != ACP_TRUE))
    {
        return ACP_RC_EBADMDL;
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t aceMsgTableLoadFrom(ace_msg_table_t        *aMsgTable,
                                        ace_msg_getdesc_func_t *aGetDescFunc,
                                        ace_msg_getmsgs_func_t *aGetErrMsgsFunc,
                                        ace_msg_getmsgs_func_t *aGetLogMsgsFunc)
{
    ace_msg_desc_t *sMsgDesc = NULL;
    acp_rc_t        sRC;

    if ((aGetDescFunc == NULL) ||
        (aGetErrMsgsFunc == NULL) ||
        (aGetLogMsgsFunc == NULL))
    {
        return ACP_RC_EBADMDL;
    }
    else
    {
        /* do nothing */
    }

    sMsgDesc = (*aGetDescFunc)();

    if (sMsgDesc == NULL)
    {
        return ACP_RC_EBADMDL;
    }
    else
    {
        /* do nothing */
    }

    sRC = aceMsgTableCheck(aMsgTable, sMsgDesc);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    aMsgTable->mErrMsgFormat = (*aGetErrMsgsFunc)();
    aMsgTable->mLogMsgFormat = (*aGetLogMsgsFunc)();

    if ((aMsgTable->mErrMsgFormat == NULL) ||
        (aMsgTable->mLogMsgFormat == NULL))
    {
        return ACP_RC_EBADMDL;
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}


ACP_EXPORT acp_rc_t aceMsgTableGetLanguageName(acp_str_t  *aCode,
                                               acp_str_t **aName)
{
    acp_sint32_t i;

    for (i = 0; acpStrGetLength(&gAceMsgLang[i].mCode) > 0; i++)
    {
        if (acpStrEqual(&gAceMsgLang[i].mCode, aCode, 0) == ACP_TRUE)
        {
            *aName = &gAceMsgLang[i].mName;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_EXPORT acp_rc_t aceMsgTableLoad(ace_msg_table_t  *aMsgTable,
                                    acp_str_t        *aDirPath,
                                    acp_str_t        *aCustomCodePrefix,
                                    acp_str_t        *aLangCode,
                                    acp_str_t        *aCustomCodeSuffix)
{
    acp_str_t               sDefaultLangCode = ACP_STR_CONST("EN");
    ace_msg_getdesc_func_t *sGetDescFunc     = NULL;
    ace_msg_getmsgs_func_t *sGetErrMsgsFunc  = NULL;
    ace_msg_getmsgs_func_t *sGetLogMsgsFunc  = NULL;
    acp_rc_t                sRC;

    ACP_STR_DECLARE_DYNAMIC(sFileName);
    ACP_STR_DECLARE_DYNAMIC(sGetDescFuncName);
    ACP_STR_DECLARE_DYNAMIC(sGetErrMsgsFuncName);
    ACP_STR_DECLARE_DYNAMIC(sGetLogMsgsFuncName);

    ACP_TEST(NULL == gAceMsgTable);

    ACP_STR_INIT_DYNAMIC(sFileName, 0, 32);
    ACP_STR_INIT_DYNAMIC(sGetDescFuncName, 0, 32);
    ACP_STR_INIT_DYNAMIC(sGetErrMsgsFuncName, 0, 32);
    ACP_STR_INIT_DYNAMIC(sGetLogMsgsFuncName, 0, 32);

    /*
     * get default language if aLangCode is not specified
     */
    if (aLangCode == NULL)
    {
        aLangCode = &sDefaultLangCode;
    }
    else
    {
        /* do nothing */
    }

    /*
     * make symbol names
     */
    sRC = aceMsgTableMakeNames(&sFileName,
                               &sGetDescFuncName,
                               &sGetErrMsgsFuncName,
                               &sGetLogMsgsFuncName,
                               aMsgTable,
                               aCustomCodePrefix,
                               aLangCode,
                               aCustomCodeSuffix);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        ACP_STR_FINAL(sFileName);
        ACP_STR_FINAL(sGetDescFuncName);
        ACP_STR_FINAL(sGetErrMsgsFuncName);
        ACP_STR_FINAL(sGetLogMsgsFuncName);

        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * open the message file
     */
    sRC = acpDlOpen(&aMsgTable->mDl, acpStrGetBuffer(aDirPath),
                    acpStrGetBuffer(&sFileName), ACP_FALSE);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        sGetDescFunc = (ace_msg_getdesc_func_t *)
            acpDlSym(&aMsgTable->mDl, acpStrGetBuffer(&sGetDescFuncName));
        sGetErrMsgsFunc = (ace_msg_getmsgs_func_t *)
            acpDlSym(&aMsgTable->mDl, acpStrGetBuffer(&sGetErrMsgsFuncName));
        sGetLogMsgsFunc = (ace_msg_getmsgs_func_t *)
            acpDlSym(&aMsgTable->mDl, acpStrGetBuffer(&sGetLogMsgsFuncName));

        /*
         * load the message table
         */
        sRC = aceMsgTableLoadFrom(aMsgTable,
                                  sGetDescFunc,
                                  sGetErrMsgsFunc,
                                  sGetLogMsgsFunc);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            /*
             * BUGBUG: maybe it should be thread safe
             */
            if ((gAceMsgTable)[aMsgTable->mDesc.mID] != NULL)
            {
                (void)acpDlClose(&aMsgTable->mDl);

                sRC = ACP_RC_EEXIST;
            }
            else
            {
                (gAceMsgTable)[aMsgTable->mDesc.mID] = aMsgTable;
            }
        }
        else
        {
            (void)acpDlClose(&aMsgTable->mDl);
        }
    }
    else
    {
        /* do nothing */
    }

    ACP_STR_FINAL(sFileName);
    ACP_STR_FINAL(sGetDescFuncName);
    ACP_STR_FINAL(sGetErrMsgsFuncName);
    ACP_STR_FINAL(sGetLogMsgsFuncName);

    return sRC;

    ACP_EXCEPTION_END;
    return ACP_RC_ENOSPC;
}

ACP_EXPORT void aceMsgTableUnload(ace_msg_table_t *aMsgTable)
{
    (gAceMsgTable)[aMsgTable->mDesc.mID] = NULL;

    (void)acpDlClose(&aMsgTable->mDl);
}

ACP_EXPORT const acp_char_t *aceMsgTableGetProduct(ace_msgid_t aMsgID)
{
    acp_sint32_t     sProductID  = ACE_ERROR_PRODUCT(aMsgID);
    ace_msg_table_t *sMsgTable   = (gAceMsgTable)[sProductID];

    if (sMsgTable != NULL)
    {
        return acpStrGetBuffer(&sMsgTable->mDesc.mAbbr);
    }
    else
    {
        return NULL;
    }
}

ACP_EXPORT const acp_char_t *aceMsgTableGetErrMsgFormat(ace_msgid_t aMsgID)
{
    acp_sint32_t     sProductID  = ACE_ERROR_PRODUCT(aMsgID);
    acp_sint32_t     sIndexCode  = ACE_ERROR_INDEX(aMsgID);
    ace_msg_table_t *sMsgTable   = (gAceMsgTable)[sProductID];

    if (sMsgTable != NULL)
    {
        return sMsgTable->mErrMsgFormat[sIndexCode];
    }
    else
    {
        return NULL;
    }
}

ACP_EXPORT const acp_char_t *aceMsgTableGetLogMsgFormat(ace_msgid_t aMsgID)
{
    acp_sint32_t     sProductID  = ACE_ERROR_PRODUCT(aMsgID);
    acp_sint32_t     sSubMsgCode = ACE_ERROR_SUBCODE(aMsgID);
    ace_msg_table_t *sMsgTable   = (gAceMsgTable)[sProductID];

    if (sMsgTable != NULL)
    {
        return sMsgTable->mLogMsgFormat[sSubMsgCode];
    }
    else
    {
        return NULL;
    }
}

ACP_EXPORT void aceMsgTableSetTableArea(ace_msg_table_t** aMsgTable)
{
    gAceMsgTable = aMsgTable;
}

ACP_EXPORT ace_msg_table_t** aceMsgTableGetTableArea(void)
{
    return gAceMsgTable;
}

