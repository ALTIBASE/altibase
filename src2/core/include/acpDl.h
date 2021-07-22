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
 * $Id: acpDl.h 5417 2009-04-27 10:05:00Z jykim $
 ******************************************************************************/

#if !defined(_O_ACP_DL_H_)
#define _O_ACP_DL_H_

/**
 * @file
 * @ingroup CoreDl
 */

#include <acpStr.h>


ACP_EXTERN_C_BEGIN


/**
 * object for dynamic linking module
 */
typedef struct acp_dl_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    HMODULE    mHandle;
    acp_rc_t   mError;
#else
    void      *mHandle;
# if defined(ALTI_CFG_OS_HPUX)
    /* Only for HP, indicating self-process is opened. */
    acp_bool_t mIsSelf;
# endif
#endif

    acp_char_t mErrorMsg[1024];
} acp_dl_t;

ACP_EXPORT acp_rc_t          acpDlOpen(acp_dl_t    *aDl,
                                       acp_char_t  *aDir,
                                       acp_char_t  *aName,
                                       acp_bool_t   aIsLibrary);
ACP_EXPORT acp_rc_t          acpDlClose(acp_dl_t *aDl);

ACP_EXPORT void             *acpDlSym(acp_dl_t *aDl, const acp_char_t *aSymbol);
ACP_EXPORT const acp_char_t *acpDlError(acp_dl_t *aDl);


ACP_EXTERN_C_END


#endif
