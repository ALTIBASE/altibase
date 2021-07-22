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

#include <cmAll.h>


/* BUG-18981 */
IDE_RC cmxXidRead(cmiProtocolContext *aProtocolContext, ID_XID *aXid)
{
    PDL_Time_Value  sTimeout;
    cmiProtocol     sProtocol;
    cmpArgDBXaXidA5  *sArg;
    UInt            sDataLen;
    idBool          sIsEnd;

    sTimeout.set(0, 0);

    //fix BUG-27346 Code-Sonar UMR
    CMI_PROTOCOL_INITIALIZE(sProtocol,sArg,DB,XaXid);
    IDE_TEST(cmiReadProtocol(aProtocolContext, &sProtocol, &sTimeout, &sIsEnd) != IDE_SUCCESS);

    IDE_TEST_RAISE(sProtocol.mOpID != CMI_PROTOCOL_OPERATION(DB, XaXid), InvalidProtocolSequence);


    aXid->formatID     = (vSLong)sArg->mFormatID;
    aXid->gtrid_length = (vSLong)sArg->mGTRIDLength;
    aXid->bqual_length = (vSLong)sArg->mBQUALLength;

    sDataLen = cmtVariableGetSize(&sArg->mData);

    /* BUG-18981 */
    IDE_TEST_RAISE(sDataLen != ID_MAXXIDDATASIZE, InvalidXidDataSize);

    IDE_TEST(cmtVariableGetData(&sArg->mData, (UChar *)aXid->data, sDataLen) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidProtocolSequence);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE));
    }
    IDE_EXCEPTION(InvalidXidDataSize);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_XA_XID_DATA_SIZE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-18981 */    
IDE_RC cmxXidWrite(cmiProtocolContext *aProtocolContext, ID_XID *aXid)
{
    cmiProtocol    sProtocol;
    cmpArgDBXaXidA5 *sArg;
    
    IDE_TEST( aXid == NULL );           //BUG-28994 [CodeSonar]Null Pointer Dereference

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, XaXid);

    sArg->mFormatID    = aXid->formatID;
    sArg->mGTRIDLength = aXid->gtrid_length;
    sArg->mBQUALLength = aXid->bqual_length;

    IDE_TEST(cmtVariableSetData(&sArg->mData, (UChar *)aXid->data, ID_MAXXIDDATASIZE) != IDE_SUCCESS);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
