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

import Altibase.jdbc.driver.sharding.core.DataNode;

import java.sql.SQLException;
import java.sql.Statement;
import java.util.Collection;
import java.util.List;

public interface ExecutorEngine
{
    <T> List<T> executeStatement(List<Statement> aStatements,
                                 ExecuteCallback<T> aExecuteCallback) throws SQLException;

    <T> List<T> generateStatement(List<DataNode> aNodes, GenerateCallback<T> aGenerateStmtCallback) throws SQLException;

    <T> void doTransaction(Collection<T> aConnections,
                       ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException;

    void closeStatements(Collection<Statement> aStatements) throws SQLException;

    <T> void closeCursor(Collection<T> aStatements, ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException;
    <T> void doPartialRollback(Collection<T> aStatements, ParallelProcessCallback<T> aParallelProcessCallback) throws SQLException;
}
