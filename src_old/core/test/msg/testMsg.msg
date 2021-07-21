
SECTION FILE;

PRODUCT_ID   = 128;
PRODUCT_NAME = "test";
PRODUCT_ABBR = "TST";

LANGUAGES    = EN, KO;


SECTION ERROR;

{
    ERR_ID = 1;
    KEY    = TEST_ERR_ERROR1;
    MSG_EN = "msg test error 1";
    MSG_KO = "msg test error 1 [KOR]";
}
{
    ERR_ID = 4;
    KEY    = TEST_ERR_ERROR4;
    MSG_EN = "msg test error 4 %1$d; "
        "message goes long";
    MSG_KO = "msg test error 4 %1$d [KOR]";
}
{
    ERR_ID = 2;
    KEY    = TEST_ERR_ERROR2;
    MSG_EN = "msg test error 2";
    MSG_KO = "msg test error 2 [KOR]; "
        "message goes long";
}


SECTION LOG;

{
    KEY    = TEST_LOG_MESSAGE1;
    MSG_EN = "msg test message 1";
    MSG_KO = "msg test message 1 [KOR]";
}
{
    KEY    = TEST_LOG_MESSAGE2;
    MSG_EN = "msg test message 2 %1$d; "
        "message goes long";
    MSG_KO = "msg test message 2 %1$d [KOR]";
}
{
    KEY    = TEST_LOG_THREAD_MESSAGE;
    MSG_EN = "log thread test message %1$d %2$d %3$d;"
        "AUDIO VIDEO DISCO";
    MSG_KO = "log thread test message %1$d %2$d %3$d [KOR]";
}
