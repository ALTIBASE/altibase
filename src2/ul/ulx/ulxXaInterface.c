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

#include <xa.h>
#include <ulxXaFunction.h>

struct xa_switch_t altibase_xa_switch = {
    "ALTIBASE_XA_SWITCH", /*name*/
    TMNOMIGRATE/*|TMUSETHREADS*/, /* flags (allow multithread?) */
    0, /* version must be 0 */
    ulxXaOpen, /*fptr of xa_open */
    ulxXaClose,
    ulxXaStart,
    ulxXaEnd,
    ulxXaRollback,
    ulxXaPrepare,
    ulxXaCommit,
    ulxXaRecover,
    ulxXaForget,
    ulxXaComplete /* xa_complete */
//    NULL, /* xa_ready */
//    NULL, /* xa_done */
//    NULL, /* xa_wait_recovery */
//    NULL, /* xa_wait */
//    NULL, /* xa_start2 */
//    NULL  /* struct ax_switch_t **ax_switch */
};

