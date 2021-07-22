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

ACP_INLINE void acpFileStatToAcpStat(struct stat *aStat, acp_stat_t *aAcpStat)
{
    switch (aStat->st_mode & S_IFMT)
    {
        case S_IFREG:
            aAcpStat->mType = ACP_FILE_REG;
            break;
        case S_IFDIR:
            aAcpStat->mType = ACP_FILE_DIR;
            break;
        case S_IFCHR:
            aAcpStat->mType = ACP_FILE_CHR;
            break;
        case S_IFBLK:
            aAcpStat->mType = ACP_FILE_BLK;
            break;
        case S_IFIFO:
            aAcpStat->mType = ACP_FILE_FIFO;
            break;
        case S_IFLNK:
            aAcpStat->mType = ACP_FILE_LNK;
            break;
        case S_IFSOCK:
            aAcpStat->mType = ACP_FILE_SOCK;
            break;
        default:
            aAcpStat->mType = ACP_FILE_UNK;
            break;
    }

    aAcpStat->mPermission = (acp_sint32_t)(aStat->st_mode & 0777);
    aAcpStat->mSize       = aStat->st_size;
    aAcpStat->mAccessTime = acpTimeFromSec((acp_sint64_t)aStat->st_atime);
    aAcpStat->mModifyTime = acpTimeFromSec((acp_sint64_t)aStat->st_mtime);
    aAcpStat->mChangeTime = acpTimeFromSec((acp_sint64_t)aStat->st_ctime);
}

/*
 * copies a file - Internal Private Function
 * @param aOldFile file pointer of existing file to copy
 * @param aNewFile file pinter of new file
 * @return result code
 */
static acp_rc_t acpFileCopyInternal(
    acp_sint32_t aOldFile,
    acp_sint32_t aNewFile)
{
    acp_ssize_t     sReadSize;
    acp_ssize_t     sWriteSize;
    acp_ssize_t     sTempSize;
    acp_char_t      sBuffer[65536];

    while (1)
    {
        sReadSize = read(aOldFile, sBuffer, sizeof(sBuffer));

        if (sReadSize == -1)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else if (sReadSize == 0)
        {
            break;
        }
        else
        {
            /* do nothing */
        }

        sWriteSize = 0;

        while (sReadSize > 0)
        {
            sTempSize = write(aNewFile, sBuffer + sWriteSize, sReadSize);

            if (sTempSize == -1)
            {
                return ACP_RC_GET_OS_ERROR();
            }
            else
            {
                sWriteSize += sTempSize;
                sReadSize  -= sTempSize;
            }
        }
    }

    return ACP_RC_SUCCESS;
}

/**
 * copies a file
 * @param aOldPath path of existing file to copy
 * @param aNewPath path of new file
 * @param aOverwrite #ACP_TRUE to overwrite existing file at @a aNewPath,
 * otherwise #ACP_FALSE
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aOldPath or @a aNewPath points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      The read access to @a aOldPath is not allowed,
 *      or the write access to @a aNewPath is not allowed,
 *      or search permission is not allowed for one of the directories
 *      in the prefix of @a aOldPath or @a aNewPath
 *      or the @a aNewPath did not exist and
 *      write access to the parent directory is not allowed.
 * @return #ACP_RC_EEXIST
 *      @a aNewPath already exists and @a aOverwrite is #ACP_FALSE
 * @return #ACP_RC_EINTR
 *      Received a signal or an interrupt while trying to copy
 * @return #ACP_RC_ELOOP
 *      Too many symbolic links were encountered
 *      while resolving @a aOldPath or @a aNewPath
 * @return #ACP_RC_EMFILE
 *      The process cannot open more files.
 * @return #ACP_RC_ENAMETOOLONG
 *      @a aOldPath or @a aNewPath is too long.
 * @return #ACP_RC_ENFILE
 *      The system cannot open more files.
 * @return #ACP_RC_ENOENT
 *      @a aOldPath does not exist.
 * @return #ACP_RC_ENOMEM
 *      Kernel memory is not sufficient to open @a aPath.
 * @return #ACP_RC_ENOSPC
 *      Not enough space on the device containing @a aNewPath.
 */
ACP_EXPORT acp_rc_t acpFileCopy(acp_char_t  *aOldPath,
                                acp_char_t  *aNewPath,
                                acp_bool_t   aOverwrite)
{
    struct stat     sStat;
    acp_path_pool_t sPool;
    acp_sint32_t    sOldFile;
    acp_sint32_t    sNewFile;
    acp_char_t*     sOldPath = NULL;
    acp_char_t*     sNewPath = NULL;
    acp_rc_t        sRC;

    acpPathPoolInit(&sPool);

    sOldPath = acpPathFull(aOldPath, &sPool);
    if (NULL == sOldPath)
    {
        sOldFile = -1;
    }
    else
    {
        sOldFile = open(sOldPath, O_RDONLY);
    }

    acpPathPoolFinal(&sPool);

    if (sOldFile == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    if (fstat(sOldFile, &sStat) == -1)
    {
        (void)close(sOldFile);
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    acpPathPoolInit(&sPool);
    sNewPath = acpPathFull(aNewPath, &sPool);

    if (NULL == sNewPath)
    {
        sNewFile = -1;
    }
    else
    {
        if (aOverwrite == ACP_TRUE)
        {
            sNewFile = open(sNewPath,
                            O_WRONLY | O_CREAT | O_TRUNC,
                            sStat.st_mode);
        }
        else
        {
            sNewFile = open(sNewPath,
                            O_WRONLY | O_CREAT | O_EXCL,
                            sStat.st_mode);
        }
    }

    acpPathPoolFinal(&sPool);

    if (sNewFile == -1)
    {
        (void)close(sOldFile);
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRC = acpFileCopyInternal(sOldFile, sNewFile);

    (void)close(sOldFile);
    (void)close(sNewFile);

    return sRC;
}

/**
 * locks a file
 *
 * @param aFile pointer to the file object
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aFile points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      @a aFile is locked by another process
 * @return #ACP_RC_EAGAIN
 *      Cannot lock @a aFile because the file has been memory-mapped by
 *      another process
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file descriptor
 * @return #ACP_RC_EDEADLK
 *      Locking @a aFile would cause a deadlock.
 * @return #ACP_RC_EINTR
 *      Interrupted or signaled while locking @a aFile
 */
ACP_EXPORT acp_rc_t acpFileLock(acp_file_t *aFile)
{
    struct flock sLock;
    acp_sint32_t sRet;
    acp_rc_t     sRC;

    sLock.l_whence = SEEK_SET;
    sLock.l_start  = 0;
    sLock.l_len    = 0;
    sLock.l_pid    = 0;
    sLock.l_type   = F_WRLCK;

    while (1)
    {
        sRet = fcntl(aFile->mHandle, F_SETLKW, &sLock);

        if (sRet == 0)
        {
            return ACP_RC_SUCCESS;
        }
        else
        {
            sRC = ACP_RC_GET_OS_ERROR();

            if (ACP_RC_NOT_EINTR(sRC))
            {
                return sRC;
            }
            else
            {
                /* continue */
            }
        }
    }
}

/**
 * trys lock a file
 *
 * @param aFile pointer to the file object
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aFile points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      @a aFile is locked by another process
 * @return #ACP_RC_EAGAIN
 *      Cannot lock @a aFile because the file has been memory-mapped by
 *      another process
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file descriptor
 * @return #ACP_RC_EDEADLK
 *      Locking @a aFile would cause a deadlock.
 * @return #ACP_RC_EINTR
 *      Interrupted or signaled while locking @a aFile
 * @return #ACP_RC_EBUSY
 *      @a aFile is already locked
 */
ACP_EXPORT acp_rc_t acpFileTryLock(acp_file_t *aFile)
{
    struct flock sLock;
    acp_sint32_t sRet;

    sLock.l_whence = SEEK_SET;
    sLock.l_start  = 0;
    sLock.l_len    = 0;
    sLock.l_pid    = 0;
    sLock.l_type   = F_WRLCK;

    while (1)
    {
        sRet = fcntl(aFile->mHandle, F_SETLK, &sLock);

        if (sRet == 0)
        {
            return ACP_RC_SUCCESS;
        }
        else
        {
            if ((errno == EAGAIN) || (errno == EACCES))
            {
                return ACP_RC_EBUSY;
            }
            else
            {
                return ACP_RC_GET_OS_ERROR();
            }
        }
    }
}

/**
 * unlocks a file
 *
 * @param aFile pointer to the file object
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aFile points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      @a aFile is locked by another process
 * @return #ACP_RC_EAGAIN
 *      Cannot lock @a aFile because the file has been memory-mapped by
 *      another process
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file descriptor
 * @return #ACP_RC_EDEADLK
 *      Locking @a aFile would cause a deadlock.
 * @return #ACP_RC_EINTR
 *      Interrupted or signaled while locking @a aFile
 */
ACP_EXPORT acp_rc_t acpFileUnlock(acp_file_t *aFile)
{
    struct flock sLock;
    acp_sint32_t sRet;
    acp_rc_t     sRC;

    sLock.l_whence = SEEK_SET;
    sLock.l_start  = 0;
    sLock.l_len    = 0;
    sLock.l_pid    = 0;
    sLock.l_type   = F_UNLCK;

    while (1)
    {
        sRet = fcntl(aFile->mHandle, F_SETLKW, &sLock);

        if (sRet == 0)
        {
            return ACP_RC_SUCCESS;
        }
        else
        {
            sRC = ACP_RC_GET_OS_ERROR();

            if (ACP_RC_NOT_EINTR(sRC))
            {
                return sRC;
            }
            else
            {
                /* continue */
            }
        }
    }
}

ACP_INLINE acp_rc_t acpFileStatDevice(
    acp_file_t*  aFile,
    acp_stat_t*  aStat)
{
#if defined(ALTI_CFG_OS_AIX)
    acp_rc_t sRC;
    acp_offset_t sNumBlks;
    acp_offset_t sBlkSize;
    struct devinfo sDevInfo;

    ACP_TEST(-1 == ioctl(aFile->mHandle, IOCINFO, &sDevInfo));

    if((DD_DISK == sDevInfo.devtype) ||
       (DD_SCDISK == sDevInfo.devtype))
    {
        switch(sDevInfo.devsubtype)
        {
        case DS_LV: /*@fallthrough@*/
        case DS_LVZ:
            if(0 != (sDevInfo.flags & DF_LGDSK))
            {
                sNumBlks = sDevInfo.un.dk64.hi_numblks;
                sNumBlks = (sNumBlks << 32) + sDevInfo.un.dk64.lo_numblks;
                sBlkSize = sDevInfo.un.dk64.bytpsec;
            }
            else
            {
                sNumBlks = sDevInfo.un.dk.numblks;
                sBlkSize = sDevInfo.un.dk.bytpsec;
            }
            aStat->mSize  = sBlkSize;
            aStat->mSize *= sNumBlks;
            sRC = ACP_RC_SUCCESS;
            break;

        case DS_PV:
            if(0 != (sDevInfo.flags & DF_LGDSK))
            {
                sNumBlks = sDevInfo.un.scdk.numblks;
                sBlkSize = sDevInfo.un.scdk.blksize;
            }
            else
            {
                sNumBlks = sDevInfo.un.scdk64.hi_numblks;
                sNumBlks = (sNumBlks << 32) + sDevInfo.un.scdk64.lo_numblks;
                sBlkSize = sDevInfo.un.scdk64.blksize;
            }
            aStat->mSize  = sBlkSize;
            aStat->mSize *= sNumBlks;
            sRC = ACP_RC_SUCCESS;
            break;

        default:
            ACP_RAISE(DEVICE_NOT_SUPPORTED);
            break;
        }
    }
    else if(DD_CDROM == sDevInfo.devtype)
    {
        if(0 != (sDevInfo.flags & DF_LGDSK))
        {
            sNumBlks = sDevInfo.un.sccd64.hi_numblks;
            sNumBlks = (sNumBlks << 32) + sDevInfo.un.sccd64.lo_numblks;
            sBlkSize = sDevInfo.un.sccd64.blksize;
        }
        else
        {
            sNumBlks = sDevInfo.un.sccd.numblks;
            sBlkSize = sDevInfo.un.sccd.blksize;
        }

        aStat->mSize  = sBlkSize;
        aStat->mSize *= sNumBlks;
        sRC = ACP_RC_SUCCESS;
    }
    else
    {
        ACP_RAISE(DEVICE_NOT_SUPPORTED);
    }

    return sRC;


    ACP_EXCEPTION(DEVICE_NOT_SUPPORTED)
    {
        return ACP_RC_ENOTSUP;
    }

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();

#elif defined(ALTI_CFG_OS_HPUX)
    disk_describe_type sDesc;
    ACP_TEST(-1 == ioctl(aFile->mHandle, DIOC_DESCRIBE, &sDesc));

    aStat->mSize  = (acp_offset_t)sDesc.lgblksz;
    aStat->mSize *= (acp_offset_t)sDesc.maxsva;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();

#elif defined(ALTI_CFG_OS_LINUX)
    acp_sint32_t sSize32;
    /*
     * Just a work-around to avoid gcc 3.4.6 bug
     */
    acp_stat_t* sStat = aStat;


    ACP_TEST(-1 == ioctl(aFile->mHandle, BLKGETSIZE, &sSize32));

    sStat->mSize  = (acp_offset_t)sSize32;
    sStat->mSize *= (acp_offset_t)512;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();

#elif defined(ALTI_CFG_OS_SOLARIS)
    struct dk_minfo sMediaInfo;
    ACP_TEST(-1 == ioctl(aFile->mHandle, DKIOCGMEDIAINFO, &sMediaInfo));

    aStat->mSize  = (acp_offset_t)sMediaInfo.dki_lbsize;
    aStat->mSize *= (acp_offset_t)sMediaInfo.dki_capacity;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();

#elif defined(ALTI_CFG_OS_TRU64)
    DEVGEOMST sDevGeom;
    ACP_TEST(-1 == ioctl(aFile->mHandle, DEVGETGEOM, &sDevGeom));

    aStat->mSize  = (acp_offset_t)sDevGeom.geom_info.ntracks;
    aStat->mSize *= (acp_offset_t)sDevGeom.geom_info.nsectors;
    aStat->mSize *= (acp_offset_t)sDevGeom.geom_info.ncylinders;
    aStat->mSize *= (acp_offset_t)sDevGeom.geom_info.sector_size;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();

#elif defined(ALTI_CFG_OS_FREEBSD)
    acp_offset_t sSize;
    ACP_TEST(-1 == ioctl(aFile->mHandle, DIOCGMEDIASIZE, &sSize));

    aStat->mSize = sSize;
    
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();

#elif defined(ALTI_CFG_OS_DARWIN)
    acp_uint32_t sBlockSize;
    acp_uint64_t sBlockCount;

    ACP_TEST(-1 == ioctl(aFile->mHandle, DKIOCGETBLOCKSIZE , &sBlockSize ));
    ACP_TEST(-1 == ioctl(aFile->mHandle, DKIOCGETBLOCKCOUNT, &sBlockCount));

    aStat->mSize  = (acp_offset_t)sBlockSize;
    aStat->mSize *= (acp_offset_t)sBlockCount;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();

#else
    ACP_UNUSED(aFile);
    ACP_UNUSED(aStat);
    /* No support for other OSes yet */
    return ACP_RC_ENOTSUP;
#endif
}

/**
 * gets the acp_stat_t file information structure of the file
 *
 * @param aFile pointer to the file object
 * @param aStat pointer to the acp_stat_t structure to be written
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aFile or @a aStat points outside of the process' address space
 * @return #ACP_RC_EBADF
 *      @a aFile is not a valid file descriptor
 * @return #ACP_RC_EACCES
 *      The read access to @a aFile is not allowed,
 * @return #ACP_RC_ENFILE
 *      The system cannot open more files
 * @return #ACP_RC_ENOMEM
 *      There is not enough memory to proceed
 */
ACP_EXPORT acp_rc_t acpFileStat(acp_file_t *aFile, acp_stat_t *aStat)
{
    struct stat  sStat;
    acp_sint32_t sRet;
        
    sRet = fstat(aFile->mHandle, &sStat);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        acp_rc_t sRC;

        acpFileStatToAcpStat(&sStat, aStat);

        /* If size is 0, this file may be a device file */
        if((0 == sStat.st_size) && (0 != sStat.st_rdev))
        {
            sRC = acpFileStatDevice(aFile, aStat);
        }
        else
        {
            sRC = ACP_RC_SUCCESS;
        }

        return sRC;
    }
}

/**
 * gets the acp_stat_t file information structure of the file at the @a aPath
 *
 * @param aPath the path of the file
 * @param aStat pointer to the acp_stat_t structure to be written
 * @param aTraverseLink whether follows the link or not,
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aPath or @a aStat points outside of the process' address space
 * @return #ACP_RC_EACCES
 *      The read access to @a aPath is not allowed,
 * @return #ACP_RC_ELOOP
 *      Too many symbolic links were encountered
 *      while resolving @a aOldPath or @a aNewPath
 * @return #ACP_RC_ENAMETOOLONG
 *      @a aPath is too long
 * @return #ACP_RC_ENFILE
 *      The system cannot open more files
 * @return #ACP_RC_ENOENT
 *      @a aPath does not exist
 * @return #ACP_RC_ENOMEM
 *      There is not enough memory to proceed
 */
ACP_EXPORT acp_rc_t acpFileStatAtPath(acp_char_t *aPath,
                                      acp_stat_t *aStat,
                                      acp_bool_t  aTraverseLink)
{
    acp_path_pool_t sPool;
    struct stat     sStat;
    acp_sint32_t    sRet;
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

    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);

    if(NULL == sPath)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        if (aTraverseLink == ACP_TRUE)
        {
            sRet = stat(sPath, &sStat);
        }
        else
        {
            sRet = lstat(sPath, &sStat);
        }

        if (sRet != 0)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            acpFileStatToAcpStat(&sStat, aStat);

            /* Once more for device file */
            if((0 == sStat.st_size) && (0 != sStat.st_rdev))
            {
                acp_file_t sFile;
                sRC = acpFileOpen(&sFile, sPath, ACP_O_RDONLY, 0);
                if(ACP_RC_IS_SUCCESS(sRC))
                {
                       sRC = acpFileStatDevice(&sFile, aStat);
                       (void)acpFileClose(&sFile);
                }
                else
                {
                       /* Do nothing
                        * We already know error code */
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
