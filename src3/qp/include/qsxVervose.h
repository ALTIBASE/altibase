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
 
#ifndef _O_QSX_VERVOSE_H_
#define _O_QSX_VERVOSE_H_ 1

#include <qcuProperty.h>

#define QSX_DBSTART_VERVOSE1(str, err_label)                             \
if (_STORED_PROC_MODE & QCU_SPM_MASK_VERVOSE1)                           \
{                                                                        \
    IDE_TEST_RAISE(IDE_CALLBACK_SEND_SYM(str) != IDE_SUCCESS, err_label);\
}

#define QSX_DBSTART_VERVOSE2(str, err_label)                             \
if (_STORED_PROC_MODE & QCU_SPM_MASK_VERVOSE1 ||                         \
    _STORED_PROC_MODE & QCU_SPM_MASK_VERVOSE2 )                          \
{                                                                        \
    IDE_TEST_RAISE(IDE_CALLBACK_SEND_SYM(str) != IDE_SUCCESS, err_label);\
}

#define QSX_DBSTART_VERVOSE3(str, err_label)                             \
if (_STORED_PROC_MODE & QCU_SPM_MASK_VERVOSE1 ||                         \
    _STORED_PROC_MODE & QCU_SPM_MASK_VERVOSE2 ||                         \
    _STORED_PROC_MODE & QCU_SPM_MASK_VERVOSE3 )                          \
{                                                                        \
    IDE_TEST_RAISE(IDE_CALLBACK_SEND_SYM(str) != IDE_SUCCESS, err_label);\
}

#endif /* _O_QSX_VERVOSE_H_ */
