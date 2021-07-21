/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduCompression.h 47966 2011-07-04 07:36:01Z cgkim $
 **********************************************************************/

#ifndef _O_IDU_COMPRESSION_H_
#define _O_IDU_COMPRESSION_H_ 1

#include <idl.h>

/***********************************************************************
 * 여기나온 매크로 함수는 모두 해시테이블에 관련된 연산을 하는데 쓰인다.
 *
 * IDU_COMPRESSION_D_BITS : 해시테이블의 키의 비트수를 나타낸다. 즉 몇비트의 키를 생성할 
 *                          것인지를 나타냄. 이 비트를 조정함으로 해서 해시 테이블의 
 *                          크기도 변경 할 수 있다. 즉, 메모리사용을 줄일 수도, 늘릴
 *                          수도 있다. compress함수의 aWorkMem의 크기에 영향을 준다.
 *
 * IDU_COMPRESSION_D_SIZE : 해시테이블 키의 최대 크기를 나타냄. 
 *
 * IDU_COMPRESSION_D_MASK : IDU_COMPRESSION_D_BITS 개수 만큼의 하위 비트가 모두 1이다. 
 *                          이것과 and연산을 하면 해시키의 유효 범위안에 들어 올 수 있다.
 *
 * IDU_COMPRESSION_D_HIGH : IDU_COMPRESSION_D_MASK의 최 상위인 한 bit만 set된 것
 *
 **********************************************************************/
#define IDU_COMPRESSION_D_BITS          (12)    // 유효범위 : (D_BITS >= 8) && (D_BITS <= 18)
#define IDU_COMPRESSION_D_SIZE          ((UInt)1UL << (IDU_COMPRESSION_D_BITS))
#define IDU_COMPRESSION_D_MASK          (IDU_COMPRESSION_D_SIZE - 1)
#define IDU_COMPRESSION_D_HIGH          ((IDU_COMPRESSION_D_MASK >> 1) + 1)


/***********************************************************************
 * IDU_COMPRESSION_WORK_SIZE :	compess함수의 인자로 들어가는 aWorkMem의 크기는 반드시 
 *                              이 크기로 만들어야 한다. 이것의 크기는 IDU_COMPRESSION_D_BITS에
 *                              영향을 받는다.  왜냐하면 해시 키의 범위에 따라 해시 테이블의
 *                              크기가 달라지기 때문이고, aWorkMem은 바로 해시테이블이기
 *                              때문이다. 즉 압축하는데 있어서 참조하는 해시 테이블을 압축할때
 *                              같이 보내 주어야 한다. 
 *
 * IDU_COMPRESSION_MAX_OUTSIZE : compress함수의 aSrcBuf는 임의의 크기로 할 수 있으나,
 *                               aDestBuf의 크기는 반드시 aSrcBuf size를 인자로 주어 이
 *                               매크로 함수를 적용하여 크기를 생성해야 한다.
 *                               이 크기에 대한 설명은 iduCompression.cpp의 '압축결과 크기 예측가능'
 *                               부분을 참고 하기 바란다.
 *
 **********************************************************************/
#define IDU_COMPRESSION_WORK_SIZE               ((UInt) (IDU_COMPRESSION_D_SIZE * sizeof(UChar *)))
#define IDU_COMPRESSION_MAX_OUTSIZE(size)       (size + (size / 16) + 64 + 3)

class iduCompression
{
/***********************************************************************
 * Description : 실제 압축을 수행하는 부분
 * 파라미터는 compress와 같다.
 *
 **********************************************************************/
    static UInt   compressInternal(UChar *aSrcBuf ,
                                   UInt   aSrcLen,
                                   UChar *aDestBuf,
                                   UInt   aDestLen,
                                   UInt*  aResultLen,
                                   void*  aWorkMem );
public:
/***********************************************************************
 * Description : 압축을 수행 할 때 호출하는 함수
 * 
 * aSrc		- [IN] : 실제 압축을 수행하고자 하는 소스가 들어 있는 버퍼, 이때 임의의 크기
 *			 를 적용 할 수 있다. 
 *
 * aSrcLen	- [IN] : aSrc버퍼의 길이
 *
 * aDest	- [IN] : 압축이 수행된 결과가 입력되는 버퍼, 이 버퍼의 크기는 반드시
 *		 IDU_COMPRESSION_MAX_OUTSIZE(소스버퍼의크기) 를 적용하여 생성한다.
 *
 * aDestLen	- [IN] : aDest버퍼의 길이 
 *
 * aResultLen	- [OUT]: 압축된 내용의 길이
 * aWorkMem	- [IN] : 압축시 해시테이블로 사용할 메모리를 같이 주어서 실행 시킨다.
 *		 이때 위의 IDU_COMPRESSION_WORK_SIZE크기로 메모리를 생성한다.
 *
 *
 *
 * aWorkMem의 초기화가 필요 없다.
 * 해시 테이블의 엔트리는 srcBuf의 특정 위치를 가리키는 포인터이다.
 * 여기서 해시 테이블은 workMem을 뜻한다. 즉 외부에서 받은 workMem을 내부에서 
 * 해시 테이블로 사용한다. 
 *
 * 그렇기 때문에 해시 테이블의 최대크기는 
 * "IDU_COMPRESSION_D_SIZE(해시키의 최대 크기) * 주소크기" 가 된다.
 *
 * iduCompression.cpp의 COMPRESSION_CHECK_MPOS_NON_DET(m_pos,m_off,aSrcBuf,sSrcPtr,M4_MAX_OFFSET)
 *	 이부분을 유심히 보면 그 이유를 알 수 있다. 
 *
 * 제대로 된 경우를 제외한 쓰레기 값이 들어가서 문제가 되는 경우를 생각해 보면,
 * 1. srcBuf의 처음 주소보다 작은 값이 들어 있을 경우( 0포함) 
 *	=> 위의 매크로함수에서 검사된다.
 *
 * 2. srcBuf의 마지막 주소보다 큰 값이 들어있을 경우 
 *	=> 위의 매크로함수를 보면 현재의 위치보다 큰 위치의 값은 탈락하게 되어있다.
 *
 * 3. 현재의 위치보다 작고 srcBuf보다 큰 값이 들어있을 경우( 즉, 쓰레기 값이 
 *    실제로 정당한 주소인 양 하고 있을 경우) 
 *	=> 이때는 위의 매크로에서는 정당하다고 판결이 난다. 하지만 아래에서 
 *	실제로 쓰레기 주소를 포인터로 하여 실제 데이터를 찾아서 검사를 하게 된다. 
 *	즉 아무리 쓰레기 값이라고 해도 포인터로써 동작할 때는 더이상 쓰레기가 아닌 값이 된다. 
 *	이때에 정당하지 않을 경우엔 마지 hash 값만 같고 실제 데이터는 다른 것처럼 결론이
 *	나게 되어 탈락하게 된다. 
 *
 **********************************************************************/
    static IDE_RC compress(UChar *aSrc,
                           UInt   aSrcLen,
                           UChar *aDest,
                           UInt   aDestLen,
                           UInt*  aResultLen,
                           void*  aWorkMem );
    
/***********************************************************************
 * Description : 압축된 내용을 원본 소스로 풀때 사용하는 함수
 * aSrc		- [IN] : 압축 내용이 있는 버퍼
 * aSrcLen	- [IN] : aSrc의 크기
 * aDest	- [IN] : 압축 해제 내용을 넣을 버퍼, 이것의 크기는 compress에서의
 *			 원본의 크기와 동일하다. 압축은 전과 후가 크기가 같음을 상기
 * aDestLen	- [IN] : aDest의 크기
 * aResultLen	- [OUT]: 압축 해제 내용의 크기
 **********************************************************************/
    static IDE_RC decompress(UChar *aSrc,
                             UInt   aSrcLen,
                             UChar *aDest,
                             UInt   aDestLen,
                             UInt  *aResultLen);
};


#endif  // _O_IDU_COMPRESSION_H_

 
