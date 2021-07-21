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
 * $Id: acpDir.c 5910 2009-06-08 08:20:20Z djin $
 ******************************************************************************/

#include <acpError.h>
#include <acpDir.h>
#include <acpPath.h>
#include <acpCStr.h>
#include <acpPrintf.h>
#include <acpMem.h>

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

ACP_INLINE acp_rc_t acpDirOpenHandle(acp_dir_t *aDir)
{
    aDir->mHandle = FindFirstFile(aDir->mPath,
                                  &aDir->mEntry);

    if (aDir->mHandle == INVALID_HANDLE_VALUE)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpDirCloseHandle(acp_dir_t *aDir)
{
    BOOL sRet;

    sRet = FindClose(aDir->mHandle);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpDirOpen(acp_dir_t *aDir, acp_char_t *aPath)
{
    acp_path_pool_t sPool;
    acp_sint32_t    sLen;
    acp_rc_t        sRC;
    acp_sint32_t    i;
    acp_char_t*     sPath = NULL;

    /*
     * get file system representation
     */
    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);

    if (NULL == sPath)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = acpSnprintf(aDir->mPath,
                          sizeof(aDir->mPath),
                          "%s%n",
                          sPath,
                          &sLen);

        acpPathPoolFinal(&sPool);

        if (ACP_RC_IS_ETRUNC(sRC))
        {
            return ACP_RC_ENAMETOOLONG;
        }
        else
        {
            /* do nothing */
        }

        /*
         * remove trailing "\"
         */
        for (i = sLen - 1; i >= 0; i--)
        {
            if (aDir->mPath[i] == '\\')
            {
                /* continue */
            }
            else
            {
                break;
            }
        }

        sLen = i + 1;

        /*
         * append "\*"
         */
        sRC = acpSnprintf(aDir->mPath + sLen,
                          sizeof(aDir->mPath) - sLen,
                          "\\*");

        if (ACP_RC_IS_ETRUNC(sRC))
        {
            return ACP_RC_ENAMETOOLONG;
        }
        else
        {
            /* do nothing */
        }

        /*
         * open dir
         */
        aDir->mHandle = FindFirstFile(aDir->mPath,
                                      &aDir->mEntry);

        if (aDir->mHandle == INVALID_HANDLE_VALUE)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
}

ACP_EXPORT acp_rc_t acpDirClose(acp_dir_t *aDir)
{
    acp_rc_t sRC;

    sRC = acpDirCloseHandle(aDir);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpDirRead(acp_dir_t *aDir, acp_char_t **aEntryName)
{
    acp_rc_t sRC;
    BOOL     sRet = 0;

    sRet = FindNextFile(aDir->mHandle, &aDir->mEntry);

    if (sRet == 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();

        if (ACP_RC_IS_ENOENT(sRC))
        {
            return ACP_RC_EOF;
        }
        else
        {
            return sRC;
        }
    }
    else
    {
        *aEntryName = aDir->mEntry.cFileName;

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpDirRewind(acp_dir_t *aDir)
{
    acp_rc_t sRC;

    sRC = acpDirCloseHandle(aDir);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        return acpDirOpenHandle(aDir);
    }
    else
    {
        return sRC;
    }
}

ACP_EXPORT acp_rc_t acpDirMake(acp_char_t *aPath, acp_sint32_t aMode)
{
    acp_path_pool_t sPool;
    BOOL            sRet;
    acp_char_t*     sPath = NULL;
    acp_rc_t        sRC;

    ACP_UNUSED(aMode);

    if (aPath == NULL)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        acpPathPoolInit(&sPool);
        sPath = acpPathFull(aPath, &sPool);

        if(NULL == sPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sRet = CreateDirectory(sPath, NULL);
            sRC = (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpDirRemove(acp_char_t *aPath)
{
    acp_path_pool_t sPool;
    BOOL            sRet;
    acp_char_t*     sPath = NULL;
    acp_rc_t        sRC;

    if (aPath == NULL)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        acpPathPoolInit(&sPool);
        sPath = acpPathFull(aPath, &sPool);

        if(NULL == sPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sRet = RemoveDirectory(sPath);
            sRC = (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

#else

/**
 * open directory stream
 * @param aDir pointer to the directory stream object
 * @param aPath path to directory to open
 * @return result code
 */
ACP_EXPORT acp_rc_t acpDirOpen(acp_dir_t *aDir, acp_char_t *aPath)
{
    acp_path_pool_t  sPool;
    DIR             *sDir = NULL;
    const acp_char_t*     sPath;
    acp_rc_t        sRC;

    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);

    if(NULL == sPath)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sDir = opendir(sPath);

        if (sDir == NULL)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            aDir->mHandle = sDir;
#if defined(ALTI_CFG_OS_SOLARIS)
            aDir->mEntry  = (struct dirent *)aDir->mBuffer;
#else
            aDir->mEntry  = &aDir->mBuffer;
#endif

            sRC = ACP_RC_SUCCESS;
        }
    }

    acpPathPoolFinal(&sPool);

    return sRC;
}

/**
 * close directory stream
 * @param aDir pointer to the directory stream object
 * @return result code
 */
ACP_EXPORT acp_rc_t acpDirClose(acp_dir_t *aDir)
{
    acp_sint32_t sRet;

    sRet = closedir(aDir->mHandle);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aDir->mHandle = NULL;

        return ACP_RC_SUCCESS;
    }
}

/**
 * read the next directory entry in the directory stream
 *
 * @param aDir pointer to the directory stream object
 * @param aEntryName pointer to the constant string object to get the file name
 * @return result code
 *
 * The returned file name is pointer to the internal buffer
 * of directory stream object.
 * So, the content of the file name should not be modified.
 * The file name is always null terminated.
 *
 * it returns #ACP_RC_EOF if there is no more file in the directory
 */
ACP_EXPORT acp_rc_t acpDirRead(acp_dir_t *aDir, acp_char_t **aEntryName)
{
    struct dirent *sEntry = NULL;
    acp_sint32_t   sRet;

    ACP_RC_SET_OS_ERROR(ACP_RC_SUCCESS);

    sRet  = readdir_r(aDir->mHandle, aDir->mEntry, &sEntry);

    if (sRet != 0)
    {
#if defined(ALTI_CFG_OS_AIX)
        if (ACP_RC_GET_OS_ERROR() != ACP_RC_SUCCESS)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            return ACP_RC_EOF;
        }
#else
        return sRet;
#endif
    }
    else
    {
        if (sEntry == NULL)
        {
            return ACP_RC_EOF;
        }
        else
        {
            *aEntryName = sEntry->d_name;

            return ACP_RC_SUCCESS;
        }
    }
}

/**
 * resets the position of the directory stream to the beginning of the directory
 *
 * @param aDir pointer to the directory stream object
 * @return result code
 */
ACP_EXPORT acp_rc_t acpDirRewind(acp_dir_t *aDir)
{
    rewinddir(aDir->mHandle);

    return ACP_RC_SUCCESS;
}

/**
 * makes a directory
 *
 * @param aPath path to the directory to make
 * @param aMode file permission
 * @return result code
 */
ACP_EXPORT acp_rc_t acpDirMake(acp_char_t *aPath, acp_sint32_t aMode)
{
    acp_path_pool_t sPool;
    acp_sint32_t    sRet;
    acp_char_t*     sPath = NULL;
    acp_rc_t        sRC;

    if (aPath == NULL)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        acpPathPoolInit(&sPool);
        sPath = acpPathFull(aPath, &sPool);

        if(NULL == sPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sRet = mkdir(sPath, aMode);
            sRC = (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);

    }

    return sRC;
}

/**
 * removes a directory
 *
 * @param aPath path to the directory to remove
 * @return result code
 */
ACP_EXPORT acp_rc_t acpDirRemove(acp_char_t *aPath)
{
    acp_path_pool_t sPool;
    acp_sint32_t    sRet;
    acp_char_t*     sPath = NULL;
    acp_rc_t        sRC;

    if (aPath == NULL)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        acpPathPoolInit(&sPool);
        sPath = acpPathFull(aPath, &sPool);

        if(NULL == sPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sRet = rmdir(sPath);
            sRC = (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);
    }
    
    return sRC;
}

#endif


/**
 * set current working directory of calling process
 *
 * @param aPath pointer to string of path to change
 * @param aBufLen length of aPath
 * @return result code
 */
ACP_EXPORT acp_rc_t acpDirSetCwd(acp_char_t *aPath,
                                acp_size_t  aBufLen)
{
    acp_rc_t sRC;
    acp_char_t* sPath = NULL;
    acp_path_pool_t sPool;

    if(NULL == aPath)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        if(acpCStrLen(aPath, aBufLen) == aBufLen)
        {
            sRC = ACP_RC_EINVAL;
        }
        else
        {
            acpPathPoolInit(&sPool);
            sPath = acpPathFull(aPath, &sPool);

            if(NULL == sPath)
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
#if defined(ALTI_CFG_OS_WIND0WS)
                if(0 == SetCurrentDirectory(sPath))
                {
                    sRC = ACP_RC_GET_OS_ERROR();
                }
                else
                {
                    sRC = ACP_RC_SUCCESS;
                }
#else
                if(0 != chdir(sPath))
                {
                    sRC = ACP_RC_GET_OS_ERROR();
                }
                else
                {
                    sRC = ACP_RC_SUCCESS;
                }
#endif
            }

            acpPathPoolFinal(&sPool);
        }
    }

    return sRC;
}

/**
 * get current directory
 *
 * @param aPath string object which keep the current directory
 * @param aBufLen length of @a aPath buffer
 * @return result code
 */
ACP_EXPORT acp_rc_t acpDirGetCwd(acp_char_t *aPath,
                                 acp_size_t  aBufLen)
{
    acp_rc_t sRC;

#if defined(ALTI_CFG_OS_WINDOWS)
    acp_sint32_t sLen;

    if (NULL == aPath)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        sLen = GetCurrentDirectory((acp_sint32_t)aBufLen, aPath);
        if (0 == sLen)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if((acp_size_t)sLen >= aBufLen)
            {
                sRC = ACP_RC_ERANGE;
            }
            else
            {
                sRC = ACP_RC_SUCCESS;
            }
        }
    }
#else
    acp_char_t *sValue = NULL;

    if (NULL == aPath)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        sValue = getcwd(aPath, aBufLen);
        if (NULL == sValue)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sRC = ACP_RC_SUCCESS;
        }
    }
#endif

    return sRC;
}

/**
 * get home directory(folder
 *
 * @param aPath string object which keep the current directory. or pass NULL to
 * acquire the length of home path
 * @param aLen length of apath
 * @return ACP_RC_SUCCESS when success.
 * ACP_RC_ETRUNC when aLen is smaller than needed.
 */


ACP_EXPORT acp_rc_t acpDirGetHome(acp_char_t* aPath,
                                  acp_size_t aLen)
{
    acp_rc_t sRC;

#if defined(ALTI_CFG_OS_WINDOWS)
    acp_char_t sPath[MAX_PATH + 1];
    acp_sint32_t sRet;
    acp_size_t sLen;

    sRet = SHGetFolderPath(NULL,
                           CSIDL_PROFILE,
                           NULL,
                           SHGFP_TYPE_CURRENT,
                           sPath);

    if(SUCCEEDED(sRet))
    {
        if(NULL == aPath)
        {
            sRC = ACP_RC_EFAULT;
        }
        else
        {
            sLen = acpCStrLen(sPath, ACP_PATH_MAX_LENGTH);
            sRC = acpCStrCpy(aPath, aLen, sPath, sLen);
        }
    }
    else
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
#else
    struct passwd* sPwd = NULL;
    uid_t sUid = getuid();
    acp_size_t sLen;

    sRC = ACP_RC_ENOENT;

    (void)setpwent();
    while(1)
    {
        ACP_RC_SET_OS_ERROR(ACP_RC_SUCCESS);
        sPwd = getpwent();

        if(NULL == sPwd)
        {
            sRC = ACP_RC_GET_OS_ERROR();
            if(ACP_RC_IS_SUCCESS(sRC))
            {
                /* UID Not found until the end of pwd file */
                sRC = ACP_RC_ENOENT;
            }
            else
            {
                /* Do nothing */
            }
            break;
        }
        else
        {
            /* Fall through */
        }

        if(sPwd->pw_uid == sUid)
        {
            if(NULL == aPath)
            {
                sRC = ACP_RC_EFAULT;
            }
            else
            {
                sLen = acpCStrLen(sPwd->pw_dir, ACP_PATH_MAX_LENGTH);
                sRC = acpCStrCpy(aPath, aLen, sPwd->pw_dir, sLen);
            }
            break;
        }
        else
        {
            /* loop */
        }
    }

    endpwent();
#endif

    return sRC;
}
