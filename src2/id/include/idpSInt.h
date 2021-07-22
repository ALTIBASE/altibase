/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idpSInt.h 36965 2009-11-26 02:32:17Z raysiasd $
 * Description:
 * 설명은 idpBase.cpp 참조.
 * 
 **********************************************************************/

#ifndef _O_IDP_SINT_H_
# define _O_IDP_SINT_H_ 1

# include <idl.h>
# include <iduMutex.h>
# include <idpBase.h>


class idpSInt : public idpBase
{
    SInt mInMax;
    SInt mInMin;
    SInt mInDefault;
    
public:
    idpSInt(const SChar *aName,
            idpAttr     aAttr,
            SInt        aMin,
            SInt        aMax,
            SInt        aDefault);

    SInt   compare(void *aVal1, void *aVal2);
    IDE_RC validateRange(void *aVal);
    IDE_RC convertFromString(void *aString, void **aResult);
    UInt   convertToString(void  *aSrcMem,
                           void  *aDestMem,
                           UInt   aDestSize) ; /* for conversion to string*/
    
    IDE_RC clone(idpSInt* aObj, SChar* aSID, void** aCloneObj);
    void cloneValue(void* aSrc, void** aDst);
};




#endif /* _O_IDP_H_ */
