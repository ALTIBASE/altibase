/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpBase.cpp 87064 2020-03-30 04:21:12Z jake.jang $
 *
 * Description:
 *
 * 이 클래스는 프로퍼티 속성의 다양한 데이타 타입에 대한
 * 베이스 클래스이다.
 * 이 클래스에서는 pure virtual 함수를 몇가지 준비해 놓고 있으며,
 * 만일 새로운 데이타 타입의 프로퍼티가 추가될 경우에는
 * 그 타입에 맞는 상속 클래스를 만들어서 등록하면 된다.
 *
 * idp.cpp의 함수를 참조하는 것을 대신하고,
 * 이 클래스의 method에 대한 주석은 생략한다.
 *
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <idp.h>
#include <idu.h>

SChar *idpBase::mErrorBuf;

idpBase::idpBase()
{
    mVirtFunc             = NULL;
    mUpdateBefore = defaultChangeCallback;
    mUpdateAfter  = defaultChangeCallback;

    IDE_ASSERT( idlOS::thread_mutex_init(&mMutex) == 0 );
}

idpBase::~idpBase()
{
    IDE_ASSERT( idlOS::thread_mutex_destroy(&mMutex) == 0 );
}



IDE_RC idpBase::defaultChangeCallback(idvSQL*, SChar *, void *, void *, void *)
{
    return IDE_SUCCESS;
}

void idpBase::initializeStatic(SChar *aErrorBuf)
{
    mErrorBuf = aErrorBuf;
}

void   idpBase::setupBeforeUpdateCallback(idpChangeCallback mCallback)
{
    IDE_DASSERT( mUpdateBefore == defaultChangeCallback );
    mUpdateBefore = mCallback;
}

void   idpBase::setupAfterUpdateCallback(idpChangeCallback mCallback)
{
    IDE_DASSERT( mUpdateAfter == defaultChangeCallback );
    mUpdateAfter = mCallback;
}

IDE_RC idpBase::checkRange(void * aValue)
{
    if ( (mAttr & IDP_ATTR_CK_MASK) == IDP_ATTR_CK_CHECK )
    {
        IDE_TEST( validateRange(aValue) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*  Memovery Value에 값을 복제하여 삽입한다.
*
* void           *aValue,      - [IN]  삽입하려고하는 값의 포인터 (타입별 Raw Format)
*******************************************************************************************/ 
IDE_RC idpBase::insertMemoryRawValue(void *aValue) /* called by build() */
{
    void *sValue = NULL;
    
    // Multiple Flag Check
    IDE_TEST_RAISE( ((mAttr & IDP_ATTR_ML_MASK) == IDP_ATTR_ML_JUSTONE) &&
                    mMemVal.mCount == 1, only_one_error);
    
    // Store Count Check
    IDE_TEST_RAISE(mMemVal.mCount >= IDP_MAX_VALUE_COUNT,
                   no_more_insert);
    
    cloneValue(this, aValue, &sValue);
    
    /* Value Range Validation */
    IDE_TEST(checkRange(sValue) != IDE_SUCCESS);

    mMemVal.mVal[mMemVal.mCount++] = sValue;
    
    return IDE_SUCCESS;
    IDE_EXCEPTION(only_one_error);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insert() Error : "
                        "Property [%s] Can't Store Multiple Values.",
                        getName());
    }
    IDE_EXCEPTION(no_more_insert);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insert() Error : "
                        "Property [%s] Can't Store more than %"ID_UINT32_FMT
                        " Values.",
                        getName(), 
                        (UInt)IDP_MAX_VALUE_COUNT);
    }
    
    IDE_EXCEPTION_END;
    
    if (sValue != NULL)
    {
        iduMemMgr::freeRaw(sValue);
    }
    
    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*  aSrc로 들어온 value source 위치에 스트링 형태의 aValue를 자신의 타입으로 변환하고, 
*  값을 복제하여 삽입한다.
*
*  SChar         *aValue,   - [IN] 삽입하려고하는 값의 포인터 (String Format)
*  idpValueSource aSrc      - [IN] 값을 삽입할 Source 위치
*                                  (default/env/pfile/spfile by asterisk, spfile by sid)
*******************************************************************************************/ 
IDE_RC idpBase::insertBySrc(SChar *aValue, idpValueSource aSrc) 
{
    void *sValue = NULL;
    UInt sValueIdx;

    // Multiple Flag Check
    IDE_TEST_RAISE(((mAttr & IDP_ATTR_ML_MASK) == IDP_ATTR_ML_JUSTONE) &&
                   mSrcValArr[aSrc].mCount == 1, only_one_error);
    
    // Store Count Check
    IDE_TEST_RAISE(mSrcValArr[aSrc].mCount >= IDP_MAX_VALUE_COUNT,
                   no_more_insert);
    
    switch(aSrc)
    {
        case IDP_VALUE_FROM_PFILE:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_PFILE) != IDP_ATTR_SL_PFILE, 
                            err_cannot_set_from_pfile);
            break;
            
        case IDP_VALUE_FROM_SPFILE_BY_ASTERISK:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_SPFILE) != IDP_ATTR_SL_SPFILE, 
                            err_cannot_set_from_spfile);
            break;
            
        case IDP_VALUE_FROM_SPFILE_BY_SID:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_SPFILE) != IDP_ATTR_SL_SPFILE, 
                            err_cannot_set_from_spfile);
            break;        
        
        case IDP_VALUE_FROM_ENV:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_ENV) != IDP_ATTR_SL_ENV, 
                            err_cannot_set_from_env);
            break;
        
        default:
            //IDP_VALUE_FROM_DEFAULT NO CHECK 
            break;
    }

    IDE_TEST(convertFromString(aValue, &sValue) != IDE_SUCCESS);

    /* Value Range Validation */
    IDE_TEST(checkRange(sValue) != IDE_SUCCESS);
    
    sValueIdx = mSrcValArr[aSrc].mCount++;
    mSrcValArr[aSrc].mVal[sValueIdx] = sValue;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(only_one_error);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't Store Multiple Values.",
                        getName());
    }
    IDE_EXCEPTION(no_more_insert);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't Store more than %"ID_UINT32_FMT
                        " Values.",
                        getName(), 
                        (UInt)IDP_MAX_VALUE_COUNT);
    }
    IDE_EXCEPTION(err_cannot_set_from_pfile);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't set from PFILE.", 
                        getName());
    }
    IDE_EXCEPTION(err_cannot_set_from_spfile);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't set from SPFILE.", 
                        getName());
    }
    IDE_EXCEPTION(err_cannot_set_from_env);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't set from ENV.", 
                        getName());
    }
    
    IDE_EXCEPTION_END;
    
    if (sValue != NULL)
    {
        iduMemMgr::freeRaw(sValue);
    }
    
    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*  aSrc로 들어온 value source 위치에 Raw 형태의 aValue 
*  값을 복제하여 삽입한다.
*
*  SChar         *aValue,   - [IN] 삽입하려고하는 값의 포인터 (타입별 Raw Format)
*  idpValueSource aSrc      - [IN] 값을 삽입할 Source 위치
*                                  (default/env/pfile/spfile by asterisk, spfile by sid)
*******************************************************************************************/ 
IDE_RC idpBase::insertRawBySrc(void *aValue, idpValueSource aSrc) 
{
    void *sValue = NULL;    
    UInt sValueIdx;
    
    // Multiple Flag Check
    IDE_TEST_RAISE(((mAttr & IDP_ATTR_ML_MASK) == IDP_ATTR_ML_JUSTONE) &&
                   mSrcValArr[aSrc].mCount == 1, only_one_error);
    
    // Store Count Check
    IDE_TEST_RAISE(mSrcValArr[aSrc].mCount >= IDP_MAX_VALUE_COUNT,
                   no_more_insert);
    
    switch(aSrc)
    {
        case IDP_VALUE_FROM_PFILE:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_PFILE) != IDP_ATTR_SL_PFILE, 
                            err_cannot_set_from_pfile);
            break;
            
        case IDP_VALUE_FROM_SPFILE_BY_ASTERISK:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_SPFILE) != IDP_ATTR_SL_SPFILE, 
                            err_cannot_set_from_spfile);
            break;
            
        case IDP_VALUE_FROM_SPFILE_BY_SID:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_SPFILE) != IDP_ATTR_SL_SPFILE, 
                            err_cannot_set_from_spfile);
            break;        
        
        case IDP_VALUE_FROM_ENV:
            IDE_TEST_RAISE((mAttr & IDP_ATTR_SL_ENV) != IDP_ATTR_SL_ENV, 
                            err_cannot_set_from_env);
            break;
        
        default:
            //IDP_VALUE_FROM_DEFAULT NO CHECK 
            break;
    }

    cloneValue(this, aValue, &sValue);
    
    /* Value Range Validation */
    IDE_TEST(checkRange(aValue) != IDE_SUCCESS);
    
    sValueIdx = mSrcValArr[aSrc].mCount++;
    mSrcValArr[aSrc].mVal[sValueIdx] = aValue;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(only_one_error);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insert() Error : "
                        "Property [%s] Can't Store Multiple Values.",
                        getName());
    }
    IDE_EXCEPTION(no_more_insert);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insert() Error : "
                        "Property [%s] Can't Store more than %"ID_UINT32_FMT" Values.",
                        getName(), 
                        (UInt)IDP_MAX_VALUE_COUNT);
    }
    IDE_EXCEPTION(err_cannot_set_from_pfile);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't set from PFILE.", 
                        getName());
    }
    IDE_EXCEPTION(err_cannot_set_from_spfile);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't set from SPFILE.", 
                        getName());
    }
    IDE_EXCEPTION(err_cannot_set_from_env);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp insertBySrc() Error : "
                        "Property [%s] Can't set from ENV.", 
                        getName());
    }    
    IDE_EXCEPTION_END;
        
    if (sValue != NULL)
    {
        iduMemMgr::freeRaw(sValue);
    }
    
    return IDE_FAILURE;
}

IDE_RC idpBase::read(void *aOut, UInt aNum)
{
    idBool sLocked = ID_FALSE;


    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
    sLocked = ID_TRUE;


    IDE_TEST_RAISE(mMemVal.mCount == 0, not_initialized);

    // 카운트가 틀린 경우.
    IDE_TEST_RAISE(aNum >= mMemVal.mCount, no_exist_error);

    idlOS::memcpy(aOut, mMemVal.mVal[aNum], getSize(mMemVal.mVal[aNum]));

    sLocked = ID_FALSE;
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;
    IDE_EXCEPTION(no_exist_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NoSuchEntry, getName(), aNum));
    }
    IDE_EXCEPTION(not_initialized);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp read() Error : Property not loaded\n");
    }
    IDE_EXCEPTION_END;

    if ( sLocked == ID_TRUE )
    {
        IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

IDE_RC idpBase::readBySrc(void *aOut, idpValueSource aSrc, UInt aNum)
{
    idBool sLocked = ID_FALSE;


    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
    sLocked = ID_TRUE;

    IDE_TEST_RAISE(mSrcValArr[aSrc].mCount == 0, not_initialized);

    // 카운트가 틀린 경우.
    IDE_TEST_RAISE(aNum >= mSrcValArr[aSrc].mCount , no_exist_error);

    idlOS::memcpy(aOut, mSrcValArr[aSrc].mVal[aNum], getSize(mSrcValArr[aSrc].mVal[aNum]));

    sLocked = ID_FALSE;
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;
    IDE_EXCEPTION(no_exist_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NoSuchEntry, getName(), aNum));
    }
    IDE_EXCEPTION(not_initialized);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp readBySrc() Error : Property not loaded\n");
    }
    IDE_EXCEPTION_END;

    if ( sLocked == ID_TRUE )
    {
        IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

IDE_RC idpBase::readPtr(void **aOut, UInt aNum)
{
    idBool sLocked = ID_FALSE;

    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
    sLocked = ID_TRUE;

    IDE_TEST_RAISE(mMemVal.mCount == 0, not_initialized);

    // 카운트가 틀린 경우.
    IDE_TEST_RAISE(aNum >= mMemVal.mCount, no_exist_error);

    // 변경 가능한 경우 검사
    //IDE_TEST_RAISE( (mAttr & IDP_ATTR_RD_MASK) == IDP_ATTR_RD_WRITABLE,
    //                cant_read_error);

    // String의 경우만 리턴함.
    IDE_TEST_RAISE( (mAttr & IDP_ATTR_TP_MASK) != IDP_ATTR_TP_String,
                    cant_read_error);

    *aOut= mMemVal.mVal[aNum];

    sLocked = ID_FALSE;
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION(cant_read_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NotReadOnly, getName()));
    }
    IDE_EXCEPTION(no_exist_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NoSuchEntry, getName(), aNum));
    }
    IDE_EXCEPTION(not_initialized);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp read() Error : Property not loaded\n");
    }

    IDE_EXCEPTION_END;

    if ( sLocked == ID_TRUE )
    {
        IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
* 특정 source의 위치에 저장된 값의 포인터를 얻는다.
* 그 값은 더블 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
* 사용해야 한다.
* 대상 프로퍼티가, String 타입일 경우에만 유효하다.
*
* UInt           aNum,      - [IN]  프로퍼티에 저장된 n 번째 값을 의미하는 인덱스  
* idpValueSource aSrc,      - [IN]  어떤 설정 방법에 의해서 설정된 값인지를 나타내는 Value Source                                   
* void         **aOut,      - [OUT] 결과 값의 포인터
*******************************************************************************************/ 
IDE_RC idpBase::readPtrBySrc (UInt aNum, idpValueSource aSrc, void **aOut)
{
    idBool sLocked = ID_FALSE;

    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
    sLocked = ID_TRUE;

    IDE_TEST_RAISE(mSrcValArr[aSrc].mCount == 0, not_initialized);

    // 카운트가 틀린 경우.
    IDE_TEST_RAISE(aNum >= mSrcValArr[aSrc].mCount , no_exist_error);

    // 변경 가능한 경우 검사
    //IDE_TEST_RAISE( (mAttr & IDP_ATTR_RD_MASK) == IDP_ATTR_RD_WRITABLE,
    //                cant_read_error);

    // String의 경우만 리턴함.
    IDE_TEST_RAISE( (mAttr & IDP_ATTR_TP_MASK) != IDP_ATTR_TP_String,
                    cant_read_error);

    *aOut = (void*) mSrcValArr[aSrc].mVal[aNum];

    sLocked = ID_FALSE;
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION(cant_read_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NotReadOnly, getName()));
    }
    IDE_EXCEPTION(no_exist_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NoSuchEntry, getName(), aNum));
    }
    IDE_EXCEPTION(not_initialized);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp read() Error : Property not loaded\n");
    }

    IDE_EXCEPTION_END;

    if ( sLocked == ID_TRUE )
    {
        IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*  idp에서만 사용하기 위한 함수이며, 내부적으로 lock을 안잡기 때문에, 
*  다른 모듈에서는 이 함수를 통해서 값을 가져가면 안된다. 
*  aNum 번째 값을 반환한다. 
*
*  UInt       aNum   - [IN] 몇 번째 값인지 나타내는 index
*  void     **aOut   - [OUT] 반환되는 값
*******************************************************************************************/ 
IDE_RC idpBase::readPtr4Internal(UInt aNum, void **aOut)
{
    IDE_TEST_RAISE(mMemVal.mCount == 0, not_initialized);
    
    // 카운트가 틀린 경우.
    IDE_TEST_RAISE(aNum >= mMemVal.mCount, err_no_exist);
   
    *aOut= mMemVal.mVal[aNum];
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_no_exist);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, 
                        "The entry [<%d>] of the property [<%s>] does not exist.\n", 
                        aNum, getName());
    }
    IDE_EXCEPTION(not_initialized);
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idp read() Error : Property not loaded\n");
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

// BUG-43533 OPTIMIZER_FEATURE_ENABLE
IDE_RC idpBase::update4Startup(idvSQL *aStatistics, SChar *aIn, UInt aNum, void *aArg)
{
    // BUG-43533
    // Property file, 환경변수 등에 등록하지 않은 property만 변경한다.
    if ( (mSrcValArr[IDP_VALUE_FROM_PFILE].mCount == 0) &&
         (mSrcValArr[IDP_VALUE_FROM_ENV].mCount == 0) &&
         (mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_ASTERISK].mCount == 0) &&
         (mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_SID].mCount == 0) )
    {
        IDE_TEST( update( aStatistics, aIn, aNum, aArg )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idpBase::update(idvSQL *aStatistics, SChar *aIn, UInt aNum, void *aArg)
{
    void  *sValue       = NULL;
    void  *sOldValue    = NULL;
    UInt   sUpdateValue = 0;
    idBool sLocked      = ID_FALSE;

    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
    sLocked = ID_TRUE;

    // 카운트 검사
    IDE_TEST_RAISE(aNum >= mMemVal.mCount, no_exist_error);

    // 변경 불가능한 경우 검사
    IDE_TEST_RAISE( (mAttr & IDP_ATTR_RD_MASK) == IDP_ATTR_RD_READONLY,
                    cant_modify_error);

    if ( ( mAttr & IDP_ATTR_SK_MASK ) != IDP_ATTR_SK_MULTI_BYTE )
    {
        IDE_TEST(convertFromString(aIn, &sValue) != IDE_SUCCESS);
    }
    else
    {
        sValue = iduMemMgr::mallocRaw(idlOS::strlen((SChar *)aIn) + 1, IDU_MEM_FORCE);
        IDE_ASSERT(sValue != NULL);

        idlOS::strncpy((SChar *)sValue, (SChar *)aIn, idlOS::strlen((SChar *)aIn) + 1);
    }

    sOldValue = mMemVal.mVal[aNum];

    /* Value Range Validation */
    IDE_TEST(checkRange(sValue) != IDE_SUCCESS);

    IDE_TEST( mUpdateBefore(aStatistics, getName(), sOldValue, sValue, aArg)
              != IDE_SUCCESS);

    /* 기존 메모리 해제 & Set */
    mMemVal.mVal[aNum] = sValue;
    /* 프로퍼티 값을 바꾸었다고 표시하고 에러발생시 원래값으로 원복한다. */
    sUpdateValue = 1;

    IDE_TEST( mUpdateAfter(aStatistics, getName(), sOldValue, sValue, aArg)
              != IDE_SUCCESS);

    iduMemMgr::freeRaw(sOldValue);

    sLocked = ID_FALSE;
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION(cant_modify_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_ReadOnlyEntry, getName()));
    }
    IDE_EXCEPTION(no_exist_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, getName()));
    }
    IDE_EXCEPTION_END;

    /* BUG-17763: idpBase::update()에서 FMR을 유발시키고 있습니다.
     *
     * 이전에 원래값을 에러발생시 Free시켜서 문제가 되었습니다.
     * Free시키고 않고
     * 에러발생시 프로퍼티 값을 원래 값으로 원복해야 합니다. */
    if( sUpdateValue ==  1 )
    {
        mMemVal.mVal[aNum] = sOldValue;
    }

    if (sValue != NULL)
    {
        iduMemMgr::freeRaw(sValue);
    }

    if ( sLocked == ID_TRUE )
    {
        IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

IDE_RC idpBase::updateForce(SChar *aIn, UInt aNum, void *aArg)
{
    void  *sValue       = NULL;
    void  *sOldValue    = NULL;
    UInt   sUpdateValue = 0;
    idBool sLocked      = ID_FALSE;

    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
    sLocked = ID_TRUE;

    // 카운트 검사
    IDE_TEST_RAISE(aNum >= mMemVal.mCount, no_exist_error);

    /* 변경 불가능한 경우를 검사하지 않는다.
    IDE_TEST_RAISE( (mAttr & IDP_ATTR_RD_MASK) == IDP_ATTR_RD_READONLY,
                    cant_modify_error);
                    */

    if ( ( mAttr & IDP_ATTR_SK_MASK ) != IDP_ATTR_SK_MULTI_BYTE )
    {
    IDE_TEST(convertFromString(aIn, &sValue) != IDE_SUCCESS);
    }
    else
    {
        sValue = iduMemMgr::mallocRaw(idlOS::strlen((SChar *)aIn) + 1, IDU_MEM_FORCE);
        IDE_ASSERT(sValue != NULL);

        idlOS::strncpy((SChar *)sValue, (SChar *)aIn, idlOS::strlen((SChar *)aIn) + 1);
    }

    sOldValue = mMemVal.mVal[aNum];

    /* Value Range Validation */
    IDE_TEST(checkRange(sValue) != IDE_SUCCESS);

    IDE_TEST( mUpdateBefore(NULL, getName(), sOldValue, sValue, aArg)
              != IDE_SUCCESS);

    /* 기존 메모리 해제 & Set */
    mMemVal.mVal[aNum] = sValue;
    /* 프로퍼티 값을 바꾸었다고 표시하고 에러발생시 원래값으로 원복한다. */
    sUpdateValue = 1;

    IDE_TEST( mUpdateAfter(NULL, getName(), sOldValue, sValue, aArg)
              != IDE_SUCCESS);

    iduMemMgr::freeRaw(sOldValue);

    sLocked = ID_FALSE;
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;

    /* 변경 불가능한 경우를 검사하지 않는다.
    IDE_EXCEPTION(cant_modify_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_ReadOnlyEntry, getName()));
    }
    */
    IDE_EXCEPTION(no_exist_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, getName()));
    }
    IDE_EXCEPTION_END;

    /* BUG-17763: idpBase::update()에서 FMR을 유발시키고 있습니다.
     *
     * 이전에 원래값을 에러발생시 Free시켜서 문제가 되었습니다.
     * Free시키고 않고
     * 에러발생시 프로퍼티 값을 원래 값으로 원복해야 합니다. */
    if( sUpdateValue ==  1 )
    {
        mMemVal.mVal[aNum] = sOldValue;
    }

    if (sValue != NULL)
    {
        iduMemMgr::freeRaw(sValue);
    }

    if ( sLocked == ID_TRUE )
    {
        IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description:
 *     1.aIn에 해당하는 값이 Property의 Min, Max사이에 있는지 조사한다.
 *     2.aIsSystem에 해당하는 Property의 속성 체크
 *           SYSTEM, SESSION PROPERTY 동일하게 체크하기 때문에 두 속성이
 *           다른 경우 구분하여 처리
 *           ex> DATE_FORMAT, TIME_ZONE
 *     3.SHARD USER, LIBRARY, COORDINATION SESSION Property attribute구분
 * aIn       - [IN] Input Value
 * aIsSystem - [IN] System property 
 *************************************************************************/
IDE_RC idpBase::validate( SChar * aIn,
                          idBool  aIsSystem )
{
    void  * sValue    = NULL;
    idBool  sLocked   = ID_FALSE;

    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
    sLocked = ID_TRUE;

    if ( aIsSystem == ID_TRUE )
    {
        // 변경 불가능한 경우 검사
        IDE_TEST_RAISE( (mAttr & IDP_ATTR_RD_MASK) == IDP_ATTR_RD_READONLY,
                        cant_modify_error);
    }
    else
    {
        // nothing to do
    }

    IDE_TEST(convertFromString(aIn, &sValue) != IDE_SUCCESS);

    /* Value Range Validation */
    IDE_TEST(checkRange(sValue) != IDE_SUCCESS);

    // BUG-20486
    iduMemMgr::freeRaw(sValue);
    sValue = NULL;

    sLocked = ID_FALSE;
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION(cant_modify_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_ReadOnlyEntry, getName()));
    }
    IDE_EXCEPTION_END;

    // BUG-20486
    if(sValue != NULL)
    {
        iduMemMgr::freeRaw(sValue);
        sValue = NULL;
    }

    if ( sLocked == ID_TRUE )
    {
        IDE_PUSH();
        IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);
        IDE_POP();
    }

    return IDE_FAILURE;
}

// PROJ-2727
IDE_RC idpBase::getPropertyAttribute( UInt * aOut )
{
    IDE_ASSERT( idlOS::thread_mutex_lock(&mMutex) == 0);
        
    if (( mAttr & IDP_ATTR_SH_MASK ) == IDP_ATTR_SH_ALL )
    {
        *aOut = IDP_ATTR_SHARD_ALL;
    }
    else if (( mAttr & IDP_ATTR_SH_MASK ) == IDP_ATTR_SH_USER )
    {
        *aOut = IDP_ATTR_SHARD_USER;
    }
    else if (( mAttr & IDP_ATTR_SH_MASK ) == IDP_ATTR_SH_COORD )
    {
        *aOut = IDP_ATTR_SHARD_COORD;
    }
    else
    {
        *aOut = IDP_ATTR_SHARD_LIB;
    }
    
    IDE_ASSERT( idlOS::thread_mutex_unlock(&mMutex) == 0);

    return IDE_SUCCESS;
}

IDE_RC idpBase::verifyInsertedValues()
{
    UInt i;
    UInt sMultiple;

    // [1] Check Multiple Value Consistency, 설정된 값이 정확히 n개여야하는 값 확인
    sMultiple = mAttr & IDP_ATTR_ML_MASK;

    if ( (sMultiple != IDP_ATTR_ML_JUSTONE) &&
         (sMultiple != IDP_ATTR_ML_MULTIPLE)
       ) 
    {
        /* 고정 갯수에 대한 검사 요망 */

        UInt sCurCount;

        sCurCount = (UInt)IDP_ATTR_ML_COUNT(mAttr); /* 설정된 갯수 */

        IDE_TEST_RAISE(sCurCount != mMemVal.mCount, multiple_count_error);

    }

    // [3] Check Range of Insered Value

    for (i = 0; i < mMemVal.mCount; i++)
    {
        IDE_TEST(checkRange(mMemVal.mVal[i]) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(multiple_count_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp verifyInsertedValues() Error : "
                        "Property [%s] must have %"ID_UINT32_FMT" Value Entries."
                        " But, %"ID_UINT32_FMT" Entries Exist",
                        getName(), (UInt)IDP_ATTR_ML_COUNT(mAttr), mMemVal.mCount);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * 디스크립터가 등록된 이후 호출됨.
 * 이 함수에선 실제로 값이 Conf로 부터 입력되기 전까지의
 * 선행 작업이 이루어져야 함.
 */

void idpBase::registCallback()
{
    /*
      Environment Check & Read
      [ALTIBASE_]PROPNAME
    */

    void  *sValue;
    SChar *sEnvName;
    SChar *sEnvValue;
    UInt   sLen;

    sLen = idlOS::strlen(IDP_PROPERTY_PREFIX) + idlOS::strlen(getName()) + 2;

    sEnvName = (SChar *)iduMemMgr::mallocRaw(sLen);

    IDE_ASSERT(sEnvName != NULL);

    idlOS::memset(sEnvName, 0, sLen);

    idlOS::snprintf(sEnvName, sLen, "%s%s", IDP_PROPERTY_PREFIX, getName());

    sEnvValue = idlOS::getenv( (const SChar *)sEnvName);

    // Re-Validation of return-Value
    if (sEnvValue != NULL)
    {
        if (idlOS::strlen(sEnvValue) == 0)
        {
            sEnvValue = NULL;
        }
    }

    // If Exist, Read It.
    if (sEnvValue != NULL)
    {
        sValue = NULL;

        if (convertFromString(sEnvValue, &sValue) == IDE_SUCCESS)
        {
            mSrcValArr[IDP_VALUE_FROM_ENV].mVal[0] = sValue;
            mSrcValArr[IDP_VALUE_FROM_ENV].mCount++;
        }
        else
        {
            /* ------------------------------------------------
             *  환경변수 프로퍼티의 값 스트링이
             *  Data Type이 맞지 않아 실패할 경우에는
             *  Default Value를 그대로 쓴다.
             * ----------------------------------------------*/
            ideLog::log(IDE_SERVER_0, ID_TRC_PROPERTY_TYPE_INVALID, sEnvName, sEnvValue);
        }
    }

    iduMemMgr::freeRaw(sEnvName);

}

UInt idpBase::convertToChar(void        *aBaseObj,
                            void        *aMember,
                            UChar       *aBuf,
                            UInt         aBufSize)
{
    idpBase *sBase = (idpBase *)(((idpBaseInfo *)aBaseObj)->mBase);

    return sBase->convertToString(aMember, aBuf, aBufSize);
}

