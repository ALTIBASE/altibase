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
 

/***********************************************************************
 * $Id: mtfModules.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h> // for win32 porting
#include <mtf.h>

extern mtfModule mtfAbs;
extern mtfModule mtfAcos;
//extern mtfModule mtfAdd;
extern mtfModule mtfAdd2;
extern mtfModule mtfAdd_months;
extern mtfModule mtfAnd;
extern mtfModule mtfAscii;
extern mtfModule mtfAsin;
extern mtfModule mtfAtan;
extern mtfModule mtfAtan2;
extern mtfModule mtfAvg;
//extern mtfModule mtfAvgGroupBy;
extern mtfModule mtfBetween;
extern mtfModule mtfBin_To_Num;
extern mtfModule mtfBinary_length;
extern mtfModule mtfCase2;
extern mtfModule mtfCeil;
extern mtfModule mtfCharacter_length;
extern mtfModule mtfChosung;
extern mtfModule mtfChr;
extern mtfModule mtfConcat;
extern mtfModule mtfCos;
extern mtfModule mtfCosh;
extern mtfModule mtfCount;
extern mtfModule mtfDateadd;
extern mtfModule mtfDatediff;
extern mtfModule mtfDatename;
extern mtfModule mtfDecode;
extern mtfModule mtfDESDecrypt;
extern mtfModule mtfDESEncrypt;
extern mtfModule mtfAESDecrypt;
extern mtfModule mtfAESEncrypt;
extern mtfModule mtfPKCS7PAD;
extern mtfModule mtfPKCS7UNPAD;
extern mtfModule mtfDigits;
extern mtfModule mtfDigest;
extern mtfModule mtfDivide;
//extern mtfModule mtfDivide2;
extern mtfModule mtfDump;
extern mtfModule mtfLdump;
extern mtfModule mtfEqual;
extern mtfModule mtfEqualAll;
extern mtfModule mtfEqualAny;
extern mtfModule mtfExists;
extern mtfModule mtfExp;
extern mtfModule mtfExtract;
extern mtfModule mtfFloor;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfGreaterEqualAll;
extern mtfModule mtfGreaterEqualAny;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterThanAll;
extern mtfModule mtfGreaterThanAny;
extern mtfModule mtfGreatest;
extern mtfModule mtfHex_To_Num;
extern mtfModule mtfInitcap;
extern mtfModule mtfInstrb;
extern mtfModule mtfIsNotNull;
extern mtfModule mtfIsNull;
extern mtfModule mtfLast_day;
extern mtfModule mtfLeast;
extern mtfModule mtfLessEqual;
extern mtfModule mtfLessEqualAll;
extern mtfModule mtfLessEqualAny;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessThanAll;
extern mtfModule mtfLessThanAny;
extern mtfModule mtfLike;
extern mtfModule mtfList;
extern mtfModule mtfLn;
extern mtfModule mtfLog;
extern mtfModule mtfLower;
extern mtfModule mtfLpad;
extern mtfModule mtfLtrim;
extern mtfModule mtfMax;
extern mtfModule mtfMin;
extern mtfModule mtfMinus;
extern mtfModule mtfMod;
extern mtfModule mtfMonths_between;
extern mtfModule mtfMultiply;
//extern mtfModule mtfMultiply2;
extern mtfModule mtfNext_day;
extern mtfModule mtfNextHangul;
extern mtfModule mtfNot;
extern mtfModule mtfNotBetween;
extern mtfModule mtfNotEqual;
extern mtfModule mtfNotEqualAll;
extern mtfModule mtfNotEqualAny;
extern mtfModule mtfNotExists;
extern mtfModule mtfNotLike;
extern mtfModule mtfNotUnique;
extern mtfModule mtfNvl;
extern mtfModule mtfNvl2;
extern mtfModule mtfNvlEqual;
extern mtfModule mtfNvlNotEqual;
extern mtfModule mtfLnnvl;
extern mtfModule mtfOctet_length;
extern mtfModule mtfOct_To_Num;
extern mtfModule mtfOr;
extern mtfModule mtfPosition;
extern mtfModule mtfPower;
extern mtfModule mtfRand;
extern mtfModule mtfReplace2;
extern mtfModule mtfReplicate;
extern mtfModule mtfReverse_str;
extern mtfModule mtfRound;
extern mtfModule mtfRpad;
extern mtfModule mtfRtrim;
extern mtfModule mtfSign;
extern mtfModule mtfSin;
extern mtfModule mtfSinh;
extern mtfModule mtfSizeof;
extern mtfModule mtfSnmp_get;
extern mtfModule mtfSnmp_getnext;
extern mtfModule mtfSnmp_name;
extern mtfModule mtfSnmp_set;
extern mtfModule mtfSnmp_translate;
extern mtfModule mtfSnmp_walk;
extern mtfModule mtfSqrt;
extern mtfModule mtfStddev;
extern mtfModule mtfStddevGroupBy;
extern mtfModule mtfStuff;
extern mtfModule mtfSubstrb;
extern mtfModule mtfSubstring;
//extern mtfModule mtfSubtract;
extern mtfModule mtfSubtract2;
extern mtfModule mtfSum;
extern mtfModule mtfTan;
extern mtfModule mtfTanh;
extern mtfModule mtfTo_bin;
extern mtfModule mtfTo_char;
extern mtfModule mtfTo_date;
extern mtfModule mtfTo_number;
extern mtfModule mtfTo_hex;
extern mtfModule mtfTo_oct;
extern mtfModule mtfTranslate;
extern mtfModule mtfTrim;
extern mtfModule mtfTrunc;
extern mtfModule mtfUnique;
extern mtfModule mtfUpper;
extern mtfModule mtfVariance;
extern mtfModule mtfVarianceGroupBy;
extern mtfModule mtfBitAnd;
extern mtfModule mtfBitOr;
extern mtfModule mtfBitNot;
extern mtfModule mtfBitXor;
extern mtfModule mtfCast;
extern mtfModule mtfTo_fpid;
extern mtfModule mtfTo_fid;

// PROJ-1579 NCHAR
extern mtfModule mtfConvert;
extern mtfModule mtfNchr;
extern mtfModule mtfUnistr;
extern mtfModule mtfAsciistr;
extern mtfModule mtfTo_nchar;

/* BUG-32622 inlist operator */
extern mtfModule mtfInlist;
extern mtfModule mtfNotInlist;

// PROJ-2188 Pivot
extern mtfModule mtfDecodeSum;
extern mtfModule mtfDecodeAvg;
extern mtfModule mtfDecodeVariance;
extern mtfModule mtfDecodeStddev;
extern mtfModule mtfDecodeCount;
extern mtfModule mtfDecodeMax;
extern mtfModule mtfDecodeMin;

// BUG-33663
extern mtfModule mtfRowNumber;
/* BUG-40354 */
extern mtfModule mtfRowNumberLimit;
extern mtfModule mtfDenseRank;
extern mtfModule mtfRank;

// PROJ-1861
extern mtfModule mtfRegExpLike;
extern mtfModule mtfNotRegExpLike;

extern mtfModule mtfRegExpInstr;

extern mtfModule mtfRegExpCount;
extern mtfModule mtfRegExpSubstr;

// BUG-44796
extern mtfModule mtfRegExpReplace;

/* PROJ-2209 DBTIMEZONE */
extern mtfModule mtfConvertTimezone;

// BUG-34221
extern mtfModule mtfBase64_encode_str;
extern mtfModule mtfBase64_decode_str;

// BUG-34222
extern mtfModule mtfSys_guid_str;

/* BUG-39237 */
extern mtfModule mtfSysGuid;

// PROJ-2047 Strengthening LOB
extern mtfModule mtfEmptyBlob;
extern mtfModule mtfEmptyClob;

/* PROJ-1804 */
extern mtfModule mtfFirstValue;
extern mtfModule mtfFirstValueIgnoreNulls;
extern mtfModule mtfLastValue;
extern mtfModule mtfLastValueIgnoreNulls;
extern mtfModule mtfNthValue;
extern mtfModule mtfNthValueIgnoreNulls;
extern mtfModule mtfLag;
extern mtfModule mtfLagIgnoreNulls;
extern mtfModule mtfLead;
extern mtfModule mtfLeadIgnoreNulls;

/* BUG-36576 */
extern mtfModule mtfGroupConcat;

/* BUG-36581 */
extern mtfModule mtfNullif;

/* BUG-36582 */
extern mtfModule mtfIsNumeric;

/* BUG-36736 */
extern mtfModule mtfCoalesce;

/* BUG-36767 */
extern mtfModule mtfTDESDecrypt;
extern mtfModule mtfTDESEncrypt;
extern mtfModule mtfHex_decode;
extern mtfModule mtfHex_encode;

/* PROJ-2394 */
extern mtfModule mtfNth_element;
extern mtfModule mtfDecodeSumList;
extern mtfModule mtfDecodeAvgList;
extern mtfModule mtfDecodeCountList;
extern mtfModule mtfDecodeMinList;
extern mtfModule mtfDecodeMaxList;
extern mtfModule mtfDecodeVarianceList;
extern mtfModule mtfDecodeStddevList;

/* BUG-25198 */
extern mtfModule mtfUnixToDate;
extern mtfModule mtfDateToUnix;

/* BUG-39303 */
extern mtfModule mtfMultihash256;
extern mtfModule mtfGrouphash256;

/* BUG-39461 support RANDOM_STRING function */
extern mtfModule mtfRandomString;

/* BUG-40417 */
extern mtfModule mtfVarPop;
extern mtfModule mtfVarSamp;
extern mtfModule mtfStddevPop;
extern mtfModule mtfStddevSamp;

/* PROJ-2446 ONE SOURCE */
/* for XDB replication */
extern mtfModule mtfXSNToLSN;

extern mtfModule mtfTo_raw;         /* BUG-40973 */
extern mtfModule mtfRaw_to_integer; /* BUG-40973 */
extern mtfModule mtfRaw_to_numeric; /* BUG-40973 */
extern mtfModule mtfRawConcat;      /* BUG-40973 */
extern mtfModule mtfSubraw;         /* BUG-40973 */
extern mtfModule mtfRaw_sizeof;     /* BUG-40973 */

// PROJ-2527 WITHIN GROUP AGGR
extern mtfModule mtfListagg;
extern mtfModule mtfPercentileCont;
extern mtfModule mtfPercentileDisc;

/* BUG-43770 Median Function */
extern mtfModule mtfMedian;

/* PROJ-2529 STATS_ONE_WAY_ANOVA */
extern mtfModule mtfSOWAnova;

/* BUG-41309 to_interval */
extern mtfModule mtfToInterval;

// BUG-41346 HOST_NAME
extern mtfModule mtfHostName;

extern mtfModule mtfRaw_to_varchar;

// BUG-41555 DBMS PIPE
extern mtfModule mtfMsgCreateQueue;
extern mtfModule mtfMsgDropQueue;
extern mtfModule mtfMsgSndQueue;
extern mtfModule mtfMsgRcvQueue;

// BUG-41631 RANK WITHIN GROUP
extern mtfModule mtfRankWithinGroup;

/* PROJ-2523 Unpivot clause */
extern mtfModule mtfDecodeOffset;

//BUG-41771 PERCENT_RANK WITHIN GROUP
extern mtfModule mtfPercentRankWithinGroup;

// BUG-41800 CUME_DIST WITHIN GROUP
extern mtfModule mtfCumeDistWithinGroup;

extern mtfModule mtfHash;

// BUG-42675 QOUTED_PRINTABLE_ENCODE / DECODE
extern mtfModule mtfQuoted_printable_encode;
extern mtfModule mtfQuoted_printable_decode;

// BUG-42705 BASE64_ENCODE / DECODE
extern mtfModule mtfBase64_encode;
extern mtfModule mtfBase64_decode;

/* BUG-43086 */
extern mtfModule mtfNtile;

/* BUG-43087 */
extern mtfModule mtfRatioToReport;

/* BUG-43163 */
extern mtfModule mtfCovarSamp;
extern mtfModule mtfCovarPop;
extern mtfModule mtfCorr;

/* BUG-44256 NUMSHIFT, NUMAND 함수 추가 해야 합니다. */
extern mtfModule mtfNumShift;
extern mtfModule mtfNumAnd;

/* BUG-44260 NUMOR, NUMXOR, NUMNOT 함수 추가 해야 합니다. */
extern mtfModule mtfNumOr;
extern mtfModule mtfNumXor;

// PROJ-2528 Keep Aggregation
extern mtfModule mtfMinKeep;
extern mtfModule mtfMaxKeep;
extern mtfModule mtfSumKeep;
extern mtfModule mtfAvgKeep;
extern mtfModule mtfCountKeep;
extern mtfModule mtfVarianceKeep;
extern mtfModule mtfStddevKeep;

const mtfModule* mtf::mInternalModule[] = {
    &mtfAbs,
    &mtfAcos,
//    &mtfAdd,
    &mtfAdd2,
    &mtfAdd_months,
    &mtfAnd,
    &mtfAscii,
    &mtfAsin,
    &mtfAtan,
    &mtfAtan2,
    &mtfAvg,
//    &mtfAvgGroupBy,
    &mtfBetween,
    &mtfBin_To_Num,
    &mtfBinary_length,
    &mtfCase2,
    &mtfCeil,
    &mtfCharacter_length,
    &mtfChosung,
    &mtfChr,
    &mtfConcat,
    &mtfCos,
    &mtfCosh,
    &mtfCount,
    &mtfDateadd,
    &mtfDatediff,
    &mtfDatename,
    &mtfDecode,
    &mtfDESDecrypt,
    &mtfDESEncrypt,
    &mtfAESDecrypt,
    &mtfAESEncrypt,
    &mtfPKCS7PAD,
    &mtfPKCS7UNPAD,
    &mtfTDESDecrypt,
    &mtfTDESEncrypt,
    &mtfHex_decode,
    &mtfHex_encode,
    &mtfDigest,
    &mtfDigits,
    &mtfDivide,
//    &mtfDivide2,
    &mtfDump,
    &mtfLdump,
    &mtfEqual,
    &mtfEqualAll,
    &mtfEqualAny,
    &mtfExists,
    &mtfExp,
    &mtfExtract,
    &mtfFloor,
    &mtfGreaterEqual,
    &mtfGreaterEqualAll,
    &mtfGreaterEqualAny,
    &mtfGreaterThan,
    &mtfGreaterThanAll,
    &mtfGreaterThanAny,
    &mtfGreatest,
    &mtfHex_To_Num,
    &mtfInitcap,
    &mtfInstrb,
    &mtfIsNotNull,
    &mtfIsNull,
    &mtfLast_day,
    &mtfLeast,
    &mtfLessEqual,
    &mtfLessEqualAll,
    &mtfLessEqualAny,
    &mtfLessThan,
    &mtfLessThanAll,
    &mtfLessThanAny,
    &mtfLike,
    &mtfList,
    &mtfLn,
    &mtfLog,
    &mtfLower,
    &mtfLpad,
    &mtfLtrim,
    &mtfMax,
    &mtfMin,
    &mtfMinus,
    &mtfMod,
    &mtfMonths_between,
    &mtfMultiply,
//    &mtfMultiply2,
    &mtfNext_day,
    &mtfNextHangul,
    &mtfNot,
    &mtfNotBetween,
    &mtfNotEqual,
    &mtfNotEqualAll,
    &mtfNotEqualAny,
    &mtfNotExists,
    &mtfNotLike,
    &mtfNotUnique,
    &mtfNvl,
    &mtfNvl2,
    &mtfNvlEqual,
    &mtfNvlNotEqual,
    &mtfLnnvl,
    &mtfOctet_length,
    &mtfOct_To_Num,
    &mtfOr,
    &mtfPosition,
    &mtfPower,
    &mtfRand,
    &mtfReplace2,
    &mtfReplicate,
    &mtfReverse_str,
    &mtfRound,
    &mtfRpad,
    &mtfRtrim,
    &mtfSign,
    &mtfSin,
    &mtfSinh,
    &mtfSizeof,
    &mtfSnmp_get,
    &mtfSnmp_getnext,
    &mtfSnmp_name,
    &mtfSnmp_set,
    &mtfSnmp_translate,
    &mtfSnmp_walk,
    &mtfSqrt,
    &mtfStddev,
    &mtfStddevGroupBy,
    &mtfStuff,
    &mtfSubstrb,
    &mtfSubstring,
//    &mtfSubtract,
    &mtfSubtract2,
    &mtfSum,
    &mtfTan,
    &mtfTanh,
    &mtfTo_bin,
    &mtfTo_char,
    &mtfTo_date,
    &mtfTo_number,
    &mtfTo_hex,
    &mtfTo_oct,
    &mtfTranslate,
    &mtfTrim,
    &mtfTrunc,
    &mtfUnique,
    &mtfUpper,
    &mtfVariance,
    &mtfVarianceGroupBy,
    &mtfBitAnd,
    &mtfBitOr,
    &mtfBitNot,
    &mtfBitXor,
    &mtfCast,
    // PROJ-1579 NCHAR
    &mtfConvert,
    &mtfNchr,
    &mtfUnistr,
    &mtfAsciistr,
    &mtfTo_nchar,
    &mtfTo_fpid,
    &mtfTo_fid,
    /* BUG-32622 inlist operator */
    &mtfInlist,
    &mtfNotInlist,
    // PROJ-2188 Pivot
    &mtfDecodeSum,
    &mtfDecodeAvg,
    &mtfDecodeVariance,
    &mtfDecodeStddev,
    &mtfDecodeCount,
    &mtfDecodeMax,
    &mtfDecodeMin,
    // BUG-33663
    &mtfRowNumber,
    /* BUG-40354 */
    &mtfRowNumberLimit,
    &mtfDenseRank,
    &mtfRank,
    // PROJ-1861 regexp_like
    &mtfRegExpLike,
    &mtfNotRegExpLike,
    &mtfRegExpInstr,
    &mtfRegExpCount,
    &mtfRegExpSubstr,
    // BUG-44796
    &mtfRegExpReplace,
    /* PROJ-2209 DBTIMEZONE */
    &mtfConvertTimezone,
    // BUG-34221
    &mtfBase64_encode_str,
    &mtfBase64_decode_str,
    // BUG-34222
    &mtfSys_guid_str,    
    &mtfSysGuid,
    // PROJ-2047 Strengthening LOB
    &mtfEmptyBlob,
    &mtfEmptyClob,
    &mtfFirstValue,
    &mtfFirstValueIgnoreNulls,
    &mtfLastValue,
    &mtfLastValueIgnoreNulls,
    &mtfNthValue,
    &mtfNthValueIgnoreNulls,
    &mtfLag,
    &mtfLagIgnoreNulls,
    &mtfLead,
    &mtfLeadIgnoreNulls,
    &mtfGroupConcat,
    &mtfNullif,
    &mtfIsNumeric,
    &mtfCoalesce, /* BUG-36736 */
    // PROJ-2394 Bulk Pivot Aggregation
    &mtfNth_element,
    &mtfDecodeSumList,
    &mtfDecodeAvgList,
    &mtfDecodeCountList,
    &mtfDecodeMinList,
    &mtfDecodeMaxList,
    &mtfDecodeVarianceList,
    &mtfDecodeStddevList,
    /* BUG-25198 */
    &mtfUnixToDate,
    &mtfDateToUnix,
    /* BUG-39303 */
    &mtfMultihash256,
    &mtfRandomString,
    &mtfGrouphash256,
    &mtfVarPop,     /* BUG-40417 VarPop,VarSamp,StddevPop,StddevSamp */
    &mtfVarSamp,
    &mtfStddevPop,
    &mtfStddevSamp,
    &mtfXSNToLSN, /* PROJ-2446 ONE SOURCE */
    &mtfTo_raw,          /* BUG-40973 */
    &mtfRaw_to_integer,  /* BUG-40973 */
    &mtfRaw_to_numeric,  /* BUG-40973 */
    &mtfRawConcat,       /* BUG-40973 */
    &mtfSubraw,          /* BUG-40973 */
    &mtfRaw_sizeof,      /* BUG-40973 */
    &mtfListagg,         // PROJ-2527 WITHIN GROUP AGGR
    &mtfPercentileCont,
    &mtfPercentileDisc,
    &mtfMedian,         /* BUG-43770 Median Function */
    &mtfSOWAnova, /* PROJ-2529 STATS_ONE_WAY_ANOVA */
    &mtfToInterval,  	// BUG-41309 to_interval
    &mtfHostName,       // BUG-41346 HOSTNAME
    &mtfRaw_to_varchar,
// BUG-41555 DBMS PIPE
    &mtfMsgCreateQueue,
    &mtfMsgDropQueue,
    &mtfMsgSndQueue,
    &mtfMsgRcvQueue,
    &mtfRankWithinGroup, // BUG-41631 RANK WITHIN GROUP
    &mtfDecodeOffset, /* PROJ-2523 Unpivot clause */
    &mtfPercentRankWithinGroup, // BUG-41771 PERCENT_RANK WITHIN GROUP
    &mtfCumeDistWithinGroup,    // BUG-41800 CUME_DIST WITHIN GROUP
    &mtfHash,
    &mtfQuoted_printable_encode, // BUG-42675 QOUTED_PRINTABLE_ENCODE / DECODE
    &mtfQuoted_printable_decode,
    &mtfBase64_encode,           // BUG-42705 BASE64_ENCODE / DECODE
    &mtfBase64_decode,
    &mtfNtile,
    &mtfRatioToReport,
    &mtfCovarSamp,
    &mtfCovarPop,
    &mtfCorr,
    &mtfNumShift, /* BUG-44256 NUMSHIFT, NUMAND 함수 추가 해야 합니다. */
    &mtfNumAnd,   /* BUG-44256 NUMSHIFT, NUMAND 함수 추가 해야 합니다. */
    &mtfNumOr,  /* BUG-44260 NUMOR, NUMXOR, NUMNOT 함수 추가 해야 합니다. */
    &mtfNumXor, /* BUG-44260 NUMOR, NUMXOR, NUMNOT 함수 추가 해야 합니다. */
    &mtfMinKeep,
    &mtfMaxKeep,
    &mtfSumKeep,
    &mtfAvgKeep,
    &mtfCountKeep,
    &mtfVarianceKeep,
    &mtfStddevKeep,
    NULL
};

