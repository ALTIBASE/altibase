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
 * $Id: mtv.h 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#ifndef _O_MTV_H_
# define _O_MTV_H_ 1

# include <mtx.h>
# include <mtcDef.h>
# include <mtdTypes.h>

class mtv {
private:
    static const mtvModule*  mInternalModule[];
    static       mtvModule** mAllModule;
//  static       mtvModule* mMtvTableCheck[31][31][4]; // PROJ-2183

    static mtvTable** table;

    static UInt setModules( UInt              aTo,
                            UInt              aFrom,
                            UInt**            aTarget,
                            const mtvModule** aModule );

    static IDE_RC estimateConvertInternal( mtfCalculateFunc* aConvert,
                                           mtcNode*          aNode,
                                           mtcStack*         aStack,
                                           const mtvModule** aModule,
                                           const mtvModule** aFence,
                                           mtcTemplate*      aTemplate );

public:
    // BUG-21627 최대 conversion path
    static UInt mMaxConvCount;
    
    static IDE_RC initialize( mtvModule *** aExtCvtModuleGroup,
                              UInt          aGroupCnt );

    static IDE_RC finalize( void );

    static IDE_RC tableByNo( const mtvTable** aTable,
                             UInt             aTo,
                             UInt             aFrom );

    /* Conversion Support */
    static IDE_RC estimateConvert4Server( iduMemory*   aMemory,
                                          mtvConvert** aConvert,
                                          mtcId        aDestinationId,
                                          mtcId        aSourceId,
                                          UInt         aSourceArgument,
                                          SInt         aSourcePrecision,
                                          SInt         aSourceScale,
                                          mtcTemplate* aTemplate );

    static IDE_RC estimateConvert4Server( iduVarMemList * aMemory,
                                          mtvConvert   ** aConvert,
                                          mtcId           aDestinationId,
                                          mtcId           aSourceId,
                                          UInt            aSourceArgument,
                                          SInt            aSourcePrecision,
                                          SInt            aSourceScale,
                                          mtcTemplate   * aTemplate );

    static IDE_RC estimateConvert4Cli( mtvConvert** aConvert,
                                       mtcId        aDestinationId,
                                       mtcId        aSourceId,
                                       UInt         aSourceArgument,
                                       SInt         aSourcePrecision,
                                       SInt         aSourceScale,
                                       mtcTemplate* aTemplate );

    static IDE_RC executeConvert( mtvConvert* aConvert, mtcTemplate *aTemplate );

    static IDE_RC freeConvert( mtvConvert* aConvert );

    static IDE_RC float2String( UChar*          aBuffer,
                                UInt            aBufferLength,
                                UInt*           aLength,
                                mtdNumericType* aNumeric );

    /* PROJ-2183
     *
     * + 그룹 구분
     *
     * 1. character         : char, varchar
     * 2. nativeN (native 정수)   : bigint, integer, smallint
     * 3. nativeR (native 실수형) : double, real
     * 4. numeric (nonNative) : numeric,  float
     * 5. etc (misc, interval, date, encrypt, st, national character ..., )
     *
     * + 형변환을 그룹간의 변환과 그룹내의 변환으로 구분함
     *
     * 1. 그룹변경
     *
     * - 새로 생성한 함수
     * character2NativeR;
     * character2NativeN;
     * nativeN2Character;
     * nativeR2Character;
     * numeric2NativeN;
     * nativeN2Numeric;
     *
     * - 기존에 있거나 이 프로젝트에서 시간이 부족해 만들지 않은 함수
     * character2Numeric ;            // makeNumeric
     * nativeR2Numeric ;              // makeNumeric
     * numeric2Character ;            // float2String
     * numeric2NativeR( aOutBigint ); // don't care
     * character2Numeric ;            // makeNumeric
     *
     * 2. 그룹 내 변환 : 주로 memcpy 또는 캐스팅으로 처리하므로 함수화하지 않음
     *
     * nativeN2nativeR, nativeR2nativeN, nativeR2nativeR, nonnative2Nonnative, character2Character
     *
     */

    static IDE_RC character2NativeR( mtdCharType* aInChar, mtdDoubleType* aOutDouble );
    static IDE_RC character2NativeN( mtdCharType* aInChar, mtdBigintType* aOutBigint );
    // static IDE_RC character2Numeric( mtcStack* aStack ); // makeNumeric함수로 처리함
    static IDE_RC nativeN2Character( mtdBigintType aInBigint, mtdCharType* aOutChar );
    static IDE_RC nativeR2Character( mtdDoubleType aInDouble, mtdCharType* aOutChar );
    // static IDE_RC nativeR2Numeric( mtcStack* aStack, mtdDoubleType aInDouble ); // makeNumeric함수로 처리함
    // static IDE_RC numeric2Character( mtcStack* aStack ); // float2String함수로 처리함
    static IDE_RC numeric2NativeN( mtdNumericType* aInNumeric, mtdBigintType* aOutBigint );
    static IDE_RC nativeN2Numeric( mtdBigintType aInBigint, mtdNumericType* aOutNumeric );
};

#endif /* _O_MTV_H_ */
