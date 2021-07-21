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

#ifndef _O_ULN_META_H_
#define _O_ULN_META_H_ 1

#include <ulnTypes.h>

/*
 * BUGBUG : 지금의 ulnBindInfo.cpp 는 다른 이름으로 바꾸고,
 *          순수하게 ulnBindInfo 만 다루는 모듈을 ulnBindInfo.cpp 로 만들고,
 *          아래의 ulnBindInfo 구조체를 ulnBindInfo.h 로 옮겨야 한다.
 */
struct ulnMeta
{
    ulnCTypeID   mCTYPE;
    ulnMTypeID   mMTYPE;

    acp_sint16_t mTypeIdFromUser;

    acp_uint32_t mLanguage;

    /*
     * 아래의 멤버들은 M$ ODBC 에서 정의하고 있는 것들이다. 적절하게 잘 매핑시켜 주어야 한다.
     */
    ulvULen      mLength;               /* SQL_DESC_LENGTH */
    ulvSLen      mOctetLength;          /* SQL_DESC_OCTET_LENGTH */

    acp_sint16_t mNullable;             /* SQL_DESC_NULLABLE */
    acp_sint16_t mUpdatable;            /* PROJ-1789 */

    acp_sint16_t mOdbcType;             /* SQL_DESC_TYPE */
    acp_sint16_t mConciseType;          /* SQL_DESC_CONCISE_TYPE */
    acp_sint16_t mDatetimeIntCode;      /* SQL_DESC_DATETIME_INTERVAL_CODE */
    acp_sint32_t mDatetimeIntPrecision; /* SQL_DESC_DATETIME_INTERVAL_PRECISION */

    acp_sint16_t mOdbcPrecision;        /* SQL_DESC_PRECISION */
    acp_sint16_t mOdbcScale;            /* SQL_DESC_SCALE */
    acp_sint16_t mFixedPrecScale;       /* SQL_DESC_FIXED_PREC_SCALE */
    acp_sint32_t mNumPrecRadix;         /* SQL_DESC_NUM_PREC_RADIX */
};

/*
 * 초기화
 */
void   ulnMetaInitialize(ulnMeta *aMeta);

/*
 * 멤버 읽기 / 쓰기
 */

ACP_INLINE void ulnMetaSetCTYPE(ulnMeta *aMeta, ulnCTypeID aCTYPE)
{
    aMeta->mCTYPE = aCTYPE;
}

ACP_INLINE ulnCTypeID ulnMetaGetCTYPE(ulnMeta *aMeta)
{
    return aMeta->mCTYPE;
}

ACP_INLINE void ulnMetaSetMTYPE(ulnMeta *aMeta, ulnMTypeID aMTYPE)
{
    aMeta->mMTYPE = aMTYPE;
}

ACP_INLINE ulnMTypeID ulnMetaGetMTYPE(ulnMeta *aMeta)
{
    return aMeta->mMTYPE;
}

ACP_INLINE void ulnMetaSetLanguage(ulnMeta *aMeta, acp_uint32_t aLanguage)
{
    aMeta->mLanguage = aLanguage;
}

ACP_INLINE acp_uint32_t ulnMetaGetLanguage(ulnMeta *aMeta)
{
    return aMeta->mLanguage;
}

ACP_INLINE void ulnMetaSetOdbcType(ulnMeta *aMeta, acp_sint16_t aOdbcType)
{
    aMeta->mOdbcType = aOdbcType;
}

ACP_INLINE acp_sint16_t ulnMetaGetOdbcType(ulnMeta *aMeta)
{
    return aMeta->mOdbcType;
}

ACP_INLINE void ulnMetaSetOdbcConciseType(ulnMeta *aMeta, acp_sint16_t aConciseType)
{
    aMeta->mConciseType = aConciseType;
}

ACP_INLINE acp_sint16_t ulnMetaGetOdbcConciseType(ulnMeta *aMeta)
{
    return aMeta->mConciseType;
}

ACP_INLINE void ulnMetaSetOdbcDatetimeIntCode(ulnMeta *aMeta, acp_sint16_t aIntervalCode)
{
    aMeta->mDatetimeIntCode = aIntervalCode;
}

ACP_INLINE acp_sint16_t ulnMetaGetOdbcDatetimeIntCode(ulnMeta *aMeta)
{
    return aMeta->mDatetimeIntCode;
}

ACP_INLINE void ulnMetaSetOdbcDatetimeIntPrecision(ulnMeta *aMeta, acp_sint32_t aIntervalPrecision)
{
    aMeta->mDatetimeIntPrecision = aIntervalPrecision;
}

ACP_INLINE acp_sint32_t ulnMetaGetOdbcDatetimeIntPrecision(ulnMeta *aMeta)
{
    return aMeta->mDatetimeIntPrecision;
}

ACP_INLINE void ulnMetaSetOdbcLength(ulnMeta *aMeta, ulvULen aLength)
{
    aMeta->mLength = aLength;
}

ACP_INLINE ulvULen ulnMetaGetOdbcLength(ulnMeta *aMeta)
{
    return aMeta->mLength;
}

ACP_INLINE void ulnMetaSetOctetLength(ulnMeta *aMeta, ulvSLen aOctetLength)
{
    /*
     * Octet Length 는
     *      APD, ARD 에서는 bind parameter, bind col 에서 주는 버퍼 사이즈.
     *      IPD 에서는 ???
     *      IRD 에서는 bind column info 로 넘어온 precision.
     *          여기서, 그것을 담는데 필요한 버퍼의 크기를 계산한 값.
     *
     *      imp desc 에서는 null term 을 위한 공간을 제외한 길이.
     *      app desc 에서는 null term 을 위한 공간을 포함한 길이.
     */
    aMeta->mOctetLength = aOctetLength;
}

ACP_INLINE ulvSLen ulnMetaGetOctetLength(ulnMeta *aMeta)
{
    return aMeta->mOctetLength;
}

ACP_INLINE void ulnMetaSetPrecision(ulnMeta *aMeta, acp_sint16_t aPrecision)
{
    aMeta->mOdbcPrecision = aPrecision;
}

ACP_INLINE acp_sint16_t ulnMetaGetPrecision(ulnMeta *aMeta)
{
    return aMeta->mOdbcPrecision;
}

ACP_INLINE void ulnMetaSetScale(ulnMeta *aMeta, acp_sint16_t aScale)
{
    aMeta->mOdbcScale = aScale;
}

ACP_INLINE acp_sint16_t ulnMetaGetScale(ulnMeta *aMeta)
{
    return aMeta->mOdbcScale;
}

ACP_INLINE void ulnMetaSetFixedPrecScale(ulnMeta *aMeta, acp_sint16_t aFixedPrecScale)
{
    aMeta->mFixedPrecScale = aFixedPrecScale;
}

ACP_INLINE acp_sint16_t ulnMetaGetFixedPrecScale(ulnMeta *aMeta)
{
    return aMeta->mFixedPrecScale;
}

ACP_INLINE void ulnMetaSetNumPrecRadix(ulnMeta *aMeta, acp_sint32_t aNumPrecRadix)
{
    aMeta->mNumPrecRadix = aNumPrecRadix;
}

ACP_INLINE acp_sint32_t ulnMetaGetNumPrecRadix(ulnMeta *aMeta)
{
    return aMeta->mNumPrecRadix;
}

ACP_INLINE void ulnMetaSetNullable(ulnMeta *aMeta, acp_sint16_t aNullable)
{
    aMeta->mNullable = aNullable;
}

ACP_INLINE acp_sint16_t ulnMetaGetNullable(ulnMeta *aMeta)
{
    return aMeta->mNullable;
}

/*
 * ulnMeta 의 정보들을 채우는 함수들
 */
void ulnMetaBuild4IpdByMeta(ulnMeta     *aMeta,
                            ulnMTypeID   aMTYPE,
                            acp_sint16_t aSQL_TYPE,
                            ulvULen      aColumnSize,
                            acp_sint16_t aDecimalDigits);

void ulnMetaBuild4IpdByStmt(ulnStmt      *aStmt,
                            acp_sint16_t  aRecNumber,
                            acp_sint16_t  aFieldIdentifier,
                            acp_uint32_t  aValue);

void ulnMetaAdjustIpdByMeta(ulnDbc      *aDbc,
                            ulnMeta     *aMeta,
                            acp_ulong_t aColumnSize,
                            ulnMTypeID  aMTYPE);

void ulnMetaAdjustIpdByStmt(ulnDbc       *aDbc,
                            ulnStmt      *aStmt,
                            acp_sint16_t  aRecNumber,
                            acp_ulong_t   aPrecision);

void ulnMetaBuild4Ird(ulnDbc       *aDbc,
                      ulnMeta      *aMeta,
                      ulnMTypeID    aMTYPE,
                      ulvSLen       aPrecision,
                      acp_sint32_t  aScale,
                      acp_uint8_t   aNullable);

void ulnMetaBuild4ArdApd(ulnMeta      *aAppMeta,
                         ulnCTypeID    aCTYPE,
                         acp_sint16_t  aSQL_C_TYPE,
                         ulvSLen       aBufferLength);

/* PROJ-1789 Updatable Scrollable Cursor */

/**
 * Updatable 설정을 얻는다.
 *
 * @param[in] aMeta   Meta object
 *
 * @return Updatable: SQL_ATTR_WRITE,
 *                    SQL_ATTR_READONLY,
 *                    SQL_ATTR_READWRITE_UNKNOWN
 */
#define ulnMetaGetUpdatable(aMetaPtr) \
    ( (aMetaPtr)->mUpdatable )

/**
 * Updtable을 설정한다.
 *
 * @param[in] aMeta        Meta object
 * @param[in] aUpdatable   Updatable: SQL_ATTR_WRITE,
 *                                    SQL_ATTR_READONLY,
 *                                    SQL_ATTR_READWRITE_UNKNOWN
 */
#define ulnMetaSetUpdatable(aMetaPtr, aUpdatable) do\
{\
    (aMetaPtr)->mUpdatable = (aUpdatable);\
} while (ACP_FALSE)

#endif /* _O_ULN_META_H_ */
