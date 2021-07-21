/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpULong.h 36965 2009-11-26 02:32:17Z raysiasd $
 * Description:
 * 설명은 idpBase.cpp 참조.
 * 
 **********************************************************************/

#ifndef _O_IDP_ULONG_H_
# define _O_IDP_ULONG_H_ 1

# include <idl.h>
# include <iduMutex.h>
# include <idpBase.h>


class idpULong : public idpBase
{
    ULong mInMax;
    ULong mInMin;
    ULong mInDefault;
    
public:
    idpULong(const SChar *aName,
             idpAttr      aAttr,
             ULong        aMin,
             ULong        aMax,
             ULong        aDefault);
    
    SInt   compare(void *aVal1, void *aVal2);
    IDE_RC validateRange(void *aVal);
    IDE_RC convertFromString(void *aString, void **aResult);
    UInt   convertToString(void  *aSrcMem,
                           void  *aDestMem,
                           UInt   aDestSize); /* for conversion to string*/
    static void normalize(ULong aULong, UChar *aNormalized);
    static void normalize7Bytes(ULong aULong, UChar *aNormalized7Bytes);
    IDE_RC clone(idpULong* aObj, SChar* aSID, void** aCloneObj);
    void cloneValue(void* aSrc, void** aDst);
};

#endif /* _O_IDP_H_ */
