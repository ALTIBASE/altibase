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

#ifndef _ULN_PREPARE_H_
#define _ULN_PREPARE_H_ 1

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnEscape.h>
#include <ulnCharSet.h>

typedef enum ulnNcharLiteral
{
    ULN_NCHAR_LITERAL_NONE,
    ULN_NCHAR_LITERAL_REPLACE,
    ULN_NCHAR_LITERAL_MAX
} ulnNcharLiteral;

ACI_RC ulnSFID_43(ulnFnContext *aContext);
ACI_RC ulnSFID_44(ulnFnContext *aContext);
ACI_RC ulnSFID_45(ulnFnContext *aContext);

ACI_RC ulnPrepCheckArgs(ulnFnContext *aContext, acp_char_t *aStatementText, acp_sint32_t aTextLength);

ACI_RC ulnPrepDoText(ulnFnContext *aFnContext,
                     ulnEscape    *aEsc,
                     ulnCharSet   *aCharSet,
                     acp_char_t   *aStatementText,
                     acp_sint32_t  aTextLength);

ACI_RC ulnPrepDoTextNchar(ulnFnContext *aFnContext,
                          ulnEscape    *aEsc,
                          ulnCharSet   *aCharSet,
                          acp_char_t   *aStatementText,
                          acp_sint32_t  aTextLength);

ACI_RC ulnPrepareCore(ulnFnContext *aFnContext,
                      ulnPtContext *aPtContext,
                      acp_char_t   *aStatementText,
                      acp_sint32_t  aTextLength,
                      acp_uint8_t   aPrepareMode);

ACI_RC ulnPrepareDeferComplete(ulnFnContext *aFnContext,
                               acp_bool_t    aFlush);

ACI_RC ulnCallbackPrepareResult(cmiProtocolContext *aProtocolContext,
                                cmiProtocol        *aProtocol,
                                void               *aServiceSession,
                                void               *aUserContext);

// PROJ-1579 NCHAR
typedef ACI_RC ulnPrepareReplaceFunc(ulnFnContext *aFnContext,
                                     ulnEscape    *aEsc,
                                     ulnCharSet   *aCharSet,
                                     acp_char_t   *aStatementText,
                                     acp_sint32_t  aTextLength);

/* PROJ-1789 Updatable Scrollable Cursor */

ACI_RC ulnPrepBuildSelectForRowset(ulnFnContext      *aFnContext,
                                   acp_sint32_t       aRowCount);
ACI_RC ulnPrepBuildUpdateQstr(ulnFnContext        *aFnContext,
                              ulnColumnIgnoreFlag *aColumnIgnoreFlags);
ACI_RC ulnPrepBuildDeleteQstr(ulnFnContext *aFnContext);
ACI_RC ulnPrepBuildInsertQstr(ulnFnContext        *aFnContext,
                              ulnColumnIgnoreFlag *aColumnIgnoreFlags);

#endif /* _ULN_PREPARE_H_ */
