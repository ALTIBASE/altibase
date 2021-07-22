
/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.executor;

import Altibase.jdbc.driver.AltibaseConnection;
import Altibase.jdbc.driver.AltibaseStatement;
import Altibase.jdbc.driver.ex.ShardError;
import Altibase.jdbc.driver.ex.ShardJdbcException;
import Altibase.jdbc.driver.sharding.core.DataNode;

import java.sql.SQLException;
import java.sql.Statement;
import java.util.*;
import java.util.concurrent.*;
import java.util.logging.Level;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class MultiThreadExecutorEngine implements ExecutorEngine
{
    public <T> List<T> executeStatement(List<Statement> aStatements,
                                        ExecuteCallback<T> aExecuteCallback) throws SQLException
    {
        List<T> sRestOutputs = new ArrayList<T>();
        List<SQLException> sExceptionList = new ArrayList<SQLException>();

        if (aStatements.isEmpty())
        {
            return Collections.emptyList();
        }
        List<Future<T>> sFutures = null;
        if (aStatements.size() > 1)
        {
            sFutures = asyncExecute(aStatements, aExecuteCallback);
            sRestOutputs = getRestFutures(sFutures, sExceptionList);
        }
        else
        {
            Statement sFirstInput = aStatements.get(0);
            T sFirstOutput = null;
            try
            {
                sFirstOutput = syncExecute(sFirstInput, aExecuteCallback);
            }
            catch (SQLException aException)
            {
                shard_log(Level.SEVERE, "(NODE EXECUTION EXCEPTION) ", aException);
                sExceptionList.add(aException);
            }
            sRestOutputs.add(0, sFirstOutput);
        }
        ShardError.throwSQLExceptionIfExists(sExceptionList);
        return sRestOutputs;
    }

    public <T> List<T> generateStatement(List<DataNode> aNodes, GenerateCallback<T> aGenerateStmtCallback) throws SQLException
    {
        if (aNodes.isEmpty())
        {
            return Collections.emptyList();
        }

        List<Future<T>> sRestFutures = null;

        // BUG-47460 node 갯수가 2개 이상일때만 async로 생성
        if (aNodes.size() > 1)
        {
            sRestFutures = asyncGenerate(aNodes, aGenerateStmtCallback);
        }
        T sFirstStatement = null;
        List<T> sRestStatements;
        List<SQLException> sExceptionList = new ArrayList<SQLException>();

        DataNode sFirst = aNodes.iterator().next();
        try
        {
            sFirstStatement = syncGenerate(sFirst, aGenerateStmtCallback);
        }
        catch (SQLException aException)
        {
            shard_log(Level.SEVERE, "(GENERATE STATEMENT EXCEPTION) ", aException);
            sExceptionList.add(aException);
        }

        sRestStatements = getRestFutures(sRestFutures, sExceptionList);
        ShardError.throwSQLExceptionIfExists(sExceptionList);
        sRestStatements.add(0, sFirstStatement);

        return sRestStatements;
    }

    private <T> T syncGenerate(DataNode aFirst,
                               GenerateCallback<T> aGenerateStmtCallback) throws SQLException
    {
        T sResult = null;

        try
        {
            sResult = aGenerateStmtCallback.generate(aFirst);
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            throw aShardJdbcEx;
        }
        catch (SQLException aException)
        {
            ExecutorExceptionHandler.handleException(aException, aFirst.getNodeName());
        }

        return sResult;

    }

    private <T> List<Future<T>> asyncGenerate(List<DataNode> aNodes,
                                              final GenerateCallback<T> aGenerateStmtCallback) throws SQLException
    {
        List<Callable<T>> sCallables = new ArrayList<Callable<T>>(aNodes.size());

        boolean isFirst = true;
        for (final DataNode sEach : aNodes)
        {
            if (!isFirst) // BUG-47460 첫번째는 sync로 처리되기 때문에 건너뛴다.
            {
                sCallables.add(new Callable<T>()
                {
                    public T call() throws Exception
                    {
                        T sResult = null;

                        try
                        {
                            sResult = aGenerateStmtCallback.generate(sEach);
                        }
                        catch (ShardJdbcException aShardJdbcEx)
                        {
                            throw aShardJdbcEx;
                        }
                        catch (SQLException aException)
                        {
                            ExecutorExceptionHandler.handleException(aException, sEach.getNodeName());
                        }

                        return sResult;

                    }
                });
            }
            isFirst = false;
        }

        List<Future<T>> sResult = null;

        try
        {
            ExecutorService sExecutorService = SingletonExecutorService.getExecutorService();
            sResult = sExecutorService.invokeAll(sCallables);
        }
        catch (InterruptedException aException)
        {
            ExecutorExceptionHandler.handleException(aException);
        }

        return sResult;

    }

    private <T> List<T> getRestFutures(List<Future<T>> aRestFutures, List<SQLException> aExceptionList)
    {
        List<T> sResult = new ArrayList<T>();
        if (aRestFutures == null)
        {
            return sResult;
        }
        for (Future<T> sFuture : aRestFutures)
        {
            try
            {
                sResult.add(sFuture.get());
            }
            catch (ExecutionException aEx)
            {
                Throwable sThrowable = aEx.getCause();
                if (sThrowable instanceof SQLException)
                {
                    aExceptionList.add((SQLException)sThrowable);
                }
                else
                {
                    throw new RuntimeException(sThrowable);
                }
            }
            catch (InterruptedException aInterruptEx)
            {
                shard_log(Level.SEVERE, aInterruptEx);
                throw new RuntimeException(aInterruptEx);
            }
        }

        return sResult;
    }

    private <T> T syncExecute(Statement aStatement, ExecuteCallback<T> aExecuteCallback) throws SQLException
    {
        T sResult = null;

        try
        {
            sResult = aExecuteCallback.execute(aStatement);
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            throw aShardJdbcEx;
        }
        catch (SQLException aException)
        {
            AltibaseConnection sConn = (AltibaseConnection)aStatement.getConnection();
            ExecutorExceptionHandler.handleException(aException, sConn.getNodeName());
        }

        return sResult;
    }

    private <T> List<Future<T>> asyncExecute(List<Statement> aStatements,
                                             final ExecuteCallback<T> aExecuteCallback) throws SQLException
    {
        List<Callable<T>> sCallableList = new ArrayList<Callable<T>>(aStatements.size() - 1);

        for (final Statement sEach : aStatements)
        {
            sCallableList.add(new Callable<T>()
            {
                public T call() throws Exception
                {
                    T sResult = null;

                    try
                    {
                        sResult = aExecuteCallback.execute(sEach);
                    }
                    catch (ShardJdbcException aShardJdbcEx)
                    {
                        throw aShardJdbcEx;
                    }
                    catch (SQLException aException)
                    {
                        AltibaseConnection sConn = (AltibaseConnection)sEach.getConnection();
                        ExecutorExceptionHandler.handleException(aException, sConn.getNodeName());
                    }

                    return sResult;
                }
            });
        }

        List<Future<T>> sResult = null;

        try
        {
            ExecutorService sExecutorService = SingletonExecutorService.getExecutorService();
            sResult = sExecutorService.invokeAll(sCallableList);
        }
        catch (Exception aException)
        {
            ExecutorExceptionHandler.handleException(aException);
        }

        return sResult;
    }

    public void closeStatements(Collection<Statement> aStatements) throws SQLException
    {
        if (aStatements.isEmpty())
        {
            return;
        }

        List<Future<Void>> sFutureList = asyncStmtClose(aStatements);
        List<SQLException> sExceptionList = new ArrayList<SQLException>();

        try
        {
            aStatements.iterator().next().close();
        }
        catch (SQLException aEx)
        {
            shard_log(Level.SEVERE, "(STATEMENT CLOSE EXCEPTION) ", aEx);
            sExceptionList.add(aEx);
        }
        getFutures(sFutureList, sExceptionList);
        ShardError.throwSQLExceptionIfExists(sExceptionList);

    }

    private void getFutures(List<Future<Void>> aFutureList, List<SQLException> aExceptionList)
    {
        if (aFutureList == null)
        {
            return;
        }

        for (Future<Void> sEach : aFutureList)
        {
            try
            {
                sEach.get();
            }
            catch (ExecutionException aEx)
            {
                Throwable sThrowable = aEx.getCause();
                if (sThrowable instanceof SQLException)
                {
                    // BUG-46790 노드 중간에 에러가 나더라도 모든 노드들의 처리 결과를 취합해야 한다.
                    aExceptionList.add((SQLException)sThrowable);
                }
                else
                {
                    throw new RuntimeException(sThrowable);
                }
            }
            catch (InterruptedException aInterruptEx)
            {
                shard_log(Level.SEVERE, aInterruptEx);
                throw new RuntimeException(aInterruptEx);
            }
        }
    }

    private List<Future<Void>> asyncStmtClose(Collection<Statement> aStatements) throws SQLException
    {
        if (aStatements.size() <= 1)
        {
            return null;
        }

        boolean sIsFirst = true;
        List<Callable<Void>> sCallables = new ArrayList<Callable<Void>>();
        for (final Statement sStmt : aStatements)
        {
            if (!sIsFirst)
            {
                sCallables.add(new Callable<Void>()
                {
                    public Void call() throws SQLException
                    {
                        try
                        {
                            sStmt.close();
                        }
                        catch (SQLException aEx)
                        {
                            AltibaseConnection sConn = (AltibaseConnection)sStmt.getConnection();
                            ExecutorExceptionHandler.handleException(aEx, sConn.getNodeName());
                        }
                        return null;
                    }
                });
            }
            sIsFirst = false;
        }
        return invokeCallables(sCallables);
    }

    private List<Future<Void>> invokeCallables(List<Callable<Void>> aCallables) throws SQLException
    {
        List<Future<Void>> sFutureList = null;
        try
        {
            ExecutorService sExecutorService = SingletonExecutorService.getExecutorService();
            sFutureList = sExecutorService.invokeAll(aCallables);
        }
        catch (InterruptedException aException)
        {
            shard_log(Level.SEVERE, aException.getMessage(), aException);
            ExecutorExceptionHandler.handleException(aException);
        }

        return sFutureList;
    }

    public <T> void closeCursor(Collection<T> aStatements,
                                final ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException
    {
        doParallelProcess(aStatements, aParallelProcessCallback);
    }

    public <T> void doPartialRollback(Collection<T> aStatements,
                                      final ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException
    {
        doParallelProcess(aStatements, aParallelProcessCallback);
    }

    public <T> void doTransaction(Collection<T> aConnections,
                                  final ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException
    {
        doParallelProcess(aConnections, aParallelProcessCallback);
    }
    
    private <T> void doParallelProcess(Collection<T> aObjects,
                                       final ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException
    {
        if (aObjects.size() == 0) return;

        Iterator<T> sItr = aObjects.iterator();
        T sFirstObj = sItr.next();

        List<T> sRestObj = new ArrayList<T>();
        while (sItr.hasNext())
        {
            sRestObj.add(sItr.next());
        }

        List<Future<Void>> sFutureList = asyncProcess(sRestObj, aParallelProcessCallback);
        List<SQLException> sExceptionList = new ArrayList<SQLException>();
        
        try
        {
            syncProcess(sFirstObj, aParallelProcessCallback);
        }
        catch (SQLException aEx)
        {
            shard_log(Level.SEVERE, "(NODE DO ParallelProcess EXCEPTION) ", aEx);
            sExceptionList.add(aEx);
        }
        getFutures(sFutureList, sExceptionList);
        ShardError.throwSQLExceptionIfExists(sExceptionList);
    }

    private <T> void syncProcess(T aFirstObj, 
                                 ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException
    {
        try
        {
            aParallelProcessCallback.processInParallel(aFirstObj);
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            throw aShardJdbcEx;
        }
        catch (SQLException aException)
        {
            AltibaseConnection sConn = null;
            if (aFirstObj instanceof AltibaseConnection)
            {
                sConn = (AltibaseConnection)aFirstObj;
            }
            else if (aFirstObj instanceof AltibaseStatement)
            {
                sConn = (AltibaseConnection)((AltibaseStatement)aFirstObj).getConnection();
            }
            ExecutorExceptionHandler.handleException(aException, sConn.getNodeName());
        }
    }
    
    private <T> List<Future<Void>> asyncProcess(List<T> aRestObj, 
                                                ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException
    {
        if (aRestObj.size() == 0) return null;
        List<Callable<Void>> sCallables = makeTransactionCallables(aRestObj, aParallelProcessCallback);
        List<Future<Void>> sFutureList = null;
        try
        {
            ExecutorService sExecutorService = SingletonExecutorService.getExecutorService();
            sFutureList = sExecutorService.invokeAll(sCallables);
        }
        catch (InterruptedException aException)
        {
            shard_log(Level.SEVERE, aException.getMessage(), aException);
            ExecutorExceptionHandler.handleException(aException);
        }
        
        return sFutureList;
    }

    private <T> List<Callable<Void>> makeTransactionCallables(List<T> aRestObj,
                                                              final ParallelProcessCallback<T> aParallelProcessCallback)
    {
        List<Callable<Void>> sCallables = new ArrayList<Callable<Void>>();
        for (final T sEach : aRestObj)
        {
            sCallables.add(new Callable<Void>()
            {
                public Void call() throws Exception
                {
                    try
                    {
                        aParallelProcessCallback.processInParallel(sEach);
                    }
                    catch (ShardJdbcException aShardJdbcEx)
                    {
                        throw aShardJdbcEx;
                    }
                    catch (SQLException aException)
                    {
                        AltibaseConnection sConn = null;
                        if (sEach instanceof AltibaseConnection)
                        {
                            sConn = (AltibaseConnection)sEach;
                        }
                        else if (sEach instanceof AltibaseStatement)
                        {
                            sConn = (AltibaseConnection)((AltibaseStatement)sEach).getConnection();
                        }
                        ExecutorExceptionHandler.handleException(aException, sConn.getNodeName());
                    }
                    return null;
                }
            });
        }
        return sCallables;
    }
}
