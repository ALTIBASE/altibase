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
 
package com.altibase.altilinker.adlp.op;

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.util.LinkedList;

import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.util.TraceLogger;

public class CreateLinkerCtrlSessionOp extends RequestOperation
{
    static TraceLogger mTraceLogger = TraceLogger.getInstance();
    
    public String     mProductInfo = null;
    public int        mDBCharSet   = DBCharSet.None;
    public Properties mProperties  = null;
    
    public CreateLinkerCtrlSessionOp()
    {
        super(OpId.CreateLinkerCtrlSession, false, true);
    }

    public ResultOperation newResultOperation()
    {
        return new CreateLinkerCtrlSessionResultOp();
    }
    
    protected boolean isFragmented(CommonHeader aCommonHeader)
    {
        byte sOpSpecificParameter = aCommonHeader.getOpSpecificParameter();
        
        boolean sFragmented = (sOpSpecificParameter != 0) ? true : false;
        
        return sFragmented;
    }
    
    protected boolean readOperation(CommonHeader aCommonHeader,
                                    ByteBuffer   aOpPayload)
    {
        // common header
        setCommonHeader(aCommonHeader);
        
        if (aCommonHeader.getDataLength() > 0)
        {
            try
            {
                // product info
                mProductInfo = readVariableString(aOpPayload);
        
                // database character set
                mDBCharSet = readInt(aOpPayload);
                
                // set database character set for analyzing protocol.
                String sDBCharSetName = DBCharSet.getDBCharSetName(mDBCharSet);
                
                Operation.setDBCharSetName(sDBCharSetName);
                
                // properties
                int i;
                int sPropertyCount;
                int sPropertyType;
                int sPropertyLength;
                
                mProperties = new Properties();
        
                sPropertyCount = readInt(aOpPayload);
                for (i = 0; i < sPropertyCount; ++i)
                {
                    sPropertyType   = readInt(aOpPayload);
                    sPropertyLength = readInt(aOpPayload);
                    
                    switch (sPropertyType)
                    {
                    case PropertyType.RemoteNodeReceiveTimeout:
                        {
                            mProperties.mRemoteNodeReceiveTimeout =
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mRemoteNodeReceiveTimeout);
                            break;
                        }
                        
                    case PropertyType.QueryTimeout:
                        {
                            mProperties.mQueryTimeout =
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mQueryTimeout);
                            break;
                        }
                    
                    case PropertyType.NonQueryTimeout:
                        {
                            mProperties.mNonQueryTimeout =
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mNonQueryTimeout);
                            break;
                        }
                        
                    case PropertyType.ThreadCount:
                        {
                            mProperties.mThreadCount =
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mThreadCount);
                            break;
                        }
        
                    case PropertyType.ThreadSleepTimeout:
                        {
                            mProperties.mThreadSleepTimeout = 
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mThreadSleepTimeout);
                            break;
                        }
        
                    case PropertyType.RemoteNodeSessionCount:
                        {
                            mProperties.mRemoteNodeSessionCount = 
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mRemoteNodeSessionCount);
                            break;
                        }
        
                    case PropertyType.TraceLogFileSize:
                        {
                            mProperties.mTraceLogFileSize = 
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mTraceLogFileSize);
                            break;
                        }
        
                    case PropertyType.TraceLogFileCount:
                        {
                            mProperties.mTraceLogFileCount = 
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mTraceLogFileCount);
                            break;
                        }
        
                    case PropertyType.TraceLoggingLevel:
                        {
                            mProperties.mTraceLoggingLevel = 
                                    readInt(sPropertyLength,
                                            aOpPayload,
                                            mProperties.mTraceLoggingLevel);
                            break;
                        }
        
                    case PropertyType.TargetList:
                        {
                            readTargetList(aOpPayload);
                            break;
                        }
                        
                    default:
                        {
                            mTraceLogger.logWarning(
                                    "Unknown property type = {0}",
                                    new Object[] { new Integer(sPropertyType) } );
                            break;
                        }
                    }
                }
            }
            catch (BufferUnderflowException e)
            {
                return false;
            }
        }
        
        return true;
    }
    
    private void readTargetList(ByteBuffer aOpPayload)
    {
        int    sTargetItemCount    = 0;
        int    sTargetFieldBit     = 0;
        int    sTargetFieldBitFlag = 0;
        int    sTargetFieldType    = 0;
        String sTargetFieldValue   = null;
        Target sTarget             = null;

        if (mProperties.mTargetList == null)
        {
            mProperties.mTargetList = new LinkedList();
        }
        
        try
        {
            // read target item count
            sTargetItemCount = readInt(aOpPayload);
            
            do
            {
                // read target item's field type
                sTargetFieldType = readInt(aOpPayload);
                
                if ((sTargetFieldType < PropertyType.TargetItemBegin) ||
                    (sTargetFieldType > PropertyType.TargetItemEnd))
                {
                    break;
                }
                
                sTargetFieldBit = 1 << (sTargetFieldType - PropertyType.TargetItemBegin);
                
                // read target item's field value
                sTargetFieldValue = readVariableString(aOpPayload);
                
                if ((sTargetFieldBitFlag == 0) ||
                    ((sTargetFieldBitFlag & sTargetFieldBit) == 1))
                {
                    if (sTargetFieldBitFlag != 0)
                    {
                        // add to target list
                        mProperties.mTargetList.add(sTarget);
                        
                        sTargetFieldBitFlag = 0;
                    }
                    
                    sTarget = new Target();
                }
                
                if (sTargetFieldValue != null)
                {
                    switch (sTargetFieldType)
                    {
                    case PropertyType.TargetItemName:
                        // convert to upper case
                        sTarget.mName = sTargetFieldValue.toUpperCase();
                        break;
                        
                    case PropertyType.TargetItemJdbcDriver:
                        sTarget.mDriver.mDriverPath = sTargetFieldValue;
                        break;

                    case PropertyType.TargetItemJdbcDriverClassName:
                        sTarget.mDriver.mDriverClassName = sTargetFieldValue;
                        break;

                    case PropertyType.TargetItemConnectionUrl:
                        sTarget.mConnectionUrl = sTargetFieldValue;
                        break;
                        
                    case PropertyType.TargetItemUser:
                        sTarget.mUser = sTargetFieldValue;
                        break;
                        
                    case PropertyType.TargetItemPassword:
                        sTarget.mPassword = sTargetFieldValue;
                        break;
                        
                    case PropertyType.TargetItemXADataSourceClassName:
                        sTarget.mXADataSourceClassName = sTargetFieldValue;
                        break;
                        
                    case PropertyType.TargetItemXADataSourceUrlSetterName:
                        sTarget.mXADataSourceUrlSetterName = sTargetFieldValue;
                        break;
                        
                    default:
                        mTraceLogger.logWarning(
                                "Unknown field type of target property = {0}",
                                new Object[] { new Integer(sTargetFieldType) } );
                        break;
                    }
                }

                sTargetFieldBitFlag |= sTargetFieldBit;
                
            } while (true);
        }
        catch (BufferUnderflowException e)
        {
            mTraceLogger.log(e);
        }
        
        if (sTargetFieldBitFlag != 0)
        {
            // add to target list
            mProperties.mTargetList.add(sTarget);
        }
        
        if (mProperties.mTargetList.size() == 0)
        {
            mProperties.mTargetList = null;
        }
    }
    
    protected boolean validate()
    {
        if (getSessionId() != 0)
        {
            return false;
        }
        
        if (mProductInfo == null)
        {
            return false;
        }

        String sDBCharSetName = DBCharSet.getDBCharSetName(mDBCharSet);
        
        if (sDBCharSetName == null)
        {
            return false;
        }
        
        if (sDBCharSetName.length() == 0)
        {
            return false;
        }

        if (mProperties == null)
        {
            return false;
        }

        if (mProperties.validate() == false)
        {
            return false;
        }
        
        return true;
    }
}
