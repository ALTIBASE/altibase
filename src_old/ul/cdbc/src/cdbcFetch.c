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

#include <cdbc.h>



CDBC_INTERNAL cdbcBufferItm * altibase_get_lob (cdbcABRes      *sABRes,
                                                acp_sint32_t    aFieldIdx,
                                                acp_sint32_t    aArrayIdx);
CDBC_INTERNAL void altibase_clean_locator (cdbcABRes *aABRes);
CDBC_INTERNAL ALTIBASE_RC altibase_result_store (cdbcABRes *aABRes);



/**
 * 결과셋의 행의 개수를 얻는다.
 * altibase_store_result()를 썼을때만 올바른 값을 반환한다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 결과셋의 컬럼 수, 실패하면 ALTIBASE_INVALID_ROWCOUNT
 * @see altibase_store_result
 */
CDBC_EXPORT
ALTIBASE_LONG altibase_num_rows (ALTIBASE_RES aABRes)
{
    #define CDBC_FUNC_NAME "altibase_num_rows"

    cdbcABRes *sABRes = (cdbcABRes *) aABRes;

    CDBCLOG_IN();

    CDBC_TEST(HRES_NOT_VALID(sABRes));

    altibase_init_errinfo(sABRes->mDiagRec);

    CDBCLOG_OUT_VAL("%d", (acp_sint32_t) sABRes->mRowCount);

    return sABRes->mRowCount;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_ROWCOUNT");

    return ALTIBASE_INVALID_ROWCOUNT;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋의 행의 개수를 얻는다.
 * altibase_stmt_store_result()를 썼을때만 올바른 값을 반환한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 결과셋의 컬럼 수, 실패하면 ALTIBASE_INVALID_ROWCOUNT
 * @see altibase_stmt_store_result
 */
CDBC_EXPORT
ALTIBASE_LONG altibase_stmt_num_rows (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_num_rows"

    cdbcABStmt    *sABStmt = (cdbcABStmt *) aABStmt;
    ALTIBASE_LONG  sRowCount;
    ALTIBASE_RC    sRC = ALTIBASE_ERROR;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    sRowCount = altibase_num_rows(sABStmt->mRes);
    CDBC_TEST(sRowCount == ALTIBASE_INVALID_ROWCOUNT);

    CDBCLOG_OUT_VAL("%d", (acp_sint32_t) sRowCount);

    return sRowCount;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_ROWCOUNT");

    return ALTIBASE_INVALID_ROWCOUNT;

    #undef CDBC_FUNC_NAME
}

/**
 * 현재 행을 구성하고있는 각 컬럼의 데이타 길이를 배열로 얻는다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 각 컬럼의 데이타 길이를 담은 배열, 실패하면 NULL
 */
CDBC_EXPORT
ALTIBASE_LONG * altibase_fetch_lengths (ALTIBASE_RES aABRes)
{
    #define CDBC_FUNC_NAME "altibase_fetch_lengths"

    cdbcABRes *sABRes = (cdbcABRes *) aABRes;

    CDBCLOG_IN();

    CDBC_TEST(HRES_NOT_VALID(sABRes));

    altibase_init_errinfo(sABRes->mDiagRec);

    CDBC_TEST_RAISE(RES_NOT_FETCHED(sABRes), FuncSeqError);

    CDBC_DASSERT(sABRes->mLengths != NULL);

    CDBCLOG_OUT_VAL("%p", sABRes->mLengths);

    return sABRes->mLengths;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(sABRes->mDiagRec,
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * LOB 데이타를 얻는다.
 *
 * 얻은 데이타는 결과셋 핸들의 버퍼 목록에서 관리되므로
 * 임의로 메모리 해제를 해서는 안된다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @param[in] aFieldIdx 필드 번호
 * @return 얻은 데이타의 포인터. 실패하면 NULL
 */
CDBC_INTERNAL
cdbcBufferItm * altibase_get_lob (cdbcABRes *aABRes, acp_sint32_t aFieldIdx, acp_sint32_t aArrayIdx)
{
    #define CDBC_FUNC_NAME "altibase_get_lob"

    cdbcBufferItm       *sBufItm;
    acp_sint32_t         sBufSize;
    ALTIBASE_BIND       *sBind;
    ALTIBASE_LOBLOCATOR  sLobLocator;
    SQLUINTEGER          sLobLength;
    SQLUINTEGER          sLobReadLen;
    SQLSMALLINT          sLocatorCType;
    SQLSMALLINT          sTargetCType;
    acp_rc_t             sRC;

    CDBCLOG_IN();

    CDBC_DASSERT(HRES_IS_VALID(aABRes));
    CDBC_DASSERT(aABRes->mBindResult != NULL);

    CDBCLOG_PRINT_VAL("%d", aArrayIdx);
    CDBCLOG_PRINT_VAL("%d", aFieldIdx);
    sBind = &(aABRes->mBindResult[aFieldIdx]);
    CDBCLOG_PRINT_VAL("%d", sBind->buffer_type);
    CDBCLOG_PRINT_VAL("%p", sBind->buffer);
    CDBCLOG_PRINT_VAL("%d", (acp_sint32_t)(sBind->length[aArrayIdx]));
    sLobLocator = ((ALTIBASE_LOBLOCATOR *)(sBind->buffer))[aArrayIdx];
    CDBCLOG_PRINT_VAL("%ld", (acp_ulong_t)sLobLocator);
    if (aABRes->mFieldInfos[aFieldIdx].type == ALTIBASE_TYPE_BLOB)
    {
        sLocatorCType = SQL_C_BLOB_LOCATOR;
        sTargetCType = SQL_C_BINARY;
    }
    else
    {
        sLocatorCType = SQL_C_CLOB_LOCATOR;
        sTargetCType = SQL_C_CHAR;
    }

    CDBCLOG_CALL("SQLGetLobLength");
    sRC = SQLGetLobLength(aABRes->mHstmt, sLobLocator, sLocatorCType, &sLobLength);
    CDBCLOG_BACK_VAL("SQLGetLobLength", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
    CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sLobLength);

    if ((sLobLength > 0)
     && (aABRes->mFieldInfos[aFieldIdx].type == ALTIBASE_TYPE_CLOB))
    {
        /* 인코딩 변환과 NULL-term 고려.
           NLS_USE가 UTF8이면 한 문자가 최대 4바이트가 될 수 있다. */
        /* BUGBUG: 버퍼 크기가 실제 데이타 크기의 배로 지정되므로
           최대 4GB(CLOB의 최대 크기 2G * 4)까지 할당할 수 있다.
           CLOB일 때 버퍼 크기 정하는 방식을 개선해야한다.*/
        sBufSize = sLobLength * 4 + CDBC_NULLTERM_SIZE;
    }
    else
    {
        sBufSize = sLobLength;
    }
    CDBCLOG_PRINT_VAL("%d", sBufSize);

    sBufItm = altibase_new_buffer(&(aABRes->mDatBuffer), sBufSize,
                               CDBC_BUFFER_TAIL);
    CDBC_TEST_RAISE(sBufItm == NULL, MAllocError);

    if (sBufSize == 0) /* lob is null */
    {
        sBufItm->mLength = ALTIBASE_NULL_DATA;
    }
    else /* lob is not null */
    {
        CDBCLOG_PRINT_VAL("%d", aFieldIdx);
        CDBCLOG_PRINT_VAL("%d", sLocatorCType);
        CDBCLOG_CALL("SQLGetLob");
        sRC = SQLGetLob(aABRes->mHstmt, sLocatorCType, sLobLocator,
                        0, sLobLength, sTargetCType,
                        sBufItm->mBuffer, sBufItm->mBufferLength,
                        &sLobReadLen);
        CDBCLOG_BACK_VAL("SQLGetLob", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
        CDBCLOG_PRINT_VAL("%u", sLobReadLen);
        sBufItm->mLength = sLobReadLen;
    }

    CDBCLOG_CALL("SQLFreeLob");
    sRC = SQLFreeLob(aABRes->mHstmt, sLobLocator);
    CDBCLOG_BACK_VAL("SQLFreeLob", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
    ((ALTIBASE_LOBLOCATOR *)(sBind->buffer))[aArrayIdx] = 0;

    CDBCLOG_OUT_VAL("%p", sBufItm);

    return sBufItm;

    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(aABRes->mDiagRec, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(aABRes, sRC);
    }
    CDBC_EXCEPTION_END;

    /* 에러가 발생해도 새로 생성한 버퍼를 굳이 해제하지 않는다.
       altibase_clean_buffer()를 호출할 때 해제되도록 한다. */

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과 핸들이 갖고있는 LOB Locator를 모두 해지한다.
 *
 * @param[in] aABRes 결과셋 핸들
 */
CDBC_INTERNAL
void altibase_clean_locator (cdbcABRes *aABRes)
{
    #define CDBC_FUNC_NAME "altibase_clean_locator"

    ALTIBASE_LOBLOCATOR  sLobLocator;
    ALTIBASE_BIND       *sBind;
    acp_sint32_t         sColIdx;
    acp_sint32_t         sArrIdx;
    acp_rc_t             sRC;

    CDBCLOG_IN();

    CDBC_DASSERT(HRES_IS_VALID(aABRes));

    for (sColIdx = 0; sColIdx < aABRes->mFieldCount; sColIdx++)
    {
        if ( IS_LOB_TYPE(aABRes->mFieldInfos[sColIdx].type) )
        {
            sBind = &(aABRes->mBindResult[sColIdx]);
            for (sArrIdx = 0; sArrIdx < aABRes->mArrayFetched; sArrIdx++)
            {
                sLobLocator = ((ALTIBASE_LOBLOCATOR *)(sBind->buffer))[sArrIdx];
                if (sLobLocator != 0)
                {
                    CDBCLOG_CALL("SQLFreeLob");
                    sRC = SQLFreeLob(aABRes->mHstmt, sLobLocator);
                    CDBCLOG_BACK_VAL("SQLFreeLob", "%d", sRC);
                    /* 해제에 실패하면 stmt 핸들 또는 locator가
                       유효하지 않은 것이므로 무시 */
                    ((ALTIBASE_LOBLOCATOR *)(sBind->buffer))[sArrIdx] = 0;
                }
            }
        }
    }

    CDBCLOG_OUT();

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋에서 한 행을 얻는다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 한 행의 데이타. 더 가져올 행이 없거나 에러가 발생하면 NULL
 */
CDBC_EXPORT
ALTIBASE_ROW altibase_fetch_row (ALTIBASE_RES aABRes)
{
    #define CDBC_FUNC_NAME "altibase_fetch_row"

    cdbcABRes           *sABRes = (cdbcABRes *) aABRes;
    cdbcBufferItm       *sLobDat;
    ALTIBASE_ROW         sRow = NULL;
    ALTIBASE_LONG        sLen;
    acp_rc_t             sRC;
    acp_sint32_t         i;

    CDBCLOG_IN();

    CDBC_TEST(HRES_NOT_VALID(sABRes));

    altibase_init_errinfo(sABRes->mDiagRec);

    if (RES_IS_STORED(sABRes))
    {
        CDBC_TEST_RAISE(sABRes->mRowCursor == NULL, NoData);

        sRow = sABRes->mRowCursor->mRow;
        sABRes->mLengths = sABRes->mRowCursor->mLengths;

        sABRes->mRowCursor = sABRes->mRowCursor->mNext;
    }
    else /* not stored */
    {
        CDBC_DASSERT(sABRes->mFetchedRow != NULL);
        CDBC_DASSERT(sABRes->mLengths != NULL);

        /* 전에 LOB을 위해 할당한 버퍼가 있으면 해제 */
        altibase_clean_buffer(&(sABRes->mDatBuffer));
        altibase_clean_locator(sABRes);

        CDBCLOG_CALL("SQLFetch");
        sRC = SQLFetch(sABRes->mHstmt);
        CDBCLOG_BACK_VAL("SQLFetch", "%d", sRC);
        CDBC_TEST_RAISE(sRC == SQL_NO_DATA, NoData);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_PRINT_VAL("%d", sABRes->mFieldCount);
        for (i = 0; i < sABRes->mFieldCount; i++)
        {
            CDBCLOG_PRINT_VAL("%d", i);
            /* LOB은 한번에 다 가져올 수 없으므로
               locator를 이용해 순차적으로 데이타를 얻는다.
             */
            if ( IS_LOB_TYPE(sABRes->mFieldInfos[i].type) )
            {
                sLobDat = altibase_get_lob(sABRes, i, 0);
                CDBCLOG_PRINT_VAL("%d", altibase_result_errno(sABRes));
                CDBC_TEST(altibase_result_errno(sABRes) != 0);

                CDBCLOG_PRINT_VAL("%d", i);
                CDBCLOG_PRINT_VAL("%p", &(sABRes->mFetchedRow[i]));
                CDBCLOG_PRINT_VAL("%p", &(sABRes->mLengths[i]));
                CDBCLOG_PRINT_VAL("%p", sLobDat);
                if (sLobDat == NULL)
                {
                    sABRes->mFetchedRow[i] = NULL;
                    sABRes->mLengths[i]    = ALTIBASE_NULL_DATA;
                }
                else
                {
                    sABRes->mFetchedRow[i] = sLobDat->mBuffer;
                    sABRes->mLengths[i]    = sLobDat->mLength;
                }
                CDBCLOG_PRINT_VAL("%p", sABRes->mFetchedRow[i]);
                CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sABRes->mLengths[i]);
            }
            else /* if (not lob) */
            {
                sLen = *(sABRes->mBindResult[i].length);
                CDBCLOG_PRINT_VAL("%p", sABRes->mBindResult);
                CDBCLOG_PRINT_VAL("%p", &sABRes->mBindResult[i]);
                CDBCLOG_PRINT_VAL("%p", sABRes->mBindResult[i].buffer);
                CDBCLOG_PRINT_VAL("%p", sABRes->mBindResult[i].length);
                CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sLen);

                sABRes->mFetchedRow[i] = (sLen == ALTIBASE_NULL_DATA)
                                       ? NULL : sABRes->mBindResult[i].buffer;
                sABRes->mLengths[i]    = sLen;
            }
        } /* for (each col) */

        sRow = sABRes->mFetchedRow;
        (sABRes->mRowCount)++;
    } /* else (not stored) */

    RES_SET_FETCHED(sABRes);

    CDBC_EXCEPTION_CONT(NoData);

    CDBCLOG_OUT_VAL("%p", sRow);

    return sRow;

    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(sABRes, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 모든 결과를 결과 핸들에 받아온다.
 *
 * @param[in] aABRes 결과 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC altibase_result_store (cdbcABRes *aABRes)
{
    #define CDBC_FUNC_NAME "altibase_result_store"

    ALTIBASE_BIND  *sBind;
    cdbcBufferItm  *sBufItm;
    acp_char_t     *sBufPtr;
    cdbcABRowList  *sRowPrev;
    cdbcABRowList  *sRowCur;
    cdbcBufferItm  *sLobDat;
    ALTIBASE_LONG   sBufSize;
    ALTIBASE_RC     sRC;
    acp_sint32_t    sColIdx;
    acp_sint32_t    sArrIdx;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HRES_NOT_VALID(aABRes), InvalidHandle);

    sRC = altibase_ensure_basic_fieldinfos(aABRes);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sBind = aABRes->mBindResult;
    sRowPrev = NULL;
    aABRes->mRowCount = 0;
    while (ACP_TRUE)
    {
        CDBCLOG_CALL("SQLFetch");
        sRC = SQLFetch(aABRes->mHstmt);
        CDBCLOG_BACK_VAL("SQLFetch", "%d", sRC);
        if (sRC == SQL_NO_DATA)
        {
            break;
        }
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_PRINT_VAL("%d", (acp_sint32_t)(aABRes->mArrayFetched));
        aABRes->mRowCount += aABRes->mArrayFetched;

        /* 한 행을 담는데 필요한 메모리 크기 계산 */
        sBufSize = ACI_SIZEOF(cdbcABRowList) + aABRes->mFieldCount
                  * (ACI_SIZEOF(ALTIBASE_COL) + ACI_SIZEOF(ALTIBASE_LONG));
        CDBC_ADJUST_ALIGN(sBufSize);
        sBufSize *= aABRes->mArrayFetched;

        CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sBufSize);
        for (sArrIdx = 0; sArrIdx < aABRes->mArrayFetched; sArrIdx++)
        {
            CDBCLOG_PRINT_VAL("%d", sArrIdx);
            for (sColIdx = 0; sColIdx < aABRes->mFieldCount; sColIdx++)
            {
                CDBCLOG_PRINT_VAL("%d", sColIdx);
                /* for ALTIBASE_COL */
                if ( IS_LOB_TYPE(aABRes->mFieldInfos[sColIdx].type) )
                {
                    /* altibase_get_lob()을 이용해 얻어오므로 따로 alloc할 필요 없다 */
                }
                else if (sBind[sColIdx].length[sArrIdx] != ALTIBASE_NULL_DATA)
                {
                    CDBCLOG_PRINT_VAL("%d", sBind[sColIdx].buffer_type);
                    CDBCLOG_PRINT_VAL("%p", sBind[sColIdx].length);
                    CDBCLOG_PRINT_VAL("%d", (acp_sint32_t)(sBind[sColIdx].length[sArrIdx]));
                    sBufSize += sBind[sColIdx].length[sArrIdx];

                    if (IS_STR_BIND(sBind[sColIdx].buffer_type))
                    {
                        sBufSize += CDBC_NULLTERM_SIZE;
                    }

                    CDBC_ADJUST_ALIGN(sBufSize);
                }
                CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sBufSize);
            }
        }
        CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sBufSize);

        /* mem alloc */
        sBufItm = altibase_new_buffer(&(aABRes->mDatBuffer), sBufSize,
                                      CDBC_BUFFER_TAIL);
        CDBC_TEST_RAISE(sBufItm == NULL, MAllocError);
        sBufPtr = sBufItm->mBuffer;

        /* set list head */
        if (aABRes->mRowList == NULL)
        {
            aABRes->mRowList = aABRes->mRowCursor = (cdbcABRowList *) sBufPtr;
        }

        /* data copy */
        for (sArrIdx = 0; sArrIdx < aABRes->mArrayFetched; sArrIdx++)
        {
            CDBCLOG_PRINT_VAL("%d", sArrIdx);
            sRowCur = (cdbcABRowList *) sBufPtr;
            sBufPtr += ACI_SIZEOF(cdbcABRowList);
            CDBCLOG_PRINT_VAL("%p", sRowCur);

            sRowCur->mRow = (ALTIBASE_ROW) sBufPtr;
            sBufPtr += aABRes->mFieldCount * ACI_SIZEOF(ALTIBASE_COL);

            sRowCur->mLengths = (ALTIBASE_LONG *) sBufPtr;
            sBufPtr += aABRes->mFieldCount * ACI_SIZEOF(ALTIBASE_LONG);

            sBufSize = ACI_SIZEOF(cdbcABRowList) + aABRes->mFieldCount
                      * (ACI_SIZEOF(ALTIBASE_COL) + ACI_SIZEOF(ALTIBASE_LONG));
            sBufPtr += (sBufSize % CDBC_ALIGN_SIZE);

            for (sColIdx = 0; sColIdx < aABRes->mFieldCount; sColIdx++)
            {
                if (sBind[sColIdx].length[sArrIdx] == ALTIBASE_NULL_DATA)
                {
                    sRowCur->mRow[sColIdx]     = NULL;
                    sRowCur->mLengths[sColIdx] = ALTIBASE_NULL_DATA;
                    sBufSize = 0;
                }
                else if ( IS_LOB_TYPE(aABRes->mFieldInfos[sColIdx].type) )
                {
                    sLobDat = altibase_get_lob(aABRes, sColIdx, sArrIdx);
                    CDBCLOG_PRINT_VAL("%d", altibase_result_errno(aABRes));
                    CDBC_TEST_RAISE(altibase_result_errno(aABRes) != 0, ErrorSet);

                    if (sLobDat == NULL)
                    {
                        sRowCur->mRow[sColIdx]     = NULL;
                        sRowCur->mLengths[sColIdx] = ALTIBASE_NULL_DATA;
                    }
                    else
                    {
                        sRowCur->mRow[sColIdx]     = sLobDat->mBuffer;
                        sRowCur->mLengths[sColIdx] = sLobDat->mLength;
                    }

                    sBufSize = 0;
                }
                else
                {
                    sBufSize = sBind[sColIdx].length[sArrIdx];

                    sRowCur->mRow[sColIdx]     = sBufPtr;
                    sRowCur->mLengths[sColIdx] = sBufSize;

                    acpMemCpy(sRowCur->mRow[sColIdx],
                              (acp_char_t *)(sBind[sColIdx].buffer)
                              + sArrIdx * sBind[sColIdx].buffer_length,
                              sBufSize);

                    if (IS_STR_BIND(sBind[sColIdx].buffer_type))
                    {
                        sBufSize += CDBC_NULLTERM_SIZE;
                    }
                    CDBC_ADJUST_ALIGN(sBufSize);
                }

                sBufPtr += sBufSize;
            }
            CDBC_DASSERT(sBufPtr <= (sBufItm->mBuffer + sBufItm->mBufferLength));

            if (sRowPrev != NULL)
            {
                sRowPrev->mNext = sRowCur;
            }
            sRowPrev = sRowCur;
        }
        CDBC_DASSERT(sBufPtr == (sBufItm->mBuffer + sBufItm->mBufferLength));
    }

    if (aABRes->mFetchedColOffsetMaxCount < aABRes->mFieldCount)
    {
        SAFE_FREE_AND_CLEAN(aABRes->mFetchedColOffset);
        aABRes->mFetchedColOffsetMaxCount = 0;

        CDBCLOG_CALL("acpMemCalloc");
        sRC = acpMemCalloc((void **)&(aABRes->mFetchedColOffset),
                           aABRes->mFieldCount, ACI_SIZEOF(ALTIBASE_LONG));
        CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
        CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);

        aABRes->mFetchedColOffsetMaxCount = aABRes->mFieldCount;
    }

    /* 데이타를 다 가져왔으므로 커서를 닫는다. */
    CDBCLOG_CALL("SQLFreeStmt : SQL_CLOSE");
    sRC = SQLFreeStmt(aABRes->mHstmt, SQL_CLOSE);
    CDBCLOG_BACK_VAL("SQLFreeStmt", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    RES_SET_STORED(aABRes);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(aABRes->mDiagRec,
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(aABRes, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(ErrorSet)
    {
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        altibase_clean_locator(aABRes);
        altibase_clean_stored_result(aABRes);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 저장된 결과를 해제한다.
 *
 * @param[in] aABRes 결과 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
void altibase_clean_stored_result (cdbcABRes *aABRes)
{
    #define CDBC_FUNC_NAME "altibase_clean_stored_result"

    CDBCLOG_IN();

    CDBC_DASSERT(HRES_IS_VALID(aABRes));

    if (aABRes->mRowList != NULL)
    {
        /* 실제 메모리는 altibase_new_buffer()로 잡았으므로 변수만 초기화 */
        aABRes->mRowList = NULL;
        aABRes->mRowCursor = NULL;
        aABRes->mFetchedRowCursor = NULL;
        altibase_clean_buffer(&(aABRes->mDatBuffer));

        SAFE_FREE_AND_CLEAN(aABRes->mRowMap);
        aABRes->mRowMapAllocFailed = ACP_FALSE;

        aABRes->mRowCount = 0;
    }

    CDBCLOG_OUT();

    #undef CDBC_FUNC_NAME
}

/**
 * 결과 핸들을 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 성공했으면 결과 핸들, 그렇지 않으면 NULL
 */
CDBC_EXPORT
ALTIBASE_RES altibase_use_result (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_use_result"

    cdbcABConn     *sABConn = (cdbcABConn *) aABConn;
    cdbcABRes      *sABRes = NULL;
    acp_char_t     *sBuffer = NULL;
    acp_sint32_t    sFieldCount;
    acp_rc_t        sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBC_TEST_RAISE(CONN_NOT_EXECUTED(sABConn), FuncSeqError);
    CDBC_TEST_RAISE(QUERY_NOT_SELECTABLE(sABConn->mQueryType), NoResult);
    CDBC_TEST_RAISE(CONN_IS_RESRETURNED(sABConn), FuncSeqError);

    sFieldCount = altibase_field_count(sABConn);
    CDBCLOG_PRINT_VAL("%d", sFieldCount);
    CDBC_TEST(sFieldCount == ALTIBASE_INVALID_FIELDCOUNT);
    CDBC_TEST_RAISE(sFieldCount == 0, FuncSeqError);

    sABRes = altibase_result_init(sABConn);
    CDBCLOG_PRINT_VAL("%p", sABRes);
    CDBC_TEST(sABRes == NULL);

    /* 일전에 altibase_store_result()를 한 영향이 CLI의 stmt에 남아있을 수 있으므로
       반드시 초기화해야한다. */
    sRC = altibase_result_set_array_fetch(sABRes, 1);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sRC = altibase_result_bind_init(sABRes, NULL, CDBC_ALLOC_BUF_ON);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sRC = altibase_result_bind_proc(sABRes, CDBC_USE_LOCATOR_ON);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    /* mLengths 까지 한번에 alloc */
    CDBCLOG_CALL("acpMemCalloc");
    sRC = acpMemCalloc((void **)&(sBuffer), sFieldCount,
                       ACI_SIZEOF(ALTIBASE_COL) + ACI_SIZEOF(ALTIBASE_LONG));
    CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
    CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);

    sABRes->mFetchedRow = (ALTIBASE_ROW) sBuffer;
    sABRes->mLengths    = (ALTIBASE_LONG *)(sBuffer +
                          (ACI_SIZEOF(ALTIBASE_COL) * sFieldCount));
    sABRes->mRowCount   = 0;
    CDBCLOG_PRINT_VAL("%d", sFieldCount);
    CDBCLOG_PRINT_VAL("%d", (acp_sint32_t)(sFieldCount * (ACI_SIZEOF(ALTIBASE_COL) + ACI_SIZEOF(ALTIBASE_LONG))) );
    CDBCLOG_PRINT_VAL("%p", sABRes->mFetchedRow);
    CDBCLOG_PRINT_VAL("%p", sABRes->mLengths);

    /* 상태 변경: res returned */
    RES_SET_RESRETURNED(sABRes);

    CDBC_EXCEPTION_CONT(NoResult);

    CDBCLOG_OUT_VAL("%p", sABRes);

    return (ALTIBASE_RES) sABRes;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(sABRes->mDiagRec,
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
    }
    CDBC_EXCEPTION_END;

    if (sABRes != NULL)
    {
        /* sABRes는 유효한 핸들이므로 에러 메시지가 덮어써질 일은 없다 */
        altibase_free_result(sABRes);
    }

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 모든 결과를 담은 결과 핸들을 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 성공했으면 결과 핸들, 그렇지 않으면 NULL
 */
CDBC_EXPORT
ALTIBASE_RES altibase_store_result (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_store_result"

    cdbcABConn     *sABConn = (cdbcABConn *) aABConn;
    cdbcABRes      *sABRes = NULL;
    acp_sint32_t    sFieldCount;
    acp_rc_t        sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBC_TEST_RAISE(CONN_NOT_EXECUTED(sABConn), FuncSeqError);
    CDBC_TEST_RAISE(QUERY_NOT_SELECTABLE(sABConn->mQueryType), NoResult);
    CDBC_TEST_RAISE(CONN_IS_RESRETURNED(sABConn), FuncSeqError);

    sFieldCount = altibase_field_count(sABConn);
    CDBCLOG_PRINT_VAL("%d", sFieldCount);
    CDBC_TEST(sFieldCount == ALTIBASE_INVALID_FIELDCOUNT);
    CDBC_TEST_RAISE(sFieldCount == 0, FuncSeqError);

    sABRes = altibase_result_init(sABConn);
    CDBCLOG_PRINT_VAL("%p", sABRes);
    CDBC_TEST(sABRes == NULL);

    /* store는 항상 array fetch */
    sRC = altibase_result_set_array_fetch(sABRes, CDBC_DEFAULT_ARRAY_SIZE);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sRC = altibase_result_bind_init(sABRes, NULL, CDBC_ALLOC_BUF_ON);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sRC = altibase_result_bind_proc(sABRes, CDBC_USE_LOCATOR_ON);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sRC = altibase_result_store(sABRes);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    /* 상태 변경: res returned */
    RES_SET_RESRETURNED(sABRes);

    CDBC_EXCEPTION_CONT(NoResult);

    CDBCLOG_OUT_VAL("%p", sABRes);

    return (ALTIBASE_RES) sABRes;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION_END;

    if (sABRes != NULL)
    {
        /* sABRes는 유효한 핸들이므로 에러 메시지가 덮어써질 일은 없다 */
        altibase_free_result(sABRes);
    }

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 모든 결과를 받아둔다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_store_result (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_store_result"

    cdbcABStmt     *sABStmt = (cdbcABStmt *) aABStmt;
    ALTIBASE_RC     sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(STMT_NOT_EXECUTED(sABStmt), FuncSeqError);
    CDBC_TEST_RAISE(QUERY_NOT_SELECTABLE(sABStmt->mQueryType), NoResult);
    CDBC_TEST_RAISE(STMT_IS_RESRETURNED(sABStmt), FuncSeqError);
    CDBC_TEST_RAISE(sABStmt->mBindResult == NULL, FuncSeqError);

    CDBC_DASSERT(sABStmt->mRes != NULL);

    if (sABStmt->mArrayFetchSize > 1)
    {
        /* 사용자가 설정한 array fetch 사용 */
        sRC = altibase_result_rebind_for_lob(sABStmt->mRes);
        CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));
    }
    else /* if (not set array fetch) */
    {
        sRC = altibase_result_set_array_fetch(sABStmt->mRes, CDBC_DEFAULT_ARRAY_SIZE);
        CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

        sRC = altibase_stmt_resultbind_proc(sABStmt, sABStmt->mBindResult,
                                            ALTIBASE_TRUE);
        CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));
    }

    sRC = altibase_result_store(sABStmt->mRes);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    /* 상태 변경: res returned */
    STMT_SET_RESRETURNED(sABStmt);

    CDBC_EXCEPTION_CONT(NoResult);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 커서를 이동한다.
 * altibase_store_result()를 썼을때만 올바로 동작한다.
 *
 * @param[in] aABRes 결과 핸들
 * @param[in] aOffset 이동한 커서 위치 (0부터 시작)
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 * @see altibase_store_result
 */
CDBC_EXPORT
ALTIBASE_RC altibase_data_seek (ALTIBASE_RES aABRes, ALTIBASE_LONG aOffset)
{
    #define CDBC_FUNC_NAME "altibase_data_seek"

    cdbcABRes     *sABRes = (cdbcABRes *) aABRes;
    cdbcABRowList *sRowCur;
    ALTIBASE_RC    sRC;
    acp_sint32_t   i;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HRES_NOT_VALID(sABRes), InvalidHandle);

    altibase_init_errinfo(sABRes->mDiagRec);

    CDBC_TEST_RAISE(RES_NOT_STORED(sABRes), FuncSeqError);
    CDBC_TEST_RAISE(INDEX_NOT_VALID(aOffset, 0, sABRes->mRowCount),
                    InvalidParamRange);

    /* 메모리에 여유가 있으면 row map 생성 */
    if ((sABRes->mRowMapAllocFailed == ACP_FALSE)
     && (sABRes->mRowMap == NULL))
    {
        CDBCLOG_CALL("acpMemCalloc");
        sRC = acpMemCalloc((void **)&(sABRes->mRowMap), sABRes->mRowCount,
                           ACI_SIZEOF(cdbcABRowList *));
        CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRowCur = sABRes->mRowList;
            for (i = 0; i < sABRes->mRowCount; i++)
            {
                CDBC_DASSERT(sRowCur != NULL);
                sABRes->mRowMap[i] = sRowCur;
                sRowCur = sRowCur->mNext;
            }
        }
        else
        {
            sABRes->mRowMap = NULL;
            sABRes->mRowMapAllocFailed = ACP_TRUE;
        }
    }

    /* 메모리가 부족해서 row map 생성에 실패했다면 row list로 처리 */
    if (sABRes->mRowMap == NULL)
    {
        sRowCur = sABRes->mRowList;
        for (i = 0; i < aOffset; i++)
        {
            CDBC_DASSERT(sRowCur != NULL);
            sRowCur = sRowCur->mNext;
        }
    }
    else /* if (row map exist) */
    {
        sRowCur = sABRes->mRowMap[aOffset];
    }

    sABRes->mRowCursor = sRowCur;
    sABRes->mFetchedRowCursor = NULL;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(sABRes->mDiagRec,
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(InvalidParamRange);
    {
        altibase_set_errinfo(sABRes->mDiagRec,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 커서를 이동한다.
 * altibase_stmt_store_result()를 썼을때만 올바로 동작한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @param[in] aOffset 이동한 커서 위치 (0부터 시작)
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 * @see altibase_stmt_store_result
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_data_seek (ALTIBASE_STMT aABStmt, ALTIBASE_LONG aOffset)
{
    #define CDBC_FUNC_NAME "altibase_stmt_data_seek"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    sRC = altibase_data_seek(sABStmt->mRes, aOffset);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * stored result의 현재 커서의 한 컬럼 값을 ALTIBASE_BIND로 복사한다.
 *
 * altibase_stmt_fetch(), altibase_stmt_fetch_column()에서 사용하는 내부 함수로
 * altibase_store_result()를 썼을때만 사용한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @param[in] aBind 데이타를 담을 곳
 * @param[in] aRowCursor 데이타 커서
 * @param[in] aColIdx 컬럼 인덱스 (0부터 시작)
 * @param[in] aArrIdx 어레이 인덱스 (0부터 시작)
 * @param[in] aOffset 데이타를 얻기 시작할 위치 (0부터 시작).
 *                    데이타가 NULL이면 무시.
 * @return 성공했고 데이타를 전부 잘 가져왔으면 ALTIBASE_SUCCESS,
 *         성공했는데 버퍼 길이가 작아서 데이타가 짤렸거나 더 얻을 데이타가 없으면 ALTIBASE_SUCCESS_WITH_INFO,
 *         실패했으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC altibase_stmt_copy_col (cdbcABStmt       *aABStmt,
                                    ALTIBASE_BIND    *aBind,
                                    cdbcABRowList    *aRowCursor,
                                    acp_sint32_t      aColIdx,
                                    acp_sint32_t      aArrIdx,
                                    ALTIBASE_LONG     aOffset)
{
    #define CDBC_FUNC_NAME "altibase_copy_col"

    acp_rc_t       sRC = ALTIBASE_SUCCESS;
    ALTIBASE_LONG  sDatLen;
    ALTIBASE_LONG  sBufLen;
    ALTIBASE_LONG  sCpyLen;
    acp_char_t    *sBufPtr;
    acp_sint32_t   sTermLen;

    CDBCLOG_IN();

    CDBC_DASSERT(HSTMT_IS_VALID(aABStmt));
    CDBC_DASSERT(STMT_IS_EXECUTED(aABStmt)
                && QUERY_IS_SELECTABLE(aABStmt->mQueryType));
    CDBC_DASSERT(STMT_IS_STORED(aABStmt));
    CDBC_DASSERT(aBind != NULL);
    CDBC_DASSERT(aRowCursor != NULL);

    /* column index 범위 확인 */
    CDBCLOG_PRINT_VAL("%d", aABStmt->mRes->mFieldCount);
    CDBC_TEST_RAISE(INDEX_NOT_VALID(aColIdx, 0, aABStmt->mRes->mFieldCount),
                    InvalidParamRange);

    /* altibase_stmt_store_result()를 호출했다면,
       바인드 했을때의 buffer_type과 동일한 buffer_type만이 허용된다. */
    CDBCLOG_PRINT_VAL("%d", aABStmt->mRes->mBindResult[aColIdx].buffer_type);
    CDBCLOG_PRINT_VAL("%d", aBind->buffer_type);
    CDBC_TEST_RAISE(aABStmt->mRes->mBindResult[aColIdx].buffer_type
                    != aBind->buffer_type, InvalidParamRange);

    sDatLen = aRowCursor->mLengths[aColIdx];
    CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sDatLen);

    /* ALTIBASE_FETCH_CONT 등을 이용해서 마지막 데이타까지 얻어서, 더 줄게 없으면.. */
    if (aOffset == sDatLen)
    {
        sRC = ALTIBASE_SUCCESS_WITH_INFO;
        CDBC_RAISE(NoMoreData);
    }

    if (sDatLen != ALTIBASE_NULL_DATA)
    {
        /* 데이타를 다 얻어서 index 범위를 넘긴게 아니면,
           예를들어 임의로 offset을 sDatLen + n (n > 0)로 설정하면 에러로 처리 */
        CDBC_TEST_RAISE(INDEX_NOT_VALID(aOffset, 0, sDatLen), InvalidParamRange);

        /* length에는 ALTIBASE_NULL_DATA 외에는 음수가 오지 않는다 */
        CDBC_TEST_RAISE(sDatLen == 0, EmptyData);
        CDBC_DASSERT(sDatLen > 0);

        CDBCLOG_PRINT_VAL("%d", aBind->buffer_type);
        if (aBind->buffer_type == ALTIBASE_BIND_STRING)
        {
            sTermLen = 1;
        }
        else if (aBind->buffer_type == ALTIBASE_BIND_WSTRING)
        {
            sTermLen = 2;
        }
        else
        {
            sTermLen = 0;
        }

        CDBCLOG_PRINT_VAL("%d", (acp_sint32_t)(aBind->buffer_length));
        if (aBind->buffer_length != 0)
        {
            sBufLen = aBind->buffer_length;
        }
        else /* if (aBaseBindInfo[i].buffer_length == 0) */
        {
            /* 크기가 정해져있는 바인드 타입의 buffer_size가 0이면
               자동으로 적절한 값을 설정한다. */
            sBufLen = altibase_bind_max_typesize(aBind->buffer_type);
            CDBC_DASSERT(sBufLen != CDBC_EXPNO_INVALID_BIND_TYPE);
            if (sBufLen == CDBC_EXPNO_VARSIZE_TYPE)
            {
                /* ALTIBASE_BIND_BINARY 같은 바인드 타입은
                   반드시 buffet_length를 유효한 값으로 지정해주어야 하므로
                   여기서는 버퍼 길이가 0인 것으로 간주한다. */
                sBufLen = 0;
                CDBCLOG_PRINT_VAL("%d", aBind->buffer_type);
            }
            CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sBufLen);
        }

        CDBC_DASSERT(aOffset < sDatLen);
        sCpyLen = sDatLen - aOffset;
        if (sBufLen < (sCpyLen + sTermLen))
        {
            sCpyLen = sBufLen - sTermLen;
            sRC = ALTIBASE_SUCCESS_WITH_INFO; /* string truncated */
        }

        CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sBufLen);
        CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) sCpyLen);
        CDBCLOG_PRINT_VAL("%p", aRowCursor->mRow[aColIdx]);
        sBufPtr = ((acp_char_t *)(aBind->buffer)) + (aArrIdx * sBufLen);

        acpMemCpy(sBufPtr, (acp_char_t *)(aRowCursor->mRow[aColIdx]) + aOffset,
                  sCpyLen);
        aABStmt->mRes->mFetchedColOffset[aColIdx] = aOffset + sCpyLen;

        if (sTermLen > 0)
        {
            acpMemSet(sBufPtr + sCpyLen, 0, sTermLen);
        }

        CDBC_EXCEPTION_CONT(EmptyData);
    }

    CDBC_EXCEPTION_CONT(NoMoreData);

    if (aBind->length != NULL)
    {
        aBind->length[aArrIdx] = (sDatLen == ALTIBASE_NULL_DATA)
                               ? ALTIBASE_NULL_DATA : (sDatLen - aOffset);
    }
    if (aBind->is_null != NULL)
    {
        aBind->is_null[aArrIdx] = (sDatLen == ALTIBASE_NULL_DATA)
                                ? ALTIBASE_TRUE : ALTIBASE_FALSE;
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    CDBC_EXCEPTION(InvalidParamRange);
    {
        altibase_set_errinfo(&(aABStmt->mDiagRec),
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_ERROR");

    return ALTIBASE_ERROR;

    #undef CDBC_FUNC_NAME
}

/**
 * fetch 한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 성공했고 데이타를 전부 잘 가져왔으면 ALTIBASE_SUCCESS,
 *         성공했는데 버퍼 길이가 작아서 데이타가 짤렸거나 더 얻을 데이타가 없으면 ALTIBASE_SUCCESS_WITH_INFO,
 *         실패했으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_fetch (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_fetch"

    cdbcABStmt    *sABStmt = (cdbcABStmt *) aABStmt;
    cdbcABRes     *sABRes;
    cdbcABRowList *sRowCursor;
    ALTIBASE_BIND *sBindSrc;
    ALTIBASE_BIND *sBindDest;
    acp_sint32_t   sArrIdx;
    acp_sint32_t   sColIdx;
    acp_sint32_t   sFetched;
    ALTIBASE_RC    sRC = ALTIBASE_ERROR;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(STMT_NOT_EXECUTED(sABStmt), FuncSeqError);

    sABRes = sABStmt->mRes;

    if (STMT_IS_STORED(sABStmt))
    {
        CDBCLOG_PRINT_VAL("%p", sABRes->mRowList);
        CDBCLOG_PRINT_VAL("%p", sABRes->mRowCursor);
        sABRes->mFetchedRowCursor = sABRes->mRowCursor;
        sRowCursor = sABRes->mRowCursor;
        if (sRowCursor == NULL)
        {
            sRC = ALTIBASE_NO_DATA;
            CDBC_RAISE(NoData);
        }

        sFetched = 0;
        for (sArrIdx = 0; (sRowCursor != NULL) && (sArrIdx < sABStmt->mArrayFetchSize); sArrIdx++)
        {
            CDBCLOG_PRINT_VAL("%p", sRowCursor);
            if (sABRes->mArrayFetchSize > 1)
            {
                sABRes->mArrayStatusResult[sArrIdx] = sRowCursor->mStatus;
            }

            /* store가 잘 됐다면 SUCCESS_WITH_INFO는 없어야 한다 */
            CDBC_DASSERT(sRowCursor->mStatus != ALTIBASE_ROW_SUCCESS_WITH_INFO);

            sRC = ALTIBASE_SUCCESS;
            for (sColIdx = 0; sColIdx < sABRes->mFieldCount; sColIdx++)
            {
                /* SUCCESS == 0, SUCCESS_WITH_INFO == 1 인걸 이용 */
                sRC |= altibase_stmt_copy_col(sABStmt,
                                              &(sABStmt->mBindResult[sColIdx]),
                                              sRowCursor, sColIdx, sArrIdx, 0);
            }

            if ((sABStmt->mArrayFetchSize > 1)
             && (sRC != ALTIBASE_SUCCESS))
            {
                sABRes->mArrayStatusResult[sArrIdx] = ALTIBASE_ROW_SUCCESS_WITH_INFO;
            }

            sFetched++;
            sRowCursor = sRowCursor->mNext;
        }
        for (; sArrIdx < sABStmt->mArrayFetchSize; sArrIdx++)
        {
            sABRes->mArrayStatusResult[sArrIdx] = ALTIBASE_ROW_NOROW;
        }

        acpMemSet(sABStmt->mRes->mFetchedColOffset, 0, ACI_SIZEOF(ALTIBASE_LONG)
                  * sABStmt->mRes->mFetchedColOffsetMaxCount);
        sABRes->mRowCursor = sRowCursor;
        sABRes->mArrayFetched = sFetched;
    }
    else /* not stored */
    {
        /* BUG-42089 altibase_stmt_store_result()의 영향이 남아있을 수 있으므로 재설정 */
        if (sABRes->mArrayFetchSize != sABStmt->mArrayFetchSize)
        {
            CDBCLOG_PRINTF_ARG2("reset sABRes->mArrayFetchSize : %d ==> %d",
                                sABRes->mArrayFetchSize, sABStmt->mArrayFetchSize);

            sRC = altibase_result_set_array_fetch(sABRes, sABStmt->mArrayFetchSize);
            CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

            sRC = altibase_stmt_resultbind_proc(sABStmt, sABStmt->mBindResult,
                                                ALTIBASE_FALSE);
            CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));
        }

        CDBCLOG_CALL("SQLFetch");
        sRC = SQLFetch(sABStmt->mHstmt);
        CDBCLOG_BACK_VAL("SQLFetch", "%d", sRC);
        if (sRC == SQL_NO_DATA)
        {
            sRC = ALTIBASE_NO_DATA;
            CDBC_RAISE(NoData);
        }
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        sFetched = (sABRes->mArrayFetchSize > 1)
                 ? (acp_sint32_t)(sABRes->mArrayFetched) : 1;
        CDBCLOG_PRINT_VAL("%d", sABRes->mArrayFetchSize);
        CDBCLOG_PRINT_VAL("%d", (acp_sint32_t)(sABRes->mArrayFetched));
        CDBCLOG_PRINT_VAL("%d", sFetched);

        if (sABStmt->mBindResult != NULL)
        {
            for (sColIdx = 0; sColIdx < sABRes->mFieldCount; sColIdx++)
            {
                sBindSrc  = &(sABRes->mBindResult[sColIdx]);
                sBindDest = &(sABStmt->mBindResult[sColIdx]);

                if (sBindDest->length != NULL)
                {
                    CDBCLOG_PRINT_VAL("%p", sBindSrc->length);
                    CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) *(sBindSrc->length));

                    acpMemCpy(sBindDest->length, sBindSrc->length,
                              ACI_SIZEOF(ALTIBASE_LONG) * sABStmt->mArrayFetchSize);
                }
                if (sBindDest->is_null != NULL)
                {
                    for (sArrIdx = 0; sArrIdx < sABStmt->mArrayFetchSize; sArrIdx++)
                    {
                        sBindDest->is_null[sArrIdx]
                            = (sBindSrc->length[sArrIdx] == ALTIBASE_NULL_DATA)
                            ? ALTIBASE_TRUE : ALTIBASE_FALSE;
                    }
                }
            }
        }
        (sABRes->mRowCount) += sFetched;
    }

    STMT_SET_FETCHED(sABStmt);

    CDBC_EXCEPTION_CONT(NoData);

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_stmt(sABStmt, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 특정 컬럼의 데이타를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @param[in] aBind 데이타를 얻는데 사용할 정보
 * @param[in] aIdx 컬럼 인덱스 (0 부터 시작)
 * @param[in] aOffset 얻기 시작할 위치 (0 부터 시작)
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_fetch_column (ALTIBASE_STMT  aABStmt,
                                        ALTIBASE_BIND *aBind,
                                        acp_sint32_t   aIdx,
                                        ALTIBASE_LONG  aOffset)
{
    #define CDBC_FUNC_NAME "altibase_stmt_fetch_column"

    cdbcABStmt    *sABStmt = (cdbcABStmt *) aABStmt;
    cdbcABRowList *sRowCursor;
    SQLLEN         sLen;
    ALTIBASE_RC    sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(STMT_NOT_FETCHED(sABStmt), FuncSeqError);
    CDBC_TEST_RAISE(aBind == NULL, InvalidNullPtr);

    CDBCLOG_PRINT_VAL("%d", aIdx);
    CDBCLOG_PRINT_VAL("%d", (acp_sint32_t) aOffset);

    CDBCLOG_PRINT_VAL("%d", STMT_IS_STORED(sABStmt));
    if (STMT_IS_STORED(sABStmt))
    {
        /* fetch 하면 커서가 다음으로 이동하므로 FecthedRowCursor 사용 */
        sRowCursor = sABStmt->mRes->mFetchedRowCursor;

        /* altibase_stmt_fetch()에 성공해야한다. */
        CDBC_TEST_RAISE(sRowCursor == NULL, FuncSeqError);

        if (aOffset == ALTIBASE_FETCH_CONT)
        {
            aOffset = sABStmt->mRes->mFetchedColOffset[aIdx];
        }

        sRC = altibase_stmt_copy_col(sABStmt, aBind, sRowCursor,
                                     aIdx, 0, aOffset);
        CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));
    }
    else /* not stored */
    {
        /* aOffset에 ALTIBASE_FETCH_CONT 외의 값을 쓰려면,
           먼저 altibase_stmt_store_result()를 호출해야 한다. */
        CDBC_TEST_RAISE(aOffset != ALTIBASE_FETCH_CONT, FuncSeqError);

        CDBCLOG_CALL("SQLGetData");
        sRC = SQLGetData(sABStmt->mHstmt,
                         aIdx + 1,
                         aBind->buffer_type,
                         aBind->buffer,
                         aBind->buffer_length,
                         &sLen);
        CDBCLOG_BACK_VAL("SQLGetData", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        if (aBind->length != NULL)
        {
            *(aBind->length) = sLen;
        }
        if (aBind->is_null != NULL)
        {
            *(aBind->is_null) = (sLen == SQL_NULL_DATA)
                              ? ALTIBASE_TRUE : ALTIBASE_FALSE;
        }
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(InvalidNullPtr);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_stmt(sABStmt, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

