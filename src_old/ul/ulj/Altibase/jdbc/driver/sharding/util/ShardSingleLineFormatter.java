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

package Altibase.jdbc.driver.sharding.util;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.MessageFormat;
import java.util.Date;
import java.util.logging.Formatter;
import java.util.logging.LogRecord;

public class ShardSingleLineFormatter extends Formatter
{
    private final static String DATE_FORMAT    = "{0,date} {0,time}";
    private MessageFormat       mFormatter;
    private String              mLineSeparator = System.getProperty("line.separator");

    /**
     * 샤드 로그를 포매팅한다.<br>
     * 모든 요청이 ShardingTraceLogger로부터 생성되기 때문에 클래스와 메소드 정보는 생략한다.
     * @param aRecord 로그 레코드
     * @return 포맷팅된 스트링
     */
    public String format(LogRecord aRecord)
    {
        Date sDate = new Date();
        sDate.setTime(aRecord.getMillis());

        Object sArgs[] = new Object[1];
        sArgs[0] = sDate;

        // Date, Time
        if (mFormatter == null)
        {
            mFormatter = new MessageFormat(DATE_FORMAT);
        }
        StringBuffer sText = new StringBuffer();
        mFormatter.format(sArgs, sText, null);

        StringBuilder sSb = new StringBuilder();
        sSb.append(sText).append(' ');
        sSb.append(" - ");

        String sMessage = formatMessage(aRecord);

        // 레벨
        sSb.append(aRecord.getLevel().getLocalizedName()).append(": ");

        int sOffset = (1000 - aRecord.getLevel().intValue()) / 100;
        for (int i = 0; i < sOffset; i++)
        {
            sSb.append(' ');
        }

        sSb.append(sMessage).append(mLineSeparator);
        if (aRecord.getThrown() != null)
        {
            try
            {
                StringWriter sStringWriter = new StringWriter();
                PrintWriter sPrintWriter = new PrintWriter(sStringWriter);
                aRecord.getThrown().printStackTrace(sPrintWriter);
                sPrintWriter.close();
                sSb.append(sStringWriter.toString());
            }
            catch (Exception aException)
            {
                aException.printStackTrace();
            }
        }
        return sSb.toString();
    }
}
