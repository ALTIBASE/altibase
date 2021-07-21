#include <rpdStatistics.h>

void rpdStatistics::initialize( void )
{
    mEventFlag = ID_ULONG(0);

    idvManager::initSession( &mStatSession, 
                             0,     /* unuse */
                             NULL   /* unuse */ );

    idvManager::initSQL( &mStatistics,
                         &mStatSession,
                         &mEventFlag,
                         NULL,
                         NULL,
                         NULL,
                         IDV_OWNER_UNKNOWN );
}

void rpdStatistics::finalize( void )
{
    setCancelEvent();
}
