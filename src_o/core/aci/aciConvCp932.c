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
 * $Id$
 **********************************************************************/
#include <aciErrorMgr.h>
#include <aciConvJisx0201.h>
#include <aciConvJisx0208.h>
#include <aciConvAscii.h>
#include <aciConvCp932.h>
#include <aciConvCp932ext.h>

/* PROJ-2590 [기능성] CP932 database character set 지원 */
ACP_EXPORT
acp_sint32_t aciConvConvertMbToWc4Cp932( void         * aSrc,
                                         acp_sint32_t   aSrcRemain,
                                         acp_sint32_t * aSrcAdvance,
                                         void         * aDest,
                                         acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      CP932 ==> UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   * sSrcCharPtr = ( acp_uint8_t * )aSrc;
    acp_sint32_t    sRet = ACICONV_RET_ILUNI;
    acp_uint8_t     sBuf[2];
    acp_uint8_t     sTemp1 = 0;
    acp_uint8_t     sTemp2 = 0;
    acp_uint16_t    sWc = 0;
    
    if ( sSrcCharPtr[0] < 0x80 )
    {
        sRet = aciConvConvertMbToWc4Ascii( aSrc, 
                                           aSrcRemain, 
                                           aSrcAdvance,
                                           aDest,
                                           aDestRemain );
    }
    else if ( ( sSrcCharPtr[0] >= 0xa1 ) && ( sSrcCharPtr[0] <= 0xdf ) )
    {
        sRet = aciConvConvertMbToWc4Jisx0201( aSrc,
                                              aSrcRemain, 
                                              aSrcAdvance,
                                              aDest, 
                                              aDestRemain );
    }
    else
    {
        if ( ( ( sSrcCharPtr[0] >= 0x81 ) && ( sSrcCharPtr[0] <= 0x9f ) && ( sSrcCharPtr[0] != 0x87 ) ) ||
             ( ( sSrcCharPtr[0] >= 0xe0 ) && ( sSrcCharPtr[0] <= 0xea ) ) )
        {
            if ( aSrcRemain < 2 )
            {
                sRet = ACICONV_RET_TOOFEW;
                *aSrcAdvance = aSrcRemain;
            }
            else
            {
                if ( ( ( sSrcCharPtr[1] >= 0x40 ) && ( sSrcCharPtr[1] <= 0x7e ) ) ||
                     ( ( sSrcCharPtr[1] >= 0x80 ) && ( sSrcCharPtr[1] <= 0xfc ) ) )
                {
                    sTemp1 = ( ( sSrcCharPtr[0] < 0xe0 ) ? ( sSrcCharPtr[0] - 0x81 )
                                                         : ( sSrcCharPtr[0] - 0xc1 ) );
                    sTemp2 = ( ( sSrcCharPtr[1] < 0x80 ) ? ( sSrcCharPtr[1] - 0x40 )
                                                         : ( sSrcCharPtr[1] - 0x41 ) );

                    sBuf[0] = 2 * sTemp1 + ( ( sTemp2 < 0x5e ) ? 0 : 1 ) + 0x21;
                    sBuf[1] = ( ( sTemp2 < 0x5e ) ? sTemp2 : ( sTemp2 - 0x5e ) ) + 0x21;

                    sRet = aciConvConvertMbToWc4Jisx0208( sBuf, 
                                                          2, 
                                                          aSrcAdvance, 
                                                          aDest, 
                                                          aDestRemain );
                }
                else
                {
                    sRet = ACICONV_RET_ILSEQ;
                    *aSrcAdvance = 2;
                }
            }
        }
        else if ( ( sSrcCharPtr[0] == 0x87 ) ||
                  ( ( sSrcCharPtr[0] >= 0xed ) && ( sSrcCharPtr[0] <= 0xee ) ) ||
                  ( sSrcCharPtr[0] >= 0xfa ) )
        {
            if ( aSrcRemain < 2 )
            {
                sRet = ACICONV_RET_TOOFEW;
                *aSrcAdvance = aSrcRemain;
            }
            else
            {
                sRet = aciConvConvertMbToWc4Cp932ext( aSrc, 
                                                      2, 
                                                      aSrcAdvance, 
                                                      aDest );
            }
        }
        else if ( ( sSrcCharPtr[0] >= 0xf0 ) && ( sSrcCharPtr[0] <= 0xf9 ) )
        {
            /* User-defined range. See
            * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
            if ( aSrcRemain < 2 )
            {
                sRet = ACICONV_RET_TOOFEW;
                *aSrcAdvance = aSrcRemain;
            }
            else
            {
                if ( ( ( sSrcCharPtr[1] >= 0x40 ) && ( sSrcCharPtr[1] <= 0x7e ) ) ||
                     ( ( sSrcCharPtr[1] >= 0x80 ) && ( sSrcCharPtr[1] <= 0xfc ) ) )
                {
                    sWc = 0xe000 +
                          188 * ( sSrcCharPtr[0] - 0xf0 ) +
                          ( ( sSrcCharPtr[1] < 0x80 ) ?
                            ( sSrcCharPtr[1] - 0x40 ) :
                            ( sSrcCharPtr[1] - 0x41 ) );

                    ACICONV_WC_TO_UTF16BE( aDest, sWc );
                    sRet = 2;
                }
                else
                {
                    sRet = ACICONV_RET_ILSEQ;
                }
                *aSrcAdvance = 2;
            }
        }
        else
        {
            sRet = ACICONV_RET_ILSEQ;
            *aSrcAdvance = 1;
        }
    }

    return sRet;
}

/* PROJ-2590 [기능성] CP932 database character set 지원 */
ACP_EXPORT
acp_sint32_t aciConvConvertWcToMb4Cp932( void         * aSrc,
                                         acp_sint32_t   aSrcRemain,
                                         acp_sint32_t * aSrcAdvance,
                                         void         * aDest,
                                         acp_sint32_t   aDestRemain )
{
/***********************************************************************
 *
 * Description :
 *      CP932 <== UTF16BE
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint8_t   sBuf[2];
    acp_sint32_t  sRet         = ACICONV_RET_ILUNI;
    acp_uint8_t * sDestCharPtr = (acp_uint8_t *)aDest;
    acp_uint8_t   sTemp1 = 0;
    acp_uint8_t   sTemp2 = 0;
    acp_uint16_t  sWc = 0;
    
    ACICONV_UTF16BE_TO_WC( sWc, aSrc );
    
    sRet = aciConvConvertWcToMb4Ascii( aSrc,
                                       aSrcRemain,
                                       aSrcAdvance,
                                       sBuf,
                                       aDestRemain );
    if ( sRet != ACICONV_RET_ILUNI )
    {
        if ( sBuf[0] < 0x80 )
        {
            sDestCharPtr[0] = sBuf[0];
            ACI_RAISE( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* Try JIS X 0201-1976. */
    sRet = aciConvConvertWcToMb4Jisx0201( aSrc,
                                          aSrcRemain,
                                          aSrcAdvance,
                                          sBuf,
                                          1 );
    if ( sRet != ACICONV_RET_ILUNI )
    {
        if ( ( sBuf[0] <= 0x80 ) ||
             ( ( sBuf[0] >= 0xa1 ) && ( sBuf[0] <= 0xdf ) ) )
        {
            sDestCharPtr[0] = sBuf[0];
            ACI_RAISE( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* Try JIS X 0208-1990. */
    sRet = aciConvConvertWcToMb4Jisx0208( aSrc,
                                          aSrcRemain,
                                          aSrcAdvance,
                                          sBuf,
                                          2 );
    if ( sRet != ACICONV_RET_ILUNI )
    {
        if ( aDestRemain < 2 )
        {
            ACI_RAISE( NORMAL_EXIT );
        }
        else
        {
            if ( ( ( sBuf[0] >= 0x21 ) && ( sBuf[0] <= 0x74 ) ) && 
                 ( ( sBuf[1] >= 0x21 ) && ( sBuf[1] <= 0x7e ) ) )
            {
                sTemp1 = ( sBuf[0] - 0x21 ) >> 1;
                sTemp2 = ( ( ( sBuf[0] - 0x21 ) & 1) ? 0x5e : 0 ) + ( sBuf[1] - 0x21 );

                sDestCharPtr[0] = ( ( sTemp1 < 0x1f ) ? ( sTemp1 + 0x81 ) : ( sTemp1 + 0xc1 ) );
                sDestCharPtr[1] = ( ( sTemp2 < 0x3f ) ? ( sTemp2 + 0x40 ) : ( sTemp2 + 0x41 ) );

                ACI_RAISE( NORMAL_EXIT );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    sRet = aciConvConvertWcToMb4Cp932ext( aSrc,
                                          aSrcAdvance,
                                          sBuf,
                                          2 );
    if ( sRet != ACICONV_RET_ILUNI )
    {
        if ( aDestRemain >= 2 )
        {
            sDestCharPtr[0] = sBuf[0];
            sDestCharPtr[1] = sBuf[1];
        }
        else
        {
            /* Nothing to do */
        }

        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* User-defined range. See
    * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
    if ( ( sWc >= 0xe000 ) && ( sWc < 0xe758 ) )
    {
        if ( aDestRemain < 2 )
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_TOOSMALL;
        }
        else
        {
            sBuf[0] = (acp_uint32_t) ( sWc - 0xe000 ) / 188;
            sBuf[1] = (acp_uint32_t) ( sWc - 0xe000 ) % 188;

            sDestCharPtr[0] = sBuf[0] + 0xf0;
            sDestCharPtr[1] = ( ( sBuf[1] < 0x3f ) ? ( sBuf[1] + 0x40 ) : ( sBuf[1] + 0x41 ) );

            *aSrcAdvance = 2;
            sRet = 2;
        }
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* Irreversible mappings.  */
    if ( sWc == 0xff5e )
    {
        if ( aDestRemain < 2 )
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x60;
            *aSrcAdvance = 2;
            sRet = 2;
        }
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sWc == 0x2225 )
    {
        if ( aDestRemain < 2 )
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x61;
            *aSrcAdvance = 2;
            sRet = 2;
        }
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sWc == 0xff0d )
    {
        if ( aDestRemain < 2 )
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x7c;
            *aSrcAdvance = 2;
            sRet = 2;
        }
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sWc == 0xffe0 )
    {
        if ( aDestRemain < 2 )
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x91;
            *aSrcAdvance = 2;
            sRet = 2;
        }
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sWc == 0xffe1 )
    {
        if ( aDestRemain < 2 )
        {
            *aSrcAdvance = 2;
            sRet = ACICONV_RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x92;
            *aSrcAdvance = 2;
            sRet = 2;
        }
        ACI_RAISE( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    *aSrcAdvance = 2;
    ACI_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}
