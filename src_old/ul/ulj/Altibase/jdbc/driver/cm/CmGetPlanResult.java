/*
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

package Altibase.jdbc.driver.cm;

public class CmGetPlanResult extends CmStatementIdResult
{
    static final byte MY_OP = CmOperation.DB_OP_GET_PLAN_RESULT;

    private String mPlanText;

    CmGetPlanResult()
    {
    }

    protected byte getResultOp()
    {
        return MY_OP;
    }

    public String getPlanText()
    {
        return mPlanText;
    }

    void setPlanText(String aPlanText)
    {
        mPlanText = aPlanText;
    }
}
