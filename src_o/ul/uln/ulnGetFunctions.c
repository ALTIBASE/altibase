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


/*
 * 이 함수들은 ODBC 3.x에서 Deprecated 된 함수들임
 * SQL_API_ALL_FUNCTIONS 하면 이 함수들도 사용가능하다고 알려준다.
 */
static const acp_uint16_t ulnDeprecatedFunctions[] =
{
    SQL_API_SQLALLOCCONNECT,
    SQL_API_SQLALLOCENV,
    SQL_API_SQLALLOCSTMT,
    SQL_API_SQLBINDPARAM,
    SQL_API_SQLERROR,
    SQL_API_SQLFREECONNECT,
    SQL_API_SQLFREEENV,
    SQL_API_SQLGETCONNECTOPTION,
    SQL_API_SQLPARAMOPTIONS,
    SQL_API_SQLSETCONNECTOPTION,
    SQL_API_SQLSETPARAM,
    SQL_API_SQLSETSCROLLOPTIONS,
    SQL_API_SQLTRANSACT
};

static const acp_uint16_t ulnSupportedFunctions[] =
{
    SQL_API_SQLALLOCHANDLE,
    SQL_API_SQLALLOCHANDLESTD,
    SQL_API_SQLBINDCOL,
    SQL_API_SQLBINDPARAMETER,
    //SQL_API_SQLBROWSECONNECT,
    SQL_API_SQLBULKOPERATIONS, /* PROJ-1789 */
    SQL_API_SQLCANCEL, /* PROJ-2177 */
    SQL_API_SQLCLOSECURSOR,
    SQL_API_SQLCOLATTRIBUTE,
    //SQL_API_SQLCOLUMNPRIVILEGES,
    SQL_API_SQLCOLUMNS,
    SQL_API_SQLCONNECT,
    //SQL_API_SQLCOPYDESC,
    //SQL_API_SQLDATASOURCES,
    SQL_API_SQLDESCRIBECOL,
    SQL_API_SQLDESCRIBEPARAM,
    SQL_API_SQLDISCONNECT,
    SQL_API_SQLDRIVERCONNECT,
    //SQL_API_SQLDRIVERS,
    SQL_API_SQLENDTRAN,
    SQL_API_SQLEXECDIRECT,
    SQL_API_SQLEXECUTE,
    SQL_API_SQLEXTENDEDFETCH, // fix BUG-19287
    SQL_API_SQLFETCH,
    SQL_API_SQLFETCHSCROLL,
    SQL_API_SQLFOREIGNKEYS,
    SQL_API_SQLFREEHANDLE,
    SQL_API_SQLFREESTMT,
    SQL_API_SQLGETCONNECTATTR,
    //SQL_API_SQLGETCURSORNAME,
    SQL_API_SQLGETDATA,
    SQL_API_SQLGETDESCFIELD,
    SQL_API_SQLGETDESCREC,
    SQL_API_SQLGETDIAGFIELD,
    SQL_API_SQLGETDIAGREC,
    SQL_API_SQLGETENVATTR,
    SQL_API_SQLGETFUNCTIONS,
    SQL_API_SQLGETINFO,
    SQL_API_SQLGETSTMTATTR,
    SQL_API_SQLGETSTMTOPTION,  // bug-20940
    SQL_API_SQLGETTYPEINFO,
    SQL_API_SQLMORERESULTS,
    SQL_API_SQLNATIVESQL,
    SQL_API_SQLNUMPARAMS,
    SQL_API_SQLNUMRESULTCOLS,
    SQL_API_SQLPARAMDATA,
    SQL_API_SQLPREPARE,
    SQL_API_SQLPRIMARYKEYS,
    SQL_API_SQLPROCEDURECOLUMNS,
    SQL_API_SQLPROCEDURES,
    SQL_API_SQLPUTDATA,
    SQL_API_SQLROWCOUNT,
    SQL_API_SQLSETCONNECTATTR,
    //SQL_API_SQLSETCURSORNAME,
    SQL_API_SQLSETDESCFIELD,
    SQL_API_SQLSETDESCREC,
    SQL_API_SQLSETENVATTR,
    SQL_API_SQLSETPOS, /* PROJ-1789 */
    SQL_API_SQLSETSTMTATTR,
    SQL_API_SQLSETSTMTOPTION,  // bug-20940
    SQL_API_SQLSPECIALCOLUMNS,
    SQL_API_SQLSTATISTICS,
    SQL_API_SQLTABLEPRIVILEGES,
    SQL_API_SQLTABLES
};

static ACI_RC ulnGetOdbc3AllFunctions(acp_uint16_t *aSupportedPtr)
{
    acp_uint16_t i;
    acp_uint16_t sFuncID;

    for(i = 0; i < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE; i ++)
    {
        aSupportedPtr[i] = SQL_FALSE;
    }

    for(i = 0; i < ACI_SIZEOF(ulnSupportedFunctions) / ACI_SIZEOF(ulnSupportedFunctions[0]); i++)
    {
      sFuncID = ulnSupportedFunctions[i];

        aSupportedPtr[sFuncID >> 4] |= (1 << (sFuncID & 0x000F));
    }

    return ACI_SUCCESS;
}

static ACI_RC ulnGetAllFunctions(acp_uint16_t *aSupportedPtr)
{
    acp_uint16_t i;
    acp_uint16_t sFuncID;

    for(i = 0; i < 100; i ++)
    {
        aSupportedPtr[i] = SQL_FALSE;
    }

    for(i = 0; i < ACI_SIZEOF(ulnSupportedFunctions) / ACI_SIZEOF(ulnSupportedFunctions[0]); i++)
    {
        sFuncID = ulnSupportedFunctions[i];

        if(sFuncID < 100)
        {
            aSupportedPtr[sFuncID] = SQL_TRUE;
        }
    }

    for(i = 0; i < ACI_SIZEOF(ulnDeprecatedFunctions) / ACI_SIZEOF(ulnDeprecatedFunctions[0]); i++)
    {
        sFuncID = ulnDeprecatedFunctions[i];

        if(sFuncID < 100)
        {
            aSupportedPtr[sFuncID] = SQL_TRUE;
        }
    }

    return ACI_SUCCESS;
}

static ACI_RC ulnGetSpecificFunction(acp_sint32_t aOdbcVersion, acp_uint16_t aFunctionID, acp_uint16_t *aSupportedPtr)
{
    acp_uint16_t i;

    *aSupportedPtr = SQL_FALSE;

    for(i = 0; i < ACI_SIZEOF(ulnSupportedFunctions) / ACI_SIZEOF(ulnSupportedFunctions[0]); i++)
    {
        if(ulnSupportedFunctions[i] == aFunctionID)
        {
            *aSupportedPtr = SQL_TRUE;
            break;
        }
    }

    if ((*aSupportedPtr != SQL_TRUE) && (aOdbcVersion == SQL_OV_ODBC2))
    {
        for(i = 0; i < ACI_SIZEOF(ulnDeprecatedFunctions) / ACI_SIZEOF(ulnDeprecatedFunctions[0]); i++)
        {
            if (ulnDeprecatedFunctions[i] == aFunctionID)
            {
                *aSupportedPtr = SQL_TRUE;
                break;
            }
        }
    }

    return ACI_SUCCESS;
}

static ACI_RC ulnGetFunctionsCheckArgs(ulnFnContext *aContext, acp_uint16_t *aSupportedPtr)
{
    ACI_TEST_RAISE( aSupportedPtr == NULL, LABEL_INVALID_USE_OF_NULL );
    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        /*
         * HY009 : Invalide Use of Null Pointer
         */
        ulnError(aContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnGetFunctions(ulnDbc *aDbc, acp_uint16_t aFunctionID, acp_uint16_t *aSupportedPtr)
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETFUNCTIONS, aDbc, ULN_OBJ_TYPE_DBC);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    /*
     * Check if arguments are valid
     */
    ACI_TEST_RAISE(ulnGetFunctionsCheckArgs(&sFnContext, aSupportedPtr) != ACI_SUCCESS,
                   ULN_LABEL_NEED_EXIT);

    switch(aFunctionID)
    {
        case SQL_API_ODBC3_ALL_FUNCTIONS:
            ACI_TEST_RAISE(ulnGetOdbc3AllFunctions(aSupportedPtr) != ACI_SUCCESS,
                           ULN_LABEL_NEED_EXIT);
            break;

        case SQL_API_ALL_FUNCTIONS:
            ACI_TEST_RAISE(ulnGetAllFunctions(aSupportedPtr) != ACI_SUCCESS,
                           ULN_LABEL_NEED_EXIT);
            break;

        /*
         * ISO 92 standards - compliance level functions
         */
        case SQL_API_SQLALLOCHANDLE:
        case SQL_API_SQLBINDCOL:
        case SQL_API_SQLCANCEL:
        case SQL_API_SQLCLOSECURSOR:
        case SQL_API_SQLCOLATTRIBUTE:
        case SQL_API_SQLCONNECT:
        case SQL_API_SQLCOPYDESC:
        case SQL_API_SQLDATASOURCES:
        case SQL_API_SQLDESCRIBECOL:
        case SQL_API_SQLDISCONNECT:
        case SQL_API_SQLENDTRAN:
        case SQL_API_SQLEXECDIRECT:
        case SQL_API_SQLEXECUTE:
        case SQL_API_SQLFETCH:
        case SQL_API_SQLFETCHSCROLL:
        case SQL_API_SQLFREEHANDLE:
        case SQL_API_SQLFREESTMT:
        case SQL_API_SQLGETCONNECTATTR:
        case SQL_API_SQLGETCURSORNAME:
        case SQL_API_SQLGETDATA:
        case SQL_API_SQLGETDESCFIELD:
        case SQL_API_SQLGETDESCREC:
        case SQL_API_SQLGETDIAGFIELD:
        case SQL_API_SQLGETDIAGREC:
        case SQL_API_SQLGETENVATTR:
        case SQL_API_SQLGETFUNCTIONS:
        case SQL_API_SQLGETINFO:
        case SQL_API_SQLGETSTMTATTR:
        case SQL_API_SQLGETTYPEINFO:
        case SQL_API_SQLNUMPARAMS:
        case SQL_API_SQLNUMRESULTCOLS:
        case SQL_API_SQLPARAMDATA:
        case SQL_API_SQLPREPARE:
        case SQL_API_SQLPUTDATA:
        case SQL_API_SQLROWCOUNT:
        case SQL_API_SQLSETCONNECTATTR:
        case SQL_API_SQLSETCURSORNAME:
        case SQL_API_SQLSETDESCFIELD:
        case SQL_API_SQLSETDESCREC:
        case SQL_API_SQLSETENVATTR:
        case SQL_API_SQLSETSTMTATTR:
        case SQL_API_SQLSTATISTICS:

        /*
         * X/Open standards - compliance level functions
         */
        case SQL_API_SQLCOLUMNPRIVILEGES:
        case SQL_API_SQLCOLUMNS:
        case SQL_API_SQLSPECIALCOLUMNS:
        case SQL_API_SQLTABLES:

        /*
         * ODBC standards - compliance level functions
         */
        case SQL_API_SQLALLOCHANDLESTD:
        case SQL_API_SQLBINDPARAMETER:
        case SQL_API_SQLBROWSECONNECT:
        case SQL_API_SQLBULKOPERATIONS:
        case SQL_API_SQLDESCRIBEPARAM:
        case SQL_API_SQLDRIVERCONNECT:
        case SQL_API_SQLDRIVERS:
        case SQL_API_SQLFOREIGNKEYS:
        case SQL_API_SQLMORERESULTS:
        case SQL_API_SQLNATIVESQL:
        case SQL_API_SQLPRIMARYKEYS:
        case SQL_API_SQLPROCEDURECOLUMNS:
        case SQL_API_SQLPROCEDURES:
        case SQL_API_SQLSETPOS:
        case SQL_API_SQLTABLEPRIVILEGES:

        /*
         * Deprecated
         */
        case SQL_API_SQLALLOCCONNECT:
        case SQL_API_SQLALLOCENV:
        case SQL_API_SQLALLOCSTMT:
        case SQL_API_SQLBINDPARAM:
        case SQL_API_SQLERROR:
        case SQL_API_SQLEXTENDEDFETCH:
        case SQL_API_SQLFREECONNECT:
        case SQL_API_SQLFREEENV:
        case SQL_API_SQLGETCONNECTOPTION:
        case SQL_API_SQLGETSTMTOPTION:
        case SQL_API_SQLPARAMOPTIONS:
        case SQL_API_SQLSETCONNECTOPTION:
        case SQL_API_SQLSETPARAM:
        case SQL_API_SQLSETSCROLLOPTIONS:
        case SQL_API_SQLSETSTMTOPTION:
        case SQL_API_SQLTRANSACT:
            ACI_TEST_RAISE(ulnGetSpecificFunction(aDbc->mParentEnv->mOdbcVersion,
                                                  aFunctionID,
                                                  aSupportedPtr) != ACI_SUCCESS,
                           ULN_LABEL_NEED_EXIT);
            break;

        default:
            /*
             * HY095 : FUNCTION TYPE OUT OF RANGE
             */
            ACI_TEST_RAISE(ulnError(&sFnContext, ulERR_ABORT_FUNCTION_TYPE_OUT_OF_RANGE)
                           != ACI_SUCCESS,
                           ULN_LABEL_NEED_EXIT);
            break;
    }

    /*
     * Exit
     */
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(ULN_LABEL_NEED_EXIT)
    {
        ulnExit(&sFnContext);
    }

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

