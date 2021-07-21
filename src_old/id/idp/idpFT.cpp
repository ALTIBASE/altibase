/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpFT.cpp 74637 2016-03-07 06:01:43Z donovan.seo $
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idp.h>
#include <idErrorCode.h>


iduFixedTableColDesc gPropertyColDesc[] =
{
    {
        (SChar *)"SID",
        offsetof(idpBaseInfo, mSID),
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"NAME",
        offsetof(idpBaseInfo, mName),
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE_COUNT",
        offsetof(idpBaseInfo, mMemValCount),
        IDU_FT_SIZEOF(idpBaseInfo, mMemValCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ATTR",
        offsetof(idpBaseInfo, mAttr),
        IDU_FT_SIZEOF(idpBaseInfo, mAttr),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MIN",
        offsetof(idpBaseInfo, mMin),
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MAX",
        offsetof(idpBaseInfo, mMax),
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE1",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 0,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE2",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 1,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE3",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 2,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE4",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 3,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE5",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 4,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE6",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 5,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE7",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 6,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MEMORY_VALUE8",
        offsetof(idpBaseInfo, mMemVal) + ID_SIZEOF(void *) * 7,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE_COUNT",
        offsetof(idpBaseInfo, mDefaultCount),
        IDU_FT_SIZEOF(idpBaseInfo, mDefaultCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE1",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 0,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE2",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 1,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE3",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 2,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE4",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 3,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE5",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 4,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE6",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 5,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE7",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 6,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_VALUE8",
        offsetof(idpBaseInfo, mDefaultVal) + ID_SIZEOF(void *) * 7,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },        
    {
        (SChar *)"ENV_VALUE_COUNT",
        offsetof(idpBaseInfo, mEnvCount),
        IDU_FT_SIZEOF(idpBaseInfo, mEnvCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ENV_VALUE1",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 0,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },      
    {
        (SChar *)"ENV_VALUE2",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 1,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ENV_VALUE3",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 2,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ENV_VALUE4",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 3,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ENV_VALUE5",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 4,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ENV_VALUE6",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 5,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ENV_VALUE7",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 6,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ENV_VALUE8",
        offsetof(idpBaseInfo, mEnvVal) + ID_SIZEOF(void *) * 7,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },        
    {
        (SChar *)"PFILE_VALUE_COUNT",
        offsetof(idpBaseInfo, mPFileCount),
        IDU_FT_SIZEOF(idpBaseInfo, mPFileCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE1",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 0,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE2",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 1,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE3",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 2,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE4",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 3,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE5",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 4,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE6",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 5,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE7",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 6,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PFILE_VALUE8",
        offsetof(idpBaseInfo, mPFileVal) + ID_SIZEOF(void *) * 7,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE_COUNT",
        offsetof(idpBaseInfo, mSPFileByAsteriskCount),
        IDU_FT_SIZEOF(idpBaseInfo, mSPFileByAsteriskCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE1",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 0,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE2",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 1,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE3",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 2,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE4",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 3,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE5",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 4,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE6",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 5,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE7",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 6,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_ASTERISK_VALUE8",
        offsetof(idpBaseInfo, mSPFileByAsteriskVal) + ID_SIZEOF(void *) * 7,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE_COUNT",
        offsetof(idpBaseInfo, mSPFileBySIDCount),
        IDU_FT_SIZEOF(idpBaseInfo, mSPFileByAsteriskCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE1",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 0,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE2",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 1,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE3",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 2,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE4",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 3,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE5",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 4,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE6",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 5,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE7",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 6,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPFILE_BY_SID_VALUE8",
        offsetof(idpBaseInfo, mSPFileBySIDVal) + ID_SIZEOF(void *) * 7,
        IDP_MAX_VALUE_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        idpBase::convertToChar,
        0, 0,NULL // for internal use
    },        
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
};


iduFixedTableDesc gPropertyTable =
{
    (SChar *)"X$PROPERTY",
    idp::buildRecordCallback,
    gPropertyColDesc,
    IDU_STARTUP_INIT,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC idp::buildRecordCallback(idvSQL              * /* aStatistics */,
                                void                * aHeader,
                                void                * /* aDumpObj */,
                                iduFixedTableMemory * aMemory)
{
    UInt            sLstIdx, i;
    idpBaseInfo     sBaseData;
    idpBase*        sBase;
    iduList*        sBaseList;
    iduListNode*    sNode;
    void          * sIndexValues[1];

    for (sLstIdx = 0; sLstIdx < mCount; sLstIdx++)
    {
        sBaseList = &mArrBaseList[sLstIdx];
        /*하나의 프로퍼티 리스트의 각 항목에 대해서*/
        IDU_LIST_ITERATE(sBaseList, sNode)
        {
            sBase = (idpBase*)sNode->mObj;

            /* BUG-43006 FixedTable Indexing Filter
             * Indexing Filter를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sBase->mName;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gPropertyColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }
            sBaseData.mSID = sBase->mSID;
            sBaseData.mName = sBase->mName;
            sBaseData.mAttr = sBase->mAttr;
            sBaseData.mMin = sBase->mMin;
            sBaseData.mMax = sBase->mMax;            
            sBaseData.mBase = sBase;

            /*Property Memory Values*/
            sBaseData.mMemValCount = sBase->mMemVal.mCount;    
            for(i = 0; i < IDP_FIXED_TBL_VALUE_COUNT; i++)
            {
                sBaseData.mMemVal[i] = sBase->mMemVal.mVal[i];
            }
            /*Property Default Values*/
            sBaseData.mDefaultCount = sBase->mSrcValArr[IDP_VALUE_FROM_DEFAULT].mCount;    
            for(i = 0; i < IDP_FIXED_TBL_VALUE_COUNT; i++)
            {
                sBaseData.mDefaultVal[i] = sBase->mSrcValArr[IDP_VALUE_FROM_DEFAULT].mVal[i];
            }
            /*Property Env Values*/
            sBaseData.mEnvCount = sBase->mSrcValArr[IDP_VALUE_FROM_ENV].mCount;    
            for(i = 0; i < IDP_FIXED_TBL_VALUE_COUNT; i++)
            {
                sBaseData.mEnvVal[i] = sBase->mSrcValArr[IDP_VALUE_FROM_ENV].mVal[i];
            }
            /*Property PFILE Values*/    
            sBaseData.mPFileCount = sBase->mSrcValArr[IDP_VALUE_FROM_PFILE].mCount;    
            for(i = 0; i < IDP_FIXED_TBL_VALUE_COUNT; i++)
            {
                sBaseData.mPFileVal[i] = sBase->mSrcValArr[IDP_VALUE_FROM_PFILE].mVal[i];
            }
            /*Property SPFILE By Asterisk Values*/
            sBaseData.mSPFileByAsteriskCount = 
                sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_ASTERISK].mCount;    
            for(i = 0; i < IDP_FIXED_TBL_VALUE_COUNT; i++)
            {
                sBaseData.mSPFileByAsteriskVal[i] = 
                    sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_ASTERISK].mVal[i];
            }
            /*Property SPFILE By SID Values*/    
            sBaseData.mSPFileBySIDCount = sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_SID].mCount;    
            for(i = 0; i < IDP_FIXED_TBL_VALUE_COUNT; i++)
            {
                sBaseData.mSPFileBySIDVal[i] = 
                    sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_SID].mVal[i];
            }
            //make one fixed table record 
            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                        aMemory,
                        (void *)&(sBaseData))
                    != IDE_SUCCESS);

        }

    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * PROJ-2118 Bug Reporting
 */
void idp::dumpProperty( ideLogEntry &aLog )
{
    SInt             sCmpRlt;
    UInt             sLstIdx, i;
    UInt             sCount, sBufSize;
    SChar            sMemValBuf[256];
    idpBase         *sBase;
    iduList         *sBaseList;
    iduListNode     *sNode;
    void            *sDefVal, *sMemVal;

    sBufSize = ID_SIZEOF( sMemValBuf );

    aLog.append( "*--------------------------------------------------------*\n" );
    aLog.append( "*                   Changed Properties                   *\n" );
    aLog.append( "*--------------------------------------------------------*\n" );
    aLog.append( "[ Property name, ( Value count / Total ), Memory value ]\n\n" );
                                       // ex) AUTO_COMMIT, ( 0 / 0 ), 0

    for( sLstIdx = 0; sLstIdx < mCount; sLstIdx++ )
    {
        sBaseList = &mArrBaseList[sLstIdx];

        IDU_LIST_ITERATE(sBaseList, sNode) 
        {
            sBase   = (idpBase*)sNode->mObj;

            sDefVal = sBase->mSrcValArr[IDP_VALUE_FROM_DEFAULT].mVal[0]; // Default value
            sCount  = sBase->mMemVal.mCount; // Memory value count

            /***************************************************
             * Case 1 : sDefVal != NULL, sMemVal != NULL
             * Case 2 : sDefVal != NULL, sMemVal == NULL
             * Case 3 : sDefVal == NULL, sMemVal != NULL
             ***************************************************/
            for(i = 0; i < sCount; i++)
            {
                sMemVal = sBase->mMemVal.mVal[i]; // Memory value

                if ( sMemVal != NULL )
                {
                    // Case 1
                    if ( sDefVal != NULL )
                    {
                        sCmpRlt = sBase->compare( sDefVal, sMemVal );

                        if ( sCmpRlt != 0 )
                        {
                            idlOS::memset( &sMemValBuf, 0, sBufSize ); 
                            sBase->convertToString( sMemVal, (void *)sMemValBuf, sBufSize );
                            aLog.appendFormat( "%-32s, ( %u / %u ), %s\n",
                                               sBase->mName,
                                               i + 1,
                                               sCount,
                                               sMemValBuf );
                        }
                    }
                    // Case 3
                    else
                    {
                        idlOS::memset( &sMemValBuf, 0, sBufSize );
                        sBase->convertToString( sMemVal, (void *)sMemValBuf, sBufSize );
                        aLog.appendFormat( "%-32s, ( %u / %u ), %s\n",
                                           sBase->mName,
                                           i + 1,
                                           sCount,
                                           sMemValBuf );
                    }
                }
                // Case 2.
                else
                {
                    if ( sDefVal != NULL )
                    {
                        aLog.appendFormat( "%-32s, ( %u / %u ), %s\n",
                                           sBase->mName,
                                           i + 1,
                                           sCount,
                                           "NULL" );
                    }
                }
            }
        }
    }
}
