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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDR_REC_PROCESS_H_)
#define _O_IDR_REC_PROCESS_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <ide.h>

class idrRecProcess : public idtBaseThread
{
private:
    iduShmProcInfo  * mDeadProcInfo;

public:
    idrRecProcess();
    virtual ~idrRecProcess();

    IDE_RC initialize( iduShmProcInfo  * aDeadProcInfo );
    IDE_RC destroy();

    IDE_RC startThread();
    IDE_RC shutdown();

    virtual void run();
};

inline IDE_RC idrRecProcess::startThread()
{
    IDE_TEST( shutdown() != IDE_SUCCESS );

    IDE_TEST( start() != IDE_SUCCESS );
    IDE_TEST( waitToStart(0) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC idrRecProcess::shutdown()
{
    if( isStarted() == ID_TRUE )
    {
        (void)this->join();
    }

    return IDE_SUCCESS;
}

#endif /* _O_IDR_REC_PROCESS_H_ */
