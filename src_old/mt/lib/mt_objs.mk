# $Id: mt_objs.mk 84991 2019-03-11 09:21:00Z andrew.shin $

MTC_SRCS = $(MT_DIR)/mtc/mtc.cpp

MTD_SRCS = $(MT_DIR)/mtd/mtd.cpp                \
           $(MT_DIR)/mtd/mtdBigint.cpp          \
           $(MT_DIR)/mtd/mtdBit.cpp             \
           $(MT_DIR)/mtd/mtdVarbit.cpp          \
           $(MT_DIR)/mtd/mtdBinary.cpp          \
           $(MT_DIR)/mtd/mtdBlob.cpp            \
           $(MT_DIR)/mtd/mtdBoolean.cpp         \
           $(MT_DIR)/mtd/mtdChar.cpp            \
           $(MT_DIR)/mtd/mtddl.cpp              \
           $(MT_DIR)/mtd/mtdDate.cpp            \
           $(MT_DIR)/mtd/mtdDouble.cpp          \
           $(MT_DIR)/mtd/mtdFloat.cpp           \
           $(MT_DIR)/mtd/mtdInteger.cpp         \
           $(MT_DIR)/mtd/mtdInterval.cpp        \
           $(MT_DIR)/mtd/mtdList.cpp            \
           $(MT_DIR)/mtd/mtdNull.cpp            \
           $(MT_DIR)/mtd/mtdNumber.cpp          \
           $(MT_DIR)/mtd/mtdNumeric.cpp         \
           $(MT_DIR)/mtd/mtdReal.cpp            \
           $(MT_DIR)/mtd/mtdSmallint.cpp        \
           $(MT_DIR)/mtd/mtdVarchar.cpp         \
           $(MT_DIR)/mtd/mtdByte.cpp            \
           $(MT_DIR)/mtd/mtdNibble.cpp          \
           $(MT_DIR)/mtd/mtdClob.cpp            \
           $(MT_DIR)/mtd/mtdBlobLocator.cpp     \
           $(MT_DIR)/mtd/mtdClobLocator.cpp     \
           $(MT_DIR)/mtd/mtdNchar.cpp           \
           $(MT_DIR)/mtd/mtdNvarchar.cpp        \
           $(MT_DIR)/mtd/mtdEchar.cpp           \
           $(MT_DIR)/mtd/mtdEvarchar.cpp        \
           $(MT_DIR)/mtd/mtdUndef.cpp           \
           $(MT_DIR)/mtd/mtdVarbyte.cpp

MTF_SRCS = $(MT_DIR)/mtf/mtf.cpp                 \
           $(MT_DIR)/mtf/mtfModules.cpp          \
           $(MT_DIR)/mtf/mtfAbs.cpp              \
           $(MT_DIR)/mtf/mtfAcos.cpp             \
           $(MT_DIR)/mtf/mtfAdd2.cpp             \
           $(MT_DIR)/mtf/mtfAdd_months.cpp       \
           $(MT_DIR)/mtf/mtfAnd.cpp              \
           $(MT_DIR)/mtf/mtfAscii.cpp            \
           $(MT_DIR)/mtf/mtfAsin.cpp             \
           $(MT_DIR)/mtf/mtfAtan.cpp             \
           $(MT_DIR)/mtf/mtfAtan2.cpp            \
           $(MT_DIR)/mtf/mtfAvg.cpp              \
           $(MT_DIR)/mtf/mtfBetween.cpp          \
           $(MT_DIR)/mtf/mtfBin_To_Num.cpp       \
           $(MT_DIR)/mtf/mtfBinary_length.cpp    \
           $(MT_DIR)/mtf/mtfCase2.cpp            \
           $(MT_DIR)/mtf/mtfCeil.cpp             \
           $(MT_DIR)/mtf/mtfCharacter_length.cpp \
           $(MT_DIR)/mtf/mtfChosung.cpp          \
           $(MT_DIR)/mtf/mtfChr.cpp              \
           $(MT_DIR)/mtf/mtfConcat.cpp           \
           $(MT_DIR)/mtf/mtfCos.cpp              \
           $(MT_DIR)/mtf/mtfCosh.cpp             \
           $(MT_DIR)/mtf/mtfCount.cpp            \
           $(MT_DIR)/mtf/mtfDateadd.cpp          \
           $(MT_DIR)/mtf/mtfDatediff.cpp         \
           $(MT_DIR)/mtf/mtfDatename.cpp         \
           $(MT_DIR)/mtf/mtfDecode.cpp           \
           $(MT_DIR)/mtf/mtfDESDecrypt.cpp       \
           $(MT_DIR)/mtf/mtfDESEncrypt.cpp       \
           $(MT_DIR)/mtf/mtfAESDecrypt.cpp       \
           $(MT_DIR)/mtf/mtfAESEncrypt.cpp       \
           $(MT_DIR)/mtf/mtfPKCS7PAD.cpp         \
           $(MT_DIR)/mtf/mtfPKCS7UnPAD.cpp       \
           $(MT_DIR)/mtf/mtfTDESDecrypt.cpp      \
           $(MT_DIR)/mtf/mtfTDESEncrypt.cpp      \
           $(MT_DIR)/mtf/mtfHex_decode.cpp       \
           $(MT_DIR)/mtf/mtfHex_encode.cpp       \
           $(MT_DIR)/mtf/mtfDigits.cpp           \
           $(MT_DIR)/mtf/mtfDigest.cpp           \
           $(MT_DIR)/mtf/mtfDivide.cpp           \
           $(MT_DIR)/mtf/mtfDump.cpp             \
           $(MT_DIR)/mtf/mtfLdump.cpp            \
           $(MT_DIR)/mtf/mtfEqual.cpp            \
           $(MT_DIR)/mtf/mtfEqualAll.cpp         \
           $(MT_DIR)/mtf/mtfEqualAny.cpp         \
           $(MT_DIR)/mtf/mtfExists.cpp           \
           $(MT_DIR)/mtf/mtfExp.cpp              \
           $(MT_DIR)/mtf/mtfExtract.cpp          \
           $(MT_DIR)/mtf/mtfFloor.cpp            \
           $(MT_DIR)/mtf/mtfGreaterEqual.cpp     \
           $(MT_DIR)/mtf/mtfGreaterEqualAll.cpp  \
           $(MT_DIR)/mtf/mtfGreaterEqualAny.cpp  \
           $(MT_DIR)/mtf/mtfGreaterThan.cpp      \
           $(MT_DIR)/mtf/mtfGreaterThanAll.cpp   \
           $(MT_DIR)/mtf/mtfGreaterThanAny.cpp   \
           $(MT_DIR)/mtf/mtfGreatest.cpp         \
           $(MT_DIR)/mtf/mtfHex_To_Num.cpp       \
           $(MT_DIR)/mtf/mtfInitcap.cpp          \
           $(MT_DIR)/mtf/mtfInstrb.cpp           \
           $(MT_DIR)/mtf/mtfIsNotNull.cpp        \
           $(MT_DIR)/mtf/mtfIsNull.cpp           \
           $(MT_DIR)/mtf/mtfLast_day.cpp         \
           $(MT_DIR)/mtf/mtfLeast.cpp            \
           $(MT_DIR)/mtf/mtfLessEqual.cpp        \
           $(MT_DIR)/mtf/mtfLessEqualAll.cpp     \
           $(MT_DIR)/mtf/mtfLessEqualAny.cpp     \
           $(MT_DIR)/mtf/mtfLessThan.cpp         \
           $(MT_DIR)/mtf/mtfLessThanAll.cpp      \
           $(MT_DIR)/mtf/mtfLessThanAny.cpp      \
           $(MT_DIR)/mtf/mtfLike.cpp             \
           $(MT_DIR)/mtf/mtfList.cpp             \
           $(MT_DIR)/mtf/mtfLn.cpp               \
           $(MT_DIR)/mtf/mtfLog.cpp              \
           $(MT_DIR)/mtf/mtfLower.cpp            \
           $(MT_DIR)/mtf/mtfLpad.cpp             \
           $(MT_DIR)/mtf/mtfLtrim.cpp            \
           $(MT_DIR)/mtf/mtfMax.cpp              \
           $(MT_DIR)/mtf/mtfMin.cpp              \
           $(MT_DIR)/mtf/mtfMinus.cpp            \
           $(MT_DIR)/mtf/mtfMod.cpp              \
           $(MT_DIR)/mtf/mtfMonths_between.cpp   \
           $(MT_DIR)/mtf/mtfMultiply.cpp         \
           $(MT_DIR)/mtf/mtfNext_day.cpp         \
           $(MT_DIR)/mtf/mtfNextHangul.cpp       \
           $(MT_DIR)/mtf/mtfNot.cpp              \
           $(MT_DIR)/mtf/mtfNotBetween.cpp       \
           $(MT_DIR)/mtf/mtfNotEqual.cpp         \
           $(MT_DIR)/mtf/mtfNotEqualAll.cpp      \
           $(MT_DIR)/mtf/mtfNotEqualAny.cpp      \
           $(MT_DIR)/mtf/mtfNotExists.cpp        \
           $(MT_DIR)/mtf/mtfNotLike.cpp          \
           $(MT_DIR)/mtf/mtfNotUnique.cpp        \
           $(MT_DIR)/mtf/mtfNvl.cpp              \
           $(MT_DIR)/mtf/mtfNvl2.cpp             \
           $(MT_DIR)/mtf/mtfNvlEqual.cpp         \
           $(MT_DIR)/mtf/mtfNvlNotEqual.cpp      \
           $(MT_DIR)/mtf/mtfLnnvl.cpp            \
           $(MT_DIR)/mtf/mtfOctet_length.cpp     \
           $(MT_DIR)/mtf/mtfOct_To_Num.cpp       \
           $(MT_DIR)/mtf/mtfOr.cpp               \
           $(MT_DIR)/mtf/mtfPosition.cpp         \
           $(MT_DIR)/mtf/mtfPower.cpp            \
           $(MT_DIR)/mtf/mtfRand.cpp             \
           $(MT_DIR)/mtf/mtfReplace2.cpp         \
           $(MT_DIR)/mtf/mtfReplicate.cpp        \
           $(MT_DIR)/mtf/mtfReverse_str.cpp      \
           $(MT_DIR)/mtf/mtfRound.cpp            \
           $(MT_DIR)/mtf/mtfRpad.cpp             \
           $(MT_DIR)/mtf/mtfRtrim.cpp            \
           $(MT_DIR)/mtf/mtfSign.cpp             \
           $(MT_DIR)/mtf/mtfSin.cpp              \
           $(MT_DIR)/mtf/mtfSinh.cpp             \
           $(MT_DIR)/mtf/mtfSizeof.cpp           \
           $(MT_DIR)/mtf/mtfSnmp_get.cpp         \
           $(MT_DIR)/mtf/mtfSnmp_getnext.cpp     \
           $(MT_DIR)/mtf/mtfSnmp_name.cpp        \
           $(MT_DIR)/mtf/mtfSnmp_set.cpp         \
           $(MT_DIR)/mtf/mtfSnmp_translate.cpp   \
           $(MT_DIR)/mtf/mtfSnmp_walk.cpp        \
           $(MT_DIR)/mtf/mtfSqrt.cpp             \
           $(MT_DIR)/mtf/mtfStddev.cpp           \
           $(MT_DIR)/mtf/mtfStddevGroupBy.cpp    \
           $(MT_DIR)/mtf/mtfStuff.cpp            \
           $(MT_DIR)/mtf/mtfSubstrb.cpp          \
           $(MT_DIR)/mtf/mtfSubstring.cpp        \
           $(MT_DIR)/mtf/mtfSubtract2.cpp        \
           $(MT_DIR)/mtf/mtfSum.cpp              \
           $(MT_DIR)/mtf/mtfTan.cpp              \
           $(MT_DIR)/mtf/mtfTanh.cpp             \
           $(MT_DIR)/mtf/mtfTo_bin.cpp           \
           $(MT_DIR)/mtf/mtfTo_char.cpp          \
           $(MT_DIR)/mtf/mtfTo_date.cpp          \
           $(MT_DIR)/mtf/mtfTo_number.cpp        \
           $(MT_DIR)/mtf/mtfTo_hex.cpp           \
           $(MT_DIR)/mtf/mtfTo_oct.cpp           \
           $(MT_DIR)/mtf/mtfTranslate.cpp        \
           $(MT_DIR)/mtf/mtfTrim.cpp             \
           $(MT_DIR)/mtf/mtfTrunc.cpp            \
           $(MT_DIR)/mtf/mtfUnique.cpp           \
           $(MT_DIR)/mtf/mtfUpper.cpp            \
           $(MT_DIR)/mtf/mtfVariance.cpp         \
           $(MT_DIR)/mtf/mtfVarianceGroupBy.cpp  \
           $(MT_DIR)/mtf/mtfGetBlobLocator.cpp   \
           $(MT_DIR)/mtf/mtfGetClobLocator.cpp   \
           $(MT_DIR)/mtf/mtfGetBlobValue.cpp     \
           $(MT_DIR)/mtf/mtfGetClobValue.cpp     \
           $(MT_DIR)/mtf/mtfBitAnd.cpp           \
           $(MT_DIR)/mtf/mtfBitOr.cpp            \
           $(MT_DIR)/mtf/mtfBitNot.cpp           \
           $(MT_DIR)/mtf/mtfBitXor.cpp           \
           $(MT_DIR)/mtf/mtfCast.cpp             \
           $(MT_DIR)/mtf/mtfConvert.cpp          \
           $(MT_DIR)/mtf/mtfNchr.cpp             \
           $(MT_DIR)/mtf/mtfUnistr.cpp           \
           $(MT_DIR)/mtf/mtfAsciistr.cpp         \
           $(MT_DIR)/mtf/mtfTo_nchar.cpp         \
           $(MT_DIR)/mtf/mtfTo_fpid.cpp          \
           $(MT_DIR)/mtf/mtfTo_fid.cpp           \
           $(MT_DIR)/mtf/mtfDecrypt.cpp          \
           $(MT_DIR)/mtf/mtfInlist.cpp           \
           $(MT_DIR)/mtf/mtfNotInlist.cpp        \
           $(MT_DIR)/mtf/mtfDecodeSum.cpp        \
           $(MT_DIR)/mtf/mtfDecodeAvg.cpp        \
           $(MT_DIR)/mtf/mtfDecodeVariance.cpp   \
           $(MT_DIR)/mtf/mtfDecodeStddev.cpp     \
           $(MT_DIR)/mtf/mtfDecodeCount.cpp      \
           $(MT_DIR)/mtf/mtfDecodeMax.cpp        \
           $(MT_DIR)/mtf/mtfDecodeMin.cpp        \
           $(MT_DIR)/mtf/mtfRowNumber.cpp        \
           $(MT_DIR)/mtf/mtfRowNumberLimit.cpp   \
           $(MT_DIR)/mtf/mtfDenseRank.cpp        \
           $(MT_DIR)/mtf/mtfRank.cpp             \
           $(MT_DIR)/mtf/mtfRegExp.cpp           \
           $(MT_DIR)/mtf/mtfRegExpLike.cpp       \
           $(MT_DIR)/mtf/mtfNotRegExpLike.cpp    \
           $(MT_DIR)/mtf/mtfRegExpInstr.cpp      \
           $(MT_DIR)/mtf/mtfRegExpCount.cpp      \
           $(MT_DIR)/mtf/mtfRegExpSubstr.cpp     \
           $(MT_DIR)/mtf/mtfRegExpReplace.cpp     \
           $(MT_DIR)/mtf/mtfConv_tz.cpp          \
           $(MT_DIR)/mtf/mtfBase64_encode_str.cpp\
           $(MT_DIR)/mtf/mtfBase64_decode_str.cpp\
           $(MT_DIR)/mtf/mtfSys_guid_str.cpp     \
           $(MT_DIR)/mtf/mtfEmptyBlob.cpp        \
           $(MT_DIR)/mtf/mtfEmptyClob.cpp        \
           $(MT_DIR)/mtf/mtfFirstValue.cpp       \
           $(MT_DIR)/mtf/mtfFirstValueIgnoreNulls.cpp \
           $(MT_DIR)/mtf/mtfLastValue.cpp        \
           $(MT_DIR)/mtf/mtfLastValueIgnoreNulls.cpp \
           $(MT_DIR)/mtf/mtfNthValue.cpp         \
           $(MT_DIR)/mtf/mtfNthValueIgnoreNulls.cpp \
           $(MT_DIR)/mtf/mtfLag.cpp              \
           $(MT_DIR)/mtf/mtfLagIgnoreNulls.cpp   \
           $(MT_DIR)/mtf/mtfLead.cpp             \
           $(MT_DIR)/mtf/mtfLeadIgnoreNulls.cpp  \
           $(MT_DIR)/mtf/mtfNullif.cpp           \
           $(MT_DIR)/mtf/mtfIsNumeric.cpp        \
           $(MT_DIR)/mtf/mtfGroupConcat.cpp      \
           $(MT_DIR)/mtf/mtfCoalesce.cpp         \
           $(MT_DIR)/mtf/mtfNth_element.cpp      \
           $(MT_DIR)/mtf/mtfDecodeSumList.cpp    \
           $(MT_DIR)/mtf/mtfDecodeAvgList.cpp    \
           $(MT_DIR)/mtf/mtfDecodeCountList.cpp  \
           $(MT_DIR)/mtf/mtfDecodeMinList.cpp    \
           $(MT_DIR)/mtf/mtfDecodeMaxList.cpp    \
           $(MT_DIR)/mtf/mtfDecodeVarianceList.cpp \
           $(MT_DIR)/mtf/mtfDecodeStddevList.cpp \
           $(MT_DIR)/mtf/mtfSys_guid.cpp         \
           $(MT_DIR)/mtf/mtfUnixToDate.cpp       \
           $(MT_DIR)/mtf/mtfDateToUnix.cpp       \
           $(MT_DIR)/mtf/mtfMultihash256.cpp     \
           $(MT_DIR)/mtf/mtfRandom_string.cpp    \
           $(MT_DIR)/mtf/mtfGrouphash256.cpp     \
           $(MT_DIR)/mtf/mtfVarPop.cpp           \
           $(MT_DIR)/mtf/mtfVarSamp.cpp          \
           $(MT_DIR)/mtf/mtfStddevPop.cpp        \
           $(MT_DIR)/mtf/mtfStddevSamp.cpp	     \
           $(MT_DIR)/mtf/mtfXSNToLSN.cpp         \
           $(MT_DIR)/mtf/mtfTo_raw.cpp           \
           $(MT_DIR)/mtf/mtfRaw_to_integer.cpp   \
           $(MT_DIR)/mtf/mtfRaw_to_numeric.cpp   \
           $(MT_DIR)/mtf/mtfRawConcat.cpp        \
           $(MT_DIR)/mtf/mtfSubraw.cpp           \
           $(MT_DIR)/mtf/mtfRaw_sizeof.cpp       \
           $(MT_DIR)/mtf/mtfListagg.cpp          \
           $(MT_DIR)/mtf/mtfPercentileCont.cpp   \
           $(MT_DIR)/mtf/mtfPercentileDisc.cpp   \
           $(MT_DIR)/mtf/mtfMedian.cpp           \
           $(MT_DIR)/mtf/mtfSOWAnova.cpp         \
           $(MT_DIR)/mtf/mtfTo_interval.cpp      \
           $(MT_DIR)/mtf/mtfHostName.cpp         \
           $(MT_DIR)/mtf/mtfVc2coll.cpp          \
           $(MT_DIR)/mtf/mtfRaw_to_varchar.cpp   \
           $(MT_DIR)/mtf/mtfMsgCreateQueue.cpp   \
           $(MT_DIR)/mtf/mtfMsgDropQueue.cpp     \
           $(MT_DIR)/mtf/mtfMsgSndQueue.cpp      \
           $(MT_DIR)/mtf/mtfMsgRcvQueue.cpp      \
           $(MT_DIR)/mtf/mtfRankWithinGroup.cpp  \
           $(MT_DIR)/mtf/mtfDecodeOffset.cpp     \
           $(MT_DIR)/mtf/mtfPercentRankWithinGroup.cpp \
           $(MT_DIR)/mtf/mtfCumeDistWithinGroup.cpp \
           $(MT_DIR)/mtf/mtfHash.cpp \
           $(MT_DIR)/mtf/mtfQuoted_printable_encode.cpp \
           $(MT_DIR)/mtf/mtfQuoted_printable_decode.cpp \
           $(MT_DIR)/mtf/mtfBase64_encode.cpp \
           $(MT_DIR)/mtf/mtfBase64_decode.cpp \
           $(MT_DIR)/mtf/mtfNtile.cpp \
           $(MT_DIR)/mtf/mtfRatioToReport.cpp \
           $(MT_DIR)/mtf/mtfCovarSamp.cpp \
           $(MT_DIR)/mtf/mtfCovarPop.cpp \
           $(MT_DIR)/mtf/mtfCorr.cpp \
           $(MT_DIR)/mtf/mtfNumShift.cpp \
           $(MT_DIR)/mtf/mtfNumAnd.cpp \
           $(MT_DIR)/mtf/mtfNumOr.cpp \
           $(MT_DIR)/mtf/mtfNumXor.cpp \
           $(MT_DIR)/mtf/mtfMinKeep.cpp \
           $(MT_DIR)/mtf/mtfMaxKeep.cpp \
           $(MT_DIR)/mtf/mtfSumKeep.cpp \
           $(MT_DIR)/mtf/mtfAvgKeep.cpp \
           $(MT_DIR)/mtf/mtfCountKeep.cpp \
           $(MT_DIR)/mtf/mtfVarianceKeep.cpp \
           $(MT_DIR)/mtf/mtfStddevKeep.cpp

MTK_SRCS = $(MT_DIR)/mtk/mtk.cpp

MTL_SRCS = $(MT_DIR)/mtl/mtl.cpp                 \
           $(MT_DIR)/mtl/mtlUTF8.cpp             \
           $(MT_DIR)/mtl/mtlUTF16.cpp            \
           $(MT_DIR)/mtl/mtlASCII.cpp            \
           $(MT_DIR)/mtl/mtlKSC5601.cpp          \
           $(MT_DIR)/mtl/mtlShiftJIS.cpp         \
           $(MT_DIR)/mtl/mtlEUCJP.cpp            \
           $(MT_DIR)/mtl/mtlGB231280.cpp         \
           $(MT_DIR)/mtl/mtlBig5.cpp             \
           $(MT_DIR)/mtl/mtlMS949.cpp            \
           $(MT_DIR)/mtl/mtlMS936.cpp            \
           $(MT_DIR)/mtl/mtlMS932.cpp            \
           $(MT_DIR)/mtl/mtlCollate.cpp          \
           $(MT_DIR)/mtl/mtlTerritory.cpp

MTV_SRCS = $(MT_DIR)/mtv/mtv.cpp                               \
           $(MT_DIR)/mtv/mtvModules.cpp                        \
           $(MT_DIR)/mtv/mtvBigint2Double.cpp                  \
           $(MT_DIR)/mtv/mtvBigint2Real.cpp                    \
           $(MT_DIR)/mtv/mtvBigint2Integer.cpp                 \
           $(MT_DIR)/mtv/mtvBigint2Smallint.cpp                \
           $(MT_DIR)/mtv/mtvBigint2Numeric.cpp                 \
           $(MT_DIR)/mtv/mtvBigint2Float.cpp                   \
           $(MT_DIR)/mtv/mtvBigint2Varchar.cpp                 \
           $(MT_DIR)/mtv/mtvBigint2Char.cpp                    \
           $(MT_DIR)/mtv/mtvChar2Bigint.cpp                    \
           $(MT_DIR)/mtv/mtvChar2Double.cpp                    \
           $(MT_DIR)/mtv/mtvChar2Integer.cpp                   \
           $(MT_DIR)/mtv/mtvChar2Nvarchar.cpp                  \
           $(MT_DIR)/mtv/mtvChar2Numeric.cpp                   \
           $(MT_DIR)/mtv/mtvChar2Date.cpp                      \
           $(MT_DIR)/mtv/mtvChar2Float.cpp                     \
           $(MT_DIR)/mtv/mtvChar2Real.cpp                      \
           $(MT_DIR)/mtv/mtvChar2Smallint.cpp                  \
           $(MT_DIR)/mtv/mtvChar2Varchar.cpp                   \
           $(MT_DIR)/mtv/mtvDate2Char.cpp                      \
           $(MT_DIR)/mtv/mtvDate2Varchar.cpp                   \
           $(MT_DIR)/mtv/mtvDouble2Bigint.cpp                  \
           $(MT_DIR)/mtv/mtvDouble2Float.cpp                   \
           $(MT_DIR)/mtv/mtvDouble2Interval.cpp                \
           $(MT_DIR)/mtv/mtvDouble2Numeric.cpp                 \
           $(MT_DIR)/mtv/mtvDouble2Real.cpp                    \
           $(MT_DIR)/mtv/mtvDouble2Char.cpp                    \
           $(MT_DIR)/mtv/mtvDouble2Varchar.cpp                 \
           $(MT_DIR)/mtv/mtvFloat2Bigint.cpp                   \
           $(MT_DIR)/mtv/mtvFloat2Integer.cpp                  \
           $(MT_DIR)/mtv/mtvFloat2Smallint.cpp                 \
           $(MT_DIR)/mtv/mtvFloat2Double.cpp                   \
           $(MT_DIR)/mtv/mtvFloat2Numeric.cpp                  \
           $(MT_DIR)/mtv/mtvFloat2Char.cpp                     \
           $(MT_DIR)/mtv/mtvFloat2Varchar.cpp                  \
           $(MT_DIR)/mtv/mtvInteger2Real.cpp                   \
           $(MT_DIR)/mtv/mtvInteger2Float.cpp                  \
           $(MT_DIR)/mtv/mtvInteger2Numeric.cpp                \
           $(MT_DIR)/mtv/mtvInteger2Bigint.cpp                 \
           $(MT_DIR)/mtv/mtvInteger2Double.cpp                 \
           $(MT_DIR)/mtv/mtvInteger2Smallint.cpp               \
           $(MT_DIR)/mtv/mtvInteger2Char.cpp                   \
           $(MT_DIR)/mtv/mtvInteger2Varchar.cpp                \
           $(MT_DIR)/mtv/mtvInterval2Double.cpp                \
           $(MT_DIR)/mtv/mtvNull2Bigint.cpp                    \
           $(MT_DIR)/mtv/mtvNull2Blob.cpp                      \
           $(MT_DIR)/mtv/mtvNull2Boolean.cpp                   \
           $(MT_DIR)/mtv/mtvNull2Char.cpp                      \
           $(MT_DIR)/mtv/mtvNull2Clob.cpp                      \
           $(MT_DIR)/mtv/mtvNull2Date.cpp                      \
           $(MT_DIR)/mtv/mtvNull2Double.cpp                    \
           $(MT_DIR)/mtv/mtvNull2Float.cpp                     \
           $(MT_DIR)/mtv/mtvNull2Integer.cpp                   \
           $(MT_DIR)/mtv/mtvNull2Interval.cpp                  \
           $(MT_DIR)/mtv/mtvNull2Numeric.cpp                   \
           $(MT_DIR)/mtv/mtvNull2Real.cpp                      \
           $(MT_DIR)/mtv/mtvNull2Smallint.cpp                  \
           $(MT_DIR)/mtv/mtvNull2Varchar.cpp                   \
           $(MT_DIR)/mtv/mtvNull2Byte.cpp                      \
           $(MT_DIR)/mtv/mtvNull2Nibble.cpp                    \
           $(MT_DIR)/mtv/mtvNull2Bit.cpp                       \
           $(MT_DIR)/mtv/mtvNull2Varbit.cpp                    \
           $(MT_DIR)/mtv/mtvNumeric2Bigint.cpp                 \
           $(MT_DIR)/mtv/mtvNumeric2Integer.cpp                \
           $(MT_DIR)/mtv/mtvNumeric2Smallint.cpp               \
           $(MT_DIR)/mtv/mtvNumeric2Double.cpp                 \
           $(MT_DIR)/mtv/mtvNumeric2Float.cpp                  \
           $(MT_DIR)/mtv/mtvNumeric2Char.cpp                   \
           $(MT_DIR)/mtv/mtvNumeric2Varchar.cpp                \
           $(MT_DIR)/mtv/mtvReal2Double.cpp                    \
           $(MT_DIR)/mtv/mtvReal2Char.cpp                      \
           $(MT_DIR)/mtv/mtvReal2Varchar.cpp                   \
           $(MT_DIR)/mtv/mtvSmallint2Bigint.cpp                \
           $(MT_DIR)/mtv/mtvSmallint2Integer.cpp               \
           $(MT_DIR)/mtv/mtvSmallint2Numeric.cpp               \
           $(MT_DIR)/mtv/mtvSmallint2Float.cpp                 \
           $(MT_DIR)/mtv/mtvSmallint2Double.cpp                \
           $(MT_DIR)/mtv/mtvSmallint2Real.cpp                  \
           $(MT_DIR)/mtv/mtvSmallint2Char.cpp                  \
           $(MT_DIR)/mtv/mtvSmallint2Varchar.cpp               \
           $(MT_DIR)/mtv/mtvVarchar2Char.cpp                   \
           $(MT_DIR)/mtv/mtvVarchar2Date.cpp                   \
           $(MT_DIR)/mtv/mtvVarchar2Bigint.cpp                 \
           $(MT_DIR)/mtv/mtvVarchar2Double.cpp                 \
           $(MT_DIR)/mtv/mtvVarchar2Integer.cpp                \
           $(MT_DIR)/mtv/mtvVarchar2Numeric.cpp                \
           $(MT_DIR)/mtv/mtvVarchar2Real.cpp                   \
           $(MT_DIR)/mtv/mtvVarchar2Smallint.cpp               \
           $(MT_DIR)/mtv/mtvVarchar2Nchar.cpp                  \
           $(MT_DIR)/mtv/mtvVarchar2Float.cpp                  \
           $(MT_DIR)/mtv/mtvVarbit2Varchar.cpp                 \
           $(MT_DIR)/mtv/mtvBit2Varbit.cpp                     \
           $(MT_DIR)/mtv/mtvVarbit2Bit.cpp                     \
           $(MT_DIR)/mtv/mtvVarchar2Blob.cpp                   \
           $(MT_DIR)/mtv/mtvVarchar2Clob.cpp                   \
           $(MT_DIR)/mtv/mtvVarbyte2Blob.cpp                   \
           $(MT_DIR)/mtv/mtvBinary2Blob.cpp                    \
           $(MT_DIR)/mtv/mtvBlobLocator2Blob.cpp               \
           $(MT_DIR)/mtv/mtvClobLocator2Clob.cpp               \
           $(MT_DIR)/mtv/mtvBlob2BlobLocator.cpp               \
           $(MT_DIR)/mtv/mtvClob2ClobLocator.cpp               \
           $(MT_DIR)/mtv/mtvClob2Varchar.cpp                   \
           $(MT_DIR)/mtv/mtvNibble2Varchar.cpp                 \
           $(MT_DIR)/mtv/mtvVarchar2Nibble.cpp                 \
           $(MT_DIR)/mtv/mtvNchar2Char.cpp                     \
           $(MT_DIR)/mtv/mtvChar2Nchar.cpp                     \
           $(MT_DIR)/mtv/mtvNvarchar2Varchar.cpp               \
           $(MT_DIR)/mtv/mtvVarchar2Nvarchar.cpp               \
           $(MT_DIR)/mtv/mtvNchar2Nvarchar.cpp                 \
           $(MT_DIR)/mtv/mtvNvarchar2Nchar.cpp                 \
           $(MT_DIR)/mtv/mtvNull2Nchar.cpp                     \
           $(MT_DIR)/mtv/mtvNull2Nvarchar.cpp                  \
           $(MT_DIR)/mtv/mtvChar2Echar.cpp                     \
           $(MT_DIR)/mtv/mtvVarchar2Evarchar.cpp               \
           $(MT_DIR)/mtv/mtvEchar2Char.cpp                     \
           $(MT_DIR)/mtv/mtvEvarchar2Varchar.cpp               \
           $(MT_DIR)/mtv/mtvNull2Echar.cpp                     \
           $(MT_DIR)/mtv/mtvNull2Evarchar.cpp                  \
           $(MT_DIR)/mtv/mtvUndef2All.cpp                      \
           $(MT_DIR)/mtv/mtvByte2Varbyte.cpp                   \
           $(MT_DIR)/mtv/mtvVarbyte2Byte.cpp                   \
           $(MT_DIR)/mtv/mtvVarbyte2Varchar.cpp                \
           $(MT_DIR)/mtv/mtvVarchar2Varbyte.cpp                \
           $(MT_DIR)/mtv/mtvNull2Varbyte.cpp

MTZ_SRCS = $(MT_DIR)/mtz/mtz.cpp

MTU_SRCS = $(MT_DIR)/mtu/mtuProperty.cpp $(MT_DIR)/mtu/mtuTraceCode.cpp

MTS_SRCS = $(MT_DIR)/mts/mtsFileType.cpp                \
           $(MT_DIR)/mts/mtsConnectType.cpp

MTX_SRCS = $(MT_DIR)/mtx/mtx.cpp               \
           $(MT_DIR)/mtx/mtxRid.cpp            \
           $(MT_DIR)/mtx/mtxColumn.cpp         \
           $(MT_DIR)/mtx/mtxFromDateTo.cpp     \
           $(MT_DIR)/mtx/mtxFromSmallintTo.cpp \
           $(MT_DIR)/mtx/mtxFromIntegerTo.cpp  \
           $(MT_DIR)/mtx/mtxFromBigintTo.cpp   \
           $(MT_DIR)/mtx/mtxFromRealTo.cpp     \
           $(MT_DIR)/mtx/mtxFromDoubleTo.cpp   \
           $(MT_DIR)/mtx/mtxFromFloatTo.cpp    \
           $(MT_DIR)/mtx/mtxFromNumericTo.cpp  \
           $(MT_DIR)/mtx/mtxFromCharTo.cpp     \
           $(MT_DIR)/mtx/mtxFromVarcharTo.cpp  \
           $(MT_DIR)/mtx/mtxFromNcharTo.cpp    \
           $(MT_DIR)/mtx/mtxFromNvarcharTo.cpp \
           $(MT_DIR)/mtx/mtxFromIntervalTo.cpp \
           $(MT_DIR)/mtx/mtxFromNibbleTo.cpp   \
           $(MT_DIR)/mtx/mtxFromBitTo.cpp      \
           $(MT_DIR)/mtx/mtxFromVarbitTo.cpp   \
           $(MT_DIR)/mtx/mtxFromByteTo.cpp     \
           $(MT_DIR)/mtx/mtxFromVarbyteTo.cpp  \
           $(MT_DIR)/mtx/mtxAnd.cpp            \
           $(MT_DIR)/mtx/mtxOr.cpp             \
           $(MT_DIR)/mtx/mtxNot.cpp            \
           $(MT_DIR)/mtx/mtxEqual.cpp          \
           $(MT_DIR)/mtx/mtxNotEqual.cpp       \
           $(MT_DIR)/mtx/mtxGreaterThan.cpp    \
           $(MT_DIR)/mtx/mtxGreaterEqual.cpp   \
           $(MT_DIR)/mtx/mtxLessThan.cpp       \
           $(MT_DIR)/mtx/mtxLessEqual.cpp      \
           $(MT_DIR)/mtx/mtxAdd.cpp            \
           $(MT_DIR)/mtx/mtxSubtract.cpp       \
           $(MT_DIR)/mtx/mtxMultiply.cpp       \
           $(MT_DIR)/mtx/mtxDivide.cpp         \
           $(MT_DIR)/mtx/mtxIsNotNull.cpp      \
           $(MT_DIR)/mtx/mtxLnnvl.cpp

MT_SRCS = $(MTC_SRCS) $(MTD_SRCS) $(MTF_SRCS) $(MTK_SRCS) $(MTL_SRCS) $(MTV_SRCS) $(MTZ_SRCS) $(MTU_SRCS) $(MTS_SRCS) $(MTX_SRCS) 
MT_OBJS = $(MT_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
MT_SHOBJS = $(MT_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

MT_CLIENT_SRCS = $(MTC_SRCS) $(MTD_SRCS) $(MTS_SRCS)    \
                 $(MT_DIR)/mtf/mtf.cpp                  \
                 $(MT_DIR)/mtf/mtfModulesForClient.cpp  \
                 $(MTL_SRCS) $(MTK_SRCS)                \
                 $(MT_DIR)/mtv/mtv.cpp                  \
                 $(MT_DIR)/mtv/mtvModulesForClient.cpp  \
                 $(MT_DIR)/mtu/mtuProperty.cpp

MT_CLIENT_OBJS = $(MT_CLIENT_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
MT_CLIENT_SHOBJS = $(MT_CLIENT_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

 
MTCC_SRCS = $(MT_DIR)/mtcc/mtcc.c                 

MTCL_SRCS = $(MT_DIR)/mtcl/mtcl.c                \
            $(MT_DIR)/mtcl/mtclUTF8.c            \
            $(MT_DIR)/mtcl/mtclUTF16.c           \
            $(MT_DIR)/mtcl/mtclASCII.c           \
            $(MT_DIR)/mtcl/mtclKSC5601.c         \
            $(MT_DIR)/mtcl/mtclShiftJIS.c        \
            $(MT_DIR)/mtcl/mtclEUCJP.c           \
            $(MT_DIR)/mtcl/mtclGB231280.c        \
            $(MT_DIR)/mtcl/mtclBig5.c            \
            $(MT_DIR)/mtcl/mtclMS949.c           \
            $(MT_DIR)/mtcl/mtclMS936.c           \
            $(MT_DIR)/mtcl/mtclMS932.c           \
            $(MT_DIR)/mtcl/mtclCollate.c 

MTCD_SRCS = $(MT_DIR)/mtcd/mtcd.c                \
            $(MT_DIR)/mtcd/mtcdBigint.c          \
            $(MT_DIR)/mtcd/mtcdBit.c             \
            $(MT_DIR)/mtcd/mtcdChar.c            \
            $(MT_DIR)/mtcd/mtcdVarbit.c          \
            $(MT_DIR)/mtcd/mtcdVarchar.c         \
            $(MT_DIR)/mtcd/mtcdBinary.c          \
            $(MT_DIR)/mtcd/mtcdBoolean.c         \
            $(MT_DIR)/mtcd/mtcdBlob.c            \
            $(MT_DIR)/mtcd/mtcdBlobLocator.c     \
            $(MT_DIR)/mtcd/mtcddl.c              \
            $(MT_DIR)/mtcd/mtcdDouble.c          \
            $(MT_DIR)/mtcd/mtcdDate.c            \
            $(MT_DIR)/mtcd/mtcdFloat.c           \
            $(MT_DIR)/mtcd/mtcdInteger.c         \
            $(MT_DIR)/mtcd/mtcdInterval.c        \
            $(MT_DIR)/mtcd/mtcdList.c            \
            $(MT_DIR)/mtcd/mtcdNull.c            \
            $(MT_DIR)/mtcd/mtcdReal.c            \
            $(MT_DIR)/mtcd/mtcdSmallint.c        \
            $(MT_DIR)/mtcd/mtcdByte.c            \
            $(MT_DIR)/mtcd/mtcdNibble.c          \
            $(MT_DIR)/mtcd/mtcdNumber.c          \
            $(MT_DIR)/mtcd/mtcdNumeric.c         \
            $(MT_DIR)/mtcd/mtcdNchar.c 	         \
            $(MT_DIR)/mtcd/mtcdNvarchar.c        \
            $(MT_DIR)/mtcd/mtcdEchar.c           \
            $(MT_DIR)/mtcd/mtcdEvarchar.c        \
            $(MT_DIR)/mtcd/mtcdClob.c            \
            $(MT_DIR)/mtcd/mtcdClobLocator.c     \
            $(MT_DIR)/mtcd/mtcdVarbyte.c

MTCA_SRCS = $(MT_DIR)/mtca/mtcaConst.c           \
            $(MT_DIR)/mtca/mtcaCTypes.c          \
            $(MT_DIR)/mtca/mtcaTNumeric.c        \
            $(MT_DIR)/mtca/mtcaVarchar.c         \
            $(MT_DIR)/mtca/mtcaCompare.c         \
            $(MT_DIR)/mtca/mtcaOperator.c        \
            $(MT_DIR)/mtca/mtcaTNumericMisc.c

MTCN_SRCS = $(MT_DIR)/mtcn/mtcn.c

MTCF_SRCS = $(MT_DIR)/mtcf/mtcfTo_char.c

MT_C_CLIENT_SRCS = $(MTCC_SRCS) $(MTCD_SRCS) $(MTCL_SRCS) \
                   $(MTCA_SRCS) $(MTCN_SRCS) $(MTCF_SRCS)

MT_C_CLIENT_OBJS = $(MT_C_CLIENT_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))

