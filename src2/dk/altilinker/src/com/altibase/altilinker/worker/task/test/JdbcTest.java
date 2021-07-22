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
 
package com.altibase.altilinker.worker.task.test;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.LinkedList;

import com.altibase.altilinker.adlp.type.Target;
import com.altibase.altilinker.jdbc.JdbcDriverManager;

import junit.framework.TestCase;

public class JdbcTest extends TestCase
{
    protected void setUp() throws Exception
    {
        super.setUp();
    }

    protected void tearDown() throws Exception
    {
        super.tearDown();
    }

    public static void test()
    {
        Target sAltibase6Target = new Target();
        sAltibase6Target.mDriver.mDriverPath = "/home/flychk/work/JDBCDrivers/Altibase6/Altibase.jar";
        sAltibase6Target.mName          = "flychk1";
        sAltibase6Target.mConnectionUrl = "jdbc:Altibase://dbdm.altibase.in:21141/mydb";
        sAltibase6Target.mUser          = "SYS";
        sAltibase6Target.mPassword      = "MANAGER";

        Target sOracle10Target = new Target();
        sOracle10Target.mDriver.mDriverPath = "/home/flychk/work/JDBCDrivers/ojdbc14.jar";
        sOracle10Target.mName          = "flychk2";
        sOracle10Target.mConnectionUrl = "jdbc:oracle:thin:@dbdm.altibase.in:1521:ORCL";
        sOracle10Target.mUser          = "new_dblink";
        sOracle10Target.mPassword      = "new_dblink";
        
        LinkedList sTargetList = new LinkedList();
        sTargetList.add(sAltibase6Target);
        sTargetList.add(sOracle10Target);
        
        JdbcDriverManager.loadDrivers(sTargetList);
        
        try
        {
            Connection sConnection = null;

            int i;
            for (i = 0; i < 5; ++i)
            {
                try
                {
                    sConnection = JdbcDriverManager.getConnection(sOracle10Target);
                    break;
                }
                catch (SQLException e)
                {
                    fail(e.toString());
                }
                
                Thread.sleep(1000);
            }
            
            Statement sStatement = sConnection.createStatement();

            try
            {
                sStatement.execute("drop table t1");
            }
            catch (SQLException e)
            {
                fail(e.toString());
            }
            
            sStatement.execute("create table t1 (c1 integer)");

            PreparedStatement sPreparedStatement = sConnection.prepareStatement("insert into t1 values (?)");

            sPreparedStatement.setString(1, "test");
            
            sPreparedStatement.execute();
        }
        catch (SQLException e)
        {
            fail(e.toString());
        }
        catch (Exception e)
        {
            fail(e.toString());
        }
    }
}
