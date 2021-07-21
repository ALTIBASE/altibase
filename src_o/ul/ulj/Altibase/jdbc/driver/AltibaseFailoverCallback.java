/*
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

package Altibase.jdbc.driver;

import java.sql.Connection;

import Altibase.jdbc.driver.ex.ErrorDef;

public interface AltibaseFailoverCallback
{
    public final static class Event
    {
        public static final int BEGIN       = 0;
        public static final int COMPLETED   = 1;
        public static final int ABORT       = 2;
    }

    public final static class Result
    {
        public static final int GO   = 3;
        public static final int QUIT = 4;
    }

    public final static class FailoverValidation
    {
        public static final int FAILOVER_SUCCESS = ErrorDef.FAILOVER_SUCCESS;
    }
    
    int failoverCallback(Connection aConnection,
                         Object     aAppContext,
                         int        aFailoverEvent);
};
