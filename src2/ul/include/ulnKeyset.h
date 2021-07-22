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

#ifndef _O_ULN_KEYSET_H_
#define _O_ULN_KEYSET_H_ 1

#include <uluArray.h>
#include <ulnPrivate.h>

#define ULN_KEYSET_INVALID_POSITION     -1
#define ULN_KEYSET_KEY_NOT_EXISTS       1
#define ULN_KEYSET_KEY_EXISTS           0
#define ULN_KEYSET_FILL_ALL             -1

#define ULN_KEYSET_KSAB_UNIT_COUNT              256
#define ULN_KEYSET_MAX_KEY_IN_KSA               256
#define ULN_KEYSET_MAX_KEY_SIZE                 8
#define ULN_KEYSET_HASH_BUCKET_COUNT            1000

typedef struct ulnKey
{
    acp_sint64_t    mSeq;                               /**< seq number */
    acp_uint8_t     mKeyValue[ULN_KEYSET_MAX_KEY_SIZE]; /**< _PROWID */
} ulnKey;

typedef struct ulnKSAB
{
    acp_uint32_t    mCount;       /**< mBlock 에 현재 할당되어 있는 element 의 갯수 */
    acp_uint32_t    mUsedCount;   /**< 사용하고 있는 element 의 갯수 */
    ulnKey        **mBlock;       /**< KSA 배열 */
} ulnKSAB;

struct ulnKeyset
{
    ulnStmt                 *mParentStmt;   /**< parent statement */
    acp_sint64_t             mKeyCount;     /**< 서버로부터 수신해서 갖고있는 _PROWID 의 갯수 */
    acp_bool_t               mIsFullFilled;

    ulnKSAB                  mSeqKeyset;    /**< Seq-Keyset */
    acl_hash_table_t         mHashKeyset;   /**< Hash-Keyset */

    acl_mem_area_t          *mChunk;        /**< DATA가 CACHING될 메모리 공간 */
    acl_mem_area_snapshot_t  mChunkStatus;  /**<  */
};

ACI_RC          ulnKeysetCreate(ulnKeyset **aKeyset);
void            ulnKeysetDestroy(ulnKeyset *aKeyset);
ACI_RC          ulnKeysetInitialize(ulnKeyset *aKeyset);
acp_sint32_t    ulnKeysetGetKeyCount(ulnKeyset *aKeyset);
acp_bool_t      ulnKeysetIsFullFilled(ulnKeyset *aKeyset);
void            ulnKeysetSetFullFilled(ulnKeyset *aKeyset, acp_bool_t aIsFullFilled);
ACI_RC          ulnKeysetMarkChunk(ulnKeyset *aKeyset);
ACI_RC          ulnKeysetBackChunkToMark(ulnKeyset *aKeyset);
acp_sint32_t    ulnKeysetIsKeyExists(ulnKeyset *aKeyset, acp_sint64_t aPosition);
ACI_RC          ulnKeysetAddNewKey(ulnKeyset *aKeyset, acp_uint8_t *aKeyValue);
acp_uint8_t *   ulnKeysetGetKeyValue(ulnKeyset *aKeyset, acp_sint64_t aSeq);
acp_sint64_t    ulnKeysetGetSeq(ulnKeyset *aKeyset, acp_uint8_t *aKeyValue);

#endif /* _O_ULN_KEYSET_H_ */
