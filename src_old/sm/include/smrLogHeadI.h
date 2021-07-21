/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 *
 * Description :
 *
 * smrLogHead 자료구조에 대한 인터페이스 클래스 정의
 *
 *
 **********************************************************************/

#ifndef _O_SMR_LOGHEADI_H_
#define _O_SMR_LOGHEADI_H_ 1

#include <smrDef.h>

class smrLogHeadI
{
  public:
    inline static smrLogType getType( smrLogHead  * aLogHead )
    {
        return aLogHead->mType;
    }

    inline static smTID getTransID( smrLogHead  * aLogHead )
    {
        return aLogHead->mTransID;
    }

    inline static smLSN getLSN( smrLogHead  * aLogHead )
    {
        return aLogHead->mLSN;
    }

    inline static UInt getSize( smrLogHead  * aLogHead )
    {
        return aLogHead->mSize;
    }

    /* BUG-35392
     * mFlag의 크기가 UChar -> UInt로 변경 */
    inline static UInt getFlag( smrLogHead  * aLogHead )
    {
        return aLogHead->mFlag;
    }

    inline static smLSN getPrevLSN( smrLogHead  * aLogHead )
    {
        return aLogHead->mPrevUndoLSN;
    }

    inline static UInt getPrevLSNFileNo( smrLogHead  * aLogHead )
    {
        return aLogHead->mPrevUndoLSN.mFileNo;
    }

    inline static UInt getPrevLSNOffset( smrLogHead  * aLogHead )
    {
        return aLogHead->mPrevUndoLSN.mOffset;
    }

    inline static smMagic getMagic( smrLogHead  * aLogHead )
    {
        return aLogHead->mMagic;
    }

    inline static void set( smrLogHead *aLogHead,
                            smTID       aTID,
                            smrLogType  aType,
                            UInt        aSize,
                            UInt        aFlag )
    {
        setTransID( aLogHead, aTID );
        setType( aLogHead, aType );
        setSize( aLogHead, aSize );
        setFlag( aLogHead, aFlag );
    }

    inline static void setType( smrLogHead *aLogHead, smrLogType aType )
    {
        aLogHead->mType = aType;
    }

    inline static void setTransID( smrLogHead  * aLogHead, smTID aID )
    {
        aLogHead->mTransID = aID;
    }

    inline static void setLSN( smrLogHead  * aLogHead, smLSN aLSN )
    {
        aLogHead->mLSN = aLSN;
    }

    inline static void setSize( smrLogHead  * aLogHead, UInt aSize )
    {
        aLogHead->mSize = aSize;
    }

    inline static void setFlag( smrLogHead  * aLogHead, UInt aFlag ) 
    {
        aLogHead->mFlag = aFlag;
    }

    inline static void setPrevLSN( smrLogHead  * aLogHead, smLSN aLSN )
    {
        aLogHead->mPrevUndoLSN = aLSN;
    }

    inline static void setPrevLSN( smrLogHead  * aLogHead,
                                   UInt          aFileNo,
                                   UInt          aOffset )
    {
        SM_SET_LSN(aLogHead->mPrevUndoLSN, aFileNo, aOffset);
    }

    inline static void setMagic( smrLogHead  * aLogHead, smMagic aMagic )
    {
        aLogHead->mMagic = aMagic;
    }

    inline static void copyTail( SChar *aTargetPtr, smrLogHead *aSource )
    {
        UChar   * sPtr = (UChar *)(&aSource->mType);

        IDE_DASSERT(aTargetPtr  != NULL);
        IDE_DASSERT(aSource     != NULL);

        IDE_DASSERT(ID_SIZEOF(aSource->mType) == 1);

        aTargetPtr[0] = sPtr[0];
    }

    inline static void setReplStmtDepth( smrLogHead  * aLogHead, UInt aStmtDepth )
    {
        aLogHead->mReplSvPNumber = (UChar)aStmtDepth;
    }

    inline static UInt getReplStmtDepth( smrLogHead  * aLogHead )
    {
        return (UInt)(aLogHead->mReplSvPNumber);
    }

    /* BUG-35392 */
    inline static idBool isDummyLog( void   * aLogPtr )
    {
        UInt    sLogFlag    = 0;

        /* smrLogHead의 mFlag 는 UInt 이므로 memcpy로 읽어야
         * align 문제를 회피 할 수 있다. */
        idlOS::memcpy( &sLogFlag,
                       aLogPtr,
                       ID_SIZEOF( UInt ) );

        if ( ( sLogFlag & SMR_LOG_DUMMY_LOG_MASK ) == SMR_LOG_DUMMY_LOG_OK )
        {
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }
};

#endif /* _O_SMR_LOGHEADI_H_ */

