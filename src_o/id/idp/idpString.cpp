/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpString.cpp 68698 2015-01-28 02:32:20Z djin $
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <idp.h>
#include <idu.h>
#include <idpString.h>

static UInt idpStringGetSize(void *, void *aSrc)
{
    return idlOS::strlen((SChar *)aSrc) + 1;
}

static SInt idpStringCompare(void *aObj, void *aVal1, void *aVal2)
{
    idpString *obj = (idpString *)aObj;
    return obj->compare(aVal1, aVal2);
}

static IDE_RC idpStringValidateLength(void* aObj, void *aVal)
{
    idpString *obj = (idpString *)aObj;
    return obj->validateLength(aVal);
}

static IDE_RC idpStringConvertFromString(void *aObj, void *aString, void **aResult)
{
    idpString *obj = (idpString *)aObj;
    return obj->convertFromString(aString, aResult);
}

static UInt idpStringConvertToString(void *aObj,
                                   void *aSrcMem,
                                   void *aDestMem,
                                   UInt aDestSize)
{
    idpString *obj = (idpString *)aObj;
    return obj->convertToString(aSrcMem, aDestMem, aDestSize);
}

static IDE_RC idpStringClone(void* aObj, SChar* aSID, void** aCloneObj)
{
    idpString* obj = (idpString *)aObj;
    return obj->clone(obj, aSID, aCloneObj);
}

static void idpStringCloneValue(void* aObj, void* aSrc, void** aDst)
{
    idpString* obj = (idpString *)aObj;
    obj->cloneValue(aSrc, aDst);
}

static idpVirtualFunction gIdpVirtFuncString =
{
    idpStringGetSize,
    idpStringCompare,
    idpStringValidateLength,
    idpStringConvertFromString,
    idpStringConvertToString,
    idpStringClone,
    idpStringCloneValue
};

idpString::idpString(const SChar *aName,
                     idpAttr      aAttr,
                     UInt         aMinLength,
                     UInt         aMaxLength,
                     const SChar *aDefault)
{
    UInt sValSrc, sValNum;

    IDE_ASSERT(aDefault != NULL);

    mVirtFunc = &gIdpVirtFuncString;

    mName          = (SChar*)aName;
    mAttr          = aAttr;
    mMemVal.mCount = 0;
    
    idlOS::memset(mMemVal.mVal, 0, ID_SIZEOF(mMemVal.mVal));
    
    for( sValSrc = 0; sValSrc < IDP_MAX_VALUE_SOURCE_COUNT; sValSrc++)
    {
        mSrcValArr[sValSrc].mCount = 0;
        for( sValNum = 0; sValNum < IDP_MAX_VALUE_COUNT; sValNum++)
        {
            mSrcValArr[sValSrc].mVal[sValNum] = NULL;
        }
    }

    // Store Value
    mMin         = (SChar *)"";
    mMax         = (SChar *)"";
    
    mInMinLength = aMinLength;
    mInMaxLength = aMaxLength;
    mInDefault   = aDefault;

    /*string type은 값이 없는 경우 ""을 넘긴다*/
    //default로부터 온 값을 Source Value에 넣는다. 
    mSrcValArr[IDP_VALUE_FROM_DEFAULT].mVal[0] = (void*)aDefault;
    mSrcValArr[IDP_VALUE_FROM_DEFAULT].mCount++;
}

SInt  idpString::compare(void *aVal1, void *aVal2)
{
    return idlOS::strcmp((SChar *)aVal1, (SChar *)aVal2);
}

IDE_RC idpString::validateLength(void *aVal)
{
    UInt sLength;

    sLength = (UInt)idlOS::strlen((SChar *)aVal);

    // BUG-27276 [ID] String 타입의 프라퍼티에 길이 제한을 둘 수 있으면 좋겠습니다
    // String의 경우에는 Value 대신 Length를 검사합니다.
    IDE_TEST_RAISE((sLength < mInMinLength) || (sLength > mInMaxLength),
                   ERR_LENGTH);

    return IDE_SUCCESS;

    // 여기서는 에러 버퍼 및 에러코드를 모두 설정한다.
    // 왜냐하면, 이 함수는 insert() 뿐 아니라, update()에서도 호출되기 때문이다.
    IDE_EXCEPTION(ERR_LENGTH);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp checkRange() Error : Property [%s] %s"
                        " Overflowed the Value Length."
                        "(%"ID_UINT32_FMT"~%"ID_UINT32_FMT")",
                        getName(),
                        (SChar *)aVal,
                        mInMinLength,
                        mInMaxLength);
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_RangeOverflow, getName()));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt  idpString::convertToString(void  *aSrcMem,
                                 void  *aDestMem,
                                 UInt   aDestSize) /* for conversion to string*/
{
    UInt sSize = 0;

    if (aSrcMem != NULL)
    {
        sSize = idlOS::strlen((SChar *)aSrcMem);

        if (sSize > aDestSize)
        {
            sSize = aDestSize;
        }

        idlOS::memcpy(aDestMem, aSrcMem, sSize);
    }
    return sSize;
}

IDE_RC idpString::convertFromString(void *aString, void **aResult) // When Startup
{
    void  *sValue;

    //if alphanumeric 확인 
    if((mAttr & IDP_ATTR_SK_MASK) == IDP_ATTR_SK_ALNUM)
    {
        IDE_TEST_RAISE(isAlphanumericString((SChar*)aString) != ID_TRUE, 
                       err_data_validation);
    }

    //if Ascii 확인
    if((mAttr & IDP_ATTR_SK_MASK) == IDP_ATTR_SK_ASCII)
    {
        IDE_TEST_RAISE(isASCIIString((SChar*)aString) != ID_TRUE, err_data_validation);
    }
    
    sValue = iduMemMgr::mallocRaw(idlOS::strlen((SChar *)aString) + 1, IDU_MEM_FORCE);
    IDE_ASSERT(sValue != NULL);
    
    idlOS::strcpy((SChar *)sValue, (SChar *)aString);

    *aResult = sValue;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_data_validation);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp convertFromString() Error : "
                        "The property [%s] value [%s] is not acceptable.",
                        getName(),
                        aString);
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_Value_Accept_Error, getName() ,aString));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

/**************************************************************************
 * Description :
 *    aSrc 값을 복제하여 aDst로 반환하며, aSrc에 ?가 있는 경우 ALTIBASE_HOME/conf로
 *    변환하여 복제한다.
 * aSrc      - [IN] 복제를 위한 Source 스트링
 * aDst      - [OUT] 복제된 후 반환되는 스트링 값의 포인터
 **************************************************************************/
void idpString::cloneNExpandValues(SChar* aSrc, SChar** aDst)
{
    SChar *sSrc = aSrc;
    SChar *sDest;
    SChar *sHomeDir;
    void  *sValue;
    UInt   sQuestion = 0;
    UInt   sMemSize;
    UInt   sHomeLen;

    for (; *sSrc != 0; sSrc++)
    {
        if (*sSrc == '?') sQuestion++;
    }

    if ((sQuestion > 0) && ((mAttr & IDP_ATTR_SK_MASK) == IDP_ATTR_SK_PATH))
    {
        sSrc  = (SChar *)aSrc;

        sHomeDir = idp::getHomeDir();
        sHomeLen = idlOS::strlen(sHomeDir);

        // String 내부에 ?가 있을 경우 expansion 수행.
        // 메모리 크기는 expantion 갯수 + 원래의 스트링 임.

        sMemSize = idlOS::strlen((SChar *)sSrc) +
            ( (idlOS::strlen(sHomeDir) + 1) * sQuestion) + 1;
        sValue = iduMemMgr::mallocRaw(sMemSize, IDU_MEM_FORCE);
        IDE_ASSERT(sValue != NULL);
        idlOS::memset(sValue, 0, sMemSize);

        sDest = (SChar *)sValue;

        for (; *sSrc != 0; sSrc++)
        {
            if (*sSrc == '?')
            {
                idlOS::strncpy(sDest, sHomeDir, sHomeLen);
                sDest += sHomeLen;
            }
            else
            {
                *sDest++ = *sSrc;
            }
        }
        *aDst = (SChar*)sValue;
    }
    else
    {
        sValue = iduMemMgr::mallocRaw(idlOS::strlen((SChar *)aSrc) + 1, IDU_MEM_FORCE);
        IDE_ASSERT(sValue != NULL);
        idlOS::strcpy((SChar *)sValue, (SChar *)aSrc);
        *aDst = (SChar*)sValue;
    }
}

/**************************************************************************
 * Description :
 *    aObj 객체와 동일한 타입의 객체를 생성하여 idpBase*로 반환한다. 
 *    복제시, aSID로 들어온 값을 SID로 설정하며, "*"로 설정된 값만
 *    복제하고 그 외의 값은 null 값으로 초기화 된다.  
 * aObj      - [IN] 복제를 위한 Source 객체
 * aSID      - [IN] 복제된 객체에 부여할 새로운 SID
 * aCloneObj - [OUT] 복제된 후 반환되는 객체
 **************************************************************************/
IDE_RC idpString::clone(idpString* aObj, SChar* aSID, void** aCloneObj)
{
    idpString     *sCloneObj = NULL;
    UInt           sValNum;
    idpValueSource sSrc;
    
    *aCloneObj = NULL;
    
    sCloneObj = (idpString*)iduMemMgr::mallocRaw(ID_SIZEOF(idpString));
    
    IDE_TEST_RAISE(sCloneObj == NULL, memory_alloc_error);
    
    new (sCloneObj) idpString(aObj->mName, 
                              aObj->mAttr,
                              aObj->mInMinLength,
                              aObj->mInMaxLength,
                              aObj->mInDefault);
    
    sCloneObj->setSID(aSID);
    
    /*"*"로 설정된 내용만 복사한다.*/
    sSrc = IDP_VALUE_FROM_SPFILE_BY_ASTERISK;
    for(sValNum = 0; sValNum < aObj->mSrcValArr[sSrc].mCount; sValNum++)
    {
        IDE_TEST(sCloneObj->insertRawBySrc(aObj->mSrcValArr[sSrc].mVal[sValNum], 
                                           IDP_VALUE_FROM_SPFILE_BY_ASTERISK) != IDE_SUCCESS);
    }
    
    *aCloneObj = (idpBase*)sCloneObj;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(memory_alloc_error)
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE, 
                        "idpString clone() Error : memory allocation error\n");
    }
    IDE_EXCEPTION_END;
    
    if(sCloneObj != NULL)
    {
        iduMemMgr::freeRaw(sCloneObj);
        sCloneObj = NULL;
    }
    *aCloneObj = NULL;
    
    return IDE_FAILURE;    
}

void idpString::cloneValue(void* aSrc, void** aDst)
{
    cloneNExpandValues((SChar*)aSrc, (SChar**)aDst);
    
}

/**************************************************************************
 * Description :
 *    aStr로 들어온 문자열이 ASCII로 구성되어있는지 검사한다.
**************************************************************************/
idBool idpString::isASCIIString(SChar *aStr)
{
    UInt sLen, i;
    
    sLen = idlOS::strlen(aStr);
    
    for( i = 0; i < sLen; i++ )
    {
        /*7bit Ascii 0x7F(01111111)의 not인 10000000과 and를 했을 때, 
         *0이 나와야 Ascii 값이므로, 하나라도 0이 아니면 Ascii string으로 볼 수 없다.*/
        if(((aStr[i]) & (~0x7F)) == 0)
        {
        }
        else
        {
            return ID_FALSE;
        }
    }
    
    return ID_TRUE;
}

/**************************************************************************
 * Description :
 *    aStr로 들어온 문자열이 Alphanumeric문자들로 구성되어있는지 검사한다.
**************************************************************************/
idBool idpString::isAlphanumericString(SChar *aStr)
{
    UInt sLen, i;
    
    sLen = idlOS::strlen(aStr);
    
    for( i = 0; i < sLen; i++ )
    {
        if(((aStr[i] >= 'a') && (aStr[i] <= 'z')) ||
           ((aStr[i] >= 'A') && (aStr[i] <= 'Z')) ||
           ((aStr[i] >= '0') && (aStr[i] <= '9')))
        {
        }
        else
        {
            return ID_FALSE;
        }
    }
    
    return ID_TRUE;
}
