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
 * $Id: acpSym.c 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

#include <acpConfigPlatform.h>
#include <acpPath.h>
#include <acpSym.h>

#if defined(ALTI_CFG_OS_AIX)

/*
 * AIX XCOFF
 */

ACP_EXPORT acp_rc_t acpSymTableInit(acp_sym_table_t *aSymTable)
{
    acp_sint32_t sRet;

    sRet = loadquery(L_GETINFO,
                     aSymTable->mObjInfoBuffer,
                     (acp_uint32_t)sizeof(aSymTable->mObjInfoBuffer));

    if (sRet != -1)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_OS_ERROR();
    }
}

ACP_EXPORT acp_rc_t acpSymTableFinal(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t acpSymFindTracebackTable(acp_sym_table_t  *aSymTable,
                                             acp_sym_info_t   *aSymInfo,
                                             struct tbtable  **aTbTable,
                                             void             *aAddr)
{
    struct ld_info *sInfo = (struct ld_info *)aSymTable->mObjInfoBuffer;
    acp_uint32_t   *sPtr = aAddr;
    void           *sEnd = NULL;

    while (1)
    {
        sEnd = (acp_char_t *)sInfo->ldinfo_textorg + sInfo->ldinfo_textsize;

        if (((void *)sPtr >= sInfo->ldinfo_textorg) && ((void *)sPtr < sEnd))
        {
            acpStrSetConstCString(&aSymInfo->mFileName, sInfo->ldinfo_filename);
            aSymInfo->mFileAddr = sInfo->ldinfo_textorg;

            break;
        }
        else
        {
            /* do nothing */
        }

        if (sInfo->ldinfo_next == 0)
        {
            return ACP_RC_ENOENT;
        }
        else
        {
            /* do nothing */
        }

        sInfo = (struct ld_info *)((acp_char_t *)sInfo + sInfo->ldinfo_next);
    }

    while (1)
    {
        if ((void *)sPtr < sEnd)
        {
            if (*sPtr == 0)
            {
                break;
            }
            else
            {
                sPtr++;
            }
        }
        else
        {
            return ACP_RC_ENOENT;
        }
    }

    *aTbTable = (struct tbtable *)(sPtr + 1);

    return ACP_RC_SUCCESS;
}

ACP_INLINE void acpSymFillFuncInfo(acp_sym_info_t *aSymInfo,
                                   struct tbtable *aTbTable)
{
    acp_uint32_t *sPtr = (acp_uint32_t *)aTbTable +
        sizeof(struct tbtable_short) / sizeof(*sPtr);

    if ((aTbTable->tb.fixedparms != 0) || (aTbTable->tb.floatparms != 0))
    {
        sPtr++;
    }
    else
    {
        /* do nothing */
    }

    if (aTbTable->tb.has_tboff != 0)
    {
        aSymInfo->mFuncAddr = (acp_uint8_t *)aTbTable - *sPtr - sizeof(*sPtr);

        sPtr++;
    }
    else
    {
        /* do nothing */
    }

    if (aTbTable->tb.int_hndl != 0)
    {
        sPtr++;
    }
    else
    {
        /* do nothing */
    }

    if (aTbTable->tb.has_ctl != 0)
    {
        sPtr += (*sPtr) + 1;
    }
    else
    {
        /* do nothing */
    }

    if (aTbTable->tb.name_present != 0)
    {
        acpStrSetConstCStringWithLen(&aSymInfo->mFuncName,
                                     (acp_char_t *)sPtr + sizeof(acp_uint16_t),
                                     *(acp_uint16_t *)sPtr);
    }
    else
    {
        /* do nothing */
    }
}

ACP_EXPORT acp_rc_t acpSymFind(acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr)
{
    struct tbtable *sTbTable = NULL;
    acp_rc_t        sRC;

    acpStrSetConstCString(&aSymInfo->mFileName, NULL);
    acpStrSetConstCString(&aSymInfo->mFuncName, NULL);

    aSymInfo->mFileAddr = NULL;
    aSymInfo->mFuncAddr = NULL;

    sRC = acpSymFindTracebackTable(aSymTable, aSymInfo, &sTbTable, aAddr);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acpSymFillFuncInfo(aSymInfo, sTbTable);
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/* of defined(ALTI_CFG_OS_AIX) */
#elif defined(ALTI_CFG_OS_HPUX) ||               \
    defined(ALTI_CFG_OS_LINUX) ||                \
    defined(ALTI_CFG_OS_FREEBSD) ||                \
    defined(ALTI_CFG_OS_SOLARIS)

/*
 * ELF/SOM
 */

/**
 * initialize a symbol table
 *
 * @param aSymTable pointer to the symbol table object
 *
 * @return #ACP_RC_SUCCESS when successfully initialize object
 * @return system error code
 */
ACP_EXPORT acp_rc_t acpSymTableInit(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    return ACP_RC_SUCCESS;
}

/**
 * finalize a symbol table
 *
 * @param aSymTable pointer to the symbol table object
 *
 * @return #ACP_RC_SUCCESS when successfully finalize object
 * @return system error code
 */
ACP_EXPORT acp_rc_t acpSymTableFinal(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    return ACP_RC_SUCCESS;
}

#if defined(ALTI_CFG_OS_HPUX)

#if defined(ALTI_CFG_CPU_PARISC) && defined(ACP_CFG_COMPILE_32BIT)
#define ACP_SYM_SHL_START_INDEX 0
#else
#define ACP_SYM_SHL_START_INDEX -2
#endif
/* of defined(ALTI_CFG_CPU_PARISC) &&
 * defined(ACP_CFG_COMPILE_32BIT) */

/* Code For HPUX */
ACP_INLINE acp_rc_t acpSymFindFile(acp_sym_info_t *aSymInfo, void *aAddr)
{
    struct shl_descriptor *sDesc = NULL;
    acp_sint32_t           i;

    for (i = ACP_SYM_SHL_START_INDEX; shl_get(i, &sDesc) == 0; i++)
    {
        if ((aAddr >= (void *)sDesc->tstart) && (aAddr < (void *)sDesc->tend))
        {
            acpStrSetConstCString(&aSymInfo->mFileName, sDesc->filename);
            aSymInfo->mFileAddr = (void *)sDesc->tstart;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* do nothing */
        }
    }

    acpStrSetConstCString(&aSymInfo->mFileName, NULL);
    aSymInfo->mFileAddr = NULL;

    return ACP_RC_ENOENT;
}

#else
/* of defined(ALTI_CFG_OS_HPUX) */

/* Code for Linux and Solaris */
ACP_INLINE acp_rc_t acpSymFindFile(acp_sym_info_t *aSymInfo, void *aAddr)
{
    Dl_info sDlInfo;

    /* dladdr: takes a function pointer and tries to resolve */
    /* name and file where it is located. Information is stored in the */
    /* Dl_info structure */
    if (dladdr(aAddr, &sDlInfo))
    {
        /* Pathname of shared object that contains address */
        acpStrSetConstCString(&aSymInfo->mFileName, sDlInfo.dli_fname);
        /* Address at which shared object is loaded */
        aSymInfo->mFileAddr = sDlInfo.dli_fbase;

        return ACP_RC_SUCCESS;
    }
    else
    {
        acpStrSetConstCString(&aSymInfo->mFileName, NULL);
        aSymInfo->mFileAddr = NULL;

        return ACP_RC_ENOENT;
    }
}

#endif
/* of defined(ALTI_CFG_OS_HPUX) */

#define ACP_SYM_SURE(aExpr)                                     \
    do                                                          \
    {                                                           \
        if ((aExpr))                                            \
        {                                                       \
        }                                                       \
        else                                                    \
        {                                                       \
            acpStrSetConstCString(&aSymInfo->mFuncName, NULL);  \
            aSymInfo->mFuncAddr = NULL;                         \
            return;                                             \
        }                                                       \
    } while (0)

#define ACP_SYM_SEEK(aFD, aOffset)                                      \
    ACP_SYM_SURE(lseek((aFD), (aOffset), SEEK_SET) == (off_t)(aOffset))

#define ACP_SYM_READ(aFD, aBuffer, aLen)                        \
    ACP_SYM_SURE(read((aFD), (aBuffer), (aLen)) == (ssize_t)(aLen))

#if defined(ALTI_CFG_OS_HPUX) &&                 \
    defined(ALTI_CFG_CPU_PARISC) &&              \
    defined(ACP_CFG_COMPILE_32BIT)

/*
 * SOM
 */

/* Code for HPUX */
ACP_INLINE acp_bool_t acpSymMatchEntry(struct symbol_dictionary_record *aSymbol,
                                       acp_uint32_t                    *aEntry,
                                       acp_size_t                      *aOffset,
                                       void                           *aMapAddr,
                                       void                            *aAddr)
{
    acp_byte_t *sSymAddr = NULL;

    if (((aSymbol->symbol_type == ST_CODE) ||
         (aSymbol->symbol_type == ST_ENTRY)) &&
        (aSymbol->symbol_scope == SS_UNIVERSAL))
    {
        sSymAddr = (acp_byte_t *)aMapAddr +
            (aSymbol->symbol_value & 0x3ffffffc);

        if ((acp_byte_t *)aAddr == sSymAddr)
        {
            *aEntry  = aSymbol->n_offset;
            *aOffset = 0;

            return ACP_TRUE;
        }
        else
        {
            /* do nothing */
        }

        if (((acp_byte_t *)aAddr > sSymAddr) &&
            ((acp_size_t)((acp_byte_t *)aAddr - sSymAddr) < *aOffset))
        {
            *aEntry  = aSymbol->n_offset;
            *aOffset = (acp_byte_t *)aAddr - sSymAddr;
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

    return ACP_FALSE;
}

/* Code for HPUX */
ACP_INLINE void acpSymFindFunc(acp_sint32_t     aFD,
                               acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr)
{
    struct header filehdr;
    struct som_exec_auxhdr auxhdr;
    struct symbol_dictionary_record sSymbol;
    void *sMapAddr = NULL;
    acp_uint32_t sEntry;
    acp_size_t sOffset;
    acp_size_t i;

    ACP_SYM_READ(aFD, &filehdr, sizeof(filehdr));

    ACP_SYM_SURE(filehdr.symbol_total != 0);

    ACP_SYM_READ(aFD, &auxhdr, sizeof(auxhdr));

    ACP_SYM_SURE(auxhdr.som_auxhdr.type == HPUX_AUX_ID);

    sMapAddr = (acp_byte_t *)aSymInfo->mFileAddr -
               (acp_byte_t *)auxhdr.exec_tmem;
    sEntry = (acp_uint32_t)-1;
    sOffset = (acp_size_t)-1;

    ACP_SYM_SEEK(aFD, filehdr.symbol_location);

    for (i = 0; i < filehdr.symbol_total; i++)
    {
        ACP_SYM_READ(aFD, &sSymbol, sizeof(sSymbol));

        if (acpSymMatchEntry(&sSymbol,
                             &sEntry,
                             &sOffset,
                             sMapAddr,
                             aAddr) == ACP_TRUE)
        {
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    ACP_SYM_SURE(sEntry != (acp_uint32_t)-1);

    ACP_SYM_SEEK(aFD, filehdr.symbol_strings_location + sEntry);

    ACP_SYM_SURE(read(aFD,
                      aSymTable->mSymNameBuffer,
                      sizeof(aSymTable->mSymNameBuffer)) > 0);

    acpStrSetConstCString(&aSymInfo->mFuncName, aSymTable->mSymNameBuffer);
    aSymTable->mSymNameBuffer[sizeof(aSymTable->mSymNameBuffer) - 1] = 0;

    aSymInfo->mFuncAddr = (acp_size_t)aAddr - sOffset;
}

#else
/* of defined(ALTI_CFG_OS_HPUX) && 
 * defined(ALTI_CFG_CPU_PARISC) && 
 * defined(ACP_CFG_COMPILE_32BIT) */

/*
 * ELF
 */

#if defined(ACP_CFG_COMPILE_64BIT)
typedef Elf64_Sym  acpSymElf_Sym;
typedef Elf64_Ehdr acpSymElf_Ehdr;
typedef Elf64_Shdr acpSymElf_Shdr;
typedef Elf64_Word acpSymElf_Word;
typedef Elf64_Addr acpSymElf_Addr;

#define ACP_SYM_ELF_WORD_NONE \
    ((acpSymElf_Word)ACP_UINT64_LITERAL(0xFFFFFFFFFFFFFFFF))

#define ACP_SYM_ELF_ST_TYPE(a) ELF64_ST_TYPE(a)
#else /* of defined(ACP_CFG_COMPILE_64BIT) */
typedef Elf32_Sym  acpSymElf_Sym;
typedef Elf32_Ehdr acpSymElf_Ehdr;
typedef Elf32_Shdr acpSymElf_Shdr;
typedef Elf32_Word acpSymElf_Word;
typedef Elf32_Addr acpSymElf_Addr;
#define ACP_SYM_ELF_ST_TYPE(a) ELF32_ST_TYPE(a)

#define ACP_SYM_ELF_WORD_NONE ((acpSymElf_Word)(0xFFFFFFFF))

#endif /* of defined(ACP_CFG_COMPILE_64BIT) */

/* Code for Linux, Solaris */
ACP_INLINE acpSymElf_Word acpSymMatchEntry(acpSymElf_Sym  *aSymbol,
                                           acp_sym_info_t *aSymInfo,
                                           void           *aMapAddr,
                                           void           *aAddr)
{
    acp_byte_t *sSymAddr = NULL;

    if (ACP_SYM_ELF_ST_TYPE(aSymbol->st_info) == STT_FUNC)
    {
#if defined(ALTI_CFG_OS_HPUX)
#if defined(ACP_CFG_COMPILE_64BIT)
        sSymAddr = (acp_byte_t *)aMapAddr +
                   (aSymbol->st_value & 0x3ffffffffffffffcL);
#else  /* of defined(ACP_CFG_COMPILE_64BIT) */
        sSymAddr = (acp_byte_t *)aMapAddr + (aSymbol->st_value & 0x3fffffc);
#endif /* of defined(ACP_CFG_COMPILE_64BIT) */
#else  /* of defined(ALTI_CFG_OS_HPUX) */
        sSymAddr = (acp_byte_t *)aMapAddr + aSymbol->st_value;
#endif /* of defined(ALTI_CFG_OS_HPUX) */

        if (((acp_byte_t *)aAddr >= sSymAddr) &&
            ((acp_byte_t *)aAddr < sSymAddr + aSymbol->st_size))
        {
            aSymInfo->mFuncAddr = sSymAddr;

            return aSymbol->st_name;
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

    return ACP_SYM_ELF_WORD_NONE;
}

/* Code for Linux, Solaris */
ACP_INLINE void acpSymFindFunc(acp_sint32_t     aFD,
                               acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr)
{
    acpSymElf_Ehdr  sElfHdr;
    acpSymElf_Shdr  sElfSectionHdr;
    acpSymElf_Shdr  sSymSectionHdr;
    acpSymElf_Sym   sSymbol;
    acpSymElf_Word  sStringSection;
    acpSymElf_Word  sStringTabOffset;
    acpSymElf_Word  sEntry;
    void           *sMapAddr = aSymInfo->mFileAddr;
    acp_size_t      sSymCount;
    acp_size_t      i;

    /* read header of ELF */
    ACP_SYM_READ(aFD, &sElfHdr, sizeof(sElfHdr));

#if defined(ALTI_CFG_OS_LINUX) || \
    defined(ALTI_CFG_OS_SOLARIS) || \
    defined(ALTI_CFG_OS_FREEBSD)
    if (sElfHdr.e_type == ET_EXEC)
    {
        sMapAddr = NULL;
    }
    else
    {
        /* do nothing */
    }
#endif
/* of defined(ALTI_CFG_OS_LINUX) ||
 * defined(ALTI_CFG_OS_SOLARIS) */

    ACP_SYM_SURE(sElfHdr.e_shnum != 0);

    ACP_SYM_SURE((acp_size_t)sElfHdr.e_shentsize <= sizeof(sElfSectionHdr));

    ACP_SYM_SEEK(aFD, sElfHdr.e_shoff);

    sStringSection = 0;

    for (i = 0; i < (acp_size_t)sElfHdr.e_shnum; i++)
    {
        ACP_SYM_READ(aFD, &sElfSectionHdr, sElfHdr.e_shentsize);

        if (sElfSectionHdr.sh_type == SHT_SYMTAB)
        {
            acpMemCpy(&sSymSectionHdr, &sElfSectionHdr, sizeof(sElfSectionHdr));
            sStringSection = sElfSectionHdr.sh_link;
        }
        else
        {
            /* do nothing */
        }
    }

    ACP_SYM_SURE((sStringSection != 0) && (sStringSection < sElfHdr.e_shnum));

    ACP_SYM_SEEK(aFD, sElfHdr.e_shoff);

    for (i = 0; i < sStringSection; i++)
    {
        ACP_SYM_READ(aFD, &sElfSectionHdr, sElfHdr.e_shentsize);
    }

    ACP_SYM_READ(aFD, &sElfSectionHdr, sElfHdr.e_shentsize);

    ACP_SYM_SURE(sElfSectionHdr.sh_size != 0);

    sStringTabOffset = sElfSectionHdr.sh_offset;

    sSymCount = sSymSectionHdr.sh_size / sSymSectionHdr.sh_entsize;

    sEntry = ACP_SYM_ELF_WORD_NONE;

    ACP_SYM_SEEK(aFD, sSymSectionHdr.sh_offset);

    for (i = 0; (i < sSymCount) && (sEntry == ACP_SYM_ELF_WORD_NONE); i++)
    {
        ACP_SYM_READ(aFD, &sSymbol, sizeof(acpSymElf_Sym));

        sEntry = acpSymMatchEntry(&sSymbol, aSymInfo, sMapAddr, aAddr);
    }

    ACP_SYM_SURE(sEntry != ACP_SYM_ELF_WORD_NONE);

    ACP_SYM_SEEK(aFD, sStringTabOffset + sEntry);

    ACP_SYM_SURE(read(aFD,
                      aSymTable->mSymNameBuffer,
                      sizeof(aSymTable->mSymNameBuffer)) > 0);

    acpStrSetConstCString(&aSymInfo->mFuncName, aSymTable->mSymNameBuffer);
    aSymTable->mSymNameBuffer[sizeof(aSymTable->mSymNameBuffer) - 1] = 0;
}

#endif
/* of defined(ALTI_CFG_OS_HPUX) &&
 * defined(ALTI_CFG_CPU_PARISC) &&
 * defined(ACP_CFG_COMPILE_32BIT) */

ACP_INLINE acp_sint32_t acpSymOpenLink(acp_char_t* aAbsolutePath)
{
    acp_sint32_t sPathSize;
    acp_sint32_t sFD;
    /* Get Absolute path for LINUX */
    sPathSize = readlink("/proc/self/exe", aAbsolutePath, 1023);

    if (sPathSize != -1)
    {
        /* Must append NULL at the end of path */
        aAbsolutePath[sPathSize] = 0;
        sFD = open(aAbsolutePath, O_RDONLY);
    }
    else
    {
        /* Cannot determine the path. Give up finding */
        sFD = -1;
    }

    return sFD;
}

static acp_sint32_t acpSymFindPath(acp_sym_info_t *aSymInfo,
                                   acp_char_t*     aAbsolutePath)
{
    /* Get Absolute path for HPUX, Solaris and
     * for Linux when /proc/self/exe is errornous */

    acp_bool_t    sCont = ACP_TRUE;
    acp_char_t   *sPath = "";
    acp_sint32_t  sPIndex;
    acp_sint32_t  sAIndex;
    acp_sint32_t  sCIndex;
    acp_sint32_t  sFD;
    acp_size_t    sLength = acpStrGetLength(&aSymInfo->mFileName);
    acp_str_t     sPathStr = ACP_STR_CONST(sPath);
    acp_rc_t      sSearchRC;

    ACP_TEST(sLength > 0);

    /* Get PATH environment variable */
    sPath = getenv("PATH");
    if(NULL == sPath)
    {
        /* The PATH env.var. is NULL. Unbelivable! */
        sPath = "";
    }
    else
    {
        /* Do nothing */
    }

    /* Explore each path - Is there no other smart way? :( */
    sFD = -1;
    /* 
     * for the first search,
     * finding index should be initialized
     * with ACP_STR_INDEX_INITIALIZER 
     */
    sPIndex = ACP_STR_INDEX_INITIALIZER;

    while((ACP_TRUE == sCont) && (sFD < 0))
    {
        sPathStr.mString = sPath;
        sSearchRC = acpStrFindChar(
            &sPathStr, ':', &sCIndex, sPIndex, 0
            );

        if(ACP_RC_IS_SUCCESS(sSearchRC))
        {
            /* The seperator ':' found in PATH */
            sAIndex = sCIndex - sPIndex;
        }
        else if(ACP_RC_IS_ENOENT(sSearchRC))
        {
            /* End of path was reached */
            /* sAIndex can be ACP_PATH_MAX_LENGTH */
            sAIndex = (acp_sint32_t)acpCStrLen(
                sPath, ACP_PATH_MAX_LENGTH - 1);
            sCont = ACP_FALSE;
        }
        else
        {
            break;
        }

        if((sAIndex + sLength) >= ACP_PATH_MAX_LENGTH)
        {
            continue;
        }
        else
        {
            acpMemCpy(aAbsolutePath, sPath, sAIndex);
            aAbsolutePath[sAIndex] = '/';

            acpMemCpy(&aAbsolutePath[sAIndex],
                      acpStrGetBuffer(&aSymInfo->mFileName),
                      sLength);
            aAbsolutePath[sAIndex + sLength] = '\0';

            sFD = open(aAbsolutePath, O_RDONLY);
        }

        sPath += sAIndex + 1;
    }

    if (sFD >= 0)
    {
        /* Full path of excutable found
         * Use found path instead of sole excutable name */
        acpStrSetConstCString(&aSymInfo->mFileName,
                              aAbsolutePath);
    }
    else
    {
        /* do nothing */
    }

    return sFD;

    ACP_EXCEPTION_END;
    return -1;
}

/* Code for HPUX, Linux, Solaris */


/** 
 * find a symbol and fill out the symbol information
 *
 * This function do search dynamic-loaded libraries for @a aAddr
 * (function address). If success, @a aSymInfo has the path of the
 * library that contains address and function name and so on.
 * @a aSymTable is used only for internal purpose, do not use it at
 * your own code.
 * 
 * @param aSymTable symbol table object
 * @param aSymInfo store information of the symbol
 * @param aAddr pointer to the symbol
 * 
 * @return #ACP_RC_SUCCESS when successfully finalize object
 * @return system error code
 */
ACP_EXPORT acp_rc_t acpSymFind(acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr)/* function addr */
{
    acp_char_t   sAbsolutePath[ACP_PATH_MAX_LENGTH];
    acp_sint32_t sFD;
    acp_rc_t     sRC;

    /* Get symbol information such as Filename and symbol address
     * To use this, you must compile with -rdynamic gcc option :
     *  defined at /core/makefiles/platform.mk.in */
    sRC = acpSymFindFile(aSymInfo, aAddr);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        /* open a file which includes the function,
           that aAddr points */
        sFD = open(acpStrGetBuffer(&aSymInfo->mFileName), O_RDONLY);

#if defined(ALTI_CFG_OS_LINUX)  || defined(ALTI_CFG_OS_FREEBSD)
        if (sFD < 0)
        {
            sFD = acpSymOpenLink(sAbsolutePath);
        }
        else
        {
            /* Cannot still find out a full path */
        }
#endif /* of ALTI_CFG_OS_LINUX */

        /* BUG-20110 If cannot open, try get full path of excutable */
        if (sFD < 0)
        {
            sFD = acpSymFindPath(aSymInfo, sAbsolutePath);
        }
        else
        {
            /* Do nothing */
            /* We already know the path of the excutable */
        }

        /* To open a file is successed,
           then read information of the function from ELF headers */
        if (sFD >= 0)
        {
            acpSymFindFunc(sFD, aSymTable, aSymInfo, aAddr);
            (void)close(sFD);
        }
        else
        {
            acpStrSetConstCString(&aSymInfo->mFuncName, NULL);
            aSymInfo->mFuncAddr = NULL;
        }
    }
    else
    {
        /*
         * fail to read a file
         */
        acpStrSetConstCString(&aSymInfo->mFuncName, NULL);
        aSymInfo->mFuncAddr = NULL;
    }

    return sRC;
}

#elif defined(ALTI_CFG_OS_TRU64)
/* of defined(ALTI_CFG_OS_HPUX) ||
 * defined(ALTI_CFG_OS_LINUX) ||
 * defined(ALTI_CFG_OS_SOLARIS) */

/*
 * TRU64 COFF
 */

ACP_EXPORT acp_rc_t acpSymTableInit(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSymTableFinal(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSymFind(acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr)
{
    Dl_info sDlInfo;

    ACP_UNUSED(aSymTable);

    if (_rld_new_interface(_RLD_DLADDR, aAddr, &sDlInfo))
    {
        acpStrSetConstCString(&aSymInfo->mFileName, sDlInfo.dli_fname);
        acpStrSetConstCString(&aSymInfo->mFuncName, sDlInfo.dli_sname);

        aSymInfo->mFileAddr = sDlInfo.dli_fbase;
        aSymInfo->mFuncAddr = sDlInfo.dli_saddr;

        return ACP_RC_SUCCESS;
    }
    else
    {
        acpStrSetConstCString(&aSymInfo->mFileName, NULL);
        acpStrSetConstCString(&aSymInfo->mFuncName, NULL);

        aSymInfo->mFileAddr = NULL;
        aSymInfo->mFuncAddr = NULL;

        return ACP_RC_ENOENT;
    }
}

#elif defined(ALTI_CFG_OS_WINDOWS)

/*
 * Code for windows
 * Using ImageHLP.dll
 */
ACP_EXPORT acp_rc_t acpSymTableInit(acp_sym_table_t *aSymTable)
{
    IMAGEHLP_MODULE64 *sModule = NULL;
    SYMBOL_INFO       *sSymbol = NULL;
    BOOL               sRet;

    /*
     * BUGBUG: SymInitialize() must not be called recursively.
     *          -> I don't understand 'called recursively'.
     *             There is no recursive call for SymInitialize
     * SOLUTION: (1) synchronize symbol table access
     *           (2) duplicate the current process handle and SymInitialize (?)
     */

    sRet = SymInitialize(GetCurrentProcess(), NULL, TRUE);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    /* These symbols will be passed to SymGetModuleInfo64 */
    sModule = &aSymTable->mModule;
    sSymbol = (SYMBOL_INFO *)aSymTable->mSymbolBuffer;

    acpMemSet(sModule, 0, sizeof(aSymTable->mModule));
    acpMemSet(sSymbol, 0, sizeof(aSymTable->mSymbolBuffer));

    sModule->SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    sSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    sSymbol->MaxNameLen   = ACP_SYM_NAME_LEN_MAX + 1;

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSymTableFinal(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    (void)SymCleanup(GetCurrentProcess());

    return ACP_RC_SUCCESS;
}

/*
 * Find Symbol in symbol table
 * To use this, this source must be compiled in debug mode.
 * No solution without /DEBUG option yet.
 */
ACP_EXPORT acp_rc_t acpSymFind(acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr)
{
    IMAGEHLP_MODULE64 *sModule = &aSymTable->mModule;
    SYMBOL_INFO       *sSymbol = (SYMBOL_INFO *)aSymTable->mSymbolBuffer;
    BOOL               sRet;

    sRet = SymGetModuleInfo64(GetCurrentProcess(), (DWORD64)aAddr, sModule);

    if (sRet == 0)
    {
        acpStrSetConstCString(&aSymInfo->mFileName, NULL);
        aSymInfo->mFileAddr = NULL;
    }
    else
    {
        acpStrSetConstCString(&aSymInfo->mFileName, sModule->LoadedImageName);
        aSymInfo->mFileAddr = (void *)(acp_ulong_t)sModule->BaseOfImage;
    }

    sRet = SymFromAddr(GetCurrentProcess(), (DWORD64)aAddr, NULL, sSymbol);

    if (sRet == 0)
    {
        acpStrSetConstCString(&aSymInfo->mFuncName, NULL);
        aSymInfo->mFuncAddr = NULL;
    }
    else
    {
        acpStrSetConstCString(&aSymInfo->mFuncName, sSymbol->Name);
        aSymInfo->mFuncAddr = (void *)(acp_ulong_t)sSymbol->Address;
    }

    return ACP_RC_SUCCESS;
}

#else /* of defined(ALTI_CFG_OS_WINDOWS) */

ACP_EXPORT acp_rc_t acpSymTableInit(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    return ACP_RC_ENOIMPL;
}

ACP_EXPORT acp_rc_t acpSymTableFinal(acp_sym_table_t *aSymTable)
{
    ACP_UNUSED(aSymTable);

    return ACP_RC_ENOIMPL;
}

ACP_EXPORT acp_rc_t acpSymFind(acp_sym_table_t *aSymTable,
                               acp_sym_info_t  *aSymInfo,
                               void            *aAddr)
{
    ACP_UNUSED(aSymTable);
    ACP_UNUSED(aAddr);

    acpStrSetConstCString(&aSymInfo->mFileName, NULL);
    acpStrSetConstCString(&aSymInfo->mFuncName, NULL);

    aSymInfo->mFileAddr = NULL;
    aSymInfo->mFuncAddr = NULL;

    return ACP_RC_ENOIMPL;
}

#endif /* of #else */
