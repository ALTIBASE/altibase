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

#include <mmuFixedTable.h>


UInt mmuFixedTable::buildConvertTimeMSEC(idvTime *aBegin, idvTime *aEnd, UChar *aBuf)
{
    buildConvertTime(aBegin, aEnd, aBuf);
    *((ULong *)aBuf) = *((ULong *)aBuf)/1000;

    return ID_SIZEOF(ULong);
}

UInt mmuFixedTable::buildConvertTime(idvTime *aBegin, idvTime *aEnd, UChar *aBuf)
{
    /* BUG-45553 */
    idvTime sBegin = *aBegin;
    idvTime sEnd   = *aEnd;

    if (IDV_TIME_IS_INIT(&sBegin) == ID_TRUE)
    {
        *((ULong *)aBuf) = 0;
    }
    else
    {
        *((ULong *)aBuf) = IDV_TIME_DIFF_MICRO_SAFE(&sBegin, &sEnd);
    }

    return ID_SIZEOF(ULong);
}
