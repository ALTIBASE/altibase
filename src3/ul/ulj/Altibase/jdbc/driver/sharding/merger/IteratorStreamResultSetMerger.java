/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.merger;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Iterator;
import java.util.List;

public class IteratorStreamResultSetMerger extends AbstractStreamResultSetMerger
{
    private final Iterator<ResultSet> mResultSetsItr;

    public IteratorStreamResultSetMerger(List<ResultSet> aResultSets)
    {
        this.mResultSetsItr = aResultSets.iterator();
        setCurrentResultSet(this.mResultSetsItr.next());
    }

    public boolean next() throws SQLException
    {
        boolean sHasNext = getCurrentResultSet().next();
        if (sHasNext)
        {
            return true;
        }
        while (!sHasNext && mResultSetsItr.hasNext())
        {
            setCurrentResultSet(mResultSetsItr.next());
            sHasNext = getCurrentResultSet().next();
        }

        return sHasNext;
    }
}
