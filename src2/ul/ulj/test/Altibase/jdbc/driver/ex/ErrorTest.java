package Altibase.jdbc.driver.ex;

import junit.framework.TestCase;

import java.io.IOException;
import java.sql.SQLException;
import java.sql.SQLWarning;

/**
 * PROJ-2427 Error.java 리팩토링 후 CheckAndXXX 메소드들이 정상적으로 %s 문자를 치환하는지 검증한다.
 * @author yjpark
 *
 */
public class ErrorTest extends TestCase
{
    public void testThrowSQLExceptionWithNoStringArguments()
    {
        try
        {
            Error.throwSQLException(ErrorDef.CLOSED_CONNECTION);
            fail();
        } 
        catch (SQLException se)
        {
            assertEquals("Connection already closed.", se.getMessage());
        }
    }
    
    public void testThrowSQLExceptionWithOneStringArguments()
    {
        try
        {
            Error.throwSQLException(ErrorDef.COMMUNICATION_ERROR, "arg1");
            fail();
        }
        catch (SQLException se)
        {
            assertEquals("Communication link failure: arg1", se.getMessage());
        }
    }
    
    public void testThrowSQLExceptionWithTwoStringArguments()
    {
        try
        {
            Error.throwSQLException(ErrorDef.INVALID_PACKET_HEADER_VERSION, "arg1", "arg2");
            fail();
        }
        catch (SQLException se)
        {
            assertEquals("Invalid packet header version: <arg1> expected, but <arg2>", se.getMessage());
        }
    }
    
    public void testThrowSQLExceptionWithThreeStringArguments()
    {
        try
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT, "arg1", "arg2", "arg3");
            fail();
        }
        catch (SQLException se)
        {
            assertEquals("Invalid argument: arg1: <arg2> expected, but <arg3>", se.getMessage());
        }
    }
    
    public void testCreateWarningWithNoStringArgument()
    {
        SQLWarning sWarning = Error.createWarning(null, ErrorDef.CLOSED_CONNECTION);
        assertEquals("Connection already closed.", sWarning.getMessage());
    }
    
    public void testCreateWarningWithOneStringArgument()
    {
        SQLWarning sWarning = Error.createWarning(null, ErrorDef.COMMUNICATION_ERROR, "arg1");
        assertEquals("Communication link failure: arg1", sWarning.getMessage());
    }
    
    public void testCreateWarningWithTwoStringArguments()
    {
        SQLWarning sWarning = Error.createWarning(null, ErrorDef.INVALID_PACKET_HEADER_VERSION, "arg1", "arg2");
        assertEquals("Invalid packet header version: <arg1> expected, but <arg2>", sWarning.getMessage());
    }
    
    public void testthrowIOExceptionWithNoStringArgument()
    {
        try
        {
            Error.throwIOException(ErrorDef.CLOSED_CONNECTION);
        }
        catch (IOException ie)
        {
            assertEquals("Connection already closed.", ie.getMessage());
        }
    }
}