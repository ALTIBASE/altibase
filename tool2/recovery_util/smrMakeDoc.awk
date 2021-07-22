
# usage : gawk -v SRC_INPUT=error.cpp -f getline.awk 

BEGIN {

    chkcount = 0;
    PATTERN="SMU_REC_POINT"
    num = split(SRC_INPUT, basename, "/");
    rege1 =  "[^a-zA-Z_:>.][ ]*"PATTERN"[ ]*[\\(][ ]*[\"][a-zA-Z0-9_:>#,. ]*[\"][ ]*[\\)]";
    rege2 =  "[\"][a-zA-Z0-9_:>#,. ]*[\"]";

    printf ("####### RECOVERY CHECK LIST << %s >> ", basename[num]); 

    while( (getline < SRC_INPUT) > 0)
    {
        if (match($0, rege1))
        {
			if(chkcount == 0)
			{
				printf("\n");
			}
            chkcount++;
            start = match($0, rege2);
			end   = index(substr($0,start+1),"\"");
            printf ("%s\n", substr($0,start+1,end-1));
        }
    }

	printf ("####### total count %d\n\n", chkcount);
    close(SRC_INPUT);
    exit 0;
}

