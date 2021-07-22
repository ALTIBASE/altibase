
# usage : gawk -v API_INPUT=API.txt -v SRC_INPUT=error.cpp -f getline.awk 

BEGIN {
    line = 0;
    printf("\n\n** Check API -> %s ", SRC_INPUT);
    while( (getline < SRC_INPUT) > 0)
      {
        printf(".");
        line++;
        while((getline API_name < API_INPUT) > 0)
        {
            if (API_name == "" || match(API_name, "^\\#") ) continue;

            rege1 =  "[^a-zA-Z_:>\.][ ]*"API_name"[ ]*[\\(]";
            rege2 =  "^"API_name"[ \\(/t/n]";
            rege3 =  "->"API_name"[ \\(/t/n]";
            
            if ( match($0, rege1) || match($0, rege2) )
            {
              printf ("\nERROR(%s:%d) %s (API => %s)\n", SRC_INPUT, line, $0, API_name);
            }
        }
        close(API_INPUT);
      }
     close(SRC_INPUT);
     exit 0;
}

