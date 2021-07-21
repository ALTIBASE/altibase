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
#include <ulnBindParameter.h>
#include <ulnBindParamDataIn.h>
#include <ulnBindInfo.h>

#define scAPD   ULN_SCALE_FUNC_APD
#define scIPD   ULN_SCALE_FUNC_IPD

#define prCOL   ULN_PREC_FUNC_COLSIZE
#define prCO2   ULN_PREC_FUNC_COLSIZE2
#define prAPD   ULN_PREC_FUNC_APD
#define prIPD   ULN_PREC_FUNC_IPD
#define prBUF   ULN_PREC_FUNC_BUFFERSIZE

#define mNULL   ULN_MTYPE_NULL
#define mCHAR   ULN_MTYPE_CHAR
#define mVCHR   ULN_MTYPE_VARCHAR
#define mNCHR   ULN_MTYPE_NCHAR
#define mNVCH   ULN_MTYPE_NVARCHAR
#define mBIN    ULN_MTYPE_BINARY
#define mGEOM   ULN_MTYPE_GEOMETRY
#define mBIT    ULN_MTYPE_BIT
#define mVBIT   ULN_MTYPE_VARBIT
#define mBYTE   ULN_MTYPE_BYTE
#define mVBYTE  ULN_MTYPE_VARBYTE
#define mNIB    ULN_MTYPE_NIBBLE
#define mSINT   ULN_MTYPE_SMALLINT
#define mINT    ULN_MTYPE_INTEGER
#define mBINT   ULN_MTYPE_BIGINT
#define mNUMB   ULN_MTYPE_NUMBER
#define mNUME   ULN_MTYPE_NUMERIC
#define mREAL   ULN_MTYPE_REAL
#define mFLOT   ULN_MTYPE_FLOAT
#define mDBL    ULN_MTYPE_DOUBLE
#define mDATE   ULN_MTYPE_TIMESTAMP
#define mINTR   ULN_MTYPE_INTERVAL
#define mBLOB   ULN_MTYPE_BLOB
#define mCLOB   ULN_MTYPE_CLOB
#define mBLOC   ULN_MTYPE_BLOB_LOCATOR
#define mCLOC   ULN_MTYPE_CLOB_LOCATOR
#define mX      ULN_MTYPE_MAX

/*
 * ----------------------------
 * Bind Param Data In Functions
 * ----------------------------
 */

#define fOLD    ULN_BIND_PARAMDATA_IN_FUNC_OLD
#define fcCHAR  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_CHAR
#define fcSINT  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_SMALLINT
#define fcINT   ULN_BIND_PARAMDATA_IN_FUNC_CHAR_INTEGER
#define fcBINT  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BIGINT
#define fcNUME  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_NUMERIC
#define fcBIT   ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BIT
#define fcREAL  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_REAL
#define fcFLOT  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_FLOAT
#define fcDBL   ULN_BIND_PARAMDATA_IN_FUNC_CHAR_DOUBLE
#define fcBIN   ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BINARY
#define fcNIBB  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_NIBBLE
#define fcBYTE  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_BYTE
#define fcTMST  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_TIMESTAMP
#define fcINTV  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_INTERVAL
#define fcNCHR  ULN_BIND_PARAMDATA_IN_FUNC_CHAR_NCHAR

#define fnNUME  ULN_BIND_PARAMDATA_IN_FUNC_NUMERIC_NUMERIC

#define fbCHAR  ULN_BIND_PARAMDATA_IN_FUNC_BIT_CHAR
#define fbNCHR  ULN_BIND_PARAMDATA_IN_FUNC_BIT_NCHAR
#define fbNUME  ULN_BIND_PARAMDATA_IN_FUNC_BIT_NUMERIC
#define fbBIT   ULN_BIND_PARAMDATA_IN_FUNC_BIT_BIT
#define fbSINT  ULN_BIND_PARAMDATA_IN_FUNC_BIT_SMALLINT
#define fbINT   ULN_BIND_PARAMDATA_IN_FUNC_BIT_INTEGER
#define fbBINT  ULN_BIND_PARAMDATA_IN_FUNC_BIT_BIGINT
#define fbREAL  ULN_BIND_PARAMDATA_IN_FUNC_BIT_REAL
#define fbDBL   ULN_BIND_PARAMDATA_IN_FUNC_BIT_DOUBLE

#define f2SINT  ULN_BIND_PARAMDATA_IN_FUNC_TO_SMALLINT
#define f2INT   ULN_BIND_PARAMDATA_IN_FUNC_TO_INTEGER
#define f2BINT  ULN_BIND_PARAMDATA_IN_FUNC_TO_BIGINT

#define ffREAL  ULN_BIND_PARAMDATA_IN_FUNC_FLOAT_REAL
#define fdDBL   ULN_BIND_PARAMDATA_IN_FUNC_DOUBLE_DOUBLE

#define fBCHAR  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_CHAR
#define fBNCHR  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_NCHAR
#define fBBIN   ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BINARY
#define fBNUME  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_NUMERIC
#define fBBIT   ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BIT
#define fBNIBB  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_NIBBLE
#define fBBYTE  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BYTE
#define fBSINT  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_SMALLINT
#define fBINT   ULN_BIND_PARAMDATA_IN_FUNC_BINARY_INTEGER
#define fBBINT  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_BIGINT
#define fBDBL   ULN_BIND_PARAMDATA_IN_FUNC_BINARY_DOUBLE
#define fBREAL  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_REAL
#define fBINTV  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_INTERVAL
#define fBTMST  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_TIMESTAMP
#define fBDATE  ULN_BIND_PARAMDATA_IN_FUNC_BINARY_DATE

#define fDtD    ULN_BIND_PARAMDATA_IN_FUNC_DATE_DATE
#define fTtT    ULN_BIND_PARAMDATA_IN_FUNC_TIME_TIME
#define fTStD   ULN_BIND_PARAMDATA_IN_FUNC_TIMESTAMP_DATE
#define fTStT   ULN_BIND_PARAMDATA_IN_FUNC_TIMESTAMP_TIME
#define fTStTS  ULN_BIND_PARAMDATA_IN_FUNC_TIMESTAMP_TIMESTAMP

/*
 * ================================================================================
 * 배열의 구조 :
 *
 *      SQLBindParameter(Stmt, ParamNumber, InOutType,
 *                       CTYPE_n, MTYPE,
 *                       ColumnSize, DecimalDigits,
 *                       ParameterValuePtr,
 *                       BuferLength,
 *                       StrLenOrIndPtr)
 *
 *      처럼 사용자가 CTYPE_n 과 MTYPE 으로 바인드를 했을 때,
 *
 *      서버로 BIND PARAM INFO SET 을 할 때 건네줄
 *
 *          4n     행 : 타입 (MTYPE. 후에 MTD_TYPE 으로 매핑)
 *
 *          4n + 1 행 : precision 을 구하기 위해
 *                      precision 을 구하는 함수의 배열인
 *                      ulnBindInfoPrecisionFuncArray 에서 호출해야 할 함수의 인덱스
 *
 *          4n + 2 행 : scale 을 구하기 위해
 *                      scale 을 구하는 함수의 배열인
 *                      ulnBindInfoScaleFuncArray 에서 호출해야 할 함수의 인덱스
 *
 *          4n + 3 행 : ParamDataIn 을 할 때
 *                      conversion 을 위해서
 *                      gUlnParamDataInFuncArray 에서 호출해야 할 함수의 인덱스
 * ================================================================================
 */

#define ULN_BINDINFO_CTYPE_MULTIPLIER   4

#define ULN_BINDINFO_TYPE_BIAS          0
#define ULN_BINDINFO_PRECISION_BIAS     1
#define ULN_BINDINFO_SCALE_BIAS         2
#define ULN_BINDINFO_DATAIN_FUNC_BIAS   3

static const acp_uint32_t ulnBindInfoMap [ULN_CTYPE_MAX * ULN_BINDINFO_CTYPE_MULTIPLIER] [ULN_MTYPE_MAX] =
{
/*                                                                                                                                                        T                                                                                        */
/*                                                               S                                                                                        I                       I                                       G               N        */
/*                               V               N               M       I                                                                        V       M                       N                       B       C       E               V        */
/*                               A       N       U               A       N       B                       D       B       V       N                A       E                       T                       L       L       O               A        */
/*                               R       U       M               L       T       I               F       O       I       A       I                R       S                       E                       O       O       M       N       R        */
/*               N       C       C       M       E               L       E       G       R       L       U       N       R       B       B        B       T       D       T       R       B       C       B       B       E       C       C        */
/*               U       H       H       B       R       B       I       G       I       E       O       B       A       B       B       Y        Y       A       A       I       V       L       L       L       L       T       H       H        */
/*               L       A       A       E       I       I       N       E       N       A       A       L       R       I       L       T        T       M       T       M       A       O       O       O       O       R       A       A        */
/*               L       R       R       R       C       T       T       R       T       L       T       E       Y       T       E       E        E       P       E       E       L       B       B       C       C       Y       R       R        */
                                                                                                                                                                  
/* NULL */     { mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX     },
/* NULL */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* NULL */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* NULL */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
                                                                                                                                                                  
/* DEFAULT */  { mX,     mCHAR,  mVCHR,  mVCHR,  mVCHR,  mBIT,   mSINT,  mINT,   mBINT,  mREAL,  mDBL,   mDBL,   mBIN,   mVBIT,  mNIB,   mBYTE,   mVBYTE, mDATE,  mDATE,  mDATE,  mX,     mBLOB,  mCLOB,  mX,     mX,     mBIN,   mNCHR,  mNVCH  },
/* DEFAULT */  { 0,      prCOL,  prCOL,  prAPD,  prAPD,  1,      0,      0,      0,      0,      0,      0,      prCOL,  prCOL,  prCOL,  prCO2,   prCO2,  0,      0,      0,      0,      0,      0,      0,      0,      0,      prCOL,  prCOL  },
/* DEFAULT */  { 0,      0,      0,      scAPD,  scAPD,  0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* DEFAULT */  { 0,      fcCHAR, fcCHAR, fcCHAR, fcCHAR, fbBIT,  f2SINT, f2INT,  f2BINT, ffREAL, fdDBL,  fdDBL,  fBBIN,  fBBIT,  fBNIBB, fBBYTE,  fBBYTE, fTStTS, fTStD,  fTStT,  0,      0,      0,      0,      0,      fBBIN,  fcNCHR, fcNCHR },
                                                                                                                                                                 
/* CHAR */     { mCHAR,  mCHAR,  mVCHR,  mNUME,  mNUME,  mBIT,   mSINT,  mINT,   mBINT,  mREAL,  mFLOT,  mDBL,   mBIN,   mVBIT,  mNIB,   mBYTE,   mVBYTE, mDATE,  mDATE,  mDATE,  mINTR,  mBLOB,  mCLOB,  mX,     mX,     mX,     mNCHR,  mNVCH  },
/* CHAR */     { 0,      prCOL,  prCOL,  prBUF,  prBUF,  1,      prBUF,  prBUF,  prBUF,  prBUF,  prBUF,  prBUF,  prCOL,  prCOL,  prCOL,  prCO2,   prCO2,  prBUF,  prBUF,  prBUF,  prBUF,  0,      0,      0,      0,      0,      prCOL,  prCOL  },
/* CHAR */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* CHAR */     { 0,      fcCHAR, fcCHAR, fcNUME, fcNUME, fcBIT,  fcSINT, fcINT,  fcBINT, fcREAL, fcFLOT, fcDBL,  fcBIN,  fcBIT,  fcNIBB, fcBYTE,  fcBYTE, fcTMST, fcTMST, fcTMST, fcINTV, 0,      0,      0,      0,      0,      fcNCHR, fcNCHR },
                                                                                                                                                                  
/* NUMERIC */  { mNUME,  mNUME,  mNUME,  mNUME,  mNUME,  mX,     mNUME,  mNUME,  mNUME,  mNUME,  mFLOT,  mNUME,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mNUME,  mNUME  },
/* NUMERIC */  { 0,      prAPD,  prAPD,  prIPD,  prIPD,  1,      prAPD,  prAPD,  prAPD,  prAPD,  prAPD,  prAPD,  0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      prAPD,  prAPD  },
/* NUMERIC */  { 0,      scAPD,  scAPD,  scIPD,  scIPD,  0,      scAPD,  scAPD,  scAPD,  scAPD,  scAPD,  scAPD,  0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      scAPD,  scAPD  },
/* NUMERIC */  { 0,      fnNUME, fnNUME, fnNUME, fnNUME, 0,      fnNUME, fnNUME, fnNUME, fnNUME, fnNUME, fnNUME, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      fnNUME, fnNUME },
                                                                                                                                                                  
/* BIT */      { mBIT,   mCHAR,  mVCHR,  mNUME,  mNUME,  mBIT,   mSINT,  mINT,   mBINT,  mREAL,  mREAL,  mDBL,   mX,     mBIT,   mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mNCHR,  mNVCH  },
/* BIT */      { 0,      prCOL,  prCOL,  prIPD,  prIPD,  1,      0,      0,      0,      0,      0,      0,      0,      prCOL,  0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      prCOL,  prCOL  },
/* BIT */      { 0,      0,      0,      scIPD,  scIPD,  0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* BIT */      { 0,      fbCHAR, fbCHAR, fbNUME, fbNUME, fbBIT,  fbSINT, fbINT,  fbBINT, fbREAL, fbREAL, fbDBL,  0,      fbBIT,  0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      fbNCHR, fbNCHR },
                                                                                                                                                                  
/* STINYINT */ { mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mX,     mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mSINT,  mSINT  },
/* STINYINT */ { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* STINYINT */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* STINYINT */ { 0,      f2SINT, f2SINT, f2SINT, f2SINT, 0,      f2SINT, f2SINT, f2SINT, f2SINT, f2SINT, f2SINT, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2SINT, f2SINT },
                                                                                                                                                                  
/* UTINYINT */ { mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mX,     mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mSINT,  mSINT  },
/* UTINYINT */ { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* UTINYINT */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* UTINYINT */ { 0,      f2SINT, f2SINT, f2SINT, f2SINT, 0,      f2SINT, f2SINT, f2SINT, f2SINT, f2SINT, f2SINT, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2SINT, f2SINT },
                                                                                                                                                                  
/*               N       C       V       N       N       B       S       I       B       R       F       D       B       V       N       B        V       T       D       T       I       B       C       B       C       G       N       N        */
/*               U       H       A       U       U       I       M       N       I       E       L       O       I       A       I       Y        A       I       A       I       N       L       L       L       L       E       C       V        */
/*               L       A       R       M       M       T       A       T       G       A       O       U       N       R       B       T        R       M       T       M       T       O       O       O       O       O       H       A        */
/*               L       R       C       B       E               L       E       I       L       A       B       A       B       B       E        B       E       E       E       E       B       B       B       B       M       A       R        */
/*                               H       E       R               L       G       N               T       L       R       I       L                Y       S                       R                       L       L       E       R       C        */
/*                               A       R       I               I       E       T                       E       Y       T       E                T       T                       V                       O       O       T               H        */
/*                               R               C               N       R                                                                        E       A                       A                       C       C       R               A        */
/*                                                               T                                                                                        M                       L                                       Y               R        */
/*                                                                                                                                                        P                                                                        */
                                                                                                                                                         
/* SSHORT */   { mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mX,     mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mSINT,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mSINT,  mSINT  },
/* SSHORT */   { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* SSHORT */   { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* SSHORT */   { 0,      f2SINT, f2SINT, f2SINT, f2SINT, 0,      f2SINT, f2SINT, f2SINT, f2SINT, f2SINT, f2SINT, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2SINT, f2SINT },
                                                                                                                                                                  
/* USHORT */   { mINT,   mINT,   mINT,   mINT,   mINT,   mX,     mSINT,  mINT,   mINT,   mINT,   mINT,   mINT,   mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mSINT,  mSINT  },
/* USHORT */   { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* USHORT */   { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* USHORT */   { 0,      f2INT,  f2INT,  f2INT,  f2INT,  0,      f2SINT, f2INT,  f2INT,  f2INT,  f2INT,  f2INT,  0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2SINT, f2SINT },
                                                                                                                                                                  
/* SLONG */    { mINT,   mINT,   mINT,   mINT,   mINT,   mX,     mSINT,  mINT,   mINT,   mINT,   mINT,   mINT,   mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mINT,   mINT   },
/* SLONG */    { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* SLONG */    { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* SLONG */    { 0,      f2INT,  f2INT,  f2INT,  f2INT,  0,      f2SINT, f2INT,  f2INT,  f2INT,  f2INT,  f2INT,  0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2INT,  f2INT  },
                                                                                                                                                                  
/* ULONG */    { mBINT,  mBINT,  mBINT,  mBINT,  mBINT,  mX,     mSINT,  mINT,   mBINT,  mBINT,  mBINT,  mBINT,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mINT,   mINT   },
/* ULONG */    { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* ULONG */    { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* ULONG */    { 0,      f2BINT, f2BINT, f2BINT, f2BINT, 0,      f2SINT, f2INT,  f2BINT, f2BINT, f2BINT, f2BINT, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2INT,  f2INT  },
                                                                                                                                                                  
/* SBIGINT */  { mBINT,  mBINT,  mBINT,  mBINT,  mBINT,  mX,     mSINT,  mINT,   mBINT,  mBINT,  mBINT,  mBINT,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mBINT,  mBINT  },
/* SBIGINT */  { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* SBIGINT */  { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* SBIGINT */  { 0,      f2BINT, f2BINT, f2BINT, f2BINT, 0,      f2SINT, f2INT,  f2BINT, f2BINT, f2BINT, f2BINT, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2BINT, f2BINT },
                                                                                                                                                                  
/* UBIGINT */  { mBINT,  mBINT,  mBINT,  mBINT,  mBINT,  mX,     mSINT,  mINT,   mBINT,  mBINT,  mBINT,  mBINT,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mBINT,  mBINT  },
/* UBIGINT */  { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* UBIGINT */  { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* UBIGINT */  { 0,      f2BINT, f2BINT, f2BINT, f2BINT, 0,      f2SINT, f2INT,  f2BINT, f2BINT, f2BINT, f2BINT, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2BINT, f2BINT },
                                                                                                                                                                  
/* FLOAT */    { mREAL,  mREAL,  mREAL,  mREAL,  mREAL,  mX,     mREAL,  mREAL,  mREAL,  mREAL,  mREAL,  mREAL,  mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mREAL,  mREAL  },
/* FLOAT */    { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* FLOAT */    { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* FLOAT */    { 0,      ffREAL, ffREAL, ffREAL, ffREAL, 0,      ffREAL, ffREAL, ffREAL, ffREAL, ffREAL, ffREAL, 0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ffREAL, ffREAL },
                                                                                                                                                                  
/* DOUBLE */   { mDBL,   mDBL,   mDBL,   mDBL,   mDBL,   mX,     mDBL,   mDBL,   mDBL,   mDBL,   mDBL,   mDBL,   mX,     mX,     mX,     mX,      mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mDBL,   mDBL   },
/* DOUBLE */   { 0,      0,      0,      0,      0,      1,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* DOUBLE */   { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* DOUBLE */   { 0,      fdDBL,  fdDBL,  fdDBL,  fdDBL,  0,      fdDBL,  fdDBL,  fdDBL,  fdDBL,  fdDBL,  fdDBL,  0,      0,      0,      0,       0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      fdDBL,  fdDBL  },
                                                                                                                                                                 
/*                                                                                                                                                       T                                                                                        */
/*                                                               S                                                                                       I                       I                                       G               N        */
/*                               V               N               M       I                                                                       V       M                       N                       B       C       E               V        */
/*                               A       N       U               A       N       B                       D       B       V       N               A       E                       T                       L       L       O               A        */
/*                               R       U       M               L       T       I               F       O       I       A       I               R       S                       E                       O       O       M       N       R        */
/*               N       C       C       M       E               L       E       G       R       L       U       N       R       B       B       B       T       D       T       R       B       C       B       B       E       C       C        */
/*               U       H       H       B       R       B       I       G       I       E       O       B       A       B       B       Y       Y       A       A       I       V       L       L       L       L       T       H       H        */
/*               L       A       A       E       I       I       N       E       N       A       A       L       R       I       L       T       T       M       T       M       A       O       O       O       O       R       A       A        */
/* ULN_CTYPE_    L       R       R       R       C       T       T       R       T       L       T       E       Y       T       E       E       E       P       E       E       L       B       B       C       C       Y       R       R        */
                                                                                                                                                                 
/* BINARY */   { mBIN,   mCHAR,  mVCHR,  mNUME,  mNUME,  mBIT,   mSINT,  mINT,   mBINT,  mREAL,  mREAL,  mDBL,   mBIN,   mVBIT,  mNIB,   mBYTE,  mVBYTE, mDATE,  mDATE,  mDATE,  mX,     mBLOB,  mCLOB,  mX,     mX,     mBIN,   mNCHR,  mNVCH  },
/* BINARY */   { 0,      prCOL,  prCOL,  prIPD,  prIPD,  prCOL,  0,      0,      0,      0,      0,      0,      prCOL,  prCOL,  prCOL,  prCOL,  prCOL,  0,      0,      0,      0,      0,      0,      0,      0,      0,      prCOL,  prCOL  },
/* BINARY */   { 0,      0,      0,      scIPD,  scIPD,  0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* BINARY */   { 0,      fBCHAR, fBCHAR, fBNUME, fBNUME, fBBIT,  fBSINT, fBINT,  fBBINT, fBREAL, fBREAL, fBDBL,  fBBIN,  fBBIT,  fBNIBB, fBBYTE, fBBYTE, fBTMST, fBDATE, fBDATE, 0,      0,      0,      0,      0,      fBBIN,  fBNCHR, fBNCHR },
                                                                                                                                                                 
/* DATE */     { mDATE,  mDATE,  mDATE,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mDATE,  mDATE,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mDATE,  mDATE  },
/* DATE */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* DATE */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* DATE */     { 0,      fDtD,   fDtD,   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      fDtD,   fDtD,   0,      0,      0,      0,      0,      0,      0,      fDtD,   fDtD   },
                                                                                                                                                                 
/* TIME */     { mDATE,  mDATE,  mDATE,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mDATE,  mX,     mDATE,  mX,     mX,     mX,     mX,     mX,     mX,     mDATE,  mDATE  },
/* TIME */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* TIME */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* TIME */     { 0,      fTtT,   fTtT,   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      fTtT,   0,      fTtT,   0,      0,      0,      0,      0,      0,      fTtT,   fTtT   },
                                                                                                                                                                 
/* TIMESTAMP */{ mDATE,  mDATE,  mDATE,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mDATE,  mDATE,  mDATE,  mX,     mX,     mX,     mX,     mX,     mX,     mDATE,  mDATE  },
/* TIMESTAMP */{ 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* TIMESTAMP */{ 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* TIMESTAMP */{ 0,      fTStTS, fTStTS, 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      fTStTS, fTStD,  fTStT,  0,      0,      0,      0,      0,      0,      fTStTS, fTStTS },
                                                                                                                                                                 
/* INTERVAL */ { mINTR,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX     },
/* INTERVAL */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* INTERVAL */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* INTERVAL */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
                                                                                                                                                                 
/* BLOB_LOC */ { mBLOB,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mBLOC,  mX,     mBLOC,  mX,     mX,     mX,     mX     },
/* BLOB_LOC */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* BLOB_LOC */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* BLOB_LOC */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2BINT, 0,      f2BINT, 0,      0,      0,      0      },
                                                                                                                                                                 
/* CLOB_LOC */ { mCLOB,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mCLOC,  mX,     mCLOC,  mX,     mX,     mX     },
/* CLOB_LOC */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* CLOB_LOC */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* CLOB_LOC */ { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      f2BINT, 0,      f2BINT, 0,      0,      0      },
                                                                                                                                                                 
/* FILE */     { mBLOB,  mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mX,     mBLOB,  mCLOB,  mX,     mX,     mX,     mX,     mX     },
/* FILE */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* FILE */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* FILE */     { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
                                                                                                                                                                 
/* WCHAR */    { mNCHR,  mCHAR,  mVCHR,  mNUME,  mNUME,  mX,     mSINT,  mINT,   mBINT,  mREAL,  mFLOT,  mDBL,   mBIN,   mVBIT,  mNIB,   mBYTE,  mVBYTE, mDATE,  mDATE,  mDATE,  mINTR,  mBLOB,  mCLOB,  mX,     mX,     mX,     mNCHR,  mNVCH  },
/* WCHAR */    { 0,      prCOL,  prCOL,  prBUF,  prBUF,  1,      prBUF,  prBUF,  prBUF,  prBUF,  prBUF,  prBUF,  prCOL,  prCOL,  prCOL,  prCO2,  prCO2,  prBUF,  prBUF,  prBUF,  prBUF,  0,      0,      0,      0,      0,      prCOL,  prCOL  },
/* WCHAR */    { 0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0      },
/* WCHAR */    { 0,      fcCHAR, fcCHAR, fcNUME, fcNUME, 0,      fcSINT, fcINT,  fcBINT, fcREAL, fcFLOT, fcDBL,  fcBIN,  fcBIT,  fcNIBB, fcBYTE, fcBYTE, fcTMST, fcTMST, fcTMST, fcINTV, 0,      0,      0,      0,      0,      fcNCHR, fcNCHR },
                                                                                                                                                         
/*               N       C       V       N       N       B       S       I       B       R       F       D       B       V       N       B       V       T       D       T       I       B       C       B       C       G       N       N        */
/*               U       H       A       U       U       I       M       N       I       E       L       O       I       A       I       Y       A       I       A       I       N       L       L       L       L       E       C       V        */
/*               L       A       R       M       M       T       A       T       G       A       O       U       N       R       B       T       R       M       T       M       T       O       O       O       O       O       H       A        */
/*               L       R       C       B       E               L       E       I       L       A       B       A       B       B       E       B       E       E       E       E       B       B       B       B       M       A       R        */
/*                               H       E       R               L       G       N               T       L       R       I       L               Y       S                       R                       L       L       E       R       C        */
/*                               A       R       I               I       E       T                       E       Y       T       E               T       T                       V                       O       O       T               H        */
/*                               R               C               N       R                                                                       E       A                       A                       C       C       R               A        */
/*                                                               T                                                                                       M                       L                                       Y               R        */
/*                                                                                                                                                       P                                                                                        */
};

#undef fOLD  
#undef fcCHAR
#undef fcSINT
#undef fcINT 
#undef fcBINT
#undef fcNUME
#undef fcBIT 
#undef fcREAL
#undef fcFLOT
#undef fcDBL 
#undef fcBIN 
#undef fcNIBB
#undef fcBYTE
#undef fcTMST
#undef fcINTV
#undef fcNCHR

#undef fnNUME

#undef fbCHAR
#undef fbNCHR
#undef fbNUME
#undef fbBIT 
#undef fbSINT
#undef fbINT 
#undef fbBINT
#undef fbREAL
#undef fbDBL 

#undef f2SINT
#undef f2INT 
#undef f2BINT

#undef ffREAL
#undef fdDBL 

#undef fBCHAR
#undef fBNCHR
#undef fBBIN 
#undef fBNUME
#undef fBBIT 
#undef fBNIBB
#undef fBBYTE
#undef fBSINT
#undef fBINT 
#undef fBBINT
#undef fBDBL 
#undef fBREAL
#undef fBINTV
#undef fBTMST
#undef fBDATE

#undef fDtD  
#undef fTtT  
#undef fTStD 
#undef fTStT 
#undef fTStTS

#undef sc0
#undef scAPD
#undef scIPD
#undef pr0
#undef prCOL
#undef prCO2
#undef prAPD
#undef prIPD
#undef prBUF

#undef mNULL
#undef mCHAR
#undef mVCHR
#undef mBIN
#undef mGEOM
#undef mBIT
#undef mVBIT
#undef mBYTE
#undef mNIB
#undef mSINT
#undef mINT
#undef mBINT
#undef mNUMB
#undef mNUME
#undef mREAL
#undef mFLOT
#undef mDBL
#undef mDATE
#undef mINTR
#undef mBLOB
#undef mCLOB
#undef mNCHR
#undef mNVCH
#undef mX

/*
 * =====================================================
 * ulnBindInfo 에 세팅할 전송함수를 구하는 함수
 * =====================================================
 */

static ulnParamDataInBuildAnyFunc * const gUlnParamDataInBuildAnyFuncArray[ULN_BIND_PARAMDATA_IN_FUNC_MAX] =
{
    ulnParamDataInBuildAny_OLD,
    ulnParamDataInBuildAny_CHAR_CHAR,
    ulnParamDataInBuildAny_CHAR_SMALLINT,
    ulnParamDataInBuildAny_CHAR_INTEGER,
    ulnParamDataInBuildAny_CHAR_BIGINT,
    ulnParamDataInBuildAny_CHAR_NUMERIC,
    ulnParamDataInBuildAny_CHAR_BIT,
    ulnParamDataInBuildAny_CHAR_REAL,
    ulnParamDataInBuildAny_CHAR_FLOAT,
    ulnParamDataInBuildAny_CHAR_DOUBLE,
    ulnParamDataInBuildAny_CHAR_BINARY,
    ulnParamDataInBuildAny_CHAR_NIBBLE,
    ulnParamDataInBuildAny_CHAR_BYTE,
    ulnParamDataInBuildAny_CHAR_TIMESTAMP,
    ulnParamDataInBuildAny_CHAR_INTERVAL,
    ulnParamDataInBuildAny_CHAR_NCHAR,

    ulnParamDataInBuildAny_NUMERIC_NUMERIC,

    ulnParamDataInBuildAny_BIT_CHAR,
    ulnParamDataInBuildAny_BIT_NCHAR,
    ulnParamDataInBuildAny_BIT_NUMERIC,
    ulnParamDataInBuildAny_BIT_BIT,
    ulnParamDataInBuildAny_BIT_SMALLINT,
    ulnParamDataInBuildAny_BIT_INTEGER,
    ulnParamDataInBuildAny_BIT_BIGINT,
    ulnParamDataInBuildAny_BIT_REAL,
    ulnParamDataInBuildAny_BIT_DOUBLE,

    ulnParamDataInBuildAny_TO_SMALLINT,
    ulnParamDataInBuildAny_TO_INTEGER,
    ulnParamDataInBuildAny_TO_BIGINT,

    ulnParamDataInBuildAny_FLOAT_REAL,
    ulnParamDataInBuildAny_DOUBLE_DOUBLE,

    ulnParamDataInBuildAny_BINARY_CHAR,
    ulnParamDataInBuildAny_BINARY_NCHAR,
    ulnParamDataInBuildAny_BINARY_BINARY,
    ulnParamDataInBuildAny_BINARY_NUMERIC,
    ulnParamDataInBuildAny_BINARY_BIT,
    ulnParamDataInBuildAny_BINARY_NIBBLE,
    ulnParamDataInBuildAny_BINARY_BYTE,
    ulnParamDataInBuildAny_BINARY_SMALLINT,
    ulnParamDataInBuildAny_BINARY_INTEGER,
    ulnParamDataInBuildAny_BINARY_BIGINT,
    ulnParamDataInBuildAny_BINARY_DOUBLE,
    ulnParamDataInBuildAny_BINARY_REAL,
    ulnParamDataInBuildAny_BINARY_INTERVAL,
    ulnParamDataInBuildAny_BINARY_TIMESTAMP,
    ulnParamDataInBuildAny_BINARY_DATE,

    ulnParamDataInBuildAny_DATE_DATE,
    ulnParamDataInBuildAny_TIME_TIME,
    ulnParamDataInBuildAny_TIMESTAMP_DATE,
    ulnParamDataInBuildAny_TIMESTAMP_TIME,
    ulnParamDataInBuildAny_TIMESTAMP_TIMESTAMP
};

ulnParamDataInBuildAnyFunc *ulnBindInfoGetParamDataInBuildAnyFunc(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE)
{
    acp_uint32_t sRow;
    acp_uint32_t sDataInFuncID;

    sRow = (aCTYPE * ULN_BINDINFO_CTYPE_MULTIPLIER) + ULN_BINDINFO_DATAIN_FUNC_BIAS;

    sDataInFuncID = ulnBindInfoMap[sRow][aMTYPE];

    ACE_ASSERT(sDataInFuncID < ULN_BIND_PARAMDATA_IN_FUNC_MAX);

    return gUlnParamDataInBuildAnyFuncArray[sDataInFuncID];
}

/*
 * =====================================================
 * ulnBindInfo 에 세팅할 type 을 구하는 함수
 * =====================================================
 */

ulnMTypeID ulnBindInfoGetMTYPEtoSet(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE)
{
    acp_uint32_t sRow;

    if(aCTYPE > ULN_CTYPE_MAX)
    {
        return ULN_MTYPE_MAX;
    }

    if(aMTYPE > ULN_MTYPE_MAX)
    {
        return ULN_MTYPE_MAX;
    }

    sRow = (aCTYPE * ULN_BINDINFO_CTYPE_MULTIPLIER) + ULN_BINDINFO_TYPE_BIAS;

    return (ulnMTypeID)ulnBindInfoMap[sRow][aMTYPE];
}

/*
 * =====================================================
 * ulnBindInfo 에 세팅할 precision 을 계산하는 함수들
 * =====================================================
 */

static acp_sint32_t ulnBindInfoPrecision_0(ulnMeta          *aAppMeta,
                                           ulnMeta          *aImpMeta,
                                           void             *aUserBuffer,
                                           ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aAppMeta);
    ACP_UNUSED(aImpMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    return 0;
}

static acp_sint32_t ulnBindInfoPrecision_1(ulnMeta          *aAppMeta,
                                           ulnMeta          *aImpMeta,
                                           void             *aUserBuffer,
                                           ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aAppMeta);
    ACP_UNUSED(aImpMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    return 1;
}

static acp_sint32_t ulnBindInfoPrecision_ColSize(ulnMeta          *aAppMeta,
                                                 ulnMeta          *aImpMeta,
                                                 void             *aUserBuffer,
                                                 ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aAppMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    /*
     * 이 함수는 BindInfo 의 mType 이
     *
     *      ULN_MTYPE_VARCHAR
     *      ULN_MTYPE_CHAR
     *      ULN_MTYPE_BINARY
     *      ULN_MTYPE_BYTE
     *      ULN_MTYPE_VARBYTE
     *      ULN_MTYPE_NIBBLE
     *      ULN_MTYPE_VARBIT
     *      ULN_MTYPE_BIT
     *      ULN_MTYPE_GEOMETRY
     *
     * 일 때에만 올바른 값을 리턴한다.
     * SQLBindParameter() 할 때에 ColumnSize parameter 가 IPD record Meta 의
     * OdbcOctetLength 에 저장되기 때문이다.
     */

    return (acp_sint32_t)ulnMetaGetOctetLength(aImpMeta);
}

static acp_sint32_t ulnBindInfoPrecision_ColSize2(ulnMeta          *aAppMeta,
                                                  ulnMeta          *aImpMeta,
                                                  void             *aUserBuffer,
                                                  ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aAppMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    /*
     * 이 함수는 BindInfo 의 mType 이
     *
     *      ULN_MTYPE_VARCHAR
     *      ULN_MTYPE_CHAR
     *      ULN_MTYPE_BINARY
     *      ULN_MTYPE_BYTE
     *      ULN_MTYPE_VARBYTE
     *      ULN_MTYPE_NIBBLE
     *      ULN_MTYPE_VARBIT
     *      ULN_MTYPE_BIT
     *      ULN_MTYPE_GEOMETRY
     *
     * 일 때에만 올바른 값을 리턴한다.
     * SQLBindParameter() 할 때에 ColumnSize parameter 가 IPD record Meta 의
     * OdbcOctetLength 에 저장되기 때문이다.
     */

    return ulnMetaGetOctetLength(aImpMeta) * 2;
}

static acp_sint32_t ulnBindInfoPrecision_APD(ulnMeta          *aAppMeta,
                                             ulnMeta          *aImpMeta,
                                             void             *aUserBuffer,
                                             ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aImpMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    /*
     * 이 함수는 BindInfo 의 mType 이
     *
     *      ULN_MTYPE_NUMBER
     *      ULN_MTYPE_NUMERIC
     *
     * 일 때에만 올바른 값을 리턴한다.
     */

    return ulnMetaGetPrecision(aAppMeta);
}

static acp_sint32_t ulnBindInfoPrecision_IPD(ulnMeta          *aAppMeta,
                                             ulnMeta          *aImpMeta,
                                             void             *aUserBuffer,
                                             ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aAppMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    /*
     * 이 함수는 BindInfo 의 mType 이
     *
     *      ULN_MTYPE_NUMBER
     *      ULN_MTYPE_NUMERIC
     *
     * 일 때에만 올바른 값을 리턴한다.
     */

    return ulnMetaGetPrecision(aImpMeta);
}

static acp_sint32_t ulnBindInfoPrecision_BufferSize(ulnMeta          *aAppMeta,
                                                    ulnMeta          *aImpMeta,
                                                    void             *aUserBuffer,
                                                    ulnIndLenPtrPair *aUserIndLenPair)
{
    acp_sint32_t   sUserBufferSize = ulnMetaGetOctetLength(aAppMeta);
    acp_sint32_t   sUserIndLenValue;

    ACP_UNUSED(aImpMeta);

    /*
     * 이 함수는 BindInfo 의 mType 이
     *
     *      ULN_MTYPE_CHAR
     *      ULN_MTYPE_VARCHAR
     *
     * 이고, IPD record 의 MTYPE 이 fixed size 타입일 때,
     * 즉, ColumnSize 에 0 이 왔을 때, 다시말해서,
     *
     *      SQLBindParameter(SQL_C_CHAR, SQL_INTEGER, 0, 0, ...)
     *
     * 과 같은 형태의 파라미터일 경우에만 호출되며, 그래야 한다.
     *
     * 만약
     *
     *  1. BufferSize 가 0 이 아니라면 happy 하게 바로 BufferSize 를 리턴
     *  2. BufferSize 가 0 이고,
     *     2-1. StrIndOrLenPtr 이 가리키는 버퍼에 길이값이 있을 경우 그 길이를 리턴.
     *     2-2. StrIndOrLenPtr 이 가리키는 버퍼에 SQL_NTS 가 있거나,
     *          StrIndOrLenPtr 이 NULL 일 경우, 버퍼에 있는 글자의 갯수를 세어서 리턴.
     */

    if (sUserBufferSize != 0)
    {
        return sUserBufferSize;
    }

    /*
     * BufferSize 가 0 이다. StrLenOrIndPtr 에 있는 길이를 체크한다.
     */

    sUserIndLenValue = ulnBindGetUserIndLenValue(aUserIndLenPair);

    if (sUserIndLenValue < 0)
    {
        switch (sUserIndLenValue)
        {
            case SQL_NULL_DATA:
                return 0;

            case SQL_NTS:
                return acpCStrLen((acp_char_t *)aUserBuffer, ACP_SINT32_MAX);

            case SQL_DEFAULT_PARAM:
            case SQL_DATA_AT_EXEC:
            default:
                /*
                 * BUGBUG : 이 경우는 SQL_LEN_DATA_AT_EXEC() 매크로에 의한 값이다.
                 *          어떻게 처리할지 생각해 봐야 한다.
                 */
                return 0;
        }
    }
    else
    {
        return sUserIndLenValue;
    }
}

static ulnPrecisionFunc * const ulnBindInfoPrecisionFuncArray[ULN_PREC_FUNC_MAX] =
{
    ulnBindInfoPrecision_0,
    ulnBindInfoPrecision_1,
    ulnBindInfoPrecision_ColSize,
    ulnBindInfoPrecision_ColSize2,
    ulnBindInfoPrecision_APD,
    ulnBindInfoPrecision_IPD,
    ulnBindInfoPrecision_BufferSize
};

ulnPrecisionFunc *ulnBindInfoGetPrecisionFunc(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE)
{
    acp_uint32_t sRow;
    acp_uint32_t sPrecisionFuncID;

    sRow = (aCTYPE * ULN_BINDINFO_CTYPE_MULTIPLIER) + ULN_BINDINFO_PRECISION_BIAS;

    sPrecisionFuncID = ulnBindInfoMap[sRow][aMTYPE];

    ACE_ASSERT(sPrecisionFuncID < ULN_PREC_FUNC_MAX);

    return ulnBindInfoPrecisionFuncArray[sPrecisionFuncID];
}

/*
 * =====================================================
 * ulnBindInfo 에 세팅할 scale 을 계산하는 함수들
 * =====================================================
 */

static acp_sint32_t ulnBindInfoScale_0(ulnMeta          *aAppMeta,
                                       ulnMeta          *aImpMeta,
                                       void             *aUserBuffer,
                                       ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aAppMeta);
    ACP_UNUSED(aImpMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    return 0;
}

static acp_sint32_t ulnBindInfoScale_APD(ulnMeta          *aAppMeta,
                                         ulnMeta          *aImpMeta,
                                         void             *aUserBuffer,
                                         ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aImpMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    return ulnMetaGetScale(aAppMeta);
}

static acp_sint32_t ulnBindInfoScale_IPD(ulnMeta          *aAppMeta,
                                         ulnMeta          *aImpMeta,
                                         void             *aUserBuffer,
                                         ulnIndLenPtrPair *aUserIndLenPair)
{
    ACP_UNUSED(aAppMeta);
    ACP_UNUSED(aUserBuffer);
    ACP_UNUSED(aUserIndLenPair);

    return ulnMetaGetScale(aImpMeta);
}

static ulnScaleFunc * const ulnBindInfoScaleFuncArray[ULN_SCALE_FUNC_MAX] =
{
    ulnBindInfoScale_0,
    ulnBindInfoScale_APD,
    ulnBindInfoScale_IPD
};

ulnScaleFunc *ulnBindInfoGetScaleFunc(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE)
{
    acp_uint32_t sRow;
    acp_uint32_t sScaleFuncID;

    sRow = aCTYPE * ULN_BINDINFO_CTYPE_MULTIPLIER +
           ULN_BINDINFO_SCALE_BIAS;

    sScaleFuncID = ulnBindInfoMap[sRow][aMTYPE];

    ACE_ASSERT(sScaleFuncID < ULN_SCALE_FUNC_MAX);

    return ulnBindInfoScaleFuncArray[sScaleFuncID];
}

/*
 * =========================================================
 * 여기까지 새로 만들고 있는 함수들
 * =========================================================
 */

acp_uint8_t ulnBindInfoGetArgumentsForMTYPE(ulnMTypeID aMTYPE)
{
    /*
     * 0 : 아무것도 의미있지 않음.
     * 1 : mPrecision 만 유의미함.
     * 2 : mPrecision, mScale 둘 다 유의미함.
     */
    switch (aMTYPE)
    {
        case ULN_MTYPE_NULL:

        case ULN_MTYPE_DATE:
        case ULN_MTYPE_TIME:
        case ULN_MTYPE_TIMESTAMP:
        case ULN_MTYPE_INTERVAL:

        case ULN_MTYPE_REAL:
        case ULN_MTYPE_FLOAT:
        case ULN_MTYPE_DOUBLE:

        case ULN_MTYPE_SMALLINT:
        case ULN_MTYPE_INTEGER:
        case ULN_MTYPE_BIGINT:

        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_CLOB:
            return 0;

        case ULN_MTYPE_GEOMETRY:
        case ULN_MTYPE_BINARY:
        case ULN_MTYPE_VARBIT:
        case ULN_MTYPE_NIBBLE:
        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
        case ULN_MTYPE_BIT:
        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_VARCHAR:
        case ULN_MTYPE_NCHAR:
        case ULN_MTYPE_NVARCHAR:
            return 1;

        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:
            return 2;

        default:
            return 0;
    }
}

void ulnBindInfoInitialize(ulnBindInfo *aBindInfo)
{
    aBindInfo->mMTYPE       = ULN_MTYPE_MAX;
    aBindInfo->mLanguage    = 0;
    aBindInfo->mInOutType   = ULN_PARAM_INOUT_TYPE_MAX;

    aBindInfo->mArguments   = 0;
    aBindInfo->mPrecision   = 0;
    aBindInfo->mScale       = 0;

    aBindInfo->mParamDataInBuildAnyFunc = NULL;
}

void ulnBindInfoSetType(ulnBindInfo *aBindInfo, ulnMTypeID aMTYPE)
{
    aBindInfo->mMTYPE = aMTYPE;
}

ulnMTypeID ulnBindInfoGetType(ulnBindInfo *aBindInfo)
{
    return aBindInfo->mMTYPE;
}

void ulnBindInfoSetInOutType(ulnBindInfo *aBindInfo, ulnParamInOutType aInOutType)
{
    aBindInfo->mInOutType = aInOutType;
}

ulnParamInOutType ulnBindInfoGetInOutType(ulnBindInfo *aBindInfo)
{
    return aBindInfo->mInOutType;
}

void ulnBindInfoSetLanguage(ulnBindInfo *aBindInfo, acp_uint32_t aLanguage)
{
    aBindInfo->mLanguage = aLanguage;
}

acp_uint32_t ulnBindInfoGetLanguage(ulnBindInfo *aBindInfo)
{
    return aBindInfo->mLanguage;
}

void ulnBindInfoSetPrecision(ulnBindInfo *aBindInfo, acp_sint32_t aPrecision)
{
    aBindInfo->mPrecision = aPrecision;
}

acp_sint32_t ulnBindInfoGetPrecision(ulnBindInfo *aBindInfo)
{
    return aBindInfo->mPrecision;
}

void ulnBindInfoSetScale(ulnBindInfo *aBindInfo, acp_sint32_t aScale)
{
    aBindInfo->mScale = aScale;
}

acp_sint32_t ulnBindInfoGetScale(ulnBindInfo *aBindInfo)
{
    return aBindInfo->mScale;
}

void ulnBindInfoSetArguments(ulnBindInfo *aBindInfo, acp_uint8_t aArguments)
{
    aBindInfo->mArguments = aArguments;
}

acp_uint8_t ulnBindInfoGetArguments(ulnBindInfo *aBindInfo)
{
    return aBindInfo->mArguments;
}

static const acp_uint8_t gUlnParamTypeMap[ULN_PARAM_INOUT_TYPE_MAX] =
{
    99,
    CMP_DB_PARAM_INPUT,
    CMP_DB_PARAM_OUTPUT,
    CMP_DB_PARAM_INPUT_OUTPUT
};

acp_uint8_t ulnBindInfoCmParamInOutType( ulnBindInfo *aBindInfo )
{
    return gUlnParamTypeMap[aBindInfo->mInOutType];
}

/* BUG-42521 */
ulnParamInOutType ulnBindInfoConvUlnParamInOutType(acp_uint8_t aCmParamInOutType)
{
    ulnParamInOutType sType = ULN_PARAM_INOUT_TYPE_INIT;

    switch (aCmParamInOutType)
    {
        case CMP_DB_PARAM_INPUT:
            sType = ULN_PARAM_INOUT_TYPE_INPUT;
            break;

        case CMP_DB_PARAM_INPUT_OUTPUT:
            sType = ULN_PARAM_INOUT_TYPE_IN_OUT;
            break;

        case CMP_DB_PARAM_OUTPUT:
            sType = ULN_PARAM_INOUT_TYPE_OUTPUT;
            break;

        default:
            sType = ULN_PARAM_INOUT_TYPE_INPUT;
            break;
    }

    return sType;
}

ACI_RC ulnBindInfoBuild4Param(ulnFnContext      *aFnContext,
                              acp_uint32_t       aRowNumber, /* 0 베이스 */
                              ulnDescRec        *aDescRecApd,
                              ulnDescRec        *aDescRecIpd,
                              ulnParamInOutType  aInOutType,
                              acp_bool_t        *aIsChanged)
{
#ifndef COMPILE_SHARDCLI
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);
#endif

    acp_bool_t        sIsChanged = ACP_FALSE;
    ulnMTypeID        sMTYPE     = ULN_MTYPE_MAX;
    acp_sint32_t      sPrecision = 0;
    acp_sint32_t      sScale     = 0;
    acp_uint32_t      sLanguage  = 0;
    ulnParamInOutType sInOutType = ULN_PARAM_INOUT_TYPE_MAX;
    acp_uint8_t       sArguments = 0;
    acp_bool_t        sChangeInOutType = ACP_FALSE;
    ulnBindInfo      *sBindInfo;
    ulnMeta          *sAppMeta;
    ulnMeta          *sImpMeta;

    ulnCTypeID sMetaCTYPE;
    ulnMTypeID sMetaMTYPE;

    sAppMeta  = &aDescRecApd->mMeta;
    sImpMeta  = &aDescRecIpd->mMeta;
    sBindInfo = &aDescRecApd->mBindInfo;

    sMetaCTYPE = ulnMetaGetCTYPE(sAppMeta);
    sMetaMTYPE = ulnMetaGetMTYPE(sImpMeta);

#ifdef COMPILE_SHARDCLI
    /* PROJ-2739 Client-side Sharding LOB
     * locator InBind이면 src, dest 노드 동일 여부를 따져서
     * OUT으로 바꿀 것인지를 결정한다. */
    if ( ulsdTypeIsLocInBoundLob( sMetaCTYPE,
                                  aInOutType ) == ACP_TRUE )
    {
        ACI_TEST( ulsdLobBindInfoBuild4Param(
                      aFnContext,
                      ulnBindCalcUserDataAddr(aDescRecApd, aRowNumber),
                      &sChangeInOutType )
                  != ACI_SUCCESS );
    }
    else
    {
        sChangeInOutType = ACP_FALSE;
    }
#endif

    /*
     * ulnBindInfo 구조체에 세팅할 새 값들을 준비한다.
     */

    sMTYPE = ulnBindInfoGetMTYPEtoSet(sMetaCTYPE, sMetaMTYPE);
    ACI_TEST(sMTYPE == ULN_MTYPE_MAX);

    sBindInfo->mParamDataInBuildAnyFunc = ulnBindInfoGetParamDataInBuildAnyFunc(sMetaCTYPE, sMetaMTYPE);

    sPrecision = ulnTypeGetColumnSizeOfType(sMTYPE, sImpMeta);
    sScale     = ulnTypeGetDecimalDigitsOfType(sMTYPE, sImpMeta);

    sArguments = ulnBindInfoGetArgumentsForMTYPE(sMTYPE);
    sLanguage  = ulnMetaGetLanguage(sAppMeta);

    if (ulnTypeIsMemBoundLob(sMetaMTYPE,
                             sMetaCTYPE) == ACP_TRUE)
    {
        sInOutType = ULN_PARAM_INOUT_TYPE_OUTPUT;
    }
    else if (sChangeInOutType == ACP_TRUE)
    {
        sInOutType = ULN_PARAM_INOUT_TYPE_OUTPUT;
    }
    else
    {
        sInOutType = aInOutType;
    }

    /*
     * ulnBindInfo 에 원래 있던 값들과 비교해서 다르면 새 값으로 세팅한다.
     */

    if (sBindInfo->mMTYPE != sMTYPE)
    {
        sBindInfo->mMTYPE = sMTYPE;
        sIsChanged = ACP_TRUE;
    }

    if (sBindInfo->mPrecision != sPrecision)
    {
        sBindInfo->mPrecision = sPrecision;
        sIsChanged = ACP_TRUE;
    }

    if (sBindInfo->mScale != sScale)
    {
        sBindInfo->mScale = sScale;
        sIsChanged = ACP_TRUE;
    }

    if (sBindInfo->mLanguage != sLanguage)
    {
        sBindInfo->mLanguage = sLanguage;
        sIsChanged = ACP_TRUE;
    }

    if (sBindInfo->mInOutType != sInOutType)
    {
        sBindInfo->mInOutType = sInOutType;
        sIsChanged = ACP_TRUE;
    }

    if (sBindInfo->mArguments != sArguments)
    {
        sBindInfo->mArguments = sArguments;
        sIsChanged = ACP_TRUE;
    }

    *aIsChanged = sIsChanged;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
