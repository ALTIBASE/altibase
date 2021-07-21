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
 
#ifndef _O_ULSD_CONN_STRING_H_
#define _O_ULSD_CONN_STRING_H_ 1

#define MAX_CONN_STR_KEYWORD_LEN 36

acp_char_t * ulsdStrTok(acp_char_t     *aString,
                        acp_char_t      aToken,
                        acp_char_t    **aCurrentTokenPtr);

ACI_RC ulsdRemoveConnStrAttribute(acp_char_t    *aConnString,
                                  acp_char_t    *aRemoveKeyword);

ACI_RC ulsdMakeNodeBaseConnStr(ulnFnContext    *aFnContext,
                               acp_char_t      *aConnString,
                               acp_sint16_t     aConnStringLength,
                               acp_bool_t       aIsTestEnable,
                               acp_char_t      *aOutConnString);

#endif /* _O_ULSD_CONN_STRING_H_ */
