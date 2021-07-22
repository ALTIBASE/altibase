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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnBindConvSize.h>

acp_sint32_t ulnBindConvSize_NONE(ulnDbc       *aDbc,
                                  acp_sint32_t  aUserOctetLength)
{
    ACP_UNUSED(aDbc);

    return aUserOctetLength;
}


acp_sint32_t ulnBindConvSize_DB_CHARACTER(ulnDbc       *aDbc,
                                          acp_sint32_t  aUserOctetLength)
{
    acp_sint32_t sConvSize = 0;

    if (aDbc->mCharsetLangModule != NULL)
    {
        sConvSize = (aUserOctetLength * aDbc->mCharsetLangModule->maxPrecision(1)) /
                    ((aDbc->mClientCharsetLangModule)->maxPrecision(1));   //BUG-22684
    }
    else
    {
        sConvSize = aUserOctetLength;
    }

    return sConvSize;
}


acp_sint32_t ulnBindConvSize_NATIONAL_CHARACTER(ulnDbc       *aDbc,
                                                acp_sint32_t  aUserOctetLength)
{
    return (aUserOctetLength * aDbc->mNcharCharsetLangModule->maxPrecision(1)) /
           (aDbc->mWcharCharsetLangModule->maxPrecision(1)) ;
}
