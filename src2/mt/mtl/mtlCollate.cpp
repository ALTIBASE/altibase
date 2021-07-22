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
 * $Id: 
 **********************************************************************/

#include <mte.h>
#include <mtd.h>
#include <mtlCollate.h>
#include <mtlMS949Collation.h>

extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern UShort mtlMS949CollationTable[MS949_NUM];

void mtlCollate::mtlSetCodeAndLenInfo( const UChar * aSource,
                                       UInt          aSourceIdx,
                                       UShort      * aCode,
                                       UShort      * aLength )
{
/***********************************************************************
 *
 * Description : 한문자에 대한 코드와 length 반환
 *
 *              ( US7ASCII, KSC5601, MS949 에 대해서 유효함 )
 *
 * Implementation :
 *
 *   예 : 가 
 *
 *   1. KSC5601  (1) code = 0xb0a1 (2) 길이 : 2
 *   2. MS949    (1) code = 0xb0a1 (2) 길이 : 2
 *
 ***********************************************************************/
    IDE_ASSERT( aSource != NULL );
    IDE_ASSERT( aCode != NULL );
    IDE_ASSERT( aLength != NULL );

    if( ((UChar)aSource[aSourceIdx] ) < 0x80 )
    {
        *aLength    = 1;
        *aCode = aSource[aSourceIdx];
    }
    else
    {
        *aLength    = 2;
        *aCode = ( aSource[aSourceIdx] << 8 ) + aSource[aSourceIdx+1];
    }
}

void mtlCollate::mtlSetCodeAndLenInfoWithCheck2ByteCharCut( const UChar * aSource,
                                                            UShort        aSourceLen,
                                                            UInt          aSourceIdx,
                                                            UShort      * aCode,
                                                            UShort      * aLength,
                                                            idBool      * aIs2ByteCharCut )
{
/***********************************************************************
 *
 * BUG-43106
 *
 * Description : 위의 mtlCollate::mtlSetCodeAndLenInfo() 함수에
 *               2byte문자 체크 코드를 추가함.
 *
 *             ( 입력문자열에 2bytes 문자가 포함됐을때
 *               앞 1byte만 문자열에 포함되고
 *               뒤 1byte는 문자열에 포함되지 않은경우
 *               *aIs2ByteCharCut = TRUE를 세팅한다. )
 *
 ***********************************************************************/
    IDE_ASSERT( aSource != NULL );
    IDE_ASSERT( aCode != NULL );
    IDE_ASSERT( aLength != NULL );
    IDE_ASSERT( aIs2ByteCharCut != NULL );

    *aIs2ByteCharCut = ID_FALSE;

    if ( ((UChar)aSource[aSourceIdx] ) < 0x80 )
    {
        *aLength = 1;
        *aCode   = aSource[aSourceIdx];
    }
    else
    {
        if ( ( aSourceIdx + 1 ) == aSourceLen )
        {
            *aLength         = 0;
            *aCode           = 0;
            *aIs2ByteCharCut = ID_TRUE;
        }
        else
        {
            *aLength = 2;
            *aCode   = ( aSource[aSourceIdx] << 8 ) + aSource[aSourceIdx + 1];
        }
    }
}

SInt mtlCollate::mtlCharMS949collateMtdMtdAsc( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------    
    sCharValue1 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                        aValueInfo1->value,
                                        aValueInfo1->flag,
                                        mtdChar.staticNull );
    sLength1 = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                        aValueInfo2->value,
                                        aValueInfo2->flag,
                                        mtdChar.staticNull );

    sLength2 = sCharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            //sCompared = idlOS::memcmp( sValue1->value, sValue2->value,
            //                           sValue2->length );

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while( sIterator2 < sLength2 )
            {
                sCompared = mtlCompareMS949Collate( sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1,
                                                    sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2 );

                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for( sIterator = sValue1 + sLength2,
                 sFence    = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        // sCompared = idlOS::memcmp( sValue1->value, sValue2->value,
        //                            sValue1->length );

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;

        while( sIterator1 < sLength1 )
        {
            sCompared = mtlCompareMS949Collate( sValue1,
                                                sIterator1,
                                                & sOneCharLength1,
                                                sValue2,
                                                sIterator2,
                                                & sOneCharLength2 );

            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for( sIterator = sValue2 + sLength1,
             sFence    = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlCharMS949collateMtdMtdDesc( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 ) 
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------    
    sCharValue1 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                        aValueInfo1->value,
                                        aValueInfo1->flag,
                                        mtdChar.staticNull );
    sLength1 = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                        aValueInfo2->value,
                                        aValueInfo2->flag,
                                        mtdChar.staticNull );

    sLength2 = sCharValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            // sCompared = idlOS::memcmp( sValue2->value, sValue1->value,
            //                            sValue1->length );

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while( sIterator1 < sLength1 )
            {
                sCompared = mtlCompareMS949Collate( sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2,
                                                    sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1 );

                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for( sIterator = sValue2 + sLength1,
                 sFence    = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        //sCompared = idlOS::memcmp( sValue2->value, sValue1->value,
        //                           sValue2->length );

        sIterator1 = 0;
        sIterator2 = 0;
        sOneCharLength1   = 0;
        sOneCharLength2   = 0;

        while( sIterator2 < sLength2 )
        {
            sCompared = mtlCompareMS949Collate( sValue2,
                                                sIterator2,
                                                & sOneCharLength2,
                                                sValue1,
                                                sIterator1,
                                                & sOneCharLength1 );

            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for( sIterator = sValue1 + sLength2,
             sFence    = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlCharMS949collateStoredMtdAsc( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;
    
    //---------
    // value1
    //---------

    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value2
    //---------    
    sCharValue2 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                        aValueInfo2->value,
                                        aValueInfo2->flag,
                                        mtdChar.staticNull );
    sLength2 = sCharValue2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength1 >= sLength2 )
        {
            //sCompared = idlOS::memcmp( sValue1->value, sValue2->value,
            //                           sValue2->length );

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while( sIterator2 < sLength2 )
            {
                sCompared = mtlCompareMS949Collate( sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1,
                                                    sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2 );

                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for( sIterator = sValue1 + sLength2,
                 sFence    = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        // sCompared = idlOS::memcmp( sValue1->value, sValue2->value,
        //                            sValue1->length );

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;

        while( sIterator1 < sLength1 )
        {
            sCompared = mtlCompareMS949Collate( sValue1,
                                                sIterator1,
                                                & sOneCharLength1,
                                                sValue2,
                                                sIterator2,
                                                & sOneCharLength2 );

            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for( sIterator = sValue2 + sLength1,
             sFence    = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlCharMS949collateStoredMtdDesc( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2) 
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------
    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value1
    //---------    
    sCharValue2 = (const mtdCharType*)
                   mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                        aValueInfo2->value,
                                        aValueInfo2->flag,
                                        mtdChar.staticNull );
    sLength2 = sCharValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = sCharValue2->value;
        
        if( sLength2 >= sLength1 )
        {
            // sCompared = idlOS::memcmp( sValue2->value, sValue1->value,
            //                            sValue1->length );

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while( sIterator1 < sLength1 )
            {
                sCompared = mtlCompareMS949Collate( sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2,
                                                    sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1 );

                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for( sIterator = sValue2 + sLength1,
                 sFence    = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        //sCompared = idlOS::memcmp( sValue2->value, sValue1->value,
        //                           sValue2->length );

        sIterator1 = 0;
        sIterator2 = 0;
        sOneCharLength1   = 0;
        sOneCharLength2   = 0;

        while( sIterator2 < sLength2 )
        {
            sCompared = mtlCompareMS949Collate( sValue2,
                                                sIterator2,
                                                & sOneCharLength2,
                                                sValue1,
                                                sIterator1,
                                                & sOneCharLength1 );

            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for( sIterator = sValue1 + sLength2,
             sFence    = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlCharMS949collateStoredStoredAsc( mtdValueInfo * aValueInfo1,
                                                     mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------
    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value2
    //---------
    sLength2 = (UShort)aValueInfo2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = (UChar*)aValueInfo2->value;
        
        if( sLength1 >= sLength2 )
        {
            //sCompared = idlOS::memcmp( sValue1->value, sValue2->value,
            //                           sValue2->length );

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while( sIterator2 < sLength2 )
            {
                sCompared = mtlCompareMS949Collate( sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1,
                                                    sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2 );

                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for( sIterator = sValue1 + sLength2,
                 sFence    = sValue1 + sLength1;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        // sCompared = idlOS::memcmp( sValue1->value, sValue2->value,
        //                            sValue1->length );

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;

        while( sIterator1 < sLength1 )
        {
            sCompared = mtlCompareMS949Collate( sValue1,
                                                sIterator1,
                                                & sOneCharLength1,
                                                sValue2,
                                                sIterator2,
                                                & sOneCharLength2 );

            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for( sIterator = sValue2 + sLength1,
             sFence    = sValue2 + sLength2;
             sIterator < sFence;
             sIterator++ )
        {
            if( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlCharMS949collateStoredStoredDesc( mtdValueInfo * aValueInfo1,
                                                      mtdValueInfo * aValueInfo2 ) 
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/
    
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------    
    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value2
    //---------
    sLength2 = (UShort)aValueInfo2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = (UChar*)aValueInfo2->value;
        
        if( sLength2 >= sLength1 )
        {
            // sCompared = idlOS::memcmp( sValue2->value, sValue1->value,
            //                            sValue1->length );

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while( sIterator1 < sLength1 )
            {
                sCompared = mtlCompareMS949Collate( sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2,
                                                    sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1 );

                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for( sIterator = sValue2 + sLength1,
                 sFence    = sValue2 + sLength2;
                 sIterator < sFence;
                 sIterator++ )
            {
                if( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        //sCompared = idlOS::memcmp( sValue2->value, sValue1->value,
        //                           sValue2->length );

        sIterator1 = 0;
        sIterator2 = 0;
        sOneCharLength1   = 0;
        sOneCharLength2   = 0;

        while( sIterator2 < sLength2 )
        {
            sCompared = mtlCompareMS949Collate( sValue2,
                                                sIterator2,
                                                & sOneCharLength2,
                                                sValue1,
                                                sIterator1,
                                                & sOneCharLength1 );

            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for( sIterator = sValue1 + sLength2,
             sFence    = sValue1 + sLength1;
             sIterator < sFence;
             sIterator++ )
        {
            if( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

/* BUG-43106 */
SInt mtlCollate::mtlCharMS949collateIndexKeyMtdAsc( mtdValueInfo * aValueInfo1,
                                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * BUG-43106 
 *
 * Description : mtlCollate::mtlCharMS949collateMtdMtdAsc() 함수를
 *               index key 비교용으로 변경함 함수.
 *
 *             ( PROJ-2433 Direct Key INDEX의 
 *               partial direct key 비교를 위한 코드가 추가됨 ) 
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDirectKeyPartialSize;
    idBool               sIs2ByteCharCut = ID_FALSE;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------    
    sCharValue1 = (const mtdCharType *)
                  mtd::valueForModule( (smiColumn *)aValueInfo1->column,
                                       aValueInfo1->value,
                                       aValueInfo1->flag,
                                       mtdChar.staticNull );
    sLength1 = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)
                  mtd::valueForModule( (smiColumn *)aValueInfo2->column,
                                       aValueInfo2->value,
                                       aValueInfo2->flag,
                                       mtdChar.staticNull );

    sLength2 = sCharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     *
     * BUG-43106
     * 
     * - 위의 이유로 length가 수정됐을 경우
     *   문자열 마지막의 2byte문자가 1byte만 포함되고 잘릴가능성이 있다.
     *   그러나 아래 키비교시에 확인하므로, 여기서는 추가처리하지 않는다.
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdChar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdChar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdChar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdChar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------
    
    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if ( sLength1 >= sLength2 )
        {
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while ( sIterator2 < sLength2 )
            {
                sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue1,
                                                                         sLength1,
                                                                         sIterator1,
                                                                         &sOneCharLength1,
                                                                         sValue2,
                                                                         sLength2,
                                                                         sIterator2,
                                                                         &sOneCharLength2,
                                                                         &sIs2ByteCharCut );

                if ( sIs2ByteCharCut == ID_TRUE )
                {
                    /* BUG-43106
                       문자열 마지막의 2byte문자가
                       앞 1bytes만 문자열에 포함되고,
                       뒤 1bytes는 문자열에 포함되지 않은 경우이다. */
                    return 0;
                }

                if ( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for ( sIterator = sValue1 + sLength2,
                  sFence    = sValue1 + sLength1;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;

        while ( sIterator1 < sLength1 )
        {
            sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue1,
                                                                     sLength1,
                                                                     sIterator1,
                                                                     &sOneCharLength1,
                                                                     sValue2,
                                                                     sLength2,
                                                                     sIterator2,
                                                                     &sOneCharLength2,
                                                                     &sIs2ByteCharCut );

            if ( sIs2ByteCharCut == ID_TRUE )
            {
                /* BUG-43106 */
                return 0;
            }

            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for ( sIterator = sValue2 + sLength1,
              sFence    = sValue2 + sLength2;
              sIterator < sFence;
              sIterator++ )
        {
            if ( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }
    
    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlCharMS949collateIndexKeyMtdDesc( mtdValueInfo * aValueInfo1,
                                                     mtdValueInfo * aValueInfo2 ) 
{
/***********************************************************************
 *
 * BUG-43106 
 *
 * Description : mtlCollate::mtlCharMS949collateMtdMtdDesc() 함수를
 *               index key 비교용으로 변경함 함수.
 *
 *             ( PROJ-2433 Direct Key INDEX의 
 *               partial direct key 비교를 위한 코드가 추가됨 ) 
 *
 ***********************************************************************/

    const mtdCharType  * sCharValue1;
    const mtdCharType  * sCharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    SInt                 sCompared;
    const UChar        * sIterator;
    const UChar        * sFence;
    UInt                 sDirectKeyPartialSize;
    idBool               sIs2ByteCharCut = ID_FALSE;

    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------    
    sCharValue1 = (const mtdCharType *)
                  mtd::valueForModule( (smiColumn *)aValueInfo1->column,
                                        aValueInfo1->value,
                                        aValueInfo1->flag,
                                        mtdChar.staticNull );
    sLength1 = sCharValue1->length;

    //---------
    // value2
    //---------
    sCharValue2 = (const mtdCharType*)
                  mtd::valueForModule( (smiColumn *)aValueInfo2->column,
                                       aValueInfo2->value,
                                       aValueInfo2->flag,
                                       mtdChar.staticNull );

    sLength2 = sCharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     *
     * BUG-43106
     * 
     * - 위의 이유로 length가 수정됐을 경우
     *   문자열 마지막의 2byte문자가 1byte만 포함되고 잘릴가능성이 있다.
     *   그러나 아래 키비교시에 확인하므로, 여기서는 추가처리하지 않는다.
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdChar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdChar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdChar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdChar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------

    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sCharValue1->value;
        sValue2  = sCharValue2->value;
        
        if ( sLength2 >= sLength1 )
        {
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;

            while ( sIterator1 < sLength1 )
            {
                sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue2,
                                                                         sLength2,
                                                                         sIterator2,
                                                                         &sOneCharLength2,
                                                                         sValue1,
                                                                         sLength1,
                                                                         sIterator1,
                                                                         &sOneCharLength1,
                                                                         &sIs2ByteCharCut );

                if ( sIs2ByteCharCut == ID_TRUE )
                {
                    /* BUG-43106
                       문자열 마지막의 2byte문자가
                       앞 1bytes만 문자열에 포함되고,
                       뒤 1bytes는 문자열에 포함되지 않은 경우이다. */
                    return 0;
                }

                if ( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            
            for ( sIterator = sValue2 + sLength1,
                  sFence    = sValue2 + sLength2;
                  sIterator < sFence;
                  sIterator++ )
            {
                if ( *sIterator != ' ' )
                {
                    return 1;
                }
            }
            return 0;
        }
        
        sIterator1 = 0;
        sIterator2 = 0;
        sOneCharLength1   = 0;
        sOneCharLength2   = 0;

        while ( sIterator2 < sLength2 )
        {
            sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue2,
                                                                     sLength2,
                                                                     sIterator2,
                                                                     &sOneCharLength2,
                                                                     sValue1,
                                                                     sLength1,
                                                                     sIterator1,
                                                                     &sOneCharLength1,
                                                                     &sIs2ByteCharCut );

            if ( sIs2ByteCharCut == ID_TRUE )
            {
                /* BUG-43106 */
                return 0;
            }

            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        
        for ( sIterator = sValue1 + sLength2,
              sFence    = sValue1 + sLength1;
              sIterator < sFence;
              sIterator++ )
        {
            if ( *sIterator != ' ' )
            {
                return -1;
            }
        }
        return 0;
    }

    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}
/* BUG-43106 end */

SInt mtlCollate::mtlVarcharMS949collateMtdMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sVarcharValue1;
    const mtdCharType  * sVarcharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    
    SInt                 sCompared;
    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------    
    sVarcharValue1 = (const mtdCharType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                           aValueInfo1->value,
                                           aValueInfo1->flag,
                                           mtdChar.staticNull );
    sLength1 = sVarcharValue1->length;

    //---------
    // value2
    //---------
    sVarcharValue2 = (const mtdCharType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdChar.staticNull );

    sLength2 = sVarcharValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarcharValue1->value;
        sValue2  = sVarcharValue2->value;
        
        if( sLength1 > sLength2 )
        {
            //return idlOS::memcmp( sValue1->value, sValue2->value,
            //                      sValue2->length ) >= 0 ? 1 : -1 ;

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while( sIterator2 < sLength2 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1,
                                                        sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2 );
                    
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        
        if( sLength1 < sLength2 )
        {
            //return idlOS::memcmp( sValue1->value, sValue2->value,
            //                      sValue1->length ) > 0 ? 1 : -1 ;
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while( sIterator1 < sLength1 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1,
                                                        sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2 );
                   
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }
        //return idlOS::memcmp( sValue1->value, sValue2->value,
        //                      sValue1->length );
        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while( sIterator1 < sLength1 )
        {
            sCompared =
                mtlCollate::mtlCompareMS949Collate( sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1,
                                                    sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2 );
                   
            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlVarcharMS949collateMtdMtdDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sVarcharValue1;
    const mtdCharType  * sVarcharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;

    SInt               sCompared;
    UInt               sIterator1;
    UInt               sIterator2;
    UShort             sOneCharLength1;
    UShort             sOneCharLength2;

    //---------
    // value1
    //---------    
    sVarcharValue1 = (const mtdCharType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                                           aValueInfo1->value,
                                           aValueInfo1->flag,
                                           mtdChar.staticNull );
    sLength1 = sVarcharValue1->length;

    //---------
    // value2
    //---------
    sVarcharValue2 = (const mtdCharType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdChar.staticNull );

    sLength2 = sVarcharValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarcharValue1->value;
        sValue2  = sVarcharValue2->value;
        
        if( sLength2 > sLength1 )
        {
            //return idlOS::memcmp( sValue2->value, sValue1->value,
            //                      sValue1->length ) >= 0 ? 1 : -1 ;

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while( sIterator1 < sLength1 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2,
                                                        sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1 );
                    
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        if( sLength2 < sLength1 )
        {
            //return idlOS::memcmp( sValue2->value, sValue1->value,
            //                      sValue2->length ) > 0 ? 1 : -1 ;
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while( sIterator2 < sLength2 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2,
                                                        sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1 );
                   
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }
        //return idlOS::memcmp( sValue2->value, sValue1->value,
        //                      sValue2->length );

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while( sIterator2 < sLength2 )
        {
            sCompared =
                mtlCollate::mtlCompareMS949Collate( sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2,
                                                    sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1 );
                   
            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}
                                  
SInt mtlCollate::mtlVarcharMS949collateStoredMtdAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sVarcharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    
    SInt                 sCompared;
    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------

    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value2
    //---------

    sVarcharValue2 = (const mtdCharType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdChar.staticNull );
    sLength2 = sVarcharValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = sVarcharValue2->value;
        
        if( sLength1 > sLength2 )
        {
            //return idlOS::memcmp( sValue1->value, sValue2->value,
            //                      sValue2->length ) >= 0 ? 1 : -1 ;

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while( sIterator2 < sLength2 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1,
                                                        sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2 );
                    
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        
        if( sLength1 < sLength2 )
        {
            //return idlOS::memcmp( sValue1->value, sValue2->value,
            //                      sValue1->length ) > 0 ? 1 : -1 ;
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while( sIterator1 < sLength1 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1,
                                                        sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2 );
                   
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }
        //return idlOS::memcmp( sValue1->value, sValue2->value,
        //                      sValue1->length );
        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while( sIterator1 < sLength1 )
        {
            sCompared =
                mtlCollate::mtlCompareMS949Collate( sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1,
                                                    sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2 );
                   
            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}
                 
SInt mtlCollate::mtlVarcharMS949collateStoredMtdDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    const mtdCharType  * sVarcharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    
    SInt                 sCompared;
    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------
    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value2
    //---------
    sVarcharValue2 = (const mtdCharType*)
                      mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                           aValueInfo2->value,
                                           aValueInfo2->flag,
                                           mtdChar.staticNull );
    sLength2 = sVarcharValue2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = sVarcharValue2->value;
        
        if( sLength2 > sLength1 )
        {
            //return idlOS::memcmp( sValue2->value, sValue1->value,
            //                      sValue1->length ) >= 0 ? 1 : -1 ;

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while( sIterator1 < sLength1 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2,
                                                        sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1 );
                    
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        if( sLength2 < sLength1 )
        {
            //return idlOS::memcmp( sValue2->value, sValue1->value,
            //                      sValue2->length ) > 0 ? 1 : -1 ;
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while( sIterator2 < sLength2 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2,
                                                        sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1 );
                   
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }
        //return idlOS::memcmp( sValue2->value, sValue1->value,
        //                      sValue2->length );

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while( sIterator2 < sLength2 )
        {
            sCompared =
                mtlCollate::mtlCompareMS949Collate( sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2,
                                                    sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1 );
                   
            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlVarcharMS949collateStoredStoredAsc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    
    SInt                 sCompared;
    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------

    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value2
    //---------

    sLength2 = (UShort)aValueInfo2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = (UChar*)aValueInfo2->value;
        
        if( sLength1 > sLength2 )
        {
            //return idlOS::memcmp( sValue1->value, sValue2->value,
            //                      sValue2->length ) >= 0 ? 1 : -1 ;

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while( sIterator2 < sLength2 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1,
                                                        sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2 );
                    
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        
        if( sLength1 < sLength2 )
        {
            //return idlOS::memcmp( sValue1->value, sValue2->value,
            //                      sValue1->length ) > 0 ? 1 : -1 ;
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while( sIterator1 < sLength1 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1,
                                                        sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2 );
                   
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }
        //return idlOS::memcmp( sValue1->value, sValue2->value,
        //                      sValue1->length );
        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while( sIterator1 < sLength1 )
        {
            sCompared =
                mtlCollate::mtlCompareMS949Collate( sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1,
                                                    sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2 );
                   
            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlVarcharMS949collateStoredStoredDesc(
    mtdValueInfo * aValueInfo1,
    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               매핑되는 UNICODE 로 변환해서 비교 수행
 *               ( MS949가 KSC5601의 super set으로
 *                 KSC5601 코드가 모두 포함되어 있음. )
 *
 *               즉, 초성을 포함한 한글정렬을 수행하기 위함.
 * 
 * Implementation :
 *
 ***********************************************************************/

    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    
    SInt                 sCompared;
    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------
    sLength1 = (UShort)aValueInfo1->length;

    //---------
    // value2
    //---------
    sLength2 = (UShort)aValueInfo2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = (UChar*)aValueInfo1->value;
        sValue2  = (UChar*)aValueInfo2->value;
        
        if( sLength2 > sLength1 )
        {
            //return idlOS::memcmp( sValue2->value, sValue1->value,
            //                      sValue1->length ) >= 0 ? 1 : -1 ;

            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while( sIterator1 < sLength1 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2,
                                                        sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1 );
                    
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        if( sLength2 < sLength1 )
        {
            //return idlOS::memcmp( sValue2->value, sValue1->value,
            //                      sValue2->length ) > 0 ? 1 : -1 ;
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while( sIterator2 < sLength2 )
            {
                sCompared =
                    mtlCollate::mtlCompareMS949Collate( sValue2,
                                                        sIterator2,
                                                        & sOneCharLength2,
                                                        sValue1,
                                                        sIterator1,
                                                        & sOneCharLength1 );
                   
                if( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }
        //return idlOS::memcmp( sValue2->value, sValue1->value,
        //                      sValue2->length );

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while( sIterator2 < sLength2 )
        {
            sCompared =
                mtlCollate::mtlCompareMS949Collate( sValue2,
                                                    sIterator2,
                                                    & sOneCharLength2,
                                                    sValue1,
                                                    sIterator1,
                                                    & sOneCharLength1 );
                   
            if( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

/* BUG-43106 */
SInt mtlCollate::mtlVarcharMS949collateIndexKeyMtdAsc( mtdValueInfo * aValueInfo1,
                                                       mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * BUG-43106 
 *
 * Description : mtlCollate::mtlVarcharMS949collateMtdMtdAsc() 함수를
 *               index key 비교용으로 변경함 함수.
 *
 *             ( PROJ-2433 Direct Key INDEX의 
 *               partial direct key 비교를 위한 코드가 추가됨 ) 
 *
 ***********************************************************************/

    const mtdCharType  * sVarcharValue1;
    const mtdCharType  * sVarcharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    UInt                 sDirectKeyPartialSize;
    idBool               sIs2ByteCharCut;
    
    SInt                 sCompared;
    UInt                 sIterator1;
    UInt                 sIterator2;
    UShort               sOneCharLength1;
    UShort               sOneCharLength2;

    //---------
    // value1
    //---------    
    sVarcharValue1 = (const mtdCharType *)
                     mtd::valueForModule( (smiColumn *)aValueInfo1->column,
                                          aValueInfo1->value,
                                          aValueInfo1->flag,
                                          mtdChar.staticNull );
    sLength1 = sVarcharValue1->length;

    //---------
    // value2
    //---------
    sVarcharValue2 = (const mtdCharType *)
                     mtd::valueForModule( (smiColumn *)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdChar.staticNull );

    sLength2 = sVarcharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     *
     * BUG-43106
     * 
     * - 위의 이유로 length가 수정됐을 경우
     *   문자열 마지막의 2byte문자가 1byte만 포함되고 잘릴가능성이 있다.
     *   그러나 아래 키비교시에 확인하므로, 여기서는 추가처리하지 않는다.
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdVarchar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdVarchar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdVarchar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdVarchar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------

    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sVarcharValue1->value;
        sValue2  = sVarcharValue2->value;
        
        if ( sLength1 > sLength2 )
        {
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while ( sIterator2 < sLength2 )
            {
                sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue1,
                                                                         sLength1,
                                                                         sIterator1,
                                                                         &sOneCharLength1,
                                                                         sValue2,
                                                                         sLength2,
                                                                         sIterator2,
                                                                         &sOneCharLength2,
                                                                         &sIs2ByteCharCut );

                if ( sIs2ByteCharCut == ID_TRUE )
                {
                    /* BUG-43106
                       문자열 마지막의 2byte문자가
                       앞 1bytes만 문자열에 포함되고,
                       뒤 1bytes는 문자열에 포함되지 않은 경우이다. */
                    return 0;
                }

                if ( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        
        if ( sLength1 < sLength2 )
        {
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while ( sIterator1 < sLength1 )
            {
                sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue1,
                                                                         sLength1,
                                                                         sIterator1,
                                                                         &sOneCharLength1,
                                                                         sValue2,
                                                                         sLength2,
                                                                         sIterator2,
                                                                         &sOneCharLength2,
                                                                         &sIs2ByteCharCut );

                if ( sIs2ByteCharCut == ID_TRUE )
                {
                    /* BUG-43106 */
                    return 0;
                }

                if ( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while ( sIterator1 < sLength1 )
        {
            sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue1,
                                                                     sLength1,
                                                                     sIterator1,
                                                                     &sOneCharLength1,
                                                                     sValue2,
                                                                     sLength2,
                                                                     sIterator2,
                                                                     &sOneCharLength2,
                                                                     &sIs2ByteCharCut );

            if ( sIs2ByteCharCut == ID_TRUE )
            {
                /* BUG-43106 */
                return 0;
            }

            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

SInt mtlCollate::mtlVarcharMS949collateIndexKeyMtdDesc( mtdValueInfo * aValueInfo1,
                                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * BUG-43106 
 *
 * Description : mtlCollate::mtlVarcharMS949collateMtdMtdDesc() 함수를
 *               index key 비교용으로 변경함 함수.
 *
 *             ( PROJ-2433 Direct Key INDEX의 
 *               partial direct key 비교를 위한 코드가 추가됨 ) 
 *
 ***********************************************************************/

    const mtdCharType  * sVarcharValue1;
    const mtdCharType  * sVarcharValue2;
    UShort               sLength1;
    UShort               sLength2;
    const UChar        * sValue1;
    const UChar        * sValue2;
    UInt                 sDirectKeyPartialSize;
    idBool               sIs2ByteCharCut;

    SInt               sCompared;
    UInt               sIterator1;
    UInt               sIterator2;
    UShort             sOneCharLength1;
    UShort             sOneCharLength2;

    //---------
    // value1
    //---------    
    sVarcharValue1 = (const mtdCharType *)
                     mtd::valueForModule( (smiColumn *)aValueInfo1->column,
                                          aValueInfo1->value,
                                          aValueInfo1->flag,
                                          mtdChar.staticNull );
    sLength1 = sVarcharValue1->length;

    //---------
    // value2
    //---------
    sVarcharValue2 = (const mtdCharType *)
                     mtd::valueForModule( (smiColumn *)aValueInfo2->column,
                                          aValueInfo2->value,
                                          aValueInfo2->flag,
                                          mtdChar.staticNull );

    sLength2 = sVarcharValue2->length;

    /*
     * PROJ-2433 Direct Key Index
     * Partial Direct Key 처리
     * 
     * - Direct Key가 partial direct key인 경우
     *   partial된 길이만큼만 비교하도록 length를 수정한다
     *
     * BUG-43106
     * 
     * - 위의 이유로 length가 수정됐을 경우
     *   문자열 마지막의 2byte문자가 1byte만 포함되고 잘릴가능성이 있다.
     *   그러나 아래 키비교시에 확인하므로, 여기서는 추가처리하지 않는다.
     */ 
    if ( ( aValueInfo1->flag & MTD_PARTIAL_KEY_MASK ) == MTD_PARTIAL_KEY_ON )
    {
        sDirectKeyPartialSize = aValueInfo1->length;

        /* partail key 이면 */
        if ( sDirectKeyPartialSize != 0 )
        {
            /* direct key 길이보정*/
            if ( ( sLength1 + mtdVarchar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength1 = (UShort)( sDirectKeyPartialSize - mtdVarchar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }

            /* search key 도 partial 길이만큼 보정*/
            if ( ( sLength2 + mtdVarchar.headerSize() ) > sDirectKeyPartialSize )
            {
                sLength2 = (UShort)( sDirectKeyPartialSize - mtdVarchar.headerSize() );
            }
            else
            {
                /* nothing todo */
            }
        }
        else
        {
            /* nothing todo */
        }
    }
    else
    {
        /* nothing todo */
    }

    //---------
    // compare
    //---------

    if ( ( sLength1 != 0 ) && ( sLength2 != 0 ) )
    {
        sValue1  = sVarcharValue1->value;
        sValue2  = sVarcharValue2->value;
        
        if ( sLength2 > sLength1 )
        {
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
                
            while ( sIterator1 < sLength1 )
            {
                sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue2,
                                                                         sLength2,
                                                                         sIterator2,
                                                                         &sOneCharLength2,
                                                                         sValue1,
                                                                         sLength1,
                                                                         sIterator1,
                                                                         &sOneCharLength1,
                                                                         &sIs2ByteCharCut );

                if ( sIs2ByteCharCut == ID_TRUE )
                {
                    /* BUG-43106
                       문자열 마지막의 2byte문자가
                       앞 1bytes만 문자열에 포함되고,
                       뒤 1bytes는 문자열에 포함되지 않은 경우이다. */
                    return 0;
                }

                if ( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return 1;
        }
        if ( sLength2 < sLength1 )
        {
            sIterator1      = 0;
            sIterator2      = 0;
            sOneCharLength1 = 0;
            sOneCharLength2 = 0;
            
            while( sIterator2 < sLength2 )
            {
                sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue2,
                                                                         sLength2,
                                                                         sIterator2,
                                                                         &sOneCharLength2,
                                                                         sValue1,
                                                                         sLength1,
                                                                         sIterator1,
                                                                         &sOneCharLength1,
                                                                         &sIs2ByteCharCut );

                if ( sIs2ByteCharCut == ID_TRUE )
                {
                    /* BUG-43106 */
                    return 0;
                }

                if ( sCompared != 0 )
                {
                    return sCompared;
                }
                else
                {
                    sIterator1 += sOneCharLength1;
                    sIterator2 += sOneCharLength2;
                }
            }
            return -1;
        }

        sIterator1      = 0;
        sIterator2      = 0;
        sOneCharLength1 = 0;
        sOneCharLength2 = 0;
            
        while ( sIterator2 < sLength2 )
        {
            sCompared = mtlCompareMS949CollateWithCheck2ByteCharCut( sValue2,
                                                                     sLength2,
                                                                     sIterator2,
                                                                     &sOneCharLength2,
                                                                     sValue1,
                                                                     sLength1,
                                                                     sIterator1,
                                                                     &sOneCharLength1,
                                                                     &sIs2ByteCharCut );

            if ( sIs2ByteCharCut == ID_TRUE )
            {
                /* BUG-43106 */
                return 0;
            }

            if ( sCompared != 0 )
            {
                return sCompared;
            }
            else
            {
                sIterator1 += sOneCharLength1;
                sIterator2 += sOneCharLength2;
            }
        }
        return 0;
    }
    
    if ( sLength1 < sLength2 )
    {
        return 1;
    }
    if ( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}
/* BUG-43106 end */

SInt mtlCollate::mtlCompareMS949Collate( const UChar  * aValue1,
                                         UInt           aIterator1,
                                         UShort       * aLength1,
                                         const UChar  * aValue2,
                                         UInt           aIterator2,
                                         UShort       * aLength2 )
{
/***********************************************************************
 *
 * Description : MS949코드와 KSC5601 코드를
 *               대응되는 UNICODE로 변환해서 비교
 *
 * Implementation :
 *
 *   mtMS949ToUnicodeTable[] 에서는 
 *   모든 MS949의 코드와 대응되는 UNICODE 의 일차원배열로 구성되어 있음.
 *   이 UNICODE는 표준 UNICODE를 초성을 포함 한글정렬로 재배열되어 있음.
 *
 *   따라서, mtMS949ToUnicodeTable에 대응되는 UNICODE가
 *   존재하지 않는 경우, mtMS949ToUnicodeTable[0][0] (0x0000 )의 값이 리턴됨.
 *
 ***********************************************************************/

    SInt      sCompared;
    UShort    sMs949Code1;
    UShort    sMs949Code2;
    UShort    sUnicode1;
    UShort    sUnicode2;

    //-------------------------------------------
    // 한문자에 대한 ms949 code와 length를 구함.
    //-------------------------------------------

    mtlCollate::mtlSetCodeAndLenInfo( aValue1,
                                      aIterator1,
                                      & sMs949Code1,
                                      aLength1 );
    

    mtlCollate::mtlSetCodeAndLenInfo( aValue2,
                                      aIterator2,
                                      & sMs949Code2,
                                      aLength2 );

    //-------------------------------------------
    // mtMS949ToUnicodeTable 에서
    // ms949 code에 대응되는 unicode를 구한다.
    //-------------------------------------------

    sUnicode1 = mtlMS949CollationTable[sMs949Code1];
    sUnicode2 = mtlMS949CollationTable[sMs949Code2];

    //----------------------------------------
    // unicode로 비교
    //----------------------------------------
        
    if( sUnicode1 > sUnicode2 )
    {
        sCompared = 1;
    }
    else if( sUnicode1 < sUnicode2 )
    {
        sCompared = -1;
    }
    else
    {
        sCompared = 0;
    }

    return sCompared;
}

SInt mtlCollate::mtlCompareMS949CollateWithCheck2ByteCharCut( const UChar  * aValue1,
                                                              UShort         aLength1,
                                                              UInt           aIterator1,
                                                              UShort       * aOneCharLength1,
                                                              const UChar  * aValue2,
                                                              UShort         aLength2,
                                                              UInt           aIterator2,
                                                              UShort       * aOneCharLength2,
                                                              idBool       * aIs2ByteCharCut )
{
/***********************************************************************
 *
 * BUG-43106
 *
 * Description : 위의 mtlCollate::mtlCompareMS949Collate() 함수에
 *               2byte문자 체크 코드를 추가함.
 *
 *               두 입력문자열 비교중에 잘린 2byte문자가 포함됐을 경우
 *               *aIs2ByteCharCut = TRUE를 세팅한다. 
 *               ( 앞 1byte만 문자열에 포함되고
 *                 뒤 1byte는 문자열에 포함되지 않았다.)
 *
 ***********************************************************************/

    SInt      sCompared;
    UShort    sMs949Code1;
    UShort    sMs949Code2;
    UShort    sUnicode1;
    UShort    sUnicode2;
    idBool    sIs2ByteCharCut1 = ID_FALSE;
    idBool    sIs2ByteCharCut2 = ID_FALSE;

    //-------------------------------------------
    // 한문자에 대한 ms949 code와 length를 구함.
    //-------------------------------------------

    mtlCollate::mtlSetCodeAndLenInfoWithCheck2ByteCharCut( aValue1,
                                                           aLength1,
                                                           aIterator1,
                                                           & sMs949Code1,
                                                           aOneCharLength1,
                                                           &sIs2ByteCharCut1 );

    mtlCollate::mtlSetCodeAndLenInfoWithCheck2ByteCharCut( aValue2,
                                                           aLength2,
                                                           aIterator2,
                                                           & sMs949Code2,
                                                           aOneCharLength2,
                                                           &sIs2ByteCharCut2 );

    if ( ( sIs2ByteCharCut1 == ID_TRUE ) ||
         ( sIs2ByteCharCut2 == ID_TRUE ) )
    {
        *aIs2ByteCharCut = ID_TRUE;
        sCompared        = 0;
    }
    else
    {
        *aIs2ByteCharCut = ID_FALSE;

        //-------------------------------------------
        // mtMS949ToUnicodeTable 에서
        // ms949 code에 대응되는 unicode를 구한다.
        //-------------------------------------------

        sUnicode1 = mtlMS949CollationTable[sMs949Code1];
        sUnicode2 = mtlMS949CollationTable[sMs949Code2];

        //----------------------------------------
        // unicode로 비교
        //----------------------------------------

        if ( sUnicode1 > sUnicode2 )
        {
            sCompared = 1;
        }
        else if ( sUnicode1 < sUnicode2 )
        {
            sCompared = -1;
        }
        else
        {
            sCompared = 0;
        }
    }

    return sCompared;
}

