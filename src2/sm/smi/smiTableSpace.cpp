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
 * $Id: smiTableSpace.cpp 86110 2019-09-02 04:52:04Z et16 $
 **********************************************************************/

/***********************************************************************
 * FILE DESCRIPTION : smiTableSpace.cpp
 *   본 파일은 smiTableSpace 클래스의 구현 파일이다.
 *   smiTableSpace 클래스는 Disk Resident DB에서 사용되는
 *   user table space을 생성, 삭제 및 파일을 추가하
 *   는 루틴을 포함하고 있다.
 *   현재 TableSpace의 정보는 Meta(Catalog)에 저장되지 않고
 *   sdpTableSpace 내부적으로 정의된 특정 메모리 구조체에 저장될 예정
 *   이다. 이 메모리 구조체는 Log Anchor File에 정의된 정보들을 그대로
 *   저장한다.
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smiTableSpace.h>
#include <smiMain.h>
#include <smiMisc.h>
#include <smiTrans.h>
#include <smm.h>
#include <svm.h>
#include <sdpTableSpace.h>
#include <sddTableSpace.h>
#include <sddDiskMgr.h>
#include <sddDataFile.h>
#include <sctTableSpaceMgr.h>
#include <sctDef.h>
#include <sctTBSAlter.h>
#include <smpTBSAlterOnOff.h>
#include <smuProperty.h>

/* (참고) Memory Tablespace에 대해서는 이 Function Pointer Array가
          전혀 참조되지 않도록 구현되어있다
*/
static const smiCreateDiskTBSFunc smiCreateDiskTBSFunctions[SMI_TABLESPACE_TYPE_MAX]=
{
    //SMI_MEMORY_SYSTEM_DICTIONARY
    NULL,
    //SMI_MEMORY_SYSTEM_DATA
    NULL,
    //SMI_MEMORY_USER_DATA
    NULL,
    //SMI_DISK_SYSTEM_DATA
    sdpTableSpace::createTBS,
    //SMI_DISK_USER_DATA
    sdpTableSpace::createTBS,
    // SMI_DISK_SYSTEM_TEMP
    sdpTableSpace::createTBS,
    // SMI_DISK_USER_TEMP
    sdpTableSpace::createTBS,
    // SMI_DISK_SYSTEM_UNDO,
    sdpTableSpace::createTBS
};

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::createDiskTBS()
 *   본 함수는 create tablespace 구문에 의하여 불리며
 *   새로운 Disk TableSpace를 생성한다.
 *   인자로 넘어온 테이블 스페이스 속성과 데이타 파일 속성의
 *   list를 받아서 sdpTableSpace의 함수를 호출한다.
 *   그 결과로 table space의 id를 aTableSpaceAttr.mID에
 *   assign하게 된다.
 *   테이블 스페이스 type은 다음과 같다.
 *   1> 일반테이블 스페이스   :  SMI_DISK_USER_DATA
 *   2> system temp table 스페이스 :  SMI_DISK_SYSTEM_TEMP
 *   3> user   temp table 스페이스 :  SMI_DISK_USER_TEMP
 *   4> undo table 스페이스   :  SMI_DISK_SYSTEM_UNDO
 *   5> system table 스페이스 :  SMI_DISK_SYSTEM_DATA
 * BUGBUG : ext page count가 attr의 속성이 되었으므로
 * 이를 인자로 따로 받을 필요가 없다. QP의 많은 부분이 바뀌어야 하므로
 * 작업이 끝난 후 따로 수정할 예정임.
 **********************************************************************/
IDE_RC smiTableSpace::createDiskTBS(idvSQL            * aStatistics,
                                    smiTrans          * aTrans,
                                    smiTableSpaceAttr * aTableSpaceAttr,
                                    smiDataFileAttr  ** aDataFileAttrList,
                                    UInt                aFileAttrCnt,
                                    UInt                aExtPageCnt,
				                    ULong               aExtSize)
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTableSpaceAttr != NULL );
    IDE_DASSERT( (aTableSpaceAttr->mType >  SMI_MEMORY_SYSTEM_DICTIONARY) &&
                 (aTableSpaceAttr->mType <  SMI_TABLESPACE_TYPE_MAX) );
    IDE_DASSERT( aTableSpaceAttr->mAttrType == SMI_TBS_ATTR );
    IDE_ASSERT( aTableSpaceAttr->mDiskAttr.mExtMgmtType == 
                SMI_EXTENT_MGMT_BITMAP_TYPE );

    /* To Fix BUG-21394 */
    IDE_TEST_RAISE( aExtSize < 
		    ( smiGetPageSize(aTableSpaceAttr->mType) * smiGetMinExtPageCnt()),
		      ERR_INVALID_EXTENTSIZE);

    // BUGBUG-1548 CONTROL/PROCESS단계이면 에러를 내도록 처리해야함
    aTableSpaceAttr->mDiskAttr.mExtPageCount = aExtPageCnt;

    IDE_TEST( smiCreateDiskTBSFunctions[aTableSpaceAttr->mType](
                  aStatistics,
                  aTableSpaceAttr,
                  aDataFileAttrList,
                  aFileAttrCnt,
                  aTrans->getTrans() )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_EXTENTSIZE)
    {
      IDE_SET(ideSetErrorCode(smERR_ABORT_EXTENT_SIZE_IS_TOO_SMALL));
    } 

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
     메모리 Tablespace를 생성한다.
     [IN] aTrans          - Tablespace를 생성하려는 Transaction
     [IN] aName           - Tablespace의 이름
     [IN] aAttrFlag       - Tablespace의 속성 Flag
     [IN] aChkptPathList  - Tablespace의 Checkpoint Image들을 저장할 Path들
     [IN] aSplitFileSize  - Checkpoint Image File의 크기
     [IN] aInitSize       - Tablespace의 초기크기
     [IN] aIsAutoExtend   - Tablespace의 자동확장 여부
     [IN] aNextSize       - Tablespace의 자동확장 크기
     [IN] aMaxSize        - Tablespace의 최대크기
     [IN] aIsOnline       - Tablespace의 초기 상태 (ONLINE이면 ID_TRUE)
     [IN] aDBCharSet      - 데이터베이스 캐릭터 셋
     [IN] aNationalCharSet- 내셔널 캐릭터 셋
     [OUT] aTBSID         - 생성한 Tablespace의 ID
 */
IDE_RC smiTableSpace::createMemoryTBS(
                          smiTrans             * aTrans,
                          SChar                * aName,
                          UInt                   aAttrFlag,
                          smiChkptPathAttrList * aChkptPathList,
                          ULong                  aSplitFileSize,
                          ULong                  aInitSize,
                          idBool                 aIsAutoExtend,
                          ULong                  aNextSize,
                          ULong                  aMaxSize,
                          idBool                 aIsOnline,
                          scSpaceID            * aTBSID )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aName != NULL );
    // aChkptPathList는 사용자가 지정하지 않은 경우 NULL이다.
    // aSplitFileSize는 사용자가 지정하지 않은 경우 0이다.
    // aInitSize는 사용자가 지정하지 않은 경우 0이다.
    // aNextSize는 사용자가 지정하지 않은 경우 0이다.
    // aMaxSize는 사용자가 지정하지 않은 경우 0이다.

    // BUGBUG-1548 CONTROL/PROCESS단계이면 에러를 내도록 처리해야함
    
    IDE_TEST( smmTBSCreate::createTBS( aTrans->getTrans(),
                                       smmDatabase::getDBName(),
                                       aName,
                                       aAttrFlag,
                                       SMI_MEMORY_USER_DATA,
                                       aChkptPathList,
                                       aSplitFileSize,
                                       aInitSize,
                                       aIsAutoExtend,
                                       aNextSize,
                                       aMaxSize,
                                       aIsOnline,
                                       smmDatabase::getDBCharSet(),
                                       smmDatabase::getNationalCharSet(),
                                       aTBSID )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*   PROJ-1594 Volatile TBS
     Volatile Tablespace를 생성한다.
     [IN] aTrans          - Tablespace를 생성하려는 Transaction
     [IN] aName           - Tablespace의 이름
     [IN] aAttrFlag       - Tablespace의 속성 Flag
     [IN] aInitSize       - Tablespace의 초기크기
     [IN] aIsAutoExtend   - Tablespace의 자동확장 여부
     [IN] aNextSize       - Tablespace의 자동확장 크기
     [IN] aMaxSize        - Tablespace의 최대크기
     [IN] aState          - Tablespace의 상태
     [OUT] aTBSID         - 생성한 Tablespace의 ID
*/
IDE_RC smiTableSpace::createVolatileTBS(
                          smiTrans             * aTrans,
                          SChar                * aName,
                          UInt                   aAttrFlag,
                          ULong                  aInitSize,
                          idBool                 aIsAutoExtend,
                          ULong                  aNextSize,
                          ULong                  aMaxSize,
                          UInt                   aState,
                          scSpaceID            * aTBSID )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aName != NULL );
    // aInitSize는 사용자가 지정하지 않은 경우 0이다.
    // aNextSize는 사용자가 지정하지 않은 경우 0이다.
    // aMaxSize는 사용자가 지정하지 않은 경우 0이다.

    IDE_TEST( svmTBSCreate::createTBS(aTrans->getTrans(),
                                       smmDatabase::getDBName(),
                                       aName,
                                       SMI_VOLATILE_USER_DATA,
                                       aAttrFlag,
                                       aInitSize,
                                       aIsAutoExtend,
                                       aNextSize,
                                       aMaxSize,
                                       aState,
                                       aTBSID )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
  FUNCTION DESCRIPTION : smiTableSpace::drop()
    본 함수는 drop tablespace 구문에 의하여 불리며,
     기존의 TableSpace를 삭제할때 호출되는 함수이다.
    touch mode의 값은 다음과 같다.

 SMI_ALL_TOUCH = 0,  ->tablespace의 모든 datafile 노드의
                          datafile을 건드린다.
 SMI_ALL_NOTOUCH,   ->tablespace의 모든 datafile 노드의
                          datafile을 건드리지 않는다.
 SMI_EACH_BYMODE    -> 각 datafile 노드의 create 모드에 따른다.
 **********************************************************************/
IDE_RC smiTableSpace::drop( idvSQL       */*aStatistics*/,
                            smiTrans*     aTrans,
                            scSpaceID     aTableSpaceID,
                            smiTouchMode  aTouchMode )
{
    sctTableSpaceNode * sTBSNode;

    IDE_DASSERT( aTrans != NULL );

    // fix bug-9563.
    // system tablespace는 drop할수 없다.
    IDE_TEST_RAISE( aTableSpaceID <= SMI_ID_TABLESPACE_SYSTEM_DISK_TEMP,
                    error_drop_system_tablespace);

    // BUGBUG-1548 CONTROL/PROCESS단계이면 에러를 내도록 처리해야함

    // TBSID로 TBSNode를 알아낸다.
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                                          aTableSpaceID,
                                          (void**) & sTBSNode )
              != IDE_SUCCESS );

    switch ( sTBSNode->mType )
    {
        case SMI_MEMORY_USER_DATA:
            IDE_TEST( smmTBSDrop::dropTBS(
                          aTrans->getTrans(),
                          (smmTBSNode*) sTBSNode,
                          aTouchMode )
                      != IDE_SUCCESS );
            break;
        case SMI_VOLATILE_USER_DATA:
            /* PROJ-1594 Volatile TBS */
            IDE_TEST( svmTBSDrop::dropTBS( aTrans->getTrans(),
                                              (svmTBSNode*)sTBSNode )
                      != IDE_SUCCESS );
            break;
        case SMI_DISK_USER_DATA:
        case SMI_DISK_USER_TEMP:
            //sddTableSpace의 TableSpace를 삭제하는 함수 호출
            IDE_TEST(sdpTableSpace::dropTBS(
                         NULL, // BUGBUG : 추가 요망 from QP
                         aTrans->getTrans(),
                         aTableSpaceID,
                         aTouchMode)
                     != IDE_SUCCESS);
            break;

        case SMI_MEMORY_SYSTEM_DICTIONARY :
        case SMI_MEMORY_SYSTEM_DATA:
        case SMI_DISK_SYSTEM_TEMP:
        case SMI_DISK_SYSTEM_UNDO:
        case SMI_DISK_SYSTEM_DATA:
        default :
            // 위에서 Drop불가능한 TBS인지 체크하였으므로
            // 여기에서는 Drop불가능한 TBS는 들어올 수 없다
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(error_drop_system_tablespace);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CannotDropTableSpace));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::alterStatus()
 *   ALTER TABLESPACE ONLINE/OFFLINE을 수행 
 *
 *
 *   본 함수는 alter tablespace tablespace이름 [online| offline]
 *   구문에 의하여 불리며, 특정 TableSpace의 상태를
 *   online/offline으로 변경한다.
 *   파일의 추가를 제외한 모든 작업들은 online 상태에서만 가능하다.
 *
 * [IN] aTrans        - 상태를 변경하려는 Transaction
 * [IN] aTableSpaceID - 상태를 변경하려는 Tablespace의 ID
 * [IN] aState        - 새로 전이할 상태 ( Online or Offline )
 **********************************************************************/
IDE_RC smiTableSpace::alterStatus(idvSQL*     aStatistics,
                                  smiTrans  * aTrans,
                                  scSpaceID   aTableSpaceID,
                                  UInt        aState )
{
    sctTableSpaceNode * sTBSNode;

    IDE_DASSERT( aTrans != NULL );

    if ( ( smiGetStartupPhase() != SMI_STARTUP_META ) &&
         ( smiGetStartupPhase() != SMI_STARTUP_SERVICE ) )
    {
        IDE_RAISE( error_alter_tbs_onoff_allowed_only_at_meta_service_phase);
    }
    
    // TBSID로 TBSNode를 알아낸다.
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTableSpaceID,
                                                        (void**) & sTBSNode )
              != IDE_SUCCESS );

    switch ( sTBSNode->mType )
    {
        case SMI_MEMORY_USER_DATA:

            IDE_TEST( smpTBSAlterOnOff::alterTBSStatus( aTrans->getTrans(),
                                                        (smmTBSNode*) sTBSNode,
                                                        aState )
                      != IDE_SUCCESS );                
            break;
            
        case SMI_DISK_USER_DATA:
            IDE_TEST( sdpTableSpace::alterTBSStatus(
                          aStatistics, 
                          aTrans->getTrans(),
                          (sddTableSpaceNode*) sTBSNode,
                          aState ) != IDE_SUCCESS );
            break;
            
        case SMI_MEMORY_SYSTEM_DICTIONARY:
        case SMI_MEMORY_SYSTEM_DATA:
        case SMI_DISK_USER_TEMP:
        case SMI_DISK_SYSTEM_TEMP:
        case SMI_DISK_SYSTEM_UNDO:
        case SMI_DISK_SYSTEM_DATA:
        default :
            // 위에서 Drop불가능한 TBS인지 체크하였으므로
            // 여기에서는 Drop불가능한 TBS는 들어올 수 없다
            IDE_ASSERT(0);
            break;
            
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(error_alter_tbs_onoff_allowed_only_at_meta_service_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ALTER_TBS_ONOFF_ALLOWED_ONLY_AT_META_SERVICE_PHASE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
    Tablespace의 Attribute Flag를 변경한다.
    
    [IN] aTrans - Transaction
    [IN] aTableSpaceID - Tablespace의 ID
    [IN] aAttrFlagMask  - Attribute Flag의 Mask
    [IN] aAttrFlagValue - Attribute Flag의 Value
 */
IDE_RC smiTableSpace::alterTBSAttrFlag(smiTrans  * aTrans,
                                       scSpaceID   aTableSpaceID,
                                       UInt        aAttrFlagMask,
                                       UInt        aAttrFlagValue )
{
    IDE_DASSERT( aTrans != NULL );

    // Attribute Mask가 0일 수 없다.
    IDE_ASSERT( aAttrFlagMask != 0 );
    
    // Attribute Mask외의 비트가
    // Attribute Value에 세팅될 수 없다.
    IDE_ASSERT( (~aAttrFlagMask & aAttrFlagValue) == 0 );
    
    IDE_TEST( sctTBSAlter::alterTBSAttrFlag(
                               aTrans->getTrans(),
                               aTableSpaceID,
                               aAttrFlagMask,
                               aAttrFlagValue )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::addFile()
 *   본 함수는 alter tablespace add datafile ..구문에 의하여 불리며,
 *   특정 TableSpace에 데이타 파일들을 추가한다.
 **********************************************************************/
IDE_RC smiTableSpace::addDataFile( idvSQL          * aStatistics,
                                   smiTrans*         aTrans,
                                   scSpaceID         aTblSpaceID,
                                   smiDataFileAttr** aDataFileAttr,
                                   UInt              aDataFileAttrCnt)
{
    IDE_DASSERT( aTrans != NULL );
    
    IDE_TEST(sdpTableSpace::createDataFiles(aStatistics,
                                            aTrans->getTrans(),
                                            aTblSpaceID,
                                            aDataFileAttr,
                                            aDataFileAttrCnt)
             != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::alterDataFileAutoExtend()
 *   본 함수는 alter tablespace modify datafile 절에 의하여
 *   불리며,autoextend관련 속성을 변경한다.
 *   next size, max size, autoextend on/off.
 **********************************************************************/
IDE_RC  smiTableSpace::alterDataFileAutoExtend(smiTrans*   aTrans,
                                               scSpaceID   aSpaceID,
                                               SChar      *aFileName,
                                               idBool      aAutoExtend,
                                               ULong       aNextSize,
                                               ULong       aMaxSize,
                                               SChar*      aValidFileName)
{

    IDE_DASSERT( aFileName != NULL );
    IDE_DASSERT( aFileName != NULL );
    
    IDE_TEST( sdpTableSpace::alterDataFileAutoExtend(NULL, // BUGBUG : 추가 요망 from QP
                                                     aTrans->getTrans(),
                                                     aSpaceID,
                                                     aFileName,
                                                     aAutoExtend,aNextSize,
                                                     aMaxSize,
                                                     aValidFileName)
              != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::alterDataFileOnlineMode()
 *   본 함수는 alter tablespace modify datafile 절에 의하여
 *   불리며,online관련 속성을 변경한다.
 **********************************************************************/
IDE_RC smiTableSpace::alterDataFileOnLineMode( scSpaceID  /* aSpaceID */,
                                               SChar *    /* aFileName */,
                                               UInt       /* aOnline */)
{
    // datafile  offline /online은 altibase 4에서는
    // 의미가 없다((BUG-11341).
    IDE_SET(ideSetErrorCode(smERR_ABORT_NotSupport,"DATAFILE ONLINE/OFFLINE"));
    
    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace:resizeDataFileSize()
 *   본 함수는 alter tablespace modify datafile.. resize 절에 의하여
 *   불리며, 특정 TableSpace에 속한 데이타파일의 크기를 조정한다.
 **********************************************************************/
IDE_RC smiTableSpace::resizeDataFile(idvSQL    * aStatistics,
                                     smiTrans  * aTrans,
                                     scSpaceID   aTblSpaceID,
                                     SChar     * aFileName,
                                     ULong       aSizeWanted,
                                     ULong     * aSizeChanged,
                                     SChar     * aValidFileName )
{
    IDE_DASSERT( aTrans != NULL );
    
    IDE_TEST(sdpTableSpace::alterDataFileReSize(aStatistics,
                                                aTrans->getTrans(),
                                                aTblSpaceID,
                                                aFileName,
                                                aSizeWanted,
                                                aSizeChanged,
                                                aValidFileName)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::renameDataFile()
 * 본 함수는 alter tablespace rename datafile 절에 의하여
   불리며, 특정 TableSpace에 속한 파일의 이름을 변경한다.
 **********************************************************************/
IDE_RC smiTableSpace::renameDataFile( scSpaceID  aTblSpaceID,
                                      SChar*     aOldFileName,
                                      SChar*     aNewFileName )
{
    IDE_TEST_RAISE( smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                    err_startup_phase );        

    IDE_TEST(sdpTableSpace::alterDataFileName( NULL, /* idvSQL* */
                                               aTblSpaceID,
                                               aOldFileName,
                                               aNewFileName )
             != IDE_SUCCESS);

    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::removeDataFile()
 *   본 함수는 alter tablespace drop datafile절에 의하여
 *   불리며 특정 TableSpace에 속한 데이타파일 노드를 삭제한다.
 *   !! 파일은 삭제하지 않는다.
 **********************************************************************/
IDE_RC smiTableSpace::removeDataFile( smiTrans* aTrans,
                                      scSpaceID aTblSpaceID,
                                      SChar*    aFileName,
                                      SChar*    aValidFileName)
{                                     
    IDE_DASSERT( aTrans != NULL );
    
    IDE_TEST(sdpTableSpace::removeDataFile(NULL, // BUGBUG : 추가 요망 from QP
                                           aTrans->getTrans(),
                                           aTblSpaceID,
                                           aFileName,
                                           aValidFileName)
             != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *
 * Description :
 *
 * 상대 경로를 받아서 절대 경로를 리턴한다.
 * 상대 경로로 지정된 화일은 ALTIBASE_HOME/dbs 디렉토리의 화일로 취급된다.
 * Abs : Absolute
 * Rel : Relative
 * space ID는 system, undo, temp란 예약어가 file의 이름으로 사용될 수
 * 있는 지 결정하기 위해 필요하다 user 테이블스페이스의 datafile은
 * 시스템 예약어가 들어간 화일을 사용할 수 없다.
 * 
 * tablespace ID를 알 수 없는 경우에
 * 이 함수를 사용하기 위해서는 
 * SMI_ID_TABLESPACE_SYSTEM_TEMP보다 큰 값을 넘기면 된다.
 *
 * BUG-29812
 * getAbsPath 함수를 Memory/Disk TBS에서 모두 사용할 수 있도록 변경한다.
 *
 * Memory TBS의 경우 한 개의 버퍼에 상대경로를 인자로 넘겨주고,
 * 절대경로를 받기 때문에 aRelName과 aAbsName이 동일한 주소이다.
 * 
 * ----------------------------------------------*/
IDE_RC smiTableSpace::getAbsPath( SChar         * aRelName, 
                                  SChar         * aAbsName,
                                  smiTBSLocation  aTBSLocation )
{                                     
    UInt   sNameSize;
    
    IDE_DASSERT( aRelName != NULL );
    IDE_DASSERT( aAbsName != NULL );

    if( aRelName != aAbsName )
    {
        idlOS::strcpy( aAbsName,
                       aRelName );
    }
    
    sNameSize = idlOS::strlen(aAbsName);
 

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath( 
                                ID_TRUE,
                                aAbsName,
                                &sNameSize,
                                aTBSLocation )
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *
 * Description :
 * tablespace id로 tablespace를 찾아
 * 그 속성을 반환한다.
 *
 * ----------------------------------------------*/
IDE_RC smiTableSpace::getAttrByID( scSpaceID           aTableSpaceID,
                                   smiTableSpaceAttr * aTBSAttr )
{                                     
    
    IDE_DASSERT( aTBSAttr != NULL );

    IDE_TEST( sctTableSpaceMgr::getTBSAttrByID( NULL,
                                                aTableSpaceID,
                                                aTBSAttr )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ------------------------------------------------
 *
 * Description :
 * tablespace name으로 tablespace를 찾아
 * 그 속성을 반환한다.
 * ----------------------------------------------*/
IDE_RC smiTableSpace::getAttrByName( SChar             * aTableSpaceName,
                                     smiTableSpaceAttr * aTBSAttr )
{
    IDE_DASSERT( aTableSpaceName != NULL );
    IDE_DASSERT( aTBSAttr != NULL );
    
    IDE_TEST( sctTableSpaceMgr::getTBSAttrByName(aTableSpaceName,
                                                 aTBSAttr)
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *
 * Description :
 *
 * tablespace id와 datafile의 이름을 받아
 * 해당 datafile이 존재하는지 검사한다.
 * ----------------------------------------------*/
IDE_RC smiTableSpace::existDataFile( scSpaceID         aTableSpaceID,
                                     SChar*            aDataFileName,
                                     idBool*           aExist)
{                                     
    IDE_DASSERT( aDataFileName != NULL );
    IDE_DASSERT( aExist != NULL );
    
    IDE_TEST( sddDiskMgr::existDataFile(NULL, /* idvSQL* */
                                        aTableSpaceID,
                                        aDataFileName,
                                        aExist)
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ------------------------------------------------
 *
 * Description :
 *
 * 모든 테이블스페이스에 대해 datafile이
 * 이미 존재하는지 존재하는지 검사한다.
 * ----------------------------------------------*/
IDE_RC smiTableSpace::existDataFile( SChar*            aDataFileName,
                                     idBool*           aExist)
{                                     
    IDE_DASSERT( aDataFileName != NULL );
    IDE_DASSERT( aExist != NULL );

    IDE_TEST( sddDiskMgr::existDataFile(aDataFileName,
                                        aExist)
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

// fix BUG-13646
IDE_RC smiTableSpace::getExtentAnTotalPageCnt(scSpaceID  aTableSpaceID,
                                              UInt*      aExtPageCount,
                                              ULong*     aTotalPageCount)
{
    return sddDiskMgr::getExtentAnTotalPageCnt(NULL, /* idvSQL* */
                                               aTableSpaceID,
                                               aExtPageCount,
                                               aTotalPageCount);
}
/*
 * 시스템 테이블스페이스 여부 반환
 *
 * [IN] aSpaceID - Tablespace의 ID
 */
idBool smiTableSpace::isSystemTableSpace( scSpaceID aSpaceID )
{
    return sctTableSpaceMgr::isSystemTableSpace( aSpaceID );
}

idBool smiTableSpace::isMemTableSpace( scSpaceID aTableSpaceID )
{
    return sctTableSpaceMgr::isMemTableSpace( aTableSpaceID );
}

idBool smiTableSpace::isVolatileTableSpace( scSpaceID aTableSpaceID )
{
    return sctTableSpaceMgr::isVolatileTableSpace( aTableSpaceID );
}

idBool smiTableSpace::isDiskTableSpace( scSpaceID  aTableSpaceID )
{
    return sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID );
}

idBool smiTableSpace::isTempTableSpace( scSpaceID  aTableSpaceID )
{
    return sctTableSpaceMgr::isTempTableSpace( aTableSpaceID );
}

idBool smiTableSpace::isMemTableSpaceType( smiTableSpaceType  aType )
{
    return sctTableSpaceMgr::isMemTableSpaceType( aType );
}

idBool smiTableSpace::isVolatileTableSpaceType( smiTableSpaceType aType )
{
    return sctTableSpaceMgr::isVolatileTableSpaceType( aType );
}

idBool smiTableSpace::isDiskTableSpaceType( smiTableSpaceType  aType )
{
    return sctTableSpaceMgr::isDiskTableSpaceType( aType );
}

idBool smiTableSpace::isTempTableSpaceType( smiTableSpaceType  aType )
{
    return sctTableSpaceMgr::isTempTableSpaceType( aType );
}

idBool smiTableSpace::isDataTableSpaceType( smiTableSpaceType  aType )
{
    return sctTableSpaceMgr::isDataTableSpaceType( aType );
}

/*
    ALTER TABLESPACE TBSNAME ADD CHECKPOINT PATH ... 를 실행

    [IN] aSpaceID   - Tablespace ID
    [IN] aChkptPath - 추가할 Checkpoint Path
*/
IDE_RC  smiTableSpace::alterMemoryTBSAddChkptPath( scSpaceID      aSpaceID,
                                                   SChar        * aChkptPath )
{
    IDE_DASSERT( aChkptPath != NULL );

    // 다단계startup단계중 control 단계에서만 불릴수 있다.
    IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                   err_startup_phase);

    IDE_TEST( smmTBSAlterChkptPath::alterTBSaddChkptPath(
                                 aSpaceID,
                                 aChkptPath ) != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_INVALID_STARTUP_PHASE_NOT_CONTROL,
                                "ALTER TABLESPACE ADD CHECKPOINT PATH"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    ALTER TABLESPACE TBSNAME RENAME CHECKPOINT PATH ... 를 실행

    [IN] aSpaceID      - Tablespace ID
    [IN] aOrgChkptPath - 변경전 Checkpoint Path
    [IN] aNEwChkptPath - 변경후 Checkpoint Path
*/
IDE_RC  smiTableSpace::alterMemoryTBSRenameChkptPath( scSpaceID   aSpaceID,
                                                      SChar     * aOrgChkptPath,
                                                      SChar     * aNewChkptPath )
{
    IDE_DASSERT( aOrgChkptPath != NULL );
    IDE_DASSERT( aNewChkptPath != NULL );
    
    // 다단계startup단계중 control 단계에서만 불릴수 있다.
    IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                   err_startup_phase);
    
    IDE_TEST( smmTBSAlterChkptPath::alterTBSrenameChkptPath(
                                 aSpaceID,
                                 aOrgChkptPath,
                                 aNewChkptPath ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_INVALID_STARTUP_PHASE_NOT_CONTROL,
                                "ALTER TABLESPACE RENAME CHECKPOINT PATH"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    ALTER TABLESPACE TBSNAME DROP CHECKPOINT PATH ... 를 실행

    [IN] aSpaceID   - Tablespace ID
    [IN] aChkptPath - 제거할 Checkpoint Path
*/
IDE_RC  smiTableSpace::alterMemoryTBSDropChkptPath( scSpaceID      aSpaceID,
                                                    SChar        * aChkptPath )
{
    IDE_DASSERT( aChkptPath != NULL );

    // 다단계startup단계중 control 단계에서만 불릴수 있다.
    IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                   err_startup_phase);
    
    IDE_TEST( smmTBSAlterChkptPath::alterTBSdropChkptPath(
                                 aSpaceID,
                                 aChkptPath ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_INVALID_STARTUP_PHASE_NOT_CONTROL,
                                "ALTER TABLESPACE DROP CHECKPOINT PATH"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    ALTER TABLESPACE TBSNAME AUTOEXTEND ... 를 수행한다 

   들어올 수 있는 문장 종류:
     - ALTER TABLESPACE TBSNAME AUTOEXTEND OFF
     - ALTER TABLESPACE TBSNAME AUTOEXTEND ON NEXT 10M
     - ALTER TABLESPACE TBSNAME AUTOEXTEND ON MAXSIZE 10M/UNLIMITTED
     - ALTER TABLESPACE TBSNAME AUTOEXTEND ON NEXT 10M MAXSIZE 10M/UNLIMITTED
    
*/
IDE_RC  smiTableSpace::alterMemoryTBSAutoExtend(smiTrans*   aTrans,
                                                scSpaceID   aSpaceID,
                                                idBool      aAutoExtend,
                                                ULong       aNextSize,
                                                ULong       aMaxSize )
{
    IDE_DASSERT( aTrans != NULL );

    // dictionary tablespace는 alter autoextend할수 없다.
    IDE_TEST_RAISE( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    error_alter_autoextend_dictionary_tablespace);
    
    // ALTER TABLESPACE TBSNAME AUTOEXTEND ON/OFF
    IDE_TEST( smmTBSAlterAutoExtend::alterTBSsetAutoExtend( aTrans->getTrans(),
                                                    aSpaceID,
                                                    aAutoExtend,
                                                    aNextSize,
                                                    aMaxSize)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    
    IDE_EXCEPTION(error_alter_autoextend_dictionary_tablespace);
    {
        IDE_SET(ideSetErrorCode(
                   smERR_ABORT_CANNOT_ALTER_AUTOEXTEND_DICTIONARY_TABLESPACE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*  PROJ-1594 Volatile TBS
    ALTER TABLESPACE TBSNAME AUTOEXTEND ... 를 수행한다
*/
IDE_RC  smiTableSpace::alterVolatileTBSAutoExtend(smiTrans*   aTrans,
                                                  scSpaceID   aSpaceID,
                                                  idBool      aAutoExtend,
                                                  ULong       aNextSize,
                                                  ULong       aMaxSize )
{
    IDE_DASSERT( aTrans != NULL );

    // ALTER TABLESPACE TBSNAME AUTOEXTEND ON/OFF
    IDE_TEST( svmTBSAlterAutoExtend::alterTBSsetAutoExtend( aTrans->getTrans(),
                                                    aSpaceID,
                                                    aAutoExtend,
                                                    aNextSize,
                                                    aMaxSize)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * FUNCTION DESCRIPTION : smiTableSpace::alterDiscard
 *   ALTER TABLESPACE DISCARD을 수행 
 *
 *   본 함수는 특정 tablespace를 disacrd 상태로 변경한다.
 *   startup control 단계에서만 가능하다. 
 *
 *   DISCARD된 Tablespace는 사용할 수 없게 되며 오직 Drop만 가능하다.
 *   그 안의 Table, Data역시 Drop만 가능하다.
 *
 *   동시성 제어 - CONTROL단계에만 호출되므로 필요하지 않다
 *
 * [IN] aTableSpaceID - 상태를 변경하려는 Tablespace의 ID
 **********************************************************************/
IDE_RC smiTableSpace::alterDiscard( scSpaceID aTableSpaceID )
{
    sctTableSpaceNode * sTBSNode;

    IDE_TEST_RAISE( smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                    err_startup_phase );        
    
    // system tablespace는 online/offline할수 없다.
    IDE_TEST_RAISE( aTableSpaceID <= SMI_ID_TABLESPACE_SYSTEM_DISK_TEMP,
                    error_discard_system_tablespace);
    
    // TBSID로 TBSNode를 알아낸다.
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(  
                                    aTableSpaceID,
                                    (void**) & sTBSNode )
              != IDE_SUCCESS );

    switch ( sTBSNode->mType )
    {
        case SMI_MEMORY_USER_DATA:
            IDE_TEST( smmTBSAlterDiscard::alterTBSdiscard(
                          (smmTBSNode*) sTBSNode )
                      != IDE_SUCCESS );                
            break;
            
        case SMI_DISK_USER_DATA:
        case SMI_DISK_USER_TEMP:
            IDE_TEST( sdpTableSpace::alterTBSdiscard(
                          ( sddTableSpaceNode *) sTBSNode )
                      != IDE_SUCCESS );                
            break;
        case SMI_MEMORY_SYSTEM_DICTIONARY :
        case SMI_MEMORY_SYSTEM_DATA:
        case SMI_DISK_SYSTEM_TEMP:
        case SMI_DISK_SYSTEM_UNDO:
        case SMI_DISK_SYSTEM_DATA:
        default :
            // 위에서 Discard불가능한 TBS인지 체크하였으므로
            // 여기에서는 Discard불가능한 TBS는 들어올 수 없다
            IDE_ASSERT(0);
            break;
            
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION(error_discard_system_tablespace);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CannotDiscardTableSpace));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

 
ULong smiTableSpace::getSysDataTBSExtentSize()
{
    return smuProperty::getSysDataTBSExtentSize();
}

// BUG-14897 - 속성 SYS_DATA_FILE_INIT_SIZE 값 리턴.
ULong smiTableSpace::getSysDataFileInitSize()
{
    return getValidSize4Disk( smuProperty::getSysDataFileInitSize() );
}

// BUG-14897 - 속성 SYS_DATA_FILE_MAX_SIZE 값 리턴.
ULong smiTableSpace::getSysDataFileMaxSize()
{
    return getValidSize4Disk( smuProperty::getSysDataFileMaxSize() );
}

// BUG-14897 - 속성 SYS_DATA_FILE_NEXT_SIZE 값 리턴.
ULong smiTableSpace::getSysDataFileNextSize()
{
    return smuProperty::getSysDataFileNextSize();
}

// BUG-14897 - 속성 SYS_UNDO_TBS_EXTENT_SIZE 값 리턴.
ULong smiTableSpace::getSysUndoTBSExtentSize()
{
    return smuProperty::getSysUndoTBSExtentSize();
}

// BUG-14897 - 속성 SYS_UNDO_FILE_INIT_SIZE 값 리턴.
ULong smiTableSpace::getSysUndoFileInitSize()
{
    return getValidSize4Disk( smuProperty::getSysUndoFileInitSize() );
}

// BUG-14897 - 속성 SYS_UNDO_FILE_MAX_SIZE 값 리턴.
ULong smiTableSpace::getSysUndoFileMaxSize()
{
    return getValidSize4Disk( smuProperty::getSysUndoFileMaxSize() );
}

// BUG-14897 - 속성 SYS_UNDO_FILE_NEXT_SIZE 값 리턴.
ULong smiTableSpace::getSysUndoFileNextSize()
{
    return smuProperty::getSysUndoFileNextSize();
}

// BUG-14897 - 속성 SYS_TEMP_TBS_EXTENT_SIZE 값 리턴.
ULong smiTableSpace::getSysTempTBSExtentSize()
{
    return smuProperty::getSysTempTBSExtentSize();
}

// BUG-14897 - 속성 SYS_TEMP_FILE_INIT_SIZE 값 리턴.
ULong smiTableSpace::getSysTempFileInitSize()
{
    return getValidSize4Disk( smuProperty::getSysTempFileInitSize() );
}

// BUG-14897 - 속성 SYS_TEMP_FILE_MAX_SIZE 값 리턴.
ULong smiTableSpace::getSysTempFileMaxSize()
{
    return getValidSize4Disk( smuProperty::getSysTempFileMaxSize() );
}

// BUG-14897 - 속성 SYS_TEMP_FILE_NEXT_SIZE 값 리턴.
ULong smiTableSpace::getSysTempFileNextSize()
{
    return smuProperty::getSysTempFileNextSize();
}

// BUG-14897 - 속성 USER_DATA_TBS_EXTENT_SIZE 값 리턴.
ULong smiTableSpace::getUserDataTBSExtentSize()
{
    return smuProperty::getUserDataTBSExtentSize();
}

// BUG-14897 - 속성 USER_DATA_FILE_INIT_SIZE 값 리턴.
ULong smiTableSpace::getUserDataFileInitSize()
{
    return getValidSize4Disk( smuProperty::getUserDataFileInitSize() );
}

// BUG-14897 - 속성 USER_DATA_FILE_MAX_SIZE 값 리턴.
ULong smiTableSpace::getUserDataFileMaxSize()
{
    return getValidSize4Disk( smuProperty::getUserDataFileMaxSize() );
}

// BUG-14897 - 속성 USER_DATA_FILE_NEXT_SIZE 값 리턴.
ULong smiTableSpace::getUserDataFileNextSize()
{
    return smuProperty::getUserDataFileNextSize();
}

// BUG-14897 - 속성 USER_TEMP_TBS_EXTENT_SIZE 값 리턴.
ULong smiTableSpace::getUserTempTBSExtentSize()
{
    return smuProperty::getUserTempTBSExtentSize();
}

// BUG-14897 - 속성 USER_TEMP_FILE_INIT_SIZE 값 리턴.
ULong smiTableSpace::getUserTempFileInitSize()
{
    return getValidSize4Disk( smuProperty::getUserTempFileInitSize() );
}

// BUG-14897 - 속성 USER_TEMP_FILE_MAX_SIZE 값 리턴.
ULong smiTableSpace::getUserTempFileMaxSize()
{
    return getValidSize4Disk( smuProperty::getUserTempFileMaxSize() );
}

// BUG-14897 - 속성 USER_TEMP_FILE_NEXT_SIZE 값 리턴.
ULong smiTableSpace::getUserTempFileNextSize()
{
    return smuProperty::getUserTempFileNextSize();
}

/**********************************************************************
 * Description:OS limit,file header를 고려한 적절한 크기를 얻어내도록 함.
 *
 * aSizeInBytes       - [IN]   바이트단위의 크기 
 **********************************************************************/
ULong  smiTableSpace::getValidSize4Disk( ULong aSizeInBytes )
{
    ULong sPageCount = aSizeInBytes / SD_PAGE_SIZE ;
    ULong sRet;
    UInt  sFileHdrSizeInBytes;
    UInt  sFileHdrPageCnt;

    // BUG-27911 file header 크기를 define 정의문으로 대체.
    sFileHdrSizeInBytes = idlOS::align( SM_DBFILE_METAHDR_PAGE_SIZE, SD_PAGE_SIZE );
    sFileHdrPageCnt = sFileHdrSizeInBytes / SD_PAGE_SIZE;

    if( ( sPageCount + sFileHdrPageCnt ) > sddDiskMgr::getMaxDataFileSize() )
    {
        sRet = (sddDiskMgr::getMaxDataFileSize() - sFileHdrPageCnt) * SD_PAGE_SIZE;
    }
    else
    {
        sRet = aSizeInBytes;
    }
    
    return sRet;
}
