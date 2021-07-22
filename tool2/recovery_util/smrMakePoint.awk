
# usage : gawk -v SRC_INPUT=error.cpp -f getline.awk 

BEGIN {
    line = 0;
    
    PATTERN1="IDU_REC_POINT_KILL";
    PATTERN2="IDU_REC_POINT_ABRT";
    PATTERN3="IDU_REC_POINT_WAIT";
    
    num = split(SRC_INPUT, basename, "/");
    rege1 =  "[^a-zA-Z_:>.][ ]*"PATTERN1"[ ]*[\\(][ ]*[\"][a-zA-Z0-9\\-_:>#@. ]*[\"][ ]*[\\)]";
    rege2 =  "[^a-zA-Z_:>.][ ]*"PATTERN2"[ ]*[\\(][ ]*[\"][a-zA-Z0-9\\-_:>#@. ]*[\"][a-zA-Z0-9_\\-, ]*[\\)]";
    rege3 =  "[^a-zA-Z_:>.][ ]*"PATTERN3"[ ]*[\\(][ ]*[\"][a-zA-Z0-9\\-_:>#@. ]*[\"][ ]*[\\)]";
    rege4 =  "[\"][a-zA-Z0-9\\-_:>##@. ]*[\"]";
    
    while( (getline < SRC_INPUT) > 0)
    {
        line++;
        
        linestr1 = $0;
        linestr2 = $0;
        linestr3 = $0;
        
        if (match(linestr1, rege1))
        {
            start = match(linestr1, rege4);
            end   = index(substr(linestr1,start+1),"\"");
            #printf ("%s %d\n", basename[num], line);
            #printf ("%s\n", substr(linestr1,start+1,end-1));
            printf ("%s %d %s\n", basename[num], 
					               line, 
								   substr(linestr1,start+1,end-1));
        }
        else if (match(linestr2, rege2))
        {
            start = match(linestr2, rege4);
            end   = index(substr(linestr2,start+1),"\"");
            #printf ("%s %d\n", basename[num], line);
            #printf ("%s\n", substr(linestr2,start+1,end-1));
            printf ("%s %d %s\n", basename[num], 
					               line, 
								   substr(linestr2,start+1,end-1));
        }
        else if (match(linestr3, rege3))
        {
            start = match(linestr3, rege4);
            end   = index(substr(linestr3,start+1),"\"");
            #printf ("%s %d\n", basename[num], line);
            #printf ("%s\n", substr(linestr3,start+1,end-1));
            printf ("%s %d %s\n", basename[num], 
					               line, 
								   substr(linestr3,start+1,end-1));
        }
    }
    
    close(SRC_INPUT);
    
    exit 0;
}

