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
#include <smi.h>
#include <mmm.h>
#include <sti.h>


static IDE_RC mmmPhaseActionInitSM(mmmPhase        aPhase,
                                   UInt            aOptionflag,
                                   mmmPhaseAction * /*aAction*/)
{
    smiStartupPhase sStartupPhase = SMI_STARTUP_MAX;

    switch(aPhase)
    {
        case MMM_STARTUP_PRE_PROCESS:
            sStartupPhase = SMI_STARTUP_PRE_PROCESS;
            break;
        case MMM_STARTUP_PROCESS:
            sStartupPhase = SMI_STARTUP_PROCESS;
            break;
        case MMM_STARTUP_CONTROL:
            sStartupPhase = SMI_STARTUP_CONTROL;
            break;
        case MMM_STARTUP_META:
            sStartupPhase = SMI_STARTUP_META;
              break;
        case MMM_STARTUP_DOWNGRADE: /* PROJ-2689 */
        case MMM_STARTUP_SERVICE:
            sStartupPhase = SMI_STARTUP_SERVICE;
            break;
        case MMM_STARTUP_SHUTDOWN:
        default:
            IDE_CALLBACK_FATAL("invalid startup phase");
            break;
    }

    IDE_TEST(smiStartup(sStartupPhase,aOptionflag, &mmm::mSmiGlobalCallBackList) != IDE_SUCCESS);

    /* PROJ-1488 Altibase Spatio-Temporal DBMS */
    /* R-Tree 와 3DR-Tree Index 추가 */
    if ( sStartupPhase == SMI_STARTUP_PRE_PROCESS )
    {
        IDE_TEST( sti::addExtSM_Index() != IDE_SUCCESS );
    }
    else
    {
        // 최초 한번만 등록한다.
    }

    /* BUG-25279 Btree for spatial과 Disk Btree의 자료구조 및 로깅 분리 
     *  본 프로젝트를 통해 Disk Index의 저장구조가 변경되기 때문에
     * Btree for spatial, 즉 Disk Rtree Index는 Disk Btree index와
     * 저장 구조 및 로깅이 독립되어야 한다.
     *
     *  이에 따라 Disk Rtree를 위한 Recovery 함수들이 st 모듈로 이동
     * 되며 sm에서는 st모듈의 Recovery 함수를 사용하기 위해 외부모듈
     * 로 연결한다.
     *
     *  또한 외부모듈로 Recovery함수를 연결하는 일을 Control단계에서
     * 수행한다. 왜냐하면 Control 초입부에서 Redo - Undo Map을 초기화
     * 하기 때문이다.          */
    if ( sStartupPhase == SMI_STARTUP_CONTROL )
    {
        IDE_TEST( sti::addExtSM_Recovery() != IDE_SUCCESS );
    }
    else
    {
        // 최초 한번만 등록한다.
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitSM =
{
    (SChar *)"Initialize Storage Manager",
    0,
    mmmPhaseActionInitSM
};

