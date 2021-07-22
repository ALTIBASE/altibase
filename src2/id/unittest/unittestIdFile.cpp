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
 
#include <idl.h>
#include <act.h>
#include <acp.h>

#define ERROR_HANDLE (PDL_HANDLE)-1

/*
 * idlOS::open() unittests
 */
void unittestIdlOsOpen()
{
    PDL_HANDLE    sFile;

    /*
     * Check file open
     */
    sFile = idlOS::open("unittestFile.txt", O_RDONLY);

    ACT_CHECK_DESC(sFile != ERROR_HANDLE, ("open(unittestFile.txt) couldn't open file."));

    idlOS::close(sFile);

    /*
     * Check file creation
     */
    sFile = idlOS::open("unittestFile-create.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    ACT_CHECK_DESC(sFile != ERROR_HANDLE, ("open(unittestFile-create.txt) couldn't open file."));
    if (sFile == ERROR_HANDLE)
    {
        ACT_ASSERT(idlOS::unlink("unittestFile-create.txt"));
    }

    idlOS::close(sFile);

    /*
     * Check open of the absent file
     */
    sFile = idlOS::open("NoFile.txt", O_RDONLY);

    ACT_CHECK_DESC(sFile == ERROR_HANDLE, ("Missed file opened correctly."));

    idlOS::close(sFile);
}

/*
 * Init environment before unlink test
 */
void unittestIdlOsUnlinkInit()
{
    PDL_HANDLE    sFile;

    sFile = idlOS::open("unittestUnlinkFile.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    idlOS::close(sFile);
}

/*
 * idlOS::unlink() unittests
 */
void unittestIdlOsUnlink()
{
    SInt sUnlinkResult;

    sUnlinkResult = idlOS::unlink("unittestUnlinkFile.txt");

    ACT_CHECK_DESC(sUnlinkResult != -1, ("Couldn't delete unittestUnlinkFile.txt."));

    sUnlinkResult = idlOS::unlink("NoFile.txt");

    ACT_CHECK_DESC(sUnlinkResult == -1, ("NoFile.txt deleted successfuly."));
}

SInt main()
{
    ACT_TEST_BEGIN();

    unittestIdlOsOpen();

    unittestIdlOsUnlinkInit();
    unittestIdlOsUnlink();

    ACT_TEST_END();

    return 0;
}
