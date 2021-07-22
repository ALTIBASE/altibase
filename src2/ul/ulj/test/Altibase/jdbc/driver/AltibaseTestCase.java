package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;

import junit.framework.SqlTestCase;

public abstract class AltibaseTestCase extends SqlTestCase
{
    public static final String DRIVER_CLASS = "Altibase.jdbc.driver.AltibaseDriver";
    public static final String CONN_DSN     = "newjdbc_unittest";
    public static final String CONN_URL     = "jdbc:Altibase://" + CONN_DSN;
    public static final String CONN_USER    = "SYS";
    public static final String CONN_PWD     = "MANAGER";

    protected void ensureLoadDriverClass() throws ClassNotFoundException
    {
        try
        {
            Class.forName(DRIVER_CLASS);
        }
        catch (SecurityException sEx)
        {
            // 한번만 수행되기 때문에 이런일이 일어나지는 않아야 할 것이다만, 일단 넣어둔다.
            assertTrue(sEx.getMessage().endsWith(": already loaded"));
        }
    }

    protected String getURL()
    {
        return CONN_URL;
    }

    protected String getUser()
    {
        return CONN_USER;
    }

    protected String getPassword()
    {
        return CONN_PWD;
    }

    protected boolean isIgnorableErrorForClean(int aErrorCode)
    {
        switch (aErrorCode)
        {
            case ErrorDef.TABLE_NOT_FOUND:
            case ErrorDef.PROCEDURE_OR_FUNCTION_NOT_FOUND:
            case ErrorDef.SEQUENCE_NOT_FOUND:
            case ErrorDef.MATERIALIZED_VIEW_NOT_FOUND:
            case ErrorDef.SYNONYM_NOT_FOUND:
            case ErrorDef.UNDEFINED_USER_NAME:
                return true;

        }
        return false;
    }
}
