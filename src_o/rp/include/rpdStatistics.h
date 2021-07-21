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

/***********************************************************************
 * $Id:
 **********************************************************************/

#ifndef _O_RPD_STATISTICS_H_
#define _O_RPD_STATISTICS_H_ 1

#include <idv.h>
#include <iduSessionEvent.h>

class rpdStatistics
{
private:
    idvSQL             mStatistics;
    ULong              mEventFlag;
    idvSession         mStatSession;

    /* member function */
public:
    void initialize( void );
    void finalize( void );

    inline void setCancelEvent( void )
    {
        IDU_SESSION_SET_CANCELED( *( mStatistics.mSessionEvent ) );
    };

    inline idvSQL * getStatistics( void )
    {
        return &mStatistics;
    }

private:

    /* static function */

};

#endif
