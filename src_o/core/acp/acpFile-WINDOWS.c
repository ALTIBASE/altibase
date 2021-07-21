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
 * $Id: acpFile.c 5401 2009-04-27 06:01:47Z djin $
 ******************************************************************************/

#include <acpFile.h>
#include <acpMem.h>
#include <acpOS.h>
#include <acpTimePrivate.h>
#include <acpTest.h>

ACP_INLINE void acpFileStatToAcpStat(WIN32_FILE_ATTRIBUTE_DATA *aStat,
                                     acp_stat_t                *aAcpStat)
{

    /*
     * Set File Permission
     */
    aAcpStat->mPermission |= (ACP_S_IRUSR | ACP_S_IRGRP | ACP_S_IROTH);

    if ((aStat->dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0)
    {
        aAcpStat->mPermission |= (ACP_S_IWUSR | ACP_S_IWGRP | ACP_S_IWOTH);
    }
    else
    {
        /* do nothing */
    }

    /*
     * Set File Type
     */
    if ((aStat->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
    {
        aAcpStat->mType = ACP_FILE_LNK;
    }
    else if ((aStat->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        aAcpStat->mType = ACP_FILE_DIR;
    }
    else if ((aStat->dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0)
    {
        aAcpStat->mType = ACP_FILE_CHR;
    }
    else
    {
        aAcpStat->mType = ACP_FILE_REG;
    }

    /*
     * Set File Size
     */
    aAcpStat->mSize = (acp_offset_t)aStat->nFileSizeHigh << 32 |
        aStat->nFileSizeLow;

    /*
     * Set File Time
     */
    aAcpStat->mAccessTime = acpTimeFromAbsFileTime(&aStat->ftLastAccessTime);
    aAcpStat->mModifyTime = acpTimeFromAbsFileTime(&aStat->ftCreationTime);
    aAcpStat->mChangeTime = acpTimeFromAbsFileTime(&aStat->ftLastWriteTime);
}


static acp_rc_t acpFileOpenInternal(acp_file_t*             aFile,
                                    acp_char_t*             aPath,
                                    acp_sint32_t            aFlag,
                                    SECURITY_DESCRIPTOR*    aSD)
{
    acp_path_pool_t sPool;
    DWORD           sAccessFlag = 0;
    DWORD           sShareFlag  = 0;
    DWORD           sCreatFlag  = 0;
    DWORD           sAttrFlag   = 0;
    HANDLE          sHandle;
    acp_char_t*     sPath = NULL;
    acp_rc_t        sRC;

    if (aPath == NULL)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Set sAccessFlag
     */
    if ((aFlag & ACP_O_WRONLY) != 0)
    {
        sAccessFlag = GENERIC_WRITE;
    }
    else if ((aFlag & ACP_O_RDWR) != 0)
    {
        sAccessFlag = GENERIC_READ | GENERIC_WRITE;
    }
    else
    {
        sAccessFlag = GENERIC_READ;
    }

    /*
     * Set sShareFlag
     */
    sShareFlag = FILE_SHARE_READ | FILE_SHARE_WRITE;

    if (acpOSGetVersion() >= ACP_OS_WIN_NT)
    {
        sShareFlag |= FILE_SHARE_DELETE;
    }
    else
    {
        /* do nothing */
    }

    /*
     * Set sCreatFlag
     */
    if ((aFlag & ACP_O_CREAT) != 0)
    {
        if ((aFlag & ACP_O_EXCL) != 0)
        {
            /*
             * only create new if file does not already exist
             */
            sCreatFlag = CREATE_NEW;
        }
        else if ((aFlag & ACP_O_TRUNC) != 0)
        {
            /*
             * truncate existing file or create new
             */
            sCreatFlag = CREATE_ALWAYS;
        }
        else
        {
            /*
             * open existing but create if necessary
             */
            sCreatFlag = OPEN_ALWAYS;
        }
    }
    else
    {
        if ((aFlag & ACP_O_EXCL) != 0)
        {
            return ACP_RC_EACCES;
        }
        else if ((aFlag & ACP_O_TRUNC) != 0)
        {
            /*
             * only truncate if file already exists
             */
            sCreatFlag = TRUNCATE_EXISTING;
        }
        else
        {
            /*
             * only open if file already exists
             */
            sCreatFlag = OPEN_EXISTING;
        }
    }

    /*
     * Set sAttrFlag
     */
    if ((aFlag & ACP_O_SYNC) != 0)
    {
        sAttrFlag |= FILE_FLAG_WRITE_THROUGH;
    }
    else
    {
        /* do nothing */
    }

    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);
    if (NULL == sPath)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        SECURITY_ATTRIBUTES sSA;
        sSA.nLength = sizeof(SECURITY_ATTRIBUTES);
        sSA.lpSecurityDescriptor = aSD;
        sSA.bInheritHandle = TRUE;

        sHandle = CreateFile(aPath,
                             sAccessFlag,
                             sShareFlag,
                             &sSA,
                             sCreatFlag,
                             sAttrFlag,
                             0);

        acpPathPoolFinal(&sPool);

        if (sHandle == INVALID_HANDLE_VALUE)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            aFile->mHandle = sHandle;
            aFile->mAppendMode =
                ((aFlag & ACP_O_APPEND) != 0)?
                ACP_TRUE : ACP_FALSE;

            sRC = ACP_RC_SUCCESS;
        }
    }

    return sRC;
}

static acp_rc_t acpFileMakePermissionEntry(
    PSID                        aSIDs[3],
    EXPLICIT_ACCESS             aEA[3],
    acp_mode_t                  aMode)
{
    acp_uint32_t sPerm[3] = 
    {
        GENERIC_ALL,    /* All permission for Administrator(root group) */
        0,              /* Owner Permission */
        0               /* Other Permission */
    };
    SID_IDENTIFIER_AUTHORITY sAuth[3] =
    {
        SECURITY_NT_AUTHORITY,
        SECURITY_CREATOR_SID_AUTHORITY,
        SECURITY_WORLD_SID_AUTHORITY
    };
    DWORD sAuthority0[3] =
    {
        SECURITY_BUILTIN_DOMAIN_RID,
        SECURITY_CREATOR_OWNER_RID,
        SECURITY_WORLD_RID,
    };
    DWORD sAuthority1[3] =
    {
        DOMAIN_ALIAS_RID_ADMINS,
        0,
        0
    };
    BYTE sCount[3] =
    {
        2,
        1,
        1
    };

    acp_rc_t        sRC;
    acp_sint32_t    i;

    /* Creator and owner permission */
    if(ACP_S_IRWXU == (aMode & ACP_S_IRWXU))
    {
        /* Enable all permission */
        sPerm[1] = GENERIC_ALL;
    }
    else
    {
        /* Creator and owner must have Delete permission */
        sPerm[1] = DELETE;

        if(0 != (aMode & ACP_S_IRUSR))
        {
            sPerm[1] |= GENERIC_READ;
        }
        else
        {
            /* Do nothing */
        }

        if(0 != (aMode & ACP_S_IWUSR))
        {
            sPerm[1] |= GENERIC_WRITE;
        }
        else
        {
            /* Do nothing */
        }

        if(0 != (aMode & ACP_S_IXUSR))
        {
            sPerm[1] |= GENERIC_EXECUTE;
        }
        else
        {
            /* Do nothing */
        }
    }

    /* Not using GENERIC_ALL Permission to Other
     * not to allow DELETE permission */
    if(0 != (aMode & ACP_S_IROTH))
    {
        sPerm[2] |= GENERIC_READ;
    }
    else
    {
        /* Do nothing */
    }

    if(0 != (aMode & ACP_S_IWOTH))
    {
        sPerm[2] |= GENERIC_WRITE;
    }
    else
    {
        /* Do nothing */
    }

    if(0 != (aMode & ACP_S_IXOTH))
    {
        sPerm[2] |= GENERIC_EXECUTE;
    }
    else
    {
        /* Do nothing */
    }

    /* Initialize an EXPLICIT_ACCESS structure for an ACE.
     * The ACE will allow Other read access to the file. */
    acpMemSet(aEA, 0, sizeof(EXPLICIT_ACCESS) * 3);

    /* Create SID and assign it to Explicit Access structure */
    for(i = 0; i < 3; i++)
    {
        ACP_TEST_RAISE(0 == AllocateAndInitializeSid(
            &(sAuth[i]),
            sCount[i],
            sAuthority0[i], sAuthority1[i],
            0, 0, 0, 0, 0, 0,
            &(aSIDs[i])
            ), ALLOC_FAILED);

        aEA[i].grfAccessPermissions     = sPerm[i];
        aEA[i].grfAccessMode            = SET_ACCESS;
        aEA[i].grfInheritance           = NO_INHERITANCE;
        aEA[i].Trustee.TrusteeForm      = TRUSTEE_IS_SID;
        aEA[i].Trustee.TrusteeType      = TRUSTEE_IS_GROUP;
        aEA[i].Trustee.ptstrName        = (LPTSTR)aSIDs[i];

    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ALLOC_FAILED);
    {
        /* No error handling needed */
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;
    return sRC;
}

static acp_rc_t acpFileMakeAccessControlList(
    PSID                    aSIDs[3],
    PACL*                   aACL,
    PSECURITY_DESCRIPTOR*   aSD,
    EXPLICIT_ACCESS         aEA[3])
{
    acp_rc_t            sRC;

    /* Create a new ACL that contains the new ACEs. */
    ACP_TEST_RAISE(ERROR_SUCCESS != SetEntriesInAcl(3, aEA, NULL, aACL), NEED_RC);

    ACP_TEST_RAISE(
        ACP_RC_NOT_SUCCESS(sRC = acpMemCalloc(aSD, SECURITY_DESCRIPTOR_MIN_LENGTH, 1)),
        HAVE_RC
        );
    ACP_TEST_RAISE(
        0 == InitializeSecurityDescriptor(*aSD, SECURITY_DESCRIPTOR_REVISION),
        NEED_RC
        );
    ACP_TEST_RAISE(
        0 == SetSecurityDescriptorDacl(*aSD, TRUE, *aACL, FALSE),
        NEED_RC
        );

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(NEED_RC);
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(HAVE_RC);
    {
        /* We got error code 
         * Nothing more to do */
    }

    ACP_EXCEPTION_END;
    return sRC;
}

static void acpFileCleanupAllocated(
        PSID                        aSIDs[3],
        PACL                        aACL,
        PSECURITY_DESCRIPTOR        aSD
        )
{
    acp_sint32_t i;

    for(i = 0; i < 3; i++)
    {
        if(NULL != aSIDs[i])
        {
            FreeSid(aSIDs[i]);
        }
        else
        {
            /* Do nothing */
        }
    }

    if(NULL != aACL)
    {
        LocalFree(aACL);
    }
    else
    {
        /* Do nothing */
    }

    if(NULL != aSD)
    {
        acpMemFree(aSD);
    }
    else
    {
        /* Do nothing */
    }
}

ACP_EXPORT acp_rc_t acpFileOpen(acp_file_t*     aFile,
                                acp_char_t*     aPath,
                                acp_sint32_t    aFlag,
                                acp_mode_t      aMode)
{
    PSID                        sSIDs[3] = {NULL, };
    PACL                        sACL = NULL;
    PSECURITY_DESCRIPTOR        sSD = NULL;
    EXPLICIT_ACCESS             sEA[3];

    acp_rc_t                    sRC;

    if(0 == aMode)
    {
        /* No permission information.
         * Preserve current. */
        return acpFileOpenInternal(aFile, aPath, aFlag, NULL);
    }
    else
    {
        sRC = acpFileMakePermissionEntry(sSIDs, sEA, aMode);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CLEANUP_AND_RETURN);

        sRC = acpFileMakeAccessControlList(sSIDs, &sACL, &sSD, sEA);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CLEANUP_AND_RETURN);

        sRC = acpFileOpenInternal(aFile, aPath, aFlag, sSD);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CLEANUP_AND_RETURN);

        /* Cleanup and return success */
        acpFileCleanupAllocated(sSIDs, sACL, sSD);
        return ACP_RC_SUCCESS;

        ACP_EXCEPTION(CLEANUP_AND_RETURN);
        {
            /* Cleanup Allocated memory */
            acpFileCleanupAllocated(sSIDs, sACL, sSD);
        }
        
        ACP_EXCEPTION_END;
        return sRC;
    }
}

ACP_EXPORT acp_rc_t acpFileLock(acp_file_t *aFile)
{
    OVERLAPPED sOffset;
    DWORD      sLen = 0xffffffff;
    BOOL       sRet;
    acp_rc_t   sRC;

    if (acpOSGetVersion() >= ACP_OS_WIN_NT)
    {
        acpMemSet(&sOffset, 0, sizeof(sOffset));

        sRet = LockFileEx(aFile->mHandle,
                          LOCKFILE_EXCLUSIVE_LOCK,
                          0,
                          sLen,
                          sLen,
                          &sOffset);

        if (sRet == 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
    else
    {
        while (LockFile(aFile->mHandle, 0, 0, sLen, 0) == 0)
        {
            sRC = ACP_RC_GET_OS_ERROR();

            if (sRC == (ACP_RR_WIN + ERROR_LOCK_VIOLATION))
            {
                Sleep(500);
            }
            else
            {
                return sRC;
            }
        }

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpFileTryLock(acp_file_t *aFile)
{
    acp_rc_t sRC;
    DWORD    sLen = 0xffffffff;
    BOOL     sRet;

    if (acpOSGetVersion() >= ACP_OS_WIN_NT)
    {
        sRet = LockFile(aFile->mHandle, 0, 0, sLen, sLen);
    }
    else
    {
        sRet = LockFile(aFile->mHandle, 0, 0, sLen, 0);
    }

    if (sRet == 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();

        if (sRC == (ACP_RR_WIN + ERROR_LOCK_VIOLATION))
        {
            return ACP_RC_EBUSY;
        }
        else
        {
            return sRC;
        }
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpFileUnlock(acp_file_t *aFile)
{
    DWORD sLen = 0xffffffff;
    BOOL  sRet;

    if (acpOSGetVersion() >= ACP_OS_WIN_NT)
    {
        sRet = UnlockFile(aFile->mHandle, 0, 0, sLen, sLen);
    }
    else
    {
        sRet = UnlockFile(aFile->mHandle, 0, 0, sLen, 0);
    }

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpFileTruncate(acp_file_t *aFile, acp_offset_t aSize)
{
    acp_offset_t sCurPos;
    acp_rc_t     sRC;
    BOOL         sRet;

    /*
     * get the current file position
     */
    sRC = acpFileSeek(aFile, 0, ACP_SEEK_CUR, &sCurPos);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * set the file position to truncate
     */
    sRC = acpFileSeek(aFile, aSize, ACP_SEEK_SET, NULL);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpFileSeek(aFile, sCurPos, ACP_SEEK_SET, NULL);
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * set the physical file size to the current file position
     */
    sRet = SetEndOfFile(aFile->mHandle);

    if (sRet == 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    /*
     * restore the current file position
     */
    (void)acpFileSeek(aFile, sCurPos, ACP_SEEK_SET, NULL);

    return sRC;
}

ACP_EXPORT acp_rc_t acpFileTruncateAtPath(acp_char_t *aPath, acp_offset_t aSize)
{
    acp_file_t sFile;
    acp_rc_t   sRC;

    if (aPath == NULL)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* do nothing */
    }

    /*
     * The file should be exist
     */
    sRC = acpFileOpen(&sFile, aPath, ACP_O_RDWR, 0);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        sRC = acpFileTruncate(&sFile, aSize);

        (void)acpFileClose(&sFile);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* do nothing */
        }

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpFileStat(acp_file_t *aFile, acp_stat_t *aStat)
{
    BY_HANDLE_FILE_INFORMATION sFileInfo;
    WIN32_FILE_ATTRIBUTE_DATA  sAttrData;
    DWORD                      sFileType;
    BOOL                       sRet;

    sRet = GetFileInformationByHandle(aFile->mHandle,
                                      &sFileInfo);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sAttrData.dwFileAttributes = sFileInfo.dwFileAttributes;
        sAttrData.ftCreationTime   = sFileInfo.ftCreationTime;
        sAttrData.ftLastAccessTime = sFileInfo.ftLastAccessTime;
        sAttrData.ftLastWriteTime  = sFileInfo.ftLastWriteTime;
        sAttrData.nFileSizeHigh    = sFileInfo.nFileSizeHigh;
        sAttrData.nFileSizeLow     = sFileInfo.nFileSizeLow;

        acpFileStatToAcpStat(&sAttrData, aStat);
    }

    if ((sFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        sFileType = GetFileType(aFile->mHandle);

        if (sFileType != NO_ERROR)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if (aStat->mType == ACP_FILE_REG)
            {
                if (sFileType == FILE_TYPE_PIPE)
                {
                    aStat->mType = ACP_FILE_FIFO;
                }
                else if (sFileType == FILE_TYPE_CHAR)
                {
                    aStat->mType = ACP_FILE_CHR;
                }
                else
                {
                    /* do nothing */
                }
            }
            else
            {
                /* otherwise leave the original conclusion  */
            }
        }
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpFileStatAtPath(acp_char_t  *aPath,
                                      acp_stat_t  *aStat,
                                      acp_bool_t   aTraverseLink)
{
    WIN32_FILE_ATTRIBUTE_DATA  sAttrData;
    BOOL                       sRet;
    DWORD                      sFileType;
    acp_path_pool_t            sPool;
    acp_file_t                 sFile;
    acp_rc_t                   sRC;
    acp_char_t*                sPath = NULL;

    if (aPath == NULL)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* do nothing */
    }

    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);

    if(NULL == sPath)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRet = GetFileAttributesEx(sPath,
                                   GetFileExInfoStandard,
                                   &sAttrData);


        if (sRet == 0)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /*
             * Get File Attribute (by checking symbolic link or origin file)
             */
            if ((sAttrData.dwFileAttributes == FILE_ATTRIBUTE_REPARSE_POINT) &&
                (aTraverseLink              == ACP_TRUE))
            {
                /*
                 * Get information of target file which  is indicated
                 * by symblic link
                 */
            }
            else
            {
                /* do nothing */
            }

            acpFileStatToAcpStat(&sAttrData, aStat);

            if ((sAttrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                sRC = acpFileOpen(&sFile, aPath, ACP_O_RDONLY, 0);

                if (ACP_RC_NOT_SUCCESS(sRC))
                {
                    /* Return Error code */
                }
                else
                {
                    sFileType = GetFileType(sFile.mHandle);

                    (void)acpFileClose(&sFile);

                    if (sFileType != NO_ERROR)
                    {
                        sRC = ACP_RC_GET_OS_ERROR();
                    }
                    else
                    {
                        sRC = ACP_RC_SUCCESS;

                        if (aStat->mType == ACP_FILE_REG)
                        {
                            switch (sFileType)
                            {
                            case FILE_TYPE_PIPE:
                                aStat->mType = ACP_FILE_FIFO;
                                break;
                            case FILE_TYPE_CHAR:
                                aStat->mType = ACP_FILE_CHR;
                                break;
                            default:
                                /* Do nothing */
                                break;
                            }
                        }
                        else
                        {
                            /* Otherwise leave the original conclusion  */
                        }
                    }
                }
            }
            else
            {
                sRC = ACP_RC_SUCCESS;
            }
        }
    }

    acpPathPoolFinal(&sPool);
    return sRC;
}

ACP_EXPORT acp_rc_t acpFileGetKey(
    acp_char_t*  aPath,
    acp_sint32_t aProj,
    acp_key_t*   aKey)
{
    acp_file_t        sFile;
    acp_rc_t          sRC;
    acp_sint32_t      sKey;
    acp_char_t*       sPath = NULL;
    acp_path_pool_t   sPool;

    BOOL         sResult;

    BY_HANDLE_FILE_INFORMATION sInfo;


    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);
    ACP_TEST_RAISE(NULL == sPath, RESOLVE_FAILED);
    
    /* project id must be specified */
    ACP_TEST_RAISE(aProj == 0, PROJID_FAILED);

    sRC = acpFileOpen(&sFile, sPath, ACP_O_RDONLY, 0);
    if(ACP_RC_IS_EACCES(sRC) || ACP_RC_IS_ENOENT(sRC))
    {
        /* When EACCES, aPath may be a directory
         * When ENOENT, aPath may be like "C:\", "D:\", ...
         * Open aPath as directory and try again */

        HANDLE sHandle;

        sHandle = CreateFile(sPath,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL);
        ACP_TEST_RAISE(INVALID_HANDLE_VALUE == sHandle, OPEN_FAILED);

        sResult = GetFileInformationByHandle(sHandle, &sInfo);
        (void)CloseHandle(sHandle);
        ACP_TEST_RAISE(0 == sResult, GET_FAILED);
    }
    else
    {
        /* Get file information if successful */
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), OPEN_FAILED);
        sResult = GetFileInformationByHandle(sFile.mHandle, &sInfo);
        (void)acpFileClose(&sFile);
        ACP_TEST_RAISE(0 == sResult, GET_FAILED);
    }

    acpPathPoolFinal(&sPool);

    /* Make key */
    sKey = (acp_sint32_t)(sInfo.nFileIndexHigh);
    sKey = (sKey << 16) | ((acp_sint32_t)(sInfo.nFileIndexLow));
    sKey = (sKey << 8) | ((acp_sint32_t)(aProj & 0xFF));
    *aKey = (acp_key_t)sKey;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(RESOLVE_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION(PROJID_FAILED)
    {
        sRC = ACP_RC_ENOTSUP;
    }

    ACP_EXCEPTION(OPEN_FAILED)
    {
        /* Nothing to handle */
    }

    ACP_EXCEPTION(GET_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;
    acpPathPoolFinal(&sPool);

    return sRC;
}

