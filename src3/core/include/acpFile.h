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
 * $Id: acpFile.h 6904 2009-08-12 05:29:49Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_FILE_H_)
#define _O_ACP_FILE_H_

/**
 * @file
 * @ingroup CoreFile
 */

#include <acpPath.h>
#include <acpTime.h>
#include <acpTest.h>

ACP_EXTERN_C_BEGIN


/*
 * file open flags
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * open file for read only
 * @see acpFileOpen()
 */
#define ACP_O_RDONLY
/**
 * open file for write only
 * @see acpFileOpen()
 */
#define ACP_O_WRONLY
/**
 * open file for read and write
 * @see acpFileOpen()
 */
#define ACP_O_RDWR
/**
 * open file for append
 * @see acpFileOpen()
 */
#define ACP_O_APPEND
/**
 * create file if the file to open does not exist
 * @see acpFileOpen()
 */
#define ACP_O_CREAT
/**
 * with ACP_O_CREAT option, error if the file already exists
 * @see acpFileOpen()
 */
#define ACP_O_EXCL
/**
 * after open, the file will be truncated to size 0
 * @see acpFileOpen()
 */
#define ACP_O_TRUNC
/**
 * synchronous I/O
 * @see acpFileOpen()
 */
#define ACP_O_SYNC
#else
#if defined (ALTI_CFG_OS_WINDOWS)
#define ACP_O_RDONLY 0x00000000
#define ACP_O_WRONLY 0x00000001
#define ACP_O_RDWR   0x00000002
#define ACP_O_APPEND 0x00000008
#define ACP_O_CREAT  0x00000010
#define ACP_O_TRUNC  0x00000020
#define ACP_O_EXCL   0x00000040
#define ACP_O_SYNC   0x00000100
#else
#define ACP_O_RDONLY O_RDONLY
#define ACP_O_WRONLY O_WRONLY
#define ACP_O_RDWR   O_RDWR
#define ACP_O_APPEND O_APPEND
#define ACP_O_CREAT  O_CREAT
#define ACP_O_TRUNC  O_TRUNC
#define ACP_O_EXCL   O_EXCL
#define ACP_O_SYNC   O_SYNC
#endif
#endif

/*
 * permission
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * read, write, execute permissions are allowed to the user who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IRWXU
/**
 * read permission is allowed to the user who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IRUSR
/**
 * write permission is allowed to the user who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IWUSR
/**
 * execute permission is allowed to the user who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IXUSR
/**
 * read, write, execute permissions are allowed to the group who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IRWXG
/**
 * read permission is allowed to the group who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IRGRP
/**
 * write permission is allowed to the group who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IWGRP
/**
 * execute permission is allowed to the group who owns the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IXGRP
/**
 * read, write, execute permissions are allowed to the users
 * who do not own the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IRWXO
/**
 * read permission is allowed to the users who do not own the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IROTH
/**
 * write permission is allowed to the users who do not own the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IWOTH
/**
 * execute permission is allowed to the users who do not own the file
 * @see acpFileOpen() acp_stat_t::mPermission
 */
#define ACP_S_IXOTH
#else
#if defined (ALTI_CFG_OS_WINDOWS)
#define ACP_S_IRWXU  0700
#define ACP_S_IRUSR  0400
#define ACP_S_IWUSR  0200
#define ACP_S_IXUSR  0100
#define ACP_S_IRWXG  0070
#define ACP_S_IRGRP  0040
#define ACP_S_IWGRP  0020
#define ACP_S_IXGRP  0010
#define ACP_S_IRWXO  0007
#define ACP_S_IROTH  0004
#define ACP_S_IWOTH  0002
#define ACP_S_IXOTH  0001
#else
#define ACP_S_IRWXU  S_IRWXU
#define ACP_S_IRUSR  S_IRUSR
#define ACP_S_IWUSR  S_IWUSR
#define ACP_S_IXUSR  S_IXUSR
#define ACP_S_IRWXG  S_IRWXG
#define ACP_S_IRGRP  S_IRGRP
#define ACP_S_IWGRP  S_IWGRP
#define ACP_S_IXGRP  S_IXGRP
#define ACP_S_IRWXO  S_IRWXO
#define ACP_S_IROTH  S_IROTH
#define ACP_S_IWOTH  S_IWOTH
#define ACP_S_IXOTH  S_IXOTH
#endif
#endif

/*
 * whence to seek
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * seek from beginning of the file
 * @see acpFileSeek()
 */
#define ACP_SEEK_SET
/**
 * seek from current location of the file
 * @see acpFileSeek()
 */
#define ACP_SEEK_CUR
/**
 * seek from end of file
 * @see acpFileSeek()
 */
#define ACP_SEEK_END
#else
#if defined (ALTI_CFG_OS_WINDOWS)
#define ACP_SEEK_SET FILE_BEGIN
#define ACP_SEEK_CUR FILE_CURRENT
#define ACP_SEEK_END FILE_END
#else
#define ACP_SEEK_SET SEEK_SET
#define ACP_SEEK_CUR SEEK_CUR
#define ACP_SEEK_END SEEK_END
#endif
#endif

/*
 * Pipe types
 */
#if defined(ACP_CFG_DOXYGEN)
/*
 * index of the read handle of the pipe
 * @see acpFilePipe()
 */
#define ACP_PIPE_READ
/*
 * index of the write handle of the pipe
 * @see acpFilePipe()
 */
#define ACP_PIPE_WRITE
#else
#define ACP_PIPE_READ  0
#define ACP_PIPE_WRITE 1
#endif

/*
 * file object
 */
/**
 * file object
 */
typedef struct acp_file_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    HANDLE       mHandle;
    acp_bool_t   mAppendMode;
#else
    acp_sint32_t mHandle;
#endif
} acp_file_t;

/*
 * file type
 */
/**
 * file types
 */
typedef enum acp_filetype_t
{
    ACP_FILE_UNK = 0, /**< unknown file type */
    ACP_FILE_REG,     /**< regular file      */
    ACP_FILE_DIR,     /**< directory         */
    ACP_FILE_CHR,     /**< character device  */
    ACP_FILE_BLK,     /**< block device      */
    ACP_FILE_FIFO,    /**< named pipe, FIFO  */
    ACP_FILE_LNK,     /**< symbolic link     */
    ACP_FILE_SOCK     /**< socket file       */
} acp_filetype_t;

/*
 * file stat
 *
 * BUGBUG:
 * There are no user or group who own this file.
 * Currently, There are no api for user or group in ACP
 */
/**
 * file stat structure
 *
 * @see acpFileStat() acpFileStatAtPath()
 */
typedef struct acp_stat_t
{
    acp_sint32_t   mPermission; /**< file permission             */
    acp_filetype_t mType;       /**< file type                   */
    acp_offset_t   mSize;       /**< file size                   */
    acp_time_t     mAccessTime; /**< last accessed time          */
    acp_time_t     mModifyTime; /**< last modified time          */
    acp_time_t     mChangeTime; /**< last attribute changed time */
} acp_stat_t;


#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

ACP_EXPORT acp_rc_t acpFileOpen(acp_file_t   *aFile,
                                acp_char_t   *aPath,
                                acp_sint32_t  aFlag,
                                acp_mode_t    aMode);

ACP_INLINE acp_rc_t acpFileClose(acp_file_t *aFile)
{
    BOOL sRet;

    sRet = CloseHandle(aFile->mHandle);

    return (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t acpFileRead(acp_file_t *aFile,
                                void       *aBuffer,
                                acp_size_t  aSize,
                                acp_size_t *aReadSize)
{
    BOOL     sRet;
    DWORD    sReadSize;
    acp_rc_t sRC;

    if (aSize == 0)
    {
        sReadSize = 0;
        sRC       = ACP_RC_SUCCESS;
    }
    else
    {
        if (aSize > (acp_size_t)ACP_SINT32_MAX)
        {
            return ACP_RC_EINVAL;
        }
        else
        {
            /* do nothing */
        }

        sRet = ReadFile(aFile->mHandle,
                        aBuffer,
                        (DWORD)aSize,
                        &sReadSize,
                        NULL);

        if (sRet == 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if (sReadSize == 0)
            {
                sRC = ACP_RC_EOF;
            }
            else
            {
                sRC = ACP_RC_SUCCESS;
            }
        }
    }

    if (aReadSize != NULL)
    {
        *aReadSize = (acp_size_t)sReadSize;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

ACP_INLINE acp_rc_t acpFileWrite(acp_file_t *aFile,
                                 const void *aBuffer,
                                 acp_size_t  aSize,
                                 acp_size_t *aWrittenSize)
{
    BOOL  sRet;
    DWORD sWrittenSize;
    DWORD sAppendRet;

    if (aSize > (acp_size_t)ACP_SINT32_MAX)
    {
        return ACP_RC_EINVAL;
    }
    else
    {

        if (aFile->mAppendMode == ACP_TRUE)
        {
            sAppendRet = SetFilePointer(aFile->mHandle,
                                        0,
                                        NULL,
                                        ACP_SEEK_END);

            if (sAppendRet == INVALID_SET_FILE_POINTER)
            {
                return ACP_RC_GET_OS_ERROR();
            }
        }
        else
        {
            /* do nothing */
        }

        sRet = WriteFile(aFile->mHandle,
                         aBuffer,
                         (DWORD)aSize,
                         &sWrittenSize,
                         NULL);

        if (aWrittenSize != NULL)
        {
            *aWrittenSize = sWrittenSize;
        }
        else
        {
            /* do nothing */
        }

        return (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpFileSeek(acp_file_t   *aFile,
                                acp_offset_t  aOffset,
                                acp_sint32_t  aWhence,
                                acp_offset_t *aAbsOffset)
{
    LARGE_INTEGER sOffset;
    acp_rc_t      sRC;

    sOffset.QuadPart = aOffset;

    sOffset.LowPart  = SetFilePointer(aFile->mHandle,
                                      sOffset.LowPart,
                                      &sOffset.HighPart,
                                      aWhence);

    if (sOffset.LowPart == INVALID_SET_FILE_POINTER)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    if (ACP_RC_IS_SUCCESS(sRC) && (aAbsOffset != NULL))
    {
        *aAbsOffset = (acp_offset_t)sOffset.QuadPart;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

ACP_INLINE acp_rc_t acpFileSync(acp_file_t *aFile)
{
    BOOL sRet;

    sRet = FlushFileBuffers(aFile->mHandle);

    return (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpFileTruncate(acp_file_t *aFile, acp_offset_t aSize);
ACP_EXPORT acp_rc_t acpFileTruncateAtPath(acp_char_t *aPath,
                                          acp_offset_t aSize);

ACP_INLINE acp_rc_t acpFileDup(acp_file_t *aOldFile, acp_file_t *aNewFile)
{
    HANDLE sHandleProc = GetCurrentProcess();
    HANDLE sNewHandle  = NULL;
    BOOL   sRet;

    sRet = DuplicateHandle(sHandleProc,
                           aOldFile->mHandle,
                           sHandleProc,
                           &sNewHandle,
                           0,
                           FALSE,
                           DUPLICATE_SAME_ACCESS);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aNewFile->mHandle     = sNewHandle;
        aNewFile->mAppendMode = aOldFile->mAppendMode;

        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpFileCopy(acp_char_t *aOldPath,
                                acp_char_t *aNewPath,
                                acp_bool_t  aOverwrite)
{
    acp_path_pool_t sPool;
    BOOL            sRet;
    acp_char_t*     sOldPath = NULL;
    acp_char_t*     sNewPath = NULL;
    acp_rc_t        sRC;

    if((NULL == aOldPath) || (NULL == aNewPath))
    {
        sRC = ACP_RC_EINVAL;
    }
    else
    {
        acpPathPoolInit(&sPool);

        sOldPath = acpPathFull(aOldPath, &sPool);

        if (NULL == sOldPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sNewPath = acpPathFull(aNewPath, &sPool);

            if(NULL == sNewPath)
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
                sRet = CopyFile(sOldPath, sNewPath,
                                (aOverwrite == ACP_TRUE) ? FALSE : TRUE);
                sRC = (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
            }
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

ACP_INLINE acp_rc_t acpFileRename(acp_char_t *aOldPath, acp_char_t *aNewPath)
{
    acp_path_pool_t sPool;
    BOOL            sRet;
    acp_char_t*     sOldPath = NULL;
    acp_char_t*     sNewPath = NULL;
    acp_rc_t        sRC;

    if ((aOldPath == NULL) || (aNewPath == NULL))
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        acpPathPoolInit(&sPool);

        sOldPath = acpPathFull(aOldPath, &sPool);
        if(NULL == sOldPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sNewPath = acpPathFull(aNewPath, &sPool);
            if(NULL == sNewPath)
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
                sRet = MoveFile(sOldPath, sNewPath);
                sRC = (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
            }
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

ACP_INLINE acp_rc_t acpFileRemove(acp_char_t *aPath)
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
            sRet = DeleteFile(sPath);
            sRC = (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpFileGetKey(
    acp_char_t*  aPath,
    acp_sint32_t aProj,
    acp_key_t*   aKey);

ACP_INLINE acp_rc_t acpFilePipe(acp_file_t aPipe[])
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    acp_bool_t sRet = FALSE;

    SECURITY_ATTRIBUTES sSecAttr;

    sSecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sSecAttr.bInheritHandle = TRUE;
    sSecAttr.lpSecurityDescriptor = NULL;

    sRet = CreatePipe(&(aPipe[ACP_PIPE_READ].mHandle),
                      &(aPipe[ACP_PIPE_WRITE].mHandle),
                      &sSecAttr,
                      0);

    ACP_TEST_RAISE(0 == sRet, PIPE_CREATE_FAILED);

    return sRC;

    ACP_EXCEPTION(PIPE_CREATE_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;

    return sRC;
}

#else

/**
 * opens the file at path @a aPath with @a aFlag
 *
 * when it create file, the permission of the file will be @a aMode
 *
 * @param aFile pointer to the file object
 * @param aPath a null terminated string that contains the path to the file
 * @param aFlag the bit masked flags combined with Open Flags;<br>
 *      #ACP_O_RDONLY   : Open file in read-only mode<br>
 *      #ACP_O_WRONLY   : Open file in write-only mode<br>
 *      #ACP_O_RDWR     : Open file in read-write mode<br>
 *      #ACP_O_APPEND   : Open file in append mode<br>
 *      #ACP_O_CREAT    : Open file and create if the file does not exist<br>
 *      #ACP_O_EXCL     : With #ACP_O_CREAT, Error if the file exists<br>
 *      #ACP_O_TRUNC    : Open file and make the file as size 0<br>
 *      #ACP_O_SYNC     : Open file in I/O synchronous mode<br>
 * @param aMode the bit masked permission of the creating file<br>
 *      Group permissions are ignored in Windows.<br>
 *      #ACP_S_IRWXU    : Owner  Read, Write, Execute    (rwx------)<br>
 *      #ACP_S_IRUSR    : Owner  Read                    (r--------)<br>
 *      #ACP_S_IWUSR    : Owner  Write                   (-w-------)<br>
 *      #ACP_S_IXUSR    : Owner  Execute                 (--x------)<br>
 *      #ACP_S_IRWXG    : Group  Read, Write, Execute    (---rwx---)<br>
 *      #ACP_S_IRGRP    : Group  Read                    (---r-----)<br>
 *      #ACP_S_IWGRP    : Group  Write                   (----w----)<br>
 *      #ACP_S_IXGRP    : Group  Execute                 (-----x---)<br>
 *      #ACP_S_IRWXO    : Others Read, Write, Execute    (------rwx)<br>
 *      #ACP_S_IROTH    : Others Read                    (------r--)<br>
 *      #ACP_S_IWOTH    : Others Write                   (-------w-)<br>
 *      #ACP_S_IXOTH    : Others Execute                 (--------x)<br>
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aFile or @a aPath points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      The requested access to @a aPath is not allowed,<br>
 *      Or search permission is not allowed for one of the directories
 *      in the prefix of @a aPath,<br>
 *      Or the @a aPath did not exist and write access to the parent directory
 *      is not allowed.
 * @return #ACP_RC_EEXIST
 *      @a aPath already exists and ACP_O_CREAT and ACP_O_EXCL were used.
 * @return #ACP_RC_EINTR
 *      Received a signal or an interrupt while trying to open @a aPath.
 * @return #ACP_RC_ELOOP
 *      Too many symbolic links were encountered while resolving @a aPath.
 * @return #ACP_RC_EMFILE
 *      The process cannot open more files.
 * @return #ACP_RC_ENAMETOOLONG
 *      @a aPath is too long.
 * @return #ACP_RC_ENFILE
 *      The system cannot open more files.
 * @return #ACP_RC_ENOENT
 *      #ACP_O_CREAT is not specified on @a aFlag and @a aPath does not exist.
 * @return #ACP_RC_ENOMEM
 *      Kernel memory is not sufficient to open @a aPath.
 * @return #ACP_RC_ENOSPC
 *      @a aPath was to be created and
 *      not enough space on the device containing @a aPath.
 * @return #ACP_RC_EAI_OVERFLOW
 *      @a aPath refers to a regular file that is too large to be opened.
 * @return ETXTBSY
 *      @a aPath refers to an executable image which is currently running.
 */
ACP_INLINE acp_rc_t acpFileOpen(acp_file_t   *aFile,
                                acp_char_t   *aPath,
                                acp_sint32_t  aFlag,
                                acp_mode_t    aMode)
{
    acp_path_pool_t sPool;
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
            (void)umask(0);
            aFile->mHandle = open(sPath, aFlag, aMode);
            sRC = (aFile->mHandle == -1) ?
                ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

/**
 * closes the file
 *
 * @param aFile pointer to the file object
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file
 * @return #ACP_RC_EINTR
 *      Received a signal or an interrupt while trying to open @a aPath.
 */
ACP_INLINE acp_rc_t acpFileClose(acp_file_t *aFile)
{
    acp_sint32_t sRet;

    sRet = close(aFile->mHandle);

    return (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

/**
 * reads data from the file
 *
 * @param aFile pointer to the file object
 * @param aBuffer the buffer to store the read data
 * @param aSize the size of buffer (the data cannot be read more than @a aSize)
 * @param aReadSize pointer to the variable to get the really read size
 * in bytes (this can be NULL, if you don't want to get)
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aFile or @a aBuffer points outside of the process' address space
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file
 * @return #ACP_RC_EINVAL
 *      The offset in @a aFile or @a aBuffer is not properly aligned.
 * @return #ACP_RC_EINTR 
 *      Signaled while trying to read
 */
ACP_INLINE acp_rc_t acpFileRead(acp_file_t *aFile,
                                void       *aBuffer,
                                acp_size_t  aSize,
                                acp_size_t *aReadSize)
{
    acp_ssize_t sRet;
    acp_rc_t    sRC;

    if (aSize == 0)
    {
        sRet = 0;
        sRC  = ACP_RC_SUCCESS;
    }
    else
    {
        if (aSize > (acp_size_t)ACP_SINT32_MAX)
        {
            return ACP_RC_EINVAL;
        }
        else
        {
            sRet = read(aFile->mHandle, aBuffer, aSize);

            if (sRet == -1)
            {
                return ACP_RC_GET_OS_ERROR();
            }
            else if (sRet == 0)
            {
                sRC = ACP_RC_EOF;
            }
            else
            {
                sRC = ACP_RC_SUCCESS;
            }
        }
    }

    if (aReadSize != NULL)
    {
        *aReadSize = (acp_size_t)sRet;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * writes data to the file
 *
 * @param aFile pointer to the file object
 * @param aBuffer the buffer to be written
 * @param aSize the size to be written
 * @param aWrittenSize pointer to a variable to get written size
 * the really written size in bytes
 * (this can be NULL, if you don't want to get)
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aFile or @a aBuffer points outside of current process' address space
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file descriptor
 * @return #ACP_RC_EINVAL
 *      @a aSize is greater than 2^31 - 1
 * @return #ACP_RC_EINTR
 *      Signaled while trying to write
 */
ACP_INLINE acp_rc_t acpFileWrite(acp_file_t *aFile,
                                 const void *aBuffer,
                                 acp_size_t  aSize,
                                 acp_size_t *aWrittenSize)
{
    acp_ssize_t sRet;

    if (aSize > (acp_size_t)ACP_SINT32_MAX)
    {
        return ACP_RC_EINVAL;
    }
    else
    {

        sRet = write(aFile->mHandle, aBuffer, aSize);

        if (sRet == -1)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if (aWrittenSize != NULL)
            {
                *aWrittenSize = (acp_size_t)sRet;
            }
            else
            {
                /* do nothing */
            }

            return ACP_RC_SUCCESS;
        }
    }
}

/**
 * moves the current location of the file
 *
 * @param aFile pointer to the file object
 * @param aOffset the offset to move
 * @param aWhence the directive to move;<br>
 *      #ACP_SEEK_SET   : From the beginning of file<br>
 *      #ACP_SEEK_CUR   : From current position<br>
 *      #ACP_SEEK_END   : From the end of file<br>
 * @param aAbsOffset pointer to the variable to get the absolute offset of
 * the file after move (this can be NULL, if you don't want to get)
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aFile points outside of current process' address space
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file descriptor
 * @return #ACP_RC_EINVAL
 *      @a aOffset is not one of #ACP_SEEK_SET, #ACP_SEEK_CUR or #ACP_SEEK_END
 * @return #ACP_RC_EAI_OVERFLOW
 *      The resulting file offset cannot be represented in #acp_offset_t
 */
ACP_INLINE acp_rc_t acpFileSeek(acp_file_t   *aFile,
                                acp_offset_t  aOffset,
                                acp_sint32_t  aWhence,
                                acp_offset_t *aAbsOffset)
{
    acp_offset_t sRet;

    sRet = lseek(aFile->mHandle, aOffset, aWhence);

    if (sRet == (acp_offset_t)-1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        if (aAbsOffset != NULL)
        {
            *aAbsOffset = sRet;
        }
        else
        {
            /* do nothing */
        }

        return ACP_RC_SUCCESS;
    }
}

/**
 * synchronizes the file's in-memory state  with  that  on the physical medium
 *
 * @param aFile pointer to the file object
 */
ACP_INLINE acp_rc_t acpFileSync(acp_file_t *aFile)
{
    acp_sint32_t sRet;

    sRet = fsync(aFile->mHandle);

    return (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

/**
 * truncates the file to @a aSize byte(s)
 *
 * @param aFile pointer to the file object
 * @param aSize the truncate size in bytes
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file descriptor
 * @return #ACP_RC_EINVAL
 *      @a aFile is bound to a special file
 *      that does not support synchronization.
 */
ACP_INLINE acp_rc_t acpFileTruncate(acp_file_t *aFile, acp_offset_t aSize)
{
    acp_sint32_t sRet;

    sRet = ftruncate(aFile->mHandle, aSize);

    return (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

/**
 * truncates the file at @a aPath to @a aSize byte(s)
 *
 * @param aPath the path of the file
 * @param aSize the truncate size in bytes
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aFile or @a aPath points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      Or search permission is not allowed for one of the directories
 *      in the prefix of @a aPath,<br>
 *      Or write permission to @a aPath is not allowed.
 * @return #ACP_RC_EEXIST
 *      @a aPath already exists and ACP_O_CREAT and ACP_O_EXCL were used.
 * @return #ACP_RC_EINTR
 *      Received a signal or an interrupt while trying to open @a aPath.
 * @return #ACP_RC_EINVAL
 *      @a aSize is negative of larger than the maximum file size
 * @return #ACP_RC_ELOOP
 *      Too many symbolic links were encountered while resolving @a aPath.
 * @return #ACP_RC_ENAMETOOLONG
 *      @a aPath is too long.
 * @return #ACP_RC_ENOENT
 *      @a aPath does not exist.
 * @return #ACP_RC_ENOTDIR
 *      A path component of @a aPath does not exists
 * @return #ACP_RC_ENOSPC
 *      @a aPath was to be created and
 *      not enough space on the device containing @a aPath.
 * @return #ACP_RC_EAI_OVERFLOW
 *      @a aPath refers to a regular file that is too large to be opened.
 * @return #ACP_RC_EPERM
 *      The file system does not allow @a aFile to be extended
 *      more than current size.
 * @return ETXTBSY
 *      @a aFile is an executable and is being run.
 */
ACP_INLINE acp_rc_t acpFileTruncateAtPath(acp_char_t *aPath, acp_offset_t aSize)
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
            sRet = truncate(sPath, aSize);
            sRC = (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

/**
 * duplicates file object
 *
 * @param aOldFile the file object to be duplicated from
 * @param aNewFile the file object to be duplicated to
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aFile or @a aPath points outside of the process' address space
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file
 * @return #ACP_RC_EINTR
 *      Received a signal or an interrupt while trying to open @a aPath.
 * @return #ACP_RC_EMFILE
 *      The process cannot open more files.
 */
ACP_INLINE acp_rc_t acpFileDup(acp_file_t *aOldFile, acp_file_t *aNewFile)
{
    aNewFile->mHandle = dup(aOldFile->mHandle);

    return (aNewFile->mHandle == -1) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpFileCopy(acp_char_t  *aOldPath,
                                acp_char_t  *aNewPath,
                                acp_bool_t  aOverwrite);

/**
 * renames the file
 *
 * @param aOldPath the path of the file to be renamed from
 * @param aNewPath the path of the file to be renamed to
 * @return #ACP_RC_EFAULT
 *      @a aOldFile or @a aNewFile points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      The requested access to @a aPath is not allowed,<br>
 *      Or search permission is not allowed for one of the directories
 *      in the prefix of @a aPath,<br>
 *      Or the @a aPath did not exist and write access to the parent directory
 *      is not allowed.
 * @return #ACP_RC_EINVAL
 *      @a aNewPath is a subdirectory of @a aOldPath itself
 * @return #ACP_RC_ELOOP
 *      Too many symbolic links were encountered while resolving @a aPath.
 * @return #ACP_RC_ENAMETOOLONG
 *      @a aPath is too long.
 * @return #ACP_RC_ENOENT
 *      #ACP_O_CREAT is not specified on @a aFlag and @a aPath does not exist.
 * @return #ACP_RC_ENOMEM
 *      Kernel memory is not sufficient to open @a aPath.
 * @return #ACP_RC_ENOSPC
 *      Not enough space on the device containing @a aNewPath.
 * @return #ACP_RC_ENOTDIR
 *      A path component of @a aOldPath or @a aNewPath is not a directory,
 *      or @a aOldPath is a directory
 *      and @a aNewPath exists and is not a directory
 * @return #ACP_RC_ENOTEMPTY
 *      @a aNewPath is a non-empty directory
 */
ACP_INLINE acp_rc_t acpFileRename(acp_char_t *aOldPath, acp_char_t *aNewPath)
{
    acp_path_pool_t sPool;
    acp_sint32_t    sRet;
    acp_char_t*     sOldPath = NULL;
    acp_char_t*     sNewPath = NULL;
    acp_rc_t        sRC;

    if ((aOldPath == NULL) || (aNewPath == NULL))
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        acpPathPoolInit(&sPool);
        sOldPath = acpPathFull(aOldPath, &sPool);

        if(NULL == sOldPath)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            sNewPath = acpPathFull(aNewPath, &sPool);

            if(NULL == sNewPath)
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
                sRet = rename(sOldPath, sNewPath);
                sRC = (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
            }
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

/**
 * removes the file
 *
 * @param aPath the path of the file to be removed
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aFile or @a aPath points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      Write access to @a aPath is not allowed,<br>
 *      Or search permission is not allowed for one of the directories
 *      in the prefix of @a aPath,
 * @return #ACP_RC_EBUSY
 *      @a aPath is used by system of another application
 * @return #ACP_RC_ELOOP
 *      Too many symbolic links were encountered while resolving @a aPath.
 * @return #ACP_RC_ENAMETOOLONG
 *      @a aPath is too long.
 * @return #ACP_RC_ENOENT
 *      #ACP_O_CREAT is not specified on @a aFlag and @a aPath does not exist.
 * @return #ACP_RC_ENOMEM
 *      Kernel memory is not sufficient to open @a aPath.
 * @return #ACP_RC_EPERM
 *      The file system does not allow @a aFile to be removed
 */
ACP_INLINE acp_rc_t acpFileRemove(acp_char_t *aPath)
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
            sRet = unlink(sPath);
            sRC = (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
        }

        acpPathPoolFinal(&sPool);
    }

    return sRC;
}

/**
 * generates the System V IPC key with file path and project id
 *
 * @param aPath Path to the file
 *
 * @param aProj Project ID
 *
 * @param aKey Pointer to store the key value
 *
 * @return result code
 */
ACP_INLINE acp_rc_t acpFileGetKey(
    acp_char_t*  aPath,
    acp_sint32_t aProj,
    acp_key_t*   aKey)
{
    if (aProj == 0)
    {
        /* ftok's behavior is undefined with zero project id */
        return ACP_RC_ENOTSUP;
    }
    else
    {
        *aKey = ftok(aPath, aProj);
        return (-1 == (*aKey))? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
    }
}

/**
 * create pipe
 *
 * @param aPipe Array of pipe handles of size 2.
 *   #ACP_PIPE_READ for read handle of the pipe.
 *   #ACP_PIPE_WRITE for write handle.
 *
 * @return result code
 */
ACP_INLINE acp_rc_t acpFilePipe(acp_file_t aPipe[])
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    acp_sint32_t sPipe[2];
    acp_sint32_t sRet;

    sRet = pipe(sPipe);
    ACP_TEST_RAISE(-1 == sRet, PIPE_CREATE_FAILED);

    aPipe[ACP_PIPE_READ].mHandle = sPipe[ACP_PIPE_READ];
    aPipe[ACP_PIPE_WRITE].mHandle = sPipe[ACP_PIPE_WRITE];

    return sRC;

    ACP_EXCEPTION(PIPE_CREATE_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;

    return sRC;
}

#endif

ACP_EXPORT acp_rc_t acpFileLock(acp_file_t *aFile);
ACP_EXPORT acp_rc_t acpFileTryLock(acp_file_t *aFile);
ACP_EXPORT acp_rc_t acpFileUnlock(acp_file_t *aFile);

ACP_EXPORT acp_rc_t acpFileStat(acp_file_t *aFile, acp_stat_t *aStat);
ACP_EXPORT acp_rc_t acpFileStatAtPath(acp_char_t  *aPath,
                                      acp_stat_t *aStat,
                                      acp_bool_t  aTraverseLink);

ACP_EXTERN_C_END


#endif
