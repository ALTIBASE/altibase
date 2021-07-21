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
#include <idn.h>
#include <smi.h>
#include <smErrorCode.h>
#include <qci.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmtServiceThread.h>
#include <mmuProperty.h>
#include <sdiZookeeper.h>

static IDE_RC doCreateDatabase(idvSQL * aStatistics, qciArgCreateDB *aArg)
{
    SChar    *sDBName;
    UInt      sDBNameLen;
    scPageID  sCreatePageCountExeptMeta;
    scPageID  sCreatePageCount;
    SChar     sOutputMsg[256];
    scPageID  sHLimitPageCnt;
    scPageID  sLLimitPageCnt;
    UInt      sShmDBKey;
    UInt      i = 0;

    /* ---------------------------
     *  DB 화일명 길이 검사
     * --------------------------*/
    sDBName = mmuProperty::getDbName();

    sDBNameLen = idlOS::strlen(sDBName);
    IDE_TEST_RAISE((sDBNameLen == 0) || (sDBNameLen >= SM_MAX_DB_NAME),
                   DBNameLengthError);

    // PROJ-1579 NCHAR
    // DB 이름으로 ASCII 이외의 문자가 올 수 없다.
    for( i = 0; i < sDBNameLen; i++ )
    {
        IDE_TEST_RAISE( IDN_IS_ASCII(sDBName[i]) == 0, DBNameAsciiError);
    }

    // fix BUG-10470 DBName 검사
    IDE_TEST_RAISE(idlOS::strCaselessMatch(sDBName,
                                           sDBNameLen,
                                           aArg->mDBName,
                                           aArg->mDBNameLen) != IDE_SUCCESS,
                   DBNameError);

    /* -------------------------------------------
     *  Zookeeper 메타 동일성 검사
     * ------------------------------------------*/
#ifndef ALTI_CFG_OS_LINUX
    IDE_TEST( sdiZookeeper::validateDB( sDBName,
                                        aArg->mDBCharSet,
                                        aArg->mNationalCharSet ) != IDE_SUCCESS );
#endif
    /* -------------------------------------------
     *  SHM 존재 검사 : 존재하면 에러!!!
     * ------------------------------------------*/

    sShmDBKey = mmuProperty::getShmDbKey();

    if (sShmDBKey != 0)
    {
        idBool            sShmExist;

        IDE_TEST(smmFixedMemoryMgr::checkExist(sShmDBKey, sShmExist, NULL)
                 != IDE_SUCCESS);

        IDE_TEST_RAISE(sShmExist != ID_FALSE, ShmAlreadyExistError);
    }

    IDE_TEST( smiCheckMemMaxDBSize() != IDE_SUCCESS );

    // 사용자가 지정한 Page수를 토대로 SM이 생성할 수 있는 Page의 수를 계산한다.
    IDE_TEST(smiCalculateDBSize(aArg->mUserCreatePageCount,
                                & sCreatePageCountExeptMeta ) != IDE_SUCCESS);

    // smiCalculateDBSize를 통해 계산한
    // sCreatePageCount에는 0번 페이지를 저장하는
    // META PAGE수가 빠져있는 상태이다.
    // 실제 생성될 Page수는 Meta Page수만큼을 더하여야 한다.
    sCreatePageCount = sCreatePageCountExeptMeta + 1;

    /* --------------------
     *  page range 검사
     * -------------------*/
    sHLimitPageCnt = smiGetMaxDBPageCount();
    sLLimitPageCnt = smiGetMinDBPageCount();

    IDE_TEST_RAISE( (sCreatePageCount < sLLimitPageCnt) ||
                    (sCreatePageCount > sHLimitPageCnt),
                    PageRangeError );

    /* ---------------------------
     *  DB & LOG 화일 크기 검사
     * --------------------------*/
#if !defined(WRS_VXWORKS)
    {
        struct rlimit limit;

        IDE_TEST_RAISE(idlOS::getrlimit(RLIMIT_FSIZE, &limit) != 0, GetrlimitError);

        IDE_TEST_RAISE((limit.rlim_cur < mmuProperty::getLogFileSize() - 1),
                       CheckOSLimitError);
    }
#endif
    idlOS::snprintf(sOutputMsg,
                    ID_SIZEOF(sOutputMsg),
                    MM_TRC_DATABASE_INFO,
                    (ULong)SM_PAGE_SIZE,
                    (ULong)sCreatePageCount ,
                    (ULong)(sCreatePageCount) * SM_PAGE_SIZE,
                    (ULong)(mmuProperty::getDefaultMemDbFileSize() ));
    ideLog::log(IDE_SERVER_0, "%s", sOutputMsg);

    IDE_CALLBACK_SEND_MSG(sOutputMsg);

    sShmDBKey = 0;
    IDE_TEST(idp::update(aStatistics, (SChar*)"SHM_DB_KEY", sShmDBKey) != IDE_SUCCESS);

    /* --------------------------------------------------------------------
     *  [1] DB Creation
     * ------------------------------------------------------------------*/
    ideLog::log(IDE_SERVER_0, MM_TRC_CREATE_DATABASE_BEGIN);

    IDE_TEST_RAISE(smiCreateDBCoreInit(NULL) != IDE_SUCCESS, SmiInitError);

    IDE_TEST_RAISE(smiCreateDB(sDBName,
                       sCreatePageCountExeptMeta,
                       aArg->mDBCharSet,
                       aArg->mNationalCharSet,
                       aArg->mArchiveLog ? SMI_LOG_ARCHIVE : SMI_LOG_NOARCHIVE)
                   != IDE_SUCCESS, SmiCreateDBError);

    ideLog::log(IDE_SERVER_0, MM_TRC_CREATE_DATABASE_SUCCESS);

    /* --------------------------------------------------------------------
     *  [2] META INFORMATION CREATION
     * ------------------------------------------------------------------*/
    IDE_TEST_RAISE(smiCreateDBMetaInit(NULL) != IDE_SUCCESS, SmiInitError);

    ideLog::log(IDE_SERVER_0, MM_TRC_CREATE_QP_META_BEGIN);

    IDE_TEST(qciMisc::createDB(NULL, sDBName, aArg->mOwnerDN, aArg->mOwnerDNLen) != IDE_SUCCESS);

    ideLog::log(IDE_SERVER_0, MM_TRC_CREATE_QP_META_SUCCESS);
    /* --------------------------------------------------------------------
     *  [3] Creation End & Final
     * ------------------------------------------------------------------*/
    ideLog::log(IDE_SERVER_0, MM_TRC_DATABASE_WRITING_BEGIN, (ULong)sCreatePageCount);

    IDE_TEST_RAISE(smiCreateDBMetaShutdown(NULL) != IDE_SUCCESS, SmiFinalError);

    IDE_TEST_RAISE(smiCreateDBCoreShutdown(NULL) != IDE_SUCCESS, SmiFinalError);

    ideLog::log(IDE_SERVER_0, MM_TRC_DATABASE_WRITING_SUCCESS);

    IDE_CALLBACK_SEND_MSG("\nDB Writing Completed. All Done.\n");

    return IDE_SUCCESS;

#if !defined(WRS_VXWORKS)
    IDE_EXCEPTION(GetrlimitError);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_GETRLIMIT_FAILED);

        IDE_SET(ideSetErrorCode(mmERR_ABORT_GETRLIMIT_ERROR));
    }

    IDE_EXCEPTION(CheckOSLimitError);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_OS_FILE_SIZE_LIMIT);

        IDE_SET(ideSetErrorCode(mmERR_ABORT_OSFileSizeLimit_ERROR));
    }
#endif

    IDE_EXCEPTION(DBNameLengthError);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_DATABASE_NAME_LENGTH_ERROR, SM_MAX_DB_NAME - 1);

        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATABASE_NAME_LENGTH_ERROR, SM_MAX_DB_NAME - 1));
    }
    IDE_EXCEPTION(DBNameAsciiError);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_DATABASE_NAME_ASCII_ERROR);

        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATABASE_NAME_ASCII_ERROR));
    }
    IDE_EXCEPTION(PageRangeError);
    {
        ideLog::log(IDE_SERVER_0,
                    MM_TRC_DATABASE_PAGE_RANGE_ERROR,
                    (ULong)aArg->mUserCreatePageCount * SM_PAGE_SIZE,
                    (ULong)sLLimitPageCnt * SM_PAGE_SIZE,
                    (ULong)sHLimitPageCnt * SM_PAGE_SIZE);

        IDE_SET(ideSetErrorCode(
                    mmERR_ABORT_PAGE_RANGE_ERROR,
                        (ULong)aArg->mUserCreatePageCount * SM_PAGE_SIZE,
                        (ULong)sLLimitPageCnt * SM_PAGE_SIZE,
                        (ULong)sHLimitPageCnt * SM_PAGE_SIZE));
    }
    IDE_EXCEPTION(DBNameError);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_DATABASE_NAME_ERROR);

        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATABASE_NAME_ERROR));
    }
    IDE_EXCEPTION(SmiInitError);
    {
        ideLogEntry sLog(IDE_SERVER_0);
        ideLog::logErrorMsgInternal(sLog);
        sLog.append(MM_TRC_SMI_INIT_FAILED);
        sLog.write();
    }
    IDE_EXCEPTION(SmiFinalError);
    {
        ideLogEntry sLog(IDE_SERVER_0);
        ideLog::logErrorMsgInternal(sLog);
        sLog.append(MM_TRC_SMI_SHUTDOWN_FAILED);
        sLog.write();
    }
    IDE_EXCEPTION(SmiCreateDBError);
    {
        ideLogEntry sLog(IDE_SERVER_0);
        ideLog::logErrorMsgInternal(sLog);
        sLog.append(MM_TRC_DATABASE_CREATE_FAILED);
        sLog.write();
    }
    IDE_EXCEPTION(ShmAlreadyExistError);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_SHM_ALREADY_EXIST);

        IDE_SET(ideSetErrorCode(mmERR_ABORT_SHM_ALREADY_EXIST_ERROR));
    }
    IDE_EXCEPTION_END;
    {
        if ((ideGetErrorCode() & E_ACTION_MASK) != E_ACTION_ABORT)
        {
            IDE_SET(ideSetErrorCode(mmERR_ABORT_CREATE_DB_ERROR));
        }

        IDE_CALLBACK_SEND_MSG("FAILURE of createdb.\n");

        // BUG-31098 DB 생성 실패 시 오류를 출력하고 서버 종료
        idlOS::snprintf(sOutputMsg,
                        ID_SIZEOF(sOutputMsg),
                        "%s\n",
                        ideGetErrorMsg(ideGetErrorCode()));
        IDE_ERRLOG(IDE_SERVER_0);
        IDE_CALLBACK_SEND_MSG( sOutputMsg );

        IDE_ASSERT(0);
    }

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::createDatabase(idvSQL * aStatistics, void *aArg)
{
    IDE_TEST_RAISE( mmm::getCurrentPhase() != MMM_STARTUP_PROCESS, InvalidServerPhaseError );

    IDE_TEST(doCreateDatabase(aStatistics, (qciArgCreateDB *)aArg) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidServerPhaseError );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SERVER_PHASE_MISMATCHES_QUERY_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
