/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduString.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_IDU_STRING_H_
#define _O_IDU_STRING_H_ 1

#include <idl.h>
#include <ide.h>


typedef UShort iduString;

typedef struct iduStringHeader
{
    UShort mSize;
    UShort mLen;
} iduStringHeader;

typedef struct iduStringObj
{
    iduStringHeader mHeader;
    SChar           mBuffer[1];
} iduStringObj;

/*
 *  Alloc in Stack
 */
#define IDU_STRING(aVar, aSize)                                                 \
    iduString aVar[(aSize + 1) / ID_SIZEOF(iduString) + 2] = {aSize, 0, 0};

/*
 *  Alloc & Free in Heap
 *
 *  TODO: idlOS::malloc이 아니라 iduMeMgr을 사용하도록 개선되어야 함.
 */
IDL_EXTERN_C IDE_RC iduStringAlloc(iduString **aObj, SInt aSize);
IDL_EXTERN_C IDE_RC iduStringFree(iduString *aObj);

/*
 *  Internal Member Accessor
 *
 *  The memory pointer returned by iduStringGetBuffer() should be used as read-only.
 *  If you want to modify the internal buffer, add new interfaces and use them.
 *
 *  iduStringGetBuffer()로 리턴된 버퍼는 반드시 read-only로 사용할 것.
 *  대신, 필요시 새로운 인터페이스를 구현하여 사용해야 함.
 */
IDL_EXTERN_C SChar *iduStringGetBuffer(iduString *aObj);
IDL_EXTERN_C UInt   iduStringGetSize(iduString *aObj);
IDL_EXTERN_C UInt   iduStringGetLength(iduString *aObj);

/*
 *  Clear Internal Buffer
 *
 *  It does not zero-fill all buffer, just make empty string.
 */
IDL_EXTERN_C IDE_RC iduStringClear(iduString *aObj);

/*
 *  Copy
 *
 *  from iduString, null-terminated string, memory buffer, or format.
 */
IDL_EXTERN_C IDE_RC iduStringCopy(iduString *aObj, iduString *aString);
IDL_EXTERN_C IDE_RC iduStringCopyString(iduString *aObj, const SChar *aString);
IDL_EXTERN_C IDE_RC iduStringCopyBuffer(iduString *aObj, const void *aBuffer, UInt aLen);
IDL_EXTERN_C IDE_RC iduStringCopyFormat(iduString *aObj, const SChar *aFormat, ...);

/*
 *  Concatenate
 *
 *  Append a copy of iduString, null-terminated string, memory buffer, or format.
 */
IDL_EXTERN_C IDE_RC iduStringAppend(iduString *aObj, iduString *aString);
IDL_EXTERN_C IDE_RC iduStringAppendString(iduString *aObj, const SChar *aString);
IDL_EXTERN_C IDE_RC iduStringAppendBuffer(iduString *aObj, const void *aBuffer, UInt aLen);
IDL_EXTERN_C IDE_RC iduStringAppendFormat(iduString *aObj, const SChar *aFormat, ...);


#endif
