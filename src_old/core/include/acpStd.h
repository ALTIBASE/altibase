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
 * $Id: acpStd.h 10912 2010-04-23 01:44:15Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_STD_H_)
#define _O_ACP_STD_H_

/**
 * @file
 * @ingroup CoreFile
 */

#include <acpFile.h>
#include <acpStr.h>


ACP_EXTERN_C_BEGIN

/**
 * File open flags
 */
#define ACP_STD_OPEN_READ             "rb"
#define ACP_STD_OPEN_APPEND           "ab"
#define ACP_STD_OPEN_READWRITE        "r+b"
#define ACP_STD_OPEN_READAPPEND       "a+b"
#define ACP_STD_OPEN_WRITE_TRUNC      "wb"
#define ACP_STD_OPEN_READWRITE_TRUNC  "w+b"

#define ACP_STD_OPEN_READ_BINARY            ACP_STD_OPEN_READ
#define ACP_STD_OPEN_APPEND_BINARY          ACP_STD_OPEN_APPEND
#define ACP_STD_OPEN_READWRITE_BINARY       ACP_STD_OPEN_READWRITE
#define ACP_STD_OPEN_READAPPEND_BINARY      ACP_STD_OPEN_READAPPEND
#define ACP_STD_OPEN_WRITE_TRUNC_BINARY     ACP_STD_OPEN_WRITE_TRUNC
#define ACP_STD_OPEN_READWRITE_TRUNC_BINARY ACP_STD_OPEN_READWRITE_TRUNC

#define ACP_STD_OPEN_READ_TEXT             "r"
#define ACP_STD_OPEN_APPEND_TEXT           "a"
#define ACP_STD_OPEN_READWRITE_TEXT        "r+"
#define ACP_STD_OPEN_READAPPEND_TEXT       "a+"
#define ACP_STD_OPEN_WRITE_TRUNC_TEXT      "w"
#define ACP_STD_OPEN_READWRITE_TRUNC_TEXT  "w+"

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

/**
 * standard buffered file stream object
 */
typedef struct acp_std_file_t
{
    acp_file_t      mHandle;    /* File pointer */
    acp_rc_t        mError;     /* Last error */
    acp_sint32_t    mUngetc;    /* Used for ungetc */
    acp_bool_t      mIsUngetc;  /* Used for ungetc */
} acp_std_file_t;

#else

/**
 * standard buffered file stream object
 */
typedef struct acp_std_file_t
{
    FILE* mFP;  /* FILE pointer */
} acp_std_file_t;

#endif

#define ACP_STD_IN  acpStdGetStdin()
#define ACP_STD_OUT acpStdGetStdout()
#define ACP_STD_ERR acpStdGetStderr()

ACP_EXPORT acp_std_file_t* acpStdGetStdin(void);
ACP_EXPORT acp_std_file_t* acpStdGetStdout(void);
ACP_EXPORT acp_std_file_t* acpStdGetStderr(void);

/**
 * File Seek Position
 */
#define ACP_STD_SEEKCUR ACP_SEEK_CUR
#define ACP_STD_SEEKSET ACP_SEEK_SET
#define ACP_STD_SEEKEND ACP_SEEK_END

ACP_EXPORT void acpStdClearError(acp_std_file_t*        aFile);
ACP_EXPORT acp_rc_t acpStdOpen(acp_std_file_t*          aFile,
                               const acp_char_t*        aPath,
                               const acp_char_t*        aMode);
ACP_EXPORT acp_rc_t acpStdClose(acp_std_file_t*         aFile);
ACP_EXPORT acp_rc_t acpStdFlush(acp_std_file_t*         aFile);
ACP_EXPORT acp_rc_t acpStdIsEOF(acp_std_file_t*         aFile,
                                acp_bool_t*             aIsEOF);
ACP_EXPORT acp_bool_t acpStdIsTTY(acp_std_file_t*       aFile);
ACP_EXPORT acp_rc_t acpStdGetError(acp_std_file_t*      aFile);
ACP_EXPORT acp_rc_t acpStdTell(acp_std_file_t*          aFile,
                               acp_offset_t*            aPos);
ACP_EXPORT acp_rc_t acpStdSeek(acp_std_file_t*          aFile,
                               acp_offset_t             aPosition,
                               acp_sint32_t             aWhence,
                               acp_offset_t*            aAbsPosition);
ACP_EXPORT acp_rc_t acpStdGetByte(acp_std_file_t*       aFile,
                                  acp_byte_t*           aByte);
ACP_EXPORT acp_rc_t acpStdUngetByte(acp_std_file_t*     aFile,
                                    acp_byte_t          aByte);
ACP_EXPORT acp_rc_t acpStdPutByte(acp_std_file_t*       aFile,
                                  acp_byte_t            aByte);
ACP_EXPORT acp_rc_t acpStdGetChar(acp_std_file_t*       aFile,
                                  acp_char_t*           aChar);
ACP_EXPORT acp_rc_t acpStdPutChar(acp_std_file_t*       aFile,
                                  acp_char_t            aChar);
ACP_EXPORT acp_rc_t acpStdGetString(acp_std_file_t*     aFile,
                                    acp_str_t*          aString);
ACP_EXPORT acp_rc_t acpStdRead(acp_std_file_t*          aFile,
                               void*                    aBuf,
                               acp_size_t               aSizeRecord,
                               acp_size_t               aNoRecords,
                               acp_size_t*              aRecordsRead);
ACP_EXPORT acp_rc_t acpStdWrite(acp_std_file_t*         aFile,
                                const void*             aBuf,
                                acp_size_t              aSizeRecord,
                                acp_size_t              aNoRecords,
                                acp_size_t*             aRecordsWritten);
ACP_EXPORT acp_rc_t acpStdGetCString(acp_std_file_t*    aFile,
                                     acp_char_t*        aStr,
                                     acp_size_t         aMaxLen);
ACP_EXPORT acp_rc_t acpStdPutCString(acp_std_file_t*    aFile,
                                     acp_char_t*        aStr,
                                     acp_size_t         aMaxLen,
                                     acp_size_t*        aWritten);
ACP_EXPORT acp_bool_t acpStdDupStderr(acp_std_file_t    aFile);

ACP_EXTERN_C_END

#endif
