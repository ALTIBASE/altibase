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

#include <ide.h>
#include <idnAscii.h>
#include <idnJisx0201.h>
#include <idnJisx0208.h>
#include <idnCp932.h>
#include <idnCp932ext.h>

/* PROJ-2590 [기능성] CP932 database character set 지원 */
SInt convertMbToWc4Cp932( void * aSrc,
                          SInt   aSrcRemain,
                          void * aDest,
                          SInt   aDestRemain )
{
    UChar   * sSrcCharPtr;
    SInt      sRet;
    UChar     sBuf[ 2 ];
    UChar     t1;
    UChar     t2;
    UShort    wc;

    sSrcCharPtr = ( UChar * ) aSrc;

    if( sSrcCharPtr[0] < 0x80 )
    {
        sRet = convertMbToWc4Ascii( aSrc,
                                    aSrcRemain,
                                    aDest,
                                    aDestRemain );
    }
    else if ( sSrcCharPtr[0] >= 0xa1 && sSrcCharPtr[0] <= 0xdf )
    {
        sRet = convertMbToWc4Jisx0201( aSrc,
                                       aSrcRemain,
                                       aDest,
                                       aDestRemain);
    }
    else
    {
        if( (sSrcCharPtr[0] >= 0x81 && sSrcCharPtr[0] <= 0x9f && sSrcCharPtr[0] != 0x87 ) ||
            (sSrcCharPtr[0] >= 0xe0 && sSrcCharPtr[0] <= 0xea) )
        {
            if ( aSrcRemain < 2 )
            {
                sRet = RET_TOOFEW;
            }
            else
            {
                if( (sSrcCharPtr[1] >= 0x40 && sSrcCharPtr[1] <= 0x7e) ||
                    (sSrcCharPtr[1] >= 0x80 && sSrcCharPtr[1] <= 0xfc) )
                {
                    t1 = (sSrcCharPtr[0] < 0xe0 ?
                                    sSrcCharPtr[0]-0x81 : sSrcCharPtr[0]-0xc1);
                    t2 = (sSrcCharPtr[1] < 0x80 ?
                                    sSrcCharPtr[1]-0x40 : sSrcCharPtr[1]-0x41);

                    sBuf[0] = 2 * t1 + ( t2 < 0x5e ? 0 : 1 ) + 0x21;
                    sBuf[1] = (t2 < 0x5e ? t2 : t2-0x5e) + 0x21;

                    sRet = convertMbToWc4Jisx0208( sBuf,
                                                   2,
                                                   aDest,
                                                   aDestRemain);
                }
                else
                {
                    sRet = RET_ILSEQ;
                }
            }
        }
        else if ( (sSrcCharPtr[0] == 0x87) ||
                  (sSrcCharPtr[0] >= 0xed && sSrcCharPtr[0] <= 0xee) ||
                  (sSrcCharPtr[0] >= 0xfa) )
        {
            if ( aSrcRemain < 2 )
            {
                sRet = RET_TOOFEW;
            }
            else
            {
                sRet = convertMbToWc4Cp932ext( aSrc,
                                               2,
                                               aDest,
                                               aDestRemain );
            }
        }
        else if (sSrcCharPtr[0] >= 0xf0 && sSrcCharPtr[0] <= 0xf9)
        {
            if (aSrcRemain < 2)
            {
                sRet = RET_TOOFEW;
            }
            else
            {
                if( (sSrcCharPtr[1] >= 0x40 && sSrcCharPtr[1] <= 0x7e) ||
                    (sSrcCharPtr[1] >= 0x80 && sSrcCharPtr[1] <= 0xfc) )
                {
                    wc = 0xe000 +
                         188 * ( sSrcCharPtr[0] - 0xf0 ) +
                         ( sSrcCharPtr[1] < 0x80 ? sSrcCharPtr[1]-0x40 : sSrcCharPtr[1]-0x41 );

                    WC_TO_UTF16BE( aDest, wc );

                    sRet = 2;
                }
                else
                {
                    sRet = RET_ILSEQ;
                }
            }
        }
        else
        {
            sRet = RET_ILSEQ;
        }
    }

    return sRet;
}

/* PROJ-2590 [기능성] CP932 database character set 지원 */
SInt convertWcToMb4Cp932( void * aSrc,
                          SInt   aSrcRemain,
                          void * aDest,
                          SInt   aDestRemain )
{
    UChar   sBuf[2];
    SInt    sRet = RET_ILUNI;
    UChar * sDestCharPtr;
    UChar   t1;
    UChar   t2;
    UShort  wc;

    sDestCharPtr = (UChar *)aDest;

    UTF16BE_TO_WC( wc, aSrc );

    // ASCII
    sRet = convertWcToMb4Ascii( aSrc,
                                aSrcRemain,
                                sBuf,
                                aDestRemain );
    if ( sRet != RET_ILUNI )
    {
        if( sBuf[0] < 0x80 )
        {
            sDestCharPtr[0] = sBuf[0];
            sRet = 1;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    // JIS X 0201-1976.
    sRet = convertWcToMb4Jisx0201( aSrc,
                                   aSrcRemain,
                                   sBuf,
                                   1 );
    if ( sRet != RET_ILUNI )
    {
        if( ( sBuf[0] < 0x80 ) ||
            ( sBuf[0] >= 0xa1 && sBuf[0] <= 0xdf ) )
        {
            sDestCharPtr[0] = sBuf[0];
            sRet = 1;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    // JIS X 0208-1990.
    sRet = convertWcToMb4Jisx0208( aSrc,
                                   aSrcRemain,
                                   sBuf,
                                   2 );
    if ( sRet != RET_ILUNI )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            if( ( sBuf[0] >= 0x21 && sBuf[0] <= 0x74 ) &&
                ( sBuf[1] >= 0x21 && sBuf[1] <= 0x7e ) )
            {
                t1 = ( sBuf[0] - 0x21 ) >> 1;
                t2 = ( ( (sBuf[0] - 0x21) & 1 ) ? 0x5e : 0 ) + ( sBuf[1] - 0x21 );

                sDestCharPtr[0] = ( t1 < 0x1f ? t1 + 0x81 : t1 + 0xc1 );
                sDestCharPtr[1] = ( t2 < 0x3f ? t2 + 0x40 : t2 + 0x41 );

                sRet = 2;
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                // Nothing to do
            }
        }
    }
    else
    {
        // Nothing to do
    }

    // CP932 extensions
    sRet = convertWcToMb4Cp932ext( aSrc,
                                   aSrcRemain,
                                   sBuf,
                                   2 );
    if ( sRet != RET_ILUNI )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = sBuf[0];
            sDestCharPtr[1] = sBuf[1];
            sRet = 2;
        }
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do
    }

    // User-defined range. See
    // Ken Lunde's "CJKV Information Processing", table 4-66, p. 206.
    if ( ( wc >= 0xe000 ) && ( wc < 0xe758 ) )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
        }
        else
        {
            sBuf[0] = (UInt) ( wc - 0xe000 ) / 188;
            sBuf[1] = (UInt) ( wc - 0xe000 ) % 188;

            sDestCharPtr[0] = sBuf[0]+0xf0;
            sDestCharPtr[1] = ( sBuf[1] < 0x3f ? sBuf[1] + 0x40 : sBuf[1] + 0x41 );

            sRet = 2;
        }
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do
    }

    // Irreversible mappings.
    if ( wc == 0xff5e )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x60;
            sRet = 2;
        }

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do
    }

    if ( wc == 0x2225 )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x61;
            sRet = 2;
        }

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do
    }

    if ( wc == 0xff0d )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x7c;
            sRet = 2;
        }
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do
    }

    if ( wc == 0xffe0 )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x91;
            sRet = 2;
        }
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do
    }

    if ( wc == 0xffe1 )
    {
        if ( aDestRemain < 2 )
        {
            sRet = RET_TOOSMALL;
        }
        else
        {
            sDestCharPtr[0] = 0x81;
            sDestCharPtr[1] = 0x92;
            sRet = 2;
        }
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return sRet;
}
