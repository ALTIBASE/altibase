#
# Apre 에러
#

# 에러코드 영역 번호
# 0 => id
# 1 => sm
# 2 => mt
# 3 => qp
# 4 => mm
# 5 => ul
# 6 => rp
# 7 => cm
# 9 => ut
# 10 => st
# 11 => lk
# 12 => dk

# 에러코드 할당 정책은 아래 위키 페이지를 참조하시기 바랍니다.
# http://wiki.altibase.in/wiki/index.php/Altimsg
# 브랜치에서 에러코드를 할당받으시면 안됩니다.
# 반드시 main trunk에서 번호를 할당받은 후 브랜치에서 사용하셔야 합니다.
# 의문 사항이 있으시면 AK팀 혹은 QP(품질본부)팀에 문의 부탁드립니다.
# Last Number = 126

SECTION = 5

############# EmSQL Precompiler ##############

### system error (101-199H) ###

0, 101H0, ulpERR_ABORT_FILE_OPEN_ERROR = Failed to open file: <0%s>, errno=<1%d>
# *Cause: Failed to open the file.
# *Action: Check the file path is valid and the client application has enough privileges.

1, 102H0, ulpERR_ABORT_COMP_Invaild_File_Size = FileSize (<0%s>) is zero.
# *Cause: The file size is zero.
# *Action: Check the file.

2, 103H0, ulpERR_ABORT_FILE_NOT_FOUND = File not found: <0%s>
# *Cause: File or directory does not exist.
# *Action: Check the path or create file or change the code.

3, 104H0, ulpERR_ABORT_FILE_DELETE_ERROR = Failed to delete file: <0%s>, errno=<1%u>
# *Cause: Failed to delete the file.
# *Action: Check the file path is valid and the client application has enough privileges.

4, 105H0, ulpERR_ABORT_Memory_Alloc_Error = Memory allocation error.
# *Cause: Memory is full or process heap corruption occurred.
# *Action: If this error occurs on the client, reduce the amount of memory being used by the client application.
#          If this error occurs on the server, verify that there is enough memory.

5, 106H0, ulpERR_ABORT_Latch_Init_Error = Latch initialize error. (<1%s>:<0%d> )
# *Cause: The system encountered an internal server error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

6, 107H0, ulpERR_ABORT_Latch_Read_Error = Latch read error. (<1%s>:<0%d> )
# *Cause: The system encountered an internal server error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

7, 108H0, ulpERR_ABORT_Latch_Release_Error = Latch release error. (<1%s>:<0%d> )
# *Cause: The system encountered an internal server error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

8, 109H0, ulpERR_ABORT_Latch_Write_Error = Latch write error. (<1%s>:<0%d> )
# *Cause: The system encountered an internal server error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

9, 110H0, ulpERR_ABORT_Latch_Destroy_Error = Latch destroy error. (<1%s>:<0%d>)
# *Cause: The system encountered an internal server error.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

10, 111H0, ulpERR_ABORT_FILE_CLOSE_ERROR = Failed to close file: <0%s>, errno=<1%d>
# *Cause: Failed to close the file.
# *Action: Check the operating system error number.

11, 112H0, ulpERR_ABORT_FILE_WRITE_ERROR = Failed to write file: <0%s>, errno=<1%u>
# *Cause: Failed to write the file.
# *Action: Check the file path is valid and the client application has enough privileges.


### C language / variable declaration error (201-299E) ###

12, 201E0, ulpERR_ABORT_COMP_C_Comment_No_Close_Error = C-type comment is not closed.
# *Cause: C-type Comment is not closed.
# *Action: Close C-type Comment.

13, 202E0, ulpERR_ABORT_COMP_C_Unknown_Structname_Error = The structure name [<0%s>] is unknown.
# *Cause: The structure name is unknown.
# *Action: Change the structure name.

14, 203E0, ulpERR_ABORT_COMP_C_Duplicate_Structname_Error = The structure name [<0%s>] is a duplicate.
# *Cause: The structure name is the same as an existing name.
# *Action: Change the structure name.

15, 204E0, ulpERR_ABORT_COMP_C_Add_Symbol_Error = The symbol name [<0%s>] cannot be added to the symbol table.
# *Cause: The symbol name cannot be added to the symbol table.
# *Action: Check the name of the symbol.

16, 205E0, ulpERR_ABORT_COMP_C_Exceed_Max_Id_Length_Error = The symbol name exceeds the maximum length: <0%s>
# *Cause: The symbol name is too long. Maximum length is 1023.
# *Action: Change the symbol name to a shorter name.

17, 206E0, ulpERR_ABORT_COMP_Symbol_Redefinition_Error = Redefinition of '<0%s>'.
# *Cause: The symbol has already been defined.
# *Action: Change the symbol name.

18, 207E0, ulpERR_ABORT_COMP_Type_Not_Exist_Error = Unknown type '<0%s>'.
# *Cause: The type is unknown.
# *Action: Use a known type.

19, 208E0, ulpERR_ABORT_COMP_Invalid_Scope_Depth_Error = Invalid scope depth: <0%d>
# *Cause: Braces are nested too deeply. Maximum depth is 100.
# *Action: Remove some braces to reduce the depth of nesting.

20, 209E0, ulpERR_ABORT_COMP_Brace_Count_Error = Inconsistent brace count.
# *Cause: The number of opening braces does not match the number of closing braces.
# *Action: Check the number of opening and closing braces.

21, 210E0, ulpERR_ABORT_COMP_Paren_Count_Error = Inconsistent parenthesis count error.
# *Cause: The number of opening parentheses does not match the number of closing parentheses.
# *Action: Check the number of opening and closing parentheses and try again.

22, 211E0, ulpERR_ABORT_COMP_Exceed_Max_Nested_Struct_Depth_Error = The nested structure exceeds the maximum possible depth.
# *Cause: The nested structure exceeds the maximum possible depth(100).
# *Action: Decrease the depth of the nested structure.

23, 212E0, ulpERR_ABORT_COMP_Varchar_In_IncludeFile_Error = VARCHAR declarations are not permitted in #include files.
# *Cause: A varchar declaration was used in an #include file.
# *Action: Remove the varchar declaration from the #include file. Consider using EXEC SQL INCLUDE instead.

24, 000H0, ulpERR_ABORT_COMP_Syntax_Error = Failed to compile with invalid syntax.
# *Cause: The invalid syntax exists in the *.sc file and could not compile the file.
# *Action: Verify the syntax of the *.sc file.

# embedded sql error (301-399L) ###

30, 301L0, ulpERR_ABORT_COMP_Emsqlstmt_Exist_in_CIncludefile_Error = The C include file can't contain embedded SQL statements.
# *Cause: The C include file can't contain embedded SQL statements.
# *Action: Delete the embedded SQL statement.

31, 302L0, ulpERR_ABORT_COMP_No_End_Declare_Section_Error = EXEC SQL END DECLARE SECTION does not exist.
# *Cause: The EXEC SQL END DECLARE SECTION does not exist.
# *Action: Check the EXEC SQL END DECLARE SECTION.

32, 303L0, ulpERR_ABORT_COMP_No_Begin_Declare_Section_Error = EXEC SQL BEGIN DECLARE SECTION does not exist.
# *Cause: The EXEC SQL BEGIN DECLARE SECTION does not exist.
# *Action: Check the EXEC SQL BEGIN DECLARE SECTION.

33, 304L0, ulpERR_ABORT_COMP_No_End_Arg_Section_Error = EXEC SQL END ARGUMENT SECTION does not exist.
# *Cause: The EXEC SQL BEGIN ARGUMENT SECTION does not exist.
# *Action: Check the EXEC SQL BEGIN ARGUMENT SECTION.

34, 305L0, ulpERR_ABORT_COMP_No_Begin_Arg_Section_Error = EXEC SQL BEGIN ARGUMENT SECTION does not exist.
# *Cause: The EXEC SQL BEGIN ARGUMENT SECTION does not exist.
# *Action: Check the EXEC SQL BEGIN ARGUMENT SECTION.

35, 306L0, ulpERR_ABORT_COMP_Unterminated_String_Error = Unterminated string error.
# *Cause: Unterminated string.
# *Action: Check that there is no unterminated string.

36, 307L0, ulpERR_ABORT_COMP_Exceed_Max_Connname_Error = Connection name [<0%s>] is too long. (max length is 50)
# *Cause: The connection name is too long.
# *Action: Check the connection name.

37, 308L0, ulpERR_ABORT_COMP_Exceed_Max_Cursorname_Error = Cursor name [<0%s>] is too long. (max length is 50)
# *Cause: The cursor name is too long.
# *Action: Check the cursor name.

38, 309L0, ulpERR_ABORT_COMP_Exceed_Max_Stmtname_Error = Statement name [<0%s>] is too long. (max length is 50)
# *Cause: The statement name is too long.
# *Action: Check the statement name.

39, 310L0, ulpERR_ABORT_COMP_FOR_iternum_Error = The number of FOR loop iterations must be greater than zero.
# *Cause: The number of FOR loop iterations was zero.
# *Action: Ensure that the number of "for" iterations is greater than zero.

40, 311L0, ulpERR_ABORT_COMP_Unknown_Hostvar_Error = The host variable [<0%s>] is unknown.
# *Cause: The host variable is unknown.
# *Action: Ensure that the host variable has been declared.

41, 312L0, ulpERR_ABORT_COMP_Lob_Locator_Error = The host variable in a FREE LOB statement must be a LOB locator.
# *Cause: The variable was not a LOB locator.
# *Action: Check the type of the host variable.

42, 313L0, ulpERR_ABORT_COMP_Unterminated_EMString_Error = Unterminated embedded SQL statement.
# *Cause: Unterminated embedded SQL statement.
# *Action: Ensure that the embedded SQL statement ends with a semicolon (';').

43, 314L0, ulpERR_ABORT_COMP_Wrong_IndicatorType_Error = The indicator variable [<0%s>] should be of type SQLLEN or a compatible type.
# *Cause: The indicator variable data type is not compatible with SQLLEN.
# *Action: Change the indicator variable data type to SQLLEN or INT.

44, 315L0, ulpERR_ABORT_COMP_Repeat_Array_Struct_Error = Two or more arrays of structures are bound to host variables in the same statement.
# *Cause: Only one array of structures can be bound to a host variable in a single statement.
# *Action: Bind only one array of structures to a host variable in one statement.

45, 316L0, ulpERR_ABORT_COMP_Invalid_Indicator_Usage_Error = A host variable that is an array of structures cannot be associated with an indicator.
# *Cause: A host variable that is an array of structures is associated with an indicator.
# *Action: Remove the indicator.

### macro error (401-499M) ###

50, 401M0, ulpERR_ABORT_COMP_Unknown_Macro_Overflow_Error = An unknown macro is too long. (>4k)
# *Cause: A macro is too long.
# *Action: Shorten the macro so it is within the allowable size.

51, 402M0, ulpERR_ABORT_COMP_IF_Macro_Syntax_Error = Macro #if statement syntax error.
# *Cause: The syntax of an #if statement in a macro is incorrect.
# *Action: Check the syntax of the #if statement in the macro.

52, 403M0, ulpERR_ABORT_COMP_ELIF_Macro_Syntax_Error = Macro #elif statement syntax error.
# *Cause: The syntax of an #elif statement in a macro is incorrect.
# *Action: Check the syntax of the #elif statement in the macro.

53, 404M0, ulpERR_ABORT_COMP_ELIF_Macro_Sequence_Error = Macro #elif statement sequence error.
# *Cause: The position of an #elif statement in a macro is incorrect.
# *Action: Check the position of the #elif statement in the macro.

54, 405M0, ulpERR_ABORT_COMP_ELSE_Macro_Sequence_Error = Macro #else statement sequence error.
# *Cause: The position of an #else statement in a macro is incorrect.
# *Action: Check the position of the #else statement in the macro.

55, 406M0, ulpERR_ABORT_COMP_ENDIF_Macro_Sequence_Error = Macro #endif statement sequence error.
# *Cause: The position of an #endif statement in a macro is incorrect.
# *Action: Check the position of the #endif statement in the macro.

56, 407M0, ulpERR_ABORT_PREPROC_If_Empty_Char_Constant_Error = An empty char constant cannot be used with an #if macro expression.
# *Cause: An empty char constant was used with an #if macro expression.
# *Action: Do not use an empty char constant with an #if macro expression.

57, 408M0, ulpERR_ABORT_COMP_Include_Depth_Too_Large_Error = Include files are nested too deeply. (maximum <0%s>)
# *Cause: Include files are nested too deeply within other include files.
# *Action: Do not nest include files more than <0%s> levels deep.

58, 409M0, ulpERR_ABORT_COMP_NO_ENDIF_Macro_Error = No #endif error.
# *Cause: An #if statement was used in a macro without an #endif statement.
# *Action: Remove the #if statement from the macro or add an #endif statement to the macro.

59, 410M0, ulpERR_ABORT_COMP_DEFINE_FUNC_NOBRACE_Error = A closing parenthesis ')' is missing from the macro parameter list.
# *Cause: A closing parenthesis ')' is missing from the macro parameter list.
# *Action: Add a closing parenthesis ')' to the macro parameter list.

60, 411M0, ulpERR_ABORT_COMP_DEFINE_Unknown_Macro_Error = Unknown macro name, or missing parenthesis after macro name. ( <0%s> )
# *Cause: The name of a macro is unknown, or a closing parenthesis is missing after a macro name.
# *Action: Check the macro name and declaration syntax.

61, 412M0, ulpERR_ABORT_COMP_DEFINE_FUNC_Unterminated_String_Error = Unterminated string error.
# *Cause: A string does not have a terminating character.
# *Action: Check that the string has been properly terminated.

62, 413M0, ulpERR_ABORT_COMP_Define_Macro_Overflow_Error = The content of a #define macro is too long. (>32k)
# *Cause: The content of a #define macro is too long.
# *Action: Check the #define macro and shorten it.

### extra error (501-599H) ###

70, 501H0, ulpERR_ABORT_COMP_Option_Duplicated_Error = <0%s> option is repeated.
# *Cause: A preprocessing command-line option was specified more than once.
# *Action: Check the preprocessing command-line option.

71, 502H0, ulpERR_ABORT_COMP_Option_String_Overflow_Error = Option string <0%s> is too long.
# *Cause: An option string was too long.
# *Action: Check the preprocessing command-line options.

# 72, 503H0, ulpERR_ABORT_COMP_Not_Used_SEAOption_Error = The -mt and -sea options cannot be used together.
# *Cause: Invalid options were used.
# *Action: Check the preprocessing command-line options.

73, 504H0, ulpERR_ABORT_COMP_Invalid_Input_fileName_Error  = Input file must be a form of '*.sc'.
# *Cause: The preprocessing filename extension was invalid.
# *Action: Check the preprocessing file name.

74, 505H0, ulpERR_ABORT_Unknown_Stmttype_Error = Unknown embedded SQL statement type.
# *Cause: Unknown embedded SQL statement.
# *Action: Check the embedded SQL statement.

### not-supported functional error (701-799L) ###

# 80, 701L0, ulpERR_ABORT_COMP_Not_Supported_CURSOR_SENSITIVITY_Error = No CURSOR SENSITIVITY options are supported yet.
# *Cause: CURSOR SENSITIVITY options are currently unsupported.
# *Action: Do not use this function.

# 81, 702L0, ulpERR_ABORT_COMP_Not_Supported_CURSOR_INSENSITIVITY_Error = No CURSOR INSENSITIVITY options are supported yet.
# *Cause: CURSOR SENSITIVITY options are currently unsupported.
# *Action: Do not use this function.

# 82, 703L0, ulpERR_ABORT_COMP_Not_Supported_CURSOR_ASENSITIVITY_Error = No CURSOR ASENSITIVITY options are supported yet.
# *Cause: CURSOR SENSITIVITY options are currently unsupported.
# *Action: Do not use this function.

# 83, 704L0, ulpERR_ABORT_COMP_Not_Supported_WITH_HOLD_Error = WITH HOLD option is not supported yet.
# *Cause: The WITH HOLD option is currently unsupported.
# *Action: Do not use this function.

84, 705L0, ulpERR_ABORT_COMP_Not_Supported_WITH_RETURN_Error = WITH RETURN option is not supported yet.
# *Cause: The WITH RETURN option is currently unsupported.
# *Action: Do not use this function.

85, 706L0, ulpERR_ABORT_COMP_Not_Supported_READ_ONLY_Error = READ ONLY option is not supported yet.
# *Cause: The READ ONLY option is currently unsupported.
# *Action: Do not use this function.

86, 707L0, ulpERR_ABORT_COMP_Not_Supported_ALTER_COMPACT_Error = ALTER COMPACT option is not supported yet.
# *Cause: The ALTER COMPACT option is currently unsupported
# *Action: Do not use this function.


## library errors ##

90, 08003, ulpERR_ABORT_Conn_Not_Exist_Error = The connection does not exist. (Name:<0%s>)
# *Cause: The server connection was released.
# *Action: Check the connection with the server.

91, HY000, ulpERR_ABORT_Conn_Aleady_Exist_Error = The connection already exists. (Name:<0%s>)
# *Cause: The connection with the server already exists.
# *Action: Check the connection with the server.

# 92, HY000, ulpERR_ABORT_Connstr_Overflow_Error = The connection string is too long. It must be less than 255 characters. (ConnStr:<0%s>)
# *Cause: The server connection string is too long.
# *Action: Check the server connection string.

93, HY000, utERR_ABORT_Conn_First_Trial_Failed = Failed first connection attempt.
# *Cause: The first server connection attempt failed.
# *Action: Check the server connection information.

94, HY000, ulpERR_ABORT_Conn_Second_Trial_Failed = Failed second connection attempt.
# *Cause: The second server connection attempt failed.
# *Action: Check the server connection information.

95, HY000, ulERR_ABORT_Disconn_Xa_Error = Cannot connect or disconnect in a XA Env.
# *Cause: Altibase Embedded SQL applications cannot invoke connect or disconnect in an XA environment.
# *Action: Do not use EXEC SQL CONNECT, DISCONNECT statements in an X/Open DTP environment.

96, HY000, ulpERR_ABORT_Conflict_Two_Emsqls_Error = Two embedded SQL statements conflict with each other in a multithreaded environment.
# *Cause: Replication failed because two or more embedded statements were executed simultaneously.
# *Action: Check replication handling in the multithreaded program.

97, HY000, ulpERR_ABORT_Stmt_Not_Exist_Error = The statement does not exist. (Name:<0%s>)
# *Cause: The statement does not exist.
# *Action: Check the statement syntax.

98, HY000, ulpERR_ABORT_Stmt_Need_Prepare_4Execute_Error = The statement must be prepared for execution. (Name:<0%s>)
# *Cause: The statement is not ready.
# *Action: Check the statement syntax.

99, HY000, ulpERR_ABORT_Cursor_Not_Exist_Error = The cursor does not exist. (Name:<0%s>)
# *Cause: The statement does not exist.
# *Action: Check the statement syntax.

100, HY000, ulpERR_ABORT_Cursor_Need_Declare_4Open_Error = The cursor must be declared to be opened. (Name:<0%s>)
# *Cause: The cursor conditions are not valid.
# *Action: Check the cursor conditions.

# 101, HY000, ulpERR_ABORT_For_Small_IterNum_Error = The FOR statement loop count is too small. (For count:<0%d>)
# *Cause: The FOR statement loop count is too small.
# *Action: Check the FOR statement.

103, HY000, ulpERR_ABORT_Stmt_Query_Overflow = The query statement is too long. It must be less than 256k.
# *Cause: The query statement exceeds 256k.
# *Action: Check the query statement.

104, HY000, ulpERR_ABORT_Too_Many_Rows_Error = Too many rows returned.
# *Cause: Too many rows were returned for the allocated space.
# *Action: Increase the size of the space allocated to receive the returned rows.

105, HY000, ulpERR_ABORT_Invalid_User_Error = Invalid user.
# *Cause: The user ID does not exist.
# *Action: Re-enter your user ID.

106, HY000, ulpERR_ABORT_Invalid_Passwd_Error = Invalid password.
# *Cause: The password was incorrect.
# *Action: Re-enter your password.

107, HY000, ulpERR_ABORT_Stmt_Need_Execute_4Fetch_Error = The statement must be executed to fetch rows.
# *Cause: The statement must be executed to fetch rows.
# *Action: Check the sequence of embedded SQL statements.

108, HY010, ulpERR_ABORT_Cursor_Need_Open_4Fetch_Error = The cursor must be opened to fetch rows.
# *Cause: The cursor must be opened to fetch rows.
# *Action: Check the sequence of embedded SQL statements.

# 109, HY000, ulpERR_ABORT_COMP_cannot_exec_inXa_Error = Cannot execute DCL or DDL in a global transaction.
# *Cause: Altibase XA cannot execute DCL or DDL SQL statements.
# *Action: Do not use DCL or DDL statements.

110, HY000, ulpERR_ABORT_NCHAR_No_Indicator_Error = The NCHAR type value must have an indicator.
# *Cause: No indicator.
# *Action: Make an indicator.

111, HY000, ulpERR_ABORT_XA_GetConnectAttr_Error = Cannot get the connection attributes for ALTIBASE_XA_NAME.
# *Cause: Cannot get the connection attributes for ALTIBASE_XA_NAME.
# *Action: Check the XA connection.

112, HY000, ulpERR_ABORT_Atomic_For_Value_Error = A FOR clause must execute at least once.
# *Cause: The constant in an Atomic FOR clause was set to less than 1.
# *Action: Set the constant in the Atomic FOR clause so that it is greater than 1.

113, HY000, ulpERR_ABORT_FORstmt_Invalid_usage_Error = The FOR clause can only be used with variables of array or pointer type.
# *Cause: An attempt was made to use a variable that was not an array or pointer type with a FOR clause.
# *Action: Use array or pointer type variables with the FOR clause.

114, HY000, ulpERR_ABORT_Invalid_Indicator_Value_Error = Invalid indicator: <0%d>
# *Cause: The value of the indicator is invalid.
# *Action: Change the indicator value.

115, HY000, ulpERR_ABORT_Unknown_Type_For_Shared_Pointer = The type of the pointer used is not supported.
# *Cause: The type of the pointer used in DECLARE POINTER statement is not supported.
# *Action: Declare the pointer with appropriate type.

116, 113H0, ulpERR_ABORT_Memory_Alloc_Failed = Memory allocation error. <0%s>.
# *Cause: Memory space has been exhausted.
# *Action: Reduce the amount of memory being used in client application.

117, HY000, ulpERR_ABORT_Invalid_Array_Size_Error = Invalid bind set array size: <0%d>
# *Cause: The value of the array size is invalid.
# *Action: Check the value of the array size.

118, HY000, ulpERR_ABORT_Type_Not_Exist_Error = Unknown type: <0%d>
# *Cause: Incompatible data type.
# *Action: Check the compatibility of the data type.

119, HY000, ulpERR_ABORT_DynamicSql_Max_ColumnSize_Small = SQLDA.N is too small: <0%d>
# *Cause: The value of the dynamic sql to be bound is too small.
# *Action: Check the column size and enlarge the value.

120, HY000, ulpERR_ABORT_DynamicSql_Max_BindSize_Small = SQLDA.N is too small: <0%d>
# *Cause: The value of the dynamic sql to be bound is too small.
# *Action: Check the bind size and enlarge the value.

121, HY000, ulpERR_ABORT_Not_Supported_Host_Var_Type = Currently not supported host variable type.
# *Cause: Currently not supported host variable type.
# *Action: Refer to the Precompiler user's manual for available host variables.

122, HY000, ulpERR_ABORT_Column_Value_Is_Null = The fetched result contains a NULL value. or Fetch column value is NULL.
# *Cause: The fetched result contains a NULL value. or Fetch column value is NULL.
# *Action: Verify that the stored procedure contains NULL.(Revise the cursor definition so that no columns possibly containing NULL values are retrieved.)

123, HY000, ulpERR_ABORT_Invalid_PSM_Array_Version = Invalid psm array meta version.
# *Cause: Invalid psm array meta version.
# *Action: Contact Altibase's Support Center (http://support.altibase.com).

124, 07006, ulpERR_ABORT_Invalid_PSM_Array_Type = The apre type and psm array type do not match.
# *Cause: The apre host variable type and psm array type do not match.
# *Action: Change the apre host variable types to the data types that are supported by psm array.

#125, 07006, ulpERR_ABORT_PSM_Not_Exist_Error = The name of PSM does not exist. (<0%s>)
# *Cause: There is no PSM with the name you specified.
# *Action: Verify the existence of PSM name and try again.

126, 07006, ulpERR_ABORT_Incompatible_Type_With_Diag_Item = Incompatible host variable type with the diagnostic item. (<0%s>)
# *Cause: The data type of the target is not compatible with the data type of the requested diagnostic item.
# *Action: Change the apre host variable type to the data type that is compatible with the data type of the diagnostic item.

