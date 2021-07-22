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

import java.util.Hashtable;

import javax.naming.Context;
import javax.naming.Name;
import javax.naming.Reference;
import javax.naming.spi.ObjectFactory;

import Altibase.jdbc.driver.util.AltibaseProperties;

public class AltibaseDataSourceFactory implements ObjectFactory
{
    public Object getObjectInstance(Object aReference, Name aName, Context aNameCtx, Hashtable aEnvironment) throws Exception
    {
        Reference sRef = (Reference)aReference;
        String sPropStr = (String)sRef.get("properties").getContent();
        AltibaseProperties sProp = AltibaseProperties.valueOf(sPropStr);

        AltibaseDataSource sDS = null;
        Class sTargetClass = sRef.getClass();
        if (AltibaseDataSource.class.equals(sTargetClass))
        {
            sDS = new AltibaseDataSource(sProp);
        }
        else if (AltibaseConnectionPoolDataSource.class.equals(sTargetClass))
        {
            sDS = new AltibaseConnectionPoolDataSource(sProp);
        }
        else if (AltibaseXADataSource.class.equals(sTargetClass))
        {
            sDS = new AltibaseXADataSource(sProp);
        }
        else
        {
            //
        }
        return sDS;
    }
}
