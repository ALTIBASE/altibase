/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpUInt.h 36965 2009-11-26 02:32:17Z raysiasd $
 * Description:
 * 설명은 idpBase.cpp 참조.
 * 
 **********************************************************************/

#ifndef _O_IDP_UINT_H_
# define _O_IDP_UINT_H_ 1

# include <idl.h>
# include <iduMutex.h>
# include <idpBase.h>


class idpUInt : public idpBase
{
    UInt mInMax;
    UInt mInMin;
    UInt mInDefault;
    
public:
    idpUInt(const SChar *aName,
            idpAttr     aAttr,
            UInt        aMin,
            UInt        aMax,
            UInt        aDefault);

    SInt   compare(void *aVal1, void *aVal2);
    IDE_RC validateRange(void *aVal);
    IDE_RC convertFromString(void *aString, void **aResult);
    UInt   convertToString(void  *aSrcMem,
                           void  *aDestMem,
                           UInt   aDestSize); /* for conversion to string*/
    static void normalize(UInt aUInt, UChar *aNormalized);
    static void normalize3Bytes(UInt aUInt, UChar *aNormalized3Bytes);
    IDE_RC clone(idpUInt* aObj, SChar* aSID, void** aCloneObj);
    void cloneValue(void* aSrc, void** aDst);
};





#endif /* _O_IDP_H_ */
