/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduString.cpp 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <iduString.h>


IDE_RC iduStringAlloc(iduString **aObj, SInt aSize)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);
    IDE_ASSERT(aSize > 0);

    sObj = (iduStringObj *)idlOS::malloc(aSize + ID_SIZEOF(iduStringHeader));

    sObj->mHeader.mSize = aSize;
    sObj->mHeader.mLen  = 0;
    sObj->mBuffer[0]     = 0;

    *aObj = (iduString *)sObj;

    return IDE_SUCCESS;
}

IDE_RC iduStringFree(iduString *aObj)
{
    IDE_ASSERT(aObj != NULL);

    idlOS::free(aObj);

    return IDE_SUCCESS;
}

SChar *iduStringGetBuffer(iduString *aObj)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);

    sObj = (iduStringObj *)aObj;

    return sObj->mBuffer;
}

UInt   iduStringGetSize(iduString *aObj)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);

    sObj = (iduStringObj *)aObj;

    return sObj->mHeader.mSize;
}

UInt   iduStringGetLength(iduString *aObj)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);

    sObj = (iduStringObj *)aObj;

    return sObj->mHeader.mLen;
}

IDE_RC iduStringClear(iduString *aObj)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);

    sObj = (iduStringObj *)aObj;

    sObj->mHeader.mLen = 0;
    sObj->mBuffer[0]   = 0;

    return IDE_SUCCESS;
}

IDE_RC iduStringCopy(iduString *aObj, iduString *aString)
{
    iduStringObj *sObj;
    iduStringObj *sString;

    IDE_ASSERT(aObj != NULL);
    IDE_ASSERT(aString != NULL);

    sObj    = (iduStringObj *)aObj;
    sString = (iduStringObj *)aString;

    UInt sLen = IDL_MIN(sObj->mHeader.mSize - 1, sString->mHeader.mLen);

    idlOS::memcpy(sObj->mBuffer, sString->mBuffer, sLen);

    sObj->mHeader.mLen  = sLen;
    sObj->mBuffer[sLen] = 0;

    return IDE_SUCCESS;
}

IDE_RC iduStringCopyString(iduString *aObj, const SChar *aString)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);
    IDE_ASSERT(aString != NULL);

    sObj = (iduStringObj *)aObj;

    UInt sLen;

    sLen = idlOS::strlen(aString);
    sLen = IDL_MIN(sObj->mHeader.mSize - 1, sLen);

    idlOS::memcpy(sObj->mBuffer, aString, sLen);

    sObj->mHeader.mLen  = sLen;
    sObj->mBuffer[sLen] = 0;

    return IDE_SUCCESS;
}

IDE_RC iduStringCopyBuffer(iduString *aObj, const void *aBuffer, UInt aLen)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);
    IDE_ASSERT(aBuffer != NULL);
    IDE_ASSERT(aLen > 0);

    sObj = (iduStringObj *)aObj;

    UInt sLen = IDL_MIN(sObj->mHeader.mSize - 1, aLen);

    idlOS::memcpy(sObj->mBuffer, aBuffer, sLen);

    sObj->mHeader.mLen  = sLen;
    sObj->mBuffer[sLen] = 0;

    return IDE_SUCCESS;
}

IDE_RC iduStringCopyFormat(iduString *aObj, const SChar *aFormat, ...)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);

    sObj = (iduStringObj *)aObj;

    SInt    sResult;
    va_list ap;

    va_start(ap, aFormat);
    sResult = idlOS::vsnprintf(sObj->mBuffer, sObj->mHeader.mSize, aFormat, ap);
    va_end(ap);

    IDE_TEST(sResult < 0);

    sObj->mHeader.mLen = IDL_MIN(sObj->mHeader.mSize - 1, sResult);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduStringAppend(iduString *aObj, iduString *aString)
{
    iduStringObj *sObj;
    iduStringObj *sString;

    IDE_ASSERT(aObj != NULL);
    IDE_ASSERT(aString != NULL);

    sObj    = (iduStringObj *)aObj;
    sString = (iduStringObj *)aString;

    UInt sLen = IDL_MIN(sObj->mHeader.mSize - sObj->mHeader.mLen - 1, sString->mHeader.mLen);

    idlOS::memcpy(sObj->mBuffer + sObj->mHeader.mLen, sString->mBuffer, sLen);

    sObj->mHeader.mLen               += sLen;
    sObj->mBuffer[sObj->mHeader.mLen] = 0;

    return IDE_SUCCESS;
}

IDE_RC iduStringAppendString(iduString *aObj, const SChar *aString)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);
    IDE_ASSERT(aString != NULL);

    sObj = (iduStringObj *)aObj;

    UInt sLen;

    sLen = idlOS::strlen(aString);
    sLen = IDL_MIN(sObj->mHeader.mSize - sObj->mHeader.mLen - 1, sLen);

    idlOS::memcpy(sObj->mBuffer + sObj->mHeader.mLen, aString, sLen);

    sObj->mHeader.mLen               += sLen;
    sObj->mBuffer[sObj->mHeader.mLen] = 0;

    return IDE_SUCCESS;
}

IDE_RC iduStringAppendBuffer(iduString *aObj, const void *aBuffer, UInt aLen)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);
    IDE_ASSERT(aBuffer != NULL);
    IDE_ASSERT(aLen > 0);

    sObj = (iduStringObj *)aObj;

    UInt sLen = IDL_MIN(sObj->mHeader.mSize - sObj->mHeader.mLen - 1, aLen);

    idlOS::memcpy(sObj->mBuffer + sObj->mHeader.mLen, aBuffer, sLen);

    sObj->mHeader.mLen               += sLen;
    sObj->mBuffer[sObj->mHeader.mLen] = 0;

    return IDE_SUCCESS;
}

IDE_RC iduStringAppendFormat(iduString *aObj, const SChar *aFormat, ...)
{
    iduStringObj *sObj;

    IDE_ASSERT(aObj != NULL);

    sObj = (iduStringObj *)aObj;

    SInt    sResult;
    va_list ap;

    va_start(ap, aFormat);
    sResult = idlOS::vsnprintf(sObj->mBuffer + sObj->mHeader.mLen, sObj->mHeader.mSize - sObj->mHeader.mLen, aFormat, ap);
    va_end(ap);

    IDE_TEST(sResult < 0);

    sObj->mHeader.mLen = IDL_MIN(sObj->mHeader.mSize - 1, sObj->mHeader.mLen + sResult);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
