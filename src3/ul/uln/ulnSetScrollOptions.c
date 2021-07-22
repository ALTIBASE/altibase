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

#include <uln.h>
#include <ulnPrivate.h>

SQLRETURN ulnSetScrollOptions(ulnStmt      *aStmt,
                              acp_uint16_t  aConcurrency,
                              ulvSLen       aKeySetSize,
                              acp_uint16_t  aRowSetSize)
{
    ulnFnContext  sFnContext;
    ulnDbc       *sDbc    = aStmt->mParentDbc;
    acp_uint16_t  sInfoType;
    acp_uint32_t  sValue  = 0;          //BUG-28623 [CodeSonar]Uninitialized Variable
    SQLRETURN     sReturn = SQL_INVALID_HANDLE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETSTMTATTR, aStmt, ULN_OBJ_TYPE_STMT);

    /* 1.0  A call to: SQLGetInfo(...
     *  with the InfoType argument set to one of the values in the following table,
     *  depending on the value of the aKeySetSize argument in SQLSetScrollOptions.
     */
    switch(aKeySetSize)
    {
        case SQL_SCROLL_FORWARD_ONLY  : sInfoType = SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2 ; break;
        case SQL_SCROLL_STATIC        : sInfoType = SQL_STATIC_CURSOR_ATTRIBUTES2       ; break;
        case SQL_SCROLL_KEYSET_DRIVEN : sInfoType = SQL_KEYSET_CURSOR_ATTRIBUTES2       ; break;
        case SQL_SCROLL_DYNAMIC       : sInfoType = SQL_DYNAMIC_CURSOR_ATTRIBUTES2      ; break;
        default :
            /* If the value of the aKeySetSize argument is not listed in the preceding table,
             * the call to SQLSetScrollOptions returns SQLSTATE S1107 (Row value out of range)
             * and none of the following steps are performed.  */
            ACI_TEST_RAISE( aKeySetSize < 0, ERR_S1107);
            sInfoType = SQL_KEYSET_CURSOR_ATTRIBUTES2;
    }

    sReturn = ulnGetInfo(sDbc, sInfoType, &sValue, SQL_IS_UINTEGER, NULL);
    ACI_TEST(!SQL_SUCCEEDED( sReturn ) );

    /* 1.1. Check after GetInfo */
    switch(aConcurrency)
    {
        case SQL_CONCUR_READ_ONLY: ACI_TEST_RAISE( (sValue & SQL_CA2_READ_ONLY_CONCURRENCY)  == 0, ERR_S1C00); break;
        case SQL_CONCUR_LOCK     : ACI_TEST_RAISE( (sValue & SQL_CA2_LOCK_CONCURRENCY)       == 0, ERR_S1C00); break;
        case SQL_CONCUR_ROWVER   : ACI_TEST_RAISE( (sValue & SQL_CA2_OPT_ROWVER_CONCURRENCY) == 0, ERR_S1C00); break;
        case SQL_CONCUR_VALUES   : ACI_TEST_RAISE( (sValue & SQL_CA2_OPT_VALUES_CONCURRENCY) == 0, ERR_S1C00); break;
        default:
            ACI_RAISE( ERR_S1108 );
    }


    /* 2.0 Cursor Type set */
    if( aKeySetSize <= 0 )
    {
        switch( aKeySetSize )
        {
            case SQL_SCROLL_FORWARD_ONLY : sValue = SQL_CURSOR_FORWARD_ONLY ; break;
            case SQL_SCROLL_STATIC       : sValue = SQL_CURSOR_STATIC       ; break;
            case SQL_SCROLL_KEYSET_DRIVEN: sValue = SQL_CURSOR_KEYSET_DRIVEN; break;
            case SQL_SCROLL_DYNAMIC      : sValue = SQL_CURSOR_DYNAMIC      ; break;
            default:
                sValue = SQL_CURSOR_KEYSET_DRIVEN;
        }
        sReturn = ulnSetStmtAttr(aStmt, SQL_ATTR_CURSOR_TYPE, &sValue, SQL_IS_UINTEGER);
        ACI_TEST(!SQL_SUCCEEDED( sReturn ) );
    }
    else
    {
        sValue =  aKeySetSize;
        sReturn = ulnSetStmtAttr(aStmt, SQL_ATTR_KEYSET_SIZE, &sValue, SQL_IS_UINTEGER);
        ACI_TEST(!SQL_SUCCEEDED( sReturn ) );
    }

    /* final call for set RowSet Size */
    sValue  = aRowSetSize;
    sReturn = ulnSetStmtAttr(aStmt, SQL_ROWSET_SIZE, &sValue, SQL_IS_UINTEGER );

    return sReturn;

    ACI_EXCEPTION(ERR_S1107)
    {
        ulnError(&sFnContext, ulERR_ABORT_ROW_VALUE_OUT_OF_RANGE_ERR );
        sReturn = ULN_FNCONTEXT_GET_RC(&sFnContext);
    }

    ACI_EXCEPTION(ERR_S1108)
    {
        ulnError(&sFnContext, ulERR_ABORT_Ignored_Option_Error );
        sReturn = ULN_FNCONTEXT_GET_RC(&sFnContext);
    }

    ACI_EXCEPTION(ERR_S1C00)
    {
        ulnError(&sFnContext, ulERR_ABORT_Ignored_Option_Error );
        sReturn = ULN_FNCONTEXT_GET_RC(&sFnContext);
    }

    ACI_EXCEPTION_END;

    return sReturn;
}

