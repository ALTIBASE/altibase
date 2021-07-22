# $Id: E_MT_US7ASCII.msg 29282 2008-11-13 08:03:38Z mhjeong $

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# Last Number = 106

SECTION = 2

###############################################################################
#########                  MT GENERAL ERROR CODE                      #########
###############################################################################

0,mtERR_IGNORE_NOERROR = Ignore this message.
# *Cause: This is not an error.
# *Action: Ignore this message.

1,mtERR_FATAL_NOT_APPLICABLE = Not applicable
# *Cause: This error occurs due to a logical programming error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

2,mtERR_ABORT_NOT_APPLICABLE = Not applicable
# *Cause: This error occurs due to a logical programming error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

3,mtERR_FATAL_MEMORY_SHORTAGE = Out of memory
# *Cause: Out of memory
# *Action: Verify that the system has sufficient memory.

4,mtERR_FATAL_CONVERSION_COLLISION = Two or more conversions <0%s> are defined for two data types.
# *Cause: This is a data type conversion error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

5,mtERR_FATAL_INCOMPATIBLE_TYPE = Incompatible data type <0%s>.
# *Cause: Incompatible data type
# *Action: Check the compatibility of the data types.

6,mtERR_FATAL_COLUMN_EXCEED = Column exceed.
# *Cause: The conversion path between MT data types exceeds the buffer size.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

7,mtERR_ABORT_LANGUAGE_MODULE_NOT_FOUND = Language module <0%s> not found.
# *Cause: The language module was not found.
# *Action: Check the language.

8,mtERR_ABORT_DATATYPE_MODULE_NOT_FOUND = Data type module <0%s> not found.
# *Cause: The data type module was not found.
# *Action: Check the data type.

9,mtERR_ABORT_CONVERSION_MODULE_NOT_FOUND = Conversion module <0%s> not found.
# *Cause: The conversion module was not found.
# *Action: Check the compatibility of the data types.

10,mtERR_ABORT_FUNCTION_MODULE_NOT_FOUND = Function module <0%s> not found.
# *Cause: The function module was not found.
# *Action: Use the correct function name.

11,mtERR_ABORT_INVALID_FUNCTION_ARGUMENT = Invalid number of arguments for a function.
# *Cause: The number of arguments for the function was invalid.
# *Action: Check the number of arguments required by the function.

12,mtERR_ABORT_CONVERSION_NOT_APPLICABLE = Conversion not applicable.
# *Cause: The conversion is not applicable.
# *Action: Check the compatibility of the data types.

13,mtERR_ABORT_INVALID_LENGTH = Invalid data type length
# *Cause: The length of the data type was invalid.
# *Action: Check the length of the data type.

14,mtERR_ABORT_INVALID_PRECISION = Invalid data type precision
# *Cause: The precision of the data type was invalid.
# *Action: Check the precision of the data type.

15,mtERR_ABORT_INVALID_SCALE = Invalid data type scale
# *Cause: The scale of the data type was invalid.
# *Action: Check the scale of the data type.

16,mtERR_ABORT_VALUE_OVERFLOW = Value overflow
# *Cause: Value overflow
# *Action: Change the value or data type.

17,mtERR_ABORT_INVALID_LITERAL = Invalid literal
# *Cause: Invalid literal.
# *Action: Check the constant of the data type.

19,mtERR_ABORT_STACK_OVERFLOW = Calculation stack overflow
# *Cause: Calculation stack overflow.
# *Action: Alter Alter the calculation stack size using the ALTER SESSION statement.

20,mtERR_ABORT_NOT_AGGREGATION = The function is not an aggregate function.
# *Cause: The function is not an aggregate function.
# *Action: Remove the ALL or DISTINCT keyword.

22,mtERR_ABORT_DIVIDE_BY_ZERO = Division by zero
# *Cause: An attempt was made to divide a number by zero.
# *Action: Find and remove the attempt to divide a number by zero.

23, mtERR_ABORT_ARGUMENT_NOT_APPLICABLE = The argument is not applicable. 
# *Cause: The argument is not applicable to the function.
# *Action: Verify that the argument falls within the valid range.

24, mtERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE = The specified object type is currently not supported.
# *Cause: Unsupported object type
# *Action: For geometry types, only the POINT type is currently supported.

25, mtERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE = Inapplicable object type
# *Cause: The specified object type is not applicable to the function.
# *Action: Check the type of the object.

26, mtERR_ABORT_INVALID_WKT = Error parsing well-known-text
# *Cause: The specified well-known-text is not correct.
# *Action: Check the well-known-text.

27, mtERR_ABORT_TO_CHAR_MAX_PRECISION = The value exceeds the maximum precision ( <0%d> ) of the format.
# *Cause: The maximum precision of the format has been exceeded.
# *Action: Check the size of the format string.

28, mtERR_ABORT_VALIDATE_INVALID_VALUE = Invalid data value
# *Cause: The value of the data exceeds the logical value scope.
# *Action: Check the data value.

29, mtERR_ABORT_VALIDATE_INVALID_LENGTH = Invalid data length
# *Cause: The length of the data exceeds the valid scope.
# *Action: Check the length of the data.

30, mtERR_ABORT_CODING_INVALID_FMT = Invalid coding format
# *Cause: The compiled coding format is not valid.
# *Action: Check the compiled format.

31, mtERR_ABORT_CODING_DATA_FMT_MISMATCH = Mismatched data and format
# *Cause: The data string mismatches the specified format.
# *Action: Check the data string.

32, mtERR_ABORT_INVALID_LITERAL_AFTER_ESCAPE = Missing or invalid literal following the escape character
# *Cause: The escape character must be followed by either '%' or '_'.
# *Action: Check the LIKE predicate.

33, mtERR_ABORT_INVALID_ESCAPE = Invalid escape literal
# *Cause: The length of the escape literal is greater than one. The escape literal is invalid.
# *Action: Check the escape character of the LIKE predicate.

42, mtERR_ABORT_NOT_SUPPORTED_YET = Not supported yet.
# *Cause: The digest algorithm is not supported yet.
# *Action: No action is necessary.

43, mtERR_ABORT_INVALID_DIGEST_ALGORITHM = Invalid digest algorithm.
# *Cause: The digest algorithm name is unknown.
# *Action: Check the second argument of the digest function.

44, mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE = The argument '<0%d>' is out of range.
# *Cause: The argument value is out of range.
# *Action: Check the argument value.

45, mtERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_SECOND = The interval between startdate and enddate exceeded 68 years.
# *Cause: If the date field name is 'SECOND', the interval between startdate and enddate must be less than 68 years.
# *Action: Check the startdate and enddate values.

46, mtERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND = The interval between startdate and enddate exceeded 30 days.
# *Cause: If the date field name is 'MICROSECOND', the interval between startdate and enddate must be less than 30 days.
# *Action: Check the startdate and enddate values.

47, mtERR_ABORT_INVALID_SIZE_OF_SECOND_AND_MICROSECOND = The value of SSSSSSSS must be an eight-digit number.
# *Cause: The value of SSSSSSSS has less than eight digits.
# *Action: Check the value of SSSSSSSS.

48, mtERR_ABORT_INVALID_CHARACTER = Invalid character use.
# *Cause: An invalid character is being used.
# *Action: Verify that every character in the input string is a valid character.

49, mtERR_ABORT_CONVERSION_DISABLE = Unable to convert the data type.
# *Cause: Unable to convert the data type.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

65, mtERR_ABORT_ONLY_USE_DEC_OR_HEX_IN_DUMP_FUNC = Only a decimal or hexadecimal number is allowed. 
# *Cause: Only a decimal or hexadecimal number is allowed.
# *Action: Check the number.

66, mtERR_ABORT_INVALID_U_TYPE_STRING = '\' must be followed by four hexadecimal characters.
# *Cause: '\' must be followed by four hexadecimal characters.
# *Action: Fix the string.

75, mtERR_ABORT_INVALID_STORED_DATA_LENGTH = The data saved in the DBMS are not the expected length.
# *Cause: The disk page is broken or the meta table has invalid data.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

76,mtERR_ABORT_ART= Error generated by Automatic Recovery Test(ART)
# *Cause:  Ignore.
# *Action: Ignore.

105,mtERR_ABORT_INVALID_LENGTH_COLUMN = Invalid data type length : <0%s>.
# *Cause:The data length has been exceeded.
# *Action: Verify that the length of the data is correct.

###############################################################################
#########                  MT DATE ERROR CODE                         #########
###############################################################################

34, mtERR_ABORT_INVALID_DATE = Invalid date literal
# *Cause: The character to be converted to the date type is invalid.
# *Action: Check the arguments in the date conversion function.

35, mtERR_ABORT_INVALID_YEAR = The year is invalid or out of range.
# *Cause: The year part of the date literal is invalid or out of range.
# *Action: Check the arguments in the date conversion function.

36, mtERR_ABORT_INVALID_MONTH = The month value must be from 1 to 12.
# *Cause: The month part of the date literal is invalid or out of range.
# *Action: Check the arguments in the date conversion function.

37, mtERR_ABORT_INVALID_DAY = Invalid day of the month
# *Cause: The day part of the date literal is invalid or out of range.
# *Action: Check the arguments in the date conversion function.

38, mtERR_ABORT_INVALID_HOUR = The hour value must be between 1 and 12 inclusive.
# *Cause: An invalid hour was specified for a date using the 12-hour time format.
# *Action: Enter an hour value between 1 and 12.

39, mtERR_ABORT_INVALID_MINUTE = The minutes value must be between 0 and 59 inclusive.
# *Cause: The minutes part of the date literal is invalid or out of range.
# *Action: Check the arguments in the date conversion function.

40, mtERR_ABORT_INVALID_SECOND = The seconds value must be between 0 and 59 inclusive.
# *Cause: The seconds part of the date literal is invalid or out of range.
# *Action: Check the arguments in the date conversion function.

41, mtERR_ABORT_INVALID_MICROSECOND = The microseconds value must be between 0 and 999999 inclusive.
# *Cause: The microseconds part of the date literal is invalid or out of range.
# *Action: Check the arguments in the date conversion function.

50, mtERR_ABORT_DATE_NOT_ENOUGH_INPUT = The input literal is not long enough for the date format.
# *Cause: The data to be converted to the date format was incomplete; the date format picture was longer than the input data.
# *Action: Either add more input or shorten the date format picture, then retry the operation.

51, mtERR_ABORT_DATE_NOT_ENOUGH_FORMAT = The date format picture ends before the entire input string was converted.
# *Cause: The first part of the format picture was converted into a valid date, but the remaining data was not required.
# *Action: Check the specifications for date format pictures and correct the statement.

52, mtERR_ABORT_DATE_INVALID_HOUR24 = Hours must be between 0 and 23 inclusive.
# *Cause: An invalid hour value was specified for a date.
# *Action: Enter an hour value between 0 and 23 inclusive.

53, mtERR_ABORT_DATE_INVALID_DAY_OF_YEAR = The day of the year must be between 1 and 365 (366 for leap year) inclusive.
# *Cause: An invalid day of the year was specified in a date. The day of the year (DDD) must be between 1 and 365 inclusive for a non-leap year or 1 and 366 inclusive for a leap year.
# *Action: Enter a day of the year value between 1 and 365 (or 366) inclusive.

54, mtERR_ABORT_DATE_INVALID_SEC_IN_DAY = The seconds value in the date must be between 0 and 86399 inclusive.
# *Cause: An invalid value for the number of seconds was specified in a date.
# *Action: Specify a seconds value between 0 and 86399 inclusive.

55, mtERR_ABORT_DATE_CONFLICT_FORMAT = The same (purpose) format element(s) is (are) used twice or more.
# *Cause: The same (purpose) format element is listed more than once in a date specification.
# *Action: Check the format code.

56, mtERR_ABORT_DATE_LITERAL_MISMATCH = Literals in the input do not match the format string.
# *Cause: Literals in the input must be the same length as literals in the format string (with the exception of leading whitespaces). If the "FX" modifier has been toggled on, the literal must match exactly, with no extra whitespace.
# *Action: Correct the format string to match the literal.

57, mtERR_ABORT_DATE_NOT_RECOGNIZED_FORMAT = The date format ( <0%s> ) was not recognized.
# *Cause: A date specification contained an invalid format code.
# *Action: Check that only valid date format codes are used.

58, mtERR_ABORT_DATE_NON_NUMERIC_INPUT = A non-numeric character was found where a numeric character was expected.
# *Cause: The input data to be converted using the date format model was incorrect. The input data contained a non-number where a number was required by the format model.
# *Action: Fix the input data or the date format model so that they match each other in number and type. Then retry the operation.

59, mtERR_ABORT_DATE_SSSSS_CONFLICT_SS = The seconds-of-minute value conflicts with the seconds-in-day value.
# *Cause: A date specification contained both seconds of the minute and seconds in the day, but the values did not correspond. If both types of seconds are specified, the seconds of the minute value (SS) must correspond to the second portion of the seconds past midnight value (SSSSS).
# *Action: Remove the seconds-of-minute value from the date specification or enter a value that corresponds to the given seconds-in-day value.

60, mtERR_ABORT_DATE_SSSSS_CONFLICT_MI = The minutes-of-hour value conflicts with the seconds-in-day value.
# *Cause: A date specification contained both minutes of the hour and seconds in the day, but the values did not correspond. If both minutes in the hour (MI) and seconds past midnight (SSSSS) are specified, the minutes value must be the minute in which the seconds value falls.
# *Action: Remove the minutes value from the date specification or enter the correct minute value for the specified seconds value.

61, mtERR_ABORT_DATE_SSSSS_CONFLICT_HH = The hour value conflicts with the seconds-in-day value.
# *Cause: A date specification contained both an hour value and a seconds in the day value, but the values did not correspond. If an hour is specified together with a seconds-past-midnight (SSSSS) value, it must be the hour in which the seconds value falls.
# *Action: Remove the hour value from the date specification or specify the correct seconds-past-midnight value.

62, mtERR_ABORT_DATE_DDD_CONFLICT_DD = Day of month conflicts with day of year.
# *Cause: A date containing the day of the month was specified, but the day-of-the-month value did not correspond to the day-of-the-year value. If the day-of-the-month value is specified as part of a date, it must be the same day specified in the day-of-the-year value.
# *Action: Remove the day-of-the-month value from the date specification or enter the correct day-of-the-year value.

63, mtERR_ABORT_DATE_DDD_CONFLICT_MM = The month value conflicts with the day-of-year value.
# *Cause: The wrong month was specified in a date. If a month is specified as part of a date, it must be the month in which the date occurs.
# *Action: Remove the month value from the date specification or enter the correct date value.

64, mtERR_ABORT_DATE_DAY_CONFLICT = A day-of-week value conflicts with a date value.
# *Cause: A day-of-the-week value was specified as part of a date, but the day of the week did not correspond to the specified date value. If the day of the week is specified with a date, it must be the same day of the week as the date.
# *Action: Remove the day-of-the-week value from the date specification or enter a date value corresponding to the correct day of the week.

67, mtERR_ABORT_UNEXPECTED_ERROR = Unexpected errors may have occurred: <0%s>: <1%s>
# *Cause: This error occurs due to a logical programming error. 
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

68, mtERR_FATAL_THR_MUTEXINIT = Failed to invoke the mutex_init() system function
# *Cause: The system failed to initialize a mutex.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

69, mtERR_FATAL_THR_MUTEXLOCK = Failed to invoke the mutex_lock() system function
# *Cause: The system failed to lock a mutex.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

70, mtERR_FATAL_THR_MUTEXUNLOCK = Failed to invoke the muntex_unlock() system function
# *Cause: The system failed to unlock a mutex.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

71, mtERR_ABORT_NULL_VALUE = No NULL value is allowed to be used as the data type.
# *Cause: No NULL value is allowed to be used as the data type.
# *Action: Verify that there is no NULL value in the field.

72, mtERR_ABORT_OVERFLOW = Out of range of value supported by the type
# *Cause: Out of range of value supported by the type
# *Action: Change the input value to be in the range of value supported by the given type.

73, mtERR_ABORT_INVALID_NUMERIC = Unable to cast the string to INTEGER type
# *Cause: It is impossible to cast the string to the INTEGER type.
# *Action: Change the string appropriately.

74, mtERR_ABORT_REACH_END = Unexpected end of string
# *Cause: Unexpected end of string
# *Action: Verify that the form of the string is legal.

77, mtERR_ABORT_WRONG_PATTERN = Invalid pattern string
# *Cause: A search attempt was made using an invalid pattern string.
# *Action: Verify that the pattern string is valid.

78, mtERR_ABORT_TZ_REGION_NOT_FOUND = Time zone region not found
# *Cause: The time zone region could not be found.
# *Action: Check the time zone region in the V$TIME_ZONE_NAMES performance view.

79, mtERR_ABORT_TZ_FMT_MISMATCH = Time zone format string mismatch
# *Cause: The string literal does not match the format string.
# *Action: Check the time zone format string.

80, mtERR_ABORT_TZ_HOUR_OUT_OF_RANGE = Time zone offset out of range
# *Cause: The time zone hour (offset) was less than -12 or greater than 14.
# *Action: Set the time zone hour to a value between -12 and 14 (inclusive).
 
81, mtERR_ABORT_TZ_LENGTH_EXCEED = Time zone string too long
# *Cause: The length of the TIME_ZONE value exceeds the maximum length.
# *Action: Check the time zone string.

82, mtERR_ABORT_LONG_PATTERN = Pattern string too long
# *Cause: A search attempt was made using an excessively long pattern string.
# *Action: Verify that the size of the pattern string is less than 4KB.

83, mtERR_ABORT_REGEXP_REQUIRED_PAREN = The pattern requires parentheses, brackets, or braces.
# *Cause: The pattern requires parentheses, brackets, or braces.
# *Action: Check whether the pattern contains a closing parenthesis.
 
84, mtERR_ABORT_REGEXP_CLASS_EMPTY = The pattern contains an empty bracket.
# *Cause: The pattern contains an empty bracket.
# *Action: Check whether the pattern contains an empty bracket.
 
85, mtERR_ABORT_REGEXP_CONST_OVERFLOW = The pattern exceeds the maximum supported numeric range(10 digits).
# *Cause: The pattern exceeds the maximum supported numeric range(10 digits).
# *Action: Check whether the pattern contains a number with more than 10 digits.
  
86, mtERR_ABORT_REGEXP_REQUIRED_NUMBER = The pattern requires numbers({m} {m,} {m,n}: m and n must be numbers).
# *Cause: The pattern requires numbers({m} {m,} {m,n}: m and n must be numbers).
# *Action: Check whether valid numbers are input in the pattern format, {m} {m,} {m,n}.
 
87, mtERR_ABORT_REGEXP_REQUIRED_COMMA = The pattern requires braces or commas.
# *Cause: The pattern requires braces or commas.
# *Action: Check whether the pattern contains braces or commas.
 
88, mtERR_ABORT_REGEXP_UNEXPECTED_CAHR = The pattern contains an unexpected character.
# *Cause: The pattern contains an unexpected character.
# *Action: Verify that the pattern string is valid.
 
89, mtERR_ABORT_REGEXP_UNFINISHED_RANGE = The pattern contains an unfinished range.
# *Cause: The pattern contains an unfinished range.
# *Action: Verify that the pattern string is valid.
 
90, mtERR_ABORT_REGEXP_INVALID_RANGE = The pattern contains an invalid range.
# *Cause: The pattern contains an invalid range.
# *Action: Verify that the pattern format [A-Z] is valid.
 
91, mtERR_ABORT_REGEXP_CLASS_INVALID_CHAR = The pattern contains a predefined character class.
# *Cause: The pattern contains a predefined character class.
# *Action: Check whether the pattern contains a predefined character class.
 
92, mtERR_ABORT_REGEXP_LONG_PATTERN = The pattern's size exceeds 1024.
# *Cause: The pattern's size exceeds 1024.
# *Action: Verify that the size of the pattern string is less than 1KB.

93, mtERR_ABORT_CRYPT_PADDING_UNEXPECTED_CAHR = The padded text contains an unexpected padding character.
# *Cause: The padded text contains an unexpected padding character.
# *Action: Only a value returned by the PKCS7PAD16() function is valid for the PKCS7UNPAD() function.

97, mtERR_ABORT_TOO_LONG_CONCATENATION = result of string concatenation is too long.
# *Cause: String concatenation result is more than the maximum size.
# *Action: Make sure that the result is less than the maximum size.

98, mtERR_ABORT_INVALID_PERCENTILE_VALUE = The percentile value should be a number between 0 and 1.
# *Cause: A percentile value for PERCENTILE_CONT or PERCENTILE_DISC function is specified out of range.
# *Action: Specify a value from [0,1].

99, mtERR_ABORT_WITHIN_GORUP_COLUMN_OVERFLOW = The order by column of within group exceeds the maximum limit.
# *Cause: The column of the path exceeds the maximum limit.
# *Action: check order by column count.

100, mtERR_ABORT_MISSING_WITHIN_GROUP = WITHIN keyword required for this function.
# *Cause: The function requires the WITHIN keyword.
# *Action: Add the proper WITHIN keyword for the function.

101, mtERR_ABORT_NOT_ALLOWED_WITHIN_GROUP = A WITHIN keyword is not allowed here.
# *Cause: A WITHIN keyword is not allowed here.
# *Action: Remove the WITHIN keyword.

102, mtERR_ABORT_MEMORY_ALLOCATION = Insufficient memory for the Mathematics Module.
# *Cause: The memory manager could not allocate sufficient memory for the Mathematics Module.
# *Action: Verify that the system has sufficient memory.

103, mtERR_ABORT_INVALID_ENCODED_STRING = invalid encode string.
# *Cause: An error occurred while decoding the input string.
# *Action: Verify that source data is a valid encoded string.

104, mtERR_ABORT_SMTP_REPLY_ERROR = SMTP Reply Error: <0%s>
# *Cause: The SMTP server reply an error message.
# *Action: check an error message.

106, mtERR_ABORT_MAX_MEM_SIZE_EXCEED = The memory size allocated for the statement has exceeded the maximum limit ( Name : <0%s>, Wanted Memory Size : <1%lu>, Max size : <2%lu> ).
# *Cause: The memory size allocated for the statement has exceeded the maximum limit.
# *Action: Increase the XXXXXXX_MEMORY_MAXIMUM property value...

###############################################################################
#########       DCI ERROR CODE                              #########
###############################################################################
94, mtERR_IGNORE_CDBC_NOT_SUPPORTED = Unsupported data type.
# *Cause: Unsupported data type.
# *Action: Check your source code and use a valid data type.

95, mtERR_IGNORE_CDBC_SMALL_BUFFER = Buffer is smaller than the minimum size.
# *Cause: You allocated a buffer smaller than the minimum size.
# *Action: Check your source code and allocate a buffer of an appropriate size.

96, mtERR_IGNORE_CDBC_NULL_SCOLUMN = Argument value is null.
# *Cause: The value of the function argument is null.
# *Action: Check your source code and set a value for the function argument.

INTERNAL_TRACE_MESSAGE_BEGIN

MT_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
            "signed long =<%ld>\n unsigned long = <%lu>\n"
            "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";


INTERNAL_TRACE_MESSAGE_END


