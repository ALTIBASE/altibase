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

#ifndef CDBC_BIND_H
#define CDBC_BIND_H 1

ACP_EXTERN_C_BEGIN



typedef enum
{
    CDBC_ALLOC_BUF_OFF = 0,
    CDBC_ALLOC_BUF_ON  = 1
} CDBC_ALLOC_BUF;

typedef enum
{
    CDBC_USE_LOCATOR_OFF = 0,
    CDBC_USE_LOCATOR_ON  = 1
} CDBC_USE_LOCATOR;



CDBC_INTERNAL ALTIBASE_BOOL altibase_stmt_parambind_changed (cdbcABStmt *aABStmt,
                                                             ALTIBASE_BIND *aBind);
CDBC_INTERNAL ALTIBASE_RC altibase_stmt_parambind_alloc (cdbcABStmt *aABStmt);
CDBC_INTERNAL void altibase_stmt_parambind_backup (cdbcABStmt *aABStmt,
                                                   ALTIBASE_BIND *aBind);
CDBC_INTERNAL void altibase_stmt_parambind_free (cdbcABStmt *aABStmt);
CDBC_INTERNAL ALTIBASE_RC altibase_stmt_parambind_core (cdbcABStmt *aABStmt,
                                                        ALTIBASE_BIND *aBind);
CDBC_INTERNAL ALTIBASE_RC altibase_stmt_resultbind_proc (cdbcABStmt *aABStmt,
                                                         ALTIBASE_BIND *aBind,
                                                         ALTIBASE_BOOL aUseAllocLoc);
CDBC_INTERNAL acp_sint32_t altibase_bind_max_typesize (ALTIBASE_BIND_TYPE aBindType);
CDBC_INTERNAL acp_sint32_t altibase_bind_max_sbinsize (ALTIBASE_FIELD *aFieldInfo);
CDBC_INTERNAL acp_sint32_t altibase_bind_max_bufsize (ALTIBASE_FIELD *aFieldInfo,
                                                      ALTIBASE_BIND *aBaseBindInfo);
CDBC_INTERNAL ALTIBASE_RC altibase_result_bind_init (cdbcABRes *aABRes,
                                                     ALTIBASE_BIND *aBaseBindInfo,
                                                     CDBC_ALLOC_BUF aBufAlloc);
CDBC_INTERNAL void altibase_result_bind_free (cdbcABRes *aABRes);
CDBC_INTERNAL ALTIBASE_RC altibase_result_bind_proc (cdbcABRes *aABRes,
                                                     CDBC_USE_LOCATOR aUseLocator);
CDBC_INTERNAL ALTIBASE_RC altibase_result_rebind_for_lob (cdbcABRes *aABRes);



ACP_EXTERN_C_END

#endif /* CDBC_BIND_H */

