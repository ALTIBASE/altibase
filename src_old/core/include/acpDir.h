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
 * $Id: acpDir.h 5736 2009-05-21 08:57:48Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_DIR_H_)
#define _O_ACP_DIR_H_

/**
 * @file
 * @ingroup CoreFile
 */

#include <acpStr.h>


ACP_EXTERN_C_BEGIN


/**
 * directory stream object
 */
typedef struct acp_dir_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    HANDLE           mHandle;
    WIN32_FIND_DATA  mEntry;
    acp_char_t       mPath[MAX_PATH + 1];
#else
    DIR             *mHandle;
    struct dirent   *mEntry;
#if defined(ALTI_CFG_OS_SOLARIS)
    /*
     * BUGBUG: should verify
     */
    acp_char_t       mBuffer[288]; /* 256 + 8 * 4(number of dirent members) */
#else
    struct dirent    mBuffer;
#endif
#endif
} acp_dir_t;


ACP_EXPORT acp_rc_t acpDirOpen(acp_dir_t *aDir, acp_char_t *aPath);
ACP_EXPORT acp_rc_t acpDirClose(acp_dir_t *aDir);

ACP_EXPORT acp_rc_t acpDirRead(acp_dir_t *aDir, acp_char_t **aEntryName);
ACP_EXPORT acp_rc_t acpDirRewind(acp_dir_t *aDir);

ACP_EXPORT acp_rc_t acpDirMake(acp_char_t *aPath, acp_sint32_t aMode);
ACP_EXPORT acp_rc_t acpDirRemove(acp_char_t *aPath);

ACP_EXPORT acp_rc_t acpDirSetCwd(acp_char_t *aPath, acp_size_t aBufLen);
ACP_EXPORT acp_rc_t acpDirGetCwd(acp_char_t *aPath, acp_size_t aBufLen);

ACP_EXPORT acp_rc_t acpDirGetHome(acp_char_t* aPath, acp_size_t aLen);


ACP_EXTERN_C_END


#endif
