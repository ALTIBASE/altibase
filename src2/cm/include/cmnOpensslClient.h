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

#ifndef _O_CMN_OPENSSL_CLIENT_H_
#define _O_CMN_OPENSSL_CLIENT_H_ 1

#if !defined(CM_DISABLE_SSL)

#include <cmnOpensslDefClient.h>

/* BUG-45235 */
typedef struct cmnOpenssl
{
    acp_dl_t         mSslHandle;
    acp_dl_t         mCryptoHandle;
    cmnOpensslFuncs  mFuncs;
    acp_bool_t       mLibInitialized;

    acp_thr_mutex_t *mMutex;         /* for Crypto locking */
    acp_sint32_t     mMutexCount;
} cmnOpenssl;

ACI_RC cmnOpensslInitialize(cmnOpenssl **aOpenssl);

ACI_RC cmnOpensslDestroy(cmnOpenssl **aOpenssl);

const acp_char_t *cmnOpensslErrorMessage(cmnOpenssl *aOpenssl);

#endif /* CM_DISABLE_SSL */

#endif /* _O_CMN_OPENSSL_CLIENT_H_ */
