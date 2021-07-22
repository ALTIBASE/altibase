package junit.textui;

import org.junit.runner.JUnitCore;
import org.junit.runner.Request;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;

public class SimpleTestMethodRunner
{
    public static void main(String... aArgs) throws ClassNotFoundException
    {
        if (aArgs.length != 1)
        {
            System.out.println("USAGE: SimpleTestMethodRunner ClassName#MethodName");
            return;
        }

        String[] sClassAndMethod = aArgs[0].split("#");

        if (sClassAndMethod.length != 2)
        {
            System.out.println("USAGE: SimpleTestMethodRunner ClassName#MethodName");
            return;
        }

        Request sRequest = Request.method(Class.forName(sClassAndMethod[0]), sClassAndMethod[1]);

        System.out.println(sClassAndMethod[0]);
        System.out.print(" - " + sClassAndMethod[1] + " ");

        for (int i = sClassAndMethod[1].length(); i < 60; i++)
        {
            System.out.print(".");
        }
        System.out.print(" ");

        Result sResult = new JUnitCore().run(sRequest);
        System.out.println(sResult.wasSuccessful() ? "PASS" : "FAIL");

        if (sResult.getFailureCount() > 0)
        {
            for (Failure sFailure : sResult.getFailures())
            {
                sFailure.getException().printStackTrace();
            }
        }
    }
}
