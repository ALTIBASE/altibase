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
#if !defined(_O_IDR_REC_THREAD_H_)
#define _O_IDR_REC_THREAD_H_ 1

#include <idTypes.h>
#include <idtBaseThread.h>

class iduShmTxInfo;

class idrRecThread : public idtBaseThread
{
public:
    iduShmTxInfo  * mDeadTxInfo;

public:
    idrRecThread();
    virtual ~idrRecThread();

    IDE_RC initialize( iduShmTxInfo  * aDeadTxInfo );
    IDE_RC destroy();

    IDE_RC startThread();
    IDE_RC shutdown();

    virtual void run();

protected:
    void recoverLatchStack();
};

#endif /* _O_IDR_REC_THREAD_H_ */
