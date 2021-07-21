# $Id: E_ST_US7ASCII.msg 16608 2006-06-09 02:35:01Z peh $

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# Last Number = 83

SECTION = 10

0,stERR_IGNORE_NOERROR = Ignore this message.
# *Cause: This is not an error.
# *Action: Ignore this message.

2,stERR_ABORT_NOT_APPLICABLE = Not applicable
# *Cause: This error occurs due to a logical programming error.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

3,stERR_FATAL_MEMORY_SHORTAGE = Out of memory
# *Cause: Out of memory
# *Action: Verify that the system has sufficient memory.

5,stERR_FATAL_INCOMPATIBLE_TYPE = Incompatible data type <0%s>.
# *Cause: Incompatible data type
# *Action: Check the compatibility between data types.

7,stERR_ABORT_LANGUAGE_MODULE_NOT_FOUND = Language module <0%s> not found.
# *Cause: The language module was not found.
# *Action: Check the language.

8,stERR_ABORT_DATATYPE_MODULE_NOT_FOUND = Data type module <0%s> not found.
# *Cause: The data type module was not found.
# *Action: Check the data type.

9,stERR_ABORT_CONVERSION_MODULE_NOT_FOUND = Conversion module <0%s> not found.
# *Cause: The conversion module was not found.
# *Action: Check the compatibility between data types.

10,stERR_ABORT_FUNCTION_MODULE_NOT_FOUND = Function module <0%s> not found.
# *Cause: The function module was not found.
# *Action: Use the correct function name.

11,stERR_ABORT_INVALID_FUNCTION_ARGUMENT = Invalid number of arguments for a function.
# *Cause: The number of arguments for the function was invalid.
# *Action: Check the number of arguments for the function.

12,stERR_ABORT_CONVERSION_NOT_APPLICABLE = Conversion not applicable.
# *Cause: The conversion is not applicable.
# *Action: Check the compatibility between data types.

13,stERR_ABORT_INVALID_LENGTH = Invalid length of the data type
# *Cause: Invalid length of the data type.
# *Action: Check the length of the data type.

14,stERR_ABORT_INVALID_PRECISION = Invalid precision of the data type
# *Cause: Invalid precision of the data type.
# *Action: Check the precision of the data type.

15,stERR_ABORT_INVALID_SCALE = Invalid scale of the data type
# *Cause: Invalid scale of the data type
# *Action: Check the scale of the data type.

16,stERR_ABORT_VALUE_OVERFLOW = Value overflow
# *Cause: Value overflow
# *Action: Change the value or data type.

17,stERR_ABORT_INVALID_LITERAL = Invalid literal
# *Cause: Invalid literal
# *Action: Check the constant indicating the data type.

19,stERR_ABORT_STACK_OVERFLOW = Calculation stack overflow
# *Cause: Calculation stack overflow
# *Action: Alter the calculation stack size using the ALTER SESSION statement.

20,stERR_ABORT_NOT_AGGREGATION = The function is not an aggregate function.
# *Cause: The function is not an aggregate function.
# *Action: Remove the ALL or DISTINCT keyword.

22,stERR_ABORT_DIVIDE_BY_ZERO = Division by zero
# *Cause: Division by zero
# *Action: Determine whether an attempt to divide a number by zero is being made.

23, stERR_ABORT_ARGUMENT_NOT_APPLICABLE = The argument is not applicable.
# *Cause: The argument is not applicable to the function.
# *Action: Change the argument so that it falls within the valid range.

24, stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE = The specified object type is not currently supported.
# *Cause: Unsupported object type
# *Action: For geometry types, only the POINT type is currently supported.

25, stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE = Inapplicable object type
# *Cause: The specified object type is not applicable to the function.
# *Action: Check the object type.

26, stERR_ABORT_INVALID_WKT = Error parsing well-known-text
# *Cause: The specified well-known-text is not correct.
# *Action: Check the well-known-text.

27, stERR_ABORT_TO_CHAR_MAX_PRECISION = The value exceeds the maximum precision ( <0%d> ) of the format.
# *Cause: Maximum precision of format exceeded.
# *Action: Check the size of format string.

28, stERR_ABORT_VALIDATE_INVALID_VALUE = Invalid data value
# *Cause: The value of the data exceeds the logical value scope.
# *Action: Check the data value.

29, stERR_ABORT_VALIDATE_INVALID_LENGTH = Invalid data length
# *Cause: The length of the data exceeds the valid scope.
# *Action: Check the length of the data.

30, stERR_ABORT_CODING_INVALID_FMT = Invalid coding format
# *Cause: The compiled coding format is not valid.
# *Action: Check the compiled format.

31, stERR_ABORT_CODING_DATA_FMT_MISMATCH = Mismatched data and format
# *Cause: The data string does not match the specified format.
# *Action: Check the data string.

32, stERR_ABORT_INVALID_LITERAL_AFTER_ESCAPE = Missing or invalid literal following the escape character
# *Cause: Either '%' or '_' must follow the escape character.
# *Action: Check the LIKE predicate.

33, stERR_ABORT_INVALID_ESCAPE = Invalid escape literal
# *Cause: The length of the escape is greater than one. The escape literal is invalid.
# *Action: Check the escape character in the LIKE predicate.

34, stERR_ABORT_INVALID_DATE = Invalid date literal
# *Cause: The text to be converted to a date type is invalid.
# *Action: Check the arguments for the date conversion function.

35, stERR_ABORT_INVALID_YEAR = Invalid year
# *Cause: The year part of the date literal is invalid or out of range.
# *Action: Check the arguments for the date conversion function.

36, stERR_ABORT_INVALID_MONTH = Invalid month
# *Cause: The month part of the date literal is invalid or out of range.
# *Action: Check the arguments for the date conversion function.

37, stERR_ABORT_INVALID_DAY = Invalid day
# *Cause: The day part of the date literal is invalid or out of range.
# *Action: Check the arguments for the date conversion function.

38, stERR_ABORT_INVALID_HOUR = Invalid hour
# *Cause: The hour part of the date literal is invalid or out of range.
# *Action: Check the arguments for the date conversion function.

39, stERR_ABORT_INVALID_MINUTE = Invalid minutes
# *Cause: The minutes part of the date literal is invalid or out of range.
# *Action: Check the arguments for the date conversion function.

40, stERR_ABORT_INVALID_SECOND = Invalid seconds
# *Cause: The seconds part of the date literal is invalid or out of range.
# *Action: Check the arguments for the date conversion function.

41, stERR_ABORT_INVALID_MICROSECOND = Invalid microseconds
# *Cause: The microseconds part of the date literal is invalid or out of range.
# *Action: Check the arguments for the date conversion function.

43, stERR_ABORT_INVALID_DIGEST_ALGORITHM = Invalid digest algorithm.
# *Cause: The digest algorithm name is unknown.
# *Action: Check the second argument on the digest function.

44, stERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE = The argument '<0%d>' is out of range.
# *Cause: The argument value is out of range.
# *Action: Check the argument value.

45, stERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_SECOND = The interval between startdate and enddate exceeded 68 years.
# *Cause: If the date field name is 'SECOND', the interval between startdate and enddate must be less than 68 years.
# *Action: Check the values of startdate and enddate.

46, stERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND = The interval between startdate and enddate exceeded 30 days.
# *Cause: If the date field name is 'MICROSECOND', the interval between startdate and enddate must be less than 30 days.
# *Action: Check the values of startdate and enddate.

47, stERR_ABORT_INVALID_SIZE_OF_SECOND_AND_MICROSECOND = The values of SSSSSSSS must be a number of eight digits.
# *Cause: The values of SSSSSSSS is smaller than eight digits.
# *Action: Check the value of SSSSSSSS.

48, stERR_ABORT_INVALID_CHARACTER = Invalid character use
# *Cause: An invalid character is being used.
# *Action: Verify that every character in the input string is a valid character.

49, stERR_FATAL_COLUMN_NOT_FOUND = Unable to find a column
# *Cause: Unable to find a column.
# *Action: Verify that the column being looked for is valid.

50, stERR_ABORT_TRAVERSE_NOT_APPLICABLE = Unable to traverse
# *Cause: The traverse could not be executed.
# *Action: Verify that the traverse is valid.

51, stERR_ABORT_INVALID_BYTE_ORDER = Invalid byte order information
# *Cause: Byte order is invalid.
# *Action: Verify the validity of the byte order.

52, stERR_ABORT_INVALID_FUNCTION_PRECISION = Invalid function precision
# *Cause: The function precision is not valid.
# *Action: Verify the validity of the function precision.

53, stERR_ABORT_INVALID_BUFFER_DISTANCE = Invalid distance value for the buffer function
# *Cause: The distance value for the buffer function is invalid.
# *Action: Verify the validity of the distance value for the buffer function.

54, stERR_ABORT_INVALID_RELATE_PATTERN = Invalid pattern of the relate function
# *Cause: The pattern value of the relate function is invalid
# *Action: Verify that values matching '*TF012' are set, and that the pattern length is 9.

55,stERR_ABORT_STNMR_DUMP_EMPTY_OBJECT = Empty dump object.
# *Cause: You did not specify a dump object for the dump table.
# *Action: Specify a dump object for the dump table.

56,stERR_ABORT_STNMR_INVALID_DUMP_OBJECT = Invalid dump object
# *Cause: The object is not valid for the dump table.
# *Action: Use a valid dump object for the dump table.

57,stERR_ABORT_OBJECT_BUFFER_OVERFLOW = Object buffer overflow.
# *Cause: Object buffer overflow.
# *Action: Use the ALTER SESSION/SYSTEM SET ST_OBJECT_BUFFER_SIZE statement or the ST_OBJECT_BUFFER_SIZE hint to increase the object buffer size.

58,stERR_ABORT_OBJECT_INTEGRITY_VIOLATION = Object integrity violation.
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

59,stERR_ABORT_RING_POINT_COUNT_LESS_THAN_4 = The ring (<0%d>) has less than 4 points
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

60,stERR_ABORT_NOT_CLOSED_RING = The ring (<0%d>) is not closed.
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

61,stERR_ABORT_OBJECT_SIZE = The size of the object is incorrect
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

62,stERR_ABORT_RING_BOUND_CROSS = The ring <0%d> and ring <1%d> bounds cross
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

63,stERR_ABORT_POLYGON_HAS_MULTI_EXTERNAL_RING = The external ring does not include the internal ring <0%d>,
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

64,stERR_ABORT_LINE_POINT_COUNT = The point count of a line is less than 2 
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

65,stERR_ABORT_LINE_POINT_SAME = A line has only two points with the same value
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

66,stERR_ABORT_RING_LINE_COUNT = A ring has less than three lines
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

67,stERR_ABORT_RING_ZERO_AREA = The area of a ring is zero
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

68,stERR_ABORT_RING_LINE_CROSS = A ring has crossing lines
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

69,stERR_ABORT_POLYGON_INTERSECTS = A multipolygon has intersecting polygons : (polygon:<0%d>, ring:<1%d>), (polygon:<2%d>, ring:<3%d>)
# *Cause: Object integrity violation.
# *Action: Verify that a valid object is being used.

70, stERR_ABORT_INVALID_WKB = Error parsing well-known-binary
# *Cause: The specified well-known-binary is not correct.
# *Action: Check the well-known-binary.

71, stERR_ABORT_INVALID_OBJECT_IN_GEOMCOLLECTION = The type <0%d> of object <1%d> in the geometry collection is not valid
# *Cause: The specified well-known-binary is not correct.
# *Action: Check the well-known-binary.

72, stERR_ABORT_INVALID_STORED_DATA_LENGTH = The data saved in the DBMS are not the expected length
# *Cause: The disk page is broken or the meta table has invalid data.
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

73, stERR_ABORT_INVALID_POLYGON = Invalid Polygon
# *Cause: An attempt was made to perform an operation on an invalid polygon.
# *Action: Check the structure of the polygon and try again.

74, stERR_ABORT_UNKNOWN_POLYGON = Unverified Polygon
# *Cause: An attempt was made to perform an operation on a polygon that has not been verified.
# *Action: Insert the Polygon again, or perform a validity check on the polygon.
 
75, stERR_ABORT_UNEXPECTED_ERROR = Unexpected error: <0%s>: <1%s> 
# *Cause: This error occurs due to a logical programming error. 
# *Action: Check the error number from the trace log and contact Altibase's Support Center (http://support.altibase.com).

76, stERR_ABORT_INVALID_POINTS = Invalid combination of identical points
# *Cause: Two or more points in the geometry object that must have different values have the same values.
# *Action: Ensure that the geometry object is valid.

77, stERR_ABORT_INVALID_GEOMETRY = Invalid Geometry
# *Cause: An attempt was made to perform an operation on an invalid geometry object.
# *Action: Ensure that the geometry object is valid.

78, stERR_ABORT_INVALID_SRID = The Spatial Reference ID(SRID) is incorrect.
# *Cause: The Spatial Reference ID(SRID) of the geometry column is different from the input SRID.
# *Action: Verify the input Spatial Reference ID(SRID).

79, stERR_ABORT_MIXED_SRID = Operation on mixed SRID geometries. (<0%d>: <1%d>)
# *Cause: SRIDs of the geometries are mixed.
# # *Action: Verify the input SRIDs

80, stERR_ABORT_UNKNOWN_SRID = Unknown Spatial Reference ID (<0%d>)
# *Cause: The Spatial Reference ID(SRID) was not found.
# *Action: Verify the input Spatial Reference ID(SRID).

81, stERR_ABORT_PROJ4_INIT_FAILED = Failed to initialize PROJ4 library (<0%s>, <1%d>, <2%d>)
# *Cause: Failed to initialize PROJ4 library.
# *Action: Verify the input arguments.

82, stERR_ABORT_PROJ4_TRANSFORM_FAILED = Failed to PROJ4 transform (<0%s>)
# *Cause: Failed to PROJ4 transform.
# *Action: Verify the input arguments.

83, stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB = Invalid Geometry(<0%s>) 
# *Cause: An attempt was made to perform an operation on an invalid geometry.
# *Action: Check the error number from the trace log and contact Altibase’s Support Center (http://support.altibase.com).

INTERNAL_TRACE_MESSAGE_BEGIN

ST_TRC_SAMPLE = "\n signed int = <%d>\n unsigned int = <%u>\n"
            "signed long =<%ld>\n unsigned long = <%lu>\n"
            "hex32 = <%x>\n hex64 = <%lx>\n char = <%c>\n string  = <%s>\n";


INTERNAL_TRACE_MESSAGE_END

