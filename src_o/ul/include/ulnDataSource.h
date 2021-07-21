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

#ifndef  _O_ULN_DATASOURCE_H_
#define  _O_ULN_DATASOURCE_H_ 1

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnConnAttribute.h>

#define  ULN_DATASOURCE_BUCKET_COUNT      (11)
#define  ULN_DATASOURCE_ATTR_BUCKET_COUNT  (7)
#define  ULN_DATASOURCE_PREFIX  '['
#define  ULN_DATASOURCE_POSTFIX ']'
#define  ULN_EOS                '\0'

struct ulnFOServer;


typedef struct ulnDataSource
{
    acp_char_t      *mDataSourceName;
    acp_list_node_t  mChain;
    acp_list_t       mConnAttrList;
}ulnDataSource;


typedef struct ulnDataSourceConnAttr
{
    ulnConnAttrID    mAttrID;
    acp_char_t      *mValue;
    acp_list_node_t  mLink;
}ulnDataSourceConnAttr;



typedef struct ulnDataSourceBucket
{
    acp_list_t      mChain;
}ulnDataSourceBucket;



ACI_RC ulnDataSourceAddConnAttr(ulnDataSource *aDataSource,
                                ulnConnAttrID  aConnAttrID,
                                acp_char_t    *aValueStr);

ACI_RC ulnDataSourceSetConnAttr(ulnFnContext  *aFnContext,
                                ulnDataSource *aDataSource);

void   ulnDataSourceDumpAttributes(ulnDataSource    *aDataSource);
void   ulnDataSourceDestroyAttributes(ulnDataSource *aDataSource);


#endif /* _O_ULN_DATASOURCE_H_ */
