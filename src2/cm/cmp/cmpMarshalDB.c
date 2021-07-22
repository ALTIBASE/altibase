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


ACI_RC cmpReadDBMessage(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBMessageA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Message);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBMessage(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBMessageA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Message);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBErrorResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBErrorResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_UCHAR(sArg->mOperationID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mErrorIndex);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBErrorResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBErrorResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mOperationID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mErrorIndex);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBErrorInfo(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBErrorInfoA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorInfo);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBErrorInfo(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBErrorInfoA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorInfo);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBErrorInfoResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBErrorInfoResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorInfoResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBErrorInfoResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBErrorInfoResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorInfoResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBConnect(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBConnectA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Connect);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_VARIABLE(sArg->mDatabaseName);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_VARIABLE(sArg->mUserName);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_VARIABLE(sArg->mPassword);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_USHORT(sArg->mMode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBConnect(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBConnectA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Connect);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mDatabaseName);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mUserName);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mPassword);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_USHORT(sArg->mMode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBConnectResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBConnectResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBDisconnect(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBDisconnectA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Disconnect);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mOption);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBDisconnect(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBDisconnectA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Disconnect);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mOption);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBDisconnectResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBDisconnectResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBPropertyGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPropertyGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertyGet);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_USHORT(sArg->mPropertyID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBPropertyGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPropertyGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertyGet);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_USHORT(sArg->mPropertyID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBPropertyGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPropertyGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertyGetResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_USHORT(sArg->mPropertyID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_ANY(sArg->mValue);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBPropertyGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPropertyGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertyGetResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mPropertyID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_ANY(sArg->mValue);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBPropertySet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPropertySetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertySet);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_USHORT(sArg->mPropertyID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_ANY(sArg->mValue);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBPropertySet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPropertySetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertySet);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mPropertyID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_ANY(sArg->mValue);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBPropertySetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBPropertySetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBPrepare(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPrepareA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Prepare);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_VARIABLE(sArg->mStatementString);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mMode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBPrepare(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPrepareA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Prepare);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mStatementString);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mMode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBPrepareResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPrepareResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PrepareResult);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_READ_UINT(sArg->mStatementType);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_READ_USHORT(sArg->mParamCount);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetCount);

    /*
     * bug-25109 support ColAttribute baseTableName
     */
    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_VARIABLE(sArg->mBaseTableOwnerName);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_VARIABLE(sArg->mBaseTableName);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mBaseTableUpdatable);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mTableName); /* unused */

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBPrepareResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPrepareResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PrepareResult);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementType);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_WRITE_USHORT(sArg->mParamCount);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetCount);

    /*
     * bug-25109 support ColAttribute baseTableName
     */
    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mBaseTableOwnerName);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mBaseTableName);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mBaseTableUpdatable);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mTableName); /* unused */

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBPlanGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPlanGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PlanGet);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBPlanGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPlanGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PlanGet);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBPlanGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPlanGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PlanGetResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mPlan);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBPlanGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBPlanGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PlanGetResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mPlan);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBColumnInfoGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGet);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_USHORT(sArg->mColumnNumber);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBColumnInfoGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGet);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_USHORT(sArg->mColumnNumber);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBColumnInfoGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetResult);

    CMP_MARSHAL_BEGIN(10);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(9);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(8);
    CMP_MARSHAL_READ_USHORT(sArg->mColumnNumber);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_READ_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_READ_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_READ_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UCHAR(sArg->mNullableFlag);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mDisplayName);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBColumnInfoGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetResult);

    CMP_MARSHAL_BEGIN(10);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(9);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(8);
    CMP_MARSHAL_WRITE_USHORT(sArg->mColumnNumber);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_WRITE_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_WRITE_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mNullableFlag);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mDisplayName);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBColumnInfoGetListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoGetListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetListResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mColumnCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBColumnInfoGetListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoGetListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoGetListResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mColumnCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBColumnInfoSet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoSetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoSet);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_READ_USHORT(sArg->mColumnNumber);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_READ_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBColumnInfoSet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBColumnInfoSetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ColumnInfoSet);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_WRITE_USHORT(sArg->mColumnNumber);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_WRITE_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBColumnInfoSetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBColumnInfoSetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBParamInfoGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoGet);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamInfoGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoGet);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBParamInfoGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoGetResult);

    CMP_MARSHAL_BEGIN(9);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(8);
    CMP_MARSHAL_READ_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_READ_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_READ_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_READ_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UCHAR(sArg->mInOutType);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mNullableFlag);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamInfoGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoGetResult);

    CMP_MARSHAL_BEGIN(9);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(8);
    CMP_MARSHAL_WRITE_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_WRITE_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_WRITE_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mInOutType);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mNullableFlag);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBParamInfoSet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoSetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSet);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_READ_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_READ_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_READ_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mInOutType);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamInfoSet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoSetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSet);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_WRITE_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_WRITE_UINT(sArg->mDataType);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_WRITE_UINT(sArg->mLanguage);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mArguments);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_SINT(sArg->mPrecision);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_SINT(sArg->mScale);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mInOutType);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBParamInfoSetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBParamInfoSetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBParamInfoSetList(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoSetListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSetList);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mParamCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamInfoSetList(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamInfoSetListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamInfoSetList);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mParamCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBParamInfoSetListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBParamInfoSetListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBParamDataIn(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataInA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataIn);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_ANY(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamDataIn(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataInA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataIn);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_ANY(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

/*
 * bug-28259: ipc needs paramDataInResult
 */
ACI_RC cmpReadDBParamDataInResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBParamDataInResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}


ACI_RC cmpReadDBParamDataOut(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataOutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOut);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_ANY(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamDataOut(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataOutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOut);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mParamNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_ANY(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBParamDataOutList(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataOutListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOutList);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamDataOutList(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataOutListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataOutList);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBParamDataInList(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataInListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataInList);

    CMP_MARSHAL_BEGIN(5);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_USHORT(sArg->mFromRowNumber);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mToRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UCHAR(sArg->mExecuteOption);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamDataInList(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataInListA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataInList);

    CMP_MARSHAL_BEGIN(5);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_USHORT(sArg->mFromRowNumber);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mToRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mExecuteOption);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBParamDataInListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataInListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataInListResult);

    CMP_MARSHAL_BEGIN(5);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_USHORT(sArg->mFromRowNumber);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mToRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_ULONG(sArg->mAffectedRowCount);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBParamDataInListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBParamDataInListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ParamDataInListResult);

    CMP_MARSHAL_BEGIN(5);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_USHORT(sArg->mFromRowNumber);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mToRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_ULONG(sArg->mAffectedRowCount);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}


ACI_RC cmpReadDBExecute(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBExecuteA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Execute);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mOption);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBExecute(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBExecuteA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Execute);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mOption);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBExecuteResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBExecuteResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ExecuteResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_ULONG(sArg->mAffectedRowCount);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBExecuteResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBExecuteResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ExecuteResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mRowNumber);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_ULONG(sArg->mAffectedRowCount);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFetchMove(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchMoveA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchMove);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UCHAR(sArg->mWhence);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_SLONG(sArg->mOffset);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBFetchMove(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchMoveA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchMove);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mWhence);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_SLONG(sArg->mOffset);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFetchMoveResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBFetchMoveResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBFetch(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Fetch);

    CMP_MARSHAL_BEGIN(5);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_USHORT(sArg->mRecordCount);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mColumnFrom);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_USHORT(sArg->mColumnTo);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBFetch(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Fetch);

    CMP_MARSHAL_BEGIN(5);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mRecordCount);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mColumnFrom);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_USHORT(sArg->mColumnTo);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFetchBeginResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchBeginResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchBeginResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBFetchBeginResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchBeginResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchBeginResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFetchResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchResult);

    CMP_MARSHAL_BEGIN(1);

    CMP_MARSHAL_READ_ANY(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBFetchResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchResult);

    CMP_MARSHAL_BEGIN(1);

    CMP_MARSHAL_WRITE_ANY(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFetchListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchListResult);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_USHORT(sArg->mRecordCount);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mRecordSize);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBFetchListResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchListResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchListResult);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_USHORT(sArg->mRecordCount);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mRecordSize);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFetchEndResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchEndResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchEndResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBFetchEndResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFetchEndResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchEndResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFree(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFreeA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Free);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mMode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBFree(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBFreeA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Free);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_USHORT(sArg->mResultSetID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mMode);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBFreeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBFreeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBCancel(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBCancelA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Cancel);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBCancel(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBCancelA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Cancel);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mStatementID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBCancelResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBCancelResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBTransaction(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBTransactionA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Transaction);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBTransaction(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBTransactionA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Transaction);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBTransactionResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBTransactionResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBLobGetSize(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetSizeA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetSize);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobGetSize(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetSizeA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetSize);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobGetSizeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetSizeResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetSizeResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobGetSizeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetSizeResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetSizeResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGet);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobGet(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGet);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobGetBytePosCharLen(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetBytePosCharLenA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetBytePosCharLen);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobGetBytePosCharLen(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetBytePosCharLenA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetBytePosCharLen);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobGetCharPosCharLen(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetCharPosCharLenA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetCharPosCharLen);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobGetCharPosCharLen(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetCharPosCharLenA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetCharPosCharLen);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetResult);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobGetResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetResult);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobGetBytePosCharLenResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetBytePosCharLenResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetBytePosCharLenResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mCharLength);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobGetBytePosCharLenResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobGetBytePosCharLenResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobGetBytePosCharLenResult);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mCharLength);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobBytePos(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobBytePosA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobBytePos);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mCharOffset);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobBytePos(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobBytePosA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobBytePos);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mCharOffset);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobBytePosResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobBytePosResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobBytePosResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mByteOffset);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobBytePosResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobBytePosResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobBytePosResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mByteOffset);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobCharLength(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobCharLengthA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobCharLength);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobCharLength(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobCharLengthA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobCharLength);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobCharLengthResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobCharLengthResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobCharLengthResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mLength);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobCharLengthResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobCharLengthResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobCharLengthResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mLength);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobPutBegin(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobPutBeginA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPutBegin);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mOldSize);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UINT(sArg->mNewSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobPutBegin(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobPutBeginA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPutBegin);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mOldSize);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UINT(sArg->mNewSize);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobPutBeginResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBLobPutBeginResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBLobPut(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobPutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPut);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobPut(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobPutA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPut);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mOffset);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobPutEnd(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobPutEndA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPutEnd);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobPutEnd(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobPutEndA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobPutEnd);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobPutEndResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBLobPutEndResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBLobFree(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobFreeA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFree);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobFree(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobFreeA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFree);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorID);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobFreeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBLobFreeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpReadDBLobFreeAll(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobFreeAllA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFreeAll);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_ULONG(sArg->mLocatorCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBLobFreeAll(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBLobFreeAllA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, LobFreeAll);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_ULONG(sArg->mLocatorCount);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_COLLECTION(sArg->mListData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBLobFreeAllResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

ACI_RC cmpWriteDBLobFreeAllResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    ACP_UNUSED(aBlock);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aMarshalState);

    return ACI_SUCCESS;
}

/* PROJ-1573 XA */
ACI_RC cmpReadDBXaOperation(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaOperationA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaOperation);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_SINT(sArg->mRmID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_SLONG(sArg->mFlag);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_SLONG(sArg->mArgument);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBXaOperation(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaOperationA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaOperation);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_SINT(sArg->mRmID);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_SLONG(sArg->mFlag);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_SLONG(sArg->mArgument);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}


ACI_RC cmpReadDBXaTransaction(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaTransactionA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaTransaction);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_READ_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_READ_SINT(sArg->mRmID);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_READ_SLONG(sArg->mFlag);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_READ_SLONG(sArg->mArgument);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_READ_SLONG(sArg->mFormatID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_SLONG(sArg->mGTRIDLength);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_SLONG(sArg->mBQUALLength);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBXaTransaction(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaTransactionA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaTransaction);

    CMP_MARSHAL_BEGIN(8);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(7);
    CMP_MARSHAL_WRITE_SINT(sArg->mRmID);

    CMP_MARSHAL_STAGE(6);
    CMP_MARSHAL_WRITE_SLONG(sArg->mFlag);

    CMP_MARSHAL_STAGE(5);
    CMP_MARSHAL_WRITE_SLONG(sArg->mArgument);

    CMP_MARSHAL_STAGE(4);
    CMP_MARSHAL_WRITE_SLONG(sArg->mFormatID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_SLONG(sArg->mGTRIDLength);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_SLONG(sArg->mBQUALLength);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBXaXid(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaXidA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaXid);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_READ_SLONG(sArg->mFormatID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_READ_SLONG(sArg->mGTRIDLength);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_SLONG(sArg->mBQUALLength);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBXaXid(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaXidA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaXid);

    CMP_MARSHAL_BEGIN(4);
    CMP_MARSHAL_WRITE_SLONG(sArg->mFormatID);

    CMP_MARSHAL_STAGE(3);
    CMP_MARSHAL_WRITE_SLONG(sArg->mGTRIDLength);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_SLONG(sArg->mBQUALLength);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mData);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadDBXaResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_SINT(sArg->mReturnValue);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteDBXaResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgDBXaResultA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mOperation);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_SINT(sArg->mReturnValue);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

cmpMarshalFunction gCmpReadFunctionDBClient[CMP_OP_DB_MAX_A5] =
{
    cmpReadDBMessage,
    cmpReadDBErrorResult,
    cmpReadDBErrorInfo,
    cmpReadDBErrorInfoResult,
    cmpReadDBConnect,
    cmpReadDBConnectResult,
    cmpReadDBDisconnect,
    cmpReadDBDisconnectResult,
    cmpReadDBPropertyGet,
    cmpReadDBPropertyGetResult,
    cmpReadDBPropertySet,
    cmpReadDBPropertySetResult,
    cmpReadDBPrepare,
    cmpReadDBPrepareResult,
    cmpReadDBPlanGet,
    cmpReadDBPlanGetResult,
    cmpReadDBColumnInfoGet,
    cmpReadDBColumnInfoGetResult,
    cmpReadDBColumnInfoGetListResult,
    cmpReadDBColumnInfoSet,
    cmpReadDBColumnInfoSetResult,
    cmpReadDBParamInfoGet,
    cmpReadDBParamInfoGetResult,
    cmpReadDBParamInfoSet,
    cmpReadDBParamInfoSetResult,
    cmpReadDBParamInfoSetList,
    cmpReadDBParamInfoSetListResult,
    cmpReadDBParamDataIn,
    cmpReadDBParamDataOut,
    cmpReadDBParamDataOutList,
    cmpReadDBParamDataInList,
    cmpReadDBParamDataInListResult,
    cmpReadDBExecute,
    cmpReadDBExecuteResult,
    cmpReadDBFetchMove,
    cmpReadDBFetchMoveResult,
    cmpReadDBFetch,
    cmpReadDBFetchBeginResult,
    cmpReadDBFetchResult,
    cmpReadDBFetchListResult,
    cmpReadDBFetchEndResult,
    cmpReadDBFree,
    cmpReadDBFreeResult,
    cmpReadDBCancel,
    cmpReadDBCancelResult,
    cmpReadDBTransaction,
    cmpReadDBTransactionResult,
    cmpReadDBLobGetSize,
    cmpReadDBLobGetSizeResult,
    cmpReadDBLobGet,
    cmpReadDBLobGetResult,
    cmpReadDBLobPutBegin,
    cmpReadDBLobPutBeginResult,
    cmpReadDBLobPut,
    cmpReadDBLobPutEnd,
    cmpReadDBLobPutEndResult,
    cmpReadDBLobFree,
    cmpReadDBLobFreeResult,
    cmpReadDBLobFreeAll,
    cmpReadDBLobFreeAllResult,
    cmpReadDBXaOperation,
    cmpReadDBXaXid,
    cmpReadDBXaResult,
    cmpReadDBXaTransaction,
    cmpReadDBLobGetBytePosCharLen,
    cmpReadDBLobGetBytePosCharLenResult,
    cmpReadDBLobGetCharPosCharLen,
    cmpReadDBLobBytePos,
    cmpReadDBLobBytePosResult,
    cmpReadDBLobCharLength,
    cmpReadDBLobCharLengthResult,
    cmpReadDBParamDataInResult  /* bug-28259 for ipc */
    /* BUG-43080 Warning - Do not add protocols for A7 here anymore */
};

cmpMarshalFunction gCmpWriteFunctionDBClient[CMP_OP_DB_MAX_A5] =
{
    cmpWriteDBMessage,
    cmpWriteDBErrorResult,
    cmpWriteDBErrorInfo,
    cmpWriteDBErrorInfoResult,
    cmpWriteDBConnect,
    cmpWriteDBConnectResult,
    cmpWriteDBDisconnect,
    cmpWriteDBDisconnectResult,
    cmpWriteDBPropertyGet,
    cmpWriteDBPropertyGetResult,
    cmpWriteDBPropertySet,
    cmpWriteDBPropertySetResult,
    cmpWriteDBPrepare,
    cmpWriteDBPrepareResult,
    cmpWriteDBPlanGet,
    cmpWriteDBPlanGetResult,
    cmpWriteDBColumnInfoGet,
    cmpWriteDBColumnInfoGetResult,
    cmpWriteDBColumnInfoGetListResult,
    cmpWriteDBColumnInfoSet,
    cmpWriteDBColumnInfoSetResult,
    cmpWriteDBParamInfoGet,
    cmpWriteDBParamInfoGetResult,
    cmpWriteDBParamInfoSet,
    cmpWriteDBParamInfoSetResult,
    cmpWriteDBParamInfoSetList,
    cmpWriteDBParamInfoSetListResult,
    cmpWriteDBParamDataIn,
    cmpWriteDBParamDataOut,
    cmpWriteDBParamDataOutList,
    cmpWriteDBParamDataInList,
    cmpWriteDBParamDataInListResult,
    cmpWriteDBExecute,
    cmpWriteDBExecuteResult,
    cmpWriteDBFetchMove,
    cmpWriteDBFetchMoveResult,
    cmpWriteDBFetch,
    cmpWriteDBFetchBeginResult,
    cmpWriteDBFetchResult,
    cmpWriteDBFetchListResult,
    cmpWriteDBFetchEndResult,
    cmpWriteDBFree,
    cmpWriteDBFreeResult,
    cmpWriteDBCancel,
    cmpWriteDBCancelResult,
    cmpWriteDBTransaction,
    cmpWriteDBTransactionResult,
    cmpWriteDBLobGetSize,
    cmpWriteDBLobGetSizeResult,
    cmpWriteDBLobGet,
    cmpWriteDBLobGetResult,
    cmpWriteDBLobPutBegin,
    cmpWriteDBLobPutBeginResult,
    cmpWriteDBLobPut,
    cmpWriteDBLobPutEnd,
    cmpWriteDBLobPutEndResult,
    cmpWriteDBLobFree,
    cmpWriteDBLobFreeResult,
    cmpWriteDBLobFreeAll,
    cmpWriteDBLobFreeAllResult,
    cmpWriteDBXaOperation,
    cmpWriteDBXaXid,
    cmpWriteDBXaResult,
    cmpWriteDBXaTransaction,
    cmpWriteDBLobGetBytePosCharLen,
    cmpWriteDBLobGetBytePosCharLenResult,
    cmpWriteDBLobGetCharPosCharLen,
    cmpWriteDBLobBytePos,
    cmpWriteDBLobBytePosResult,
    cmpWriteDBLobCharLength,
    cmpWriteDBLobCharLengthResult,
    cmpWriteDBParamDataInResult  /* bug-28259 for ipc */
    /* BUG-43080 Warning - Do not add protocols for A7 here anymore */
};
