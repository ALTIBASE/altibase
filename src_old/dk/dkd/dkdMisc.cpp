/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $id$
 **********************************************************************/

#include <dkdMisc.h>
#include <dktDef.h>
#include <dkpDef.h>


/***********************************************************************
 * Description: 원격 테이블에서 Fetch 해야 하는 행의 카운트 정보를 얻는다.
 *
 *  aFetchRowCnt  - [OUT] 원격 테이블 의 Fetch Row Count
 *
 **********************************************************************/
IDE_RC  dkdMisc::getFetchRowCnt( UInt  aFetchRowSize,
                                 UInt  aFetchBuffSize,
                                 UInt  *aFetchRowCnt,
                                 UInt  aStmtType )
{

    UInt   sFetchRowCnt     = 0;
    UInt   sBuffFetchRowCnt = 0;

    if ( aFetchRowSize > 0)
    {
        /* REMOTE_TABLE_STORE 버퍼 사이즈를 기준으로 계산된 Fetch Row Count */
        sBuffFetchRowCnt = aFetchBuffSize / aFetchRowSize;

        if ( ( aStmtType == DKT_STMT_TYPE_REMOTE_TABLE_STORE ) ||
             ( aStmtType == DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT ) )
        {
            /* DK 내부에서 정한 Fetch Row Count */
            if ( aFetchRowSize > DKP_ADLP_PACKET_DATA_MAX_LEN )
            {
                sFetchRowCnt = DKT_FETCH_ROW_COUNT_FOR_LARGE_RECORD;
            }
            else
            {
                sFetchRowCnt = DKT_FETCH_ROW_COUNT_FOR_SMALL_RECORD;
            }

            /* 두개의 Fetch Row Count를 비교 해서 큰값을 Fetch Row Count로 정한다.  */
            if( sFetchRowCnt < sBuffFetchRowCnt )
            {
                sFetchRowCnt = sBuffFetchRowCnt;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else /*REMOTE_TABLE*/
        {
            sFetchRowCnt = sBuffFetchRowCnt;
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST_RAISE( sFetchRowCnt == 0, ERR_INVALID_BUFFER_SIZE );

    *aFetchRowCnt = sFetchRowCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_BUFFER_SIZE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKD_INVALID_BUFFER_SIZE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

