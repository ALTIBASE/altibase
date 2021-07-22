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

package Altibase.jdbc.driver.util;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Map;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class AltiSqlProcessor
{
    private static final int    INDEX_NOT_FOUND        = -1;

    private static final String INTERNAL_SQL_CURRVAL   = ".CURRVAL ";
    private static final String INTERNAL_SQL_NEXTVAL   = ".NEXTVAL";
    private static final String INTERNAL_SQL_VALUES    = "VALUES";
    private static final String INTERNAL_SQL_SELECT    = "SELECT ";
    private static final String INTERNAL_SQL_FROM_DUAL = " FROM DUAL";
    private static final String INTERNAL_SQL_PROWID    = ",_PROWID ";

    static class SequenceInfo
    {
        final int    mIndex;
        final String mSeqName;
        final String mColumnName;

        SequenceInfo(int aIndex, String aSeqName, String aColumnName)
        {
            mIndex = aIndex;
            mSeqName = aSeqName;
            mColumnName = aColumnName;
        }
    }

    /**
     * 쿼리문을 파싱해 SEQUENCE 이름과 SEQUENCE가 사용된 컬럼의 이름을 얻는다.
     * 
     * @param aSql SEQUENCE 정보를 파싱할 INSERT 쿼리문
     * @return SEQUENCE 정보를 담은 리스트
     */
    public static ArrayList getAllSequences(String aSql)
    {
        ArrayList sSeqInfos = new ArrayList();
        try
        {
            String[] sCols = getColumnListOfInsert(aSql);
            String[] sValues = getValueListOfInsert(aSql);
            for (int i = 0; i < sValues.length; i++)
            {
                if (StringUtils.endsWithIgnoreCase(sValues[i], INTERNAL_SQL_NEXTVAL))
                {
                    String sSeqName = sValues[i].substring(0, sValues[i].length() - INTERNAL_SQL_NEXTVAL.length());
                    String sColName = (sCols == null) ? "\"_AUTO_GENERATED_KEY_COLUMN_\"" : sCols[i];
                    sSeqInfos.add(new SequenceInfo(i + 1, sSeqName, sColName));
                }
            }
        }
        catch (Exception e)
        {
            // parsing error가 발생하면 empty list를 리턴한다.
            sSeqInfos.clear();
        }
        return sSeqInfos;
    }

    /**
     * INSERT 쿼리문에서 컬럼 목록을 얻는다.
     * 
     * @param aInsertSql INSERT 쿼리문
     * @return 컬럼 목록. INSERT 쿼리문에 컬럼 이름 목록이 없다면 null
     */
    private static String[] getColumnListOfInsert(String aInsertSql)
    {
        String sSql = aInsertSql.substring(0, aInsertSql.toUpperCase().indexOf(INTERNAL_SQL_VALUES));
        if (sSql.lastIndexOf("(") < 0)
        {
            // SQL에 column name list가 없는 경우이다.
            return null;
        }
        sSql = sSql.substring(sSql.lastIndexOf("(") + 1, sSql.lastIndexOf(")")).trim();
        String[] sResult = sSql.split(",");
        for (int i = 0; i < sResult.length; i++)
        {
            sResult[i] = sResult[i].trim();
        }
        return sResult;
    }

    /**
     * INSERT 쿼리문에서 값 목록을 얻는다.
     * 
     * @param aInsertSql INSERT 쿼리문
     * @return 값 목록
     */
    private static String[] getValueListOfInsert(String aInsertSql)
    {
        String sSql = aInsertSql.substring(aInsertSql.toUpperCase().indexOf(INTERNAL_SQL_VALUES) + INTERNAL_SQL_VALUES.length() + 1).trim();
        sSql = sSql.substring(sSql.indexOf("(") + 1, sSql.lastIndexOf(")"));
        String[] sResult = sSql.split(",");
        for (int i = 0; i < sResult.length; i++)
        {
            sResult[i] = sResult[i].trim();
        }
        return sResult;
    }

    public static String processEscape(String aOrgSql)
    {
        int sIndex;
        sIndex = aOrgSql.indexOf("{");
        if (sIndex < 0)
        {
            // 성능을 위해 가장먼저 {가 있는지 검사해 없으면 바로 리턴한다.
            return aOrgSql;
        }

        String sResult = processEscapeExpr(aOrgSql, sIndex);
        if (sResult.equals(aOrgSql))
        {
            return sResult;
        }
        return processEscape(sResult);
    }

    private static String processEscapeExpr(String aSql, int aBraceIndex)
    {
        boolean sQuatOpen = false;
        for (int i = 0; i < aBraceIndex; i++)
        {
            if (aSql.charAt(i) == '\'')
            {
                sQuatOpen = !sQuatOpen;
            }
        }
        if (sQuatOpen)
        {
            // '' 안의 {이기 때문에 무시한다. 다음 {를 찾아라~
            aBraceIndex = aSql.indexOf("{", aBraceIndex + 1);
            if (aBraceIndex >= 0)
            {
                return processEscapeExpr(aSql, aBraceIndex);
            }
            else
            {
                return aSql;
            }
        }
        int sEndBraceIndex = INDEX_NOT_FOUND;
        for (int i = aBraceIndex + 1; i < aSql.length(); i++)
        {
            if (aSql.charAt(i) == '\'')
            {
                sQuatOpen = !sQuatOpen;
            }
            else if (!sQuatOpen && aSql.charAt(i) == '}')
            {
                sEndBraceIndex = i;
                break;
            }
        }
        if (sEndBraceIndex == INDEX_NOT_FOUND)
        {
            return aSql;
        }
        String sExpr = convertToNative(aSql.substring(aBraceIndex + 1, sEndBraceIndex).trim()); // { }안의 문자열을 떼온다.
        if (sExpr == null)
        {
            return aSql;
        }
        String sResult = aSql.substring(0, aBraceIndex) + " " + sExpr;
        if (sEndBraceIndex + 1 < aSql.length())
        {
            sResult += " " + aSql.substring(sEndBraceIndex + 1, aSql.length());
        }
        return sResult;
    }

    private static String convertToNative(String aExpr)
    {
        // input은 { }안의 문자열이다. {, }는 뺀 것이다.

        int sIndex = aExpr.indexOf(" ");
        if (sIndex < 0)
        {
            return null;
        }
        String sWord = aExpr.substring(0, sIndex);
        if (sWord.equalsIgnoreCase("escape"))
        {
            // { escape ... } 구문은 그대로 리턴하면 된다.
            return aExpr;
        }
        else if (sWord.equalsIgnoreCase("fn"))
        {
            // { fn ... } 구문 역시 ...를 그대로 리턴하면 된다.
            return aExpr.substring(sIndex + 1, aExpr.length());
        }
        else if (sWord.equalsIgnoreCase("d"))
        {
            // { d '2011-10-17' }는 to_date('2011-10-17', 'yyyy-MM-dd') 형태로 바꾼다.
            return "to_date(" + aExpr.substring(sIndex + 1, aExpr.length()) + ", 'yyyy-MM-dd')";
        }
        else if (sWord.equalsIgnoreCase("t"))
        {
            // { t '12:32:56' }는 to_date('12:32:56', 'hh:mi:ss') 형태로 바꾼다.
            return "to_date(" + aExpr.substring(sIndex + 1, aExpr.length()) + ", 'hh24:mi:ss')";
        }
        else if (sWord.equalsIgnoreCase("ts"))
        {
            // { ts '2011-10-17 12:32:56.123456' }은 to_date('2011-10-17 12:32:56.123456', 'yyyy-MM-dd hh:mi:ss.ffffff')로 바꾼다.
            if (aExpr.indexOf(".") > 0)
            {
                // fraction이 있는 경우
                return "to_date(" + aExpr.substring(sIndex + 1, aExpr.length()) + ", 'yyyy-MM-dd hh24:mi:ss.ff6')";
            }
            // fraction이 없는 경우
            return "to_date(" + aExpr.substring(sIndex + 1, aExpr.length()) + ", 'yyyy-MM-dd hh24:mi:ss')";
        }
        else if (sWord.equalsIgnoreCase("call"))
        {
            // { call procedure_name(...) }는 exec procedure_name(...)로 바꾼다.
            return "execute " + aExpr.substring(sIndex + 1, aExpr.length());
        }
        else if (sWord.startsWith("?"))
        {
            int i;
            // BUG-47424 { ? = call func(?) } 같은 경우 sWord가 ? 이기때문에 aExpr String으로 비교한다.
            for (i = 1; aExpr.charAt(i) == ' '; i++)
                ;
            if (aExpr.charAt(i) != '=')
            {
                return null;
            }
            else
            {
                i++;
            }
            for (; aExpr.charAt(i) == ' '; i++)
                ;

            if (aExpr.substring(i, i + 5).equalsIgnoreCase("call "))
            {
                for (i = i + 5; aExpr.charAt(i) == ' '; i++)
                    ;

                // { ? = call procedure_name(...) }는 exec ? := procedure_name(...)로 바꾼다.
                return "execute ? := " + aExpr.substring(i);
            }
            // ?=call 문법이 아니므로 스킵
        }
        else if (sWord.equalsIgnoreCase("oj"))
        {
            // {oj table1 {left|right|full} outer join table2 on condition}는 oj를 빼고 그대로 사용한다.
            return aExpr.substring(sIndex + 1, aExpr.length());
        }
        return null;
    }

    /**
     * Generated Keys를 얻기위한 쿼리문을 만든다.
     * 
     * @param aSeqs SEQUENCE 정보를 담은 리스트
     * @return Generated Keys를 얻기위한 쿼리문
     */
    public static String makeGenerateKeysSql(ArrayList aSeqs)
    {
        StringBuffer sSql = new StringBuffer(INTERNAL_SQL_SELECT);
        for (int i = 0; i < aSeqs.size(); i++)
        {
            SequenceInfo sSeqInfo = (SequenceInfo)aSeqs.get(i);
            if (i > 0)
            {
                sSql.append(",");
            }
            sSql.append(sSeqInfo.mSeqName).append(INTERNAL_SQL_CURRVAL).append(sSeqInfo.mColumnName);
        }
        sSql.append(INTERNAL_SQL_FROM_DUAL);
        return sSql.toString();
    }

    /**
     * Generated Keys를 얻기위한 쿼리문을 만든다.
     * 
     * @param aSeqs SEQUENCE 정보를 담은 리스트
     * @param aColIndexes 값을 얻을 SEQUENCE의 순번(1 base)을 담은 배열
     * @return Generated Keys를 얻기위한 쿼리문
     */
    public static String makeGenerateKeysSql(ArrayList aSeqs, int[] aColIndexes) throws SQLException
    {
        StringBuffer sSql = new StringBuffer(INTERNAL_SQL_SELECT);
        for (int i = 0; i < aColIndexes.length; i++)
        {
            int j = 0;
            for (; j < aSeqs.size(); j++)
            {
                SequenceInfo sSeqInfo = (SequenceInfo)aSeqs.get(j);
                if (sSeqInfo.mIndex == aColIndexes[i])
                {
                    if (i > 0)
                    {
                        sSql.append(",");
                    }
                    sSql.append(sSeqInfo.mSeqName).append(INTERNAL_SQL_CURRVAL).append(sSeqInfo.mColumnName);
                    break;
                }
            }
            if (j == aSeqs.size())
            {
                StringBuffer sExpIndexs = new StringBuffer();
                for (j = 0; j < aSeqs.size(); j++)
                {
                    SequenceInfo sSeqInfo = (SequenceInfo)aSeqs.get(j);
                    if (j > 0)
                    {
                        sExpIndexs.append(',');
                    }
                    sExpIndexs.append(sSeqInfo.mIndex);
                }
                Error.throwSQLException(ErrorDef.INVALID_COLUMN_INDEX,
                                        sExpIndexs.toString(),
                                        String.valueOf(aColIndexes[i]));
            }
        }
        sSql.append(INTERNAL_SQL_FROM_DUAL);
        return sSql.toString();
    }

    /**
     * Generated Keys를 얻기위한 쿼리문을 만든다.
     * 
     * @param aSeqs SEQUENCE 정보를 담은 리스트
     * @param aColNames 값을 얻을 SEQUENCE가 사용된 컬럼 이름을 담은 배열
     * @return Generated Keys를 얻기위한 쿼리문
     */
    public static String makeGenerateKeysSql(ArrayList aSeqs, String[] aColNames) throws SQLException
    {
        StringBuffer sSql = new StringBuffer(INTERNAL_SQL_SELECT);
        for (int i = 0; i < aColNames.length; i++)
        {
            int j;
            for (j = 0; j < aSeqs.size(); j++)
            {
                SequenceInfo sSeqInfo = (SequenceInfo)aSeqs.get(j);
                if (aColNames[i].equalsIgnoreCase(sSeqInfo.mColumnName))
                {
                    if (i > 0)
                    {
                        sSql.append(',');
                    }
                    sSql.append(sSeqInfo.mSeqName).append(INTERNAL_SQL_CURRVAL).append(sSeqInfo.mColumnName);
                    break;
                }
            }
            if (j == aSeqs.size())
            {
                Error.throwSQLException(ErrorDef.INVALID_COLUMN_NAME, aColNames[i]);
            }
        }
        sSql.append(INTERNAL_SQL_FROM_DUAL);
        return sSql.toString();
    }

    private static int indexOfNonWhitespaceAndComment(String aSrcQstr)
    {
        return indexOfNonWhitespaceAndComment(aSrcQstr, 0);
    }

    private static int indexOfNonWhitespaceAndComment(String aSrcQstr, int aStartIdx)
    {
        for (int i = aStartIdx; i < aSrcQstr.length(); i++)
        {
            char sCh = aSrcQstr.charAt(i);
            if (Character.isWhitespace(sCh))
            {
                continue;
            }
            switch (sCh)
            {
                // C & C++ style comments
                case '/':
                    // block comment
                    if (aSrcQstr.charAt(i + 1) == '*')
                    {
                        for (i += 2; i < aSrcQstr.length(); i++)
                        {
                            if ((aSrcQstr.charAt(i) == '*') && (aSrcQstr.charAt(i + 1) == '/'))
                            {
                                i++;
                                break;
                            }
                        }
                    }
                    // line comment
                    else if (aSrcQstr.charAt(i + 1) == '/')
                    {
                        for (i += 2; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\n'); i++)
                            ; // skip to end-of-line
                    }
                    break;

                // line comment
                case '-':
                    if (aSrcQstr.charAt(i + 1) == '-')
                    {
                        for (i += 2; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\n'); i++)
                            ; // skip to end-of-line
                    }
                    break;

                default:
                    return i;
            }
        }
        return INDEX_NOT_FOUND;
    }

    /**
     * FROM 시작 위치를 찾는다.
     * 
     * @param aSrcQstr 원본 쿼리문
     * @return FROM 절이 있으면 FROM의 시작 위치(0 base), 아니면 -1
     */
    private static int indexOfFrom(String aSrcQstr) throws SQLException
    {
        return indexOfFrom(aSrcQstr, 0);
    }

    /**
     * FROM 시작 위치를 찾는다.
     * 
     * @param aSrcQstr 원본 쿼리문
     * @param aStartIdx 검색을 시작할 index
     * @return FROM 절이 있으면 FROM의 시작 위치(0 base), 아니면 -1
     */
    private static int indexOfFrom(String aSrcQstr, int aStartIdx) throws SQLException
    {
        for (int i = aStartIdx; i < aSrcQstr.length(); i++)
        {
            switch (aSrcQstr.charAt(i))
            {
                // ' ~ '
                case '\'':
                    for (i++; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\''); i++)
                        ;
                    break;

                // " ~ "
                case '"':
                    for (i++; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '"'); i++)
                        ;
                    break;

                // ( ~ )
                case '(':
                    for (i++; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != ')'); i++)
                        ;
                    break;

                // C & C++ style comments
                case '/':
                    // block comment
                    if (aSrcQstr.charAt(i + 1) == '*')
                    {
                        for (i += 2; i < aSrcQstr.length(); i++)
                        {
                            if ((aSrcQstr.charAt(i) == '*') && (aSrcQstr.charAt(i + 1) == '/'))
                            {
                                i++;
                                break;
                            }
                        }
                    }
                    // line comment
                    else if (aSrcQstr.charAt(i + 1) == '/')
                    {
                        for (i += 2; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\n'); i++)
                            ; // skip to end-of-line
                    }
                    break;

                // line comment
                case '-':
                    if (aSrcQstr.charAt(i + 1) == '-')
                    {
                        for (i += 2; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\n'); i++)
                            ; // skip to end-of-line
                    }
                    break;

                case 'F':
                case 'f':
                    // {공백}FROM{공백} 인지 확인
                    if ((i == 0) || (i + 5 >= aSrcQstr.length()) || !isValidFromPrevChar(aSrcQstr.charAt(i - 1)))
                    {
                        break;
                    }

                    if (StringUtils.startsWithIgnoreCase(aSrcQstr, i, "FROM") &&
                        isValidFromNextChar(aSrcQstr.charAt(i + 4)))
                    {
                        return i;
                    }
                    break;
                default:
                    break;
            }
        }
        return INDEX_NOT_FOUND;
    }

    /**
     * FROM 다음에 나올 수 있는 문자인지 확인한다.
     * 
     * @param aCh 확인할 문자
     * @return FROM 다음에 나올 수 있는 문자인지 여부
     */
    private static boolean isValidFromNextChar(char aCh)
    {
        return (Character.isWhitespace(aCh) || aCh == '"' || aCh == '/' || aCh == '(' || aCh == '-');
    }

    /**
     * FROM 앞에 나올 수 있는 문자인지 확인한다.
     * 
     * @param aCh 확인할 문자
     * @return FROM 앞에 나올 수 있는 문자인지 여부
     */
    private static boolean isValidFromPrevChar(char aCh)
    {
        return (Character.isWhitespace(aCh) || aCh == '\'' || aCh == '"' || aCh == '/' || aCh == ')');
    }

    /**
     * FROM의 끝 위치를 찾는다.
     * 
     * @param aSql 원본 쿼리문
     * @return FROM 절이 있으면 FROM의 끝 위치(0 base), 아니면 -1
     * @throws SQLException 
     */
    private static int indexOfFromEnd(String aSql) throws SQLException
    {
        int sIdxFromB = indexOfFrom(aSql);
        if (sIdxFromB == INDEX_NOT_FOUND)
        {
            return INDEX_NOT_FOUND;
        }
        else
        {
            return indexOfFromEnd(aSql, sIdxFromB);
        }
    }

    /**
     * FROM의 끝 위치를 찾는다.
     * 
     * @param aSql 원본 쿼리문
     * @param aIndexOfFrom FROM의 시작 위치(0 base)
     * @return FROM의 끝 위치(0 base)
     */
    private static int indexOfFromEnd(String aSql, int aIndexOfFrom)
    {
        // find index of from end
        int sIdxFromE = aIndexOfFrom + 4;
        for (; sIdxFromE < aSql.length() && Character.isWhitespace(aSql.charAt(sIdxFromE)); sIdxFromE++)
            /* skip white space */;
        for (; sIdxFromE < aSql.length() && !Character.isWhitespace(aSql.charAt(sIdxFromE)); sIdxFromE++)
            /* skip to end of table name */;

        if (sIdxFromE < aSql.length() && aSql.charAt(sIdxFromE) != ';')
        {
            int sIdxTokB = sIdxFromE;
            for (; sIdxTokB < aSql.length() && Character.isWhitespace(aSql.charAt(sIdxTokB)); sIdxTokB++)
                /* skip white space */;
            int sIdxTokE = sIdxTokB;
            for (; sIdxTokE < aSql.length() && !Character.isWhitespace(aSql.charAt(sIdxTokE)); sIdxTokE++)
                /* skip to end of table name */;
            int sTokLen = sIdxTokE - sIdxTokB;
            switch (sTokLen)
            {
                case 2:
                    if (StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "AS"))
                    {
                        sIdxFromE = sIdxTokE;
                        for (; sIdxFromE < aSql.length() && Character.isWhitespace(aSql.charAt(sIdxFromE)); sIdxFromE++)
                            /* skip white space */;
                        for (; sIdxFromE < aSql.length() && !Character.isWhitespace(aSql.charAt(sIdxFromE)); sIdxFromE++)
                            /* skip to end of alias name */;
                    }
                    break;
                case 3:
                    if (!StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "FOR"))
                    {
                        sIdxFromE = sIdxTokE;
                    }
                    break;
                case 4:
                    if (!StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "FOR"))
                    {
                        sIdxFromE = sIdxTokE;
                    }
                    break;
                case 5:
                    if (!StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "WHERE") &&
                        !StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "GROUP") &&
                        !StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "UNION") &&
                        !StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "MINUS") &&
                        !StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "START") &&
                        !StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "ORDER") &&
                        !StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "LIMIT"))
                    {
                        sIdxFromE = sIdxTokE;
                    }
                    break;
                case 6:
                    if (!StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "HAVING"))
                    {
                        sIdxFromE = sIdxTokE;
                    }
                    break;
                case 7:
                    if (!StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "CONNECT"))
                    {
                        sIdxFromE = sIdxTokE;
                    }
                    break;
                case 9:
                    if (!StringUtils.startsWithIgnoreCase(aSql, sIdxTokB, "INTERSECT"))
                    {
                        sIdxFromE = sIdxTokE;
                    }
                    break;
                default:
                    sIdxFromE = sIdxTokE;
                    break;
            }
        }
        return sIdxFromE;
    }

    public static final int KEY_SET_ROWID_COLUMN_INDEX = 1;

    public static String makeKeySetSql(String aSql, Map aOrderByMap) throws SQLException
    {
        if (!isSelectQuery(aSql))
        {
            return aSql;
        }

        int sIdxFrom = indexOfFrom(aSql);
        int sIdxOrderList = indexOfOrderByList(aSql, sIdxFrom);
        StringBuffer sBuf = new StringBuffer(aSql.length());
        // 다음 SQL문에 의해 KEY_SET_ROWID_COLUMN_INDEX가 1로 고정됨.
        sBuf.append("SELECT _PROWID ");
        if (sIdxOrderList == INDEX_NOT_FOUND || aOrderByMap == null || aOrderByMap.size() == 0)
        {
            sBuf.append(aSql.substring(sIdxFrom));
        }
        else
        {
            sBuf.append(aSql.substring(sIdxFrom, sIdxOrderList));
            boolean sNeedComma = false;
            for (int i = sIdxOrderList; i < aSql.length(); i++)
            {
                i = indexOfNonWhitespaceAndComment(aSql, i);
                char sCh = aSql.charAt(i);
                String sOrderBy;
                // value
                if (sCh == '\'')
                {
                    int sStartIdx = i;
                    for (i++; i < aSql.length(); i++)
                    {
                        if (aSql.charAt(i) == '\'' && (i + 1 == aSql.length() || aSql.charAt(i + 1) != '\''))
                        {
                            break;
                        }
                    }
                    i++;
                    sOrderBy = aSql.substring(sStartIdx, i);
                }
                // quoted identifier
                else if (sCh == '"')
                {
                    int sStartIdx = i + 1;
                    for (i++; i < aSql.length() && aSql.charAt(i) != '"'; i++)
                        ;
                    sOrderBy = aSql.substring(sStartIdx, i);
                    i++;
                }
                // column position
                else if (Character.isDigit(sCh))
                {
                    int sStartIdx = i;
                    for (i++; i < aSql.length() && Character.isDigit(aSql.charAt(i)); i++)
                        ;
                    sOrderBy = aSql.substring(sStartIdx, i);
                }
                // column name, alias, ...
                else if (isSQLIdentifierStart(sCh))
                {
                    int sStartIdx = i;
                    for (i++; i < aSql.length() && isSQLIdentifierPart(aSql.charAt(i)); i++)
                        ;
                    // non-quoted identifier는 UPPERCASE로
                    sOrderBy = aSql.substring(sStartIdx, i).toUpperCase();
                }
                else
                {
                    throw new AssertionError("Invalid query string");
                }
                String sColNameKey = sOrderBy;
                // order by 절에는 순번, alias, column name 외 다른것도 올 수 있다.
                String sBaseColumnName = (String)aOrderByMap.get(sColNameKey);
                if (sBaseColumnName != null)
                {
                    sOrderBy = "\"" + sBaseColumnName + "\"";
                }
                if (sNeedComma)
                {
                    sBuf.append(',').append(sOrderBy);
                }
                else
                {
                    sBuf.append(sOrderBy);
                    sNeedComma = true;
                }
                i = indexOfNonWhitespaceAndComment(aSql, i);
                if (i == INDEX_NOT_FOUND)
                {
                    break;
                }
                if (StringUtils.startsWithIgnoreCase(aSql, i, "ASC") && isValidNextCharForOrderByList(aSql, i+3))
                {
                    sBuf.append(" ASC");
                    i += 3;
                }
                else if (StringUtils.startsWithIgnoreCase(aSql, i, "DESC") && isValidNextCharForOrderByList(aSql, i+4))
                {
                    sBuf.append(" DESC");
                    i += 4;
                }
                i = indexOfNonWhitespaceAndComment(aSql, i);
                if (i == INDEX_NOT_FOUND)
                {
                    break;
                }
                // ,가 없으면 ORDER BY절이 끝난 것
                if (aSql.charAt(i) != ',')
                {
                    sBuf.append(aSql.substring(i));
                    break;
                }
            }
        }
        return sBuf.toString();
    }

    private static boolean isValidNextCharForOrderByList(String aSql, int i)
    {
        if (i >= aSql.length())
        {
            return true;
        }
        char sCh = aSql.charAt(i);
        if (Character.isWhitespace(sCh))
        {
            return true;
        }
        switch (sCh)
        {
            case '-': // comment
            case '/': // C/C++ style comment
            case ',':
                return true;
        }
        return false;
    }

    private static boolean isSQLIdentifierStart(char aCh)
    {
        return ('a' <= aCh && aCh <= 'z') || ('A' <= aCh && aCh <= 'Z') || aCh == '_';
    }

    private static boolean isSQLIdentifierPart(char aCh)
    {
        return isSQLIdentifierStart(aCh) || ('0' <= aCh && aCh <= '9');
    }

    private static int indexOfOrderByList(String aSrcQstr, int aStartIdx)
    {
        for (int i = aStartIdx; i < aSrcQstr.length(); i++)
        {
            switch (aSrcQstr.charAt(i))
            {
                // ' ~ '
                case '\'':
                    for (i++; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\''); i++)
                        ;
                    break;

                // " ~ "
                case '"':
                    for (i++; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '"'); i++)
                        ;
                    break;

                // ( ~ )
                case '(':
                    for (i++; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != ')'); i++)
                        ;
                    break;

                // C & C++ style comments
                case '/':
                    // block comment
                    if (aSrcQstr.charAt(i + 1) == '*')
                    {
                        for (i += 2; i < aSrcQstr.length(); i++)
                        {
                            if ((aSrcQstr.charAt(i) == '*') && (aSrcQstr.charAt(i + 1) == '/'))
                            {
                                i++;
                                break;
                            }
                        }
                    }
                    // line comment
                    else if (aSrcQstr.charAt(i + 1) == '/')
                    {
                        for (i += 2; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\n'); i++)
                            ; // skip to end-of-line
                    }
                    break;

                // line comment
                case '-':
                    if (aSrcQstr.charAt(i + 1) == '-')
                    {
                        for (i += 2; (i < aSrcQstr.length()) && (aSrcQstr.charAt(i) != '\n'); i++)
                            ; // skip to end-of-line
                    }
                    break;

                case 'O':
                case 'o':
                    // {공백}ORDER{공백}BY{공백} 인지 확인
                    if ((i == 0) || (i + 5 >= aSrcQstr.length()) || !isValidFromPrevChar(aSrcQstr.charAt(i - 1)))
                    {
                        break;
                    }

                    if (!StringUtils.startsWithIgnoreCase(aSrcQstr, i, "ORDER") ||
                        !Character.isWhitespace(aSrcQstr.charAt(i + 5)))
                    {
                        break;
                    }
                    int sOrgIdx = indexOfNonWhitespaceAndComment(aSrcQstr, i + 5);
                    for (i = sOrgIdx; Character.isWhitespace(aSrcQstr.charAt(i)); i++)
                        /* skip white space */;
                    if (!StringUtils.startsWithIgnoreCase(aSrcQstr, i, "BY") ||
                        !isValidFromNextChar(aSrcQstr.charAt(i + 2)))
                    {
                        i = sOrgIdx;
                        break;
                    }
                    return i + 3;

                default:
                    break;
            }
        }
        return INDEX_NOT_FOUND;
    }

    public static String makeRowSetSql(String aSql, int aRowIdCount) throws SQLException
    {
        if (aSql.indexOf(INTERNAL_SQL_PROWID) != INDEX_NOT_FOUND)
        {
            Error.throwInternalError(ErrorDef.ALREADY_CONVERTED);
        }

        int sIdxSelectB = indexOfSelect(aSql);
        if (sIdxSelectB == INDEX_NOT_FOUND)
        {
            return aSql;
        }

        char[] sSqlChArr = aSql.toCharArray();
        StringBuffer sBuf = new StringBuffer(aSql.length());
        int sIdxFromB = indexOfFrom(aSql, sIdxSelectB);
        int sIdxFromE = indexOfFromEnd(aSql, sIdxFromB);
        sBuf.append(sSqlChArr, sIdxSelectB, sIdxFromB - sIdxSelectB);
        sBuf.append(INTERNAL_SQL_PROWID);
        sBuf.append(sSqlChArr, sIdxFromB, sIdxFromE - sIdxFromB);
        sBuf.append(" WHERE _PROWID IN (?");
        for (sIdxFromE = 2; sIdxFromE <= aRowIdCount; sIdxFromE++)
        {
            sBuf.append(",?");
        }
        sBuf.append(")");
        return sBuf.toString();
    }

    /**
     * _PROWID 컬럼이 추가된 쿼리문을 얻는다.
     * 
     * @param aSql 원본 쿼리문
     * @return 단순 SELECT문이 아니면 null,
     *         이미 _PROWID 컬럼이 추가되어있으면 원본 쿼리문,
     *         _PROWID 컬럼이 추가되지않은 단순 SELECT 문이면 _PROWID 컬럼이 추가된 쿼리문
     * @throws SQLException 
     */
    public static String makePRowIDAddedSql(String aSql) throws SQLException
    {
        int sIdxSelectB = indexOfSelect(aSql);
        if (sIdxSelectB == INDEX_NOT_FOUND)
        {
            return null; // 변환할 수 없는 쿼리문인지 여부를 확인하기 위함
        }

        if (aSql.indexOf(INTERNAL_SQL_PROWID) != INDEX_NOT_FOUND)
        {
            return aSql;
        }

        int sIndex = indexOfFrom(aSql, sIdxSelectB);
        return aSql.substring(sIdxSelectB, sIndex)
               + INTERNAL_SQL_PROWID
               + aSql.substring(sIndex, aSql.length());
    }

    /**
     * SELECT의 시작 위치를 찾는다.
     * 
     * @param aSql 원본 쿼리문
     * @return SELECT로 시작하면 SELECT의 시작 위치(0 base), 아니면 -1
     */
    private static int indexOfSelect(String aSql)
    {
        int sStartPos = indexOfNonWhitespaceAndComment(aSql);
        if (StringUtils.startsWithIgnoreCase(aSql, sStartPos, "SELECT"))
        {
            return sStartPos;
        }
        return INDEX_NOT_FOUND;
    }

    /**
     * SELECT로 시작하는 쿼리인지 확인한다.
     * 
     * @param aSql 확인할 쿼리문
     * @return SELECT로 시작하는지 여부
     */
    public static boolean isSelectQuery(String aSql)
    {
        int sIdxSelectB = indexOfSelect(aSql);
        return (sIdxSelectB != INDEX_NOT_FOUND);
    }

    /**
     * INSERT로 시작하는 쿼리인지 확인한다.
     * 
     * @param aSql 확인할 쿼리문
     * @return INSERT로 시작하는지 여부
     */
    public static boolean isInsertQuery(String aSql)
    {
        int sStartPos = indexOfNonWhitespaceAndComment(aSql);
        return StringUtils.startsWithIgnoreCase(aSql, sStartPos, "INSERT");
    }

    public static String makeDeleteRowSql(String aTableName)
    {
        return "DELETE FROM " + aTableName + " WHERE _PROWID=?";
    }
}
