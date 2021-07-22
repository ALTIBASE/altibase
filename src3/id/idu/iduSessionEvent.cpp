/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduSessionEvent.cpp 64386 2014-04-09 01:34:40Z raysiasd $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <idu.h>

/*----------------------------------------------------------------------
  Name:
      iduSessionErrorFilter() -- 에러코드를 만들어 상위로 되돌림

  Argument:
      aFlag - Event Flag

  Return:
      Event가 발생했으면 IDE_FAILURE

  Description:
      이 함수는 상위에서 호출하여, 세션의 Event를 감지하는
      것으로서 Close, Timeout Event를 검출한다.
      또한, 추가될 경우 이 부분에 추가되는 코드를 넣으면 된다.
 *--------------------------------------------------------------------*/

IDE_RC iduSessionErrorFilter( ULong aFlag, UInt aCurrStmtID )
{
    IDE_RC sRet = IDE_FAILURE;

    if (aFlag & IDU_SESSION_EVENT_CLOSED)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Session_Closed));
    }
    else
    {
        if (aFlag & IDU_SESSION_EVENT_CANCELED)
        {
            IDE_SET(ideSetErrorCode(idERR_ABORT_Query_Canceled));
        }
        else
        {
            if ( IDU_SESSION_CHK_TIMEOUT( aFlag, aCurrStmtID ) )
            {
                IDE_SET(ideSetErrorCode(idERR_ABORT_Query_Timeout));
            }
            else
            {
                sRet = IDE_SUCCESS;
            }
        }
    }
    return sRet;
}
