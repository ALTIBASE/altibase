/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef _O_IDW_SERVICE_H_
#define _O_IDW_SERVICE_H_ 1

/***************************************************************
 * PROJ-1699 : Service For Windows OS
 * Windows Service is consist of three Method.
 * 1. Service Main
 *   - regist a handler and start a Altibase server.
 * 2. Service Handler
 *   - process a control signal like a Server start, Server stop.
 * 3. Main
 *   - main method in altibase.cpp file
***************************************************************/

#include <idTypes.h>
#include <idl.h>

typedef IDE_RC (*startFunction) ( void );
typedef IDE_RC (*runFunction) ( void );
typedef IDE_RC (*stopFunction) ( void );

class idwService {

 public:
    static void                    serviceHandler( SInt opCode );
    static IDE_RC                  serviceStart( void );
    static IDE_RC                  serviceMain( SInt   argc,
                                                SChar *argv );
    static void                    setStatus( SInt dwState );
 public:
 
#if defined(VC_WIN32) && !defined(VC_WINCE)
    static SERVICE_STATUS_HANDLE   mHService;
    static SInt                    mServiceStatus;
#endif

    static startFunction           mWinStart;
    static runFunction             mWinRun;
    static stopFunction            mWinStop;
    
    static char*                   serviceName;
     
 public:
    static void setStartFunction(startFunction fp)
    {
       mWinStart = fp;
    }

    static void setRunFunction(runFunction fp)
    {
       mWinRun = fp;   
    }
        
    static void setStopFunction(stopFunction fp)
    {
       mWinStop = fp;   
    }
        
};


#endif /* _O_IDW_SERVICE_H_ */
