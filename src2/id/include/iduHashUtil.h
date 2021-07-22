/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduHashUtil.h 68602 2015-01-23 00:13:11Z sbjang $
 ****************************************************************************/

#ifndef _O_IDU_HASH_UTIL_H_
#define _O_IDU_HASH_UTIL_H_ 1

#include <idl.h>

#define IDU_HASHUTIL_INITVALUE 0x01020304

class iduHashUtil
{
private:
    static const UChar hashPermut[256];

public:
    static UInt hashBinary(void *aPtr, UInt aLen);
    static UInt hashString( UInt aHash, const UChar* aValue, UInt aLength );
    static UInt hashUInt(UInt aKey);
    static UInt hashULong(ULong aKey);
};

#endif
