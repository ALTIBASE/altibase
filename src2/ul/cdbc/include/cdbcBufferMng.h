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

#ifndef CDBC_BUFFERMNG_H
#define CDBC_BUFFERMNG_H 1

ACP_EXTERN_C_BEGIN



#define CDBC_ALIGN_SIZE     8   /**< 버퍼 시작 주소가 홀수로 시작하는것을 막기 위함 */

/**
 * 지정한 변수의 값을 CDBC_ALIGN_SIZE의 배수로 맞춘다.
 *
 * CDBC_ALIGN_SIZE의 배수가 아니면,
 * 배수가 되기 위해 부족한 만큼을 지정한 변수에 더해준다.
 *
 * @param[in,out] s CDBC_ALIGN_SIZE의 배수로 맞출 값을 담은 변수
 */
#define CDBC_ADJUST_ALIGN(s) do\
{\
    if (((s) % CDBC_ALIGN_SIZE) != 0)\
    {\
        (s) += (CDBC_ALIGN_SIZE - ((s) % CDBC_ALIGN_SIZE));\
    }\
    ACE_DASSERT(((s) % CDBC_ALIGN_SIZE) == 0);\
} while (ACP_FALSE)



/**
 * NULL이 아니면 메모리를 반환한다.
 *
 * @param[in] m 반환할 메모리 주소를 담은 변수
 */
#define SAFE_FREE(m) do\
{\
    CDBCLOG_PRINT_VAL("%p", m);\
    if ((m) != NULL)\
    {\
        acpMemFree(m);\
    }\
} while (ACP_FALSE)

/**
 * NULL이 아니면 메모리를 반환하고 변수를 초기화한다.
 *
 * @param[in,out] m 반환할 메모리 주소를 담은 변수
 */
#define SAFE_FREE_AND_CLEAN(m) do\
{\
    CDBCLOG_PRINT_VAL("%p", m);\
    if ((m) != NULL)\
    {\
        acpMemFree(m);\
        (m) = NULL;\
    }\
} while (ACP_FALSE)



#define CDBC_BUFFER_HEAD            ALTIBASE_SEEK_SET
#define CDBC_BUFFER_TAIL            ALTIBASE_SEEK_END



typedef struct cdbcBufferItm
{
    acp_char_t             *mBuffer;
    acp_uint32_t            mBufferLength;
    ALTIBASE_LONG           mLength;
    struct cdbcBufferItm   *mNext;
} cdbcBufferItm;

typedef struct cdbcBufferMng
{
    cdbcBufferItm  *mHead;
    cdbcBufferItm  *mTail;
} cdbcBufferMng;



CDBC_INTERNAL cdbcBufferItm * altibase_new_buffer (cdbcBufferMng *aBufMng,
                                                   acp_sint32_t   aBufSize,
                                                   acp_sint32_t   aWhence);
CDBC_INTERNAL void altibase_clean_buffer (cdbcBufferMng *aBufMng);



ACP_EXTERN_C_END

#endif /* CDBC_BUFFERMNG_H */

