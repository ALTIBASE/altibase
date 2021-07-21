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

#ifndef _O_CMN_OPENSSL_H_
#define _O_CMN_OPENSSL_H_ 1

#if !defined(CM_DISABLE_SSL)

#include <cmnOpensslDef.h>

class cmnOpenssl
{
public:
    static cmnOpensslFuncs  mFuncs;
    static PDL_SHLIB_HANDLE mSslHandle;
    static PDL_SHLIB_HANDLE mCryptoHandle;
    static idBool           mLibInitialized;

    /* BUG-45235 */
    static PDL_thread_mutex_t *mMutex;           /* for Crypto locking */
    static SInt                mMutexCount;

public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static cmnOpensslFuncs *getOpensslFuncs()
    {
        return &mFuncs;
    }

    static const SChar *getSslErrorMessage()
    {
        return mFuncs.ERR_error_string(mFuncs.ERR_get_error(), NULL);
    }

    static const SChar *getSslCipherName(const SSL *aSslHandle)
    {
        return mFuncs.SSL_CIPHER_get_name(mFuncs.SSL_get_current_cipher(aSslHandle));
    }

    /* BUG-45235 */
    static ULong callbackCryptoThreadId();
    static void callbackCryptoLocking(SInt aMode, SInt aType, const SChar *aFile, SInt aLine);

}; /* end of class */

#endif /* CM_DISABLE_SSL */

#endif /* _O_CMN_OPENSSL_H_ */
