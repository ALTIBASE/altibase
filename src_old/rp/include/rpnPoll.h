/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

#ifndef _O_RPN_POLL_H_
#define _O_RPN_POLL_H_    1

#include <acp.h>

#define RPN_POLL_IN     (ACP_POLL_IN)
#define RPN_POLL_OUT    (ACP_POLL_OUT)

typedef struct rpnPoll  rpnPoll;

typedef IDE_RC rpnPollCallback( rpnPoll     * aPollSet,
                                void        * aData );

typedef struct rpnPoll 
{
    acp_poll_set_t      mPollSet;
    UInt                mCount;

    rpnPollCallback   * mCallback;
} rpnPoll;

IDE_RC rpnPollInitialize( rpnPoll   * aPoll,
                          SInt        aMaxCount );
void rpnPollFinalize(rpnPoll * aPoll );

IDE_RC rpnPollAddSocket( rpnPoll      * aPoll,
                         rpnSocket    * aSocket,
                         void         * aData,
                         SInt           aEvent );

void rpnPollRemoveSocket( rpnPoll     * aPoll,
                          rpnSocket   * aSocket );

IDE_RC rpnPollDispatch( rpnPoll         * aPoll,
                        SLong             aTimeoutMilliSec,
                        rpnPollCallback * aCallback );

UInt rpnPollGetCount( rpnPoll     * aPoll );

#endif
