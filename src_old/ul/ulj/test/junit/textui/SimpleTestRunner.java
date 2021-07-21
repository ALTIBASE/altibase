package junit.textui;

import java.io.PrintStream;

import junit.framework.Test;
import junit.framework.TestListener;
import junit.framework.TestResult;
import junit.runner.BaseTestRunner;

public class SimpleTestRunner extends BaseTestRunner
{
    private TestListener mTestListener;

    public SimpleTestRunner()
    {
        this(System.out);
    }

    public SimpleTestRunner(PrintStream aPrintStream)
    {
        mTestListener = new SimpleTestListener(aPrintStream);
    }

    public void testFailed(int aType, Test aTest, Throwable aError)
    {
    }

    public void testStarted(String aTestString)
    {
    }

    public void testEnded(String aTestString)
    {
    }

    protected void runFailed(String aMessage)
    {
    }

    protected TestResult createTestResult()
    {
        TestResult sResult = new TestResult();
        sResult.addListener(mTestListener);
        return sResult;
    }

    public TestResult run(String aSuiteClassName) throws Exception
    {
        Test sTestSuite = getTest(aSuiteClassName);
        TestResult sResult = createTestResult();
        sTestSuite.run(sResult);
        return sResult;
    }

    public static void main(String aSuitClassNames[])
    {
        if (aSuitClassNames.length == 0)
        {
            System.out.println("USAGE: SimpleTestRunner {suiteClassName}+");
            return;
        }

        int sTotRunCount = 0;
        int sTotFailCount = 0;
        int sTotErrorCount = 0;
        SimpleTestRunner aTestRunner = new SimpleTestRunner();
        for (int i = 0; i < aSuitClassNames.length; i++)
        {
            try
            {
                TestResult sResult = aTestRunner.run(aSuitClassNames[i]);
                sTotRunCount += sResult.runCount();
                sTotFailCount += sResult.failureCount();
                sTotErrorCount += sResult.errorCount();
            }
            catch (Exception sEx)
            {
                sEx.printStackTrace();
            }
        }
        System.out.println();
        System.out.println("Run: " + sTotRunCount + ", Fail: " + sTotFailCount + ", Error: " + sTotErrorCount);
    }
}
