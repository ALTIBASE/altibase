
SECTION FILE;

PRODUCT_ID   = 4;
PRODUCT_NAME = "UserLibrary";
PRODUCT_ABBR = "ULL";

LANGUAGES    = EN, KO;


SECTION ERROR;

{
    ERR_ID = 1;
    KEY    = ULL_ERR_ALLOC;
    MSG_EN = "memory allocation failed, errno = %d"
    MSG_KO = "memory allocation failed, errno = %d"
}
{
    ERR_ID = 3;
    KEY    = ULL_ERR_FILE_OPEN;
    MSG_EN = "file open failed, errno = %2$d, %1$S";
    MSG_KO = "file open failed, errno = %2$d, %1$S";
}
{
    ERR_ID = 4;
    KEY    = ULL_ERR_FILE_CLOSE;
    MSG_EN = "file close failed, errno = %d";
    MSG_KO = "file close failed, errno = %d";
}


SECTION LOG;

{
    KEY    = ULL_LOG_ALLOC;
    MSG_EN = "%1$zd byte memory allocated";
    MSG_KO = "%1$zd byte memory allocated";
}
{
    KEY    = ULL_LOG_FILE_OPEN;
    MSG_EN = "file opened at %1$S";
    MSG_KO = "file opened at %1$S";
}
