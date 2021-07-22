BEGIN {
    system("ls") > "test.txt";
    
    killPointName = "smuLatch.cpp 23"
        
    lastline = "SMU_REC_POINT smuLatch.cpp 23 ";
    
    regx =  "[ ]*SMU_REC_POINT[ ]*"killPointName"[ ]*";
    if (match(lastline, regx))
    {
        printf("ok..killed");
    }
    else
    {
        printf("!!! not..killed");
    }
    
#    sLogFile = ENVIRON["ALTIBASE_HOME"]"/trc/altibase_boot.log";

#    printf("last[%s]\n", getLastLine(sLogFile));
    
}

function getLastLine(file)
{
    while( (getline sLastLine < file) > 0);
    close(sLogFile);
    return sLastLine;
}
