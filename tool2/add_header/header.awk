
# usage : gawk  -f header.awk idn.cpp

# comm_mode 0 : 일반
#            1 : 주석 처리중.
# brace_depth ;
#


BEGIN {
    # /* comment erase  */
    
    slash_comm = "//";
    comment1 = "/\\*";
    comment2 = "\\*/";
    
    brace_depth  = 0;
    brace_open   = 0;
    line         = 0;
    comm_mode    = 0;
    old_str      = "";

    header_flag  = 0;
    footer_flag  = 0;
}

{
    line++;
    org_str = $0;
    
    header_flag  = 0;
    footer_flag  = 0;
    
    if (comm_mode == 0)
    {
        
        # // comment erase 
        slash_num = match($0, slash_comm);
        
        if (slash_num > 0)
        {
            $0 = substr($0, 1, slash_num - 1);
        }
        else
        {
            comm1_num = match($0, comment1);
            if (comm1_num > 0) # 시작 출현!!
            {
                old_str = substr($0, 1, comm1_num - 1);
                new_str = substr($0, comm1_num + 2, length($0) - comm1_num - 2);

                comm2_num = match(new_str, comment2);
                if (comm2_num > 0) # 마지막 주석 출현!!
                {
                    $0 = substr(new_str, comm2_num + 2, length($0) - comm2_num - 2);
                    $0 = (old_str $0 ); # 이전에 남은 스트링과 합친다.
                     
                }
                else
                {
                    comm_mode = 1;
                }
            }
        }
    }
    if (comm_mode == 1) # 주석처리중
    {
        comm2_num = match($0, comment2);
        if (comm2_num > 0) # 주석 출현!!
        {
            $0 = substr($0, comm2_num + 2, length($0) - comm2_num + 2);
            comm_mode = 0;
            $0 = (old_str $0 ); # 이전에 남은 스트링과 합친다.
        }
        else
        {
            $0 = "";
        }
    }
    for (i = 1; i <= length($0); i++)
    {
        chr = substr($0, i, 1);
        if ( chr == "{" )
        {
            if (brace_depth == 0) # 첫번째 
            {
                header_flag  = 1;
            }
            brace_depth++;
        }
        else
        if (chr == "}")
        {
            if (brace_depth == 1) # 첫번째 
            {
                footer_flag  = 1;
                
            }
            brace_depth--;
            if (brace_depth < 0)
            {
                printf("brace error\n"); exit -1;
            }
        }
    }
    
    if (footer_flag) outputFooter();
    printf("%s\n", org_str);
    if (header_flag) outputHeader();
        
}

function outputHeader()
{
    printf("\n    #define IDE_FN \"FuncName\"\n");
    printf("    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(\"\"));\n\n");
}

function outputFooter()
{
    printf("\n\n    #undef IDE_FN\n");
}
