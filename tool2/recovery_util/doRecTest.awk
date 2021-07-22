#
# Usage : gawk  -f doRecTest.awk
#
#
# Altibase 소스코드에서 SMU_REC_POINT()라고 정의를 한 부분은
# Recovery 테스트 대상이 되며,
# recovery.dat에 등록이 된다.(sm/src/에서 make gen_rec_list 를 수행)
#
#  Formats
#
#  1. 소스코드 : SMU_REC_POINT()
#
#  2. killPoint 화일 : | 화일명  라인번호  수행타입 인자| 
#     sKillPointFile = ENVIRON["ALTIBASE_HOME"]"/trc/killPoint.dat";
#
#  3. 종료시 boot.log 내용
#   |  SMU_REC_POINT 화일명 라인번호 |
#

BEGIN {

    #-------------------------- 1. DEFINE ----------------------------------------
    # 테스트를 시작할 라인 번호 : 1부터 시작 not 0
    sStartScenarioNum = 1;

    # 시나리오 테스트 중 발생하는 메시지 화일
    sMsgLogFile = LOG;

    # 시나리오 테스트 중 발생하는 메시지 화일
    #  0 : 즉시, 1 : Count, 2 : Random
    KILL_TYPE = 0;

    # Recovery Test Point를 가진 화일 (sm 디렉토리에서 make gen_rec_list;
    # 라고 하면 생성됨)
    sPointListFile = LIST;
    sKillPointFile = ENVIRON["ALTIBASE_HOME"]"/trc/killPoint.dat";

    
    line = 1;
    #-----------------------------------------------------------------------------

    #--------------------------  2. Startup 준비  ---------------------------
    system("server kill");
    printf ("\n\n[%s] BEGIN Of Recovery Test \n", strftime())  >> sMsgLogFile;

    i = 0;
    while ( (getline < sPointListFile) < 0)
    {
        printf("Loading Recovery Database File Error : Can't Find [%s] \n", sPointListFile);
        printf("generating...\n");
        res = system("sh ./gen.sh");
        if (i == 2 || res < 0)
        {
            printf("go to sm/src. and execute (make gen_rec_list)");
            exit;
        }
        i++;
    }
    close(sPointListFile);
    
    #--------------------------  3. Recovery Test 수행 ---------------------------

    while( (getline killPointName < sPointListFile) > 0)
    {
        # 테스트 시작할 위치까지 Skip 
        if ( line < sStartScenarioNum )
        {
            line++;
            continue;
        }
        # 1. Begin 로깅
        printf ("\n [%s] LineNum %d : \n",
                strftime(),  line ) >> sMsgLogFile;
        
        if((getline RecPointName < sPointListFile) <= 0)
		{
            printf("invalid recovery.dat (run gen.sh)");
            exit;
		}

        printf ("[%s]\n", RecPointName) >> sMsgLogFile;

        # 2. killPoint 화일 생성 : altibase에 의해 읽혀짐.
        makeKillingPoint(KILL_TYPE)

        # 3. 서버 구동 및 검사
        if (system("server start") != 0)
        {
            printf("Error of Server Start");
            exit 0;
        }
        checkSuccessStartup(sPointListFile, line);

        # 4. 시나리오 SQL 수행
        doScenario();
        
        # 5. 서버가 비정상 종료인지 아닌지 검사 및 로깅
        result = checkScenarioResult();

        if (result == "failure")
        {
            logMsg(" Killing Altibase. Bacause of scenario failure..");
            system("server kill");
        }

        # 6. Next Line으로 
        line++;
    }

    # 마지막 Scenario에 대한 검사. (startup 및 검증 필요
    logMsg("\n  Last Scenario Starup Test");
    if (system("server start") != 0)
    {
        printf("Error of Server Start");
        exit 0;
    }
    checkSuccessStartup(sPointListFile, line);
    
    #------------------- 4. End Of Test -----------------------------------------
    printf ("\n[%s] END Of Recovery Test\n", strftime())  >> sMsgLogFile;

    close(sMsgLogFile);
    close(sPointListFile);
    exit 0;
}

# ============================ Do Scenario =======================================

function doScenario()
{
    logMsg(" Running  Scenario");
    system("is -f scenario.sql");
}

# ============================ Check Result =======================================

function checkScenarioResult()
{
    sBootLogFile = ENVIRON["ALTIBASE_HOME"]"/trc/altibase_boot.log";
    
    lastline = getLastLine(sBootLogFile); # boot.log의 마지막 라인
    
    regx =  "[ ]*SMU_REC_POINT_KILL[ ]*"killPointName"[ ]*";
    
    printf("check String [%s]", killPointName);
    if (match(lastline, regx))
    {
        logMsg(" [SUCCESS] Died!!. going to restart recovery...");
        return "success";
    }
    else
    {
        logMsg(" [ERROR] Still Alive.. Check Your Scenario.");
        return "failure";
    }
}


# ============================ Generation Kill Point ===========================
# Point 화일 구조
#  +-----------------------------------------+
#  | 화일명  라인번호  수행타입 인자 확장인자| 
#  +-----------------------------------------+
#
#   C에서 (%s %d %s %d %d) 형태로 읽을 수 있도록 함.
#
#  1. 화일명 : Recovery 매크로가 입력된 소스코드
#  2. 해당 화일에서 종료될 라인번호
#  3. 수행 타입
#     0. IMMEDIATE  : 즉시 종료함. (인자는 0으로 입력해야 함)
#     1. DECREASE   : 인자로 명시된 값만큼 반복하고, 0이 되면, 종료.
#     2. RANDOM     : 인자/1000 의 확률로 죽을 것인지 판별. 1일 경우
#                     0.1%의 확률로 종료함.
#  4. 인자 : 수행 타입에 대한 부가인자 UInt
#
#  5. 확장인자 : 별도의 플래그를 둘 수 있도록 함.
#                0 => Restart Recovery 시에는 KillPoint를 skip
#                1 => Restart Recovery 시에도 Kill Point를 적용.
#
function makeKillingPoint(aKillType)
{
    sExtFlag = 0; # restart 시에는 Skip
    
    if (aKillType == 0)
    {
        printf ("%s KILL_IMME 0 %d\n", killPointName, sExtFlag) > sKillPointFile;
        #printf ("%s\n", RecPointName) > sKillPointFile;
    }
    else
    {
        if (aKillType == 1)
        {
            printf ("%s KILL_DECR 1 %d\n", killPointName, sExtFlag) > sKillPointFile;
            #printf ("%s\n", RecPointName) > sKillPointFile;
        }
        else
        {
            if (aKillType == 2)
            {
                printf ("%s KILL_RAND 500 %d\n", killPointName, sExtFlag) > sKillPointFile;
                printf ("%s\n", RecPointName) > sKillPointFile;
            }
            else
            {
                logMsg("Error of aKillType : %d\n", aKillType);
            }
        }
    }
    close(sKillPointFile);
}

# ============================ Startup 검사 ====================================
function checkSuccessStartup(aPointListFile, aLine)
{
    sBootLogFile = ENVIRON["ALTIBASE_HOME"]"/trc/altibase_boot.log";
    
    lastline = getLine(sBootLogFile); # boot.log의 마지막 라인
    
    regx =  "[ ]*STARTUP Process SUCCESS*";
    
    if (match(lastline, regx))
    {
        logMsg(" [SUCCESS] Startup Success.");
    }
    else
    {
        logMsg(" [ERROR] Startup Failure.. Check Recovery Source. But, proceeding ... ");
		sComComp = "sh artComp "aPointListFile"-"line;
		system(sComComp);
        system("clean");
        system("server start");
    }
}

# ============================ LIBRARY =========================================
function logMsg(msg)
{
    printf(" %s\n", msg) >> sMsgLogFile;
}

function getLine(aLogFile)
{
	system("sleep 2;");
    getlinecomm = "tail -4 " aLogFile " | head -1";
    getlinecomm | getline sLastLine;
    close(getlinecomm);
    #printf(" %s\n", sLastLine) >> sMsgLogFile;
    return sLastLine;
}

function getLastLine(aLogFile)
{
    getlinecomm = "cat " aLogFile " | tail -1 ";
    getlinecomm | getline sLastLine;
    close(getlinecomm);
    return sLastLine;
}
