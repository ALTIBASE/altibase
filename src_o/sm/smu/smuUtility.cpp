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
 

/***********************************************************************
 * $Id: smuUtility.cpp 91197 2021-07-12 01:15:29Z emlee $
 **********************************************************************/

#include <smDef.h>
#include <smErrorCode.h>
#include <smuUtility.h>
#include <sdpDef.h>
#include <sdtSortDef.h>

smuWriteErrFunc smuUtility::mWriteError = &ideLog::log;

SInt smuUtility::outputMsg(const SChar *aFmt, ...)
{
    SInt rc;
    va_list ap;
    va_start(ap, aFmt);
    /* ------------------------------------------------
     *  vfprintf가 비록 idlOS::에 없지만,
     *  PDL는 fprintf를 vfprintf를 이용해서
     *  구현한다. 따라서, PDL가 컴파일 되는 한,
     *  ::vfprintf를 사용해도 무방하다고 판단한다.
     *  2001/05/09  by gamestar
     * ----------------------------------------------*/
    rc = ::vfprintf(stdout, aFmt, ap); 
    va_end(ap);
    idlOS::fflush(stdout);

    return rc;
}

SInt smuUtility::outputErr(const SChar *aFmt, ...)
{
    SInt rc;
    va_list ap;
    va_start(ap, aFmt);
    /* ------------------------------------------------
     *  vfprintf가 비록 idlOS::에 없지만,
     *  PDL는 fprintf를 vfprintf를 이용해서
     *  구현한다. 따라서, PDL가 컴파일 되는 한,
     *  ::vfprintf를 사용해도 무방하다고 판단한다.
     *  2001/05/09  by gamestar
     * ----------------------------------------------*/
    rc = ::vfprintf(stderr, aFmt, ap); 
    va_end(ap);
    idlOS::fflush(stderr);

    return rc;
}

IDE_RC smuUtility::outputUtilityHeader(const SChar *aUtilName)
{
    
    smuUtility::outputMsg("\n%s: Release %s - Production on %s \n",
                          aUtilName,
                          iduVersionString,
                          iduGetProductionTimeString() );
    
    smuUtility::outputMsg("%s\n\n", 
                          iduCopyRightString);
    
    return IDE_SUCCESS;
}


IDE_RC smuUtility::loadErrorMsb( SChar *aRootDir, SChar *aIDN )
{
    
    SChar filename[512];

    idlOS::memset(filename, 0, 512);
    idlOS::snprintf(filename, 512, "%s%cmsg%cE_ID_%s.msb", aRootDir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, aIDN);
    IDE_TEST(ideRegistErrorMsb(filename) != IDE_SUCCESS);
    
    idlOS::memset(filename, 0, 512);
    idlOS::snprintf(filename, 512, "%s%cmsg%cE_SM_%s.msb", aRootDir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, aIDN);
    IDE_TEST(ideRegistErrorMsb(filename) != IDE_SUCCESS);

    idlOS::memset(filename, 0, 512);
    idlOS::snprintf(filename, 512, "%s%cmsg%cE_QP_%s.msb", aRootDir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, aIDN);
    IDE_TEST(ideRegistErrorMsb(filename) != IDE_SUCCESS);

    idlOS::memset(filename, 0, 512);
    idlOS::snprintf(filename, 512, "%s%cmsg%cE_MM_%s.msb", aRootDir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, aIDN);
    IDE_TEST(ideRegistErrorMsb(filename) != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/******************************************************************
 * DESCRIPTION : 
 *    TimeValue로부터 날짜/시간 문자열을 획득함
 *
 *  aTimeValue           - [IN]  입력되는 시간값
 *  aBufferLength        - [OUT] 결과값을 저장할 버퍼의 크기
 *  aBuffer              - [OUT] 결과값을 저장할 버퍼
 ******************************************************************/
void smuUtility::getTimeString( UInt    aTimeValue, 
                                UInt    aBufferLength,
                                SChar * aBuffer )
{
    struct tm        sTimeStruct;
    time_t           sTimeValue;

    sTimeValue = (time_t) aTimeValue;
    idlOS::localtime_r( &sTimeValue, &sTimeStruct);
    idlOS::snprintf( aBuffer,
                     aBufferLength,
                     "%4"ID_UINT32_FMT
                     "%02"ID_UINT32_FMT
                     "%02"ID_UINT32_FMT
                     "_%02"ID_UINT32_FMT
                     "%02"ID_UINT32_FMT
                     "%02"ID_UINT32_FMT,
                     sTimeStruct.tm_year + 1900,
                     sTimeStruct.tm_mon + 1,
                     sTimeStruct.tm_mday,
                     sTimeStruct.tm_hour,
                     sTimeStruct.tm_min,
                     sTimeStruct.tm_sec );
}

/******************************************************************
 * DESCRIPTION : 
 *    특정함수 및 변수를 받아 해당 내용을 덤프한다. 
 *
 * 1.디스크템프 테이블에서만 사용하고 있어서  __TEMPDUMP_LEVEL에
 *     따라 콜스택 Dump여부를 결정한다.
 * 2.Callstack 출력은 지금 error.log 로 고정되어 있다. 
 ******************************************************************/
void smuUtility::dumpFuncWithBuffer( UInt           aChkFlag, 
                                     ideLogModule   aModule, 
                                     UInt           aLevel, 
                                     smuDumpFunc    aDumpFunc,
                                     void         * aTarget )
{
    SChar        * sTempBuf;

    /* __TEMPDUMP_LEVEL에 따라 Dump함 */
    if ( smuProperty::getTempDumpLevel() == 2 )
    {
        ideLog::logCallStack( aChkFlag, aModule, aLevel );
    }

    ideLog::log( aChkFlag, 
                 aModule, 
                 aLevel,
                 "[DUMP] ERR-%05X(error=%"ID_UINT32_FMT") %s\n",
                 E_ERROR_CODE( ideGetErrorCode() ),
                 ideGetSystemErrno(),
                 ideGetErrorMsg( ideGetErrorCode() ) );

    if ( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                            1,
                            ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                            (void**)&sTempBuf )
         == IDE_SUCCESS )
    {
        sTempBuf[0] = '\0';
        aDumpFunc( aTarget, sTempBuf, IDE_DUMP_DEST_LIMIT );
        ideLog::log( aChkFlag, aModule, aLevel, "%s", sTempBuf );
        (void) iduMemMgr::free( sTempBuf );
    }
}
void smuUtility::printFuncWithBuffer( smuDumpFunc    aDumpFunc,
                                      void         * aTarget ) 
{
    SChar        * sTempBuf;

    if( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                           1,
                           ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                           (void**)&sTempBuf )
              == IDE_SUCCESS )
    {
        sTempBuf[0] = '\0';
        aDumpFunc( aTarget, sTempBuf, IDE_DUMP_DEST_LIMIT );
        idlOS::printf( "%s", sTempBuf );
        (void) iduMemMgr::free( sTempBuf );
    }
}

/******************************************************************
 * DESCRIPTION : 
 *    공용 변수들에 대한 DumpFunction
 ******************************************************************/
void smuUtility::dumpGRID( void  * aTarget,
                           SChar * aOutBuf, 
                           UInt    aOutSize )
{
    scGRID * sGRID = (scGRID*)aTarget;

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "GRID[%"ID_UINT32_FMT
                         ",%"ID_UINT32_FMT
                         ",%"ID_UINT32_FMT"]",
                         sGRID->mSpaceID,
                         sGRID->mPageID,
                         sGRID->mOffset );
}

void smuUtility::dumpUInt( void  * aTarget,
                           SChar * aOutBuf, 
                           UInt    aOutSize ) 
{
    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "%"ID_UINT32_FMT,
                         *(UInt*)aTarget );
}

void smuUtility::dumpExtDesc( void  * aTarget,
                              SChar * aOutBuf, 
                              UInt    aOutSize ) 
{
    sdpExtDesc * sExtDesc = (sdpExtDesc*)aTarget;

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "[%8"ID_UINT32_FMT
                         " %8"ID_UINT32_FMT"]",
                         sExtDesc->mExtFstPID,
                         sExtDesc->mLength );
}


//  IDE_RC smuUtility::makeDatabaseFileName(SChar  *aDBName,
//                                          SChar **aDBDir)
//  {
//      SInt   i;
//      SChar* sDbDir[SM_PINGPONG_COUNT];
    
//      sDbDir[0] = (SChar*)smuProperty::getDBDir(0);
//      sDbDir[1] = (SChar*)smuProperty::getDBDir(1);
    
//      IDE_TEST_RAISE(idlOS::strlen(aDBName) > (SM_MAX_DB_NAME -1),
//                     too_long_dbname);

//      for (i = 0; i < SM_PINGPONG_COUNT; i++)
//      {
//          IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMU,
//                                     1,
//                                     idlOS::strlen(aDBName) + idlOS::strlen(sDbDir[i]) + 4,
//                                     (void**)&(aDBDir[i]))
//                   != IDE_SUCCESS);

//          idlOS::sprintf(aDBDir[i], "%s%c%s-%u",
//                         sDbDir[i], IDL_FILE_SEPARATOR, aDBName, i);
//      }
    
//      return IDE_SUCCESS;

//      IDE_EXCEPTION(too_long_dbname);
//      {
//          IDE_SET( ideSetErrorCode( smERR_FATAL_TooLongDBName,
//                                    (UInt)SM_MAX_DB_NAME ) );
//      }
//      IDE_EXCEPTION_END;
//      return IDE_FAILURE;
//  }

/* --------------------------------------------------------------------

   Description :

   createdb시에 file을 저장할 database dir을 찾는다.
   db_dir이 다수개이기 때문에 그 중 임의의 하나를
   선택한다. 
   이제는 PINGPONG datafile은 구분되지 않으므로 따로 저장할 필요가 없다.
   property에서 가장 첫번째 dir을 선택할 것이다.
   
   ----------------------------------------------------------------- */



/* --------------------------------------------------------------------

   Description :

   
   ----------------------------------------------------------------- */

//  IDE_RC smuUtility::getCreateDBFileName(SChar **aFileName)
//  {
    
//      UInt sDirSelected = 0;
//      UInt sLoop;
    
//      for( sLoop = 0; sLoop != SM_DB_DIR_COUNT; ++sLoop )
//      {
        
//          makeFileName(smuProperty::getDBDir(sLoop),
//                       smuProperty::getDBName(),
//                       0,
//                       0,
//                       sFileName);
        
//          sExist = existDBFileName(sFileName);
        
//          if(sExist == ID_TRUE)
//          {
            
//          }
//      }

//      idlOS::sprintf(aDBDir, "%s",
//                     smuProperty::getDBDir(sDirSelected));
    
//      return IDE_SUCCESS;

//      IDE_EXCEPTION_END;
    
//      return IDE_FAILURE;
//  }


//  IDE_RC smuUtility::getDBDir( SChar **aDBDir)
//  {
//      SInt         i;
//      const SChar *sCurrDBDir;

//      for (i = 0; i < SM_PINGPONG_COUNT; i++)
//      {
//          sCurrDBDir = smuProperty::getDBDir(i);
        
//          IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMU,
//                                     1,
//                                     idlOS::strlen(sCurrDBDir) + 4,
//                                     (void**)&(aDBDir[i]))
//                   != IDE_SUCCESS);

//          idlOS::sprintf(aDBDir[i], "%s", sCurrDBDir);
//      }
    
//      return IDE_SUCCESS;
    
//      IDE_EXCEPTION_END;
    
//      return IDE_FAILURE;
    
//  }
