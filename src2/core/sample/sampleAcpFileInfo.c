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
 * $Id:
 ******************************************************************************/

/* A mimic of ls */

#include <acpFile.h>
#include <acpPrintf.h>
#include <acpInit.h>
#include <acpDir.h>
#include <acpMem.h>

/* Print file size with respect to their size */
static void sampleAcpFileListPrintFileSize(acp_offset_t aSize)
{
    acp_uint64_t sSize = (acp_uint64_t)aSize;

    if(sSize > ACP_UINT64_LITERAL(0x40000000))
    {
        /* Gigabytes */
        (void)acpPrintf(" %4llu.%lluG ",
                        sSize / (1000000000),
                        (sSize % (1000000000)) / (100000000)
                       );
    }
    else if(sSize > (acp_uint64_t)(10000000))
    {
        (void)acpPrintf(" %6lluM ",
                        sSize / (1000000));
    }
    else if(sSize > (acp_uint64_t)(1000000))
    {
        (void)acpPrintf("  %llu.%3lluM ",
                        sSize / (1000000),
                        (sSize % (1000000)) / 1000
                       );
    }
    else if(sSize > (acp_uint64_t)99999)
    {
        (void)acpPrintf(" %6lluK ",
                        sSize / (1000));
    }
    else
    {
        (void)acpPrintf(" %6llu  ", sSize);
    }
}

static void sampleAcpFileListPrintFileInfo(
    acp_char_t* aFilename)
{
    acp_stat_t sStat;
    acp_rc_t sRC;

    sRC = acpFileStatAtPath(aFilename, &sStat, ACP_FALSE);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    /* Print file type */
    switch(sStat.mType)
    {
    case ACP_FILE_UNK:
        (void)acpPrintf("Unknown          ");
        break;
    case ACP_FILE_REG:
        (void)acpPrintf("File             ");
        break;
    case ACP_FILE_DIR:
        (void)acpPrintf("Directory        ");
        break;
    case ACP_FILE_CHR:
        (void)acpPrintf("Character device ");
        break;
    case ACP_FILE_BLK:
        (void)acpPrintf("Block device     ");
        break;
    case ACP_FILE_FIFO:
        (void)acpPrintf("Pipe             ");
        break;
    case ACP_FILE_LNK:
        (void)acpPrintf("Symbolic link    ");
        break;
    case ACP_FILE_SOCK:
        (void)acpPrintf("Socket           ");
        break;
    }

    /* Print permission */
    (void)acpPrintf("[");
    (void)acpPrintf((0 == (sStat.mPermission & 0400))? "-" : "r");
    (void)acpPrintf((0 == (sStat.mPermission & 0200))? "-" : "w");
    (void)acpPrintf((0 == (sStat.mPermission & 0100))? "-" : "x");
    (void)acpPrintf((0 == (sStat.mPermission &  040))? "-" : "r");
    (void)acpPrintf((0 == (sStat.mPermission &  020))? "-" : "w");
    (void)acpPrintf((0 == (sStat.mPermission &  010))? "-" : "x");
    (void)acpPrintf((0 == (sStat.mPermission &   04))? "-" : "r");
    (void)acpPrintf((0 == (sStat.mPermission &   02))? "-" : "w");
    (void)acpPrintf((0 == (sStat.mPermission &   01))? "-" : "x");
    (void)acpPrintf("]");

    /* print size */
    sampleAcpFileListPrintFileSize(sStat.mSize);
    (void)acpPrintf("%s\n", aFilename);
    return;

    ACP_EXCEPTION_END;
    {
        /* Display error code */
        (void)acpPrintf("Cannot stat file [%9d]         ",
                        (acp_sint32_t)sRC);
        (void)acpPrintf("%s\n", aFilename);
    }
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_rc_t     sRC;
    acp_sint32_t i;

    sRC = acpInitialize();
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    for(i = 1; i < aArgc; i++)
    {
        /* Print information about file */
        sampleAcpFileListPrintFileInfo(aArgv[i]);
    }

    (void)acpFinalize();

    return 0;


    ACP_EXCEPTION(PROG_FAIL)
    {
        /* Nothing to handle
         * Print error message */
        (void)acpPrintf("Cannot initialize application %s\n", aArgv[0]);
    }

    ACP_EXCEPTION_END;
        (void)acpFinalize();
    return 1;
}
