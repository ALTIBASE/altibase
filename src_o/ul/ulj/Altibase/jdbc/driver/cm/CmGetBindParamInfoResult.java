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

import java.util.ArrayList;
import java.util.List;

import Altibase.jdbc.driver.datatype.ColumnInfo;

public class CmGetBindParamInfoResult extends CmStatementIdResult
{
    public static final byte MY_OP = CmOperation.DB_OP_GET_PARAM_INFO_RESULT;
    
    private List mColumnInfoList = null;
    
    public CmGetBindParamInfoResult()
    {
        mColumnInfoList = new ArrayList();
    }
    
    public ColumnInfo getColumnInfo(int aParamIdx)
    {
        return (ColumnInfo)mColumnInfoList.get(aParamIdx - 1);
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }

    public void addColumnInfo(int aParamIdx, ColumnInfo aColumnInfo)
    {
        if (mColumnInfoList.size() >= aParamIdx)
        {
            // BUG-42879 이미 컬럼정보가 있을때는 해당하는 정보를 수정한다.
            mColumnInfoList.set(aParamIdx - 1, aColumnInfo);
        }
        else
        {
            /*
             * BUG-42879 deferred상태에서 순서에 상관없이 setXXX가 호출되는 경우를 위한 처리.
             * 예를들어 setXXX(3, 1)과 같이 먼저 3번째 인덱스가 먼저 호출되면 컬럼정보리스트의 첫번째와
             * 두번째 멤버를 null로 초기화하고 세번째 멤버로 null을 추가한다.
             */
            for (int i = mColumnInfoList.size(); i < aParamIdx - 1; i++)
            {
                mColumnInfoList.add(null);
            }
            mColumnInfoList.add(aColumnInfo);
        }
    }

    public int getColumnInfoListSize()
    {
        return mColumnInfoList.size();
    }
    public void clear()
    {
        mColumnInfoList.clear();
    }
}
