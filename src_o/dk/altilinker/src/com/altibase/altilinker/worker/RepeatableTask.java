/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.worker;

/**
 * Repeatable task class
 */
public abstract class RepeatableTask extends Task
{
    private boolean mCompleted = false;

    /**
     * Constructor
     * 
     * @param aTaskType         Task type
     */
    public RepeatableTask(int aTaskType)
    {
        super(aTaskType);
    }

    /**
     * Whether this task complete or not
     * 
     * @return  true, if this task complete. false, if not.
     */
    public boolean isCompleted()
    {
        return mCompleted;
    }

    /**
     * Set to complete task processing
     */
    protected void setCompleted()
    {
        mCompleted = true;
    }
    
    /**
     * Set task completion flag
     * 
     * @param aCompleted        Task completion flag
     */
    protected void setCompleted(boolean aCompleted)
    {
        mCompleted = aCompleted;
    }
}
