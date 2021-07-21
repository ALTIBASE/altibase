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

#include <cmAll.h>


#define CHECK(expr)                                             \
    if (!(expr))                                                \
    {                                                           \
        idlOS::fprintf(stderr,                                  \
                       "SOCK CHECK FAILED at %s:%d `%s'\n",     \
                       __FILE__,                                \
                       __LINE__,                                \
                       #expr);                                  \
        exit(1);                                                \
    }


int main()
{
#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)

#if defined(MSG_PEEK)
    CHECK(MSG_PEEK != 0);
#endif

#if defined(MSG_DONTWAIT)
    CHECK(MSG_DONTWAIT != 0);
#endif

#if defined(MSG_NONBLOCK)
    CHECK(MSG_NONBLOCK != 0);
#endif

#endif

    idlOS::fprintf(stderr, "SOCK CHECK SUCCEEDED.\n");

    return 0;
}
