package junit.textui;

import junit.framework.AssertionFailedError;
import junit.framework.Test;
import junit.framework.TestListener;

import java.io.PrintStream;
import java.util.Iterator;
import java.util.LinkedList;

public class SimpleTestListener implements TestListener
{
    private static final int      MAX_TESTSTRING_LEN = 60;
    private static final int      TYPE_PASS          = 0;
    private static final int      TYPE_ERROR         = 1;
    private static final int      TYPE_FAIL          = 2;
    private static final String   ANSICOLOR_CLEAR    = "\033[0m";
    private static final String   ANSICOLOR_GRAY     = "\033[0;30;1m";
    private static final String   ANSICOLOR_RED      = "\033[0;31;1m";
    private static final String   ANSICOLOR_GREEN    = "\033[0;32m";
    private static final String   ANSICOLOR_YELLOW   = "\033[0;33;1m";
    private static final String   ANSICOLOR_WHITE    = "\033[0;37;1m";
    private static final String[] RESULT_COLOR       = { ANSICOLOR_GREEN, ANSICOLOR_RED, ANSICOLOR_YELLOW };
    private static final String[] RESULT_STR         = { "PASS", "ERROR", "FAIL" };

    private PrintStream           mPrintStream;
    private boolean               mUseAnsiColor;
    private int                   mErrorCount;
    private int                   mFailCount;
    private String                mCurrSuiteName;
    private LinkedList            mError             = new LinkedList();

    public SimpleTestListener(PrintStream aPrintStream)
    {
        mPrintStream = aPrintStream;
        // BUG-44466 ansi color 사용여부를 시스템 파라메터로 부터 얻는다.
        String sUseAnsiColor = System.getProperty("UseAnsiColor");
        mUseAnsiColor = sUseAnsiColor == null || sUseAnsiColor.toLowerCase().equals("true");
    }

    public void addError(Test aTest, Throwable aError)
    {
        mErrorCount++;
        mError.add(aError);
    }

    public void addFailure(Test aTest, AssertionFailedError aError)
    {
        mFailCount++;
        mError.add(aError);
    }

    public void endTest(Test aTest)
    {
        int sType = (mErrorCount > 0) ? TYPE_ERROR : (mFailCount > 0) ? TYPE_FAIL : TYPE_PASS;
        if (mUseAnsiColor)
        {
            mPrintStream.print(RESULT_COLOR[sType] + RESULT_STR[sType] + ANSICOLOR_CLEAR);
        }
        else
        {
            mPrintStream.print(RESULT_STR[sType]);
        }
        mPrintStream.println();
        Iterator sIt = mError.iterator();
        if (sIt.hasNext())
        {
            do
            {
                Throwable sErr = (Throwable)sIt.next();
                sErr.printStackTrace(mPrintStream);
            } while (sIt.hasNext());
            mError.clear();
            mPrintStream.println();
        }
    }

    public void startTest(Test aTest)
    {
        mErrorCount = 0;
        mFailCount = 0;

        String sTestString = aTest.toString();
        int l = sTestString.indexOf('(');
        int r = sTestString.lastIndexOf(')');
        String sTestSuite = sTestString.substring(l + 1, r);
        String sTestName = sTestString.substring(0, Math.min(l, MAX_TESTSTRING_LEN));
        if (!sTestSuite.equals(mCurrSuiteName))
        {
            if (mUseAnsiColor)
            {
                mPrintStream.println(ANSICOLOR_WHITE + sTestSuite + ANSICOLOR_CLEAR);
            }
            else
            {
                mPrintStream.println(sTestSuite);
            }
            mCurrSuiteName = sTestSuite;
        }
        mPrintStream.print(" - " + sTestName + " ");
        if (mUseAnsiColor)
        {
            mPrintStream.print(ANSICOLOR_GRAY);
        }
        for (int i = sTestName.length(); i < MAX_TESTSTRING_LEN; i++)
        {
            mPrintStream.print(".");
        }
        if (mUseAnsiColor)
        {
            mPrintStream.print(ANSICOLOR_CLEAR);
        }
        mPrintStream.print(" ");
    }
}
