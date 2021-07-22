/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpString.h 68602 2015-01-23 00:13:11Z sbjang $
 * Description:
 * 설명은 idpBase.cpp 참조.
 * 
 **********************************************************************/

#ifndef _O_IDP_STRING_H_
# define _O_IDP_STRING_H_ 1

# include <idl.h>
# include <iduMutex.h>
# include <idpBase.h>


class idpString : public idpBase
{
    UInt    mInMaxLength;
    UInt    mInMinLength;
    const SChar *mInDefault;
    
public:
    idpString(const SChar   *aName,
              idpAttr        aAttr,
              UInt           aMinLength,
              UInt           aMaxLength,
              const SChar   *aDefault);

    SInt   compare(void *aVal1, void *aVal2);
    IDE_RC validateLength(void *aVal);
    IDE_RC convertFromString(void *aString, void **aResult);
    UInt   convertToString(void  *aSrcMem,
                           void  *aDestMem,
                           UInt   aDestSize); /* for conversion to string*/
    void   cloneNExpandValues(SChar* aSrc, SChar** aDst);
    IDE_RC clone(idpString* aObj, SChar* aSID, void** aCloneObj);
    void   cloneValue(void* aSrc, void** aDst);
    idBool isASCIIString(SChar *aStr);
    idBool isAlphanumericString(SChar *aStr);
};





#endif /* _O_IDP_H_ */
