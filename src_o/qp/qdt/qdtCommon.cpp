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
 * $Id: qdtCommon.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smiDef.h>
#include <smiTableSpace.h>
#include <qdtCommon.h>
#include <qtc.h>
#include <qcm.h>
#include <qmc.h>
#include <qcmTableSpace.h>
#include <qcuProperty.h>
#include <qcuSqlSourceInfo.h>
#include <qdpRole.h>

IDE_RC qdtCommon::validateFilesSpec(
        qcStatement       * /* aStatement */,
        smiTableSpaceType   aType,
        qdTBSFilesSpec    * aFilesSpec,
        ULong               aExtentSize,
        UInt              * aDataFileCount)
{
/***********************************************************************
 *
 * Description :
 *    file specification validation 함수
 *
 * Implementation :
 *    for (each file spec)
 *    {
 *    1.1 SIZE 를 명시하지 않은 경우 기본 크기 100M 를 파스트리에 설정
 *    1.2 AUTOEXTEND ON 을 명시한 경우
 *      1.2.1 NEXT size 를 명시하지 않은 경우 size of extent * n 값으로
 *              설정한다. N 값은 프로퍼티 파일에 정의된 값을 읽어온다.
 *      1.2.2 MAXSIZE 를 명시하지 않은 경우 : Property 값으로 설정
 *            MAXSIZE를 명시한 경우 : 명시한 값으로 설정
 *            MAXSIZE가 UNLIMITED 인 경우 : MAXSIZE = UNLIMITED 설정
 *    1.3 AUTOEXTEND OFF 명시한 경우
 *      1.3.1 MAXSIZE = 0 설정
 *    1.4 AUTOEXTEND 명시하지 않은 경우 OFF 값이 기본값
 *    }
 *    2 file 개수 counting 한 값을 smiTableSpaceAttr->mDataFileCount 에 설정
 *
 ***********************************************************************/

    UInt                sFileCnt;
    qdTBSFilesSpec    * sFilesSpec;
    smiDataFileAttr   * sFileAttr;
    ULong               sNextSize;
    ULong               sMaxSize;
    ULong               sInitSize;
    ULong               sDiskMaxDBSize;
    ULong               sNewDiskSize;
    ULong               sCurTotalDiskDBPageCnt;
    idBool              sIsSizeUnlimited;

    sNewDiskSize   = 0;
    sIsSizeUnlimited   = ID_FALSE;

    sFileCnt = 0;
    for ( sFilesSpec = aFilesSpec;
          sFilesSpec != NULL;
          sFilesSpec = sFilesSpec->next )
    {
        sFileAttr = sFilesSpec->fileAttr;

        // BUG-32255
        // If nextsize was not defined, it is defined to property value.
        if ( sFileAttr->mNextSize == ID_ULONG_MAX )
        {
            switch( aType )
            {
                case SMI_DISK_SYSTEM_DATA:
                {
                    sFileAttr->mNextSize =
                        smiTableSpace::getSysDataFileNextSize();
                    break;
                }
                case SMI_DISK_USER_DATA:
                {
                    sFileAttr->mNextSize =
                        smiTableSpace::getUserDataFileNextSize();
                    break;
                }
                case SMI_DISK_SYSTEM_TEMP:
                {
                    sFileAttr->mNextSize =
                        smiTableSpace::getSysTempFileNextSize();
                    break;
                }
                case SMI_DISK_USER_TEMP:
                {
                    sFileAttr->mNextSize =
                        smiTableSpace::getUserTempFileNextSize();
                    break;
                }
                case SMI_DISK_SYSTEM_UNDO:
                {
                    sFileAttr->mNextSize =
                        smiTableSpace::getSysUndoFileNextSize();
                    break;
                }
                default:
                {
                    IDE_ASSERT(0);
                }
            }
        }

        IDE_TEST_RAISE( sFileAttr->mIsAutoExtend == ID_TRUE &&
                        sFileAttr->mNextSize < aExtentSize,
                        ERR_INVALID_NEXT_SIZE );

        if ( sFileAttr->mMaxSize == ID_ULONG_MAX )
        {
            // Max Size를 명시하지 않은 경우, Property를 보고 설정함
            switch( aType )
            {
                case SMI_DISK_SYSTEM_DATA:
                {
                    sFileAttr->mMaxSize =
                        smiTableSpace::getSysDataFileMaxSize();
                    break;
                }
                case SMI_DISK_USER_DATA:
                {
                    sFileAttr->mMaxSize =
                        smiTableSpace::getUserDataFileMaxSize();
                    break;
                }
                case SMI_DISK_SYSTEM_TEMP:
                {
                    sFileAttr->mMaxSize =
                        smiTableSpace::getSysTempFileMaxSize();
                    break;
                }
                case SMI_DISK_USER_TEMP:
                {
                    sFileAttr->mMaxSize =
                        smiTableSpace::getUserTempFileMaxSize();
                    break;
                }
                case SMI_DISK_SYSTEM_UNDO:
                {
                    sFileAttr->mMaxSize =
                        smiTableSpace::getSysUndoFileMaxSize();
                    break;
                }
                default:
                {
                    IDE_ASSERT(0);
                }
            }
        }
        else
        {
            // Max Size를 명시한 경우 : 명시한 값으로 설정됨 
            // unlimited 로 명시한 경우 : mMaxSize가 0임
        }

        //------------------------------------------------
        // To Fix BUG-10415
        // Init Size가 주어지지 않았을 경우, property에서 init size을
        // 읽어와야함
        //------------------------------------------------
        
        if ( sFileAttr->mInitSize == 0 )
        {
            // Init Size가 주어지지 않았을 경우,
            // Property 에서 읽어온 Init Size 로 설정함
            switch( aType )
            {
                case SMI_DISK_SYSTEM_DATA:
                {
                    sFileAttr->mInitSize =
                        smiTableSpace::getSysDataFileInitSize();
                    break;
                }
                case SMI_DISK_USER_DATA:
                {
                    sFileAttr->mInitSize =
                        smiTableSpace::getUserDataFileInitSize();
                    break;
                }
                case SMI_DISK_SYSTEM_TEMP:
                {
                    sFileAttr->mInitSize =
                        smiTableSpace::getSysTempFileInitSize();
                    break;
                }
                case SMI_DISK_USER_TEMP:
                {
                    sFileAttr->mInitSize =
                        smiTableSpace::getUserTempFileInitSize();
                    break;
                }
                case SMI_DISK_SYSTEM_UNDO:
                {
                    sFileAttr->mInitSize =
                        smiTableSpace::getSysUndoFileInitSize();
                    break;
                }
                default:
                {
                    IDE_ASSERT(0);
                }
            }
        }
        else
        {
            // nothing to do
        }
        
        // To Fix BUG-10378
        sNextSize =  sFileAttr->mNextSize / (ULong)smiGetPageSize(aType);
        sNextSize = ( sNextSize < 1 ) ? 1 : sNextSize;
        sFileAttr->mNextSize = sNextSize;

        // To Fix BUG-10501
        sMaxSize =  sFileAttr->mMaxSize;
        if ( sMaxSize != 0 )
        {
            //------------------------------------------------
            // MaxSize가 0이 아닌 경우
            // - Max Size가 명시된 경우
            // - Max Size가 명시되지 않아 Property값으로 설정된 경우
            //------------------------------------------------
            
            sMaxSize =  sMaxSize / (ULong)smiGetPageSize(aType);
            sMaxSize = ( sMaxSize < 1 ) ? 1 : sMaxSize;
            sFileAttr->mMaxSize = sMaxSize;
        }
        else
        {
            //------------------------------------------------
            // Max Size가 0인 경우
            // - Auto Extend Off 인 경우
            // - Max Size를 unlimited로 설정한 경우임
            //------------------------------------------------
        }

        sInitSize =  sFileAttr->mInitSize / (ULong)smiGetPageSize(aType);
        sInitSize = ( sInitSize < 1 ) ? 1 : sInitSize;
        sFileAttr->mInitSize = sInitSize;
        
        sFileAttr->mCurrSize = sFileAttr->mInitSize;
        sFileCnt++;

        if (sFileAttr->mIsAutoExtend == ID_TRUE)
        {
            if (sFileAttr->mMaxSize == 0)
            {
                sIsSizeUnlimited = ID_TRUE;
            }
            else
            {
                sNewDiskSize += sFileAttr->mMaxSize;
            }
        }
        else
        {
            sNewDiskSize += sFileAttr->mInitSize;
        }
    }
    *aDataFileCount = sFileCnt;

    sDiskMaxDBSize = iduProperty::getDiskMaxDBSize();

    if (sDiskMaxDBSize != ID_ULONG_MAX)
    {
        /*
         * TASK-6327 Community Edition License
         * 새로 생성하는 tablespace 로 인하여
         * 총 maxsize 가 DISK_MAX_DB_SIZE 를 넘어갈 경우 에러
         */
        sCurTotalDiskDBPageCnt = smiGetDiskDBFullSize();

        IDE_TEST_RAISE((sIsSizeUnlimited == ID_TRUE) ||
                       (sCurTotalDiskDBPageCnt + sNewDiskSize >
                        sDiskMaxDBSize / (ULong)smiGetPageSize(aType)),
                       ERR_DB_MAXSIZE_EXCEED);
    }
    else
    {
        /* DISK_MAX_DB_SIZE = unlimited */
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_NEXT_SIZE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_INVALID_NEXT_SIZE));
    }
    IDE_EXCEPTION(ERR_DB_MAXSIZE_EXCEED)
    {
        // BUG-44570
        // DISK_MAX_DB_SIZE 초과로 에러 발생시 사용자에게 부가적인 정보를 알려주도록 합니다.
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_DB_MAXSIZE_EXCEED,
                                (sDiskMaxDBSize/1024/1024),
                                ((sDiskMaxDBSize - (sCurTotalDiskDBPageCnt*smiGetPageSize(aType)))/1024/1024)));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qdtCommon::getAndValidateIndexTBS( qcStatement       * aStatement,
                                   scSpaceID           aTableTBSID,
                                   smiTableSpaceType   aTableTBSType,
                                   qcNamePosition      aIndexTBSName,
                                   UInt                aIndexOwnerID,
                                   scSpaceID         * aIndexTBSID,
                                   smiTableSpaceType * aIndexTBSType )
{
/***********************************************************************
 *
 * Description :
 *     INDEX TABLESPACE tbs_name에 대한 Validation을 하고
 *     Index Tablespace의 ID와 Type을 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdtCommon::getAndValidateIndexTBS"

    smiTableSpaceAttr     sIndexTBSAttr;
    
    //-----------------------------------------
    // 적합성 검사
    //-----------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aIndexTBSID != NULL );
    IDE_DASSERT( aIndexTBSType != NULL );

    if (QC_IS_NULL_NAME( aIndexTBSName ) != ID_TRUE)
    {
        IDE_TEST( qcmTablespace::getTBSAttrByName(
                      aStatement,
                      aIndexTBSName.stmtText + aIndexTBSName.offset,
                      aIndexTBSName.size,
                      & sIndexTBSAttr ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIndexTBSAttr.mType == SMI_DISK_SYSTEM_TEMP ||
                        sIndexTBSAttr.mType == SMI_DISK_USER_TEMP   ||
                        sIndexTBSAttr.mType == SMI_DISK_SYSTEM_UNDO,
                        ERR_NO_CREATE_IN_SYSTEM_TBS );

        IDE_TEST( qdpRole::checkAccessTBS( aStatement,
                                           aIndexOwnerID,
                                           sIndexTBSAttr.mID)
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sIndexTBSAttr.mType == SMI_MEMORY_SYSTEM_DICTIONARY,
            ERR_INVALID_INDEX_TABLESPACE_MEDIA );

        // To Fix PR-9770
        // Memory Table에 Disk Index를 또는
        // Disk Table에 Memory Index를 생성할 수 없다.
        // 저장 매체가 동일한지 검사하여 저장 매체가 다르면 오류
        IDE_TEST_RAISE( isSameTBSType( aTableTBSType,
                                       sIndexTBSAttr.mType ) == ID_FALSE,
                        ERR_INVALID_INDEX_TABLESPACE_MEDIA );

        *aIndexTBSID   = sIndexTBSAttr.mID;
        *aIndexTBSType = sIndexTBSAttr.mType;
    }
    else // TABLESPACENAME 명시하지 않은 경우
    {
        *aIndexTBSID   = aTableTBSID;
        *aIndexTBSType = aTableTBSType;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_CREATE_IN_SYSTEM_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_CREATE_IN_SYSTEM_TBS));
    }
    IDE_EXCEPTION(ERR_INVALID_INDEX_TABLESPACE_MEDIA);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDX_INVALID_INDEX_MEDIA));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC
qdtCommon::getAndValidateTBSOfIndexPartition( 
    qcStatement       * aStatement,
    scSpaceID           aTablePartTBSID,
    smiTableSpaceType   aTablePartTBSType,
    qcNamePosition      aIndexTBSName,
    UInt                aIndexOwnerID,
    scSpaceID         * aIndexTBSID,
    smiTableSpaceType * aIndexTBSType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *    각 테이블 파티션의 TABLESPACE에 대한 validation
 *
 * Implementation :
 *    1. TABLESPACE 명시한 경우
 *      1.1 SM 에서 존재하는 테이블스페이스인지 체크
 *      1.2 테이블스페이스의 종류가 UNDO tablespace 또는 
 *          temporary tablespace 이면 오류
 *
 *    2. TABLESPACE 명시하지 않은 경우
 *      2.1 USER_ID 로 SYS_USERS_ 검색해서 DEFAULT_TBS_ID 값을 읽어 테이블을
 *          위한 테이블스페이스로 지정
 *
 *
 ***********************************************************************/

    smiTableSpaceAttr     sIndexTBSAttr;
    
    //-----------------------------------------
    // 적합성 검사
    //-----------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aIndexTBSID != NULL );
    IDE_DASSERT( aIndexTBSType != NULL );

    //-----------------------------------------
    // 1. TABLESPACE 명시한 경우
    //-----------------------------------------
    if (QC_IS_NULL_NAME( aIndexTBSName ) == ID_FALSE )
    {
        IDE_TEST( qcmTablespace::getTBSAttrByName(
                      aStatement,
                      aIndexTBSName.stmtText + aIndexTBSName.offset,
                      aIndexTBSName.size,
                      & sIndexTBSAttr ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIndexTBSAttr.mType == SMI_DISK_SYSTEM_TEMP ||
                        sIndexTBSAttr.mType == SMI_DISK_USER_TEMP   ||
                        sIndexTBSAttr.mType == SMI_DISK_SYSTEM_UNDO,
                        ERR_NO_CREATE_IN_SYSTEM_TBS );

        IDE_TEST( qdpRole::checkAccessTBS( aStatement,
                                           aIndexOwnerID,
                                           sIndexTBSAttr.mID)
                  != IDE_SUCCESS );

        // 테이블 파티션의 TBS와 인덱스 파티션의 TBS의 종류가 다르면 오류
        // 저장 매체가 동일한지 검사하여 저장 매체가 다르면 오류
        IDE_TEST_RAISE( isSameTBSType( aTablePartTBSType,
                                       sIndexTBSAttr.mType ) == ID_FALSE,
                        ERR_INVALID_INDEX_TABLESPACE_MEDIA );

        *aIndexTBSID   = sIndexTBSAttr.mID;
        *aIndexTBSType = sIndexTBSAttr.mType;
    }
    //-----------------------------------------
    // 2. TABLESPACENAME 명시하지 않은 경우
    //    테이블 파티션의 TBS를 따른다.
    //-----------------------------------------
    else
    {
        *aIndexTBSID   = aTablePartTBSID;
        *aIndexTBSType = aTablePartTBSType;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_CREATE_IN_SYSTEM_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_CREATE_IN_SYSTEM_TBS));
    }
    IDE_EXCEPTION(ERR_INVALID_INDEX_TABLESPACE_MEDIA);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDX_INVALID_INDEX_MEDIA));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 여러 개의 Attribute Flag List의 Flag값을
   Bitwise Or연산 하여 하나의 UInt 형의 Flag 값을 만든다

   [IN] aAttrFlagList - Tablespace Attribute Flag의 List
   [OUT] aAttrFlag - Bitwise OR된 Flag
*/
IDE_RC qdtCommon::getTBSAttrFlagFromList(qdTBSAttrFlagList * aAttrFlagList,
                                         UInt              * aAttrFlag )
{
    IDE_DASSERT( aAttrFlagList != NULL );
    IDE_DASSERT( aAttrFlag != NULL );

    UInt sAttrFlag = 0;
    
    for ( ; aAttrFlagList != NULL ; aAttrFlagList = aAttrFlagList->next )
    {
        sAttrFlag |= aAttrFlagList->attrValue;
    }

    *aAttrFlag = sAttrFlag;
    
    return IDE_SUCCESS;
}


/*
    Tablespace의 Attribute Flag List에 대한 Validation수행

   [IN] qcStatement - Tablespace Attribute가 사용된 Statement
   [IN] aAttrFlagList - Tablespace Attribute Flag의 List
 */
IDE_RC qdtCommon::validateTBSAttrFlagList(qcStatement       * aStatement,
                                          qdTBSAttrFlagList * aAttrFlagList)
{
    IDE_DASSERT( aAttrFlagList != NULL );
    
    // 같은 이름의 Attribute Flag가 존재 하면 에러 처리
    IDE_TEST( checkTBSAttrIsUnique( aStatement,
                                    aAttrFlagList ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

                                  
/*
    Tablespace의 Attribute Flag List에서 동일한
    Attribute List가 존재할 경우 에러처리

    ex>  동일한 Attribute가 다른 값으로 두 번 나오면 에러
         COMPRESSED LOGGING UNCOMPRESSED LOGGING 

    Detect할 방법 :
         Mask가 겹치는 Attribute가 존재하는지 체크한다.

   [IN] qcStatement - Tablespace Attribute가 사용된 Statement
   [IN] aAttrFlagList - Tablespace Attribute Flag의 List
 */
IDE_RC qdtCommon::checkTBSAttrIsUnique(qcStatement       * aStatement,
                                       qdTBSAttrFlagList * aAttrFlagList)
{
    IDE_DASSERT( aAttrFlagList != NULL );

    qcuSqlSourceInfo    sqlInfo;
    qdTBSAttrFlagList * sAttrFlag;
    
    for ( ; aAttrFlagList != NULL ; aAttrFlagList = aAttrFlagList->next )
    {
        for ( sAttrFlag = aAttrFlagList->next;
              sAttrFlag != NULL;
              sAttrFlag = sAttrFlag->next )
        {
            if ( aAttrFlagList->attrMask & sAttrFlag->attrMask )
            {
                IDE_RAISE(err_same_duplicate_attribute);
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_same_duplicate_attribute);
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sAttrFlag->attrPosition );

        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDT_DUPLICATE_TBS_ATTRIBUTE,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

idBool qdtCommon::isSameTBSType( smiTableSpaceType  aTBSType1,
                                 smiTableSpaceType  aTBSType2 )
{
    idBool  sIsSame = ID_TRUE;
    
    if ( smiTableSpace::isMemTableSpaceType( aTBSType1 ) == ID_TRUE )
    {
        // Memory Table인 경우
        if ( smiTableSpace::isMemTableSpaceType( aTBSType2 ) == ID_FALSE )
        {
            sIsSame = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        if ( smiTableSpace::isVolatileTableSpaceType( aTBSType1 ) == ID_TRUE )
        {
            // Volatile Table인 경우
            if ( smiTableSpace::isVolatileTableSpaceType( aTBSType2 ) == ID_FALSE )
            {
                sIsSame = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Disk Table인 경우
            IDE_DASSERT( smiTableSpace::isDiskTableSpaceType( aTBSType1 )
                         == ID_TRUE );
        
            if ( smiTableSpace::isDiskTableSpaceType( aTBSType2 ) == ID_FALSE )
            {
                sIsSame = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return sIsSame;
}
