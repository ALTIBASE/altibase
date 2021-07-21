/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduLimitManager.h 40953 2010-08-09 02:09:41Z orc $
 **********************************************************************/

#ifndef _O_IDU_LIMIT_MANAGER_H_
#define _O_IDU_LIMIT_MANAGER_H_ 1

#if defined(ALTIBASE_LIMIT_CHECK)

/*
 * Limit point settings
 */
#define IDU_LIMITPOINT_RAISE(aID, aLabel) \
    if(0 != iduLimitManager::limitPoint(__FILE__, (SChar *)aID)) { goto aLabel; } else {}
#define IDU_LIMITPOINT(aID) IDU_LIMITPOINT_RAISE(aID, IDE_EXCEPTION_END_LABEL)
#define IDU_LIMITPOINT_DONE() iduLimitManager::limitPointDone();

#define IDU_LIMITPOINTENV       ( (SChar *)"ATAF_TEST_CASE" )
#define IDU_LIMITPOINTFUNC      ( (SChar *)"allimSoLimitPoint" )
#define IDU_LIMITPOINTDONEFUNC  ( (SChar *)"allimSoLimitPointDone" )
#define IDU_LIMITPOINTSO        ( (SChar *)"allimso" )

typedef SInt (*limitPointFunc)( SChar *, SChar * );
typedef void (*limitPointDoneFunc)();

class iduLimitManager
{
public:
    static IDE_RC             loadFunctions();
    static SInt               limitPoint( SChar *, SChar * );
    static void               limitPointDone();

private:
    static limitPointFunc     mLimitPoint;
    static limitPointDoneFunc mLimitPointDone;
    static PDL_SHLIB_HANDLE   mLimitDl;
};

#else
#define IDU_LIMITPOINT_RAISE(aID, aLabel)
#define IDU_LIMITPOINT(aID)
#define IDU_LIMITPOINT_DONE()
#endif

#endif /* _O_IDU_LIMIT_MANAGER_H_ */
