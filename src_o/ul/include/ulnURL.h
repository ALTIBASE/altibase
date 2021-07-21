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

#ifndef _O_ULN_URL_H_
#define _O_ULN_URL_H_ 1

typedef struct ulnURL ulnURL;

acp_char_t *urlCleanLead(acp_char_t *s);
acp_char_t *urlCleanKeyWord(acp_char_t *aKeyString);
acp_char_t *urlCleanValue(acp_char_t *aVal);

/**
 * @lnURL*  urlParse(const acp_char_t *, allocurl_f) 
 * does parsing URL into the struct below from
 * this is example of URL here:
 * tcp://192.168.1.1:8080/usr/altibase?name=xxx&addr=none#fragment
 *
 * >arguments: First one if URL C-string Buffer, second one 
 * is Parser result structure 
 */
struct ulnURL
{
    acp_char_t *mSchema;        /* Ex: "tcp","http","ipc"      */
    acp_char_t *mAuthority;     /* Ex: "192.168.1.1:8080"      */
    acp_char_t *mPath;          /* Ex: "/usr/altibase"         */
    acp_char_t *mQuery;         /* Ex: "name=xxx&addr=none"    */
    acp_char_t *mFragment;      /* Ex: "fragment"              */
};

ACI_RC urlParse(acp_char_t *,ulnURL *);

ACI_RC ulnSetConnAttrUrl(ulnFnContext *aContext,  acp_char_t *aAttr);

acp_char_t* ulnStrSep(acp_char_t **aStringp, const acp_char_t *aDelim);

#endif /* _O_ULN_URL_H_ */
