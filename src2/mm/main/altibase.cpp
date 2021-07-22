/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <idl.h>
#include <iduVersion.h>
#include <iduRevision.h>
#include <cmuVersion.h>
#include <smuVersion.h>
#include <sduVersion.h>
#include <qcm.h>
#include <rp.h>
#include <mmi.h>

int main(SInt argc, SChar *argv[])
{
    mmmPhase sPhase           = MMM_STARTUP_PRE_PROCESS;
    UInt     sOptionFlag      = 0;
    UInt     sOptCount        = 0;
    SInt     sOptVal;
    idBool   sDebugMode       = ID_FALSE;
    idBool   sPrintRevision1  = ID_FALSE;
    idBool   sPrintRevision2  = ID_FALSE;

    while ( ( sOptVal = idlOS::getopt( argc,
                                       argv,
                                       "a:p:d:f:nm:vzy" ) ) != EOF )
    {
        sOptCount++;

        switch(sOptVal)
        {
            case 'a':  // called from admin
                break;
            case 'p':  // 다단계 startup
                break;
            case 'd':  // 홈 디렉토리 명시
                //idlOS::strncpy(mmiMgr::mHomeDir, optarg, 255);
                break;
            case 'f':  // Conf 화일 명시
                //idlOS::strncpy(mmiMgr::mConfFile, optarg, 255);
                break;
            case 'n':
                sDebugMode = ID_TRUE;
                sPhase = MMM_STARTUP_SERVICE;
                break;
            case 'm':
                if (sDebugMode == ID_TRUE)
                {
                    // bug-27571: klocwork warnings
                    // add null check
                    if (optarg == NULL)
                    {
                        idlOS::printf("need phase value\n");
                        idlOS::exit(0);
                    }

                    sPhase = (mmmPhase)idlOS::atoi(optarg);

                    if ((sPhase < MMM_STARTUP_PRE_PROCESS) ||
                        (sPhase > MMM_STARTUP_SERVICE))
                    {
                        idlOS::printf("invalid phase value\n");
                        idlOS::exit(0);
                    }
                }
                break;
            case 'z':
                sPrintRevision1 = ID_TRUE;
                break;
            case 'y':
                if(sPrintRevision1 == ID_TRUE)
                {
                    sPrintRevision2 = ID_TRUE;
                }
                break;
            case 'v':
                idlOS::printf("version %s%s%s %s, "
                              "binary db version %s, "
                              "meta version %d.%d.%d, "
                              "cm protocol version %d.%d.%d, "
                              "replication protocol version %d.%d.%d, "
                              "shard version %d.%d.%d\n",
                              iduVersionString,
                              /* For open source */
                              " Open Edition ",
                              iduGetSystemInfoString(),
                              iduGetProductionTimeString(),
                              smVersionString,
                              QCM_META_MAJOR_VER,
                              QCM_META_MINOR_VER,
                              QCM_META_PATCH_VER,
                              CM_MAJOR_VERSION,
                              CM_MINOR_VERSION,
                              CM_PATCH_VERSION,
                              REPLICATION_MAJOR_VERSION,
                              REPLICATION_MINOR_VERSION,
                              REPLICATION_FIX_VERSION,
                              SHARD_MAJOR_VERSION,
                              SHARD_MINOR_VERSION,
                              SHARD_PATCH_VERSION);

                if((sPrintRevision1 == ID_TRUE) && (sPrintRevision2 == ID_TRUE))
                {
                    /*
                     * Print revision
                     */
                    idlOS::printf("Revision Information %s %d\n",
                                  ALTIBASE_SVN_URL, ALTIBASE_SVN_REVISION
                                  );
                }
                idlOS::fflush(NULL);

                idlOS::exit(0); /* just exit after print */
                break;

            default:
                idlOS::exit(0); /* just exit after print */
        }
    }

    if (sOptCount == 0)
    {
        idlOS::printf("Please use isql -sysdba\n");
        idlOS::exit(0);
    }

    /* fix PROJ-1749 */
    if ( sDebugMode == ID_TRUE )
    {
        sOptionFlag |= MMI_DEBUG_TRUE;
    }
    else
    {
        sOptionFlag |= MMI_DAEMON_TRUE;
    }

    sOptionFlag |= MMI_INNER_TRUE;
    sOptionFlag |= MMI_SIGNAL_TRUE;

    IDE_TEST( mmi::serverStart( sPhase, sOptionFlag ) != IDE_SUCCESS );

    idlOS::exit(0);

    IDE_EXCEPTION_END;
    {
        ideLog::logErrorMsg(IDE_SERVER_0);
    }

    idlOS::exit(-1);
    return 0;
}
