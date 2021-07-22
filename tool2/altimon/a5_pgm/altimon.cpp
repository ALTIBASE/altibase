/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sqlcli.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define QUERY_LEN   4096
#define BIG_SIZE    32000
#define ALLOC_SIZE  128000

void getSimpleParam ( char * ,  char * ,  char * , int);
void _log         ( char *format , ... );
void dbErrMsg     ();
void dbErrMsg2    ();
void dbErrMsg3    ();
void Ltrim        ( char [] );
void trim         ( char [] );
void getSubTag    ( char * ,  char * ,  char * );
void getQuery     ( char * , int );
void getData      ( int );
void makeQuerySet ();
void processChk   ();
void runCommand   ( char * ,  char* );
void callStack    ();
void logAnal      (int);
void chkMemset    (char *);
void getDiskTag   ();
void diskChk      ();
void vmstatChk    ();
void systemChk    ();
int  saveDB       (char *, char *, char *);

int dbConnect      ();
int dbFree         ();
int chkWORD (char * ,  char * ,  char *);


void clnt_connection(int);
void sendMsg(char *buf);


/***************************************
 ODBC연결을 위해 설정.
***************************************/
SQLHDBC dbc    = SQL_NULL_HDBC;
SQLHDBC repdbc = SQL_NULL_HDBC;
SQLHENV env    = SQL_NULL_HENV;
SQLHENV repenv = SQL_NULL_HENV;
SQLHSTMT stmt  = SQL_NULL_HSTMT;
SQLHSTMT stmt2 = SQL_NULL_HSTMT;


/***************************************
 전역변수용으로 설정.
***************************************/
char duration   [255];
char siteName   [255];
char configFile [1024];
char LOG_FILE   [255];
char SLEEP_TIME [255];
char UNAME      [255];
char SHELL      [255];
char UID        [255];
char CPU_USAGE  [255];
char CPU_ACT    [1024];
char MEM_ACT    [1024];
char MEM_USAGE  [255];
char LSOF_COM   [255];
char LSOF_LIMIT [255];
char analTag    [128][255];
char DATE_FORM  [128];
char ALARM_FILE [255];
char DISK_CHK_ENABLE[128];
char DISK_DIR   [10][255];
char DISK_USAGE [10][255];
char DISK_ACT   [1024];
char SYSTEM_FREEMEM[255];
char SYSTEM_FREESWAP[255];
char SYSTEM_IDLE[255];
char DB_SAVE[10];
int  DISK_COUNT = 0;
int  OLD_DAY = 0;
int  OLD_MON = 0;


char gUNAME[255];
char gPID[255];
char gCPU[255];
char gVSZ[255];

int BATCH_EXEC = 0;



/***************************************
 XML타입으로 만든 conf를 읽어들이기 위해.
***************************************/
typedef struct {
    char tagName    [255];         /* tagName              */
    char query      [QUERY_LEN];   /* query                */
    char display    [1];           /* 쓸지말지             */
    char checkName  [255];         /* 체크할 항목명        */
    char checkValue [255];         /* 비교값               */
    char compare    [1];           /* 비교방식 0: equal, -1: below,  1: over */
    char action     [1024];         /* action script        */
    char enable     [10];          /* eanble , disable     */
    double old_value;
    int  inc_count ;
} QUERYSET;

QUERYSET xquery [100];
int QUERYSET_COUNT = 0;







/***************************************
  주어진 인자가 공백으로 이루어지면
  memset시켜서 리턴해버린다.
***************************************/
void chkMemset(char *temp)
{
    int i;


    for (i=0; i< (int)strlen(temp); i++)
    {
        if (temp[i] != ' ')
            break;
    }


    if ( i == (int)strlen(temp) )
        memset(temp, 0x00, sizeof(temp));
}





/***************************************
 결과를 로깅하는 부분.
***************************************/
void _log(char *format, ...)
{
    FILE        *fp        ;
    va_list     ap         ;
    struct tm   t          ;
    time_t      now        ;
    char        comm[1024] ;
    int         i, find;

   
    /***************************************
     날마다 logChange를 하기 위해서 
    ***************************************/
    time ( &now );
    t = *localtime ( &now );

    if ( OLD_DAY != 0 )
    {
        if (OLD_DAY != t.tm_mday)
        {
            sprintf(comm, "cp %s %s_%02d%02d 2>/dev/null", LOG_FILE, LOG_FILE, OLD_MON, OLD_DAY);
            system(comm);
            fp = fopen(LOG_FILE, "w+");
            fclose(fp);

            /******************************************
            ** 날자가 변경되면 배치를 하나 돌려야 한다. 
            ** DB저장모드만 해당된다.
            ******************************************/
            if (memcmp(DB_SAVE, "ON", 2) == 0)
            {
                BATCH_EXEC = 1;
            }

            /******************************************
             alarm file logChange. 기존에는 매번수행됨
            ******************************************/
            if (ALARM_FILE[0] != 0x00)
            {
                sprintf(comm, "cp %s %s_%02d%02d 2>/dev/null", ALARM_FILE, ALARM_FILE, OLD_MON, OLD_DAY);
                system(comm);
                fp = fopen(ALARM_FILE, "w+");
                fclose(fp);
            }
        }
    }


    /***************************************
     별도의 ALARM_LOGFILE을 지정하지 않았다면..
    ***************************************/
    find = 0;
    for (i=0;i<(int)strlen(format); i++)
    {
         if (memcmp(format+i, (char*)"[ALARM]::",  (int)strlen("[ALARM]::")) == 0)
         {
             find  = 1;
             break;
         }
    }

    /***************************************
     실제 파일에 로그를 남긴다.
    ***************************************/
    if (find == 1 && ALARM_FILE[0] != 0x00)
    {
        fp = fopen ( ALARM_FILE,  "a+" );
        if ( fp == NULL )
        {
            fprintf(stdout, "_LOG fopen error . [%s]\n", ALARM_FILE );
            exit ( -1 );
        }
    } else {
        fp = fopen ( LOG_FILE,  "a+" );
        if ( fp == NULL )
        {
            fprintf(stdout, "_LOG fopen error . [%s]\n", LOG_FILE );
            exit ( -1 );
        }
    }

    /***************************************
     사용자가 지정한 날자포맷에 따라서.
    ***************************************/
    if (DATE_FORM[0] == '2')
    {
        fprintf(fp, "[%02d/%02d %02d:%02d:%02d] ",
                                 t.tm_mon+1,    t.tm_mday,
                                 t.tm_hour,         t.tm_min,      t.tm_sec);
    } else if (DATE_FORM[0] == '3')
    {
        fprintf(fp, "[%02d:%02d:%02d] ",
                                 t.tm_hour,         t.tm_min,      t.tm_sec);
    } else {
        fprintf(fp, "[%04d/%02d/%02d %02d:%02d:%02d] ",
                                 t.tm_year+1900,    t.tm_mon+1,    t.tm_mday,
                                 t.tm_hour,         t.tm_min,      t.tm_sec);
    }

    va_start ( ap, format );
    vfprintf ( fp, format, ap );
    va_end ( ap );
    
    fflush(fp); 
    fclose ( fp );
    
    OLD_MON = t.tm_mon+1;
    OLD_DAY = t.tm_mday;
}




/***************************************
  OS command를 할 필요가 있을때 쓰라.
  결과를 2번 인자에 담아줄테다..
***************************************/
void runCommand(char *s, char *t)
{
    FILE *fp;
    char *ret;
    char tmp[1024];

    ret = (char*)malloc(sizeof(char)*ALLOC_SIZE);
    if (!ret)
    {
        _log((char*)"malloc Error\n");
        exit(-1);
    }

    fp = popen ( s, "r" );
    if ( fp == NULL )
    {
        _log((char*)"22 system call ERROR [%s]!!\n", s);
        exit(-1);
    }

    memset ( ret, 0x00, ALLOC_SIZE);
    memset ( tmp, 0x00, sizeof(tmp));

    while ( !feof(fp) )
    {
        if ( fgets ( tmp, sizeof(tmp), fp ) == NULL)
             break;
        strcat( ret, tmp);
    }
    pclose ( fp );

    ret[ (int)strlen(ret)-1 ] = 0x00;
    memcpy ( t, ret, (int)strlen(ret) );

    free(ret);

}







/***************************************
  vmstat check를 수행한다.
***************************************/
void vmstatChk()
{

    char _PCOM[1024];
    char RESULT[BIG_SIZE];
    char A[255];
    char B[255];

    
    /************************************************
      Physical Memory Free와 CPU idle정보만.
    ************************************************/
    if (memcmp(UNAME, "SunOS", 5) == 0)
        sprintf(_PCOM, "vmstat 1 2 | tail -1 | awk '{print $5 \" \" $22}'");

    if (memcmp(UNAME, "HP-UX", 5) == 0)
        sprintf(_PCOM, "vmstat 1 2 | tail -1 | awk '{print $5 \" \" $18}'");

    if (memcmp(UNAME, "AIX", 3) == 0)
        sprintf(_PCOM, "vmstat 1 2 | tail -1  | awk '{print $4 \" \" $16}'");

    if (memcmp(UNAME, "Linux", 5) == 0)
        sprintf(_PCOM, "vmstat 1 2 | tail -1 | awk '{print $5 \" \" $16}'");

    if (memcmp(UNAME, "OSF1", 4) == 0)
        sprintf(_PCOM, "vmstat 1 2 | tail -1 | awk '{print $5 \" \" $18}'");

    memset(RESULT, 0x00, sizeof(RESULT));
    runCommand(_PCOM, RESULT);

    /*************************************************
     읽은정보에서 OS에 맞게 변환작업
    *************************************************/
    memset(SYSTEM_FREEMEM, 0x00, sizeof(SYSTEM_FREEMEM));
    memset(SYSTEM_IDLE   , 0x00, sizeof(SYSTEM_IDLE));
    memset(A, 0x00, sizeof(A));
    memset(B, 0x00, sizeof(B));

    sscanf(RESULT, "%s %s", A, B);

    if (memcmp(UNAME, "SunOS" , 5) == 0)
    {
        sprintf(SYSTEM_FREEMEM, "%ld", (long)atol(A));
    }

    if (memcmp(UNAME, "HP-UX", 5) == 0)
    {
        sprintf(SYSTEM_FREEMEM, "%ld", (long)atol(A) * 4096 / 1024);
    }

    if (memcmp(UNAME, "AIX", 3) == 0 || memcmp(UNAME, "OSF1", 4) == 0 )
    {
        sprintf(SYSTEM_FREEMEM, "%ld", (long)atol(A) * 8192 / 1024);
    }
   
    if (memcmp(UNAME, "Linux", 5) == 0)
    {
        sprintf(SYSTEM_FREEMEM, "%ld", (long)atol(A));
    }

    sprintf(SYSTEM_IDLE, "%s", B);

}




/***************************************
 Altibase Process 상태정보를 남긴다.
 DISK usage를 비교 체크한다.
***************************************/
void diskChk()
{
    char _PCOM    [1024];
    char RESULT   [BIG_SIZE];
    char ALLOC    [255]; 
    char USED     [255];
    char RATE     [255];
    char tmpMsg   [1024];

    int  i;

 
    /*******************************************
     사용자가 지정한 감시대상 디스크 갯수만큼.
    *******************************************/
    for (i=0; i<DISK_COUNT ; i++)
    {

        /*******************************************
          OS마다 df, bdf의 출력결과가 다르시다.
        *******************************************/
        if (memcmp(UNAME, "SunOS", 5) == 0 || memcmp(UNAME, "OSF1", 4) == 0 || memcmp(UNAME, "Linux", 5) == 0)
            sprintf(_PCOM, "df -k | grep \"%s\" | awk '{print $2 \" \" $3 \" \" $5}' | sed 's/%%//g'", DISK_DIR[i]);

        if (memcmp(UNAME, "HP-UX", 5) == 0 )
            sprintf(_PCOM, "bdf | grep \"%s\" | awk '{print $2 \" \" $3 \" \" $5}' | sed 's/%%//g'", DISK_DIR[i]);

        if (memcmp(UNAME, "AIX", 3) == 0 )
            sprintf(_PCOM, "df -k | grep \"%s\" | awk '{print $2 \" \" $2-$3 \" \" $4}' | sed 's/%%//g'", DISK_DIR[i]);


        memset(RESULT, 0x00, sizeof(RESULT));
        runCommand(_PCOM, RESULT);
        if (RESULT[0] == 0x00)
            continue;

        /*******************************************
         결과물을 parsing한다.
        *******************************************/
        sscanf(RESULT, "%s %s %s", ALLOC, USED, RATE);
     
        /******************************************
        ******************************************/
        if (RATE[0] == '/')
        {
            if (memcmp(UNAME, "SunOS", 5) == 0 || memcmp(UNAME, "OSF1", 4) == 0 || memcmp(UNAME, "Linux", 5) == 0)
                sprintf(_PCOM, "df -k | grep \"%s\" | awk '{print $1 \" \" $2 \" \" $4}' | sed 's/%%//g'", DISK_DIR[i]);

            if (memcmp(UNAME, "HP-UX", 5) == 0 )
                sprintf(_PCOM, "bdf | grep \"%s\" | awk '{print $1 \" \" $2 \" \" $4}' | sed 's/%%//g'", DISK_DIR[i]);

            if (memcmp(UNAME, "AIX", 3) == 0 )
                sprintf(_PCOM, "df -k | grep \"%s\" | awk '{print $1 \" \" $1-$2 \" \" $3}' | sed 's/%%//g'", DISK_DIR[i]);

            memset(RESULT, 0x00, sizeof(RESULT));
            runCommand(_PCOM, RESULT);
            if (RESULT[0] == 0x00)
                continue;

            sscanf(RESULT, "%s %s %s", ALLOC, USED, RATE);
        }


        _log((char*)"VOLUMN%d=[%s],  DISK%d_ALLOC_KB=[%s],    DISK%d_USED_KB=[%s],    DISK%d_USE_RATE=[%s]%%\n",
                     i+1, DISK_DIR[i], i+1, ALLOC, i+1, USED, i+1, RATE);
        sprintf(tmpMsg, (char*)"VOLUMN%d=[%s],  DISK%d_ALLOC_KB=[%s],    DISK%d_USED_KB=[%s],    DISK%d_USE_RATE=[%s]%%\n",
                     i+1, DISK_DIR[i], i+1, ALLOC, i+1, USED, i+1, RATE);
        saveDB((char*)"0", (char*)"DISK_USAGE",  tmpMsg);

        /*******************************************
         출력된 결과의 사용량이 해당 한계값보다 크면.
        *******************************************/
        if ( (long)atol(RATE) >= (long)atol(DISK_USAGE[i]) )
        {
             _log((char*)"[ALARM]:: [DISK.%s] Current (%s)%% >= Limit (%s)%%\n", DISK_DIR[i], RATE, DISK_USAGE[i]);
             sprintf(tmpMsg, (char*)"[ALARM]:: [DISK.%s] Current (%s)%% >= Limit (%s)%%\n", DISK_DIR[i], RATE, DISK_USAGE[i]);
            system(DISK_ACT);
            saveDB((char*)"1", (char*)"DISK_USAGE",  tmpMsg);
        }
    }
    
    _log((char*)"\n");
}






/***************************************
 SYSTEM 상태정보를 남긴다.
***************************************/
void systemChk()
{
    char _PCOM    [1024];
    char RESULT   [BIG_SIZE];


    /*****************************************
     Swap에 대한 용량 구하기
    *****************************************/
    if (memcmp(UNAME, "SunOS", 5) == 0)
        sprintf(_PCOM, "/usr/sbin/swap -s | awk '{print $11}' | sed 's/k//g'");

    if (memcmp(UNAME, "AIX", 3) == 0)
        sprintf(_PCOM, "/usr/sbin/lsps -s | grep -v Used | sed 's/MB//g' | sed 's/%%//g' | awk '{print $1*(100-$2)/100}'");

    if (memcmp(UNAME, "HP-UX", 5) == 0)
        sprintf(_PCOM, "/usr/sbin/swapinfo -t | grep total | awk '{print $4}'");

    if (memcmp(UNAME, "Linux", 5) == 0)
        sprintf(_PCOM, "/sbin/swapon -s | awk 'BEGIN {sum1=0;sum2=0;}{sum1+=$3; sum2+=$4} END {print sum1-sum2}'");

    if (memcmp(UNAME, "OSF1", 4) == 0)
        sprintf(_PCOM, "/usr/sbin/swapon -s | grep Available | awk '{print $3}'");

    
    memset(RESULT,          0x00, sizeof(RESULT));
    memset(SYSTEM_FREESWAP, 0x00, sizeof(SYSTEM_FREESWAP));

    runCommand(_PCOM, RESULT);
    memcpy(SYSTEM_FREESWAP, RESULT, strlen(RESULT));
    
    /*****************************************
     AIX 만 M단위, awk printf 가 잘 안됨. 나중에 awk수정.
    *****************************************/
    if (memcmp(UNAME, "AIX", 3) == 0)
        sprintf(SYSTEM_FREESWAP, "%ld", (long)atol(RESULT)*1024);

    if (memcmp(UNAME, "OSF1", 4) == 0)
        sprintf(SYSTEM_FREESWAP, "%ld", (long)atol(RESULT)*8192/1024);


    _log((char*)"SYSTEM_MEM_FREE=[%s]kbytes,   SYSTEM_SWAP_FREE=[%s]kbytes,    SYSTEM_IDLE=[%s]%%\n", 
                 SYSTEM_FREEMEM,  SYSTEM_FREESWAP,  SYSTEM_IDLE);

    saveDB((char*)"0",  (char*)"SYSTEM_MEM_FREE",   SYSTEM_FREEMEM);
    saveDB((char*)"0",  (char*)"SYSTEM_SWAP_FREE",  SYSTEM_FREESWAP);
    saveDB((char*)"0",  (char*)"SYSTEM_IDLE",       SYSTEM_IDLE);

}





/***************************************
 Altibase Process 상태정보를 남긴다.
 CPU usage, Memory Usage, VSZ, PID
***************************************/
void processChk()
{
    char _PCOM      [1024];
    char RESULT     [BIG_SIZE];
    char PCPU       [255];
    char PID        [255];
    char VSZ        [255];
    long FD_NUM      = -1;
    char dbIP       [255];


    memset(dbIP,  0x00, sizeof(dbIP));
    getSimpleParam ((char*)"CONNECTION_INFO", (char*)"DB_IP" ,      dbIP,    1);
    if (memcmp(dbIP,   "127.0.0.1" , 9 ) != 0)
    {
         _log((char*)"TargetServer is not Local-Machine. [ip=%s]\n", dbIP);
         _log((char*)"\n");
         return;
    }

    /*********************************************
     PID
    *********************************************/
    sprintf( _PCOM,  "ps -ef | grep \"%s/bin/altibase -p boot from\" | grep -v grep | grep %s | awk '{print $2}'", (char*)getenv(IDP_HOME_ENV), UID);
    memset( PID, 0x00, sizeof(PID));
    runCommand( _PCOM, PID);


    if ((int)strlen(PID) == 0)
    {
        _log((char*)"Altibase Die !!, Can't find ProcessID\n");
        return;
    }

    /*********************************************
     VSZ 
    *********************************************/
    sprintf( _PCOM, "ps -o vsz -p %s|grep -v VSZ", PID);
    memset( VSZ,     0x00, sizeof(VSZ)     );
    runCommand ( _PCOM,  VSZ);
    saveDB((char*)"0", (char*)"VSZ", VSZ);

    /*********************************************
     CPU usage
    *********************************************/
    sprintf(_PCOM, "ps -o pcpu -p %s|grep -v CPU", PID);
    /******************************************************************************************* 
      linux는 좀 특이한데.. 일단 주석처리합니다.
    if (memcmp(UNAME, "Linux", 5) == 0)
       sprintf(_PCOM, "top -d 1 -n 1 -p %s | grep 'altibase' |awk '{print $9}' ", PID);
    *******************************************************************************************/

    memset(PCPU,    0x00, sizeof(PCPU)   );
    runCommand ( _PCOM,  PCPU);
    saveDB((char*)"0", (char*)"CPU", PCPU);


    /*********************************************
     Lsof 할지 말지.
     등록한 lsof 명령의 basename을 알아내서 종류에 맞게 수행한다.
     일단 SUN의 pfiles만 구현한다.
    *********************************************/
    if (LSOF_COM[0] != 0x00)
    {
        sprintf(_PCOM, "basename `echo %s`", LSOF_COM);
        memset(RESULT, 0x00, sizeof(RESULT));
        runCommand ( _PCOM, RESULT);

        if (memcmp(RESULT,  "pfiles", 6) == 0)
        {
            sprintf(_PCOM, "%s -F %s | wc -l", LSOF_COM, PID);
        }

        if (memcmp(RESULT,  "procfiles", 9) == 0)
        {
            sprintf(_PCOM, "%s -F %s | wc -l", LSOF_COM, PID);
        }

        if (memcmp(RESULT,  "lsof", 4) == 0)
        {
            sprintf(_PCOM, "%s -p %s | wc -l", LSOF_COM, PID);
        }

        memset(RESULT, 0x00, sizeof(RESULT));
        runCommand ( _PCOM, RESULT);
        FD_NUM = (long)atol(RESULT);
    }

    /**********************************************
     디스크 사용량에 대한 처리.
    **********************************************/

    
    _log((char*)"ALTIBASE PID=[%s],  CPU usage=[%s]%%,   VSZ=[%s]kbytes\n", PID, PCPU, VSZ);
    if (LSOF_COM[0] != 0x00)
        _log((char*)"ALTIBASE FD=[%ld]\n", FD_NUM);


    /*********************************************
     한계치 설정한것들과 비교해서 Alarm 남긴다.
    *********************************************/
    if ((float)atof(PCPU) >= (float)atof(CPU_USAGE))
    {
         _log((char*)"[ALARM]:: [PROCESS.CPU_USAGE(%%)] Current (%s) >= Limit (%s)\n", PCPU, CPU_USAGE);
         saveDB((char*)"1",  (char*)"CPU", PCPU);
         if (CPU_ACT[0] != 0x00)
         {
             system(CPU_ACT);
         }
    }

    if ((long)atol(VSZ) >= (long)atol(MEM_USAGE))
    {
         _log((char*)"[ALARM]:: [PROCESS.MEM_USAGE(KB)] Current (%s) >= Limit (%s)\n", VSZ, MEM_USAGE);
         saveDB((char*)"1",  (char*)"CPU",  VSZ);
         if (MEM_ACT[0] != 0x00)
         {
             system(MEM_ACT);
         }
    }
    
    if (LSOF_COM[0] != 0x00)
    {
        if ( FD_NUM >= atol(LSOF_LIMIT) )
        {
            _log((char*)"[ALARM]:: [PROCESS.FD_USAGE] Current (%ld) >= Limit (%s)\n", FD_NUM, LSOF_LIMIT);
            sprintf(_PCOM, "%s -F %s > $ALTIBASE_HOME/trc/lsof.res.`date +%%Y%%m%%d_%%H%%M%%S` " , LSOF_COM, PID);
            system(_PCOM);
        }
    }

   
    /********************************
     통신용 전역변수에 할당하기
    ********************************/
    memset(gCPU, 0x00, sizeof(gCPU));
    memset(gVSZ, 0x00, sizeof(gVSZ));
    memset(gPID, 0x00, sizeof(gPID));
    
    memcpy(gPID, PID,  strlen(PID));
    memcpy(gCPU, PCPU, strlen(PCPU));
    memcpy(gVSZ, VSZ,  strlen(VSZ));

    _log((char*)"\n");
   
}




/***************************************
  DB 연결하기.
***************************************/
int dbConnect()
{
    char connStr[1024];
    char dbIP[20];
    char portNo[20];
    char passWd[100];
    char nlsUse[100];
    char user[100];


    if (SQLAllocEnv(&env) != SQL_SUCCESS)
    {
        _log((char*)"dbConnect.SQLAllocEnv error!!\n");
        return -1;
    }

    if (SQLAllocConnect(env, &dbc) != SQL_SUCCESS)
    {
        _log((char*)"dbConnect.SQLAllocConnect error!!\n");
        return -1;
    }

    /*********************************************
      configFile에서 DB_IP, PORT_NO, SYS_PASSWD, NLS_USE등을 읽어온다.
    *********************************************/
    memset(dbIP,   0x00, sizeof(dbIP));
    memset(portNo, 0x00, sizeof(portNo));
    memset(passWd, 0x00, sizeof(passWd));
    memset(nlsUse, 0x00, sizeof(nlsUse));

    getSimpleParam ((char*)"CONNECTION_INFO", (char*)"DB_IP" ,      dbIP,    1);
    getSimpleParam ((char*)"CONNECTION_INFO", (char*)"PORT_NO" ,    portNo,  1);
    getSimpleParam ((char*)"CONNECTION_INFO", (char*)"SYS_PASSWD" , passWd,  1);
    getSimpleParam ((char*)"CONNECTION_INFO", (char*)"NLS_USE",     nlsUse,  1);


    /********************************************
     연결하기
    ********************************************/
    sprintf(connStr, "DSN=%s;PORT_NO=%s;UID=SYS;PWD=%s;CONNTYPE=1;NLS_USE=%s", dbIP, portNo, passWd, nlsUse);

    if (SQLDriverConnect( dbc, NULL, (SQLCHAR *)connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT ) != SQL_SUCCESS)
    {                    
        dbErrMsg();
        return -1;
    }   

    if (SQLAllocStmt( dbc, &stmt) != SQL_SUCCESS)
    {
        _log((char*)"dbConnect.SQLAllocStmt Error\n");
        return -1;
    }



    /*********************************************
    ** 저장소 정보 얻어내기
    **********************************************/
    if (memcmp(DB_SAVE, "ON", 2) == 0)
	{
        if (SQLAllocEnv(&repenv) != SQL_SUCCESS)
        {
            _log((char*)"rep_dbConnect.SQLAllocEnv error!!\n");
            return -1;
        }
    
        if (SQLAllocConnect(repenv, &repdbc) != SQL_SUCCESS)
        {
            _log((char*)"rep_dbConnect.SQLAllocConnect error!!\n");
            return -1;
        }
    
        memset(dbIP,   0x00, sizeof(dbIP));
        memset(portNo, 0x00, sizeof(portNo));
        memset(user,   0x00, sizeof(user));
        memset(passWd, 0x00, sizeof(passWd));
        memset(nlsUse, 0x00, sizeof(nlsUse));
       
        getSimpleParam ((char*)"REPOSITORY_INFO", (char*)"DB_IP" ,      dbIP,    1);
        getSimpleParam ((char*)"REPOSITORY_INFO", (char*)"PORT_NO" ,    portNo,  1);
        getSimpleParam ((char*)"REPOSITORY_INFO", (char*)"USER" ,       user,    1);
        getSimpleParam ((char*)"REPOSITORY_INFO", (char*)"PASSWD" ,     passWd,  1);
        getSimpleParam ((char*)"REPOSITORY_INFO", (char*)"NLS_USE",     nlsUse,  1);
      
        sprintf(connStr, "DSN=%s;PORT_NO=%s;UID=%s;PWD=%s;CONNTYPE=1;NLS_USE=%s", dbIP, portNo, user, passWd, nlsUse);
    
        if (SQLDriverConnect( repdbc, NULL, (SQLCHAR *)connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT ) != SQL_SUCCESS)
        {                    
            dbErrMsg();
            return -1;
        }   
    
        if (SQLAllocStmt( repdbc, &stmt2) != SQL_SUCCESS)
        {
            _log((char*)"rep_dbConnect.SQLAllocStmt2 Error\n");
            return -1;
        }
	}

    return 1;

}



/***************************************
   DB 연결 해제하기
***************************************/
int dbFree()
{
    if (stmt != NULL)
        SQLFreeStmt (stmt, SQL_DROP);

    if (stmt2 != NULL)
        SQLFreeStmt (stmt2, SQL_DROP);

    SQLDisconnect( dbc );
    SQLDisconnect( repdbc );

    if ( dbc != NULL )
    {
        SQLFreeConnect( dbc );
    }   
    if ( repdbc != NULL )
    {
        SQLFreeConnect( repdbc );
    }   
    if ( env != NULL )
    {
        SQLFreeEnv ( env );
    }
    if ( repenv != NULL )
    {
        SQLFreeEnv ( repenv );
    }

    stmt  = NULL;
    stmt2 = NULL;
    dbc   = NULL;
    env   = NULL;
    repdbc   = NULL;
    repenv   = NULL;

    return 1;
}





/***************************************
 DB Error발생시 에러 메시지를 남긴다.
***************************************/
void dbErrMsg3()
{
    SQLINTEGER errNo;
    SQLSMALLINT msgLength;
    SQLCHAR errMsg[1024];

    if (SQLError ( repenv, repdbc, stmt2, NULL, &errNo, errMsg, 1024, &msgLength ) == SQL_SUCCESS)
    {
        _log((char*)"DB_STMT2_Error:# %ld, %s\n", errNo, errMsg);
    }

    /**************************************
     DB에러중에 dual필요로 하면 생성한다.
    **************************************/
    SQLFreeStmt (stmt2, SQL_DROP);
    SQLAllocStmt (repdbc, &stmt2);

}





/***************************************
 DB Error발생시 에러 메시지를 남긴다.
***************************************/
void dbErrMsg2()
{
    SQLINTEGER errNo;
    SQLSMALLINT msgLength;
    SQLCHAR errMsg[1024];

    if (SQLError ( env, dbc, stmt2, NULL, &errNo, errMsg, 1024, &msgLength ) == SQL_SUCCESS)
    {
        _log((char*)"DB_STMT2_Error:# %ld, %s\n", errNo, errMsg);
    }

    /**************************************
     DB에러중에 dual필요로 하면 생성한다.
    **************************************/
    SQLFreeStmt (stmt2, SQL_DROP);
    SQLAllocStmt (dbc, &stmt2);

}




/***************************************
 DB Error발생시 에러 메시지를 남긴다.
***************************************/
void dbErrMsg()
{
    SQLINTEGER errNo;
    SQLSMALLINT msgLength;
    SQLCHAR errMsg[1024];

    if (SQLError ( env, dbc, stmt, NULL, &errNo, errMsg, 1024, &msgLength ) == SQL_SUCCESS)
    {
        _log((char*)"DB_STMT1_Error:# %ld, %s\n", errNo, errMsg);
    }
    /**************************************
     DB에러중에 dual필요로 하면 생성한다.
    **************************************/
    SQLFreeStmt (stmt, SQL_DROP);
    SQLAllocStmt (dbc, &stmt);
    if (errNo == 200753)
    {
        _log((char*)"Need Dual Table , Altimon make it now\n");
        SQLExecDirect(stmt, (SQLCHAR*)"create table dual (x char(1))", SQL_NTS);
        SQLExecDirect(stmt, (SQLCHAR*)"insert into dual values ('x')", SQL_NTS);
    } 

}




/***************************************
 config file에서 읽은 tag나 value의 공백을 없을때
***************************************/
void trim(char s[])
{
    int i,  j ;
    int len       ;


    len = (int)strlen ( s );

    for ( i = 0 ; i < len ; i++)
    {
        if ( s[i] == 32 || s[i] == '#' || s[i] == 10)
        {
            for ( j=i ; j<len ; j++)
            {
                if (s[j] != 32 && s[i] != '#' && s[i] != 10)
                   break;
            }
            if (j == len)
                break;
            memcpy ( s+i, s+j, len-j );
            memset ( s+len-(j-i), 0x00, (j-i));
        }
        /*
        if ( s[i] == '#' || s[i] == '\n' )
            break;
        */
    }

}





/****************************************
  앞뒤 공백 제거 시키기.
****************************************/
void Ltrim(char s[])
{
    char tmp[1024];
    int i, j, len = (int)strlen(s);
    

    /*************************************
      첫번째 문자가 나오는 위치
    **************************************/
    for (i=0; i<len; i++)
    {
        if (s[i] != 32 && s[i] != 10)
            break;
    }

    /*************************************
      문자열 이후에 첫번째 공백이 나오는 위치
    **************************************/
    for (j=i;j<len; j++)
    {
        if (s[j] == 32 || s[j] == 10)
            break;
    }

    
    /*************************************
      j ~ i 사이꺼만 복사를 한다.
    **************************************/
    memset(tmp, 0x00, sizeof(tmp));
    memcpy(tmp, s+i, j-i);
    memset(s, 0x00, len);
    memcpy(s, tmp, (int)strlen(tmp));
}




/***************************************
 단어를 주면 값을 찾아낸다. getSimplePara용이다.
***************************************/
int chkWORD(char *src, char *tar, char *value)
{
    int i, len_src = (int)strlen ( src ), len_tar = (int)strlen ( tar );
    int start = -1;
    

    // find Tag
    for ( i = 0 ; i < len_src ; i++ )
    {
        if ( src[i] == '#' || src[i] == '\n' )
            break;
    }       
    len_src = i;
    

    // get Value
    for ( i=0 ; i < len_src ; i++ )
    {
        if ( memcmp ( src+i, tar, len_tar ) == 0 )
        {
            start = len_tar + i;
            break;
        }   
    }   
    
    if ( i != len_src )
    {
        memcpy ( value, src+start, len_src-start );
        return 1;
    }   
    return -1;
}





/***************************************
  필요한 옵션의 항목, 값을 알아낸다.
***************************************/
void getSimpleParam(char *mainTag, char *paramName, char paramValue[], int trim_tag)
{
    char TMP_VALUE[255];
    char name123[255];
    char name321[255];
    char buff[BIG_SIZE];
    char subuff[BIG_SIZE];
    FILE *fp;
    int  len;
    int  i;
    int  start = -1, end = -1;

    /***************************************
     configFile을 읽는다.
    ***************************************/
    memset(buff,   0x00, sizeof(buff));
    memset(subuff, 0x00, sizeof(subuff));

    fp = fopen ( configFile, "r" );
    if ( fp == NULL )
    {
        printf ( "%s file not found !!\n", configFile );
        exit (-1); 
    }
    fread(buff, sizeof(buff)-1, 1, fp);
    fclose(fp);
    
    sprintf(name123, "<%s>", mainTag);
    sprintf(name321, "</%s>", mainTag);
   

    /***************************************
      mainTag의 그룹문자열을 찾아낸다.
    ***************************************/
    start = -1;
    end   = -1;
    len = (int)strlen(buff);
    for (i=0; i<len ; i++)
    {
        if (memcmp(buff+i, name123, (int)strlen(name123)) == 0)
            start = i + (int)strlen(name123);

        if (memcmp(buff+i, name321, (int)strlen(name321)) == 0)
            end = i - 1;
    }
    memcpy(subuff, buff+start, (end-start) + 1);

    if (start == -1 || end == -1)
    {
        printf((char*)"Not Found [%s]\n", mainTag);
        exit(-1);
    }

    /***************************************
      ParamNameTag의 그룹문자열을 찾아낸다.
    ***************************************/
    sprintf(name123, "<%s>", paramName);
    sprintf(name321, "</%s>", paramName);
    len = (int)strlen(subuff);
    start = -1;
    end   = -1;

    for (i=0; i<len ; i++)
    {
        if (memcmp(subuff+i, name123, (int)strlen(name123)) == 0)
            start = i + (int)strlen(name123);

        if (memcmp(subuff+i, name321, (int)strlen(name321)) == 0)
            end = i - 1;
    }

    /* 이코드는 문제가 있다. 실제 포이터크기만 memset할뿐이니까.. */
    memset(TMP_VALUE, 0x00, sizeof(TMP_VALUE));
    if (start == -1 || end == -1)
        return;

    if ( (end-start) <= 0)
        return;

    memcpy(TMP_VALUE, subuff+start, (end - start) + 1);

    /********************************
     space 제거 여부
    ********************************/
    if (trim_tag)
        Ltrim(TMP_VALUE);


    memcpy(paramValue, TMP_VALUE, (int)strlen(TMP_VALUE) );

}   






/***************************************
 XML형태를 파싱할때 부분부분을 알아낸다.
***************************************/
void getSubTag(char *realTag, char *subtagName, char *TMP_VALUE)
{
    char name123[255];
    char name321[255];
    int i,  k;    
    int start = -1, end = -1;


    sprintf(name123, "<%s>",  subtagName);
    sprintf(name321, "</%s>", subtagName);
    /*****************************************************
      일단, 전체를 뒤져서 tagName을 찾아내면 
      그 tag의 시작, 끝부분의 위치를 알아내서
      그 부분만큼만 value에 담아 리턴한다.
    *****************************************************/
    k = (int)strlen(realTag);
    start = -1;
    for (i=0; i<k; i++)
    {
        if (memcmp(realTag+i,  name123, (int)strlen(name123)) == 0)
        {
            start = i + (int)strlen(subtagName) + 2;
        }
        if (memcmp(realTag+i,  name321, (int)strlen(name321)) == 0)
        {
            end = i - 1;
        }
    }

    /* tag가 없으면 그냥 리턴한다. */
    memset(TMP_VALUE, 0x00, sizeof(TMP_VALUE));
    if (start == -1 || end == -1)
    {
        return;
    } else if ( (end - start) >= QUERY_LEN){
        return;
    }

    memcpy(TMP_VALUE, realTag+start, end-start+1);
}




/***************************************
 모니터링을 할 쿼리에 대해 쫘악 파싱한다.
***************************************/
void getQuery(char *tagName, int index)
{
    char name123[255];
    char name321[255];
    char buff[BIG_SIZE];
    char realTag[BIG_SIZE];
    char display[255];
    char checkName[255];
    char checkValue[255];
    char compare[255];
    char action[255];
    char enable[30];
    char query[QUERY_LEN];
    FILE *fp;
    int  i, k;
    int  start = -1, end = -1;
   
    /*****************************************************
     일단, 파일을 통째로 읽어버린다.
    *****************************************************/
    fp = fopen ( configFile, "r" );
    if ( fp == NULL )
    {
        printf ( "%s file not found !!\n", configFile );
        exit (-1); 
    }

    memset(buff, 0x00, sizeof(buff));
    fread(buff, sizeof(buff)-1, 1, fp);
    fclose(fp); 


    sprintf(name123, "<%s>",  tagName);
    sprintf(name321, "</%s>", tagName);
    /*****************************************************
      Input으로 받은 tagName에 대한 start, end위치를 알아낸다.
    *****************************************************/
    k = (int)strlen(buff);
    start = -1;
    end   = -1;
    for (i=0; i<k; i++)
    {
        if (memcmp(buff+i,  name123, (int)strlen(name123)) == 0)
        {
            start = i + (int)strlen(tagName) + 2;
        }
        if (memcmp(buff+i,  name321, (int)strlen(name321)) == 0)
        {
            end = i - 1;
        }
    }

    /*****************************************************
      validation check
    *****************************************************/
    if (end == -1 && start == -1)
    {
        _log((char*)"NotFound !!\n");
        exit(-1);
    }

    /*****************************************************
     실제 태그의 값부분을 별도 변수에 Copy한다.
    *****************************************************/
    memset(realTag, 0x00, sizeof(realTag));
    memcpy(realTag, buff+start, end-start); 
    

    /*****************************************************
     이제 필요한 항목들을 읽어온다.
    *****************************************************/
    memset(query, 0x00, sizeof(query));
    getSubTag(realTag, (char*)"QUERY", query);

    memset(display, 0x00, sizeof(display));
    getSubTag(realTag, (char*)"DISPLAY", display);
    Ltrim(display);

    memset(checkName, 0x00, sizeof(checkName));
    getSubTag(realTag, (char*)"CHECKNAME", checkName);
    Ltrim(checkName);
    /****************************************************
     대문자로 변환해야 할 듯 해서리..
    ****************************************************/
    // PRJ-1678 : multi-byte character로 된 명령어는 없겠지...
    for (i=0; i< (int)strlen(checkName); i++)
         checkName[i] = (char)toupper(checkName[i]);

    memset(checkValue, 0x00, sizeof(checkValue));
    getSubTag(realTag, (char*)"CHECKVALUE", checkValue);
    Ltrim(checkValue);

    memset(compare, 0x00, sizeof(compare));
    getSubTag(realTag, (char*)"COMPARE", compare);
    Ltrim(compare);

    memset(action , 0x00, sizeof(action));
    getSubTag(realTag, (char*)"ACTION", action);

    memset(enable , 0x00, sizeof(enable));
    getSubTag(realTag, (char*)"ENABLE", enable);
    Ltrim(enable);

    /*****************************************************
     구조체에 저장해 둔다.
    *****************************************************/
    memcpy(xquery[index].tagName,    tagName,    strlen(tagName));
    memcpy(xquery[index].query,      query,      strlen(query));
    memcpy(xquery[index].checkName,  checkName,  strlen(checkName));
    memcpy(xquery[index].checkValue, checkValue, strlen(checkValue));
    memcpy(xquery[index].action,     action,     strlen(action));
    memcpy(xquery[index].enable,     enable,     strlen(enable));

    xquery[index].inc_count  = 0;
    xquery[index].old_value  = 0;
    xquery[index].display[0] = display[0];
    xquery[index].compare[0] = compare[0];


}




/***************************************
  실제 쿼리를 날려서 읽어온다.
***************************************/
void getData(int index)
{
    SQLSMALLINT columnCount=0, nullable, dataType, scale, columnNameLength;
    SQLLEN  *columnInd;
    SQLULEN columnSize;
    SQLCHAR     columnName[255];
    char        **columnPtr;
    char        _PCOM[1024];
    int rc;
    int i, rcount;
    char dData[8192];
    char aMsg[1024];



    /***************************************
      SQL 수행
    ***************************************/
    if (SQLExecDirect(stmt, (SQLCHAR*)xquery[index].query, SQL_NTS) != SQL_SUCCESS) 
    {
        dbErrMsg();
        return;
    } 


    /***************************************
      컬럼 갯수를 얻어온다.
    ***************************************/
    SQLNumResultCols(stmt, &columnCount);

    /***************************************
     이제 바인딩을 해야 함으로 필요한 메모리를 할당한다.
     컬럼수만큼.
    ***************************************/
    columnPtr = (char**) malloc( sizeof(char*) * columnCount );
    columnInd = (SQLLEN*) malloc( sizeof(SQLLEN) * columnCount );
    if ( columnPtr == NULL )
    {
        _log((char*)"getData.malloc Error\n");
        free(columnInd);
	    return ;
    }

    /***************************************
      바인딩한다.
    ***************************************/
    for ( i=0; i<columnCount; i++ )
    {
        memset(columnName, 0x00, sizeof(columnName));
        SQLDescribeCol(stmt, i+1, columnName, sizeof(columnName), &columnNameLength, &dataType, &columnSize, &scale, &nullable);

        /******************************************
          숫자형인경우 자릿수가 너무 작을수 있어
          15자리이하면 그냥 255자리로 할당한다.
        ******************************************/
        if (columnSize <= 15)
            columnSize = 255;

        columnPtr[i] = (char*) malloc( columnSize + 1 );
        SQLBindCol(stmt, i+1, SQL_C_CHAR, columnPtr[i], columnSize+1, &columnInd[i]);
    }

    /***************************************
      무슨항목을 찍는 태그를 출력한다.
    ***************************************/
    if (xquery[index].display[0] == '1')
        _log((char*)"[%s]\n", xquery[index].tagName);

    /***************************************
      fetch 하면서 출력한다.
    ***************************************/
    rcount = 0;
    while (1)
    {
        rc = SQLFetch(stmt);
        if (rc == SQL_NO_DATA) {
            break;
        } else if (rc != SQL_SUCCESS) {
                dbErrMsg();
                /***********************************
                  return하기전에 메모리 해제.
                ***********************************/
                for ( i=0; i<columnCount; i++ )
                {
                    free( columnPtr[i] );
                }
                free( columnPtr );
                free( columnInd );

                return;
        }

        /***************************************
          컬러명 = 실제값 형태로 찍는다. 한줄에 찍기 위해 변수에 담고 
          해당 변수를 찍는다. 쿼리 자체에 Alias를 쓰면 이쁘게 될듯.
        ***************************************/
        memset(dData, 0x00, sizeof(dData));
        memset(aMsg,  0x00, sizeof(aMsg));

        for ( i=0; i < columnCount; i++)
        { 
            memset(columnName, 0x00, sizeof(columnName));
            SQLDescribeCol(stmt, (i+1), columnName, sizeof(columnName), &columnNameLength, &dataType, &columnSize, &scale, &nullable);

            /*********************************************************
              컬럼명은 알고 있는 checkName과 같기 때문에
              같은게 나올때 checkvalue와 실제 DB에서 읽은거와 비교해서
              비교방식에 따라 조건이 맞으면 경고메시지를 뿌려준다.
            *********************************************************/
            if (memcmp((char*)columnName, xquery[index].checkName, strlen((char*)columnName)) == 0)
            {

                if (toupper(xquery[index].compare[0]) == 'E')
                {
                    if (memcmp((char*)columnPtr[i], xquery[index].checkValue, strlen(xquery[index].checkValue)) == 0)
                    {
                        sprintf(aMsg, "[%s.%s] current [%s] = checkValue [%s]", 
                                       (char*)xquery[index].tagName, (char*)xquery[index].checkName,
                                       (char*)columnPtr[i], xquery[index].checkValue);
                        sprintf(_PCOM, "%s 2>/dev/null", xquery[index].action);
                        system(_PCOM);
                    }
                }
                if (toupper(xquery[index].compare[0]) == 'G')
                {
                    if ( atol( (char*)columnPtr[i] ) > atol( xquery[index].checkValue) )
                    {
                        sprintf(aMsg, "[%s.%s] current [%s] > checkValue [%s]", 
                                       (char*)xquery[index].tagName, (char*)xquery[index].checkName,
                                       (char*)columnPtr[i], xquery[index].checkValue);
                        sprintf(_PCOM, "%s 2>/dev/null", xquery[index].action);
                        system(_PCOM);
                    }
                }
                if (toupper(xquery[index].compare[0]) == 'L')
                {
                    if ( atol( (char*)columnPtr[i] ) < atol( xquery[index].checkValue) )
                    {
                        sprintf(aMsg, "[%s.%s] current [%s] < checkValue [%s]", 
                                       (char*)xquery[index].tagName, (char*)xquery[index].checkName,
                                       (char*)columnPtr[i], xquery[index].checkValue);
                        sprintf(_PCOM, "%s 2>/dev/null", xquery[index].action);
                        system(_PCOM);
                    }
                }
            }


            /********************************************************
              출력할 값을 변수로 만든다.
            ********************************************************/
            strcat(dData,  (char*)columnName);
            strcat(dData,  "= [");
            strcat(dData,  columnPtr[i]);
            strcat(dData,  "]  ");
        }

        saveDB((char*)"0",  xquery[index].tagName,  dData);
        /********************************************
          DISPLAY 옵션에 따라 로깅여부 판단.
        ********************************************/
        if (xquery[index].display[0] == '1')
            _log((char*)"%s\n", dData);

        /********************************************
         ALARM 체크옵션이 있으면 이를 출력.
        ********************************************/
        if (aMsg[0] != 0x00)
        {
            /*****************************************
              경고시에는 모든걸 찍는다.
            *****************************************/
            if (xquery[index].display[0] != '1')
            {
                _log((char*)"[%s]\n", xquery[index].tagName);
                _log((char*)"%s\n", dData);
            }

            _log((char*)"[ALARM]:: %s\n", aMsg);

            /*****************************************
             원래 main에 줄바꿈이있지만 display=0이면 여기서 한다.
            *****************************************/
            if (xquery[index].display[0] != '1')
                _log((char*)"\n");
            saveDB((char*)"1",  xquery[index].tagName,  dData);
        }

        rcount++;
    }

    SQLFreeStmt(stmt, SQL_CLOSE);

    /***************************************
      아무것도 한게 없으면 없다 출력한다.
    ***************************************/
    if (xquery[index].display[0] == '1')
    {
       if (rcount == 0)
           _log((char*)"[NO_DATA]\n");
    }
	

    /***************************************
      메모리 해제하시라..
    ***************************************/
    for ( i=0; i<columnCount; i++ )
    {
        free( columnPtr[i] );
    }

    free( columnPtr );
    free( columnInd );
}








/***************************************
DB저장옵션을 활성화하면 directExec방법으로
처리를 수행한다.
***************************************/
int saveDB(char *logType, char *tagName, char *value)
{
    char query[65536];
 
    if (memcmp(DB_SAVE, "ON", 2) != 0 || siteName[0] == 0x00 )
        return -21;

    sprintf(query, "insert into altimon_log (sitename, intime, logType, tagName, tagValue) "
                   "values ('%s', to_char(sysdate, 'YYYYMMDDHHMISS'), '%s', '%s', trim('%s')) ",
                    siteName, logType, tagName, value);
    if (SQLExecDirect(stmt2, (SQLCHAR*)query, SQL_NTS) != SQL_SUCCESS)
    {
        dbErrMsg3();
        SQLFreeStmt(stmt2, SQL_CLOSE);
        return -1;
    }

    SQLFreeStmt(stmt2, SQL_CLOSE);

    return 1;
}



/***************************************
 configFile을 읽어서 모니터링 그룹셋에 대한 
 구조체를 생성시킨다.
***************************************/
void makeQuerySet()
{
    char realTag  [65536];
    char buff     [65536];
    char beginTag [255];
    char endTag   [255];
    char tmp      [8192];
    char name123  [255];
    char name321  [255];
    FILE *fp;
    int i, k, count;
    int start = -1, end = -1;
    int flagStart = 0;


    /**********************************************
      파일을 통째로 읽는다. 64K 이내에서.
    **********************************************/
    fp = fopen(configFile, "r");
    if (fp == NULL)
    {
        _log((char*)"makeQuerySet.fopen\n");
        exit(-1);
    }

    memset(buff, 0x00, sizeof(buff));
    fread(buff, sizeof(buff)-1, 1, fp);
    fclose(fp);

    /**********************************************
      전체를 통괄하는 MONITOR_QUERY_GROUP_SET
    **********************************************/
    sprintf(name123, "<%s>",   "MONITOR_QUERY_GROUP_SET");
    sprintf(name321, "</%s>",  "MONITOR_QUERY_GROUP_SET");
    k = (int)strlen(buff);
    start = -1;
    end   = -1;


    /**********************************************
     전체 태그의 시작, 끝이 어딘지 알아야 한다.
    **********************************************/
    for (i=0; i< k; i++)
    {
        if (memcmp(buff+i,  name123,  (int)strlen(name123)) == 0)
            start = i + (int)strlen(name123);

        if (memcmp(buff+i,  name321, (int)strlen(name321)) == 0)
            end = i  - 2;
    }


    /**********************************************
      잘못된거면 그냥 죽으시라.
    **********************************************/
    if (start == -1 ||  end == -1)
    {
        _log((char*)"Not Found %s!!!!\n", name123);
        exit(-1);
    }

    memset(realTag, 0x00, sizeof(realTag));
    memcpy(realTag, buff+start, end-start+1);

   
    memset(&xquery, 0x00, sizeof(xquery));
    memset(beginTag, 0x00, sizeof(beginTag));
    memset(endTag,   0x00, sizeof(endTag));
    memset(tmp, 0x00, sizeof(tmp));
    count = 0;
    QUERYSET_COUNT = 0;
    k = (int)strlen(realTag);


    /**********************************************
      일단, 약속하기를 아래와 같은 형태임으로
       <MainTAG> 
           <QUERY> </QUERY>
           <DISPLAY> </DISPLAY>
       </MainTAG>

       실제 여기서는 MainTAG명만 알아내기 위해 처리한다.
       상세 값들은 getQuery함수를 쓰신다.
    **********************************************/
    for (i=0; i< k; i++)
    {
        if (realTag[i] == '<')
        {
            memset(tmp, 0x00, sizeof(tmp));
            count = 0;
            flagStart = 1;
            continue;
        }
        if (flagStart == 1 && realTag[i] == '>')
        {

            /************************************************
              아래 단어들이면 무시하시라. tag를 추가할때마다 수정하셔야 한다.
            ************************************************/
            if (memcmp(tmp, "QUERY", 5) == 0       || memcmp(tmp, "/QUERY", 6) == 0       || 
                memcmp(tmp, "DISPLAY", 7) == 0     || memcmp(tmp, "/DISPLAY", 8) == 0     ||
                memcmp(tmp, "CHECKNAME", 9) == 0   || memcmp(tmp, "/CHECKNAME", 10) == 0  ||
                memcmp(tmp, "CHECKVALUE", 10) == 0 || memcmp(tmp, "/CHECKVALUE", 11) == 0 ||
                memcmp(tmp, "COMPARE", 7) == 0     || memcmp(tmp, "/COMPARE", 8) == 0     ||
                memcmp(tmp, "ACTION", 6) == 0      || memcmp(tmp, "/ACTION", 7) == 0      ||
                memcmp(tmp, "ENABLE", 6) == 0      || memcmp(tmp, "/ENABLE", 7) == 0 
               )
            {
                flagStart = 0;
                continue;
            }

            /************************************************
              BeginTag, endTag를 짝으로 비교해서 맞는지를 체크한다.
            ************************************************/
            if (beginTag[0] == 0x00)
            {
                memcpy(beginTag, tmp, strlen(tmp));
                //_log((char*)"beginTag = [%s]\n", beginTag);
            }
            else if (endTag[0] == 0x00) {
                memcpy(endTag, tmp+1, strlen(tmp)-1);
                //_log((char*)"endTag = [%s]\n", endTag);
            }
        
            if (beginTag[0] != 0x00 && endTag[0] != 0x00)
            {
                
                /************************************************
                  MainTag가 발견되었다면.. 상세항목을 저장해둔다.
                ************************************************/
                if (memcmp(beginTag, endTag, strlen(beginTag)) == 0)
                {
                    _log((char*)"mainTag =[%s]\n", beginTag);
                    getQuery(beginTag, QUERYSET_COUNT);
                    QUERYSET_COUNT++;
                    if (QUERYSET_COUNT >= 100) 
                        break;
                } 

                memset(beginTag, 0x00, sizeof(beginTag));
                memset(endTag, 0x00, sizeof(endTag));
            }
            flagStart = 0;
            continue;
        }

        tmp[count] = realTag[i];
        count++;
        if (count >= 1024) {
            memset(tmp, 0x00, sizeof(tmp));
            count = 0;
        }
    }


    _log((char*)"Total TagCount = [%d]\n", QUERYSET_COUNT);
}





/**********************************************
사용자가 callStack등을 뜨는게 불편하니까
그걸 쉽게 명령어 한방으로 만든다.
향후 조건절을 써서 이 또한 자동화되게 하면 좋겠다.
*********************************************/
void callStack()
{
    FILE *fp;
    char _PCOM[1024];
    char subDir[1024];
    char TEMP[1024];
    char PID[255];

    /**************************************
     필요한 callStack 실행을 위한 정보 수집.
    **************************************/
    memset(LOG_FILE,   0x00, sizeof(LOG_FILE));
    getSimpleParam ((char*)"ALTIMON_PROPERTY",   (char*)"LOG_FILE"   ,  LOG_FILE,  1);    


    /**************************************
     altimon.log 의 경로에 출력하기 위해 dirName을 알아낸다.
    **************************************/
    memset(TEMP,   0x00, sizeof(TEMP));
    memset(subDir, 0x00, sizeof(subDir));
    sprintf(_PCOM, "dirname %s", LOG_FILE);
    runCommand(_PCOM, TEMP);
    memcpy(subDir, TEMP, strlen(TEMP));



    /**************************************
     UID, UNAME, PID를 얻어내셔라. 염병, 함수로 했어야지..
    **************************************/
    memset(UID, 0x00, sizeof(UID));
    runCommand((char*)"id | awk '{print $1}' | sed 's/(/ /g' | sed 's/)/ /g' | awk '{print $2}'", UID);

    memset(UNAME, 0x00, sizeof(UNAME));
    runCommand((char*)"uname", UNAME);

    sprintf( _PCOM,  "ps -ef|grep \"altibase -p boot from\" | grep -v grep |grep %s|awk '{print $2}'", UID);
    memset( PID, 0x00, sizeof(PID));
    runCommand( _PCOM, PID);


    if ((int)strlen(PID) == 0)
    {
        _log((char*)"Altibase Die !!, Can't find ProcessID\n");
        return;
    }


    /**************************************
     실제 OS에 맞는 방식으로 STACK를 뜬다.
    **************************************/
    if (memcmp(UNAME, "SunOS", 5) == 0)
    {
        sprintf(_PCOM, "prstat -p %s -L 1 1 > %s/thread.list.`date +%%Y%%m%%d_%%H%%M%%S`", PID, subDir);
        system(_PCOM);

        sprintf(_PCOM, "pstack -F %s  > %s/stack.trace.`date +%%Y%%m%%d_%%H%%M%%S`", PID, subDir);
        system(_PCOM);
    }

    if (memcmp(UNAME, "AIX", 3) == 0)
    {
        sprintf(_PCOM, "ps -p %s -mo THREAD > %s/thread.list.`date +%%Y%%m%%d_%%H%%M%%S`", PID, subDir);
        system(_PCOM);

        sprintf(_PCOM, "procstack -F %s  > %s/stack.trace.`date +%%Y%%m%%d_%%H%%M%%S`", PID, subDir);
        system(_PCOM);

    }

    if (memcmp(UNAME, "HP-UX", 5) == 0 || memcmp(UNAME, "Linux", 5) == 0)
    {
        fp = fopen("gdb.sh", "w+");
        if (fp != NULL)
        {
            fprintf(fp , "gdb $ALTIBASE_HOME/bin/altibase %s << EOF_\n", PID);
            fprintf(fp , "set height 50000\n");
            fprintf(fp , "thread apply all bt\n" );
            fprintf(fp , "detach all\n");
            fprintf(fp , "y\n");
            fprintf(fp , "EOF_\n");
            fclose(fp);
            sprintf(_PCOM, "sh gdb.sh > %s/stack.trace.`date +%%Y%%m%%d_%%H%%M%%S`", subDir);
            system(_PCOM);
        }
    }

    _log((char*)"COMPLETED  GATHERING CALL_STACK INFO. CHECK ALTIMON_LOGDIR\n");
}






/**********************************************
altimon.log Analanyzer
주어진 기간에 대해서 입력받은 tagName에 대한 기록을
발췌해서 파일로 만든다.
*********************************************/
void logAnal(int tagCount)
{
    FILE *fp, *fp2, *fp3;
    char _PCOM[1024];
    char baseName[1024];
    char subDir[1024];
    char TEMP[8192];
    char fname[1024];
    char aFileName[1024];
    char logValue[255];
    char buff[1024];
    char tTime[50];
    int  i, j, k, noPlay = 0;
    int  start = -1, end = -1, endTag = -1;
    double Min[255], Max[255], Sum[255], Cnt[255];



    for (i=0;i<tagCount;i++)
    {
        Max[i] = (double)-9999999999; Min[i] = (double)9999999999; Sum[i] = 0; Cnt[i] = 0;
        printf("Anal tagName = [%s]\n", analTag[i]);
    }

    /**************************************
     필요한 callStack 실행을 위한 정보 수집.
    **************************************/
    memset(LOG_FILE,   0x00, sizeof(LOG_FILE));
    getSimpleParam ((char*)"ALTIMON_PROPERTY",   (char*)"LOG_FILE"   ,  LOG_FILE,  1);    


    /**************************************
     altimon.log 경로에 출력하기 위해 dirName을 알아낸다.
    **************************************/
    memset(TEMP,   0x00, sizeof(TEMP));
    memset(subDir, 0x00, sizeof(subDir));
    sprintf(_PCOM, "dirname %s", LOG_FILE);
    runCommand(_PCOM, TEMP);
    memcpy(subDir, TEMP, strlen(TEMP));

    /**************************************
     altimon.log 의 baseName을 알아낸다.
    **************************************/
    memset(TEMP,     0x00, sizeof(TEMP));
    memset(baseName, 0x00, sizeof(baseName));
    sprintf(_PCOM, "basename %s", LOG_FILE);
    runCommand(_PCOM, TEMP);
    memcpy(baseName, TEMP, strlen(TEMP));


    /**************************************
     altimon.log의 list를 시간순으로 만들어낸다.
     일단 이작업은 주석으로 하고 수작업으로 만들어내도록 한다.
    **************************************/
    memset(TEMP, 0x00, sizeof(TEMP));
    sprintf(_PCOM, "ls -lrt %s/%s* | awk '{print $9}' > %s/altimon.list ", subDir, baseName, subDir);
    runCommand(_PCOM, TEMP);


    /**************************************
     altimon.list 파일을 OPEN한다.
    **************************************/
    sprintf(fname, "%s/altimon.list", subDir);
    fp = fopen(fname, "r");
    if (fp == NULL)
    {
        printf("Can't find file. [%s]\n", fname);
        exit(-1);
    }


    while (!feof(fp))
    {

        memset(TEMP, 0x00, sizeof(TEMP));
        if (!fscanf(fp, "%s\n", TEMP))
             break; 

        /******************************************
          scan할 작업파일을 OPEN한다.
        ******************************************/
        sprintf(aFileName, "%s", TEMP);
        fp2 = fopen(aFileName, "r");
        if (fp2 == NULL)
        {
            printf("Can't open file [%s]\n", aFileName);
            continue;
        }
printf("[%s] reading.. wait plz.\n", aFileName);


        memset(TEMP, 0x00, sizeof(TEMP));
        while (!feof(fp2))
        {

            /******************************************
             한줄씩 읽읍시다. 아 simple하다. 쩝..
            ******************************************/
            memset(buff, 0x00, sizeof(buff)); 
            if (fgets(buff, sizeof(buff)-1, fp2) == NULL)
                 break;

            /******************************************
             timeStamp가 없으면 위로.
            ******************************************/
            if ((int)strlen(buff) < 20)
                 continue;

            /******************************************
             로깅하는 시점을 ALTMON END 가 나오면 한다.
            ******************************************/
            endTag = 0;
            for (i=0;i<(int)strlen(buff);i++)
            {
                if (memcmp("ALTIMON CHECK ENDED", buff+i, (int)strlen((char*)"ALTIMON CHECK ENDED")) == 0)
                    endTag = 1;
            }
            if (endTag == 1)
            {
                memset(tTime, 0x00, sizeof(tTime)); 
                memcpy(tTime, buff+1, 19);

                if (TEMP[0] != 0x00)
                {
                    fprintf(fp3, "%s %s\n", tTime, TEMP);
                    fflush(fp3);
                }
                memset(TEMP, 0x00, sizeof(TEMP));
                continue;
            }

            /******************************************
             읽어들인 문자열내에 ALARM 문자열이 존재하면 Skip!!
            ******************************************/
            noPlay = 0;
            for (i=0; i < (int)strlen(buff); i++)
            {
                if (memcmp(buff+i,  "[ALARM]::", (int)strlen("[ALARM]::")) == 0)
                {
                    noPlay = 1;
                    break;
                }
            }
            if (noPlay == 1) 
                continue;
            
           
            /******************************************
             읽어들인 문자열내에 tagName이 존재하는지 체크
            ******************************************/
            for (k=0; k<tagCount; k++)
            {
                for (i=0; i < (int)strlen(buff); i++)
                {
                    if (memcmp(buff+i,  analTag[k],   (int)strlen(analTag[k])) == 0)
                    {
                        for (j=i; j< (int)strlen(buff); j++)
                        {
                            /****************************************
                             [] 안에 값이 있기에 찾긴 하는데 ] 를 만나면 나가야 한다.
                             그렇지 않으면 한줄에 여러개 있을때에는 찾을길이 없당.
                            ****************************************/
                            if (buff[j] == '[')
                                start = j+1;

                            if (buff[j] == ']' ) 
                            {
                                end = j-1;
                                break;
                            }
                        }
                 
                        memset(logValue, 0x00, sizeof(logValue));
                        if (end >= start)
                            memcpy(logValue, buff+start, (end-start)+1);

                        Sum[k] = Sum[k] + atof(logValue);

                        if (atof(logValue) < Min[k])
                            Min[k] = atof(logValue); 

                        if (atof(logValue) > Max[k])
                            Max[k] = atof(logValue); 

                        Cnt[k] = Cnt[k] + 1;

                    } /* if memcmp */

                } /* for i */

            } /* for k */            

        } /* !feof */

        fclose(fp2);
    }
    fclose(fp);

printf("Job end, Check LogFile [%s/altimon.anal]\n", subDir);

    /**************************************
     기록을 저장할 파일을 OPEN한다.
    **************************************/
    sprintf(fname, "%s/altimon.anal", subDir);
    fp3 = fopen(fname , "w+");
    if (fp3 == NULL)
    {
        printf("Can't open file. [%s]\n", fname);
        exit(-1);
    }

    for (i=0; i<tagCount; i++)
    {
        if (Cnt[i] > 0)
        {
            fprintf(fp3, "[%s] dataRow = %-15.0f\n"
                         "  --> Min=%-15.2f,   Max=%-15.2f,    Avg=%-15.2f\n\n",
                      analTag[i], Cnt[i], Min[i], Max[i], Sum[i] / Cnt[i]);
        } else {
            fprintf(fp3, "[%s] dataRow = 0\n\n", analTag[i]);
        }
    }

    fclose(fp3);
    
}






/**********************************************
사용자가 지정한 디스크 정보. 최대 10개
**********************************************/
void getDiskTag()
{
    int  i;
    char tagName[255];

    
    memset(DISK_DIR,   0x00, sizeof(DISK_DIR));
    memset(DISK_USAGE, 0x00, sizeof(DISK_USAGE));

    DISK_COUNT = 0;

    for (i=0;i<10;i++)
    {
        /****************************************
         볼륨명 정보 얻기
        ****************************************/
        sprintf(tagName, "DISK%d", i+1);
        getSimpleParam((char*)"OS_QUERY_GROUP_SET",  tagName,   DISK_DIR[i],   1);
        if (DISK_DIR[i][0] == 0x00)
            break;

        /****************************************
         해당 볼륨명의 한계치 설정 정보 얻기
        ****************************************/
        sprintf(tagName, "DISK%d_USAGE", i+1);
        getSimpleParam((char*)"OS_QUERY_GROUP_SET",  tagName,   DISK_USAGE[i], 1);

        _log((char*)"DISK_TAG = [%s], Limit = [%s]\n", DISK_DIR[i], DISK_USAGE[i]);
        DISK_COUNT++;
    }

    memset(DISK_ACT, 0x00, sizeof(DISK_ACT));
    getSimpleParam((char*)"OS_QUERY_GROUP_SET",  (char*)"DISK_ACT",   DISK_ACT, 0);
    
}






/**********************************************************
 Client Socket 처리
 Output은 항상 3자리의 길이 + 실제값 형태로 string으로 반송한다.

#REQ001 : altibase CPU, VSZ usage             
#REQ002 : system Memory Free, Swap Free, Idle
#REQ997 : server start (%03dUSER%03dPASSWD) : input에 user+passwd를 필요
#REQ998 : server stop  (%03dUSER%03dPASSWD) : input에 user+passwd를 필요
***********************************************************/
void clnt_connection(int arg)
{
    int clnt_sock = arg;
    char rcvMsg[1024];
    char *sndMsg, *ret;
    char REQUEST[6+1];
    char comm[1024];
    FILE *fp;


    sndMsg = (char*)malloc(sizeof(char)*ALLOC_SIZE);
    ret = (char*)malloc(sizeof(char)*ALLOC_SIZE);

    if (!sndMsg || !ret)
    {
        _log((char*)"malloc Error\n");
        exit(-1);
    }

    memset(sndMsg, 0x00,  ALLOC_SIZE);
    memset(ret,    0x00,  ALLOC_SIZE);

    /*********************************************
     read Socket
    *********************************************/
    if (read(clnt_sock, rcvMsg, sizeof(rcvMsg)-1) < 0)
    {
        _log((char*)"Read Error\n");
        free(sndMsg);
        free(ret);
        return;
    }

    /*********************************************
     앞의 6자리는 구분코드로 사용한다.
    *********************************************/
    memset(REQUEST, 0x00, sizeof(REQUEST));
    memcpy(REQUEST,  rcvMsg, 6);
_log((char*)"REQUEST_TCP_LOG = %s\n", REQUEST);

    memset(sndMsg, 0x00, sizeof(sndMsg));
    if (memcmp(REQUEST, "REQ001", 6) == 0)
    {
        sprintf(sndMsg, "%03d%s%03d%s%03d%s", (int)strlen(gPID), gPID,
                                              (int)strlen(gCPU), gCPU, 
                                              (int)strlen(gVSZ), gVSZ);
    }
  
    if (memcmp(REQUEST, "REQ002", 6) == 0)
    {
        sprintf(sndMsg, "%03d%s%03d%s%03d%s",   (int)strlen(SYSTEM_FREEMEM),  SYSTEM_FREEMEM,
                                                (int)strlen(SYSTEM_FREESWAP), SYSTEM_FREESWAP,
                                                (int)strlen(SYSTEM_IDLE),     SYSTEM_IDLE);
    }

    if (memcmp(REQUEST, "REQ990", 6) == 0)
    {
        memset(ret, 0x00, sizeof(ret));
        if (memcmp(gUNAME, "SunOS", 5) == 0)
            sprintf(comm, "prstat -L -p %s 1 1" , gPID);

        runCommand(comm, ret);
		memcpy(sndMsg, ret, strlen(ret));
    }

    if (memcmp(REQUEST, "REQ991", 6) == 0)
    {
        memset(ret, 0x00, sizeof(ret));
        if (memcmp(gUNAME, "SunOS", 5) == 0)
        	sprintf(comm, "pstack -F %s | c++filt" , gPID); 
		if (memcmp(gUNAME, "AIX", 3) == 0)
			sprintf(comm, "procstack -F %s", gPID);
        if (memcmp(gUNAME, "HP-UX", 5) == 0 || memcmp(gUNAME, "Linux", 5) == 0)
        {
        	fp = fopen("gdb.sh", "w+");
        	if (fp != NULL)
        	{
            	fprintf(fp , "gdb $ALTIBASE_HOME/bin/altibase %s << EOF_\n", gPID);
            	fprintf(fp , "set height 50000\n");
            	fprintf(fp , "thread apply all bt\n" );
            	fprintf(fp , "detach all\n");
            	fprintf(fp , "y\n");
            	fprintf(fp , "EOF_\n");
            	fclose(fp);
            	sprintf(comm, "sh gdb.sh");
        	}
        }
        runCommand(comm, ret);
        memcpy(sndMsg, ret, strlen(ret));
    }

    if (memcmp(REQUEST, "REQ997", 6) == 0)
    {
        _log((char*)"REMOTE Server request [Server Start]\n");
        system("nohup server start 1> /dev/null 2>&1 &");
        sprintf(sndMsg, "command execute !!");
    }

/*
    if (memcmp(REQUEST, "REQ998", 6) == 0)
    {
        _log((char*)"REMOTE Server request [Server Stop]\n");
        system("nohup server stop 1> /dev/null 2>&1 &");
        sprintf(sndMsg, "command execute !!");
    }
*/

    if (send(clnt_sock, sndMsg, (int)strlen(sndMsg), 0) < 0)
    {
        _log((char*)"Send Error\n");
        free(sndMsg);
        free(ret);
        return;
    }

    free(sndMsg);
    free(ret);
}





/**********************************************
log delete batch
*********************************************/
void executeBatch()
{
    char query[1024];
    SQLINTEGER ret;

    dbConnect();

    sprintf(query, "delete from altimon_log where intime < to_char(sysdate - %s, 'yyyymmddhhmiss')", duration);
_log((char*)"BATCH=[%s]\n", query);
    ret = SQLExecDirect(stmt, (SQLCHAR*)query, SQL_NTS) ;
    if (ret != SQL_NO_DATA && ret != SQL_SUCCESS)
    {
        dbErrMsg();
_log((char*)"BATCH_ERROR\n");
    }else{
        BATCH_EXEC = 0;
_log((char*)"BATCH_SUCCESS\n");
    }

    dbFree();

}




/**********************************************
MAIN FUNCTION
*********************************************/
int main(int argc, char *argv[])
{
    pthread_mutex_t mutex;
/*
    pthread_t thread_id;
*/
    char TEMP[1024];
    char _PCOM[1024];
    int i;


    /*************************************
      환경변수 적용여부 확인.
    *************************************/
    if (getenv(IDP_HOME_ENV) == NULL)
    {
        printf("Please, Set environmentVariable ALTIBASE_HOME \n");
        exit(-1);
    }

    /*************************************
      configFile은 어디에??
    *************************************/
    sprintf(configFile, "%s/conf/altimon.conf", (char*)getenv(IDP_HOME_ENV));


    /*************************************
      start , stop 처리
    *************************************/
    if (argc < 2)
    {
        printf("Usage] altimon [start | stop]\n");
        exit(-1);
    }

    /*************************************
     start를 수행시 이미 떠 있는지를 체크한다.
    *************************************/
    if (memcmp(argv[1], "start", 5) == 0 )
    {
        sprintf(_PCOM, "ps -ef|grep altimon|grep -v tail| grep -v vi|grep -v grep | grep -v %d | wc -l", getpid());
        runCommand(_PCOM, TEMP);
        if ((int)atoi(TEMP) > 0)
        {
            printf("altimon already started.!!\n");
            exit(-1);
        }
    }



    /*************************************
      start, stop, stack에 대한 처리
    *************************************/
    if (memcmp(argv[1], "start" , 5) != 0 && 
        memcmp(argv[1], "stop",   4) != 0 && 
        memcmp(argv[1], "stack",  5) != 0 && 
        memcmp(argv[1], "anal",   4) != 0
       )
    {
        printf("Usage] altimon [start | stop]\n");
        exit(-1);
    }

    /*************************************
      stop에 대한 처리
    *************************************/
    if (memcmp(argv[1] , "stop", 4) == 0)
    {
        memset(TEMP, 0x00, sizeof(TEMP));
        sprintf(_PCOM, "kill -9 `ps -ef|grep altimon|grep -v tail| grep -v vi|grep -v grep|grep -v %d|awk '{print $2}'` > /dev/null 2>&1", getpid());
        runCommand(_PCOM, TEMP);
        exit(-1);
    }

    /*************************************
      stack에 대한 처리
    *************************************/
    if (memcmp(argv[1], "stack", 5) == 0)
    {
        callStack();
        exit(-1);
    }

    /*************************************
      anal에 대한 처리
    *************************************/
    if (memcmp(argv[1], "anal", 4) == 0)
    {
        memset(analTag, 0x00, sizeof(analTag));
        for (i=2; i<argc; i++)
            memcpy(analTag[i-2], argv[i], strlen(argv[i]));

        logAnal(argc-2);
        exit(-1);
    }

    /*************************************
      ProcessChk에서 필요하기 때문에..
    *************************************/
    if (getenv("UNIX95") == NULL)
    {
        printf("Please, Execute ""setenv UNIX95 1"" or ""export UNIX95=1"" \n");
        exit(-1);
    }


    /*************************************
      일단, 필요한 항목을 얻어오자.
    *************************************/
    memset(LOG_FILE,   0x00, sizeof(LOG_FILE));
    memset(SLEEP_TIME, 0x00, sizeof(SLEEP_TIME));
    memset(CPU_USAGE,  0x00, sizeof(CPU_USAGE));
    memset(CPU_ACT  ,  0x00, sizeof(CPU_ACT));
    memset(MEM_USAGE,  0x00, sizeof(MEM_USAGE));
    memset(LSOF_COM,   0x00, sizeof(LSOF_COM));
    memset(LSOF_LIMIT, 0x00, sizeof(LSOF_LIMIT));
    memset(DATE_FORM,  0x00, sizeof(DATE_FORM));
    memset(ALARM_FILE, 0x00, sizeof(ALARM_FILE));
    memset(DB_SAVE,    0x00, sizeof(DB_SAVE));

    memset(DISK_CHK_ENABLE, 0x00, sizeof(DISK_CHK_ENABLE));
    

    getSimpleParam ((char*)"ALTIMON_PROPERTY", (char*)"LOG_FILE" ,   LOG_FILE,   1);    
    getSimpleParam ((char*)"ALTIMON_PROPERTY", (char*)"SLEEP_TIME",  SLEEP_TIME, 1);
    getSimpleParam ((char*)"ALTIMON_PROPERTY", (char*)"DATE_FORMAT", DATE_FORM,  1);
    getSimpleParam ((char*)"ALTIMON_PROPERTY", (char*)"ALARM_FILE",  ALARM_FILE, 1);
    getSimpleParam ((char*)"ALTIMON_PROPERTY", (char*)"DB_SAVE",     DB_SAVE,    1);
_log((char*)"DB_SAVE= [%s]\n", DB_SAVE);
_log((char*)"ALARM_FILE= [%s]\n", ALARM_FILE);

    getSimpleParam ((char*)"OS_QUERY_GROUP_SET", (char*)"CPU_USAGE",  CPU_USAGE, 1);
    getSimpleParam ((char*)"OS_QUERY_GROUP_SET", (char*)"CPU_ACT",    CPU_ACT,   0);
    chkMemset(CPU_ACT);

    getSimpleParam ((char*)"OS_QUERY_GROUP_SET", (char*)"MEM_USAGE",  MEM_USAGE, 1);
    getSimpleParam ((char*)"OS_QUERY_GROUP_SET", (char*)"MEM_ACT",    MEM_ACT,   0);
    chkMemset(MEM_ACT);


    getSimpleParam((char*)"OS_QUERY_GROUP_SET", (char*)"LSOF_COM",   LSOF_COM,   1);
    getSimpleParam((char*)"OS_QUERY_GROUP_SET", (char*)"LSOF_LIMIT", LSOF_LIMIT, 1);

    getSimpleParam((char*)"OS_QUERY_GROUP_SET", (char*)"DISK_CHK_ENABLE", DISK_CHK_ENABLE, 1);
     
    memset(siteName, 0x00, sizeof(siteName));
    memset(duration, 0x00, sizeof(duration));
    getSimpleParam ((char*)"REPOSITORY_INFO", (char*)"SITE_NAME" ,  siteName,1);
    getSimpleParam ((char*)"REPOSITORY_INFO", (char*)"DURATION"  ,  duration,1);
_log((char*)"SITE_NAME= [%s]\n", siteName);
_log((char*)"DURATION = [%s]\n", duration);

    /*************************************
      디스크 사용량을 확인하기 위해
    *************************************/
    getDiskTag();

    
    /*************************************
      일단, 필요한 OS정보들..
    *************************************/
    memset(UNAME, 0x00, sizeof(UNAME));
    runCommand((char*)"uname", UNAME);
    memset(gUNAME, 0x00, sizeof(gUNAME));
    memcpy(gUNAME, UNAME, strlen(UNAME));
    _log((char*)"UNAME = [%s]\n", UNAME);

    memset(SHELL, 0x00, sizeof(SHELL));
    runCommand((char*)"basename `echo $SHELL`", SHELL);
    _log((char*)"SHELL = [%s]\n", SHELL);

    memset(UID, 0x00, sizeof(UID));
    runCommand((char*)"id | awk '{print $1}' | sed 's/(/ /g' | sed 's/)/ /g' | awk '{print $2}'", UID);
    _log((char*)"UID = [%s]\n", UID);


    /*************************************
      DB연결해보고 필요정보들 가져오기
    *************************************/
    if ((int)dbConnect() != 1)
    {
        printf("Can't Connect DB , check errorLog [%s]\n", LOG_FILE);
        exit(-1);
    }
   
    dbFree();

    /*************************************
      모니터링을 위해 필요한것들을 셋팅한다.
    *************************************/
    makeQuerySet();


    /*************************************
     make daemon
    *************************************/
    if (fork() > 0)
        exit(0);

    setsid();
    umask(0);

    /*************************************
     통신쓰레드 생성.
    *************************************/
    if (pthread_mutex_init(&mutex, NULL))
    {
        _log((char*)"Can't initialize mutex\n");
        exit(1);
    }

    /*************************************
     Main Start
    *************************************/
    while (1)
    {
        if (dbConnect() == -1)
        {
            dbFree();
            sleep((int)atoi(SLEEP_TIME));
            continue;
        }

        _log((char*)"================================= ALTIMON CHECK START ======================================\n");
        /*******************************************
           process status display
        *******************************************/
        vmstatChk();
        systemChk();
        processChk();

        /*******************************************
         디스크사용체크를 할때에만 수행한다.
        *******************************************/
        if (memcmp(DISK_CHK_ENABLE, "ON", 2) == 0)
            diskChk();

        /*******************************************
           읽어들인 MainTag들에 대한 수행을 처리한다.
        *******************************************/
        for (i=0; i< QUERYSET_COUNT;i++ )
        {
            if (memcmp(xquery[i].enable, "ON", 2) != 0)
                continue;

            getData(i);

            if (xquery[i].display[0] == '1')
                _log((char*)"\n");
        }
        dbFree();

        _log((char*)"================================= ALTIMON CHECK ENDED ======================================\n\n");

        if (memcmp(DB_SAVE, "ON", 2) == 0 && BATCH_EXEC == 1)
            executeBatch();

        /*******************************************
          사용자가 지정한 지정한 시간만큼 Sleep
        *******************************************/
        sleep((int)atoi(SLEEP_TIME) - 1);
    }

/*
    if (pthread_join(thread_id, NULL))
        _log((char*)"Error waiting for thread %d to terminate\n", thread_id);
*/

}

