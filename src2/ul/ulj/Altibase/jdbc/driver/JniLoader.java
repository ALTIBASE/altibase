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

package Altibase.jdbc.driver;

public class JniLoader
{
    /* JNI module for ALTIBASE JDBC extension */
    static private final String JNI_EXT_MODULE = "altijext";
    static private boolean mShouldLoadExt = true;
    static private boolean mIsLoadedExt;
    static private Object  mLoadExtLockObj = new Object();

    private JniLoader()
    {
    }

    static boolean shouldLoadExtModule()
    {
        return mShouldLoadExt;
    }

    static boolean isLoadedExtModule()
    {
        return mIsLoadedExt;
    }

    static void loadExtModule()
    {
        if (mShouldLoadExt)
        {
            synchronized (mLoadExtLockObj)
            {
                if (mShouldLoadExt)
                {
                    mShouldLoadExt = false;

                    System.loadLibrary(JNI_EXT_MODULE);

                    mIsLoadedExt = true;
                }
            }
        }
    }
}
