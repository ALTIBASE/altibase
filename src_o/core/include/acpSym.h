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
 * $Id: acpSym.h 9659 2010-01-20 05:16:20Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_SYM_H_)
#define _O_ACP_SYM_H_

/**
 * @file
 */

#include <acpStr.h>


ACP_EXTERN_C_BEGIN


#define ACP_SYM_NAME_LEN_MAX 64


/**
 * symbol table object
 * Only for internal, DO NOT use inside fields directly.
 */
typedef struct acp_sym_table_t
{
#if defined(ALTI_CFG_OS_AIX)
    acp_char_t mObjInfoBuffer[4096];
#elif defined(ALTI_CFG_OS_HPUX) ||               \
    defined(ALTI_CFG_OS_LINUX) ||                \
    defined(ALTI_CFG_OS_FREEBSD) ||              \
    defined(ALTI_CFG_OS_SOLARIS)
    acp_char_t mSymNameBuffer[64];
#elif defined(ALTI_CFG_OS_WINDOWS)
    IMAGEHLP_MODULE64 mModule;
    acp_char_t        mSymbolBuffer[sizeof(SYMBOL_INFO) + ACP_SYM_NAME_LEN_MAX];
#else
    acp_sint32_t mDummy;
#endif
} acp_sym_table_t;

/**
 * symbol information
 */
typedef struct acp_sym_info_t
{
    acp_str_t  mFileName; /**< module/object/file name      */
    acp_str_t  mFuncName; /**< function name                */
    void      *mFileAddr; /**< base address of the object   */
    void      *mFuncAddr; /**< base address of the function */
} acp_sym_info_t;

/** 
 * initialize a symbol information object
 * 
 * @param aSymInfo pointer to the object
 */
ACP_INLINE void acpSymInfoInit(acp_sym_info_t *aSymInfo)
{
    ACP_STR_INIT_CONST(aSymInfo->mFileName);
    ACP_STR_INIT_CONST(aSymInfo->mFuncName);

    aSymInfo->mFileAddr = NULL;
    aSymInfo->mFuncAddr = NULL;
}

/** 
 * finalize a symbol information object
 * 
 * @param aSymInfo pointer to the object
 */
ACP_INLINE void acpSymInfoFinal(acp_sym_info_t *aSymInfo)
{
    ACP_STR_FINAL(aSymInfo->mFileName);
    ACP_STR_FINAL(aSymInfo->mFuncName);
}


ACP_EXPORT acp_rc_t acpSymTableInit(acp_sym_table_t *aSymTable);
ACP_EXPORT acp_rc_t acpSymTableFinal(acp_sym_table_t *aSymTable);

ACP_EXPORT acp_rc_t acpSymFind(acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr);


ACP_EXTERN_C_END


#endif
