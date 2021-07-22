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
 * Copyright 1999-2001, ATIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <ida.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idn.h>
#include <ideCallback.h>
#include <ideLog.h>

/*
  input   0  ~ 15
  output '0' ~ 'F'
*/
#define HalfByteToHex(halfByte)                         \
    ( (  (0 <= (halfByte)) && ((halfByte) <= 9) ) ?     \
      ( (halfByte) + '0' )                              \
      :                                                 \
      ( ((halfByte) - 10) + 'A' )                       \
        )


/*
  input   '0' ~ 'F'
  output   0  ~ 15
*/
#define HexToHalfByte(hex)                      \
    ( (  ('0' <= (hex)) && ((hex) <= '9') ) ?   \
      ( (hex) - '0' )                           \
      :                                         \
      ( ((hex) - 'A') + 10 )                    \
        )

/*
  input   0 ~ 255
  output  '00' ~ 'FF'
*/
/**
 * converts @a aByte to hexadecimal form and store it into @a aHex
 */
void idaXaByteToHex(
    SChar             aByte,
    SChar           * aHex )
{
    /* BUG-20670 */
    *aHex++ = HalfByteToHex( (aByte >> 4) & (0x0F) ) ;
    *aHex = HalfByteToHex( aByte & (0x0F) );
}

/**
 * converts length 2 string @a aHex in hexadecimal form into byte and store it into @a aByte
 */
void idaXaHexToByte(
    SChar           * aHex ,
    SChar           * aByte )
{
    /* BUG-20687 */
    *aByte  = ( HexToHalfByte( ( *aHex ) ) << 4 ) & (0xF0) ;
    *aByte |= ( HexToHalfByte( ( *(aHex+1) ) ) ) & (0x0F) ;
}

/**
 * converts #ID_XID value into format ID, global transaction id, and qualifier.
 * @param aXid source #ID_XID value
 * @param aFormatId to store format ID
 * @param aGlobalTxId to store global transaction ID
 * @param aBranchQualifier to store qualifier
 */
IDE_RC idaXaXidToString(
    /* BUG-18981 */
    ID_XID          * aXid,
    vULong          * aFormatId,
    SChar           * aGlobalTxId,
    SChar           * aBranchQualifier )
{
    UInt i=0;

    *aFormatId = aXid->formatID ;

    /* bug-36037: invalid xid
       to remove assertion code to protect
       the server from any invalid xid */
    IDE_TEST(aXid->gtrid_length <= (vSLong) 0);
    IDE_TEST(aXid->gtrid_length >  (vSLong) ID_MAXGTRIDSIZE);
    IDE_TEST(aXid->bqual_length <= (vSLong) 0);
    IDE_TEST(aXid->bqual_length >  (vSLong) ID_MAXBQUALSIZE);

    for (i = 0  ;  i < (UInt)aXid->gtrid_length  ;  i++)
    {
        idaXaByteToHex( aXid->data[i], aGlobalTxId );
        aGlobalTxId += 2;
    }

    for (i = aXid->gtrid_length  ;
         i < (UInt)( aXid->gtrid_length + aXid->bqual_length)  ;
         i++)
    {
        idaXaByteToHex( aXid->data[i], aBranchQualifier );
        aBranchQualifier += 2;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* bug-36037: invalid xid
       to save invalid xid error msg into xa msg log file */
    ideLog::logLine(IDE_XA_0,
            "idaXaXidToString: invalid xid detected:"
            " formatID: %d"
            " gtrid_length: %d"
            " bqual_length: %d",
            aXid->formatID, aXid->gtrid_length, aXid->bqual_length);

    return IDE_FAILURE;
}

/**
 * converts format ID, global transaction id, and qualifier into #ID_XID.
 * @param aFormatId source format ID
 * @param aGlobalTxId source global transaction ID
 * @param aBranchQualifier source qualifier
 * @param aXid to store converted #ID_XID value
 */
void idaXaStringToXid(
    vULong            aFormatId,
    SChar           * aGlobalTxId,
    SChar           * aBranchQualifier,
    /* BUG-18981 */
    ID_XID          * aXid              )
{
    UInt i=0;
    SInt sDataLen;

    aXid->formatID = aFormatId;

    aXid->gtrid_length = idlOS::strlen(aGlobalTxId) / 2 ;
    aXid->bqual_length = idlOS::strlen(aBranchQualifier) / 2 ;

    sDataLen = aXid->gtrid_length + aXid->bqual_length ;

    IDE_ASSERT( aXid->gtrid_length <= (vSLong) ID_MAXGTRIDSIZE );
    IDE_ASSERT( aXid->bqual_length <= (vSLong) ID_MAXBQUALSIZE );

    for (i = 0  ;  i < (UInt)aXid->gtrid_length  ;  i++)
    {
        idaXaHexToByte( aGlobalTxId, & ( aXid->data[i] ) );
        aGlobalTxId += 2;
    }

    for (i = aXid->gtrid_length  ;
         i < (UInt)( aXid->gtrid_length + aXid->bqual_length)  ;
         i++)
    {
        idaXaHexToByte( aBranchQualifier, & ( aXid->data[i] ) );
        aBranchQualifier += 2;
    }

    // prevent UMR
    if ( sDataLen < ( ID_MAXGTRIDSIZE + ID_MAXBQUALSIZE ) )
    {
        idlOS::memset( & ( aXid->data[sDataLen] ),
                       0x00,
                       ( ID_MAXGTRIDSIZE + ID_MAXBQUALSIZE ) - sDataLen );
    }
}

/**
 * TBD
 */
UInt idaXaConvertXIDToString( void        */*aBaseObj*/,
                              void        * aMember,
                              UChar       * aBuf,
                              UInt          aBufSize)
{
    ID_XID *sXID;
    UInt    sLen = 0;

    vULong sFormatID;
    SChar  sGlobalTxId      [ID_MAXXIDDATASIZE + 1];
    SChar  sBranchQualifier [ID_MAXXIDDATASIZE + 1];

    idlOS::memset( sGlobalTxId,
                   0,
                   ID_MAXXIDDATASIZE + 1 );

    idlOS::memset( sBranchQualifier,
                   0,
                   ID_MAXXIDDATASIZE + 1 );

    sXID = (ID_XID *)aMember;

    idaXaXidToString( sXID,
                      &sFormatID, 
                      sGlobalTxId, 
                      sBranchQualifier );

    sLen = idlOS::snprintf((SChar*)aBuf, aBufSize,
                           "%"ID_vSLONG_FMT".%s.%s",
                           sXID->formatID, sGlobalTxId, sBranchQualifier);

    return sLen;
}

UInt idaXaConvertGlobalXIDToString( void        */*aBaseObj*/,
                                    void        * aMember,
                                    UChar       * aBuf,
                                    UInt          aBufSize)
{
    ID_XID *sXID;
    UInt    sLen = 0;

    vULong sFormatID;
    SChar  sGlobalTxId      [ID_MAXXIDDATASIZE + 1];
    SChar  sBranchQualifier [ID_MAXXIDDATASIZE + 1];

    idlOS::memset( sGlobalTxId,
                   0,
                   ID_MAXXIDDATASIZE + 1 );

    idlOS::memset( sBranchQualifier,
                   0,
                   ID_MAXXIDDATASIZE + 1 );

    sXID = (ID_XID *)aMember;

    idaXaXidToString( sXID,
                      &sFormatID, 
                      sGlobalTxId, 
                      sBranchQualifier );

    sLen = idlOS::snprintf((SChar*)aBuf, aBufSize,
                           "%"ID_vSLONG_FMT".%s",
                           sXID->formatID, sGlobalTxId);

    return sLen;
}

