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

package Altibase.jdbc.driver.util;

import java.sql.DriverPropertyInfo;

public class AltibaseDriverPropertyInfo extends DriverPropertyInfo
{
    public AltibaseDriverPropertyInfo(String aName, String aValue)
    {
        super(aName, aValue);
    }

    public AltibaseDriverPropertyInfo(String aName, String aValue, String[] aChoices, boolean aRequired, String aDescription)
    {
        super(aName, aValue);

        this.required = aRequired;
        this.choices = aChoices;
        this.description = aDescription;
    }

    public AltibaseDriverPropertyInfo(String aName, int aValue, String[] aChoices, boolean aRequired, String aDescription)
    {
        this(aName, String.valueOf(aValue), aChoices, aRequired, aDescription);
    }

    public AltibaseDriverPropertyInfo(String aName, long aValue, String[] aChoices, boolean aRequired, String aDescription)
    {
        this(aName, String.valueOf(aValue), aChoices, aRequired, aDescription);
    }

    private static final String[] BOOLEAN_CHOICES = new String[] { "true", "false" };

    public AltibaseDriverPropertyInfo(String aName, boolean aValue, boolean aRequired, String aDescription)
    {
        this(aName, String.valueOf(aValue), BOOLEAN_CHOICES, aRequired, aDescription);
    }
}
