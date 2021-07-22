import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class sampleNumberNeg
{
    ResultSet  res;
    Statement  stmt;
    Connection conn;
    String     sql;

    sampleNumberNeg(Connection cn) throws SQLException
    {
        conn = cn;
        stmt = conn.createStatement();
        try
        {/* delete table */
            stmt.executeUpdate("drop table foo");
        }
        catch (Exception e)
        {
        };

        sql = "create table foo (id numeric(38,16) )";
        System.out.println(stmt.executeUpdate(sql) + " | " + sql);
    }

    /* Simple direct SQL use */

    void directSQL() throws SQLException
    {
        System.out.println("\n Direct execution:");
        stmt.executeUpdate("INSERT INTO foo  VALUES ( -1  )");
        stmt.executeUpdate("INSERT INTO foo  VALUES ( -1.2)");
        stmt.executeUpdate("INSERT INTO foo  VALUES ( -0.1)");
        try
        {
            // res = stmt.executeQuery("select * from foo");
            res = stmt.executeQuery("select dump(id * numeric'1') from foo");
            while(res.next())
            {
                System.out.println(res.getString(1));
            }
            res.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public static void main(String[] args)
    {
        String url = "jdbc:Altibase://localhost/mydb", user = "SYS", passwd = "MANAGER";
        /* Load driver */
        if(args.length > 0) url = args[0];

        try
        {
            Class.forName("Altibase.jdbc.driver.AltibaseDriver");
            sampleNumberNeg smp = new sampleNumberNeg(DriverManager.getConnection(url, user, passwd));
            smp.directSQL();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

}
