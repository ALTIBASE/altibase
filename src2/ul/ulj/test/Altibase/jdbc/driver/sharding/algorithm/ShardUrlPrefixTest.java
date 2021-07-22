package Altibase.jdbc.driver.sharding.algorithm;

import Altibase.jdbc.driver.AltibaseUrlParser;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltibaseProperties;
import junit.framework.TestCase;

import java.sql.SQLException;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

public class ShardUrlPrefixTest extends TestCase
{
    AltibaseProperties mProp = new AltibaseProperties();

    public void testIncludesShardPrefix() throws SQLException
    {
        assertIncludeShardPrefix("jdbc:sharding:Altibase://127.0.0.1:20300/mydb", true);
        assertIncludeShardPrefix("jdbc:sharding:Altibase://127.0.0.1:20300/mydb?username=aa", true);
        assertIncludeShardPrefix("jdbc:sharding:Altibase://dsn", true);
        assertIncludeShardPrefix("jdbc:sharding:Altibase://127.0.0.1:20300/mydb?alternateservers=(127.0.0.1:20300)&stf=true"
                , true);
    }

    public void testNotIncludeShardPrefix() throws SQLException
    {
        assertIncludeShardPrefix("jdbc:Altibase://127.0.0.1:20300/mydb", false);
        assertIncludeShardPrefix("jdbc:Altibase://127.0.0.1:20300/mydb?aaa=111", false);
        assertIncludeShardPrefix("jdbc:Altibase://dsn", false);
    }

    public void testInvalidConnectionUrlSQLException()
    {
        assertInvalidConnectionUrl("jdbc:Altibase://127.0.0.1:20300");
        assertInvalidConnectionUrl("jdbc:sharding:Altibase://127.0.0.1:20300");
        assertInvalidConnectionUrl("jdbc:Altibase://127.0.0.1:20300/mydb:sharding");
        assertInvalidConnectionUrl("jdbc::sharding:Altibase://127.0.0.1:20300/mydb");
        assertInvalidConnectionUrl("jdbc::shard:Altibase://127.0.0.1:20300/mydb");
        assertInvalidConnectionUrl("jdbc::SHARDING:Altibase://127.0.0.1:20300/mydb");
        assertInvalidConnectionUrl("jdbc:SHARDING:Altibase://127.0.0.1:20300/mydb");
        assertInvalidConnectionUrl("jdbc:Altibase:sharding://127.0.0.1:20300/mydb");
    }

    private void assertInvalidConnectionUrl(String aUrl)
    {
        String sUrl;
        sUrl = aUrl;
        try
        {
            AltibaseUrlParser.parseURL(sUrl, mProp);
            fail("cannot be parsed");
        }
        catch (SQLException sEx)
        {
            assertThat(sEx.getSQLState(), is(ErrorDef.getErrorState(ErrorDef.INVALID_CONNECTION_URL)));
        }
    }

    private void assertIncludeShardPrefix(String aUrl, boolean aResult) throws SQLException
    {
        assertThat(AltibaseUrlParser.parseURL(aUrl, mProp), is(aResult));
    }
}
