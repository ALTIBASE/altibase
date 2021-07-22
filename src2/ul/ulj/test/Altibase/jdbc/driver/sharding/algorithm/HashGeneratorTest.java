package Altibase.jdbc.driver.sharding.algorithm;

import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.ColumnFactory;
import Altibase.jdbc.driver.sharding.core.ShardKeyDataType;
import Altibase.jdbc.driver.util.AltibaseProperties;
import junit.framework.TestCase;

import java.sql.SQLException;
import java.sql.Types;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

public class HashGeneratorTest extends TestCase
{
    private HashGenerator mHashGenerator;
    private ColumnFactory mColumnFactory;

    public HashGeneratorTest()
    {
        mHashGenerator = HashGenerator.getInstance();
        mColumnFactory = new ColumnFactory();
        mColumnFactory.setProperties(new AltibaseProperties());
    }

    public void testHashSmallInt() throws SQLException
    {
        Column sColumn = mColumnFactory.getMappedColumn(Types.SMALLINT);
        long[] sExpected = new long[] {
                369347305L,
                1007146017L,
                4205498267L,
                1386734784L,
                2943583942L,
                3278633334L,
                1780082249L,
                2588290050L,
                1119789981L,
                988641582L,
                3545678196L,
                639209818L
        };
        int sIndex = 0;
        for (short sValue = -5; sValue <= 5; sValue++)
        {
            sColumn.setValue(sValue);
            assertThat(mHashGenerator.doHash(sColumn, ShardKeyDataType.SMALLINT, "KSC5601"), is(sExpected[sIndex]));
            sIndex++;
        }
        sColumn.setValue(null);
        assertThat(mHashGenerator.doHash(sColumn, ShardKeyDataType.SMALLINT, "KSC5601"), is(sExpected[sIndex]));
    }

    public void testHashInteger() throws SQLException
    {
        Column sColumn = mColumnFactory.getMappedColumn(Types.INTEGER);
        long[] sExpected = new long[] {
                2589733825L,
                3547512108L,
                982015395L,
                2919947769L,
                2838822630L,
                2296798828L,
                1928985373L,
                883116791L,
                170692373L,
                2059507917L,
                192495379L,
                3514345157L
        };
        int sIndex = 0;
        for (int sValue = -5; sValue <= 5; sValue++)
        {
            sColumn.setValue(sValue);
            assertThat(mHashGenerator.doHash(sColumn, ShardKeyDataType.INTEGER, "KSC5601"), is(sExpected[sIndex]));
            sIndex++;
        }
        sColumn.setValue(null);
        assertThat(mHashGenerator.doHash(sColumn, ShardKeyDataType.INTEGER, "KSC5601"), is(sExpected[sIndex]));
    }

    public void testHashBigInteger() throws SQLException
    {
        Column sColumn = mColumnFactory.getMappedColumn(Types.BIGINT);
        long[] sExpected = new long[] {
                2854929103L,
                2528956887L,
                52148426L,
                3464261093L,
                4047718213L,
                579978139L,
                4027148833L,
                3983841990L,
                1445872320L,
                1184709306L,
                3187856275L,
                2500430971L
        };
        int sIndex = 0;
        for (long sValue = -5; sValue <= 5; sValue++)
        {
            sColumn.setValue(sValue);
            assertThat(mHashGenerator.doHash(sColumn, ShardKeyDataType.BIGINT, "KSC5601"), is(sExpected[sIndex]));
            sIndex++;
        }
        sColumn.setValue(null);
        assertThat(mHashGenerator.doHash(sColumn, ShardKeyDataType.BIGINT, "KSC5601"), is(sExpected[sIndex]));
    }

    public void testHashCharVarchar() throws SQLException
    {
        Column sColumn = mColumnFactory.getMappedColumn(Types.VARCHAR);
        long[] sExpected = new long[] {
                4214014587L,
                1731281278L,
                1731281278L,
                3734300004L,
                594997397L,
                16909060L
        };
        int sIndex = 0;
        String[] sSample = new String[] { "001", "abcd", "  abcd  ", "°¡³ª´Ù", "1234", "" };
        for (String sValue : sSample)
        {
            sColumn.setValue(sValue);
            assertThat(mHashGenerator.doHash(sColumn, ShardKeyDataType.VARCHAR, "KSC5601"), is(sExpected[sIndex]));
            sIndex++;
        }
    }
}
