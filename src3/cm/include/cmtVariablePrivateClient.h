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

#ifndef _O_CMT_VARIABLE_PRIVATE_CLIENT_H_
#define _O_CMT_VARIABLE_PRIVATE_CLIENT_H_ 1


ACI_RC cmtVariableInitializeStatic();
ACI_RC cmtVariableFinalizeStatic();

ACI_RC cmtVariableAddPiece(cmtVariable *aVariable, acp_uint32_t aOffset, acp_uint32_t aSize, acp_uint8_t *aData);

ACI_RC cmtVariableCopy(cmtVariable *aVariable, acp_uint8_t *aBuffer, acp_uint32_t aOffset, acp_uint32_t aSize);


#endif
