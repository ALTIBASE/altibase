/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduLimitManager.cpp 42657 2010-12-15 04:54:15Z djin $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idp.h>
#include <iduLimitManager.h>

#if defined(ALTIBASE_LIMIT_CHECK)

#define IDU_PATH_MAX_LENGTH (1024)

limitPointFunc     iduLimitManager::mLimitPoint     = NULL;
limitPointDoneFunc iduLimitManager::mLimitPointDone = NULL;
PDL_SHLIB_HANDLE   iduLimitManager::mLimitDl        = PDL_SHLIB_INVALID_HANDLE;

IDE_RC iduLimitManager::loadFunctions()
{
    SChar * sSo = NULL;
    SChar   sPath[IDU_PATH_MAX_LENGTH];
    

    sSo = idlOS::getenv( IDU_LIMITPOINTENV );

    IDE_TEST( sSo == NULL );

#if defined(VC_WIN32)
    idlOS::snprintf( sPath, 
                     IDU_PATH_MAX_LENGTH,
                     "%s%s%s%s%s%s%s",
                     sSo,
                     IDL_FILE_SEPARATORS,
                     "lib",
                     IDL_FILE_SEPARATORS,
                     "",
                     IDU_LIMITPOINTSO,
                     ".dll" );
#elif defined(HPUX)
    idlOS::snprintf( sPath, 
                     IDU_PATH_MAX_LENGTH,
                     "%s%s%s%s%s%s%s",
                     sSo,
                     IDL_FILE_SEPARATORS,
                     "lib",
                     IDL_FILE_SEPARATORS,
                     "lib",
                     IDU_LIMITPOINTSO,
                     ".sl" );
#else
    idlOS::snprintf( sPath, 
                     IDU_PATH_MAX_LENGTH,
                     "%s%s%s%s%s%s%s",
                     sSo,
                     IDL_FILE_SEPARATORS,
                     "lib",
                     IDL_FILE_SEPARATORS,
                     "lib",
                     IDU_LIMITPOINTSO,
                     ".so" );
#endif

    mLimitDl = idlOS::dlopen( sPath,
                              RTLD_NOW|RTLD_LOCAL ); 
    
    IDE_TEST( mLimitDl == PDL_SHLIB_INVALID_HANDLE );

    /* Error handling */
    mLimitPoint = (limitPointFunc)idlOS::dlsym( mLimitDl, 
                                                IDU_LIMITPOINTFUNC );

    mLimitPointDone = (limitPointDoneFunc)idlOS::dlsym( mLimitDl, 
                                                        IDU_LIMITPOINTDONEFUNC );

    IDE_TEST( mLimitPoint == NULL );

    IDE_TEST( mLimitPointDone == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
SInt iduLimitManager::limitPoint( SChar *aFile, SChar *aID )
{
    IDE_RC sRC = IDE_FAILURE;

    if( mLimitPoint == NULL )
    {
        sRC = iduLimitManager::loadFunctions();

        IDE_TEST( sRC != IDE_SUCCESS );
    }
    else
    {
        /* No need of loading */
    }
    
    if( mLimitPoint != NULL )
    {
        /* Limit point hit! */
        return (*mLimitPoint)( aFile, aID );
    }
    else
    {
        return 0;
    }

    IDE_EXCEPTION_END;

    return 0;
}

void iduLimitManager::limitPointDone()
{
    IDE_RC sRC = IDE_FAILURE;

    if( mLimitPoint == NULL )
    {
        sRC = iduLimitManager::loadFunctions();

        IDE_TEST( sRC != IDE_SUCCESS );
    }
    else
    {
        /* No need of loading */
    }
    if( mLimitPointDone != NULL )
    {
        (*mLimitPointDone)();
    }
    else
    {
        /* Do nothing */
    }

    return;

    IDE_EXCEPTION_END;

    return;
}

#endif
