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

package Altibase.jdbc.driver.sharding.util;

import junit.framework.TestCase;

import java.util.LinkedHashMap;
import java.util.Map;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

public class RangeTest extends TestCase
{
    private Map<String, Range> mRangeMap;

    public void setUp()
    {
        mRangeMap = new LinkedHashMap<String, Range>();
    }

    @SuppressWarnings("unchecked")
    public void testIntegerRange()
    {
        mRangeMap.put("node1", Range.between(Range.getNullRange(), 300));
        mRangeMap.put("node2", Range.between(300, 600));
        mRangeMap.put("node3", Range.between(600, 900));

        assertThat(getNodeNameFromRange(Integer.MIN_VALUE), is("node1"));
        assertThat(getNodeNameFromRange(299), is("node1"));
        assertThat(getNodeNameFromRange(300), is("node2"));
        assertThat(getNodeNameFromRange(900), is("not founded"));
    }

    @SuppressWarnings("unchecked")
    public void testStringRange()
    {
        mRangeMap.put("node1", Range.between(Range.getNullRange(), "가나다"));
        mRangeMap.put("node2", Range.between("가나다", "가나사"));
        mRangeMap.put("node3", Range.between("가나사", "가바마"));

        assertThat(getNodeNameFromRange("가나나"), is("node1"));
        assertThat(getNodeNameFromRange("가나다"), is("node2"));
        assertThat(getNodeNameFromRange("가나바"), is("node2"));
        assertThat(getNodeNameFromRange("가나"),   is("node1"));
        assertThat(getNodeNameFromRange(""),      is("node1"));
        assertThat(getNodeNameFromRange("나나다"), is("not founded"));
    }

    @SuppressWarnings("unchecked")
    private String getNodeNameFromRange(Object aShardValue)
    {
        for (Map.Entry<String, Range> sEntry : mRangeMap.entrySet())
        {
            Range sRange = sEntry.getValue();
            if (sRange.containsEndedBy(aShardValue))
            {
                return sEntry.getKey();
            }
        }

        return "not founded";
    }
}
