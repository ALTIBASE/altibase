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

#include <cmAllClient.h>


/* BUG-18981 */
ACI_RC cmxXidRead(cmiProtocolContext *aProtocolContext, ID_XID *aXid)
{
    cmiProtocol     sProtocol;
    cmpArgDBXaXidA5  *sArg;
    acp_uint32_t    sDataLen;
    acp_bool_t      sIsEnd;

    /*
     * fix BUG-27346 Code-Sonar UMR
     */
    CMI_PROTOCOL_INITIALIZE(sProtocol,sArg,DB,XaXid);
    ACI_TEST(cmiReadProtocol(aProtocolContext, &sProtocol, ACP_TIME_IMMEDIATE, &sIsEnd) != ACI_SUCCESS);

    ACI_TEST_RAISE(sProtocol.mOpID != CMI_PROTOCOL_OPERATION(DB, XaXid), InvalidProtocolSequence);


    aXid->formatID     = (acp_slong_t)sArg->mFormatID;
    aXid->gtrid_length = (acp_slong_t)sArg->mGTRIDLength;
    aXid->bqual_length = (acp_slong_t)sArg->mBQUALLength;

    sDataLen = cmtVariableGetSize(&sArg->mData);

    /* BUG-18981 */
    ACI_TEST_RAISE(sDataLen != ID_MAXXIDDATASIZE, InvalidXidDataSize);

    ACI_TEST(cmtVariableGetData(&sArg->mData, (acp_uint8_t *)aXid->data, sDataLen) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidProtocolSequence);
    {
       ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE));
    }
    ACI_EXCEPTION(InvalidXidDataSize);
    {
       ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_XA_XID_DATA_SIZE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* BUG-18981 */
ACI_RC cmxXidWrite(cmiProtocolContext *aProtocolContext, ID_XID *aXid)
{
    cmiProtocol    sProtocol;
    cmpArgDBXaXidA5 *sArg;

    ACI_TEST( aXid == NULL );    //BUG-28994 [CodeSonar]Null Pointer Dereference

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, XaXid);

    sArg->mFormatID    = aXid->formatID;
    sArg->mGTRIDLength = aXid->gtrid_length;
    sArg->mBQUALLength = aXid->bqual_length;

    ACI_TEST(cmtVariableSetData(&sArg->mData, (acp_uint8_t *)aXid->data, ID_MAXXIDDATASIZE) != ACI_SUCCESS);

    ACI_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
