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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
  
#if !defined(_O_IDU_SHM_DUMP_H_)
#define _O_IDU_SHM_DUMP_H_ 1

#include <iduShmMgr.h>

// define section
#define STRINGIZE(enum_state) #enum_state
#define LINE_BUF_MAX          512

class iduShmDump
{
public:

private:
    static const SChar     mErrTextBufInsufficient[];

public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    // aMsgBuf[aBufSize]에 해당되는 메시지를 채운다.
    static IDE_RC getMsgSysSeg( SChar * aMsgBuf,
                                UInt    aBufSize );
    static IDE_RC getMsgDataSeg( SChar * aMsgBuf,
                                 UInt    aBufSize );
    static IDE_RC getMsgMatrix( SChar * aMsgBuf,
                                UInt    aBufSize );
    static IDE_RC getMsgFreeBlocks( SChar * aMsgBuf,
                                    UInt    aBufSize );
    static IDE_RC getMsgProcessMgrInfo( SChar * aMsgBuf,
                                        UInt    aBufSize );
    static IDE_RC getMsgProcess( SChar * aMsgBuf,
                                 UInt    aBufSize,
                                 UInt    aLPID );
    static IDE_RC getMsgBinDump( SChar   * aMsgBuf,
                                 UInt      aBufSize,
                                 idShmAddr aShmAddr,
                                 UInt      aDumpLength );

private:
    // aMsgBuf[aBufSize]에 해당되는 메시지를 채운다.
    static UInt getMsgShmHeader( SChar * aMsgBuf,
                                 UInt    aBufSize );
    static UInt getMsgShmStatictics( SChar * aMsgBuf,
                                     UInt    aBufSize );
    static UInt getMsgStShmSegInfo( SChar * aMsgBuf,
                                    UInt    aBufSize,
                                    UInt    aIndex );
    static UInt getMsgShmBlock( SChar             * aMsgBuf,
                                UInt                aBufSize,
                                iduShmBlockHeader * aShmBlkHeader );
    static UInt getMsgProcWithThr( SChar * aMsgBuf,
                                   UInt    aBufSize,
                                   UInt    aIndex,
                                   idBool  aPrintWithThreads );

    static UInt getMsgThreads( SChar         * aMsgBuf,
                               UInt            aBufSize,
                               iduShmTxInfo  * aTxInfo );

    // stringize
    static const SChar * stringizeShmState( iduShmState aState );
    static const SChar * stringizeProcState( iduShmProcState aState );
    static const SChar * stringizeProcType( iduShmProcType aType );
    static const SChar * stringizeThrState( iduShmThrState aState );
    static SChar * stringizeBinStyle( UInt    aBitmap,
                                      UInt    aMaxBit,
                                      SChar * aBuf,
                                      UInt    aBufLength );
    static SChar * stringizeShmAddr( idShmAddr aShmAddr,
                                     SChar   * aBuf,
                                     UInt      aBufLength );
    static SChar * stringizeTimeval( struct timeval * aTimeval,
                                     SChar          * aBuf,
                                     UInt             aBufLength );
    static SChar * stringizeDumpAdr( SChar * aPtr,
                                     idBool  aEnableAlign,
                                     SChar * aBuf,
                                     UInt    aBufLength );
    static SChar * stringizeDumpBin( SChar * aPtr,
                                     UInt    aByte,
                                     idBool  aEnableAlign,
                                     SChar * aBuf,
                                     UInt    aBufLength );
    static SChar * stringizeDumpHex( SChar * aPtr,
                                     UInt    aByte,
                                     idBool  aEnableAlign,
                                     SChar * aBuf,
                                     UInt    aBufLength );

    // others
    static inline SChar filterBin2ASCII( SChar aChar );
    static inline SChar * convBin2Hex( SChar aChar, SChar * aHexStrBuf );
};

inline SChar iduShmDump::filterBin2ASCII( SChar aChar )
{
    SChar sFilteredChar = aChar;

    if( aChar <= 32 || aChar >= 127 )
    { // not a ASCII
        sFilteredChar = '.';
    }

    return sFilteredChar;
}

inline SChar * iduShmDump::convBin2Hex( SChar aChar, SChar * aHexStrBuf )
{
    idlOS::sprintf( aHexStrBuf, "%02x", (UChar)aChar ); // Unsigned Hex

    return aHexStrBuf;
}

#endif /* _O_IDU_SHM_DUMP_H_ */
