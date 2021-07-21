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

#include <ulnPrivate.h>
#include <ulnFailOver.h>
#include <ulnDataSource.h>
#include <ulnConnAttribute.h>
#include <ulnURL.h>


ACI_RC ulnDataSourceAddConnAttr(ulnDataSource *aDataSource,
                                ulnConnAttrID  aConnAttrID,
                                acp_char_t    *aValueStr)
{
    acp_uint32_t           sStage    = 0;
    acp_size_t             sValueLen;
    ulnDataSourceConnAttr *sConnAttr = NULL;

    // fix BUG-25971 UL-FailOver추가한 함수에서 메모리 한계상황을
    // 고려하지 않음.
    ACI_TEST(acpMemAlloc((void**)&sConnAttr, ACI_SIZEOF(ulnDataSourceConnAttr))
             != ACP_RC_SUCCESS);
    sStage = 1;
    sConnAttr->mAttrID = aConnAttrID;

    sValueLen = acpCStrLen(aValueStr, ACP_SINT32_MAX);
    ACI_TEST(acpMemAlloc((void**)&sConnAttr->mValue, sValueLen + 1)
             != ACP_RC_SUCCESS);
    acpCStrCpy(sConnAttr->mValue,
               sValueLen + 1,
               aValueStr,
               sValueLen);

    acpListInitObj(&(sConnAttr->mLink),sConnAttr);
    acpListAppendNode(&(aDataSource->mConnAttrList),&(sConnAttr->mLink));

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            acpMemFree(sConnAttr);
            break;
    }
    return ACI_FAILURE;
}

ACI_RC ulnDataSourceSetConnAttr(ulnFnContext  *aFnContext,
                                ulnDataSource *aDataSource)
{
    acp_list_node_t       *sIterator;
    ulnDataSourceConnAttr *sConnAttr = NULL;

    ACP_LIST_ITERATE(&(aDataSource->mConnAttrList), sIterator)
    {
        sConnAttr  = (ulnDataSourceConnAttr*)sIterator->mObj;
        ACI_TEST(ulnSetConnAttrById(aFnContext,
                                    sConnAttr->mAttrID,
                                    sConnAttr->mValue,
                                    acpCStrLen(sConnAttr->mValue, ACP_SINT32_MAX))
                 != ACI_SUCCESS);
    }//ACP_LIST_ITERATE

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

void ulnDataSourceDumpAttributes(ulnDataSource *aDataSource)
{
    acp_list_node_t       *sIterator;
    ulnDataSourceConnAttr *sConnAttr = NULL;

    acpPrintf(" DataSource:%s\n",aDataSource->mDataSourceName);

    ACP_LIST_ITERATE(&(aDataSource->mConnAttrList), sIterator)
    {
        sConnAttr  = (ulnDataSourceConnAttr*)sIterator->mObj;
        acpPrintf("Attribute:%s,Value:%s\n",gUlnConnAttrTable[sConnAttr->mAttrID].mKey, sConnAttr->mValue);
        acpStdFlush(ACP_STD_OUT);
    }//ACP_LIST_ITERATE
}

void ulnDataSourceDestroyAttributes(ulnDataSource *aDataSource)
{
    acp_list_node_t       *sIterator;
    acp_list_node_t       *sNodeNext;
    ulnDataSourceConnAttr *sConnAttr = NULL;

    ACP_LIST_ITERATE_SAFE(&(aDataSource->mConnAttrList), sIterator,sNodeNext)
    {
        sConnAttr  = (ulnDataSourceConnAttr*)sIterator->mObj;
        acpListDeleteNode(&(sConnAttr->mLink));
        acpMemFree(sConnAttr->mValue);
        acpMemFree(sConnAttr);
    }//ACP_LIST_ITERATE
}



