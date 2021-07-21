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

#ifndef _O_IDV_AUDIT_H_
#define _O_IDV_AUDIT_H_ 1

#include <idl.h>
#include <idu.h>
#include <idv.h>
#include <ide.h>
#include <idp.h>
#include <idvAuditDef.h>
class idvAudit
{
/* ***********************************
 * member variables
 * *********************************** */
   /* Action Flag */
    static UInt   mBufferSize;
    static UInt   mBufferFlushSize;
    static UInt   mBufferSkipFlag;
    static UInt   mFileSize;
    static SChar *mAuditLogDir;

    /* Buffer Info */
    static UChar     *mBufferArray;
    static UInt       mHeaderOffset;
    static UInt       mHeaderOffsetForWriting;
    static UInt       mTailOffset;
    static PDL_HANDLE mFP;
    static SChar      mFileName[MAXPATHLEN];
    static UInt       mFileNumber;
    static idBool     mIsWriting; 
    static ULong      mCurrFileSize;

private:
    static iduMutex mFileMutex;
    static iduMutex mHeaderMutex;
    static iduMutex mTailMutex;

    //BUG-39760  Enable AltiAudit to write log into syslog
    static UInt     mOutputMethod; 

/* ***********************************
 * functions 
 * *********************************** */
public:
    static IDE_RC initialize();
    static IDE_RC destroy();
    static IDE_RC writeAuditEntries( idvAuditTrail *aAuditTrail,
                                     SChar         *aQuery, 
                                     UInt           aQueryLen);
    static IDE_RC flushAllBufferToFile();

    /* for properties */
    static IDE_RC setBufferSize( UInt aSize );
    static IDE_RC setBufferFlushSize( UInt aSize );
    static IDE_RC setBufferFullSkipFlag( UInt aFlag );
    static IDE_RC setFileSize( UInt aSize );
    static IDE_RC setAuditLogDir( SChar *aAuditLogDir );
    /*
     * When adding member functions, you must consider something like this.
     * if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL ) { ... }
     */

private:
    /* for mutex */
    static IDE_RC lockFileMutex()   { return mFileMutex.lock( NULL ); }
    static IDE_RC unlockFileMutex() { return mFileMutex.unlock(); }

    static IDE_RC lockHeaderMutex()   { return mHeaderMutex.lock( NULL ); }
    static IDE_RC unlockHeaderMutex() { return mHeaderMutex.unlock(); }

    static IDE_RC lockTailMutex()   { return mTailMutex.lock( NULL ); }
    static IDE_RC unlockTailMutex() { return mTailMutex.unlock(); }


    static void   lockAllMutexForProperty();
    static void   unlockAllMutexForProperty();

    static IDE_RC writeAuditEntriesInternal( idvAuditTrail  *aAuditTrail,
                                             SChar          *aQuery, 
                                             UInt            aQueryLen );
    static IDE_RC openBuffer();
    static IDE_RC closeBuffer();
    static IDE_RC copyToBuffer(idvAuditDescInfo *aInfo);
    static void   copyToBufferInternal( idvAuditDescInfo *aInfo );
    static void   copyToBufferInternalOneUnit( void *aSrc, UInt aLength );
    static UInt   getBufFree();
    static idBool isEnoughSpace( UInt aToCopyDataLen );
    static idBool isWriteNeed();
    static UInt   getBufUsed();
    static IDE_RC writeBufferToDisk();
    static IDE_RC openFile();
    static IDE_RC closeFile();
    static IDE_RC writeBufferToDiskInternal();
    static void   increaseHeader( UInt aCopiedSize );
    static void   increaseTail( UInt aWrittenSize );

};

inline IDE_RC idvAudit::writeAuditEntries( idvAuditTrail *aAuditTrail,
                                           SChar         *aQuery, 
                                           UInt           aQueryLen )
{
    return writeAuditEntriesInternal( aAuditTrail, aQuery, aQueryLen );
}


#endif
