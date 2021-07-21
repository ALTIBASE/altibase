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
 * $Id: acpDl.c 5306 2009-04-16 12:15:22Z djin $
 ******************************************************************************/

#include <acpDl.h>
#include <acpStr.h>
#include <acpPrintf.h>

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

ACP_INLINE HMODULE acpDlLoadLibrary(const acp_char_t *aPath)
{
    HMODULE sHandle;

    sHandle = LoadLibraryEx(aPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (sHandle != NULL)
    {
        return sHandle;
    }
    else
    {
        /* do nothing */
    }

    sHandle = LoadLibraryEx(aPath, NULL, 0);

    return sHandle;
}

ACP_EXPORT acp_rc_t acpDlOpen(acp_dl_t    *aDl,
                              acp_char_t  *aDir,
                              acp_char_t  *aName,
                              acp_bool_t   aIsLibrary)
{
    acp_path_pool_t sPool;
    HMODULE         sHandle;
    acp_char_t*     sFullPath = NULL;
    acp_rc_t        sRC = ACP_RC_SUCCESS;

    aDl->mError = ACP_RC_SUCCESS;

    if (aName != NULL)
    {
        acp_char_t sPath[1024];
        
        acpPathPoolInit(&sPool);
        sFullPath = acpPathFull(aDir, &sPool);

        if (NULL == sFullPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if (aIsLibrary == ACP_TRUE)
            {
                if (aDir != NULL)
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_LIB_FULL_FORMAT,
                                      sFullPath,
                                      aName);
                }
                else
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_LIB_NAME_FORMAT,
                                      aName);

                }
                sHandle = acpDlLoadLibrary(sPath);
            }
            else
            {
                if (aDir != NULL)
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_MOD_FULL_FORMAT,
                                      sFullPath,
                                      aName);
                }
                else
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_MOD_NAME_FORMAT,
                                      aName);

                }
                sHandle = acpDlLoadLibrary(sPath);
            }
        }

        acpPathPoolFinal(&sPool);

    }
    else
    {
        sHandle = LoadLibraryEx(NULL, NULL, 0);
    }

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        if (sHandle == NULL)
        {
            aDl->mError = ACP_RC_GET_OS_ERROR();

            sRC = ACP_RC_EDLERR;
        }
        else
        {
            aDl->mHandle = sHandle;
            aDl->mError  = ACP_RC_SUCCESS;
        }
    }
    else
    {
        /* Return error code */
    }
    
    return sRC;
}

ACP_EXPORT acp_rc_t acpDlClose(acp_dl_t *aDl)
{
    BOOL sRet;

    sRet = FreeLibrary(aDl->mHandle);

    if (sRet == 0)
    {
        aDl->mError = ACP_RC_GET_OS_ERROR();

        return ACP_RC_EDLERR;
    }
    else
    {
        aDl->mError = ACP_RC_SUCCESS;

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT void *acpDlSym(acp_dl_t *aDl, const acp_char_t *aSymbol)
{
    void *sProcHandle = GetProcAddress(aDl->mHandle, aSymbol);

    if (sProcHandle == NULL)
    {
        aDl->mError = ACP_RC_GET_OS_ERROR();

        return NULL;
    }
    else
    {
        aDl->mError = ACP_RC_SUCCESS;

        return sProcHandle;
    }
}

ACP_EXPORT const acp_char_t *acpDlError(acp_dl_t *aDl)
{
    if (ACP_RC_IS_SUCCESS(aDl->mError))
    {
        return NULL;
    }
    else
    {
        acpErrorString(aDl->mError, aDl->mErrorMsg, sizeof(aDl->mErrorMsg));

        return aDl->mErrorMsg;
    }
}

#else

/**
 * loads a dynamic linking module
 *
 * @param aDl the pointer to the dynamic linking module object
 * @param aDir directory path to find loadable module.
 * NULL to use default search path
 * @param aName name of the module to load
 * @param aIsLibrary ACP_TRUE if aName is the name of the shared library
 * @return result code
 *
 * it returns #ACP_RC_EDLERR when error.
 * use acpDlError() to get the error description.
 */
ACP_EXPORT acp_rc_t acpDlOpen(acp_dl_t    *aDl,
                              acp_char_t  *aDir,
                              acp_char_t  *aName,
                              acp_bool_t   aIsLibrary)
{
    acp_path_pool_t sPool;
    void*           sHandle = NULL;
    acp_char_t*     sError = NULL;
    acp_char_t*     sFullPath = NULL;
    acp_rc_t        sRC = ACP_RC_SUCCESS;

    aDl->mErrorMsg[0] = 0;

    if (aName != NULL)
    {
        acp_char_t sPath[1024];

#if defined(ALTI_CFG_OS_HPUX)
        aDl->mIsSelf = ACP_FALSE;
#endif

        acpPathPoolInit(&sPool);
        sFullPath = acpPathFull(aDir, &sPool);

        if(NULL == sFullPath)
        {
            sHandle = NULL;
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if (aIsLibrary == ACP_TRUE)
            {
                if (aDir != NULL)
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_LIB_FULL_FORMAT,
                                      sFullPath,
                                      aName);
                }
                else
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_LIB_NAME_FORMAT,
                                      aName);
                }

                sHandle = dlopen(sPath, RTLD_LAZY);
            }
            else
            {
                if (aDir != NULL)
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_MOD_FULL_FORMAT,
                                      sFullPath,
                                      aName);
                }
                else
                {
                    (void)acpSnprintf(sPath,
                                      sizeof(sPath),
                                      ACP_DL_MOD_NAME_FORMAT,
                                      aName);
                }

                sHandle = dlopen(sPath, RTLD_LAZY);
            }
        }

        acpPathPoolFinal(&sPool);

    }
    else
    {
        /*
         * BUG-30240
         * 0 is an invalid flag for opening NULL
         */
        sHandle = dlopen(NULL, RTLD_LAZY);
#if defined(ALTI_CFG_OS_HPUX)
        aDl->mIsSelf = ACP_TRUE;
#endif
    }

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        if (sHandle == NULL)
        {
            sError = dlerror();

            if (sError != NULL)
            {
                (void)acpSnprintf(aDl->mErrorMsg,
                                  sizeof(aDl->mErrorMsg),
                                  "%s",
                                  sError);
            }
            else
            {
                /* do nothing */
            }

            sRC = ACP_RC_EDLERR;
        }
        else
        {
            aDl->mHandle = sHandle;
        }
    }
    else
    {
        /* Return error code */
    }

    return sRC;
}

/**
 * decrements the reference count of the dynamic linking module
 *
 * @param aDl the pointer to the dynamic linking module object
 * @return result code
 *
 * If the reference counts drops to zero
 * and no loaded modules use symbols in it,
 * then the dynamic module is unloaded.
 *
 * it returns #ACP_RC_EDLERR when error.
 * use acpDlError() to get the error description.
 */
ACP_EXPORT acp_rc_t acpDlClose(acp_dl_t *aDl)
{
    acp_sint32_t  sRet;
    acp_char_t   *sError = NULL;

    aDl->mErrorMsg[0] = 0;

#if defined(ALTI_CFG_OS_HPUX)
    if (aDl->mIsSelf == ACP_TRUE)
    {
        /* HP cannot close the handle what opened self process */
        return ACP_RC_SUCCESS;
    }
    else
    {
        /* go ahead */
    }
#endif

    sRet = dlclose(aDl->mHandle);

    if (sRet != 0)
    {
        sError = dlerror();

        if (sError != NULL)
        {
            (void)acpSnprintf(aDl->mErrorMsg,
                              sizeof(aDl->mErrorMsg),
                              "%s",
                              sError);
        }
        else
        {
            /* do nothing */
        }

        return ACP_RC_EDLERR;
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

/**
 * gets the address where the symbol @a aSymbol is loaded into memory
 *
 * @param aDl the pointer to the dynamic linking module object
 * @param aSymbol the symbol name string to find
 * @return the address of the symbol loaded
 *
 * it returns NULL if the symbol is not found.
 * use acpDlError() to get the error description.
 */
ACP_EXPORT void *acpDlSym(acp_dl_t *aDl, const acp_char_t *aSymbol)
{
    void       *sRet = NULL;
    acp_char_t *sError = NULL;

    aDl->mErrorMsg[0] = 0;

    sRet = dlsym(aDl->mHandle, aSymbol);

    if (sRet == NULL)
    {
        sError = dlerror();

        if (sError != NULL)
        {
            (void)acpSnprintf(aDl->mErrorMsg,
                              sizeof(aDl->mErrorMsg),
                              "%s",
                              sError);
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    return sRet;
}

/**
 * gets the error string
 *
 * @param aDl the pointer to the dynamic linking module object
 * @returns error description
 *
 * it returns human readable string describing the recent error that occurred
 * from acpDlOpen(), acpDlClose(), or acpDlSym()
 * since the last call to acpDlError().
 * It returns NULL if no errors have occurred since initialization
 * or since it was last called.
 */
ACP_EXPORT const acp_char_t *acpDlError(acp_dl_t *aDl)
{
    if (aDl->mErrorMsg[0] == 0)
    {
        return NULL;
    }
    else
    {
        return aDl->mErrorMsg;
    }
}

#endif
