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

#ifndef _O_ULN_SETCONNECTATTR_H_
#define _O_ULN_SETCONNECTATTR_H_ 1

#include <ulnPrivate.h>

ACI_RC ulnSFID_82(ulnFnContext *aContext);
ACI_RC ulnSFID_83(ulnFnContext *aContext);
ACI_RC ulnSFID_84(ulnFnContext *aContext);

struct ulnDataSourceConnAttr;

/*
 * 직접 connection attribute 를 세팅하는 함수
 */
ACI_RC ulnSetConnAttrById(ulnFnContext  *aFnContext,
                          ulnConnAttrID  aConnAttr,
                          acp_char_t    *aAttrValueString,
                          acp_sint32_t   aAttrValueStringLength);

acp_sint32_t ulnCallbackSetConnAttr( void                         *aContext,
                                     ulnConnStrParseCallbackEvent  aEvent,
                                     acp_sint32_t                  aPos,
                                     const acp_char_t             *aKey,
                                     acp_sint32_t                  aKeyLen,
                                     const acp_char_t             *aVal,
                                     acp_sint32_t                  aValLen,
                                     void                         *aFilter );

/* PROJ-1645 UL-FailOver  */
ACP_INLINE ACI_RC ulnSetDSNByConnString( ulnFnContext     *aContext,
                                         const acp_char_t *aConnStr,
                                         acp_sint16_t      aConnStrLen)
{
    ACI_TEST( ulnConnStrParse( aContext,
                               aConnStr,
                               aConnStrLen,
                               ulnCallbackSetConnAttr,
                               (void *)ULN_CONN_ATTR_DSN )
              == -1 );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


/*
 * Connection Attribute 를 세팅하기 위한 exported 함수
 */

ACI_RC ulnSetConnAttrByProfileFunc(ulnFnContext  *aContext,
                                   acp_char_t    *aDSNString,
                                   acp_char_t    *aResourceName);

ACP_INLINE ACI_RC ulnSetConnAttrByConnString( ulnFnContext     *aContext,
                                              const acp_char_t *aConnStr,
                                              acp_sint16_t      aConnStrLen)
{
    ACI_TEST( ulnConnStrParse( aContext,
                               aConnStr,
                               aConnStrLen,
                               ulnCallbackSetConnAttr,
                               (void *)ULN_CONN_ATTR_MAX )
              == -1 );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnSetConnAttrBySQLConstant(ulnFnContext *aFnContext,
                                   acp_sint32_t  aSQLConstant,
                                   void         *aValuePtr,
                                   acp_sint32_t  aLength);

/*
 * Meaningless callback function
 */
ACI_RC ulnCallbackDBPropertySetResult(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aServiceSession,
                                      void               *aUserContext);

/* BUG-36256 Improve property's communication */
ACI_RC ulnSetConnectAttrOff(ulnFnContext *aFnContext,
                            ulnDbc *aDbc,
                            ulnPropertyId aPropertyID);

ACI_RC ulnSetConnAttrConnType(ulnFnContext *aFnContext, ulnConnType aConnType);

ACI_RC ulnSetConnAttributeToDbc( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt );

#endif /* _O_ULN_SETCONNECTATTR_H_ */
