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
 
#include <idl.h>
#include <ideErrorMgr.h>
#include <ideMsgLog.h>
#include <iduStack.h>
#include <sample.h>

ideMsgLog mMsgLogForce;

void setMsgLog()
{
    SChar               msglogFilename[256];
    
    idlOS::memset(msglogFilename, 0, 256);
    idlOS::sprintf(msglogFilename, "sample.log");

    assert(mMsgLogForce.initialize(1, msglogFilename,
                                   1000000,
                                   0) == IDE_SUCCESS);
    ideLogSetTarget(&mMsgLogForce);
    
}

int main()
{

    setMsgLog();
    ideLog("testing");
    printf("size is %d\n", sizeof(long));

    assert(setupDefaultAltibaseSignal() == IDE_SUCCESS);
    
    ideLog("=========== before normal stack dump ============");
    extPrint();
    ideLog("=========== end normal stack dump ============");

    
    ideLog("=========== before SEGV stack dump ============");
    extCore();
    ideLog("=========== end  SEGV stack dump ============");
}
