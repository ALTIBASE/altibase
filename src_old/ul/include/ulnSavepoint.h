
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

#ifndef _ULN_SAVEPOINTE_H_
#define _ULN_SAVEPOINTE_H_ 1

#include <uln.h>
#include <ulnPrivate.h>

/* BUG-46785 Shard statement partial rollback */
ACI_RC ulnCallbackSetSavepointResult( cmiProtocolContext * aProtocolContext,
                                      cmiProtocol        * aProtocol,
                                      void               * aServiceSession,
                                      void               * aUserContext );

/* BUG-46785 Shard statement partial rollback */
ACI_RC ulnCallbackRollbackToSavepointResult( cmiProtocolContext * aProtocolContext,
                                             cmiProtocol        * aProtocol,
                                             void               * aServiceSession,
                                             void               * aUserContext );

#endif /* _ULN_SAVEPOINTE_H_ */ 
