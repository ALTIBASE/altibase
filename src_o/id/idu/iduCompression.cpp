/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduCompression.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/



/***********************************************************************
 * 이 코드에 대한 정보는 (doc-30624) 에서 볼 수 있다.
 **********************************************************************/

/***********************************************************************
 *
 * 용어정의 :
 *
 * literal  = 이전과 매치되지 않는 문자열
 * match    = 이전과 매치되는 문자열, offset, length가 표시된다.
 *          그 offset의 크기에 따라 m2, m3, m4 match로 나뉘어 진다.
 * 헤더      =  원본에 대해 현재 압축파일이 어떻게 압축이 되어진것인지를 나타내는 정보
 *            literal헤더와 match헤더가 존재하고, 예외적인 헤더로 아무런 압축되 되어있지 않음을
 *            나타내는 순결헤더와 종료를 표시하는 종료 헤더가 있다.
 * offset   = match에서 쓰이는 용어로, 현재 의 위치와 매치된 예전의 단어의 위치와의
 *            차이를 나타낸다.
 * len 또는 length = match에서 쓰이는 용어로, 매치된 단어의 길이를 나타낸다.
 **********************************************************************/


/***********************************************************************
 * 압축 알고리즘 설명
 *
 * 현재의 단어가 이전에 이미 나온 단어라면 그 단어를 적지 않고 이전의 위치와 얼마나
 * 길게 match하는지 그 정보를 나타낸다. 여기에서 압축이 일어난다.
 * 이때 이전의 어떠한 단어와도 매치되지 않는 단어를 literal이라고 하고, 이전의 단어와
 * 일치되는 단어를 match고 한다.
 * 기본적으로 3byte이상의 단어가 match될 경우 match가 된다.
 * 그 이유는 match에 쓰이는 헤더의 크기 때문인데 만약 헤더의 크기가 2byte고 match되는
 * 정보가 2byte라면 오히려 압축은 손해를 보고 압축 시간만 소비하게 된다.
 * 때문에 압축 알고리즘에서 매치는 절대로 압축되는 양이 줄거나 늘지 않는다. 하지만
 * literal도 별도의 헤더를 가지고 있고, literal은 전혀 압축이 되지 않기 때문에
 * literal 헤더 만큼이 오버헤드가 되게 된다.
 * 매치 헤더에는 이전과의 offset, 얼마나 매치되는지 길이 정보가 있다.
 *
 * literal에는 앞으로 몇byte의 literal이 나오는지 정보가 나타나 있다. 때문에
 * literal 다음에는 또 literal이 나오지 않는다.
 * 하지만 match다음에는 또 match가 나올 수 있다.
 **********************************************************************/


/***********************************************************************
 * 압축된 버퍼의 형식
 *
 * 일단 가장 처음에는 literal이 온다.  literal 다음은 무조건 match가 나온다.
 * match이후에는 다른 match가 나올 수도 있고 literal이 나올 수도 있다. 종료는 literal에서
 * 종료 될 수도 있고 match에서 종료될 수도 있다. 모든 압축이 끝난 이후에는 종료 헤더를 적는다.
 **********************************************************************/

/***********************************************************************
 *     srcBuf : aaaa bbbb cccc dddd aaaa bbbb cccc ....
 * [해시테이블]        ^현재위치
 * [        ]
 * [        ]
 * [        ]
 * [        ]
 * [        ]
 * [        ]
 *
 * 여기서 해시테이블은 임시로 쓰이는 자료구조이다.  해시테이블은 srcBuf내의 정보에 대한
 * 포인터 정보가 들어있다. 즉, 해시 테이블의 용도는 매치되는 이전 단어가 현재 srcBuf의
 * 어느 위치에 있는지를 저장하기 위해 쓰이는 정보이다. 하지만 이것은 위치만을 나타내지는
 * 않는다. 그것은 이 테이블에 저장된 정보가 포인터임을 상기하면 알 수 있다.
 * 즉, 실제 정보를 복사하여 가지고 있지 않아도 그 위치정보 뿐 아니라 실제 데이터 정보도 알 수
 * 있다. 해시테이블의 키는 현재위치에서 4개의 data를 가지고 xor연산을 이용하여 구한다.
 * 이 키를 이용하여 해시테이블의 몇번째 인덱스에 들어가는지 그 인덱스 정보를 알아 낼 수있다.
 * 만약 그곳에 어떠한 포인터가 있고, 그 포인터를 통해 접근하여 구한 실제 자료가 현재
 * 내가 가지는 자료와 3바이트 이상 일치할 경우, match는 성공하게 된다.
 * 만약 match가 실패할 경우, 생성된 키를 바탕으로 해시테이블에 현재의 위치(포인터)를
 * 저장하게 된다.
 * 즉, hashTable[index(현재위치[0-3])] = 현재위치
 * 이것이 literal이 되는 것이다. 하지만 literal은 여러개를 묶어서 한꺼번에 써야
 * 하므로, literal은 실제 쓰이지 않고, 현재위치만 이동하게 된다.
 * 나중에 match가 일어나면 그때 지금까지 모아두었던 literal을 한꺼번에 쓰게된다.
 * 그렇기 때문에 literal은 절대로 연이어서 나오지 않는다.
 **********************************************************************/


/***********************************************************************
 * 압축 예)
 *
 * srcBuf: 압축을 하려하는 버퍼
 * aaaa bbbb cccc dddd aaaa bbbb cccc dddd zzaa bbcds sdlkajsfd aa
 *
 * destBuf: 압축된 결과가 나타나는 버퍼
 * [L 6]aaaa b[M offset= 1 len= 3][L 7] cccc d[M offset= 1 len= 3][L 5] aaaa
 * [M offset= 20 len= 16][L 2]zz[M offset= 20 len= 5][L 17]cds sdlkajsfd aa
 *
 * 설명: 여기서 L은 literal을 나타내고, M은 MATCH를 나타낸다.
 * 처음으로 매치가 이루어 지기 까지의 과정을 설명 하겠다.
 *
 *
 * 1. 처음에는 현재위치를 +4한 위치에서 시작하게 된다.
 * 즉,
 *
 * aaaa bbbb cccc dddd aaaa bbbb cccc dddd zzaa bbcds sdlkajsfd aa
 *     ^현재위치
 *
 * 이때, " bbb" 으로 해시 키(index)를 생성한다. 그런후 가령 7이라는 값이 나왔다고 하면,
 *
 * 이전 단어 <= hashtable[index];
 * if 이전단어[0~2] == 현재위치[0~2] then
 *      goto match;
 * else
 *      hashtable[index] <= 현재위치
 *
 * 이런 방법을 거치게 된다. hashtable에는 아무런 값도 들어있지 않을 것이므로, else
 * 가 된다. 즉, hashTable[7] <= 현재위치
 *
 * 그런후 현재 위치++, 즉,
 * aaaa bbbb cccc dddd aaaa bbbb cccc dddd zzaa bbcds sdlkajsfd aa
 *      ^현재위치
 * 이곳을 가리키게 된다.
 *
 *
 *
 * 2. "bbbb"로 키를 생성한다. 9가 나왔다고 하자. 위의 방법을 거쳐서
 *      hashTable[7] <= 현재위치 가 되고 현재위치++ 된다.
 * aaaa bbbb cccc dddd aaaa bbbb cccc dddd zzaa bbcds sdlkajsfd aa
 *       ^현재위치
 *
 *
 *
 * 3. "bbb "로 키를 생성한다. 이값이 9가 나왔다. 9는 바로 이전에 나온 그 값이다.
 *    이전 주소와 현재 주소와의 차이는 1이다. 또한 이전의 저장된 정보는
 *    bbbb cccc dddd aaaa bbbb cccc dddd zzaa bbcds sdlkajsfd aa
 *    현재의 정보는
 *    bbb cccc dddd aaaa bbbb cccc dddd zzaa bbcds sdlkajsfd aa
 *    이므로 현재 저장된 정보에서 3개가 match하게 된다.
 *    그래서 offset=1, len=3의 정보가 match헤더에 쓰이게된다.
 *    하지만 match헤더가 쓰이기전에 이전에 쓰지않고 그냥 위치만 이동 시켰던 literal에
 *    대한 정보도 헤더와 함께 match보다 먼저 쓰이게 된다.
 *    즉, 이때 생성되는 정보는
 *
 *    [L 6]aaaa b[M offset= 1 len= 3]   이된다.
 *
 * 4. 이후, 같은 방법으로 진행하게 된다.
 *
 *
 *
 *압축해제의 예)
 * 1.  [L 6]aaaa b[M offset= 1 len= 3] 의 압축을 해제 할때, 우선 첫 바이트를 읽어서
 *     이것이 literal이고, 총 6개가 있음을 알아낸다. 그런후 앞으로 6개는 그대로
 *     버퍼에 쓴다.
 *     즉,
 *     destBuf에 "aaaa b"가 쓰이게된다.
 *
 *
 * 2.  srcBuf는  다음 부분인 "[M offset= 1 len= 3]" 부분으로 넘어간다.
 *     offset이 1이므로 '현재 위치-1'의 위치부터 3만큼 현재 위치에 복사하면 된다.
 *     하지만 얼핏 생각하면 '현재 위치-1'+3은 쓸데없는 정보를 읽을 것이라고 생각할 수 있다.
 *     하지만 복사의 source와 target이 모두 같은 버퍼임을 상기하라.
 *     즉, 복사가 수행되면서
 *     "aaaa b"          "aaaa b"
 *            ^현재위치         ^현재위치-1
 *     "aaaa bb"         "aaaa bb"
 *            ^현재위치         ^현재위치-1
 *     "aaaa bbb"        "aaaa bbb"
 *            ^현재위치         ^현재위치-1
 *     "aaaa bbbb"       "aaaa bbbb"
 *            ^현재위치         ^현재위치-1
 *     즉, 자신이 복사가 되면서 커지게 되고 이것은 바로 source도 동시게 커지게 되는것이다.
 *     이것은 순차적으로 복사가 수행되기 때문에 가능한 것이다.
 **********************************************************************/

/***********************************************************************
 * 헤더의 모양 (첫 1byte를 bit 정보로 나타낸 모습)
 * literal => 0000xxxx (0은 1bit에 0이 고정되어 있음을,
 *                      x는 1bit에 0또는 1, 어떠한 값도 상관없다는 don't care를 나타낸다.
 * match   => xxxxxxxx (이때 상위 4bit이 모두 0인 경우는 literal 이기 때문에 match헤더
 *                      에서는 제외된다.)
 *
 * 순결 헤더 => xxxxxxxx 순결헤더가 나오면 그 이하는 모두 literal 이다. 즉 순결헤더도 literal인데
 *            그 모양은 match헤더와 같다.  그렇다면 이것을 순결헤더로 판단할 수 있는가?
 *            순결헤더는 압축결과의 가장 첫 byte에 나오게 된다.  이것은 압축 파일의 형식상 literal
 *            헤더가 나와야 하는 자리이다.  이곳에 만약 match헤더의 모양을 가진 헤더가 나올 경우
 *            이것을 순결헤더로 판단 할 수 있는 것이다. literal을 실제 match헤더처럼 보이게 만들기
 *            위해서 실제 literal길이에 17을 더한 값을 순결헤더에 적게된다.
 *            만약 압축을 헤제할 때 순결
 * 종료 헤더 => 00010001 00000000 00000000, 3byte를 차지한다.  이때 첫 byte가 match헤더의
 *            모양을 하고 있다.  좀더 자세히 구분 하면 m4헤더의 모양을 하고 있다.  그렇기 때문에
 *            압축헤제시 종료를 판별하는 문장은 m4헤더를 처리하는 부분에서 하게된다. m4 헤더는
 *            위와같은 형태가 나올 수 없기 때문에 이것을 종료헤더로 판별할 수 있다.
 *
 *
 * literal 헤더의 자세한 모양 (여기서 t는 앞으로 써야할 literal의 개수를 뜻한다.).
 * literal 헤더는 t의 크기에 따라 헤더의 모양과 크기가 변한다.
 * t<=3     이경우에는 특별히 헤더가 존재하지 않는다. literal헤더의 앞에는 반드시 match헤더가
 *          나오게 된다. 매치헤더의 특정한 부분에 이 정보를 같이 삽입하게된다. 때문에 literal이
 *          3보다 작은 경우에는 헤더에 대한 오버헤드가 존재하지 않는다.
 * t<=18    0000xxxx 이때 하위4bit에 t-3의 정보를 삽입한다. 이 헤더가 나올 경우 '헤더 + 3'
 *          을 통해 실제의 literal의 길이를 구할 수 있다.
 * 18 < t   00000000 .... xxxxxxxx 18보다 큰 경우에는 첫 헤더는 모두 0으로 채우고 이 다음
 *          헤더부분에 t-18의 값을 적게 된다.  이때 만약 t-18> 255일 경우에는 그 길이 정보를
 *          1byte에 적을 수 없다. 때문에 00000000을 추가하고 t에는 추가로 255를 더 뺀다.
 *          이러한 방법은 압축 알고리즘 전반에 쓰인다.
 *
 *
 * match 헤더의 자세한 모양
 *
 * m2 => offset이 M2_MAX_OFFSET 이하, length가 M2_MAX_LEN 이하일 경우 쓴다.
 *       xxxxxx00 xxxxxxxx => 첫 바이트의 상위 3bit은 len을 표시하는데 쓰인다. 이때
 *       len-1한 값을 적는다. 왜냐하면, m2헤더의 경우에는 그 첫 byte가 항상 64이상이면서
 *       3bit에 8을 표현할 수 없기 때문이다. match의 length는 무조건 3이상이기 때문에 m2헤더는
 *       항상 64이상이 된다.
 *       첫 바이트의 그다음 3bit은 offset의 하위 3비트를 표시하고, 그다음 바이트는 모두
 *       offset의 상위 8bit을 표시하는데 쓰인다.
 *
 *       [x]  [x]  [x]  [x]  [x]  [x]  [0]  [0]
 *       |           |  |           |
 *       +-- len-1 --+  +-offset하위-+
 *
 *       [x]  [x]  [x]  [x]  [x]  [x]  [x]  [x]
 *       |                                    |
 *       +---       offset 상위         -------+
 *
 *       첫 바이트의 하위 2bit은 '3이하인 다음에 나오게 되는 literal'을 표시하는데 쓰인다.
 *       앞으로 나오는 매치헤더의 하위 2bit은 모두 위와 같이 비워져 있고, 이것은 모두 같은 용도로 쓰인다.
 *
 *
 * m3 => offset이 M2_MAX_OFFSET이하인 경우 또는,
 *       offset이 M3_MAX_OFFSET이하인 경우에 이 헤더를 쓴다.
 *       이 헤더는 001로 시작하기 때문에 첫 헤더가 32에서 64이하의 값을 가지게 된다.
 *
 *       [0]  [0]  [1]  [0]  [0]  [x]  [x]  [x]
 *                  |   |                    |
 *                  |   +---    len -2  -----+
 *                M3_MARKER
 *
 *       [x]  [x]  [x]  [x]  [x]  [x]  [0]  [0]
 *       |                          |
 *       +--- offset 하위 6bit ------+
 *
 *
 *       [x]  [x]  [x]  [x]  [x]  [x]  [0]  [0]
 *       |                                   |
 *       +--- offset 상위 8bit ---------------+
 *
 *       만약 len이 위의 헤더로 표현 할 수 없을 만큼 길어진다면 5bit을 모두 0으로 채우고
 *       1byte의 헤더를 더 추가한다.  또한 이것역시도 표현 할 수 없다면 모두 0을 채우고
 *       길이에서 255를 제거한 다음 다시 하나의 헤더를 추가 한다.  이와 같은 작업을 표현
 *       할 수 있을 때까지 계속한다.
 *
 * m4 => offset이 M3_MAX_OFFSET 이상이면서 M4_MAX_OFFSET 이하인 경우 사용
 *
 *      [0]  [0]  [0]  [1]  [x]  [x]  [x]  [x]
 *                      |        |           |
 *                      |        +- len -2  -+
 *                  M4_MARKER
 *
 *       [x]  [x]  [x]  [x]  [x]  [x]  [0]  [0]
 *       |                          |
 *       +--- offset 하위 6bit ------+
 *
 *
 *       [x]  [x]  [x]  [x]  [x]  [x]  [0]  [0]
 *       |                                   |
 *       +--- offset 상위 8bit ---------------+
 *
 *
 *      M4_MAX_OFFSET =>  1011 1111 1111 1111
 *                      - 0100 0000 0000 0000 ( M3_MAX_OFFSET )
 *                      = 0111 1111 1111 1111
 *      m4 offset은 위와 같은 방법으로 구한다.  이때 하위 14 bit는 m3와 같은 방법으로 표현
 *      하고, 최상위 1bit은 첫번째 바이트의 M4_MARKER의 바로 다음 비트에 입력하게 된다.
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <iduCompression.h>

#if defined(WRS_VXWORKS)
#undef m_len
#endif

#define COMPRESSION_BYTE(x)       ((UChar) ((x) & 0xff))

#define PTR(a)              ((vULong) (a))
#define PTR_LT(a,b)         (PTR(a) < PTR(b))
#define PTR_DIFF(a,b)       ((vULong) (PTR(a) - PTR(b)))
//pd: 두 주소간의 차이를 구하는 매크로 함수
#define pd(a,b)             PTR_DIFF(a,b)//((UInt) ((a)-(b)))

//m1은 쓰이지 않는다.
#define M1_MAX_OFFSET   0x0400

/***********************************************************************
 * 각 offset별로 나누어서 헤더를 따로 만든다.  이때 m2 는 2byte를 차지하고
 * m3, m4는 헤더가 3byte 이상이다.
 * m2는 offset이 0x0800이하, 길이 8이하인 매치이고, 이것은 정확히 2byte정보만을
 * 차지한다. 때문에 m2에서 최대한 얻을 수 있는 압축률은 1:4이다.
 * m3, m4는 offset에 따라 나뉘고, 이들의 매치되는 길이는 정해져 있지 않다. 또한
 * 매치되는 길이가 길어질 수록, 즉 255마다 1byte의 헤더가 더 붙게 된다.
 **********************************************************************/
#define M2_MAX_OFFSET   0x0800
#define M3_MAX_OFFSET   0x4000
#define M4_MAX_OFFSET   0xbfff

#define M2_MIN_LEN      3
#define M2_MAX_LEN      8
#define M4_MIN_LEN      3
#define M4_MAX_LEN      9

//헤더에 m3와 m4를 구별하기 위한 비트
#define M3_MARKER       32
#define M4_MARKER       16

/***********************************************************************
 * 아래의 6개 매크로 함수는 모두 해시 키를 생성하기 위한 것들이다.
 * DX2, DX3는 각자 2byte, 3byte의 연속된 byte를 xor로 섞기 위한 함수이다.
 * DMS, DM은 특정한 비트만을 남기기 위한 함수이다.
 * D_INDEX1 은 1차 해싱을 위한 함수, D_INDEX2는 2차 해싱을 위한 함수이다.
 * 여기서 D_INDEX1을 보면 0,1,2번째 바이트는 해시키의 형성에 많은 영향을 미치지만
 * 3번째 byte는 그리 큰 영향을 미치지 않는다.
 * 이것을 통해 해시키를 생성함에 있어 3개가 매치됨을 주로 하고, 마지막 byte는
 * 영향을 미치는 것도 아니고 영향을 안미치는것도 아니다.
 *
 **********************************************************************/
#define DX2(p,s1,s2)                                            \
 	(((((UInt)((p)[2]) << (s2)) ^ (p)[1]) << (s1)) ^ (p)[0])
#define DX3(p,s1,s2,s3) ((DX2((p)+1,s2,s3) << (s1)) ^ (p)[0])
#define DMS(v,s)        ((UInt) (((v) & (IDU_COMPRESSION_D_MASK >> (s))) << (s)))
#define DM(v)           DMS(v,0)

#define D_INDEX1(d,p)       d = DM((0x21*DX3(p,5,5,6)) >> 5)
#define D_INDEX2(d,p)       d = (d & (IDU_COMPRESSION_D_MASK & 0x7ff)) ^ (IDU_COMPRESSION_D_HIGH | 0x1f)


/***********************************************************************
 * 이 문장으로 해시 키의 정당성을 검사한다.
 * 이부분은 ../include/iduCompression.h의  'workMem 초기화가 필요한가?'에 자세히 설명되어 있다.
 * 이 부분 덕분에 workMem의 초기화가 필요 없다.
 **********************************************************************/
#define COMPRESSION_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,max_offset)        \
    (m_pos = ip - (UInt) PTR_DIFF(ip,m_pos), PTR_LT(m_pos,in) ||    \
	 (m_off = (UInt) PTR_DIFF(ip,m_pos)) <= 0 ||                    \
	 m_off > max_offset)

/***********************************************************************
 * 만약 두 데이터의 주소가 PTR_ALIGNED2_4를 만족한다면 1byte당 복사를 하지 않고,
 * COPY4를 이용하여 4byte씩 한꺼번에 복사를 수행 할 수 있다.
 **********************************************************************/
#define COPY4(dst,src)    * (UInt *)(dst) = * ( UInt *)(src)

#define PTR_ALIGNED2_4(a,b) (((PTR(a) | PTR(b)) & 3) == 0)

#define COMPRESSION_E_INPUT_OVERRUN         (-4)
#define COMPRESSION_E_OUTPUT_OVERRUN        (-5)
#define COMPRESSION_E_INPUT_NOT_CONSUMED    (-8)

/***********************************************************************
 * 압축 결과 크기 예측 가능
 *
 * 1. match는 결코 크기가 증가하지 않는다. (최악의 경우라도 1byte 절약), literal은
 * 연달아 나오지 않는다.  즉 최악의 후보에는 반드시 literal이 들어가야 하고, literal은
 * 길어질수록 이득이 되므로 어느 선에서는 match가 나와서literal을 끊어 주어야 한다.
 * 그리고 이러한 나쁜 상황이 계속해서 일어나야 한다.
 *
 * 2. 최악의 시나리오 후보 1.
 *    literal 4개 이상인 경우 별도로 1개의 헤더 필요, 이후 match3개, 이러한 패턴이
 *    계속나오는 상황
 *    => 결국 원본 size당 증가하는 크기는 없음
 *
 * 3. 최악의 시나리오 후보 2.
 *    literal 19개 이상인 경우 별도로 2개의 헤더가 필요, 이후 match 3개, 이러한 패턴
 *    이 계속나오는 상황
 *    => 각 22byte당 1개의 byte증가
 *
 * 4. 최악의 시나리오 후보 3.
 *    literal 274개 이상인 경우 별도로 3개의 헤더가 필요, 이후 match 3개인 상황,
 *    이러한 패턴이 계속 나오는 상황
 *    => 각 277byte당 2개의 byte증가.. 즉 129byte당 1개의byte증가
 *
 * 5. 즉, 최악의 시나리오 2에 따라, 22byte당 1개의 byte증가. 마지막 3bit필요
 *
 * 때문에 max_size(s) = s + ((s/22)+1) + 3이 된다.
 *
 * PROJ-2010
 * 공식의 오류가 발견되어, 더 이상 위의 공식을 사용하지 않으나
 * 알고리즘 이해에 도움이 되는 주석으로 생각되어 남겨둔다.
 * 추가 설명으로 length 255당 또는 offset 255당 헤더의 길이는 1바이트 씩
 * 계속 증가한다.
 **********************************************************************/

/***********************************************************************
 * Description : 실제 압축을 수행하는 부분
 *
 * 파라미터의 의미는 compress와 완전 동일하다.
 *
 * Description : 압축을 수행 할 때 호출하는 함수
 *
 * aSrc         - [IN] : 실제 압축을 수행하고자 하는 소스가 들어 있는 버퍼, 이때 임의의 크기
 *			 를 적용 할 수 있다.
 *
 * aSrcLen      - [IN] : aSrc버퍼의 길이
 *
 * aDest        - [IN] : 압축이 수행된 결과가 입력되는 버퍼, 이 버퍼의 크기는 반드시
 *                       IDU_COMPRESSION_MAX_OUTSIZE(소스버퍼의크기) 를 적용하여 생성한다.
 *
 * aDestLen     - [IN] : aDest버퍼의 길이
 *
 * aResultLen   - [OUT]: 압축된 내용의 길이
 * aWorkMem     - [IN] : 압축시 해시테이블로 사용할 메모리를 같이 주어서 실행 시킨다.
 *                       이때 위의 IDU_COMPRESSION_WORK_SIZE크기로 메모리를 생성한다.
 *
 **********************************************************************/
UInt iduCompression::compressInternal(  UChar *aSrcBuf ,
                                        UInt   aSrcLen,
                                        UChar *aDestBuf,
                                        UInt   /*aDestLen*/,
                                        UInt*  aResultLen,
                                        void*  aWorkMem )
{
    UChar  *sSrcPtr      = aSrcBuf;
    UChar  *sSrcRealEnd  = aSrcBuf + aSrcLen;

    /***********************************************************************
     * 여기서 M2_MAX_LEN을 제거하는 이유는 아래에 설명되어 있다.
     **********************************************************************/
    UChar  *sSrcShortEnd = aSrcBuf + aSrcLen - M2_MAX_LEN - 5;
    UChar  *sSrcCalcPtr  = aSrcBuf;

    UChar  *sDestPtr     = aDestBuf;

    //sDict는 aWorkMem을 포인터를 저장하는 해시 테이블로 쓴다.
    UChar **sDict        = (UChar **) aWorkMem;

    sSrcPtr += 4;

    for (;;)
    {
        register  UChar *m_pos;
        UInt m_off;
        UInt m_len;
        UInt dindex;

        // sSrcPtr[0~3]으로 해시키를 생성한 후 이것을 dindex에 삽입
        D_INDEX1(dindex, sSrcPtr);

        m_pos = sDict[dindex];

        //현재 나온 dindex의 정당성을 검사하는 부분, 자세한 것은 ../include/iduCompression.h의
        //'aWorkMem의 초기화가 필요 없다' 부분을 참조
        if (COMPRESSION_CHECK_MPOS_NON_DET(m_pos,m_off,aSrcBuf,sSrcPtr,M4_MAX_OFFSET))
            goto literal;

        /***********************************************************************
         * 아래의 문장이 의도는 match는 무조건 최소한 1byte라도 이익을 보자는 것이다.
         *  (가정)일단 기본적으로 match는 3byte가 맞아야 이루어 진다.
         * 아래의 조건을 만족하기 위해선 즉, 통과하기 위해선
         * 1. M2 offset이다. (이경우에는 헤더크기가 2byte이고, 최소의 match가 3byte이므로
         *    최소한 1byte 이익이다.)
         * 2. M3 또는 M4 offset이다. (이경우에는 헤더크기가 3byte이고, 최소의 match가
         *    4byte일때 아래의 조건을 만족하게 된다.
         *
         * 즉, M2_MAX_OFFSET 이거나 4개가 매치 할 경우 이 두경우 중 하나에 속하면 이 두경우
         *   모두에 속하지 않는것보다 최소한 1byte를 더 절약 할 수 있다.
         *   이 문장을 없앨경우 속도를 눈꼽 만큼 증가할 수 있겠지만, 생성가능한 destBuf의
         *   최대 크기를 이것에 기초해서 계산하므로 이문장을 없애지 않는 것이 좋다.
         **********************************************************************/
        if ( (m_off <= M2_MAX_OFFSET) || (m_pos[3] == sSrcPtr[3]))
            goto try_match;

        /* 2차 해시 수행 */
        D_INDEX2(dindex,sSrcPtr);

        m_pos = sDict[dindex];

        if (COMPRESSION_CHECK_MPOS_NON_DET(m_pos,m_off,aSrcBuf,sSrcPtr,M4_MAX_OFFSET))
            goto literal;

        if ((m_off <= M2_MAX_OFFSET) || (m_pos[3] == sSrcPtr[3]))
            goto try_match;

        goto literal;

      try_match:
        //  이곳에서 실제로 3byte가 매치되는지 살펴본다. 그렇지 않을 경우 literal취급을 한다.
        if ((m_pos[0] == sSrcPtr[0]) && (m_pos[1] == sSrcPtr[1]) && (m_pos[2] == sSrcPtr[2]))
        {
            goto match;
        }

        //  literal인 경우엔 실제로 쓰지 않고 현재의 포인터만 이동한다.
      literal:
        // 해시테이블의 정보를 현재의 값으로 바꾼다.
        sDict[dindex] = sSrcPtr;

        ++sSrcPtr;
        if (sSrcPtr >= sSrcShortEnd)
            break;
        continue;


        /***********************************************************************
         *		LITERAL COMPRESS
         * 위에서는 직접 destBuf에 쓰지않고, 단지 현재 포인터만 증가 하고, 이부분에서 실제로
         * destBuf에 쓰게 된다.  이때 가장 앞에 헤더를 달고, 실제 헤더에 명시한 길이만큼
         * literal을 srcBuf으로 부터 복사한다.
         *
         *
         * literal 헤더에 따른 실제 길이의 예
         * 0x08		=> 8+3		=> 11
         * 0x00 0x08	=> 18+8+3	=> 26
         * 0x00 0x00 0x01 => 18+255+1	=> ...
         *
         **********************************************************************/
      match:
        sDict[dindex] = sSrcPtr;
        if (pd(sSrcPtr,sSrcCalcPtr) > 0)
        {
            register UInt t = pd(sSrcPtr,sSrcCalcPtr);

            /***********************************************************************
             * t<=3 일 경우엔 실제로 따로 literal 헤더를 쓰지 않는다.
             * 그렇기 때문에 3의 정보를 표현하기 위에 match헤더 끝의 2번째 byte의 마지막에 그
             * 정보를 기록한다. match헤더의 끝의 2번째 byte의 마지막 2bit은 이 literal 길이를
             * 표현할 수 있도록 항상 비워져 있다.
             **********************************************************************/
            if (t <= 3)
            {
                // 앞의 2byte는 반드시 유효한 정보여야 한다.
                IDE_ASSERT(sDestPtr - 2 > aDestBuf);
                sDestPtr[-2] |= COMPRESSION_BYTE(t);
            }
            else
            {
                /* literal 헤더의 경우엔 앞의 4bit은 0으로 해야 하기 때문에 18이하인 경우 3을 제한다.*/
                if (t <= 18)
                {
                    *sDestPtr++ = COMPRESSION_BYTE(t - 3);
                }
                else
                {
                    /***********************************************************************
                     * literal의 길이가 18을 넘어가는 경우는 길이 정보를 표현하기 위해 추가로 하나의
                     * byte를 더 필요로 한다.
                     * 계속해서 255 길이를 표현하기 위해서 0으로 된 헤더가 하나씩 추가 된다.
                     **********************************************************************/
                    register UInt tt = t - 18;

                    *sDestPtr++ = 0;
                    while (tt > 255)
                    {
                        tt -= 255;
                        *sDestPtr++ = 0;
                    }
                    // tt는 1이상이다.왜냐하면, t - 18을 하는데, t는 19이상이기 때문이다.
                    // 위의 while을 나가는 기준이 되는 것은 255이상일때고, while내에서는
                    // 255를 제거하기 때문에 0이 나올 수 없다.
                    IDE_ASSERT(tt > 0);
                    *sDestPtr++ = COMPRESSION_BYTE(tt);
                }
            }
            //여기까지는 헤더를 만드는 부분 이었고, 아래의 루프에서 실제 literal 복사를 한다.
            do
            {
                *sDestPtr++ = *sSrcCalcPtr++;
            } while (--t > 0);
        }

        // 여기에 왔을때에는 리터럴은 모두 실제 destBuf에 쓰여 져야 한다.
        IDE_ASSERT(sSrcCalcPtr == sSrcPtr);

        /***********************************************************************
         *		MATCH COMPRESS
         * offset에 따라 각 헤더의 종류를 나누고, 그에 맞추어서 작성을 하고, length를 헤더에
         * 적는다.
         ***********************************************************************/
        //최소한 3개 이상이 매치가 됨을 위에서 확인 하였으므로
        sSrcPtr += 3;

        /***********************************************************************
         *  8개의 match를 확인 한다. 이때 마지막을 check하는 것 없이 한번에 이 모든것을
         *  check하기 때문에, 오버플로가 발생할 가능성이 있다. 그렇기 때문에 위에서
         *  M2_MAX_LEN을 제거 한다.
         ***********************************************************************/
        if (m_pos[3] != *sSrcPtr++ ||
            m_pos[4] != *sSrcPtr++ ||
            m_pos[5] != *sSrcPtr++ ||
            m_pos[6] != *sSrcPtr++ ||
            m_pos[7] != *sSrcPtr++ ||
            m_pos[8] != *sSrcPtr++
            )
        {
            --sSrcPtr;
            m_len = sSrcPtr - sSrcCalcPtr;

            //위에서 최소한 3개 이상이 매치됨을 확인 하였다.
            IDE_ASSERT(m_len >= 3);
            //만약 M2_MAX_LEN 이상이었다면 이곳으로 분기하지 않았다.
            IDE_ASSERT(m_len <= M2_MAX_LEN);

            //m2 match 헤더를 작성하는 부분
            if (m_off <= M2_MAX_OFFSET)
            {
                m_off -= 1;
                //첫 1byte 헤더 설정, 길이와 offset정보가 들어간다.
                *sDestPtr++ = COMPRESSION_BYTE(((m_len - 1) << 5) | ((m_off & 7) << 2));
                //다음 byte 헤더 설정,
                *sDestPtr++ = COMPRESSION_BYTE(m_off >> 3);
            }
            // m3 match 헤더를 작성하는 부분
            else if (m_off <= M3_MAX_OFFSET)
            {
                m_off -= 1;
                //여기서는 길이정보가 모두 첫 byte에 들어간다.
                *sDestPtr++ = COMPRESSION_BYTE(M3_MARKER | (m_len - 2));
                goto m3_m4_offset;
            }
            // m4 match 헤더를 작성하는 부분
            else
            {
                //m4 offset에서 M3_MAX_OFFSET을 뺀다.
                m_off -= 0x4000;
                IDE_ASSERT(m_off > 0); IDE_ASSERT(m_off <= 0x7fff);

                //m4 marker를 적고, offset 정보를 입력, 이때 offset의 최상위 bit을
                //첫 헤더의 특정한 공간에 삽입한다. 위 m4 헤더정보 참조
                *sDestPtr++ = COMPRESSION_BYTE(M4_MARKER |
                                               ((m_off & 0x4000) >> 11) | (m_len - 2));
                goto m3_m4_offset;
            }
        }

        /***********************************************************************
         *  9개 이상의 매치를 수행 할 때는 항상 끝을 넘어가는지 체크를 하면서 진행한다.
         *  이때는 실제 끝과 비교한다.
         ***********************************************************************/
        else
        {
            // 실제 끝을 넘어가는지 확인을 하면서 몇개가 매치되는지 확인한다.
            {
                UChar *end = sSrcRealEnd;
                UChar *m = m_pos + M2_MAX_LEN + 1;
                while (sSrcPtr < end && *m == *sSrcPtr)
                    m++, sSrcPtr++;
                m_len = pd(sSrcPtr, sSrcCalcPtr);
            }

            // m_len이 M2_MAX_LEN이하 였다면 이곳으로 분기하지 않았을 것이다.
            IDE_ASSERT(m_len > M2_MAX_LEN);

            // m3 match헤더 작성
            if (m_off <= M3_MAX_OFFSET)
            {
                m_off -= 1;
                if (m_len <= 33)
                {
                    *sDestPtr++ = COMPRESSION_BYTE(M3_MARKER | (m_len - 2));
                }
                else
                {
                    m_len -= 33;
                    *sDestPtr++ = M3_MARKER | 0;
                    //길이 정보를 첫 바이트로 다 표현 할 수 없는 경우 모두 0으로 set한 후
                    //다음 바이트로 그 정보를 표현한다.
                    goto m3_m4_len;
                }
            }
            else
            {
                // m4 match 헤더 작성,
                m_off -= 0x4000;
                IDE_ASSERT(m_off > 0); IDE_ASSERT(m_off <= 0x7fff);

                // m4 max len이란 9를 뜻한다. 즉 9까지는 첫 바이트에 그 길이 정보를 표시 할 수있으나
                // 그이상인 경우에는 헤더를 늘려서 다음 바이트에 그 정보를 표현한다.
                if (m_len <= M4_MAX_LEN)
                {
                    IDE_ASSERT(m_len == M4_MAX_LEN);
                    *sDestPtr++ = COMPRESSION_BYTE(M4_MARKER |
                                                   ((m_off & 0x4000) >> 11) | (m_len - 2));
                }

                // 헤더를 늘려서 더 많은 길이 정보를 표현
                else
                {
                    m_len -= M4_MAX_LEN;
                    *sDestPtr++ = COMPRESSION_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11));

                    /***********************************************************************
                     *  m3의 경우 길이 33초과, m4의 경우 길이 9초과 일 경우 추가로 하나의 헤더를 생성하여
                     * 그 길이를 표현한다. 이때 literal에서와 마찬가지로 255당 하나의 헤더를 추가하여 길이
                     * 정보를 표현한다.
                     ***********************************************************************/
                  m3_m4_len:
                    while (m_len > 255)
                    {
                        m_len -= 255;
                        *sDestPtr++ = 0;
                    }
                    IDE_ASSERT(m_len > 0);
                    *sDestPtr++ = COMPRESSION_BYTE(m_len);
                }
            }


            /***********************************************************************
             *  m3와 m4의 offset은 동일한 모양을 가지고 있다.
             *  이때 m4의 offset은 1bit를 앞부분에 할당 하고, 현재 offset에서 M3_MAX_OFFSET을
             * 제거 한 값을 표현 하므로, m3보다 크고 m3가 표현 할 수 있는 부분보다 2배 넓은 범위의
             *  offset을 표현 할 수 있다.
             ***********************************************************************/
          m3_m4_offset:
            *sDestPtr++ = COMPRESSION_BYTE((m_off & 63) << 2);
            *sDestPtr++ = COMPRESSION_BYTE(m_off >> 6);
        }

        sSrcCalcPtr = sSrcPtr;
        if (sSrcPtr >= sSrcShortEnd)
            break;
    }

    *aResultLen = pd(sDestPtr, aDestBuf);
    return pd(sSrcRealEnd, sSrcCalcPtr);
}

/***********************************************************************
 * Description : 압축을 수행 할 때 호출하는 함수
 *
 * aSrc         - [IN] : 실제 압축을 수행하고자 하는 소스가 들어 있는 버퍼, 이때 임의의 크기
 *			 를 적용 할 수 있다.
 *
 * aSrcLen      - [IN] : aSrc버퍼의 길이
 *
 * aDest        - [IN] : 압축이 수행된 결과가 입력되는 버퍼, 이 버퍼의 크기는 반드시
 *                       IDU_COMPRESSION_MAX_OUTSIZE(소스버퍼의크기) 를 적용하여 생성한다.
 *
 * aDestLen     - [IN] : aDest버퍼의 길이
 *
 * aResultLen   - [OUT]: 압축된 내용의 길이
 * aWorkMem     - [IN] : 압축시 해시테이블로 사용할 메모리를 같이 주어서 실행 시킨다.
 *                       이때 위의 IDU_COMPRESSION_WORK_SIZE크기로 메모리를 생성한다.
 *
 **********************************************************************/
IDE_RC iduCompression::compress   (  UChar   *aSrcBuf ,
                                     UInt     aSrcLen,
                                     UChar   *aDestBuf,
                                     UInt     aDestLen,
                                     UInt    *aResultLen,
                                     void    *aWorkMem )
{
    UChar  *sDestPtr     = aDestBuf;
    UInt sRemains;

    //길이가 특정 길이 이하 너무 작은 경우에는 압축을 하지 않는다.
    if (aSrcLen <= M2_MAX_LEN + 5) /* <= 8 + 5 */
    {
        sRemains = aSrcLen;
    }

    else
    {
        /* over 13, so from sizeof 14 byte... */
        sRemains = compressInternal(aSrcBuf,
                                    aSrcLen,
                                    sDestPtr,
                                    aDestLen,
                                    aResultLen,
                                    aWorkMem);

        IDE_TEST_RAISE(aDestLen < *aResultLen, overflow_occurred);

        sDestPtr += *aResultLen;
    }

    /*sRemains가 0이라면 compressInternal의 마지막은 match로 끝나는 것이다. */
    if (sRemains > 0)
    {
        UChar *sSrcCalcPtr = aSrcBuf + aSrcLen - sRemains;

        /***********************************************************************
         * 순결헤더의 조건은 238보다 작아야 하고, match가 일어나지 않았어야 한다.
         ***********************************************************************/
        if ((sDestPtr == aDestBuf) && (sRemains <= 238))
        {
            //순결헤더: 이부분은 항상 압축결과의 가장 처음에 오게 되어 있다.
            *sDestPtr++ = COMPRESSION_BYTE(17 + sRemains);
        }
        else
        {
            /* 아래부분은 compressInternal에서 literal을 처리하는 부분과 완전히 동일하다. */
            if (sRemains <= 3)
            {
                //compressInternal 처음에 srcPtr을 4칸 미루어 놓고 시작하기 때문에 이부분이 가장 처음에 올 수는 없다.
                sDestPtr[-2] |= COMPRESSION_BYTE(sRemains);
            }
            else
            {
                //이부분은 압축결과 처음에도 나올 수 있고, 마지막 literal이 될 수도 있다.
                if (sRemains <= 18)
                {
                    *sDestPtr++ = COMPRESSION_BYTE(sRemains - 3);
                }
                else
                {
                    UInt tt = sRemains - 18;

                    *sDestPtr++ = 0;
                    while (tt > 255)
                    {
                        tt -= 255;
                        *sDestPtr++ = 0;
                    }

                    // tt는 1이상이다.왜냐하면, t - 18을 하는데, t는 19이상이기 때문이다.
                    // 위의 while을 나가는 기준이 되는 것은 255이상일때고, while내에서는
                    // 255를 제거하기 때문에 0이 나올 수 없다.
                    IDE_ASSERT(tt > 0);
                    *sDestPtr++ = COMPRESSION_BYTE(tt);
                }
            }
        }

        do
        {
            *sDestPtr++ = *sSrcCalcPtr++;
        } while (--sRemains > 0);
    }




    //종료헤더 작성, 마지막을 M4_MARKER로 표시
    *sDestPtr++ = M4_MARKER | 1;
    *sDestPtr++ = 0;
    *sDestPtr++ = 0;

    *aResultLen = pd(sDestPtr, aDestBuf);

    return IDE_SUCCESS;

    IDE_EXCEPTION(overflow_occurred);
    {
        //return COMPRESSION_E_OUTPUT_OVERRUN;
        //IDE_SET(ideSetErrorCode(COMPRESSION_E_OUTPUT_OVERRUN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 압축된 내용을 원본 소스로 풀때 사용하는 함수
 * aSrc         - [IN] : 압축 내용이 있는 버퍼
 * aSrcLen      - [IN] : aSrc의 크기
 * aDest        - [IN] : 압축 해제 내용을 넣을 버퍼, 이것의 크기는 compress에서의
 *			 원본의 크기와 동일하다. 압축은 전과 후가 크기가 같음을 상기
 * aDestLen     - [IN] : aDest의 크기
 * aResultLen   - [OUT]: 압축 해제 내용의 크기
 **********************************************************************/
IDE_RC iduCompression::decompress  (  UChar       *aSrcBuf,
                                      UInt         aSrcLen,
                                      UChar       *aDestBuf,
                                      UInt         aDestLen,
                                      UInt        *aResultLen )
{
    register UChar *sDestPtr;
    register UChar *sSrcPtr;
    register UInt   t;
    register UChar *m_pos;

    UChar *  sSrcRealEnd  = aSrcBuf  + aSrcLen;
    UInt     sInvalidSrcLen;
    UInt     sInvalidDestLen;

    *aResultLen = 0;

    sDestPtr = aDestBuf;
    sSrcPtr = aSrcBuf;

    /***********************************************************************
     * 압축을 푸는데 있어서 핵심은 첫 1byte를 통해 어떤 헤더 정보를 가지는지 구별이 가능
     * 하다는 것이다.
     * 64이상           => m2
     * 32이상 64미만     => m3
     * 16이상 32미만     => m4
     * 16 미만          => literal헤더
     * 다만 예외적으로 버퍼의 가장 첫 1bit인 경우엔 17이상을 순결헤더라 하여, 이 헤더가
     * 나타나면 단지 하나의 literal 만으로 되어있음을 판별 할 수 있다.
     **********************************************************************/
    /*순결 헤더 처리하는 부분*/
    if (*sSrcPtr > 17)
    {
        //순결헤더는 match헤더의 모양을 하기 위해 17을 더하므로 실제 길이를 알기 위해선
        //빼주어야 한다.
        t = *sSrcPtr++ - 17;
        /**********************************************************************
         * 아래 문장에서 굳이 t가 4보다 작은지 여부를 따질 필요가 없다. 이부분은 어차피 literal
         * 복사를 수행한후 종료될 것이기 때문이다.
         * 그렇기 때문에 이곳에 goto match; 라는 명령문이 나오고 아래 문장은 수행 되지 않아도 상관없다.
         * 그럼에도 불구하고 수정하지 않는 이유는 현재 잘 돌아가는 프로그램이고, 동작에는 영향을
         * 끼치지 않기 때문이다. decompress에서는 이러한 부분이 몇개 더 있다.
         **********************************************************************/
        if (t < 4)
            goto match_next;

        do
        {
            *sDestPtr++ = *sSrcPtr++;
        } while (--t > 0);

        goto first_literal_run;

    }

    while (1)
    {
        t = *sSrcPtr++;
        if (t >= 16)			// 이것은 match임을 뜻한다.
            goto match;


        /***********************************************************************
         *		 LITERAL DECOMPRESSION
         *
         **********************************************************************/
        // literal에서 t,즉 첫 바이트가 0이라는 것은 이것은 길이가 최소한 19이상임을 뜻한다.
        if (t == 0)
        {
            // 만약 모두 0인 byte가 또 나온다면 이것은 255만큼 길이가 더 보상되어져야 함을 뜻한다.
            while (*sSrcPtr == 0)
            {
                t += 255;
                sSrcPtr++;
            }
            t += 15 + *sSrcPtr++;
        }

        //여기서 t는 literal의 길이를 뜻한다. 만약 t가 0보다 작다면 리터럴이 없다는 의미가 된다.
        //리터럴이 없는데도 헤더가 붙을 수는 없다.
        IDE_ASSERT(t > 0);

        // PTR_ALIGNED2_4를 만족하면 4byte씩 복사하고 그렇지 않으면 1byte씩 복사한다.
        if (PTR_ALIGNED2_4(sDestPtr,sSrcPtr))
        {
            COPY4(sDestPtr,sSrcPtr);
            sDestPtr += 4; sSrcPtr += 4;

            // t가 0보다 클동안 수행하고, 그렇지 않으면 그만 한다.
            if (--t > 0)
            {
                // t가 4보다 크다면 COPY4가 가능하고, 그렇지 않다면 불가능
                if (t >= 4)
                {
                    do
                    {
                        COPY4(sDestPtr,sSrcPtr);
                        sDestPtr += 4; sSrcPtr += 4; t -= 4;
                    } while (t >= 4);

                    // 위에서 4개단위로 COPY4를 수행하고, 아래에서 4이하 남은 것들을 바이트 단위로 복사한다.
                    if (t > 0)
                    {
                        do
                        {
                            *sDestPtr++ = *sSrcPtr++;
                        } while (--t > 0);
                    }

                }

                //t가 4보다 작기 때문에 바이트 단위로 복사를 한다.
                else
                {
                    do
                    {
                        *sDestPtr++ = *sSrcPtr++;
                    } while (--t > 0);
                }
            }
        }
        //1byte씩 복사
        else
        {
            //3개씩 한꺼번에 복사하면서 t를 수정하지 않는 이유는 헤더에 값이 쓰여지고
            //읽혀지면서 더해지고 빼진 값에 대한 보상이다.
            *sDestPtr++ = *sSrcPtr++;
            *sDestPtr++ = *sSrcPtr++;
            *sDestPtr++ = *sSrcPtr++;
            do
            {
                *sDestPtr++ = *sSrcPtr++;
            } while (--t > 0);
        }

      first_literal_run:

        t = *sSrcPtr++;
        if (t >= 16)
            goto match;

        /***********************************************************************
         * 순결헤더를 처리하는 부분에서 이곳을 오게 되는것을 제외하면,
         * 아래 10문장은 수행이 되지 않는다고 생각된다.
         * 하지만, 위에서 설명한 것처럼 순결헤더 처리를 변경하면 이곳에는 도달하지 않는다.
         * 왜냐하면, 여기 까지 온 이상 literal은 모두 수행이 되었기 때문이다.
         * 모든 match 헤더는 16이상이기 때문에 여기에는
         * 실행의 흐름상 도달 되지 않는다.
         **********************************************************************/
        m_pos = sDestPtr - (1 + M2_MAX_OFFSET);
        m_pos -= t >> 2;
        m_pos -= *sSrcPtr++ << 2;

        *sDestPtr++ = *m_pos++;
        *sDestPtr++ = *m_pos++;
        *sDestPtr++ = *m_pos;

        goto match_done;

        /***********************************************************************
         *		 MATCH DECOMPRESSION
         **********************************************************************/
        while (1)
        {

          match:
            if (t >= 64)                    // M2 offset
            {
                m_pos = sDestPtr - 1;       // apply low 3 bits
                m_pos -= (t >> 2) & 7;      // apply upper 8 bits
                m_pos -= *sSrcPtr++ << 3;
                t = (t >> 5) - 1;           // delete 1 more from len ( total delete 2 len)
                //t는 매치개수를 의미하는데 0일 수는 없다.
                IDE_ASSERT(t > 0);
                goto copy_match;
            }
            else if (t >= 32)               // M3 offset
            {
                t &= 31;                    // because of it, 't' left only len
                if (t == 0)                 // len > 33
                {
                    while (*sSrcPtr == 0)
                    {
                        t += 255;
                        sSrcPtr++;
                    }
                    t += 31 + *sSrcPtr++;
                }
                m_pos = sDestPtr - 1;
                m_pos -= (sSrcPtr[0] >> 2) + (sSrcPtr[1] << 6);

                sSrcPtr += 2;
            }
            else if (t >= 16)                                   // M4 offset
            {
                m_pos = sDestPtr;
                m_pos -= (t & 8) << 11;                         // offset recovery
                t &= 7;                                         // len recovery
                if (t == 0)
                {
                    while (*sSrcPtr == 0)
                    {
                        t += 255;
                        sSrcPtr++;
                    }
                    t += 7 + *sSrcPtr++;                        // len recovery
                }
                m_pos -= (sSrcPtr[0] >> 2) + (sSrcPtr[1] << 6); //offset recovery
                sSrcPtr += 2;
                if (m_pos == sDestPtr)                          //end detect!!
                    goto eof_found;
                m_pos -= 0x4000;                                // offset recovery
            }
            else
            {
                // 매치헤더의 종류가 더이상 없기 때문에 이부분은 수행될 수 없다.
                m_pos = sDestPtr - 1;
                m_pos -= t >> 2;
                m_pos -= *sSrcPtr++ << 2;
                *sDestPtr++ = *m_pos++;
                *sDestPtr++ = *m_pos;
                goto match_done;
            }

            //t는 매치되는 개수를 뜻한다. 이것은 0이 될 수 없다.
            IDE_ASSERT(t > 0);

            //윗부분에서 t를 setting하고 여기서 진짜 복사 수행
            //COPY4는 최소 2번은 일어나야 하지 않겠는가라고 저자가 말하는 것 같음.
            if (t >= 2 * 4 - (3 - 1) && PTR_ALIGNED2_4(sDestPtr,m_pos))
            {
                COPY4(sDestPtr,m_pos);

                sDestPtr += 4; m_pos += 4; t -= 4 - (3 - 1);
                do
                {
                    COPY4(sDestPtr,m_pos);
                    sDestPtr += 4; m_pos += 4; t -= 4;
                } while (t >= 4);

                if (t > 0)
                {
                    do
                    {
                        *sDestPtr++ = *m_pos++;
                    } while (--t > 0);
                }

            }
            else
            {
                // do not assign recovery, it will only byte recovery
              copy_match:
                //because so far total delete is 2
                *sDestPtr++ = *m_pos++;
                *sDestPtr++ = *m_pos++;
                do
                {
                    *sDestPtr++ = *m_pos++;
                }while (--t > 0);
            }

            /***********************************************************************
             * 가장 처음 부분을 제외하고는 literal헤더의 앞은 반드시 match헤더가 있게 된다.
             * 만약 지금 match헤더 뒤에서 2번째 byte의 마지막 2bit에 어떠한 값이 있다면
             * 이것은 길이가 3보다 작은 literal이 뒤이어 나옴을 뜻한다.
             **********************************************************************/
          match_done:

            t = sSrcPtr[-2] & 3;
            if (t == 0)
                break;

            // which is only for the 't<=3 literal'
          match_next:
            // 아래에서 do while을 수행하기 때문에 반드시 t는 0보다 커야 한다.
            // 또한 여기에는 그러한 값들만 오게 되어있다.
            IDE_ASSERT(t > 0);
            do
            {
                *sDestPtr++ = *sSrcPtr++;
            }while (--t > 0);
            t = *sSrcPtr++;
        }
    }


  eof_found:
    //이값은 종료헤더를 체크하면서 자연스럽게 세팅된다. 즉 이곳에는 종료헤더를 통해서만 올 수 있다.
    IDE_ASSERT(t == 1);
    *aResultLen = sDestPtr - aDestBuf;

    IDE_TEST_RAISE(sSrcPtr != sSrcRealEnd, size_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION( size_error );
    {
        // BUG-26695 log decompress size 불일치로 Recovery 실패합니다.

        sInvalidSrcLen  = sSrcPtr  - aSrcBuf;
        sInvalidDestLen = sDestPtr - aDestBuf;

        ideLog::log( IDE_SM_9, // SM_TRC_LOG_LEVEL_MRECOV
                     "Log decompress size is invalid\n"
                     "    Current log size : Comp Src Size %u,"
                     " Decomp Dest Size %u.\n"
                     "    Expected log size : Comp Src Size %u,"
                     " Decomp Dest Size %u.\n",
                     sInvalidSrcLen,
                     sInvalidDestLen,
                     aSrcLen,
                     aDestLen );
    }

#if defined(IDU_COMPRESSION_CHECK_OVERFLOW)
    IDE_EXCEPTION(overflow_occurred);
    {
        //return COMPRESSION_E_OUTPUT_OVERRUN;
        // IDE_SET(ideSetErrorCode(COMPRESSION_E_OUTPUT_OVERRUN));
    }
#endif /* IDU_COMPRESSION_CHECK_OVERFLOW */
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
