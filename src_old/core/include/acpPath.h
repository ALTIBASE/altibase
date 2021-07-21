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
 * $Id: acpPath.h 10068 2010-02-19 02:26:39Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_PATH_H_)
#define _O_ACP_PATH_H_

/**
 * @file
 * @ingroup CoreChar
 * @ingroup CoreFile
 *
 * Altibase Core Filesystem Path Utilities
 */
/**
 * @example sampleAcpPath.c
 */

#include <acpCStr.h>
#include <acpMem.h>
#include <acpList.h>

ACP_EXTERN_C_BEGIN


#if defined(ALTI_CFG_OS_WINDOWS)
#define ACP_PATH_IS_DIR_SEP(aChar) (((aChar) == '/') || ((aChar) == '\\'))
#else
#define ACP_PATH_IS_DIR_SEP(aChar) ((aChar) == '/')
#endif

#if defined(PATH_MAX)
#define ACP_PATH_MAX_LENGTH       (PATH_MAX) /* maximum length of path */
#else
#define ACP_PATH_MAX_LENGTH       (1024) /* maximum length of path */
#endif

/**
 * path memory pool
 */
typedef struct acp_path_pool_t
{
    acp_list_t mList;  /* list of allocated path buffer */
} acp_path_pool_t;


/**
 * initializes a path pool
 * @param aPool a path pool to initialize
 */
ACP_INLINE void acpPathPoolInit(acp_path_pool_t *aPool)
{
    acpListInit(&aPool->mList);
}

/**
 * finalizes a path pool
 * @param aPool a path pool to finalize
 */
ACP_INLINE void acpPathPoolFinal(acp_path_pool_t *aPool)
{
    acp_list_node_t *sIterator;
    acp_list_node_t *sNextNode;

    ACP_LIST_ITERATE_SAFE(&aPool->mList, sIterator, sNextNode)
    {
        acpMemFree(sIterator);
    }
}


ACP_INLINE acp_char_t *acpPathAlloc(acp_path_pool_t *aPool, acp_size_t aSize)
{
    acp_list_node_t *sNode;
    acp_rc_t         sRC;

    sRC = acpMemAlloc((void **)&sNode, sizeof(acp_list_node_t) + aSize);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return NULL;
    }
    else
    {
        acpListAppendNode(&aPool->mList, sNode);

        return (acp_char_t *)sNode + sizeof(acp_list_node_t);
    }
}

ACP_INLINE acp_char_t *acpPathMake(acp_char_t      *aStr,
                                   acp_path_pool_t *aPool,
                                   acp_sint32_t     aLocation,
                                   acp_sint32_t     aLength,
                                   acp_bool_t       aChangeDirSep)
{
    if ((aStr[aLocation + aLength] == '\0')
#if defined(ALTI_CFG_OS_WINDOWS)
        && (aChangeDirSep == ACP_FALSE)
#endif
        )
    {
        return aStr + aLocation;
    }
    else
    {
        acp_char_t *sPath = acpPathAlloc(aPool, aLength + 1);

        if (sPath != NULL)
        {
            acpMemCpy(sPath, aStr + aLocation, aLength);

            sPath[aLength] = '\0';

#if defined(ALTI_CFG_OS_WINDOWS)
            if (aChangeDirSep == ACP_TRUE)
            {
                acp_sint32_t i;

                for (i = 0; i < aLength; i++)
                {
                    if (sPath[i] == '/')
                    {
                        sPath[i] = '\\';
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
            }
            else
            {
                /* do nothing */
            }
#else
            aChangeDirSep = aChangeDirSep;
#endif
        }
        else
        {
            /* do nothing */
        }

        return sPath;
    }
}

/**
 * returns a filesystem-specific representation
 * @param aStr pointer to the string object
 * @param aPool path pool
 * @return a filesystem-specific representation of @a aStr
 * as C style null-terminated string
 */
ACP_INLINE acp_char_t *acpPathFull(acp_char_t      *aStr,
                                   acp_path_pool_t *aPool)
{
    if (aStr == NULL)
    {
        return (acp_char_t*)("");
    }
    else
    {
        acp_sint32_t sLength = (acp_sint32_t)acpCStrLen(
            aStr, ACP_PATH_MAX_LENGTH);
        return acpPathMake(aStr, aPool, 0, sLength, ACP_TRUE);
    }
}

/**
 * returns the directory part of path without trailing '/'
 * @param aStr pointer to the string object
 * @param aPool path pool
 * @return the directory part of @a aStr as C style null-terminated string
 *
 * it returns "" if the directory part of @a aStr is root directory.
 * it returns "." if @a aStr does not have a directory part.
 */
ACP_INLINE acp_char_t *acpPathDir(acp_char_t      *aStr,
                                  acp_path_pool_t *aPool)
{
    acp_sint32_t sLength = (acp_sint32_t)acpCStrLen(aStr, ACP_PATH_MAX_LENGTH);
    acp_sint32_t i;

    for (i = sLength - 1; i >= 0; i--)
    {
        if (ACP_PATH_IS_DIR_SEP(aStr[i]))
        {
            if (i == 0)
            {
                return (acp_char_t*)("");
            }
            else
            {
                return acpPathMake(aStr, aPool, 0, i, ACP_TRUE);
            }
        }
        else
        {
            /* continue */
        }
    }

    return (acp_char_t*)(".");
}

/**
 * returns the last path component
 * @param aStr pointer to the string object
 * @param aPool path pool
 * @return the last path component of @a aStr as C style null-terminated string
 */
ACP_INLINE acp_char_t *acpPathLast(acp_char_t      *aStr,
                                   acp_path_pool_t *aPool)
{
#if defined(__STATIC_ANALYSIS_DOING__)

    acp_sint32_t sLength = (acp_sint32_t)acpCStrLen(aStr, ACP_PATH_MAX_LENGTH);
    acp_sint32_t i;
    acp_char_t   sTmpBuf[ACP_PATH_MAX_LENGTH + 1] = {0};

    if(0 == sLength)
    {
        return aStr;
    }
    else
    {
        if (acpCStrCpy(sTmpBuf,
            ACP_PATH_MAX_LENGTH,
            aStr, sLength) != ACP_RC_SUCCESS)
        {
            return aStr;
        }
        else
        {
            /* Continue */
        }

        if (ACP_PATH_IS_DIR_SEP(sTmpBuf[sLength - 1]))
        {
            sLength--;
        }
        else
        {
            /* do nothing */
        }
   
        for (i = sLength - 1; i >= 0; i--)
        {
            if (ACP_PATH_IS_DIR_SEP(sTmpBuf[i]))
            {
                return acpPathMake(aStr,
                                   aPool,
                                   i + 1,
                                   sLength - i - 1,
                                   ACP_FALSE);
            }
            else
            {
                /* continue */
            }
        }
    }

    return acpPathMake(aStr, aPool, 0, sLength, ACP_FALSE);

#else

    acp_sint32_t sLength = (acp_sint32_t)acpCStrLen(aStr, ACP_PATH_MAX_LENGTH);
    acp_sint32_t i;

    if(0 == sLength)
    {
        return aStr;
    }
    else
    {
        if (ACP_PATH_IS_DIR_SEP(aStr[sLength - 1]))
        {
            sLength--;
        }
        else
        {
            /* do nothing */
        }
   
        for (i = sLength - 1; i >= 0; i--)
        {
            if (ACP_PATH_IS_DIR_SEP(aStr[i]))
            {
                return acpPathMake(aStr,
                                   aPool,
                                   i + 1,
                                   sLength - i - 1,
                                   ACP_FALSE);
            }
            else
            {
                /* continue */
            }
        }
    }

    return acpPathMake(aStr, aPool, 0, sLength, ACP_FALSE);

#endif
}

/**
 * returns the path without extension
 * @param aStr pointer to the string object
 * @param aPool path pool
 * @return the path without extension from @a aStr
 * as C style null-terminated string
 */
ACP_INLINE acp_char_t *acpPathBase(acp_char_t      *aStr,
                                   acp_path_pool_t *aPool)
{
    acp_sint32_t sLength = (acp_sint32_t)acpCStrLen(aStr, ACP_PATH_MAX_LENGTH);
    acp_sint32_t i;

    for (i = sLength - 1; i >= 0; i--)
    {
        if (ACP_PATH_IS_DIR_SEP(aStr[i]))
        {
            break;
        }
        else
        {
            /* continue */
        }

        if (aStr[i] == '.')
        {
            return acpPathMake(aStr, aPool, 0, i, ACP_TRUE);
        }
        else
        {
            /* continue */
        }
    }

    return acpPathMake(aStr, aPool, 0, sLength, ACP_TRUE);
}

/**
 * returns the path extension
 * @param aStr pointer to the string object
 * @param aPool path pool
 * @return the path extension of @a aStr as C style null-terminated string
 */
ACP_INLINE acp_char_t *acpPathExt(acp_char_t      *aStr,
                                  acp_path_pool_t *aPool)
{
    acp_sint32_t sLength = (acp_sint32_t)acpCStrLen(aStr, ACP_PATH_MAX_LENGTH);
    acp_sint32_t i;

    for (i = sLength - 1; i >= 0; i--)
    {
        if (ACP_PATH_IS_DIR_SEP(aStr[i]))
        {
            break;
        }
        else
        {
            /* continue */
        }

        if (aStr[i] == '.')
        {
            return acpPathMake(aStr, aPool, i + 1, sLength - i - 1, ACP_FALSE);
        }
        else
        {
            /* continue */
        }
    }

    return (acp_char_t*)("");
}

/**
 * returns a system-specific execution file path
 * For Windows(TM), adds ".exe" in the end of aStr
 * For Unix, identical to #acpPathFull.
 * If the file path has already ".exe", the file path is returned as it is.
 * @param aStr pointer to the string object
 * @param aPool path pool
 * @return a system-specific execution file path of @a aStr
 * as C style null-terminated string
 */
ACP_INLINE acp_char_t* acpPathMakeExe(acp_char_t*       aStr,
                                      acp_path_pool_t*  aPool)
{
    acp_char_t* sPath = acpPathFull(aStr, aPool);
#if defined(ALTI_CFG_OS_WINDOWS)
    if(NULL != sPath)
    {
        acp_sint32_t sLength;

        /* already has "exe" extension? */
        if (acpCStrCmp(acpPathExt(aStr, aPool), "exe", 3) != 0)
        {
            sLength = (acp_sint32_t)acpCStrLen(sPath, ACP_PATH_MAX_LENGTH);
            /* Extend buffer to contain ".exe" */
            sLength += 4;

            if(sLength > ACP_PATH_MAX_LENGTH)
            {
                /* Path length exceeds max path length */
                sPath = NULL;
            }
            else
            {
                acp_char_t* sExePath = acpPathAlloc(aPool, sLength + 1);
                (void)acpCStrCpy(sExePath, sLength + 1, sPath, sLength - 4 + 1);
                (void)acpCStrCat(sExePath, sLength + 1, ".exe", 4);

                sPath = sExePath;
            }
        }
        else
        {
            /* The file path has already "exe" extension,
             * it is returned as it is */
        }
    }
    else
    {
        /* Return of NULL */
    }
#else
    /* For unix, do nothing */
#endif

    return sPath;
}

ACP_EXTERN_C_END


#endif
