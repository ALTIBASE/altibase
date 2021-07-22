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
 

/***********************************************************************
 * $Id: sdiFOThread.h 14768 2006-01-03 00:57:49Z unclee $
 **********************************************************************/

# include <idtBaseThread.h>
# include <sdiZookeeper.h>

/* FOThread = FailOver Thread */
class sdiFOThread : public idtBaseThread
{
public:
    sdiFOThread();
    virtual ~sdiFOThread();

    static IDE_RC failoverExec( SChar * aMyNodeName,
                                SChar * aTargetNodeName,
                                UInt    aThreadCnt );

    /* 쓰레드 초기화 */
    IDE_RC initialize( SChar * aMyNodeName,
                       SChar * aTargetNodeName );

    IDE_RC destroy( );

    inline UInt getErrorCode() { return mErrorCode; };
    inline ideErrorMgr* getErrorMgr() { return &mErrorMgr; };
    
private:

    UInt                 mErrorCode;
    ideErrorMgr          mErrorMgr;

    SChar                mMyNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar                mTargetNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    idBool               mIsSuccess;
    
    void run();
    static IDE_RC threadRun( sdiFOThread * aThreads );
};
