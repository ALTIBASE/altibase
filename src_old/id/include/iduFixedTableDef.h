/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFixedTableDef.h 82166 2018-02-01 07:26:29Z ahra.cho $
 **********************************************************************/

#ifndef _O_IDU_FIXED_TABLE_DEF_H_
# define _O_IDU_FIXED_TABLE_DEF_H_ 1

#include <idl.h>


/* ------------------------------------------------
 *  A4를 위한 Startup Flags
 * ----------------------------------------------*/
typedef enum
{
    IDU_STARTUP_INIT         = 0,
    IDU_STARTUP_PRE_PROCESS  = 1,  /* for DB Creation    */
    IDU_STARTUP_PROCESS      = 2,  /* for DB Creation    */
    IDU_STARTUP_CONTROL      = 3,  /* for Recovery       */
    IDU_STARTUP_META         = 4,  /* for upgrade meta   */
    IDU_STARTUP_SERVICE      = 5,  /* for normal service  */
    IDU_STARTUP_SHUTDOWN     = 6,  /* for normal shutdown */
    IDU_STARTUP_DOWNGRADE    = 7,  /* for downgrade meta   */

    IDU_STARTUP_MAX
} iduStartupPhase;

/* ----------------------------------------------------------------------------
 *  Fixed Table에서 제공하는 데이타 타입 및 Macros
 * --------------------------------------------------------------------------*/

typedef UShort iduFixedTableDataType;

#define IDU_FT_TYPE_MASK       (0x00FF)
#define IDU_FT_TYPE_CHAR       (0x0000)
#define IDU_FT_TYPE_BIGINT     (0x0001)
#define IDU_FT_TYPE_SMALLINT   (0x0002)
#define IDU_FT_TYPE_INTEGER    (0x0003)
#define IDU_FT_TYPE_DOUBLE     (0x0004)
#define IDU_FT_TYPE_UBIGINT    (0x0005)
#define IDU_FT_TYPE_USMALLINT  (0x0006)
#define IDU_FT_TYPE_UINTEGER   (0x0007)
#define IDU_FT_TYPE_VARCHAR    (0x0008)
#define IDU_FT_TYPE_POINTER    (0x1000)
/* BUG-43006 Fixed Table Indexing Filter */
#define IDU_FT_COLUMN_INDEX    (0x2000)

#define IDU_FT_SIZEOF_BIGINT      ID_SIZEOF(SLong)
#define IDU_FT_SIZEOF_SMALLINT    ID_SIZEOF(SShort)
#define IDU_FT_SIZEOF_INTEGER     ID_SIZEOF(SInt)
#define IDU_FT_SIZEOF_UBIGINT     ID_SIZEOF(SLong)
#define IDU_FT_SIZEOF_USMALLINT   ID_SIZEOF(SShort)
#define IDU_FT_SIZEOF_UINTEGER    ID_SIZEOF(SInt)
#define IDU_FT_SIZEOF_DOUBLE      ID_SIZEOF(SDouble)

#define IDU_FT_GET_TYPE(a)     ( a & IDU_FT_TYPE_MASK)
#define IDU_FT_IS_POINTER(a)   ((a & IDU_FT_TYPE_POINTER) != 0)
#define IDU_FT_MAX_PATH_LEN       (1024)
/* BUG-43006 Fixed Table Indexing Filter */
#define IDU_FT_IS_COLUMN_INDEX(a) ((a & IDU_FT_COLUMN_INDEX) != 0)

#define IDU_FT_OFFSETOF(s, m) \
    (UInt)((SChar*)&(((s *) NULL)->m) - ((SChar*)NULL))

#define IDU_FT_SIZEOF(s, m)   ID_SIZEOF( ((s *)NULL)->m )
    
#define IDU_FT_SIZEOF_CHAR(s, m)  IDU_FT_SIZEOF(s, m)

/* ----------------------------------------------------------------------------
 *  Fixed Table에서 제공하는 Struct
 * --------------------------------------------------------------------------*/
class iduFixedTableMemory;
struct idvSQL;

typedef IDE_RC (*iduFixedTableBuildFunc)( idvSQL              *aStatistics,
                                          void                *aHeader,
                                          void                *aDumpObj, // PROJ-1618
                                          iduFixedTableMemory *aMemory);

// smnf의 callback 을 호출하여, 미리 valid record 인지 검사한다.
typedef IDE_RC (*iduFixedTableFilterCallback)(idBool *aResult,
                                              void   *aIterator,
                                              void   *aRecord);
/*
 *  DataType을 알 수 없는 경우, Char 타입으로 변경하기 위한 Callback
 *
 *      aObj:       객체의 포인터
 *      aObjOffset: 객체 멤버의 Offset 위치 (실제 이 위치가 변환됨)
 *      aBuf:       컬럼이 만들어질 위치
 *      aBufSize:   대상 컬럼의 메모리 길이
 *      aDataType : 대상 데이타 타입 : varchar의 경우 0으로 끝나고,
 *                                     Char의 경우 ' '로 채움.
 *      리턴값 : 변경된 스트링의 크기 : Char의 경우 해당 타입길이이고,
 *                                      Varchar의 경우 변환된 데이타 길이임.
 *
 */
typedef UInt   (*iduFixedTableConvertCallback)(void        *aBaseObj,
                                               void        *aMember,
                                               UChar       *aBuf,
                                               UInt         aBufSize);

typedef IDE_RC (*iduFixedTableAllocRecordBuffer)(void *aHeader,
                                                 UInt aRecordCount,
                                                 void **aBuffer);

typedef IDE_RC (*iduFixedTableBuildRecord)(void                *aHeader,
                                           iduFixedTableMemory *aMemory,
                                           void                *aObj);

typedef struct iduFixedTableColDesc
{
    SChar                       *mName;
    //PR-14300  Fixed Table에서 64K가 넘는 객체의 필드값을
    // 엉뚱한 값으로 출력하는 문제 해결 ; mOffset을 UInt로 변경
    UInt                         mOffset; // Object에서의 위치
    UInt                         mLength; // Object에서 차지하는 크기
    iduFixedTableDataType        mDataType;
    iduFixedTableConvertCallback mConvCallback;

    /*
     * 아래는 실제 Row로 표현될 때의 크기와 offset을 표현한다.
     * A3의 경우, Desc로부터  SQL로 표현하여 Meta Table에 등록을
     * 하기 때문에 자동적으로 생성이 가능했지만,
     * A4는 직접 Schema에 대한 고려를 해야 하기 때문에
     * 아래의 값이 초기화 과정에서 결정되어야 한다.
     */
    UInt                    mColOffset; // Row 내에서의 위치
    UInt                    mColSize;   // Row 내에서의 Column 크기
    SChar                  *mTableName; // Table Name

} iduFixedTableColDesc;

typedef idBool (*iduFixedTableCheckKeyRange)( iduFixedTableMemory  * aMemory,
                                              iduFixedTableColDesc * aColDesc,
                                              void                ** aObj );
typedef enum
{
    IDU_FT_DESC_TRANS_NOT_USE = 0,
    IDU_FT_DESC_TRANS_USE
} iduFtDescTransUse;

typedef struct iduFixedTableDesc
{
    SChar                   *mTableName;   // Fixed Table 이름
    iduFixedTableBuildFunc   mBuildFunc;   // Record 생성 callback
    iduFixedTableColDesc    *mColDesc;     // column 정보
    iduStartupPhase          mEnablePhase; // 다단계 언제부터 사용 가능?
    UInt                     mSlotSize;    // Row의 크기 : init 시 계산됨
    UShort                   mColCount;    // 총 컬럼의 갯수 : init 시 계산됨
    iduFtDescTransUse        mUseTrans;    // Transaction 사용 여부
    iduFixedTableDesc       *mNext;
} iduFixedTableDesc;


#endif /* _O_IDU_FIXED_TABLE_DEF_H_ */
