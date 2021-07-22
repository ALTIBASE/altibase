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

#ifndef _O_CMN_DISPATCHER_PRIVATE_H_
#define _O_CMN_DISPATCHER_PRIVATE_H_ 1


/*
 * Link Access Functions for Subclasses
 */

IDE_RC cmnDispatcherAddLink(cmnDispatcher *aDispatcher, cmnLink *aLink);
IDE_RC cmnDispatcherRemoveLink(cmnDispatcher *aDispatcher, cmnLink *aLink);
IDE_RC cmnDispatcherRemoveAllLinks(cmnDispatcher *aDispatcher);

/*
 * Alloc Support Functions Implemented by Each Subclasses
 */

IDE_RC cmnDispatcherMapSOCKSelect(cmnDispatcher *aDispatcher);
UInt   cmnDispatcherSizeSOCKSelect();

IDE_RC cmnDispatcherMapSOCKPoll(cmnDispatcher *aDispatcher);
UInt   cmnDispatcherSizeSOCKPoll();

IDE_RC cmnDispatcherMapSOCKEpoll(cmnDispatcher *aDispatcher);
UInt   cmnDispatcherSizeSOCKEpoll();

IDE_RC cmnDispatcherMapIPC(cmnDispatcher *aDispatcher);
UInt   cmnDispatcherSizeIPC();

IDE_RC cmnDispatcherMapIPCDA(cmnDispatcher *aDispatcher);
UInt   cmnDispatcherSizeIPCDA();

/* PROJ-2681 */
IDE_RC cmnDispatcherMapIB(cmnDispatcher *aDispatcher);
UInt   cmnDispatcherSizeIB();

/*
 * WaitLink Functions Implemented by Each Subclasses
 */

IDE_RC cmnDispatcherWaitLinkSOCKSelect(cmnLink *aLink,
                                       cmnDirection aDirection,
                                       PDL_Time_Value *aTimeout);

IDE_RC cmnDispatcherWaitLinkSOCKPoll(cmnLink *aLink,
                                     cmnDirection aDirection,
                                     PDL_Time_Value *aTimeout);

IDE_RC cmnDispatcherWaitLinkSOCKEpoll(cmnLink *aLink,
                                      cmnDirection aDirection,
                                      PDL_Time_Value *aTimeout);

IDE_RC cmnDispatcherWaitLinkIPC(cmnLink *aLink,
                                cmnDirection aDirection,
                                PDL_Time_Value *aTimeout);
/*PROJ-2616*/
IDE_RC cmnDispatcherWaitLinkIPCDA(cmnLink *aLink,
                                  cmnDirection aDirection,
                                  PDL_Time_Value *aTimeout);

/* PROJ-2681 */
IDE_RC cmnDispatcherWaitLinkIB(cmnLink        *aLink,
                               cmnDirection    aDirection,
                               PDL_Time_Value *aTimeout);

/*
 * CheckHandle Functions for SOCK Implemented by Each Subclasses
 */

SInt cmnDispatcherCheckHandleSOCKSelect(PDL_SOCKET      aHandle,
                                        PDL_Time_Value *aTimeout);

SInt cmnDispatcherCheckHandleSOCKPoll(PDL_SOCKET      aHandle,
                                      PDL_Time_Value *aTimeout);

SInt cmnDispatcherCheckHandleSOCKEpoll(PDL_SOCKET      aHandle,
                                       PDL_Time_Value *aTimeout);

#endif
