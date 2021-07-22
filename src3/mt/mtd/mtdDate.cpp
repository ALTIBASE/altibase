/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtdDate.cpp 88554 2020-09-11 06:44:49Z donovan.seo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>
#include <mtuProperty.h>
#include <mtddlLexer.h>

#define MTD_DATE_ALIGN (ID_SIZEOF(UInt))
#define MTD_DATE_SIZE  (ID_SIZEOF(mtdDateType))

/* BUG-48113 DATE conversion malloc free remove */
#define MTD_YYGUTS_T_SIZE (160)
#define MTD_FORMAT_SIZE (96)
#define MTD_YYBUFFER_STATE_SIZE (64)
#define MTD_YY_BUFFER_NEW (0)

/* BUG-48113
 * src/mt/mtd/mtddl.cpp 에 존재하는 구조체이다.
 * malloc / free / memset 을 하지 않기 위해서
 * 이 구조체를 직접 접근할 필요가 있어서 mtddl.cpp 에서 복사해왔다.
 * 만약 Lexer가 변경되거나 Update되어서 해당 구조체가 변경되면
 * 아래 구조체 역시 변경해주어야한다.
 */
typedef struct yyguts_t
{
    /* User-defined. Not touched by flex. */
    YY_EXTRA_TYPE yyextra_r;
    FILE *yyin_r, *yyout_r;
    size_t yy_buffer_stack_top; /**< index of top of stack. */
    size_t yy_buffer_stack_max; /**< capacity of stack. */
    YY_BUFFER_STATE * yy_buffer_stack; /**< Stack as an array. */
    char yy_hold_char;
    int yy_n_chars;
    int yyleng_r;
    char *yy_c_buf_p;
    int yy_init;
    int yy_start;
    int yy_did_buffer_switch_on_eof;
    int yy_start_stack_ptr;
    int yy_start_stack_depth;
    int *yy_start_stack;
    int  yy_last_accepting_state;
    char* yy_last_accepting_cpos;
    int yylineno_r;
    int yy_flex_debug_r;
    char *yytext_r;
    int yy_more_flag;
    int yy_more_len;
} mtddl_yyguts_t; /* end struct yyguts_t */

#define MTDDL_YYGUTS_T_INIT( yyg )  \
{                                   \
    yyg->yyextra_r = NULL;          \
    yyg->yyin_r = (FILE *) 0;       \
    yyg->yyout_r = (FILE *) 0;      \
    yyg->yy_buffer_stack_top = 0;   \
    yyg->yy_buffer_stack_max = 0;   \
    yyg->yy_buffer_stack = 0;       \
    yyg->yy_hold_char = 0;          \
    yyg->yy_n_chars = 0;            \
    yyg->yyleng_r = 0;              \
    yyg->yy_c_buf_p = (char *) 0;   \
    yyg->yy_init = 0;               \
    yyg->yy_start = 0;              \
    yyg->yy_did_buffer_switch_on_eof = 0; \
    yyg->yy_start_stack_ptr = 0;          \
    yyg->yy_start_stack_depth = 0;        \
    yyg->yy_start_stack =  NULL;          \
    yyg->yy_last_accepting_state = 0;     \
    yyg->yy_last_accepting_cpos = NULL;   \
    yyg->yylineno_r = 0;                  \
    yyg->yy_flex_debug_r = 0;             \
    yyg->yytext_r = NULL;                 \
    yyg->yy_more_flag = 0;                \
    yyg->yy_more_len = 0;                 \
}

#define MTDDL_YYSTATE_INIT( b )     \
{                                   \
    b->yy_input_file = 0;           \
    b->yy_buf_pos = NULL ;          \
    b->yy_ch_buf  = NULL ;          \
    b->yy_buf_size = 0;             \
    b->yy_is_our_buffer = 0;        \
    b->yy_is_interactive = 0;       \
    b->yy_at_bol = 1;               \
    b->yy_bs_lineno = 0;            \
    b->yy_bs_column = 0;            \
    b->yy_fill_buffer = 0;          \
    b->yy_n_chars = 0;              \
    b->yy_buffer_status = MTD_YY_BUFFER_NEW;\
}

#define MTDDL_SET_BUFFER( b, yyg )          \
{                                           \
    yyg->yy_buffer_stack_max = 1;           \
    yyg->yy_buffer_stack_top = 0;           \
    yyg->yy_buffer_stack[yyg->yy_buffer_stack_top] = b; \
    yyg->yy_n_chars   = b->yy_n_chars;      \
    yyg->yy_c_buf_p   = b->yy_buf_pos;      \
    yyg->yytext_r     = b->yy_buf_pos;      \
    yyg->yy_hold_char = *yyg->yy_c_buf_p;   \
    yyg->yy_did_buffer_switch_on_eof = 1;   \
}

static
    const char* gMONTHName[12] = {
        "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
        "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };
static
    const char* gMonthName[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
static
    const char* gmonthName[12] = {
        "january", "february", "march", "april", "may", "june",
        "july", "august", "september", "october", "november", "december"
    };

static
    const char* gMONName[12] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
static
    const char* gMonName[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
static
    const char* gmonName[12] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };

static
    const char* gDAYName[7] = {
        "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
        "THURSDAY", "FRIDAY", "SATURDAY"
    };
static
    const char* gDayName[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };
static
    const char* gdayName[7] = {
        "sunday", "monday", "tuesday", "wednesday",
        "thursday", "friday", "saturday"
    };  
        
static
    const char* gDYName[7] = {
        "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
    };
static  
    const char* gDyName[7] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
static
    const char* gdyName[7] = {
        "sun", "mon", "tue", "wed", "thu", "fri", "sat"
    };

static
    const char* gRMMonth[12] = {
        "I", "II", "III", "IV", "V", "VI",
        "VII", "VIII", "IX", "X", "XI", "XII"
};
static
    const char* grmMonth[12] = {
        "i", "ii", "iii", "iv", "v", "vi",
        "vii", "viii", "ix", "x", "xi", "xii"
    };

const mtdDateType mtdDateNull = { -32768, 0, 0 }; /* PROJ-2632 */

const UChar mtdDateInterface::mNONE   = 0;
const UChar mtdDateInterface::mDIGIT  = 1;
const UChar mtdDateInterface::mALPHA  = 2;
const UChar mtdDateInterface::mSEPAR  = 3;
const UChar mtdDateInterface::mWHSP   = 4;

const UChar mtdDateInterface::mInputST[256] = {
    mNONE , mNONE , mNONE , mNONE , /* 00-03 */
    mNONE , mNONE , mNONE , mNONE , /* 04-07 */
    mNONE , mWHSP , mWHSP , mNONE , /* 08-0B | BS,HT,LF,VT */
    mNONE , mWHSP , mNONE , mNONE , /* 0C-0F | FF,CR,SO,SI */
    mNONE , mNONE , mNONE , mNONE , /* 10-13 */
    mNONE , mNONE , mNONE , mNONE , /* 14-17 */
    mNONE , mNONE , mNONE , mNONE , /* 18-1B */
    mNONE , mNONE , mNONE , mNONE , /* 1C-1F */
    mWHSP , mSEPAR, mNONE , mSEPAR, /* 20-23 |  !"# */
    mSEPAR, mSEPAR, mNONE , mSEPAR, /* 24-27 | $%&' */
    mSEPAR, mSEPAR, mSEPAR, mSEPAR, /* 28-2B | ()*+ */
    mSEPAR, mSEPAR, mSEPAR, mSEPAR, /* 2C-2F | ,-./ */
    mDIGIT, mDIGIT, mDIGIT, mDIGIT, /* 30-33 | 0123 */
    mDIGIT, mDIGIT, mDIGIT, mDIGIT, /* 34-37 | 4567 */
    mDIGIT, mDIGIT, mSEPAR, mSEPAR, /* 38-3B | 89:; */
    mSEPAR, mSEPAR, mSEPAR, mSEPAR, /* 3C-3F | <=>? */
    mSEPAR, mALPHA, mALPHA, mALPHA, /* 40-43 | @ABC */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 44-47 | DEFG */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 48-4B | HIJK */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 4C-4F | LMNO */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 50-53 | PQRS */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 54-57 | TUVW */
    mALPHA, mALPHA, mALPHA, mSEPAR, /* 58-5B | XYZ[ */
    mSEPAR, mSEPAR, mSEPAR, mSEPAR, /* 5C-5F | \]^_ */
    mSEPAR, mALPHA, mALPHA, mALPHA, /* 60-63 | `abc */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 64-67 | defg */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 68-6B | hijk */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 6C-6F | lmno */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 70-73 | pqrs */
    mALPHA, mALPHA, mALPHA, mALPHA, /* 74-77 | tuvw */
    mALPHA, mALPHA, mALPHA, mSEPAR, /* 78-7B | xyz{ */
    mSEPAR, mSEPAR, mSEPAR, mSEPAR, /* 7C-7F | |}~  */
    mNONE , mNONE , mNONE , mNONE , /* 80-83 */
    mNONE , mNONE , mNONE , mNONE , /* 84-87 */
    mNONE , mNONE , mNONE , mNONE , /* 88-8B */
    mNONE , mNONE , mNONE , mNONE , /* 8C-8F */
    mNONE , mNONE , mNONE , mNONE , /* 90-93 */
    mNONE , mNONE , mNONE , mNONE , /* 94-97 */
    mNONE , mNONE , mNONE , mNONE , /* 98-9B */
    mNONE , mNONE , mNONE , mNONE , /* 9C-9F */
    mNONE , mNONE , mNONE , mNONE , /* A0-A3 */
    mNONE , mNONE , mNONE , mNONE , /* A4-A7 */
    mNONE , mNONE , mNONE , mNONE , /* A8-AB */
    mNONE , mNONE , mNONE , mNONE , /* AC-AF */
    mNONE , mNONE , mNONE , mNONE , /* B0-B3 */
    mNONE , mNONE , mNONE , mNONE , /* B4-B7 */
    mNONE , mNONE , mNONE , mNONE , /* B8-BB */
    mNONE , mNONE , mNONE , mNONE , /* BC-BF */
    mNONE , mNONE , mNONE , mNONE , /* C0-C3 */
    mNONE , mNONE , mNONE , mNONE , /* C4-C7 */
    mNONE , mNONE , mNONE , mNONE , /* C8-CB */
    mNONE , mNONE , mNONE , mNONE , /* CC-CF */
    mNONE , mNONE , mNONE , mNONE , /* D0-D3 */
    mNONE , mNONE , mNONE , mNONE , /* D4-D7 */
    mNONE , mNONE , mNONE , mNONE , /* D8-DB */
    mNONE , mNONE , mNONE , mNONE , /* DC-DF */
    mNONE , mNONE , mNONE , mNONE , /* E0-E3 */
    mNONE , mNONE , mNONE , mNONE , /* E4-E7 */
    mNONE , mNONE , mNONE , mNONE , /* E8-EB */
    mNONE , mNONE , mNONE , mNONE , /* EC-EF */
    mNONE , mNONE , mNONE , mNONE , /* F0-F3 */
    mNONE , mNONE , mNONE , mNONE , /* F4-F7 */
    mNONE , mNONE , mNONE , mNONE , /* F8-FB */
    mNONE , mNONE , mNONE , mNONE   /* FC-FF */
};

const UChar mtdDateInterface::mDaysOfMonth[2][13] = {
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } ,  // 
    { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }    // 윤년
};

const UInt mtdDateInterface::mAccDaysOfMonth[2][13] = {
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 }, // 
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }  // 윤년
};

const UInt mtdDateInterface::mHashMonth[12] = {
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[0], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[1], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[2], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[3], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[4], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[5], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[6], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[7], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[8], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[9], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[10], 3),
    mtc::hash(mtc::hashInitialValue, (const UChar*)gMONName[11], 3)
};

static IDE_RC mtdInitialize( UInt aNo );

static IDE_RC mtdEstimate( UInt * aColumnSize,
                           UInt * aArguments,
                           SInt * aPrecision,
                           SInt * aScale );

static IDE_RC mtdValue( mtcTemplate* aTemplate,
                        mtcColumn*   aColumn,
                        void*        aValue,
                        UInt*        aValueOffset,
                        UInt         aValueSize,
                        const void*  aToken,
                        UInt         aTokenLength,
                        IDE_RC*      aResult );

static UInt mtdActualSize( const mtcColumn* aColumn,
                           const void*      aRow );

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static UInt mtdHash( UInt             aHash,
                     const mtcColumn* aColumn,
                     const void*      aRow );

static idBool mtdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static SInt mtdDateLogicalAscComp( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 );

static SInt mtdDateLogicalDescComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 );

static SInt mtdDateFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static SInt mtdDateFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static SInt mtdDateMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );

static SInt mtdDateMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                      mtdValueInfo * aValueInfo2 );

static SInt mtdDateStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

static SInt mtdDateStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 );

static SInt mtdDateStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                           mtdValueInfo * aValueInfo2 );

static SInt mtdDateStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * aValueInfo2 );

static void mtdEndian( void* aValue );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static SDouble mtdSelectivityDate( void     * aColumnMax,  
                                   void     * aColumnMin,  
                                   void     * aValueMax,   
                                   void     * aValueMin,
                                   SDouble    aBoundFactor,
                                   SDouble    aTotalRecordCnt );

static IDE_RC mtdEncode( mtcColumn  * aColumn,
                         void       * aValue,
                         UInt         aValueSize,
                         UChar      * aCompileFmt,
                         UInt         aCompileFmtLen,
                         UChar      * aText,
                         UInt       * aTextLen,
                         IDE_RC     * aRet );

static IDE_RC mtdDecode( mtcTemplate * aTemplate,
                         mtcColumn   * aColumn,
                         void        * aValue,
                         UInt        * aValueSize,
                         UChar       * aCompileFmt,
                         UInt          aCompileFmtLen,
                         UChar       * aText,
                         UInt          aTextLen,
                         IDE_RC      * aRet );

static IDE_RC mtdCompileFmt( mtcColumn  * aColumn,
                             UChar      * aCompiledFmt,
                             UInt       * aCompiledFmtLen,
                             UChar      * aFormatString,
                             UInt         aFormatStringLength,
                             IDE_RC     * aRet );

static IDE_RC mtdValueFromOracle( mtcColumn*  aColumn,
                                  void*       aValue,
                                  UInt*       aValueOffset,
                                  UInt        aValueSize,
                                  const void* aOracleValue,
                                  SInt        aOracleLength,
                                  IDE_RC*     aResult );

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt mtdNullValueSize();

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"DATE" }
};

static mtcColumn mtdColumn;

mtdModule mtdDate = {
    mtdTypeName,
    &mtdColumn,
    MTD_DATE_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
      0, 0, 0, 0, 0 },
    MTD_DATE_ALIGN,
    MTD_GROUP_DATE|
      MTD_CANON_NEEDLESS|
      MTD_CREATE_ENABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_ENABLE|
      MTD_SEARCHABLE_PRED_BASIC|
      MTD_SQL_DATETIME_SUB_TRUE|
      MTD_LITERAL_TRUE|
      MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
      MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
      MTD_DATA_STORE_MTDVALUE_FALSE|  // PROJ-1705
      MTD_PSM_TYPE_ENABLE, // PROJ-1904
    19,
    0,
    0,
    (void*)&mtdDateNull,
    mtdInitialize,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtd::getPrecisionNA,
    mtdSetNull,
    mtdHash,
    mtdIsNull,
    mtd::isTrueNA,
    {
        mtdDateLogicalAscComp,    // Logical Comparison
        mtdDateLogicalDescComp
    },
    {
        // Key Comparison
        {
            // mt value들 간의 compare
            mtdDateFixedMtdFixedMtdKeyAscComp, // Ascending Key Comparison
            mtdDateFixedMtdFixedMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value들 간의 compare
            mtdDateMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdDateMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare
            mtdDateStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdDateStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare
            mtdDateStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdDateStoredStoredKeyDescComp // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 fixed mt value들 간의 compare */
            mtdDateFixedMtdFixedMtdKeyAscComp,
            mtdDateFixedMtdFixedMtdKeyDescComp
        }
        ,
        {
            /* PROJ-2433 : index Direct key와 mt value들 간의 compare */
            mtdDateMtdMtdKeyAscComp,
            mtdDateMtdMtdKeyDescComp
        }
    },
    mtd::canonizeDefault,
    mtdEndian,
    mtdValidate,
    mtdSelectivityDate,
    mtdEncode,
    mtdDecode,
    mtdCompileFmt,
    mtdValueFromOracle,
    mtd::makeColumnInfoDefault,

    // BUG-28934
    mtk::mergeAndRangeDefault,
    mtk::mergeOrRangeListDefault,

    {
        // PROJ-1705
        mtdStoredValue2MtdValue,
        // PROJ-2429
        mtd::mtdStoredValue2MtdValue4CompressColumn
    },
    mtdNullValueSize,
    mtd::mtdHeaderSizeDefault,

    // PROJ-2399
    mtd::mtdStoreSizeDefault
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdDate, aNo ) != IDE_SUCCESS );

    // mtdColumn의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdDate,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * aArguments,
                    SInt * /*aPrecision*/,
                    SInt * /*aScale*/ )
{
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );

    *aColumnSize = MTD_DATE_SIZE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdValue( mtcTemplate* aTemplate,
                 mtcColumn*   aColumn,
                 void*        aValue,
                 UInt*        aValueOffset,
                 UInt         aValueSize,
                 const void*  aToken,
                 UInt         aTokenLength,
                 IDE_RC*      aResult )
{
    UInt         sValueOffset;
    mtdDateType* sValue;

    IDE_ASSERT( aTemplate != NULL );
    IDE_ASSERT( aTemplate->dateFormat != NULL );
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_DATE_ALIGN );

    if( sValueOffset + MTD_DATE_SIZE <= aValueSize )
    {
        sValue = (mtdDateType*)( (UChar*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            *sValue = mtdDateNull;
        }
        else
        {
            idlOS::memset((void*)sValue, 0x00, ID_SIZEOF(mtdDateType));

            IDE_TEST( mtdDateInterface::toDate(
                          sValue,
                          (UChar*)aToken,
                          (UShort)aTokenLength,
                          (UChar*)aTemplate->dateFormat,
                          idlOS::strlen( aTemplate->dateFormat ) )
                      != IDE_SUCCESS );

            // PROJ-1436
            // dateFormat을 참조했음을 표시한다.
            aTemplate->dateFormatRef = ID_TRUE;
        }
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_DATE_SIZE;
        
        *aResult = IDE_SUCCESS;
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
    return MTD_DATE_SIZE;
}

void mtdSetNull( const mtcColumn* /*aColumn*/,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        *(mtdDateType*)aRow = mtdDateNull;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    mtdDateType* sDate = (mtdDateType*)aRow;

    SShort             sYear;
    UChar              sMonth;
    UChar              sDay;
    UChar              sHour;
    UChar              sMinute;
    UChar              sSecond;
    UInt               sMicroSecond;

    sYear        = mtdDateInterface::year(sDate);
    sMonth       = mtdDateInterface::month(sDate);
    sDay         = mtdDateInterface::day(sDate);
    sHour        = mtdDateInterface::hour(sDate);
    sMinute      = mtdDateInterface::minute(sDate);
    sSecond      = mtdDateInterface::second(sDate);
    sMicroSecond = mtdDateInterface::microSecond(sDate);
        
    aHash = mtc::hash( aHash,
                       (const UChar*)&sYear,
                       ID_SIZEOF(sYear) );
    aHash = mtc::hash( aHash,
                       (const UChar*)&sMonth,
                       ID_SIZEOF(sMonth) );
    aHash = mtc::hash( aHash,
                       (const UChar*)&sDay,
                       ID_SIZEOF(sDay) );
    aHash = mtc::hash( aHash,
                       (const UChar*)&sHour,
                       ID_SIZEOF(sHour) );
    aHash = mtc::hash( aHash,
                       (const UChar*)&sMinute,
                       ID_SIZEOF(sMinute) );
    aHash = mtc::hash( aHash,
                       (const UChar*)&sSecond,
                       ID_SIZEOF(sSecond) );
    aHash = mtc::hash( aHash,
                       (const UChar*)&sMicroSecond,
                       ID_SIZEOF(sMicroSecond) );
    
    return aHash;
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    SInt               sResult;

    sResult = MTD_DATE_IS_NULL( (mtdDateType*)aRow );

    return ( sResult == 1 ) ? ID_TRUE : ID_FALSE;
}

SInt mtdDateLogicalAscComp( mtdValueInfo * aValueInfo1,
                            mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType        * sValue1;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDateType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sNull1  = MTD_DATE_IS_NULL( sValue1 );

    //---------
    // value2
    //---------
    sValue2 = (mtdDateType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sNull2  = MTD_DATE_IS_NULL( sValue2 );

    //---------
    // compare 
    //---------
    
    if( !sNull1 && !sNull2 )
    {
        // Enhancement-13065 Date 비교 성능 개선
        // mtdDateInterface에 compare 함수를 구현해서
        // 비교 연산의 책임을 mtdDateInterface에 위임함.
        //
        // null에 대한 비교 정책은 mtdDateModule이 가지고 있는다.
        // mtdDateInterface::compare는 단순히 값의 크기를 비교할 뿐,
        // null에 대한 비교 정책은 가지고 있지 않기 때문에
        // null 체크는 mtdDate에서 해준다.

        return mtdDateInterface::compare( sValue1, sValue2 );
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateLogicalDescComp( mtdValueInfo * aValueInfo1,
                             mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType        * sValue1;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDateType*)MTD_VALUE_OFFSET_USELESS( aValueInfo1 );
    sNull1  = MTD_DATE_IS_NULL( sValue1 );

    //---------
    // value2
    //---------
    sValue2 = (mtdDateType*)MTD_VALUE_OFFSET_USELESS( aValueInfo2 );
    sNull2  = MTD_DATE_IS_NULL( sValue2 );

    //---------
    // compare
    //---------        

    if( !sNull1 && !sNull2 )
    {
        return mtdDateInterface::compare(sValue2, sValue1);
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateFixedMtdFixedMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType        * sValue1;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDateType*)MTD_VALUE_FIXED( aValueInfo1 );
    sNull1  = MTD_DATE_IS_NULL( sValue1 );

    //---------
    // value2
    //---------
    sValue2 = (mtdDateType*)MTD_VALUE_FIXED( aValueInfo2 );
    sNull2  = MTD_DATE_IS_NULL( sValue2 );

    //---------
    // compare 
    //---------
    
    if( !sNull1 && !sNull2 )
    {
        // Enhancement-13065 Date 비교 성능 개선
        // mtdDateInterface에 compare 함수를 구현해서
        // 비교 연산의 책임을 mtdDateInterface에 위임함.
        //
        // null에 대한 비교 정책은 mtdDateModule이 가지고 있는다.
        // mtdDateInterface::compare는 단순히 값의 크기를 비교할 뿐,
        // null에 대한 비교 정책은 가지고 있지 않기 때문에
        // null 체크는 mtdDate에서 해준다.

        return mtdDateInterface::compare( sValue1, sValue2 );
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateFixedMtdFixedMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType        * sValue1;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDateType*)MTD_VALUE_FIXED( aValueInfo1 );
    sNull1  = MTD_DATE_IS_NULL( sValue1 );

    //---------
    // value2
    //---------
    sValue2 = (mtdDateType*)MTD_VALUE_FIXED( aValueInfo2 );
    sNull2  = MTD_DATE_IS_NULL( sValue2 );

    //---------
    // compare
    //---------        

    if( !sNull1 && !sNull2 )
    {
        return mtdDateInterface::compare(sValue2, sValue1);
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                              mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType        * sValue1;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDateType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdDate.staticNull );

    sNull1  = MTD_DATE_IS_NULL( sValue1 );

    //---------
    // value2
    //---------
    sValue2 = (mtdDateType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdDate.staticNull );

    sNull2  = MTD_DATE_IS_NULL( sValue2 );

    //---------
    // compare 
    //---------
    
    if( !sNull1 && !sNull2 )
    {
        // Enhancement-13065 Date 비교 성능 개선
        // mtdDateInterface에 compare 함수를 구현해서
        // 비교 연산의 책임을 mtdDateInterface에 위임함.
        //
        // null에 대한 비교 정책은 mtdDateModule이 가지고 있는다.
        // mtdDateInterface::compare는 단순히 값의 크기를 비교할 뿐,
        // null에 대한 비교 정책은 가지고 있지 않기 때문에
        // null 체크는 mtdDate에서 해준다.

        return mtdDateInterface::compare( sValue1, sValue2 );
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType        * sValue1;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;

    //---------
    // value1
    //---------
    sValue1 = (mtdDateType*)
        mtd::valueForModule( (smiColumn*)aValueInfo1->column,
                             aValueInfo1->value,
                             aValueInfo1->flag,
                             mtdDate.staticNull );

    sNull1  = MTD_DATE_IS_NULL( sValue1 );

    //---------
    // value2
    //---------
    sValue2 = (mtdDateType*)
        mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                             aValueInfo2->value,
                             aValueInfo2->flag,
                             mtdDate.staticNull );

    sNull2  = MTD_DATE_IS_NULL( sValue2 );

    //---------
    // compare
    //---------        

    if( !sNull1 && !sNull2 )
    {
        return mtdDateInterface::compare(sValue2, sValue1);
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdDateType          sValue1;
    mtdDateType        * sValue1Ptr;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;
    UInt                 sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        // year ( SShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.year, aValueInfo1->value );
 
        // mon_day_hour ( UShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.mon_day_hour,
                              ((UChar*)aValueInfo1->value) +
                              ID_SIZEOF(SShort) );
 
        // min_sec_min ( UInt ) 
        ID_INT_BYTE_ASSIGN( &sValue1.min_sec_mic,
                            ((UChar*)aValueInfo1->value) +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(SShort) );

        sValue1Ptr = &sValue1;
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sValue1Ptr = (mtdDateType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
    }

    sNull1 = MTD_DATE_IS_NULL( sValue1Ptr );

    //-------
    // value2
    //-------
    sValue2 = (mtdDateType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdDate.staticNull );

    sNull2 = MTD_DATE_IS_NULL( sValue2 );

    //-------
    // Compare
    //-------    

    if( !sNull1 && !sNull2 )
    {
        // Enhancement-13065 Date 비교 성능 개선
        // mtdDateInterface에 compare 함수를 구현해서
        // 비교 연산의 책임을 mtdDateInterface에 위임함.
        //
        // null에 대한 비교 정책은 mtdDateModule이 가지고 있는다.
        // mtdDateInterface::compare는 단순히 값의 크기를 비교할 뿐,
        // null에 대한 비교 정책은 가지고 있지 않기 때문에
        // null 체크는 mtdDate에서 해준다.

        return mtdDateInterface::compare( sValue1Ptr, sValue2 );
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                  mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdDateType          sValue1;
    mtdDateType        * sValue1Ptr;
    mtdDateType        * sValue2;
    SInt                 sNull1;
    SInt                 sNull2;
    UInt                 sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        // year ( SShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.year, aValueInfo1->value );
 
        // mon_day_hour ( UShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.mon_day_hour,
                              ((UChar*)aValueInfo1->value) +
                              ID_SIZEOF(SShort) );
 
        // min_sec_min ( UInt ) 
        ID_INT_BYTE_ASSIGN( &sValue1.min_sec_mic,
                            ((UChar*)aValueInfo1->value) +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(SShort) );

        sValue1Ptr = &sValue1;
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sValue1Ptr = (mtdDateType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
    }

    sNull1 = MTD_DATE_IS_NULL( sValue1Ptr );

    //-------
    // value2
    //-------
    sValue2 = (mtdDateType*)
                mtd::valueForModule( (smiColumn*)aValueInfo2->column,
                                     aValueInfo2->value,
                                     aValueInfo2->flag,
                                     mtdDate.staticNull );

    sNull2 = MTD_DATE_IS_NULL( sValue2 );

    //-------
    // Compare
    //-------    

    if( !sNull1 && !sNull2 )
    {
        return mtdDateInterface::compare(sValue2, sValue1Ptr);
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                    mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType      sValue1;
    mtdDateType      sValue2;
    mtdDateType    * sValue1Ptr;
    mtdDateType    * sValue2Ptr;
    SInt             sNull1;
    SInt             sNull2;    
    UInt             sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        // year ( SShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.year, aValueInfo1->value );
 
        // mon_day_hour ( UShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.mon_day_hour,
                              ((UChar*)aValueInfo1->value) +
                              ID_SIZEOF(SShort) );
 
        // min_sec_min ( UInt ) 
        ID_INT_BYTE_ASSIGN( &sValue1.min_sec_mic,
                            ((UChar*)aValueInfo1->value) +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(SShort) );

        sValue1Ptr = &sValue1;
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sValue1Ptr = (mtdDateType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
    }

    sNull1 = MTD_DATE_IS_NULL( sValue1Ptr );
    
    //-------
    // value2
    //-------

    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        // year ( SShort )
        ID_SHORT_BYTE_ASSIGN( &sValue2.year, aValueInfo2->value );
 
        // mon_day_hour ( UShort )
        ID_SHORT_BYTE_ASSIGN( &sValue2.mon_day_hour,
                              ((UChar*)aValueInfo2->value) +
                              ID_SIZEOF(SShort) );
 
        // min_sec_min ( UInt ) 
        ID_INT_BYTE_ASSIGN( &sValue2.min_sec_mic,
                            ((UChar*)aValueInfo2->value) +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(SShort) );

        sValue2Ptr = &sValue2;
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sValue2Ptr = (mtdDateType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
    }

    sNull2 = MTD_DATE_IS_NULL( sValue2Ptr );

    //-------
    // Compare
    //-------    

    if( !sNull1 && !sNull2 )
    {
        // Enhancement-13065 Date 비교 성능 개선
        // mtdDateInterface에 compare 함수를 구현해서
        // 비교 연산의 책임을 mtdDateInterface에 위임함.
        //
        // null에 대한 비교 정책은 mtdDateModule이 가지고 있는다.
        // mtdDateInterface::compare는 단순히 값의 크기를 비교할 뿐,
        // null에 대한 비교 정책은 가지고 있지 않기 때문에
        // null 체크는 mtdDate에서 해준다.

        return mtdDateInterface::compare( sValue1Ptr, sValue2Ptr );
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}

SInt mtdDateStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 )
{
 /***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdDateType      sValue1;
    mtdDateType      sValue2;
    mtdDateType    * sValue1Ptr;
    mtdDateType    * sValue2Ptr;
    SInt             sNull1;
    SInt             sNull2;
    UInt             sDummy;

    //---------
    // value1
    //---------    
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인 경우 store type을mt type으로
    // 변환해서 실제 데이터를 가져온다.
    if ( (((smiColumn*)aValueInfo1->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        // year ( SShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.year, aValueInfo1->value );
 
        // mon_day_hour ( UShort )
        ID_SHORT_BYTE_ASSIGN( &sValue1.mon_day_hour,
                              ((UChar*)aValueInfo1->value) +
                              ID_SIZEOF(SShort) );
 
        // min_sec_min ( UInt ) 
        ID_INT_BYTE_ASSIGN( &sValue1.min_sec_mic,
                            ((UChar*)aValueInfo1->value) +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(SShort) );

        sValue1Ptr = &sValue1;
    }
    else
    {
        IDE_DASSERT( aValueInfo1->length == ID_SIZEOF(smOID) );

        sValue1Ptr = (mtdDateType*)
                      mtc::getCompressionColumn( aValueInfo1->value,
                                                 (smiColumn*)aValueInfo1->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
    }

    sNull1 = MTD_DATE_IS_NULL( sValue1Ptr );
    
    //-------
    // value2
    //-------

    if ( (((smiColumn*)aValueInfo2->column)->flag & SMI_COLUMN_COMPRESSION_MASK) !=
         SMI_COLUMN_COMPRESSION_TRUE )
    {
        // year ( SShort )
        ID_SHORT_BYTE_ASSIGN( &sValue2.year, aValueInfo2->value );
 
        // mon_day_hour ( UShort )
        ID_SHORT_BYTE_ASSIGN( &sValue2.mon_day_hour,
                              ((UChar*)aValueInfo2->value) +
                              ID_SIZEOF(SShort) );
 
        // min_sec_min ( UInt ) 
        ID_INT_BYTE_ASSIGN( &sValue2.min_sec_mic,
                            ((UChar*)aValueInfo2->value) +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(SShort) );

        sValue2Ptr = &sValue2;
    }
    else
    {
        IDE_DASSERT( aValueInfo2->length == ID_SIZEOF(smOID) );

        sValue2Ptr = (mtdDateType*)
                      mtc::getCompressionColumn( aValueInfo2->value,
                                                 (smiColumn*)aValueInfo2->column,
                                                 ID_FALSE, // aUseColumnOffset
                                                 &sDummy );
    }

    sNull2 = MTD_DATE_IS_NULL( sValue2Ptr );

    //-------
    // Compare
    //-------
    
    if( !sNull1 && !sNull2 )
    {
        return mtdDateInterface::compare(sValue2Ptr, sValue1Ptr);
    }
    if( sNull1 && !sNull2 )
    {
        return 1;
    }
    if( !sNull1 /*&& sNull2*/ )
    {
        return -1;
    }
    return 0;
}


void mtdEndian( void* aValue )
{
    UChar* sValue;
    UChar  sIntermediate;
    
    sValue = (UChar*)&(((mtdDateType*)aValue)->year);
    
    sIntermediate = sValue[0];
    sValue[0]     = sValue[1];
    sValue[1]     = sIntermediate;

    sValue = (UChar*)&(((mtdDateType*)aValue)->mon_day_hour);
    
    sIntermediate = sValue[0];
    sValue[0]     = sValue[1];
    sValue[1]     = sIntermediate;
    
    sValue = (UChar*)&(((mtdDateType*)aValue)->min_sec_mic);

    sIntermediate = sValue[0];
    sValue[0]     = sValue[3];
    sValue[3]     = sIntermediate;
    sIntermediate = sValue[1];
    sValue[1]     = sValue[2];
    sValue[2]     = sIntermediate;
}


IDE_RC mtdValidate( mtcColumn * aColumn,
                    void      * aValue,
                    UInt        aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
        
    mtdDateType * sVal = (mtdDateType*)aValue;
    
    IDE_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL );
    
    IDE_TEST_RAISE( aValueSize != ID_SIZEOF(mtdDateType), ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    IDE_TEST( mtc::initializeColumn( aColumn,
                                     & mtdDate,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );    
    
    if( mtdIsNull( aColumn, aValue ) != ID_TRUE )
    {
        IDE_TEST_RAISE( mtdDateInterface::month(sVal)       <  1  ||
                        mtdDateInterface::month(sVal)       > 12  ||
                        mtdDateInterface::day(sVal)         <  1  ||
                        mtdDateInterface::day(sVal)         > 31  ||
                        mtdDateInterface::hour(sVal)        > 23  ||
                        mtdDateInterface::minute(sVal)      > 59  ||
                        mtdDateInterface::microSecond(sVal) > 999999,
                        ERR_INVALID_VALUE);
        
        if( mtdDateInterface::month(sVal) ==  4 ||
            mtdDateInterface::month(sVal) ==  6 ||
            mtdDateInterface::month(sVal) ==  9 ||
            mtdDateInterface::month(sVal) == 11 )
        {
            IDE_TEST_RAISE( mtdDateInterface::day(sVal) == 31,
                            ERR_INVALID_VALUE );
        }
        else if( mtdDateInterface::month(sVal) == 2 )
        {
            if ( mtdDateInterface::isLeapYear( mtdDateInterface::year( sVal ) ) == ID_TRUE )
            {
                IDE_TEST_RAISE( mtdDateInterface::day(sVal) > 29,
                                ERR_INVALID_VALUE );
            }
            else
            {
                IDE_TEST_RAISE( mtdDateInterface::day(sVal) > 28,
                                ERR_INVALID_VALUE );
            }
        }

        /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
        IDE_TEST_RAISE( ( mtdDateInterface::year( sVal ) == 1582 ) &&
                        ( mtdDateInterface::month( sVal ) == 10 ) &&
                        ( 4 < mtdDateInterface::day( sVal ) ) && ( mtdDateInterface::day( sVal ) < 15 ),
                        ERR_INVALID_VALUE );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NULL);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE));
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }
    IDE_EXCEPTION( ERR_INVALID_VALUE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SDouble mtdSelectivityDate( void     * aColumnMax, 
                            void     * aColumnMin, 
                            void     * aValueMax,  
                            void     * aValueMin,
                            SDouble    aBoundFactor,
                            SDouble    aTotalRecordCnt )
{
/***********************************************************************
 *
 * Description :
 *    DATE 의 Selectivity 추출 함수
 *
 * Implementation :
 *
 *      1. NULL 검사 : S = DS
 *      2. ColumnMin > ValueMax 또는 ColumnMax < ValueMin 검증 :
 *         S = 1 / totalRecordCnt
 *      3. ValueMin, ValueMax 보정
 *       - ValueMin < ColumnMin 검증 : ValueMin = ColumnMin (보정)
 *       - ValueMax > ColumnMax 검증 : ValueMax = ColumnMax (보정)
 *      4. ColumnMin > ColumnMax 검증 (DASSERT)
 *      5. ColumnMax == ColumnMin 검증 (분모값) : S = 1
 *      6. ValueMax <= ValueMin 검증 (분자값) : S = 1 / totalRecordCnt
 *         ex) i1 < 1 and i1 > 3  -> ValueMax:1, ValueMin:3
 *      7. Etc : S = (ValueMax - ValueMin) / (ColumnMax - ColumnMin)
 *       - <=, >=, BETWEEN 에 대해 경계값 보정
 *
 *    cf) 3 번이후 분모값 획득 실패시 : S = DS
 *
 ***********************************************************************/
    
    mtdDateType   * sColumnMax;
    mtdDateType   * sColumnMin;
    mtdDateType   * sValueMax;
    mtdDateType   * sValueMin;
    mtdIntervalType sInterval1;
    mtdIntervalType sInterval2;
    mtdIntervalType sInterval3;
    mtdIntervalType sInterval4;
    mtdValueInfo    sColumnMaxInfo;
    mtdValueInfo    sColumnMinInfo;
    mtdValueInfo    sValueMaxInfo;
    mtdValueInfo    sValueMinInfo;

    SDouble       sDenominator;  // 분모값
    SDouble       sNumerator;    // 분자값
    SDouble       sSelectivity;

    sColumnMax = (mtdDateType*) aColumnMax;
    sColumnMin = (mtdDateType*) aColumnMin;
    sValueMax  = (mtdDateType*) aValueMax;
    sValueMin  = (mtdDateType*) aValueMin;
    sSelectivity = MTD_DEFAULT_SELECTIVITY;

    //------------------------------------------------------
    // Selectivity 계산
    //------------------------------------------------------

    if ( ( mtdIsNull( NULL, aColumnMax ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aColumnMin ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aValueMax  ) == ID_TRUE ) ||
         ( mtdIsNull( NULL, aValueMin  ) == ID_TRUE ) )
    {
        //------------------------------------------------------
        // 1. NULL 검사 : 계산할 수 없음
        //------------------------------------------------------

        // MTD_DEFAULT_SELECTIVITY;
    }
    else
    {
        sColumnMaxInfo.column = NULL;
        sColumnMaxInfo.value  = sColumnMax;
        sColumnMaxInfo.flag   = MTD_OFFSET_USELESS;

        sColumnMinInfo.column = NULL;
        sColumnMinInfo.value  = sColumnMin;
        sColumnMinInfo.flag   = MTD_OFFSET_USELESS;

        sValueMaxInfo.column = NULL;
        sValueMaxInfo.value  = sValueMax;
        sValueMaxInfo.flag   = MTD_OFFSET_USELESS;

        sValueMinInfo.column = NULL;
        sValueMinInfo.value  = sValueMin;
        sValueMinInfo.flag   = MTD_OFFSET_USELESS;
        

        if ( ( mtdDateLogicalAscComp( &sColumnMinInfo,
                                      &sValueMaxInfo ) > 0 )
             ||
             ( mtdDateLogicalAscComp( &sColumnMaxInfo,
                                      &sValueMinInfo ) < 0 ) )
        {
            //------------------------------------------------------
            // 2. ColumnMin > ValueMax 또는 ColumnMax < ValueMin 검증
            //------------------------------------------------------

            sSelectivity = 1 / aTotalRecordCnt;
        }
        else
        {
            //------------------------------------------------------
            // 3. 보정
            //  - ValueMin < ColumnMin 검증 : ValueMin = ColumnMin
            //  - ValueMax > ColumnMax 검증 : ValueMax = ColumnMax
            //------------------------------------------------------

            sValueMin = ( mtdDateLogicalAscComp(
                              &sValueMinInfo,
                              &sColumnMinInfo ) < 0 ) ?
                sColumnMin: sValueMin;

            sValueMax = ( mtdDateLogicalAscComp(
                              &sValueMaxInfo,
                              &sColumnMaxInfo ) > 0 ) ?
                sColumnMax: sValueMax;

            //------------------------------------------------------
            // 분모값 (ColumnMax - ColumnMin) 값 획득
            //------------------------------------------------------

            // Date(aColumnMax) -> interval1 로 전환
            // Date(aColumnMin) -> interval2 로 전환
            // Date(aValueMax) -> interval1 로 전환
            // Date(aValueMin) -> interval2 로 전환

            if ( ( mtdDateInterface::convertDate2Interval( sColumnMax,
                                                           &sInterval1 )
                   == IDE_SUCCESS ) &&
                 ( mtdDateInterface::convertDate2Interval( sColumnMin,
                                                           &sInterval2 )
                   == IDE_SUCCESS ) &&
                 ( mtdDateInterface::convertDate2Interval( sValueMax,
                                                           &sInterval3 )
                   == IDE_SUCCESS ) &&
                 ( mtdDateInterface::convertDate2Interval( sValueMin,
                                                           &sInterval4 )
                   == IDE_SUCCESS ) )
            {
                // 분모의 Interval 계산
                sInterval1.second      -= sInterval2.second;
                sInterval1.microsecond -= sInterval2.microsecond;
        
                if( sInterval1.microsecond < 0 )
                {
                    sInterval1.second--;
                    sInterval1.microsecond += 1000000;
                }
                if( sInterval1.microsecond >= 1000000 )
                {
                    sInterval1.second++;
                    sInterval1.microsecond -= 1000000;
                }

                // Double Type으로 전환
                sDenominator = (SDouble) sInterval1.second/864e2 +
                    (SDouble) sInterval1.microsecond/864e8;
        
                if ( sDenominator < 0.0 )
                {
                    //------------------------------------------------------
                    // 4. ColumnMin > ColumnMax 검증 (잘못된 통계 정보)
                    //------------------------------------------------------

                    IDE_DASSERT_MSG( sDenominator >= 0.0,
                                     "sDenominator : %"ID_DOUBLE_G_FMT"\n",
                                     sDenominator );
                }
                else if ( sDenominator == 0.0 )
                {
                    //------------------------------------------------------
                    // 5. ColumnMax == ColumnMin 검증 (분모값)
                    //------------------------------------------------------

                    sSelectivity = 1;
                }
                else
                {
                    //------------------------------------------------------
                    // 분자값 (ValueMax - ValueMin) 값 획득
                    //------------------------------------------------------

                    // 분자의 Interval 계산
                    sInterval3.second      -= sInterval4.second;
                    sInterval3.microsecond -= sInterval4.microsecond;
            
                    if( sInterval3.microsecond < 0 )
                    {
                        sInterval3.second--;
                        sInterval3.microsecond += 1000000;
                    }
                    if( sInterval3.microsecond >= 1000000 )
                    {
                        sInterval3.second++;
                        sInterval3.microsecond -= 1000000;
                    }
            
                    // Double Type으로 전환
                    sNumerator =
                        (SDouble) sInterval3.second/864e2 +
                        (SDouble) sInterval3.microsecond/864e8;
            
                    if ( sNumerator <= 0.0 )
                    {
                        //------------------------------------------------------
                        // 6. ValueMax <= ValueMin 검증 (분자값)
                        //------------------------------------------------------

                        sSelectivity = 1 / aTotalRecordCnt;
                    }
                    else
                    {
                        //-----------------------------------------------------
                        // 7. Etc : Min-Max selectivity
                        //-----------------------------------------------------
               
                        sSelectivity = sNumerator / sDenominator;
                        sSelectivity += aBoundFactor;
                        sSelectivity = ( sSelectivity > 1 ) ? 1: sSelectivity;
                    }
                }
            }
            else
            {
                // Date -> Interval이 에러가 난다면
                sSelectivity = MTD_DEFAULT_SELECTIVITY;
            }
        }
    }

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return sSelectivity;
}
    
IDE_RC mtdEncode( mtcColumn  * /* aColumn */,
                  void       * aValue,
                  UInt         /* aValueSize */,
                  UChar      * aCompileFmt,
                  UInt         aCompileFmtLen,
                  UChar      * aText,
                  UInt       * aTextLen,
                  IDE_RC     * aRet )
{
    UInt          sStringLen;

    //----------------------------------
    // Parameter Validation
    //----------------------------------

    IDE_ASSERT( aValue != NULL );
    IDE_ASSERT( aCompileFmt != NULL );
    IDE_ASSERT( aCompileFmtLen > 0 );
    IDE_ASSERT( aText != NULL );
    IDE_ASSERT( *aTextLen > 0 );
    IDE_ASSERT( aRet != NULL );
    
    //----------------------------------
    // Initialization
    //----------------------------------

    aText[0] = '\0';
    sStringLen = 0;
    
    //----------------------------------
    // Set String
    //----------------------------------

    // To Fix BUG-16801
    if ( mtdIsNull( NULL, aValue ) == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        // PROJ-1618
        IDE_TEST( mtdDateInterface::toChar( (mtdDateType*) aValue,
                                            aText,
                                            & sStringLen,
                                            *aTextLen,
                                            aCompileFmt,
                                            aCompileFmtLen )
                  != IDE_SUCCESS );
    }
    
    //----------------------------------
    // Finalization
    //----------------------------------

    aText[sStringLen] = '\0';
    *aTextLen = sStringLen;
    
    *aRet = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    *aRet = IDE_FAILURE;
    
    return IDE_FAILURE;
}


IDE_RC mtdDecode( mtcTemplate * /* aTemplate */,
                  mtcColumn   * /* aColumn */,
                  void        * aValue,
                  UInt        * aValueSize,
                  UChar       * aCompileFmt,
                  UInt          aCompileFmtLen,
                  UChar       * aText,
                  UInt          aTextLen,
                  IDE_RC      * aRet )
{
    mtdDateType* sValue;

    // BUG-40290 SET_COLUMN_STATS min, max 지원
    // char 타입을 date 타입으로 컨버젼 함수
    if( MTD_DATE_SIZE <= *aValueSize )
    {
        sValue = (mtdDateType*)aValue;

        if( aTextLen == 0 )
        {
            *sValue = mtdDateNull;
        }
        else
        {
            idlOS::memset((void*)sValue, 0x00, ID_SIZEOF(mtdDateType));

            IDE_TEST( mtdDateInterface::toDate(
                          sValue,
                          aText,
                          (UShort)aTextLen,
                          aCompileFmt,
                          aCompileFmtLen )
                      != IDE_SUCCESS );
        }

        *aRet = IDE_SUCCESS;
    }
    else
    {
        *aRet = IDE_FAILURE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mtdCompileFmt( mtcColumn  * /* aColumn */,
                      UChar      * aCompiledFmt,
                      UInt       * aCompiledFmtLen,
                      UChar      * aFormatString,
                      UInt         aFormatStringLength,
                      IDE_RC     * aRet )
{
    UInt    sLen;
    UChar * sFPtr = aFormatString;
    UChar * sFFence = aFormatString + aFormatStringLength;
    UChar * sCPtr = aCompiledFmt;
    UChar * sCFence = aCompiledFmt + MTD_COMPILEDFMT_MAXLEN;

    for(;sFPtr < sFFence;)
    {
        sLen = 1;
        switch(idlOS::idlOS_toupper(*sFPtr))
        {
            case 'Y' :
            {
                while(&sFPtr[sLen] < sFFence)
                {
                    if(idlOS::idlOS_toupper(sFPtr[sLen]) != 'Y' ||
                       sLen == 4 )
                    {
                        break;
                    }
                    sLen++;
                }
                if( sLen == 2 || sLen == 3 )
                {
                    IDE_TEST( sCPtr + 4 >= sCFence );
                    idlOS::sprintf((SChar*)sCPtr, "%%2Y");
                    sLen = 2;
                }
                else if( sLen >= 4 )
                {
                    IDE_TEST( sCPtr + 4 >= sCFence );
                    idlOS::sprintf((SChar*)sCPtr, "%%4Y");
                    sLen = 4;
                }
                else
                {
                    IDE_TEST( sCPtr + 2 >= sCFence );
                    *sCPtr = *sFPtr;
                    sCPtr[1] = '\0';
                }
                break;
            }
            case 'M' :
            {
                if( &sFPtr[sLen] < sFFence )
                {
                    if(idlOS::idlOS_toupper(sFPtr[sLen]) == 'M')
                    {
                        IDE_TEST( sCPtr + 4 >= sCFence );
                        idlOS::sprintf((SChar*)sCPtr, "%%2M");
                        sLen = 2;
                    }
                    else if(idlOS::idlOS_toupper(sFPtr[sLen]) == 'O' &&
                            &sFPtr[sLen + 1] < sFFence &&
                            idlOS::idlOS_toupper(sFPtr[sLen + 1]) == 'N')
                    {
                        IDE_TEST( sCPtr + 4 >= sCFence );
                        idlOS::sprintf((SChar*)sCPtr, "%%3M");
                        sLen = 3;
                    }
                    else if(idlOS::idlOS_toupper(sFPtr[sLen]) == 'I')
                    {
                        IDE_TEST( sCPtr + 4 >= sCFence );
                        idlOS::sprintf((SChar*)sCPtr, "%%2m");
                        sLen = 2;
                    }
                    else
                    {
                        IDE_TEST( sCPtr + 2 >= sCFence );
                        *sCPtr = *sFPtr;
                        sCPtr[1] = '\0';
                    }
                }
                else
                {
                    *sCPtr = *sFPtr;
                    sCPtr[1] = '\0';
                }
                break;
            }
            case 'D' :
            {
                if( &sFPtr[sLen] < sFFence  &&
                    idlOS::idlOS_toupper(sFPtr[sLen]) == 'D')
                {
                    IDE_TEST( sCPtr + 4 >= sCFence );
                    idlOS::sprintf((SChar*)sCPtr, "%%2D");
                    sLen = 2;
                }
                else
                {
                    IDE_TEST( sCPtr + 2 >= sCFence );
                    *sCPtr = *sFPtr;
                    sCPtr[1] = '\0';
                }
                break;
            }
            case 'H' :
            {
                if( &sFPtr[sLen] < sFFence  &&
                    idlOS::idlOS_toupper(sFPtr[sLen]) == 'H')
                {
                    IDE_TEST( sCPtr + 4 >= sCFence );
                    idlOS::sprintf((SChar*)sCPtr, "%%2H");
                    sLen = 2;
                }
                else
                {
                    IDE_TEST( sCPtr + 2 >= sCFence );
                    *sCPtr = *sFPtr;
                    sCPtr[1] = '\0';
                }
                break;
            }
            case 'S' :
            {
                while(&sFPtr[sLen] < sFFence)
                {
                    if(idlOS::idlOS_toupper(sFPtr[sLen]) != 'S' ||
                       sLen == 6 )
                    {
                        break;
                    }
                    sLen++;
                }
                if( sLen >= 2 && sLen < 6 )
                {
                    IDE_TEST( sCPtr + 4 >= sCFence );
                    idlOS::sprintf((SChar*)sCPtr, "%%2S");
                    sLen = 2;
                }
                else if( sLen >= 6 )
                {
                    IDE_TEST( sCPtr + 4 >= sCFence );
                    idlOS::sprintf((SChar*)sCPtr, "%%6U");
                    sLen = 6;
                }
                else
                {
                    IDE_TEST( sCPtr + 2 >= sCFence );
                    *sCPtr = *sFPtr;
                    sCPtr[1] = '\0';
                }
                break;
            }
            default :
            {
                IDE_TEST( sCPtr + 2 >= sCFence );
                *sCPtr = *sFPtr;
                sCPtr[1] = '\0';
                break;
            }
        } // switch
        sFPtr += sLen;
        sCPtr += idlOS::strlen((SChar*)sCPtr);
    } // for

    *aCompiledFmtLen = sCPtr - aCompiledFmt;
    *aRet = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aCompiledFmtLen = 0;
    *aRet = IDE_FAILURE;

    return IDE_SUCCESS;
}

IDE_RC mtdValueFromOracle( mtcColumn*  aColumn,
                           void*       aValue,
                           UInt*       aValueOffset,
                           UInt        aValueSize,
                           const void* aOracleValue,
                           SInt        aOracleLength,
                           IDE_RC*     aResult )
{
    UInt         sValueOffset;
    mtdDateType* sValue;
    const UChar* sOracleValue;
    
    sValueOffset = idlOS::align( *aValueOffset, MTD_DATE_ALIGN );
    
    if( sValueOffset + MTD_DATE_SIZE <= aValueSize )
    {
        sValue = (mtdDateType*)((UChar*)aValue+sValueOffset);
        aColumn->column.offset = sValueOffset;
            
        if( aOracleLength >= 0 )
        {
            IDE_TEST_RAISE( aOracleLength != 7, ERR_INVALID_LENGTH );
            
            sOracleValue = (const UChar*)aOracleValue;

            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                       ( (UShort)sOracleValue[0] - 100 ) * 100
                                       + (UShort)sOracleValue[1] - 100,
                                       sOracleValue[2],
                                       sOracleValue[3],
                                       sOracleValue[4] - 1,
                                       sOracleValue[5] - 1,
                                       sOracleValue[6] - 1,
                                       0 )
                      != IDE_SUCCESS );
        }
        else
        {
            *sValue = mtdDateNull;
        }
        
        IDE_TEST( mtdValidate( aColumn, sValue, MTD_DATE_SIZE )
                  != IDE_SUCCESS );
    
        *aValueOffset = sValueOffset + aColumn->column.size;
        
        *aResult = IDE_SUCCESS;
    }
    else
    {
        *aResult = IDE_FAILURE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_INVALID_LENGTH);
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;

    *aResult = IDE_FAILURE;
    
    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::setYear(mtdDateType* aDate, SShort aYear)
{
    IDE_ASSERT( aDate != NULL );

    aDate->year = aYear;

    return IDE_SUCCESS;
}

IDE_RC mtdDateInterface::setMonth(mtdDateType* aDate, UChar aMonth)
{
    UShort sMonth = (UShort)aMonth;

    IDE_ASSERT( aDate != NULL );

    IDE_TEST_RAISE( ( sMonth < 1 ) || ( sMonth > 12 ),
                    ERR_INVALID_MONTH );

    aDate->mon_day_hour &= ~MTD_DATE_MON_MASK;
    aDate->mon_day_hour |= sMonth << MTD_DATE_MON_SHIFT;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_MONTH);
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_MONTH));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::setDay(mtdDateType* aDate, UChar aDay)
{
    UShort sDay = (UShort)aDay;

    IDE_ASSERT( aDate != NULL );

    IDE_TEST_RAISE( ( sDay < 1 ) || ( sDay > 31 ),
                    ERR_INVALID_DAY );

    aDate->mon_day_hour &= ~MTD_DATE_DAY_MASK;
    aDate->mon_day_hour |= sDay << MTD_DATE_DAY_SHIFT;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DAY);
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_DAY));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::setHour(mtdDateType* aDate, UChar aHour)
{
    UShort sHour = (UShort)aHour;

    IDE_ASSERT( aDate != NULL );

    // UChar이므로 lower bound는 검사하지 않음
    IDE_TEST_RAISE( sHour > 23,
                    ERR_INVALID_HOUR24 );

    aDate->mon_day_hour &= ~MTD_DATE_HOUR_MASK;
    aDate->mon_day_hour |= sHour;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_HOUR24);
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DATE_INVALID_HOUR24));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::setMinute(mtdDateType* aDate, UChar aMinute)
{
    UInt sMinute = (UInt)aMinute;

    IDE_ASSERT( aDate != NULL );

    // UChar이므로 lower bound는 검사하지 않음
    IDE_TEST_RAISE( sMinute > 59,
                    ERR_INVALID_MINUTE );

    aDate->min_sec_mic &= ~MTD_DATE_MIN_MASK;
    aDate->min_sec_mic |= sMinute << MTD_DATE_MIN_SHIFT;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_MINUTE);
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_MINUTE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::setSecond(mtdDateType* aDate, UChar aSec)
{
    UInt sSec = (UInt)aSec;

    IDE_ASSERT( aDate != NULL );

    // UChar이므로 lower bound는 검사하지 않음
    IDE_TEST_RAISE( sSec > 59,
                    ERR_INVALID_SECOND );

    aDate->min_sec_mic &= ~MTD_DATE_SEC_MASK;
    aDate->min_sec_mic |= sSec << MTD_DATE_SEC_SHIFT;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SECOND);
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_SECOND));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::setMicroSecond(mtdDateType* aDate, UInt aMicroSec)
{
    IDE_ASSERT( aDate != NULL );

    // UInt이므로 lower bound는 검사하지 않음
    IDE_TEST_RAISE( aMicroSec > 999999,
                    ERR_INVALID_MICROSECOND );

    aDate->min_sec_mic &= ~MTD_DATE_MSEC_MASK;
    aDate->min_sec_mic |= aMicroSec;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_MICROSECOND);
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_MICROSECOND));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::addMonth( mtdDateType* aResult, mtdDateType* aDate, SLong aNumber )
{
    UChar sStartLastDays[13] = { 0, 31, 28, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31 };

    UChar sEndLastDays[13] = { 0, 31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31 };

    SLong  sStartYear;
    SLong  sStartMonth;
    SLong  sStartDay;
    SLong  sStartHour;
    SLong  sStartMinute;
    SLong  sStartSecond;
    SLong  sStartMicroSecond;

    SLong  sEndYear;
    SLong  sEndMonth;
    SLong  sEndDay;
    SLong  sEndHour;
    SLong  sEndMinute;
    SLong  sEndSecond;
    SLong  sEndMicroSecond;

    sStartYear = mtdDateInterface::year(aDate);
    sStartMonth = mtdDateInterface::month(aDate);
    sStartDay = mtdDateInterface::day(aDate);
    sStartHour = mtdDateInterface::hour(aDate);
    sStartMinute = mtdDateInterface::minute(aDate);
    sStartSecond = mtdDateInterface::second(aDate);
    sStartMicroSecond = mtdDateInterface::microSecond(aDate);

    sEndYear = sStartYear;
    sEndMonth = sStartMonth;
    sEndDay = sStartDay;
    sEndHour = sStartHour;
    sEndMinute = sStartMinute;
    sEndSecond = sStartSecond;
    sEndMicroSecond = sStartMicroSecond;

    if ( isLeapYear( sStartYear ) == ID_TRUE )
    {
        sStartLastDays[2] = 29;
    }
    else
    {
        /* Nothing to do */
    }

    sEndYear = sStartYear + ( aNumber / 12 );
    sEndMonth = sStartMonth + ( aNumber % 12 );

    if ( sEndMonth < 1 )
    {
        sEndYear--;
        sEndMonth += 12;
    }
    else if ( sEndMonth > 12 )
    {
        sEndYear++;
        sEndMonth -= 12;
    }
    else
    {
        // nothing to do
    }

    if ( isLeapYear( sEndYear ) == ID_TRUE )
    {
        sEndLastDays[2] = 29;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sStartDay == sStartLastDays[sStartMonth] ||
         sStartDay >= sEndLastDays[sEndMonth] )
    {
        sEndDay = sEndLastDays[sEndMonth];
    }
    else
    {
        /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
        if ( ( sEndYear == 1582 ) &&
             ( sEndMonth == 10 ) &&
             ( 4 < sStartDay ) && ( sStartDay < 15 ) )
        {
            sEndDay = 15;
        }
        else
        {
            sEndDay = sStartDay;
        }
    }

    if ( sEndYear < -9999 || sEndYear > 9999 )
    {
        IDE_RAISE ( ERR_INVALID_YEAR );
    }

    IDE_TEST( mtdDateInterface::makeDate(aResult,
                                         sEndYear,
                                         sEndMonth,
                                         sEndDay,
                                         sEndHour,
                                         sEndMinute,
                                         sEndSecond,
                                         sEndMicroSecond )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::addDay( mtdDateType* aResult, mtdDateType* aDate, SLong aNumber )
{
    mtdIntervalType sInterval = { ID_LONG(0), ID_LONG(0) };

    /* BUG-36296 Date를 Interval로 변환하여 작업하고, 다시 Date로 변환한다. */
    IDE_TEST( mtdDateInterface::convertDate2Interval( aDate,
                                                      &sInterval )
              != IDE_SUCCESS );

    sInterval.second      += (aNumber * ID_LONG(86400));

    sInterval.microsecond += (sInterval.second * 1000000);

    sInterval.second      = sInterval.microsecond / 1000000;
    sInterval.microsecond = sInterval.microsecond % 1000000;

    IDE_TEST_RAISE( ( sInterval.second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                    ( sInterval.second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                    ( ( sInterval.second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                      ( sInterval.microsecond < 0 ) ),
                    ERR_INVALID_YEAR );

    IDE_TEST( mtdDateInterface::convertInterval2Date( &sInterval,
                                                      aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::addSecond( mtdDateType* aResult,
                                    mtdDateType* aDate,
                                    SLong        aSecond,
                                    SLong        aMicroSecond )
{
    mtdIntervalType sInterval = { ID_LONG(0), ID_LONG(0) };

    /* BUG-36296 Date를 Interval로 변환하여 작업하고, 다시 Date로 변환한다. */
    IDE_TEST( mtdDateInterface::convertDate2Interval( aDate,
                                                      &sInterval )
              != IDE_SUCCESS );

    sInterval.second      += aSecond;
    sInterval.microsecond += aMicroSecond;

    sInterval.microsecond += (sInterval.second * 1000000);

    sInterval.second      = sInterval.microsecond / 1000000;
    sInterval.microsecond = sInterval.microsecond % 1000000;

    IDE_TEST_RAISE( ( sInterval.second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                    ( sInterval.second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                    ( ( sInterval.second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                      ( sInterval.microsecond < 0 ) ),
                    ERR_INVALID_YEAR );

    IDE_TEST( mtdDateInterface::convertInterval2Date( &sInterval,
                                                      aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::addMicroSecond( mtdDateType* aResult, mtdDateType* aDate, SLong aNumber )
{
    mtdIntervalType sInterval = { ID_LONG(0), ID_LONG(0) };

    /* BUG-36296 Date를 Interval로 변환하여 작업하고, 다시 Date로 변환한다. */
    IDE_TEST( mtdDateInterface::convertDate2Interval( aDate,
                                                      &sInterval )
              != IDE_SUCCESS );

    sInterval.microsecond += aNumber;

    sInterval.microsecond += (sInterval.second * 1000000);

    sInterval.second      = sInterval.microsecond / 1000000;
    sInterval.microsecond = sInterval.microsecond % 1000000;

    IDE_TEST_RAISE( ( sInterval.second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                    ( sInterval.second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                    ( ( sInterval.second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                      ( sInterval.microsecond < 0 ) ),
                    ERR_INVALID_YEAR );

    IDE_TEST( mtdDateInterface::convertInterval2Date( &sInterval,
                                                      aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::dateDiff( mtdBigintType * aResult,
                                   mtdDateType   * aStartDate,
                                   mtdDateType   * aEndDate,
                                   mtdDateField    aExtractSet )
{
/***********************************************************************
 *
 * Description : Datediff Calculate
 *
 * Implementation : 
 *    ex) DATEDIFF ( '28-DEC-1980', '21-OCT-2005', 'DAY') ==> 9064
 *
 ***********************************************************************/

    SInt          sExtractSet       = 0;
    SInt          sStartYear        = 0;
    SInt          sStartMonth       = 0;
    SInt          sStartDay         = 0;
    SInt          sStartHour        = 0;
    SInt          sStartMinute      = 0;
    SInt          sStartSecond      = 0;
    SInt          sStartMicroSecond = 0;

    SInt          sEndYear        = 0;
    SInt          sEndMonth       = 0;
    SInt          sEndDay         = 0;
    SInt          sEndHour        = 0;
    SInt          sEndMinute      = 0;
    SInt          sEndSecond      = 0;
    SInt          sEndMicroSecond = 0;

    SLong         sDiffYear        = 0;
    SLong         sDiffMonth       = 0;
    SLong         sDiffWeek        = 0;
    SLong         sDiffDay         = 0;
    SLong         sDiffHour        = 0;
    SLong         sDiffMinute      = 0;
    SLong         sDiffSecond      = 0;
    SLong         sDiffMicroSecond = 0;
    
    mtdDateType * sStartDate;
    mtdDateType * sEndDate;
    SInt          i = 0;
    
    sStartDate  = aStartDate;
    sEndDate    = aEndDate;
    sExtractSet = aExtractSet;

    sStartYear        = mtdDateInterface::year(sStartDate);
    sStartMonth       = mtdDateInterface::month(sStartDate);
    sStartDay         = mtdDateInterface::day(sStartDate);
    sStartHour        = mtdDateInterface::hour(sStartDate);
    sStartMinute      = mtdDateInterface::minute(sStartDate);
    sStartSecond      = mtdDateInterface::second(sStartDate);
    sStartMicroSecond = mtdDateInterface::microSecond(sStartDate);

    sEndYear        = mtdDateInterface::year(sEndDate);
    sEndMonth       = mtdDateInterface::month(sEndDate);
    sEndDay         = mtdDateInterface::day(sEndDate);
    sEndHour        = mtdDateInterface::hour(sEndDate);
    sEndMinute      = mtdDateInterface::minute(sEndDate);
    sEndSecond      = mtdDateInterface::second(sEndDate);
    sEndMicroSecond = mtdDateInterface::microSecond(sEndDate);

	// 모든 fmt에 대해서 enddate - startdate 를 구한다.
    sDiffYear  = sEndYear - sStartYear;
    sDiffMonth = sEndMonth - sStartMonth + ( 12 * sDiffYear );

    if ( sStartYear == sEndYear )
    {
        sDiffWeek = mtc::weekOfYear( sEndYear, sEndMonth, sEndDay ) -
            mtc::weekOfYear( sStartYear, sStartMonth, sStartDay );

        sDiffDay = mtc::dayOfYear( sEndYear, sEndMonth, sEndDay ) -
            mtc::dayOfYear( sStartYear, sStartMonth, sStartDay );
    }
    else
    {
        sDiffWeek = mtc::weekOfYear( sEndYear, sEndMonth, sEndDay ) -
            mtc::weekOfYear( sStartYear, sStartMonth, sStartDay );

        sDiffDay = mtc::dayOfYear( sEndYear, sEndMonth, sEndDay ) -
            mtc::dayOfYear( sStartYear, sStartMonth, sStartDay );

        if ( sStartYear > sEndYear )
        { 
            for (i = sEndYear; i < sStartYear; i++)
            {
                sDiffDay -= mtc::dayOfYear( i, 12, 31 );
                sDiffWeek -= mtc::weekOfYear( i, 12, 31 );

                /* 1월1일이 일요일이 아니면 주가 바뀌지 않는다.
                 * BUG-36052 EndYear 가 작은 경우는 EndYear 다음 해의 1월 1일을
                 * 체크 해야한다.
                 */
                if ( mtc::dayOfWeek( i + 1, 1, 1 ) != 0 )
                {
                    sDiffWeek++;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            for (i = sStartYear; i < sEndYear; i++)
            {
                sDiffDay += mtc::dayOfYear ( i, 12, 31 );
                sDiffWeek += mtc::weekOfYear ( i, 12, 31 );

                // 12월 31일이 토요일이 아니면 주가 바뀌지 않는다.
                if ( mtc::dayOfWeek( i, 12, 31 ) != 6 ) 
                {
                    sDiffWeek--;
                }
            }
        }
    }

    /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
    if ( ( ( sStartYear < 1582 ) ||
           ( ( sStartYear == 1582 ) && ( ( sStartMonth < 10 ) ||
                                         ( ( sStartMonth == 10 ) && ( sStartDay <= 4 ) ) ) ) ) &&
         ( ( sEndYear > 1582 ) ||
           ( ( sEndYear == 1582 ) && ( ( sEndMonth > 10 ) ||
                                       ( ( sEndMonth == 10 ) && ( sEndDay >= 15 ) ) ) ) ) )
    {
        sDiffDay -= 10;
    }
    else
    {
        if ( ( ( sEndYear < 1582 ) ||
               ( ( sEndYear == 1582 ) && ( ( sEndMonth < 10 ) ||
                                           ( ( sEndMonth == 10 ) && ( sEndDay <= 4 ) ) ) ) ) &&
             ( ( sStartYear > 1582 ) ||
               ( ( sStartYear == 1582 ) && ( ( sStartMonth > 10 ) ||
                                             ( ( sStartMonth == 10 ) && ( sStartDay >= 15 ) ) ) ) ) )
        {
            sDiffDay += 10;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sDiffHour = ( sEndHour - sStartHour ) + ( sDiffDay * 24 );
    sDiffMinute = ( sEndMinute - sStartMinute ) + ( sDiffHour * 60 );
    sDiffSecond = ( sEndSecond - sStartSecond ) + ( sDiffMinute * 60 );
    sDiffMicroSecond = ( sEndMicroSecond - sStartMicroSecond ) +
        ( sDiffSecond * 1000000 );

    if( sExtractSet == MTD_DATE_DIFF_CENTURY )
    {
        if ( sEndYear <= 0 )
        {
            if ( sStartYear <= 0 )
            {
                *aResult = (SLong)( ( sEndYear / 100 ) - ( sStartYear / 100 ) );
            }
            else
            {
                *aResult = (SLong)( ( sEndYear / 100 ) - ( ( sStartYear + 99 ) / 100 ) );
            }
        }
        else
        {
            if ( sStartYear <= 0 )
            {
                *aResult = (SLong)( ( ( sEndYear + 99 ) / 100 ) - ( sStartYear / 100 ) );
            }
            else
            {
                *aResult = (SLong)( ( ( sEndYear + 99 ) / 100 ) - ( ( sStartYear + 99 ) / 100 ) );
            }
        }
    }
    else if( sExtractSet == MTD_DATE_DIFF_YEAR )
    {
        *aResult = sDiffYear;
    }
    else if( sExtractSet == MTD_DATE_DIFF_QUARTER )
    {
        *aResult = (SLong) ( ( ( sEndMonth + 2 ) / 3 ) - ( ( sStartMonth + 2 ) / 3 )
                 + ( sDiffYear * 4 ) );
    }
    else if( sExtractSet == MTD_DATE_DIFF_MONTH )
    {
        *aResult = sDiffMonth;
    }
    else if( sExtractSet == MTD_DATE_DIFF_WEEK )
    {
        *aResult = sDiffWeek;
    }
    else if( sExtractSet == MTD_DATE_DIFF_DAY )
    {
        *aResult = sDiffDay;
    }
    else if( sExtractSet == MTD_DATE_DIFF_HOUR )
    {
        *aResult = sDiffHour;
    }
    else if( sExtractSet == MTD_DATE_DIFF_MINUTE )
    {
        *aResult = sDiffMinute;
    }
    else if( sExtractSet == MTD_DATE_DIFF_SECOND )
    {
        // 68 year = 2185574400 sec
        IDE_TEST_RAISE ( ( sDiffSecond > ID_LONG(2185574400) ) ||
                         ( sDiffSecond < ID_LONG(-2185574400) ),
                         ERR_DATEDIFF_OUT_OF_RANGE_IN_SECOND );

        *aResult = sDiffSecond;
    }
    else if( sExtractSet == MTD_DATE_DIFF_MICROSEC )
    {
        // 30day = 2592000000000 microsecond
        IDE_TEST_RAISE ( ( sDiffMicroSecond > ID_LONG(2592000000000) ) ||
                         ( sDiffMicroSecond < ID_LONG(-2592000000000) ),
                          ERR_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND );

         *aResult = sDiffMicroSecond;
    }
    else
    {
        IDE_RAISE(ERR_INVALID_LITERAL);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION( ERR_DATEDIFF_OUT_OF_RANGE_IN_SECOND );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_SECOND));

    IDE_EXCEPTION( ERR_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
mtdDateInterface::makeDate( mtdDateType* aDate,
                            SShort       aYear,
                            UChar        aMonth,
                            UChar        aDay,
                            UChar        aHour,
                            UChar        aMinute,
                            UChar        aSec,
                            UInt         aMicroSec )
{
    // Bug-10600 UMR error
    // aDate의 각 맴버를 초기화 해야 한다.
    // by kumdory, 2005-03-04
    aDate->mon_day_hour = 0;
    aDate->min_sec_mic = 0;

    // BUG-31389
    // 날짜의 유효성을 체크 하기 위해선 년/월/일이 필요 하다.
    // 이에 반해 시/분/초등은 단독으로 검사 가능하다.
    
    IDE_TEST( checkYearMonthDayAndSetDateValue( aDate,
                                                aYear,
                                                aMonth,
                                                aDay )
              != IDE_SUCCESS );

    IDE_TEST( setHour( aDate, aHour ) != IDE_SUCCESS );
    IDE_TEST( setMinute( aDate, aMinute ) != IDE_SUCCESS );
    IDE_TEST( setSecond( aDate, aSec ) != IDE_SUCCESS );
    IDE_TEST( setMicroSecond( aDate, aMicroSec ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::convertDate2Interval( mtdDateType*     aDate,
                                               mtdIntervalType* aInterval )
/***********************************************************************
 *
 * Description :
 *    Date 연산을 위해 Date -> Interval로 변환하는 함수
 *
 * Implementation :
 *    1) 해당 일 전날까지 총 일수를 구하여 하루당 초수(86400초)로 곱하여 구한다.
 *    2) 시간단위는 각각 단위당 초수로 곱하여 구한다.
 *    3) 위에서 구한것들을 전부 더해준다.
 *    4) 그레고리력만 사용한다는 가정하에 계산한다.
 ***********************************************************************/
{

    IDE_TEST_RAISE( MTD_DATE_IS_NULL(aDate) == ID_TRUE, ERR_INVALID_NULL );

    aInterval->second = 
        (SLong)mtc::dayOfCommonEra( year(aDate), month(aDate), day(aDate) ) * ID_LONG(86400);
    aInterval->second +=
          hour(aDate)   * 3600
        + minute(aDate) * 60
        + second(aDate);
    aInterval->microsecond = microSecond(aDate);

    /* DATE의 MicroSecond는 항상 0 이상이다. Second가 음수이면, MicroSecond도 음수로 만든다. */
    if ( ( aInterval->second < 0 ) && ( aInterval->microsecond > 0 ) )
    {
        aInterval->second++;
        aInterval->microsecond -= 1000000;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST_RAISE( ( aInterval->second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                    ( aInterval->second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                    ( ( aInterval->second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                      ( aInterval->microsecond < 0 ) ),
                    ERR_INVALID_YEAR );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_NULL );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE) );

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::convertInterval2Date( mtdIntervalType* aInterval,
                                               mtdDateType*     aDate )
/***********************************************************************
 *
 * Description :
 *    Date 연산 후 Date로 다시 변환을 위해 Interval -> Date로 변환하는 함수
 *
 * Implementation :
 *    1) BC인지 AD인지 구분한다.
 *      - Interval이 음수이면, BC이다.
 *      - Interval이 0이면, AD 0001년 1월 1일 0시 0초이다.
 *
 *    2) 1582년 10월 4일 이전은 율리우스력을 적용하고, 1582년 10월 15일 이후는 그레고리력을 적용한다.
 *      - 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다.
 ***********************************************************************/
{
#define DAY_PER_SECOND      86400
#define YEAR400_PER_DAY     146097
#define YEAR100_PER_DAY     36524
#define YEAR4_PER_DAY       1461

/* '1582-10-15' - '0001-01-01' */
#define FIRST_GREGORIAN_DAY ( 577737 )

    SShort sYear        = 0;
    UChar  sMonth       = 0;
    UChar  sDay         = 0;
    UChar  sHour        = 0;
    UChar  sMinute      = 0;
    UChar  sSecond      = 0;
    SInt   sMicroSecond = 0;

    SInt   sDays        = 0;
    SInt   sSeconds     = 0;
    SInt   sLeap        = 0;
    SInt   s400Years    = 0;
    SInt   s100Years    = 0;
    SInt   s4Years      = 0;
    SInt   s1Years      = 0;
    SInt   sStartDayOfMonth[2][13] =
    {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };

    IDE_TEST_RAISE( ( aInterval->second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                    ( aInterval->second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                    ( ( aInterval->second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                      ( aInterval->microsecond < 0 ) ),
                    ERR_INVALID_YEAR );

    // 일단위와 초단위로 나눠준다.
    sDays    = aInterval->second / DAY_PER_SECOND;
    sSeconds = aInterval->second % DAY_PER_SECOND;

    sMicroSecond = (SInt)aInterval->microsecond;

    /* BC by Abel.Chun */
    if ( ( sDays < 0 ) || ( sSeconds < 0 ) || ( sMicroSecond < 0 ) )
    {
        IDE_DASSERT( sDays <= 0 );
        IDE_DASSERT( sSeconds <= 0 );
        IDE_DASSERT( sMicroSecond <= 0 );

        /* DATE에서 연도만 음수가 가능하다. Second와 Microsecond를 양수로 보정한다. */
        if ( sMicroSecond < 0 )
        {
            sSeconds--;
            sMicroSecond += 1000000;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sSeconds < 0 )
        {
            sDays--;
            sSeconds += DAY_PER_SECOND;
        }
        else
        {
            /* Nothing to do */
        }

        /* BUG-36296 기원전에는 4년마다 윤년이라고 가정한다.
         *  - 고대 로마의 정치가 율리우스 카이사르가 기원전 45년부터 율리우스력을 시행하였다.
         *  - 초기의 율리우스력(기원전 45년 ~ 기원전 8년)에서는 윤년을 3년에 한 번 실시하였다. (Oracle 11g 미적용)
         *  - BC 4713년부터 율리우스일을 계산한다. 천문학자들이 율리우스일을 사용한다. 4년마다 윤년이다.
         *
         *  AD 0년은 존재하지 않는다. DATE의 연도가 0이면, BC 1년이다. 즉, BC 1년의 다음은 AD 1년이다.
         *  BC 0001년 12월 31일은 Day -1 이다. 그리고, BC 1년(aYear == 0)이 윤년이다.
         */
        s4Years = sDays / YEAR4_PER_DAY;
        sDays   = sDays % YEAR4_PER_DAY;
        if ( ( s4Years < 0 ) && ( sDays == 0 ) )
        {
            s4Years++;
            sDays = -YEAR4_PER_DAY;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sDays < -1096 )
        {
            s1Years = -3;

            /* DATE에서 연도만 음수가 가능하다. Days를 양수로 보정한다. */
            sDays = sDays + 1461;
        }
        else if ( sDays < -731 ) // && sDays >= -1096
        {
            s1Years = -2;

            /* DATE에서 연도만 음수가 가능하다. Days를 양수로 보정한다. */
            sDays = sDays + 1096;
        }
        else if ( sDays < -366 ) // && sDays >= -731
        {
            s1Years = -1;

            /* DATE에서 연도만 음수가 가능하다. Days를 양수로 보정한다. */
            sDays = sDays + 731;
        }
        else                     // sDays >= -366
        {
            s1Years = 0;

            /* DATE에서 연도만 음수가 가능하다. Days를 양수로 보정한다. */
            sDays = sDays + 366;
        }

        sYear = s4Years * 4
              + s1Years;
    }
    else
    {
        IDE_DASSERT( sDays >= 0 );
        IDE_DASSERT( sSeconds >= 0 );
        IDE_DASSERT( sMicroSecond >= 0 );

        if ( sDays < FIRST_GREGORIAN_DAY )
        {
            /* BUG-36296 그레고리력의 윤년 규칙은 1583년부터 적용한다. 1582년 이전에는 4년마다 윤년이다. */
            s4Years = sDays / YEAR4_PER_DAY;
            sDays   = sDays % YEAR4_PER_DAY;
        }
        /* BUG-36296 그레고리력은 1582년 10월 15일부터 시작한다. */
        else
        {
            /* 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
            sDays += 10;

            /* BUG-36296 1600년 이전은 율리우스력과 그레고리력의 윤년이 같다. */
            if ( sDays <= ( 400 * YEAR4_PER_DAY ) )
            {
                s4Years = sDays / YEAR4_PER_DAY;
                sDays   = sDays % YEAR4_PER_DAY;
            }
            else
            {
                /* BUG-36296 그레고리력의 윤년 규칙은 1583년부터 적용한다. 1582년 이전에는 4년마다 윤년이다. */
                s4Years = 400;
                sDays  -= ( 400 * YEAR4_PER_DAY );

                s400Years = sDays / YEAR400_PER_DAY;
                sDays     = sDays % YEAR400_PER_DAY;

                s100Years = sDays / YEAR100_PER_DAY;
                sDays     = sDays % YEAR100_PER_DAY;
                if ( s100Years == 4 )
                {
                    /* 400년 주기 > (100년 주기 * 4)이므로, 100년 주기 횟수를 조정한다. */
                    s100Years = 3;
                    sDays    += YEAR100_PER_DAY;
                }
                else
                {
                    /* Nothing to do */
                }

                s4Years += sDays / YEAR4_PER_DAY;
                sDays    = sDays % YEAR4_PER_DAY;
            }
        }

        if ( sDays >= 1095 )
        {
            s1Years = 3;
            sDays  -= 1095;
        }
        else if ( sDays >= 730 ) // && sDays < 1095
        {
            s1Years = 2;
            sDays  -= 730;
        }
        else if ( sDays >= 365 ) // && sDays < 730
        {
            s1Years = 1;
            sDays  -= 365;
        }
        else                    // sDays < 365
        {
            s1Years = 0;
        }

        sYear = s400Years * 400
              + s100Years * 100
              + s4Years   * 4
              + s1Years
              + 1; /* AD 0001년부터 시작한다. */
    }

    sDays++;    /* 0일은 존재하지 않으므로, +1로 보정한다. */

    // 달, 일 구하기
    if ( isLeapYear( sYear ) == ID_TRUE )
    {
        sLeap = 1;
    }
    else
    {
        sLeap = 0;
    }

    for ( sMonth = 1; sMonth < 13; sMonth++)
    {
        if ( sStartDayOfMonth[sLeap][(UInt)sMonth] >= sDays )
        {
            break;
        }
    }

    sDay = sDays - sStartDayOfMonth[sLeap][sMonth - 1];
    
    // 시간 구하기
    sHour = sSeconds / 3600;
    sSeconds %= 3600;
    sMinute = sSeconds / 60;
    sSecond = sSeconds % 60;

    IDE_TEST( mtdDateInterface::makeDate(aDate,
                                         sYear,
                                         sMonth,
                                         sDay,
                                         sHour,
                                         sMinute,
                                         sSecond,
                                         (UInt)sMicroSecond )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
mtdDateInterface::toDateGetInteger( UChar** aString,
                                    UInt*   aLength,
                                    UInt    aMax,
                                    UInt*   aValue )
{ 
/***********************************************************************
 *
 * Description :
 *    string->number로 변환(format length보다 짧아도 허용)
 *    YYYY, RRRR, DD, MM 등 가변길이 허용하는 format을 위해 사용
 *
 * Implementation :
 *    1) 숫자로 된 문자를 만나면 UInt value로 변환.
 *       숫자로 변환된 부분 만큼 문자열 포인터 이동 및 길이 조정
 *    2) 숫자가 아닌 문자를 만나면 break
 *    3) 만약 한 문자도 변환이 되지 않았다면 error
 ***********************************************************************/
    UChar* sInitStr;

    IDE_ASSERT( aString != NULL );
    IDE_ASSERT( aLength != NULL );
    IDE_ASSERT( aValue  != NULL );

    IDE_TEST_RAISE( *aLength < 1,
                    ERR_NOT_ENOUGH_INPUT );

    sInitStr = *aString;
    
    // 사용자가 예상한 최대 값과 현재 문자열의 길이와 비교하여 작은
    // 것을 저장함
    aMax = ( *aLength < aMax ) ? *aLength : aMax;
    
    // 숫자로 된 문자를 만나면 UInt value로 변환.
    for( *aValue = 0;
         aMax    > 0;
         aMax--, (*aString)++, (*aLength)-- )
    {
        if( mInputST[**aString] == mDIGIT  )
        {
            *aValue = *aValue * 10 + **aString - '0';
        }
        else
        {
            // 숫자가 아닌 문자를 만나면 break
            break;
        }
    }

    // 만약 한 문자도 변환이 되지 않았다면 error
    IDE_TEST_RAISE( *aString == sInitStr,
                    ERR_NON_NUMERIC_INPUT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_INPUT );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );

    IDE_EXCEPTION( ERR_NON_NUMERIC_INPUT );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_NON_NUMERIC_INPUT) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mtdDateInterface::toDateGetMonth( UChar** aString,
                                  UInt*   aLength,
                                  UInt*   aMonth )
{ 
/***********************************************************************
 * Description : '월'에 해당하는 문자열을 읽고, aMonth에 적절한 값을 리턴(1~12)
 *                의미 있는 '월'을 의미하는 문자열인 경우 그 길이 만큼
 *                aString과 aLength를 조정
 * Implementation : '월'을 의미하는 각 문자열에 대해 앞쪽 3자를 미리 해시하여
 *                  그 값을 미리 배열에 저장하여 해시값을 비교한 뒤, 문자열 비교
 *                  구현 용이를 위해 해시 테이블을 구성하지 않고, 모든 경우를 비교
 ***********************************************************************/
    SInt       sIdx;
    SInt       sMax;
    UInt       sHashVal;
    UChar      sString[10] = {0};
    SInt       sLength;
    UInt       sMonthLength[12] = { 7, 8, 5, 5, 3, 4,   // 해당 '월'을 의미하는
                                    4, 6, 9, 7, 8, 8 }; // 문자열의 길이

    IDE_ASSERT( aString != NULL );
    IDE_ASSERT( aLength != NULL );
    IDE_ASSERT( aMonth  != NULL );

    // 월을 의미하는 문자열을 최소 3자 이상이어야 함
    IDE_TEST_RAISE( *aLength < 3,
                    ERR_NOT_ENOUGH_INPUT );


    // 월을 의미하는 문자열 중 가장 긴 것이 9자 = september
    // 현재 문자열의 길이와 비교하여 작은 것을 저장함
    sMax = IDL_MIN(*aLength, 9);
        
    // 문자열의 최대 길이를 계산
    for( sLength = 0;
         sLength < sMax;
         sLength++)
    {
        if( mInputST[(*aString)[sLength]] != mALPHA  )
        {
            break;
        }
    }

    // 문자열을 복사하고, 모두 대문자로 변환
    idlOS::memcpy( sString, *aString, sLength );
    idlOS::strUpper( sString, sLength );

    // 앞자리 3자리만을 이용하여 해시 값 생성
    sHashVal = mtc::hash(mtc::hashInitialValue,
                         sString,
                         3);

    for( sIdx=0; sIdx<12; sIdx++ )
    {
        if( sHashVal == mHashMonth[sIdx] )
        {
            if( idlOS::strMatch( sString,
                                 3,
                                 gMONName[sIdx],
                                 3 ) == 0 )
            {
                // 일단 앞 3자가 일치하므로 해당 달로 인식할 수 있음
                // 하지만, 약자가 아닌 '월'이름으로 입력할 수 있으므로
                // 가능하면, 모든 입력 문자를 소비해야 함
                if( sLength >= (SInt) sMonthLength[sIdx] )
                {
                    if( idlOS::strMatch( sString,
                                         sMonthLength[sIdx],
                                         gMONTHName[sIdx],
                                         sMonthLength[sIdx] ) == 0 )
                    {
                        // 줄이지 않은 원래 이름으로 매칭이 성공한 경우
                        sLength = sMonthLength[sIdx];
                    }
                    else
                    {
                        // 줄여쓴 이름으로 매칭이 성공한 경우
                        sLength = 3;
                    }
                }
                else
                {
                    // '월'원래 월의 길이이거나 3이어야 한다.
                    // 예를 들어 to_date( 'SEPMON','MONDAY' )과 같은
                    // 입력이 있을 수 있음
                    sLength = 3;
                }

                // 매칭이 성공했으므로 월을 설정하고 반복문 종료
                *aMonth = sIdx + 1;
                *aString += sLength;
                *aLength -= sLength;
                break;
            }
            else
            {
                // 다음 '월'에 대한 값 비교
            }
        }
    }

    // 모든 경우를 검사하도고 매칭되는 문자열을 찾지 못하면 에러
    IDE_TEST_RAISE( sIdx == 12, ERR_INVALID_DATE );                    

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_INPUT );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );

    IDE_EXCEPTION(ERR_INVALID_DATE);
    IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mtdDateInterface::toDateGetRMMonth( UChar** aString,
                                    UInt*   aLength,
                                    UInt*   aMonth )
{ 
/***********************************************************************
 * Description : '월'에 해당하는 로마자 문자열을 읽고, aMonth에 적절한
 *                값을 리턴(1~12) 의미 있는 '월'을 의미하는 문자열인
 *                경우 그 길이 만큼 aString과 aLength를 조정
 * Implementation : 나올 수 있는 문자의 경우가 3개이므로 반복문으로 계산
 ***********************************************************************/
    SInt       sIdx       = 0;
    SInt       sMax;
    UChar      sString[5] = {0};
    SInt       sLength;

    idBool     sIsMinus   = ID_FALSE;
    idBool     sIsEnd     = ID_FALSE;
    SShort     sMonth     = 0;
    SShort     sICnt      = 0;
    SShort     sVCnt      = 0;
    SShort     sXCnt      = 0;

    IDE_ASSERT( aString != NULL );
    IDE_ASSERT( aLength != NULL );
    IDE_ASSERT( aMonth  != NULL );

    // 월을 의미하는 문자열을 최소 1자 이상이어야 함
    IDE_TEST_RAISE( *aLength < 1,
                    ERR_NOT_ENOUGH_INPUT );


    // 월을 의미하는 문자열 중 가장 긴 것이 4자 = VIII
    // 현재 문자열의 길이와 비교하여 작은 것을 저장함
    sMax = IDL_MIN(*aLength, 4);
        
    // 문자열의 최대 길이를 계산
    for( sLength = 0;
         sLength < sMax;
         sLength++)
    {
        if( mInputST[(*aString)[sLength]] != mALPHA  )
        {
            break;
        }
    }

    // 문자열을 복사하고, 모두 대문자로 변환
    idlOS::memcpy( sString, *aString, sLength );
    idlOS::strUpper( sString, sLength );
    
    while ( ( ( sString[sIdx] == 'I' ) ||
              ( sString[sIdx] == 'V' ) ||
              ( sString[sIdx] == 'X' ) ) &&
            ( ((SInt)sLength - ( sICnt + sVCnt + sXCnt )) > 0 ) )
    {
        if ( sIsEnd == ID_TRUE )
        {
            IDE_RAISE( ERR_INVALID_LITERAL );
        }
                    
        if ( sString[sIdx] == 'I' )
        {
            if ( sICnt >= 3 )
            {
                IDE_RAISE( ERR_INVALID_LITERAL );
            }
            else
            {
                sMonth++;
                sICnt++;
                sIsMinus = ID_TRUE;
            }
        }
        else if ( sString[sIdx] == 'V' )
        {
            if ( sICnt > 1 || sVCnt == 1 )
            {
                IDE_RAISE( ERR_INVALID_LITERAL );
            }

            if ( sIsMinus == ID_TRUE )
            {
                sMonth += 3; 
                sIsMinus = ID_FALSE;
                sIsEnd = ID_TRUE;
            } 
            else
            {
                sMonth += 5;
            }
            sVCnt++;
        }
        else if ( sString[sIdx] == 'X' )
        {
            if ( sICnt > 1 || sXCnt == 1 )
            {
                IDE_RAISE( ERR_INVALID_LITERAL );
            }

            if ( sIsMinus == ID_TRUE ) 
            {
                sMonth += 8; 
                sIsMinus = ID_FALSE;
                sIsEnd = ID_TRUE;
            }
            else
            {
                sMonth += 10;
            }
            sXCnt++;
        }
        sIdx++;
    }

    *aMonth   = sMonth;
    *aString += ( sICnt + sVCnt + sXCnt );
    *aLength -= ( sICnt + sVCnt + sXCnt );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );
    
    IDE_EXCEPTION( ERR_NOT_ENOUGH_INPUT );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdDateInterface::toDateInternal( mtdDateType * aDate,
                                         UChar       * aString,
                                         UInt          aStringLen,
                                         UChar       * aFormat,
                                         UInt          aFormatLen,
                                         void        * aScanner )
{
    // Variables for mtldl lexer (scanner)
    UInt         sToken;
    yyscan_t     sScanner;

    // Local Variable
    SInt         sIdx;
    SInt         sLeap         = 0;         // 0:일반, 1:윤년
    UInt         sNumber;
    UInt         sFormatLen;
    idBool       sIsAM         = ID_TRUE;   // 기본은 AM
    idBool       sPrecededWHSP = ID_FALSE;  // 입력 문자열에
                                            // 공백,탭,줄바꿈이
                                            // 있었는지 검사
    UInt         sOldLen;                   // 문자열 길이 비교 검사 등을 위해
    UChar       *sString;
    UInt         sStringLen;
    UChar       *sFormat;                   // 현재 처리하고 있는 포맷의 위치를 가리킴
    UChar        sErrorFormat[MTC_TO_CHAR_MAX_PRECISION+1];

    // 같은 값에 대해 중복된 입력이 있는 경우를 대비하여
    // aDate에 값을 직접 적용하지 않고, 보관하기 위한 변수
    // 가능한 포맷들: 'SSSSS', 'DDD', 'DAY'
    UInt         sHour         = 0;
    UInt         sMinute       = 0;
    UInt         sSecond       = 0;
    UChar        sMonth        = 0;
    UChar        sDay          = 0;
    UInt         sDayOfYear    = 0;         // Day of year (1-366)

    // Date format element가 중복으로 출현하는 것을 막기 위한 변수
    // To fix BUG-14516
    idBool       sSetYY        = ID_FALSE;
    idBool       sSetMM        = ID_FALSE;
    idBool       sSetDD        = ID_FALSE;
    idBool       sSetAMPM      = ID_FALSE;
    idBool       sSetHH12      = ID_FALSE;
    idBool       sSetHH24      = ID_FALSE;
    idBool       sSetMI        = ID_FALSE;
    idBool       sSetSS        = ID_FALSE;
    idBool       sSetMS        = ID_FALSE;  // microsecond
    idBool       sSetSID       = ID_FALSE;  // seconds in day
//  idBool       sSetDOW       = ID_FALSE;  // day of week (구현보류)
    idBool       sSetDDD       = ID_FALSE;  // day of year

    IDE_ASSERT( aDate   != NULL );
    IDE_ASSERT( aString != NULL );
    IDE_ASSERT( aFormat != NULL );

    // aString과 aFormat의 양쪽 공백을 제거한다.
    // To fix BUG-15087
    // 앞쪽 공백은 while loop안에서 처리한다.
    while( aStringLen > 0 )
    {
        if( mInputST[aString[aStringLen-1]] == mWHSP )
        {
            aStringLen--;
        }
        else
        {
            break;
        }
    }
    
    while( aFormatLen > 0 )
    {
        if( mInputST[aFormat[aFormatLen-1]] == mWHSP )
        {
            aFormatLen--;
        }
        else
        {
            break;
        }
    }

    sFormat       = aFormat;
    sString       = aString;
    sStringLen    = aStringLen;
    sScanner      = aScanner;

    // get first token
    sToken = mtddllex( sScanner );

    while( sToken != 0 )
    {
        
        // 공백, 탭, 줄바꿈등이 있다면 무시
        sPrecededWHSP = ID_FALSE;
        // BUG-18788 sStringLen을 검사해야 함
        while( sStringLen > 0 )
        {
            if ( mInputST[*sString] == mWHSP )
            {
                sString++;
                sStringLen--;
                sPrecededWHSP = ID_TRUE;
            }
            else
            {
                break;
            }
        }        

        switch( sToken )
        {
            /***********************************
             * 정수 입력을 받는 경우
             ***********************************/
            // Year: YCYYY, RRRR, RR, SYYYY, YYYY, YYY, YY, Y
            case MTD_DATE_FORMAT_YCYYY:
            {
                // YCYYY일때 : YYYY와 동일하나
                //            세번쩨 자리에 쉼표(,)를 포함한 연도도 입력 가능
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;
                
                sOldLen = sStringLen;
                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            4,
                                            &sNumber )
                          != IDE_SUCCESS );

                if( ( ( sOldLen - sStringLen ) == 1 ) && ( sStringLen > 0 ) )
                {
                    // 한 자리 숫자만 나온 경우 일반적으로 ','가 예상된다
                    if( *sString == ',' )
                    {
                        // 숫자가 나오고 쉼표(,)가 있다면 무조건 4자리
                        // 연도 입력으로 가정한다. 물론 다른 포맷을
                        // 위한 구분자일 수 있지만, YCYYY와 함께 쉼표를
                        // 구분자로 사용하면서 더 불어 한자리의 연도를
                        // 입력하는 비양심적인 입력 값은 에러로 간주함
                        // 예) to_date( '9,9,9', 'YCYYY,MM,DD' )
                        
                        // 천의 자리 입력
                        IDE_TEST( setYear( aDate, sNumber * 1000 )
                                  != IDE_SUCCESS );                        
                        
                        // ','가 있으면 나머지 3자리(고정) 읽어서 완성
                        sOldLen = sStringLen;
                        IDE_TEST( toDateGetInteger( &sString,
                                                    &sStringLen,
                                                    3,
                                                    &sNumber )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE(
                            ( sOldLen - sStringLen ) != 3,
                            ERR_NOT_ENOUGH_INPUT );

                        // 나머지 세 자리 추가
                        IDE_TEST( setYear( aDate, year(aDate) + sNumber )
                                  != IDE_SUCCESS );                        
                        
                    }
                    else
                    {
                        // 없으면 그대로 저장 (1자리 수의 연도를 의미)
                        IDE_TEST( setYear( aDate, sNumber )
                                  != IDE_SUCCESS );                    
                    }
                }
                else
                {
                    // 그 외의 경우 사용자가 입력한 그대로 저장
                    IDE_TEST( setYear( aDate, sNumber )
                              != IDE_SUCCESS );                    
                }

                break;
            }

            case MTD_DATE_FORMAT_RRRR:
            {
                // RRRR일때 : 4자리 일 때는 액면 그대로
                //           4자리가 아닐 경우 아래의 규칙을 따름
                //            RRRR < 50  이면 2000을 더한다.
                //            RRRR < 100 이면 1900을 더한다.
                //            RRRR >=100 이면 액면 그대로
                //     '2005' : 2005년
                //     '0005' : 5년
                //     '005'  : 2005년
                //     '05'   : 2005년
                //     '5'    : 2005년
                //     '100'  : 0100년
                //     '99'   : 1999년
                //     '50'   : 1950년
                //     '49'   : 2049년
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;

                sOldLen = sStringLen;
                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            4,
                                            &sNumber )
                          != IDE_SUCCESS );

                if( sOldLen - sStringLen < 4 )
                {
                    // 4자리가 아닌 경우 아래와 같이
                    if( sNumber < 50 )
                    {
                        sNumber += 2000;
                    }
                    else if( sNumber < 100 )
                    {
                        sNumber += 1900;
                    }
                }
                else
                {
                    // 4자리 일 때는 입력값 그대로
                    // do nothing
                }

                IDE_TEST( setYear( aDate, sNumber )
                          != IDE_SUCCESS );

                break;
            }

            case MTD_DATE_FORMAT_RR:
            {
                // RR일때   : RR < 50  이면 2000을 더한다.
                //           RR < 100 이면 1900을 더한다.
                //     '2005' : 에러
                //     '005'  : 에러
                //     '05'   : 2005년
                //     '5'    : 2005년
                //     '100'  : 에러
                //     '99'   : 1999년
                //     '50    : 1950년
                //     '49'   : 2049년
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;
                
                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            2,
                                            &sNumber )
                          != IDE_SUCCESS );

                if( sNumber < 50 )
                {
                    IDE_TEST( setYear( aDate, (SShort)(2000 + sNumber) )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( setYear( aDate, (SShort)(1900 + sNumber) )
                              != IDE_SUCCESS );
                }

                break;
            }

            case MTD_DATE_FORMAT_SYYYY :    /* BUG-36296 SYYYY Format 지원 */
            {
                // SYYYY일 때 : '-'가 먼저 오면 음수이고, '+'/''이면 양수이다.
                //     '-2005' : -2006년
                //     '-0005' : -6년
                //     '-005'  : -6년
                //     '-05'   : -6년
                //     '-5'    : -6년
                //     '2005'  : 2005년
                //     '0005'  : 5년
                //     '005'   : 5년
                //     '05'    : 5년
                //     '5'     : 5년
                //     '1970'  : 1970년
                //     '70'    : 70년
                //     '170'   : 170년
                //     '7'     : 7년
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;

                if ( *sString == '-' )
                {
                    // 부호 길이만큼 입력 문자열 줄임
                    sString++;
                    sStringLen--;

                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                4,
                                                &sNumber )
                              != IDE_SUCCESS );

                    IDE_TEST( setYear( aDate, -((SShort)sNumber) ) != IDE_SUCCESS );
                }
                else
                {
                    if ( *sString == '+' )
                    {
                        // 부호 길이만큼 입력 문자열 줄임
                        sString++;
                        sStringLen--;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                4,
                                                &sNumber )
                              != IDE_SUCCESS );

                    IDE_TEST( setYear( aDate, sNumber ) != IDE_SUCCESS );
                }

                break;
            }

            case MTD_DATE_FORMAT_YYYY:
            {
                // YYYY일때 : 입력값을 그대로 숫자로 바꾼다.
                //     '2005' : 2005년
                //     '0005' : 5년
                //     '005'  : 5년
                //     '05'   : 5년
                //     '5'    : 5년
                //     '1970' : 1970년
                //     '70'   : 70년
                //     '170'  : 170년
                //     '7'    : 7년
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;

                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            4,
                                            &sNumber )
                          != IDE_SUCCESS );

                IDE_TEST( setYear( aDate, sNumber )
                          != IDE_SUCCESS );

                break;
            }

            case MTD_DATE_FORMAT_YYY:
            {
                // YYY일때   : 값에 2000을 더한다.
                //     '005'  : 2005년
                //     '05'   : 2005년
                //     '5'    : 2005년
                //     '70'   : 2070년
                //     '170'  : 2170년
                //     '7'    : 2007년
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;

                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            3,
                                            &sNumber )
                          != IDE_SUCCESS );

                IDE_TEST( setYear( aDate, 2000 + sNumber )
                          != IDE_SUCCESS );

                break;
            }            

            case MTD_DATE_FORMAT_YY:
            {
                // YY일때   : 값에 2000을 더한다.
                //     '05'   : 2005년
                //     '5'    : 2005년
                //     '99'   : 2099년
                //     '50    : 2050년
                //     '49'   : 2049년
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;
                
                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            2,
                                            &sNumber )
                          != IDE_SUCCESS );

                IDE_TEST( setYear( aDate, (SShort)(2000 + sNumber) )
                          != IDE_SUCCESS );

                break;
            }

            case MTD_DATE_FORMAT_Y:
            {
                // Y일때   : 값에 2000을 더한다.
                //     '5'    : 2005년
                IDE_TEST_RAISE( sSetYY != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetYY = ID_TRUE;
                
                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            1,
                                            &sNumber )
                          != IDE_SUCCESS );

                IDE_TEST( setYear( aDate, (SShort)(2000 + sNumber) )
                          != IDE_SUCCESS );

                break;
            }
            
            // Month: MM (정수로 입력하는 '월'을 의미하는 format)
            case MTD_DATE_FORMAT_MM:
            {
                IDE_TEST_RAISE( sSetMM != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMM = ID_TRUE;
                
                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            2,
                                            &sNumber )
                          != IDE_SUCCESS );

                IDE_TEST( setMonth( aDate, (UChar)sNumber )
                          != IDE_SUCCESS );

                break;
            }
            
            // Day: DD    
            case MTD_DATE_FORMAT_DD:
            {
                IDE_TEST_RAISE( sSetDD != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetDD = ID_TRUE;
                
                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            2,
                                            &sNumber )
                          != IDE_SUCCESS );
                
                IDE_TEST( setDay( aDate, (UChar)sNumber )
                          != IDE_SUCCESS );

                break;
            }
            
            // Day of year
            // note that this format element set not only month but day
            case MTD_DATE_FORMAT_DDD:
            {
                IDE_TEST_RAISE( sSetDDD != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetDDD = ID_TRUE;

                IDE_TEST( toDateGetInteger( &sString,
                                            &sStringLen,
                                            3,
                                            &sDayOfYear )
                          != IDE_SUCCESS );
                
                // 다른 입력 값(년,월,일)에 따라 다른 조건을 검사해야
                // 하므로, 모든 입력이 종료된 후 실제 값을 입력

                break;
            }
            
            // Hour: HH, HH12, HH24
            case MTD_DATE_FORMAT_HH12:
            {
                // 12시간 단위
                IDE_TEST_RAISE( ( sSetHH12 != ID_FALSE ) ||
                                ( sSetHH24 != ID_FALSE ),
                                ERR_CONFLICT_FORMAT );
                sSetHH12 = ID_TRUE;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                2,
                                                &sNumber )
                              != IDE_SUCCESS );
                
                    // aDate에 입력되는 범위는 1~24 이지만,
                    // 만약 PM이 명시된 경우 함수 종료 전에 +12
                    IDE_TEST_RAISE( sNumber > 12 || sNumber < 1,
                                    ERR_INVALID_HOUR );
                }
                
                IDE_TEST( setHour( aDate, (UChar)sNumber )
                          != IDE_SUCCESS );

                break;
            }
            
            case MTD_DATE_FORMAT_HH:
            case MTD_DATE_FORMAT_HH24:
            {
                // 24시간 단위
                IDE_TEST_RAISE( ( sSetHH24 != ID_FALSE ) ||
                                ( sSetHH12 != ID_FALSE ) ||
                                ( sSetAMPM != ID_FALSE ),
                                ERR_CONFLICT_FORMAT );
                sSetHH24 = ID_TRUE;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                2,
                                                &sNumber )
                              != IDE_SUCCESS );
                }
                
                IDE_TEST( setHour( aDate, (UChar)sNumber )
                          != IDE_SUCCESS );

                break;
            }
            
            // Minute    
            case MTD_DATE_FORMAT_MI:
            {
                IDE_TEST_RAISE( sSetMI != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMI = ID_TRUE;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                2,
                                                &sNumber )
                              != IDE_SUCCESS );
                }
                
                IDE_TEST( setMinute( aDate, (UChar)sNumber )
                          != IDE_SUCCESS );

                break;
            }
            
            // Second    
            case MTD_DATE_FORMAT_SS:
            {
                IDE_TEST_RAISE( sSetSS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetSS = ID_TRUE;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {                
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                2,
                                                &sNumber )
                              != IDE_SUCCESS );
                }
                
                IDE_TEST( setSecond( aDate, (UChar)sNumber )
                          != IDE_SUCCESS );

                break;
            }
            
            // Microsecond
            case MTD_DATE_FORMAT_SSSSSS:
            {
                // microseconds
                IDE_TEST_RAISE( sSetMS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMS = ID_TRUE;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                6,
                                                &sNumber )
                              != IDE_SUCCESS );
                }
                
                IDE_TEST( setMicroSecond( aDate, sNumber ) );
                break;
            }

            // millisecond 1-digit
            case MTD_DATE_FORMAT_FF1:
            {
                // second and microsecond
                IDE_TEST_RAISE( sSetMS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMS = ID_TRUE;
                
                sOldLen = sStringLen;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                1,
                                                &sNumber )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( (sOldLen - sStringLen) != 1,
                                    ERR_NOT_ENOUGH_INPUT );
                }
                
                // microsecond를 저장하는 곳에 millisecond저장
                IDE_TEST( setMicroSecond( aDate, sNumber * 100000 )
                          != IDE_SUCCESS );
                
                break;
            }

            // millisecond 2-digit
            case MTD_DATE_FORMAT_FF2:
            {
                // second and microsecond
                IDE_TEST_RAISE( sSetMS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMS = ID_TRUE;
                
                sOldLen = sStringLen;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                2,
                                                &sNumber )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( (sOldLen - sStringLen) != 2,
                                    ERR_NOT_ENOUGH_INPUT );
                }
                
                // microsecond를 저장하는 곳에 millisecond저장
                IDE_TEST( setMicroSecond( aDate, sNumber * 10000 )
                          != IDE_SUCCESS );
                
                break;
            }

            // millisecond 3-digit
            case MTD_DATE_FORMAT_FF3:
            {
                // second and microsecond
                IDE_TEST_RAISE( sSetMS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMS = ID_TRUE;
                
                sOldLen = sStringLen;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                3,
                                                &sNumber )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( (sOldLen - sStringLen) != 3,
                                    ERR_NOT_ENOUGH_INPUT );
                }
                
                // microsecond를 저장하는 곳에 millisecond저장
                IDE_TEST( setMicroSecond( aDate, sNumber * 1000 )
                          != IDE_SUCCESS );
                
                break;
            }

            // millisecond 3-digit, microsecond 1-digit
            case MTD_DATE_FORMAT_FF4:
            {
                // second and microsecond
                IDE_TEST_RAISE( sSetMS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMS = ID_TRUE;
                
                sOldLen = sStringLen;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                4,
                                                &sNumber )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( (sOldLen - sStringLen) != 4,
                                    ERR_NOT_ENOUGH_INPUT );
                }
                
                // microsecond를 저장하는 곳에 millisecond저장
                IDE_TEST( setMicroSecond( aDate, sNumber * 100 )
                          != IDE_SUCCESS );
                
                break;
            }

            // millisecond 3-digit, microsecond 2-digit
            case MTD_DATE_FORMAT_FF5:
            {
                // second and microsecond
                IDE_TEST_RAISE( sSetMS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMS = ID_TRUE;
                
                sOldLen = sStringLen;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                5,
                                                &sNumber )
                              != IDE_SUCCESS );
                                
                    IDE_TEST_RAISE( (sOldLen - sStringLen) != 5,
                                    ERR_NOT_ENOUGH_INPUT );
                }
                
                // microsecond를 저장하는 곳에 millisecond저장
                IDE_TEST( setMicroSecond( aDate, sNumber * 10 )
                          != IDE_SUCCESS );
                
                break;
            }

            // microsecond 6-digit
            case MTD_DATE_FORMAT_FF:
            case MTD_DATE_FORMAT_FF6:
            {
                // second and microsecond
                IDE_TEST_RAISE( sSetMS != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMS = ID_TRUE;
                
                sOldLen = sStringLen;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                6,
                                                &sNumber )
                              != IDE_SUCCESS );
                
                    IDE_TEST_RAISE( (sOldLen - sStringLen) != 6,
                                    ERR_NOT_ENOUGH_INPUT );
                }
            
                IDE_TEST( setMicroSecond( aDate, sNumber )
                          != IDE_SUCCESS );
                
                break;
            }

            // both second and microsecond
            case MTD_DATE_FORMAT_SSSSSSSS:
            {
                // second and microsecond
                IDE_TEST_RAISE( ( sSetSS != ID_FALSE ) ||
                                ( sSetMS != ID_FALSE ),
                                ERR_CONFLICT_FORMAT );
                sSetSS = ID_TRUE;
                sSetMS = ID_TRUE;
                
                // fix for BUG-14067
                sOldLen = sStringLen;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sNumber = 0;
                }
                else
                {
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                8,
                                                &sNumber )
                              != IDE_SUCCESS );

                    // fix for BUG-14067
                    IDE_TEST_RAISE( (sOldLen - sStringLen) != 8,
                                    ERR_NOT_ENOUGH_INPUT );
                }
                
                IDE_TEST( setSecond( aDate, sNumber / 1000000 )
                          != IDE_SUCCESS );
                IDE_TEST( setMicroSecond( aDate, sNumber % 1000000 )
                          != IDE_SUCCESS );
                
                break;
            }
            
            // Seconds in day
            case MTD_DATE_FORMAT_SSSSS:
            {
                IDE_TEST_RAISE( sSetSID != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetSID = ID_TRUE;

                // BUG-43860
                if ( sStringLen == 0 )
                {
                    sHour = 0;
                }
                else
                {
                    // Seconds past midnight (0-86399)
                    IDE_TEST( toDateGetInteger( &sString,
                                                &sStringLen,
                                                5,
                                                &sNumber )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sNumber>86399 ,
                                    ERR_INVALID_SEC_IN_DAY );
                }
                
                sHour = sNumber / 3600;
                sMinute = ( sNumber - sHour * 3600 ) / 60;
                sSecond = sNumber % 60;
                
                // switch문이 종료된 이후에
                // 기존에 설정된 시간/분/초가 있다면 충돌하는지 검사
                
                break;
            }

            /***********************************
             * 오전/오후를 나타내는 FORMAT
             ***********************************/
            case MTD_DATE_FORMAT_AM_U:
            case MTD_DATE_FORMAT_AM_UL:
            case MTD_DATE_FORMAT_AM_L:
            case MTD_DATE_FORMAT_PM_U:
            case MTD_DATE_FORMAT_PM_UL:
            case MTD_DATE_FORMAT_PM_L:
            {
                // AM or PM
                IDE_TEST_RAISE( ( sSetAMPM != ID_FALSE ) ||
                                ( sSetHH24 != ID_FALSE ),
                                ERR_CONFLICT_FORMAT );
                sSetAMPM = ID_TRUE;

                if( idlOS::strCaselessMatch( sString,2,"AM",2) == 0 )
                {
                    sIsAM = ID_TRUE;
                }
                else if( idlOS::strCaselessMatch( sString,2,"PM",2) == 0 )
                {
                    sIsAM = ID_FALSE;
                }
                else
                {
                    // BUG-43860
                    IDE_TEST_RAISE( sStringLen != 0, ERR_LITERAL_MISMATCH );
                }
                
                // BUG-43860
                if ( sStringLen != 0 )
                {
                    sString    += 2;
                    sStringLen -= 2;
                }
                else
                {
                    // nothing to do
                }
                                    
                // 실제 aDate에 반영은 loop 종료 후
                break;
            }
            
            /***********************************
             * 영문으로 입력된 '월'을 나타내는 문자열
             ***********************************/
            case MTD_DATE_FORMAT_MON_U:
            case MTD_DATE_FORMAT_MON_UL:
            case MTD_DATE_FORMAT_MON_L:
            case MTD_DATE_FORMAT_MONTH_U:
            case MTD_DATE_FORMAT_MONTH_UL:
            case MTD_DATE_FORMAT_MONTH_L:
            {
                // Month
                IDE_TEST_RAISE( sSetMM != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMM = ID_TRUE;

                IDE_TEST( toDateGetMonth( &sString,
                                          &sStringLen,
                                          &sNumber )
                          != IDE_SUCCESS );
                
                IDE_TEST( setMonth( aDate, sNumber )
                          != IDE_SUCCESS ); 

                break;
            }
            
            // 로마자로 된 월
            case MTD_DATE_FORMAT_RM_U:
            case MTD_DATE_FORMAT_RM_L:
            {
                IDE_TEST_RAISE( sSetMM != ID_FALSE,
                                ERR_CONFLICT_FORMAT );
                sSetMM = ID_TRUE;

                IDE_TEST( toDateGetRMMonth( &sString,
                                            &sStringLen,
                                            &sNumber )
                          != IDE_SUCCESS );
                
                IDE_TEST( setMonth( aDate,sNumber )
                          != IDE_SUCCESS );
                
                break;
            }
            
            /***********************************
             * ""로 묶인 문자열
             ***********************************/
            case MTD_DATE_FORMAT_DOUBLE_QUOTE_STRING:
            {
                // 양쪽 쌍 따옴표는 제외하고 문자열을 비교해야 한다
                // 따라서 길이는 -2만큼 줄이고
                // 토큰의 시작은 +1만큼 건너뛰어 시작하면 된다

                // double quote string의 길이 계산
                sFormatLen = (UInt) mtddlget_leng( sScanner ) - 2;

                IDE_TEST_RAISE( sStringLen < sFormatLen,
                                ERR_NOT_ENOUGH_INPUT );
                
                IDE_TEST_RAISE(
                    idlOS::strMatch( sString,
                                     sFormatLen,
                                     mtddlget_text(sScanner)+1,
                                     sFormatLen ),
                    ERR_LITERAL_MISMATCH );

                // 문자열의 길이만큼 입력 문자열 줄임
                sString    += sFormatLen;
                sStringLen -= sFormatLen;
                
                break;
            }
            
            /***********************************
             * 구분자 -/,.:;'
             ***********************************/
            case MTD_DATE_FORMAT_SEPARATOR:
            {
                // FORMAT에 separator가 등장하면,
                // input string에 seperator나 white space가 꼭 나와야 함
                // BUG-18788 sStringLen을 검사해야 함
                if ( sStringLen > 0 )
                {
                    if ( ( mInputST[*sString] == mSEPAR ) || ( sPrecededWHSP == ID_TRUE ) )
                    {
                        // white space가 구분자로 사용된 경우는 이미 switch문 이전에 제거되었을 것이므로
                        // 구분자인 경우에만 입력 문자열 위치를 변경하고, 길이를 조정함
                        // 구분자가 여러 문자로 구성된 경우, 해당 문자열 길이 만큼 줄임
                        sFormatLen = (UInt) mtddlget_leng( sScanner );
                        // BUG-18788 sStringLen을 검사해야 함
                        for ( sIdx=0;
                              ( sIdx < (SInt)sFormatLen ) && ( sStringLen > 0 );
                              sIdx++ )
                        {
                            if ( mInputST[*sString] == mSEPAR )
                            {
                                sString++;
                                sStringLen--;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }

                break;
            }
            
            /***********************************
             * 이도 저도 아니면 인식할 수 없는 포맷
             ***********************************/
            default:
            {
                sFormatLen = IDL_MIN( MTC_TO_CHAR_MAX_PRECISION,
                                     aFormatLen - ( sFormat - aFormat ) );
                idlOS::memcpy( sErrorFormat,
                               sFormat,
                               sFormatLen );
                sErrorFormat[sFormatLen] = '\0';
                
                IDE_RAISE( ERR_NOT_RECONGNIZED_FORMAT );
            }
            
        }

        // 다음 포맷의 시작위치를 가리킴
        sFormat += mtddlget_leng( sScanner );
        
        // get next token
        sToken = mtddllex( sScanner );
    }

    // 스캔을 모두 끝냈는데, string이 남아 있다면 에러
    IDE_TEST_RAISE( sStringLen != 0,
                    ERR_NOT_ENOUGH_FORMAT );

    //-----------------------------------------------------------
    // 년/월/일 관련 검증
    //-----------------------------------------------------------

    if( isLeapYear( year(aDate) ) == ID_TRUE )
    {
        sLeap = 1;
    }
    else
    {
        sLeap = 0;
    }

    // Day of year가 입력되어 있는 경우 확인하여 적절하게 입력함
    // DDD는 아래와 같은 경우에 문제가 있음.
    // TO_DATE( '366-2004', 'DDD-YYYY' ) => 올바른 결과는 2004년 12월 31일
    // 하지만 현재 날짜가 2005년도이기 때문에 366이 들어가면 12월
    // 31일을 넘기 때문에 INVALID_DAY 에러 발생함.  문제의 원인은 date
    // format model을 앞에서부터 순서대로 읽기 때문임.
    // => 따라서 모든 입력을 처리한뒤, 마지막으로 적합성을 검사하여 값을 입력
    if( sSetDDD == ID_TRUE )
    {

        IDE_TEST_RAISE( (sDayOfYear < 1 ) || ( sDayOfYear > (UInt)(365+sLeap) ),
                        ERR_INVALID_DAY_OF_YEAR );

        // '월'을 계산
        // day of year를 30으로 나눈 값은 항상 해당 '월'이거나
        // 해당 '월'보다 1작은 '월'이 나오므로 아래와 같이 계산 가능
        sMonth = (UChar)(sDayOfYear/30);
        if( sDayOfYear > mAccDaysOfMonth[sLeap][sMonth] )
        {
            sMonth++;
        }
        else
        {
            // do nothing
        }

        IDE_TEST_RAISE( sMonth == 0,
                        ERR_INVALID_DAY_OF_YEAR );

        // '일'을 계산
        sDay = sDayOfYear - mAccDaysOfMonth[sLeap][sMonth-1];
        
        // 이미 '월'이 입력되었다면, day of year로 지정한 '월'과 일치하여야 함
        IDE_TEST_RAISE( ( sSetMM == ID_TRUE ) &&
                        ( sMonth != month(aDate) ),
                        ERR_DDD_CONFLICT_MM );
        
        // 이미 '일'이 입력되었다면, day of year로 지정된 '일'과 일치하여야 함
        IDE_TEST_RAISE( ( sSetDD == ID_TRUE ) &&
                        ( sDay != day(aDate) ),
                        ERR_DDD_CONFLICT_DD );

        // 위의 에러 검사를 모두 통과하면 값을 입력
        IDE_TEST( setMonth( aDate, sMonth ) != IDE_SUCCESS );
        IDE_TEST( setDay( aDate, sDay ) != IDE_SUCCESS );

        IDE_DASSERT( ( sDayOfYear > mAccDaysOfMonth[sLeap][sMonth-1] ) &&
                     ( sDayOfYear <=  mAccDaysOfMonth[sLeap][sMonth] ) );
    }
    else
    {
        // 'DDD'포맷을 이용하여 day of year가 입력되지 않은 경우
        // do nothing
    }

    /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
    if ( ( year( aDate ) == 1582 ) &&
         ( month( aDate ) == 10 ) &&
         ( 4 < day( aDate ) ) && ( day( aDate ) < 15 ) )
    {
        IDE_TEST( setDay( aDate, 15 ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // fix BUG-18787
    // year 또는 month가 세팅이 안되어있을 경우에는
    // 나중에 체크한다.
    if ( ( year( aDate ) != ID_SSHORT_MAX ) &&
         ( month( aDate ) != 0 ) )
    {
        // '일'이 제대로 (윤년등을 고려하여) 설정되었는지 검사
        // lower bound는 setDay에서 검사했으므로 upper bound만 검사
        IDE_TEST_RAISE( day(aDate) > mDaysOfMonth[sLeap][month(aDate)],
                        ERR_INVALID_DAY );
    }
    else
    {
        // do nothing
    }
    
    //-----------------------------------------------------------
    // 시/분/초 관련 검증
    //-----------------------------------------------------------    
    // To fix BUG-14516
    // am/pm format을 사용한 경우 HH나 HH24를 사용할 수 없다.
    // -> sSetXXXX 를 이용하여 모든 포맷 사용의 중복을 검사하므로
    // 이 단계에서 따로 검사를 수행할 필요가 없다.
    if( ( sSetHH12 == ID_TRUE ) ||
        ( sSetAMPM == ID_TRUE ) )
    {
        if( sIsAM == ID_TRUE )    
        {
            if( hour(aDate) == 12 )
            {
                // BUG-15640
                // 12AM은 00AM이다.
                setHour( aDate, 0 );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if( hour(aDate) == 12 )
            {
                // BUG-15640
                // 12PM은 12PM이다.
                // Nothing to do.
            }
            else
            {
                setHour( aDate, hour(aDate) + 12 );
            }
        }
    }
    else
    {
        // 24시간 단위의 입력인 경우
        // do nothing
    }    
    
    // 'SSSSS' 포맷을 통해 값이 입력된 경우, HH[12|24]?, MI, SS로
    // 입력된 값이 있다면 일치해야 함
    if( sSetSID == ID_TRUE )
    {
        // 이미 '시'가 입력 되었다면, seconds in day로 입력된 값과 일치하여야 함
        IDE_TEST_RAISE( ( ( sSetHH12 == ID_TRUE ) || ( sSetHH24 == ID_TRUE ) ) &&
                        ( sHour != hour(aDate) ),
                        ERR_SSSSS_CONFLICT_HH );
        
        // 이미 '분'이 입력 되었다면, seconds in day로 입력된 값과 일치하여야 함
        IDE_TEST_RAISE( ( sSetMI == ID_TRUE ) &&
                        ( sMinute != minute(aDate) ),
                        ERR_SSSSS_CONFLICT_MI );

        // 이미 '초'가 입력 되었다면, seconds in day로 입력된 값과 일치하여야 함
        IDE_TEST_RAISE( ( sSetSS == ID_TRUE ) &&
                        ( sSecond != second(aDate) ),
                        ERR_SSSSS_CONFLICT_SS );

        // 위의 에러 검사를 모두 통과하면 값을 입력
        IDE_TEST( setHour( aDate, sHour ) != IDE_SUCCESS );        
        IDE_TEST( setMinute( aDate, sMinute ) != IDE_SUCCESS );
        IDE_TEST( setSecond( aDate, sSecond ) != IDE_SUCCESS );
    }
    else
    {
        // 'SSSSS'포맷을 이용하여 seconds in day가 입력되지 않은 경우
        // do nothing        
    }
    
    
//  // dat of week도 day of year와 같은 이유로 여기서 입력되어야 함
//  // day of week
//  if( sSetDOW == ID_TRUE )
//  {
//  }


    // format 이나 input이 남는 경우 검사
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONFLICT_FORMAT );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_CONFLICT_FORMAT) );
    }
    IDE_EXCEPTION( ERR_INVALID_DAY_OF_YEAR );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_INVALID_DAY_OF_YEAR) );
    }
    IDE_EXCEPTION( ERR_INVALID_SEC_IN_DAY );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_INVALID_SEC_IN_DAY) );
    }
    IDE_EXCEPTION( ERR_INVALID_HOUR );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_INVALID_HOUR) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_INPUT );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_FORMAT );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_NOT_ENOUGH_FORMAT) );
    }
    IDE_EXCEPTION( ERR_LITERAL_MISMATCH );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );
    }
    IDE_EXCEPTION( ERR_NOT_RECONGNIZED_FORMAT );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_NOT_RECOGNIZED_FORMAT,
                                 sErrorFormat ) );
    }
    IDE_EXCEPTION( ERR_SSSSS_CONFLICT_SS );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_SSSSS_CONFLICT_SS) );
    }
    IDE_EXCEPTION( ERR_SSSSS_CONFLICT_MI );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_SSSSS_CONFLICT_MI) );
    }
    IDE_EXCEPTION( ERR_SSSSS_CONFLICT_HH );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_SSSSS_CONFLICT_HH) );
    }
    IDE_EXCEPTION( ERR_DDD_CONFLICT_DD );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_DDD_CONFLICT_DD) );
    }
    IDE_EXCEPTION( ERR_DDD_CONFLICT_MM );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_DDD_CONFLICT_MM) );
    }
    IDE_EXCEPTION( ERR_INVALID_DAY );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_DAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC mtdDateInterface::toCharInternal( mtdDateType * aDate,
                                         UChar       * aString,
                                         UInt        * aStringLen,    // 실제 기록된 길이 저장
                                         SInt          aStringMaxLen,
                                         UChar       * aFormat,
                                         UInt          aFormatLen,
                                         void        * aScanner )
{
    UInt    sToken;

    // temp variables
    SShort       sYear         = year( aDate );
    UChar        sMonth        = month( aDate );
    SInt         sDay          = day( aDate );
    SInt         sHour         = hour( aDate );
    SInt         sMin          = minute( aDate );
    SInt         sSec          = second( aDate );
    UInt         sMicro        = microSecond( aDate );
    SInt         sDayOfYear;
    UShort       sWeekOfMonth;
    SShort       sAbsYear      = 0;
    yyscan_t     sScanner;
    idBool       sIsFillMode   = ID_FALSE;
    UChar       *sString       = aString;
    SInt         sStringMaxLen = aStringMaxLen;
    UInt         sLength       = 0;

    // for error msg
    UChar       *sFormat;     // 현재 처리하고 있는 포맷의 위치를 가리킴
    UChar        sErrorFormat[MTC_TO_CHAR_MAX_PRECISION+1];
    UInt         sFormatLen;    

    if ( sYear < 0 )
    {
        sAbsYear = -sYear;
    }
    else
    {
        sAbsYear = sYear;
    }

    // terminate character를 고려하여 실제로 문자를 저장할 수 있는 최대
    // 길이는 하나 줄임
    sStringMaxLen--;

    sFormat         = aFormat;
    sScanner        = aScanner;

    // get first token
    sToken = mtddllex( sScanner );
    while( sToken != 0 )
    {
        /* BUG-36296 FM은 공간을 차지하지 않는다. */
        sLength = 0;

        switch ( sToken )
        {
            // 반복적으로 appendFORMAT을 호출함에 따라
            // '러시아 페인트공 알고리즘'의 문제가 발생함
            // 따라서 appendFORMAT을 snprintf로 바꾸고,
            // 출력되는 길이에따라 문자열 포인터를 조정
            case MTD_DATE_FORMAT_SEPARATOR :
                sLength = IDL_MIN( sStringMaxLen, mtddlget_leng(sScanner) );
                idlOS::memcpy( sString,
                               mtddlget_text(sScanner),
                               sLength );
                break;

            case MTD_DATE_FORMAT_AM_U :
            case MTD_DATE_FORMAT_PM_U :
                if ( sHour < 12 )
                {
                    sLength = IDL_MIN( sStringMaxLen, 2 );
                    idlOS::memcpy( sString, "AM", sLength );
                }
                else
                {
                    sLength = IDL_MIN( sStringMaxLen, 2 );
                    idlOS::memcpy( sString, "PM", sLength );
                }
                break;

            case MTD_DATE_FORMAT_AM_UL :
            case MTD_DATE_FORMAT_PM_UL :
                if ( sHour < 12 )
                {
                    sLength = IDL_MIN( sStringMaxLen, 2 );
                    idlOS::memcpy( sString, "Am", sLength );
                }
                else
                {
                    sLength = IDL_MIN( sStringMaxLen, 2 );
                    idlOS::memcpy( sString, "Pm", sLength );
                }
                break;

            case MTD_DATE_FORMAT_AM_L :
            case MTD_DATE_FORMAT_PM_L :
                if ( sHour < 12 )
                {
                    sLength = IDL_MIN( sStringMaxLen, 2 );
                    idlOS::memcpy( sString, "am", sLength );
                }
                else
                {
                    sLength = IDL_MIN( sStringMaxLen, 2 );
                    idlOS::memcpy( sString, "pm", sLength );
                }
                break;

            case MTD_DATE_FORMAT_SCC :
                if ( sYear <= 0 )
                {
                    /* Year 0은 BC -1년이다. 절대값을 구하기 전에 보정한다. */
                    if ( sIsFillMode == ID_FALSE )
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "-%02"ID_INT32_FMT,
                                                   ( ( abs( sYear - 1 ) + 99 ) / 100 ) % 100 );
                    }
                    else
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "-%"ID_INT32_FMT,
                                                   ( ( abs( sYear - 1 ) + 99 ) / 100 ) % 100 );
                    }
                }
                else
                {
                    if ( sIsFillMode == ID_FALSE )
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   " %02"ID_INT32_FMT,
                                                   ( ( sYear + 99 ) / 100 ) % 100 );
                    }
                    else
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%"ID_INT32_FMT,
                                                   ( ( sYear + 99 ) / 100 ) % 100 );
                    }
                }
                break;

            case MTD_DATE_FORMAT_CC :
                if ( sYear <= 0 )
                {
                    /* Year 0은 BC -1년이다. 절대값을 구하기 전에 보정한다.
                     * 음수일 때, 부호를 제거한다. (Oracle)
                     */
                    if ( sIsFillMode == ID_FALSE )
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%02"ID_INT32_FMT,
                                                   ( ( abs( sYear - 1 ) + 99 ) / 100 ) % 100 );
                    }
                    else
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%"ID_INT32_FMT,
                                                   ( ( abs( sYear - 1 ) + 99 ) / 100 ) % 100 );
                    }
                }
                else
                {
                    if ( sIsFillMode == ID_FALSE )
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%02"ID_INT32_FMT,
                                                   ( ( sYear + 99 ) / 100 ) % 100 );
                    }
                    else
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%"ID_INT32_FMT,
                                                   ( ( sYear + 99 ) / 100 ) % 100 );
                    }
                }

                break;

            case MTD_DATE_FORMAT_DAY_U :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gDAYName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
                break;

            case MTD_DATE_FORMAT_DAY_UL :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gDayName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
                break;

            case MTD_DATE_FORMAT_DAY_L :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gdayName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
                break;

            case MTD_DATE_FORMAT_DDD :
                sDayOfYear = mtc::dayOfYear( sYear, sMonth, sDay );

                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                         "%03"ID_INT32_FMT, sDayOfYear );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                         "%"ID_INT32_FMT, sDayOfYear );
                }
                break;

            case MTD_DATE_FORMAT_DD :

                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%02"ID_INT32_FMT, sDay );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT, sDay );
                }
                break;

            case MTD_DATE_FORMAT_DY_U :

                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gDYName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
                break;
            case MTD_DATE_FORMAT_DY_UL :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gDyName[mtc::dayOfWeek( sYear, sMonth, sDay )] );

                break;

            case MTD_DATE_FORMAT_DY_L :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gdyName[mtc::dayOfWeek( sYear, sMonth, sDay )] );
                break;

            case MTD_DATE_FORMAT_D :
                sLength =
                    idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%1"ID_INT32_FMT,
                                     mtc::dayOfWeek( sYear, sMonth, sDay ) + 1 );
                break;

            case MTD_DATE_FORMAT_FF1 :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%01"ID_INT32_FMT,
                                         sMicro / 100000);
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%"ID_INT32_FMT,
                                         sMicro / 100000);
                }
                break;

            case MTD_DATE_FORMAT_FF2 :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%02"ID_INT32_FMT,
                                         sMicro / 10000);
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%"ID_INT32_FMT,
                                         sMicro / 10000);
                }
                break;

            case MTD_DATE_FORMAT_FF3 :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%03"ID_INT32_FMT,
                                         sMicro / 1000 );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%"ID_INT32_FMT,
                                         sMicro / 1000 );
                }
                break;

            case MTD_DATE_FORMAT_FF4 :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%04"ID_INT32_FMT,
                                         sMicro / 100);
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%"ID_INT32_FMT,
                                         sMicro / 100);
                }
                break;

            case MTD_DATE_FORMAT_FF5 :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%05"ID_INT32_FMT,
                                         sMicro / 10 );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%"ID_INT32_FMT,
                                         sMicro / 10 );
                }
                break;

            case MTD_DATE_FORMAT_FF :
            case MTD_DATE_FORMAT_FF6 :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%06"ID_INT32_FMT,
                                         sMicro );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%"ID_INT32_FMT,
                                         sMicro );
                }
                break;

            case MTD_DATE_FORMAT_FM :
                // To fix BUG-17693
                sIsFillMode = ID_TRUE;
                break;

            case MTD_DATE_FORMAT_HH :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, 
                                         sStringMaxLen, 
                                         "%02"ID_INT32_FMT, 
                                         sHour );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, 
                                         sStringMaxLen, 
                                         "%"ID_INT32_FMT, 
                                         sHour );
                }
                break;

            case MTD_DATE_FORMAT_HH12 :
                if( sIsFillMode == ID_FALSE )
                {
                    if ( sHour == 0 || sHour == 12 )
                    {
                        sLength =
                            idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                             "%02"ID_INT32_FMT, 12 );
                    }
                    else
                    {
                        sLength =
                            idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                             "%02"ID_INT32_FMT, sHour % 12 );
                    }
                }
                else
                {
                    if ( sHour == 0 || sHour == 12 )
                    {
                        sLength =
                            idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                             "%"ID_INT32_FMT, 12 );
                    }
                    else
                    {
                        sLength =
                            idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                             "%"ID_INT32_FMT, sHour % 12 );
                    }
                }
                break;

            case MTD_DATE_FORMAT_HH24 :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%02"ID_INT32_FMT, sHour );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT, sHour );
                }
                break;

            case MTD_DATE_FORMAT_MI :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%02"ID_INT32_FMT, sMin );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT, sMin );
                }

                break;

            case MTD_DATE_FORMAT_MM :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%02"ID_INT32_FMT, sMonth );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT, sMonth );
                }

                break;

            case MTD_DATE_FORMAT_MON_U :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gMONName[sMonth-1] );
                break;

            case MTD_DATE_FORMAT_MON_UL :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gMonName[sMonth-1] );
                break;
            case MTD_DATE_FORMAT_MON_L :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gmonName[sMonth-1] );
                break;

            case MTD_DATE_FORMAT_MONTH_U :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gMONTHName[sMonth-1] );
                break;

            case MTD_DATE_FORMAT_MONTH_UL :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gMonthName[sMonth-1] );
                break;
            case MTD_DATE_FORMAT_MONTH_L :
                sLength =
                    idlOS::snprintf( (SChar*) sString,
                                     sStringMaxLen,
                                     "%s",
                                     gmonthName[sMonth-1] );
                break;

            case MTD_DATE_FORMAT_Q :
                sLength = idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                           "%1"ID_INT32_FMT, 
                                           (UInt) idlOS::ceil( (SDouble) sMonth / 3 ) );
                break;

            case MTD_DATE_FORMAT_RM_U :
                sLength =
                    idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                     "%s", gRMMonth[sMonth-1] );
                break;

            case MTD_DATE_FORMAT_RM_L :
                sLength =
                    idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                     "%s", grmMonth[sMonth-1] );

                break;

            case MTD_DATE_FORMAT_SSSSSSSS :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                         "%02"ID_INT32_FMT"%06"ID_INT32_FMT,
                                         sSec,
                                         sMicro );
                }
                else
                {
                    if( sSec != 0 )
                    {
                        sLength =
                            idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                             "%"ID_INT32_FMT"%06"ID_INT32_FMT,
                                             sSec,
                                             sMicro );
                    }
                    else
                    {
                        sLength =
                            idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT, sMicro );
                    }
                }
                break;

            case MTD_DATE_FORMAT_SSSSSS :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%06"ID_INT32_FMT, sMicro );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT, sMicro );
                }
                break;

            case MTD_DATE_FORMAT_SSSSS :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%05"ID_INT32_FMT,
                                         ( sHour*60*60 ) +
                                         ( sMin*60 ) + sSec );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString,
                                         sStringMaxLen,
                                         "%"ID_INT32_FMT,
                                         ( sHour*60*60 ) +
                                         ( sMin*60 ) + sSec );
                }
                break;

            case MTD_DATE_FORMAT_SS :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%02"ID_INT32_FMT, sSec );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT, sSec );
                }
                break;

            case MTD_DATE_FORMAT_WW :
                if( sIsFillMode == ID_FALSE )
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%02"ID_INT32_FMT,
                                         mtc::weekOfYear( sYear, sMonth, sDay ) );
                }
                else
                {
                    sLength =
                        idlOS::snprintf( (SChar*) sString, sStringMaxLen, "%"ID_INT32_FMT,
                                         mtc::weekOfYear( sYear, sMonth, sDay ) );
                }
                break;

            case MTD_DATE_FORMAT_IW :   /* BUG-42926 TO_CHAR()에 IW 추가 */
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%02"ID_INT32_FMT,
                                               mtc::weekOfYearForStandard( sYear, sMonth, sDay ) );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               mtc::weekOfYearForStandard( sYear, sMonth, sDay ) );
                }
                break;

            case MTD_DATE_FORMAT_IYYY :   /* BUG-46727 TO_CHAR()에 IYYY 추가 */
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%04"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) );
                }
                break;

            case MTD_DATE_FORMAT_IYY :   /* BUG-46727 TO_CHAR()에 IYY 추가 */
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%03"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) % 1000 );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) % 1000 );
                }
                break;

            case MTD_DATE_FORMAT_IY :   /* BUG-46727 TO_CHAR()에 IY 추가 */
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%02"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) % 100);
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) % 100 );
                }
                break;

            case MTD_DATE_FORMAT_I :   /* BUG-46727 TO_CHAR()에 I 추가 */
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%01"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) % 10 );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               mtc::yearForStandard( sYear, sMonth, sDay ) % 10 );
                }
                break;

            case MTD_DATE_FORMAT_WW2 :  /* BUG-42941 TO_CHAR()에 WW2(Oracle Version WW) 추가 */
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%02"ID_INT32_FMT,
                                               mtc::weekOfYearForOracle( sYear, sMonth, sDay ) );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *) sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               mtc::weekOfYearForOracle( sYear, sMonth, sDay ) );
                }
                break;

            case MTD_DATE_FORMAT_W :
                sWeekOfMonth = (UShort) idlOS::ceil( (SDouble) ( sDay +
                                                                 mtc::dayOfWeek( sYear, sMonth, 1 )
                                                                 ) / 7 );
                sLength =
                    idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                     "%1"ID_INT32_FMT, sWeekOfMonth );
                break;

            case MTD_DATE_FORMAT_YCYYY :
                /* 음수일 때, 부호를 제거한다. (Oracle) */
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%01"ID_INT32_FMT",%03"ID_INT32_FMT,
                                               ( sAbsYear / 1000 ) % 10,
                                               sAbsYear % 1000 );
                }
                else
                {
                    if ( sAbsYear < 1000 )
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%"ID_INT32_FMT,
                                                   sAbsYear % 1000 );
                    }
                    else
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%"ID_INT32_FMT",%03"ID_INT32_FMT,
                                                   ( sAbsYear / 1000 ) % 10,
                                                   sAbsYear % 1000 );
                    }
                }
                break;

            case MTD_DATE_FORMAT_SYYYY :    /* BUG-36296 SYYYY Format 지원 */
                if ( sYear < 0 )
                {
                    if ( sIsFillMode == ID_FALSE )
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "-%04"ID_INT32_FMT,
                                                   sAbsYear % 10000 );
                    }
                    else
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "-%"ID_INT32_FMT,
                                                   sAbsYear % 10000 );
                    }
                }
                else
                {
                    if ( sIsFillMode == ID_FALSE )
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   " %04"ID_INT32_FMT,
                                                   sAbsYear % 10000 );
                    }
                    else
                    {
                        sLength = idlOS::snprintf( (SChar *)sString,
                                                   sStringMaxLen,
                                                   "%"ID_INT32_FMT,
                                                   sAbsYear % 10000 );
                    }
                }
                break;

            case MTD_DATE_FORMAT_YYYY :
            case MTD_DATE_FORMAT_RRRR :
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%04"ID_INT32_FMT,
                                               sAbsYear % 10000 );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               sAbsYear % 10000 );
                }
                break;

            case MTD_DATE_FORMAT_YYY :
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%03"ID_INT32_FMT,
                                               sAbsYear % 1000 );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               sAbsYear % 1000 );
                }
                break;


            case MTD_DATE_FORMAT_YY :
            case MTD_DATE_FORMAT_RR :
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%02"ID_INT32_FMT,
                                               sAbsYear % 100 );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               sAbsYear % 100 );
                }
                break;

            case MTD_DATE_FORMAT_Y :
                if ( sIsFillMode == ID_FALSE )
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%01"ID_INT32_FMT,
                                               sAbsYear % 10 );
                }
                else
                {
                    sLength = idlOS::snprintf( (SChar *)sString,
                                               sStringMaxLen,
                                               "%"ID_INT32_FMT,
                                               sAbsYear % 10 );
                }
                break;

            case MTD_DATE_FORMAT_DOUBLE_QUOTE_STRING :
                sLength =
                    idlOS::snprintf( (SChar*) sString, sStringMaxLen,
                                     "%s", mtddlget_text(sScanner) + 1 );
                // BUG-27290
                // 빈 double-quoted string이라하더라도 적어도 '"'는
                // 복사되므로 sLength는 1보다 작을 수 없다.
                sLength--;
                break;

            default:
                sFormatLen = IDL_MIN( MTC_TO_CHAR_MAX_PRECISION,
                                      aFormatLen - ( sFormat - aFormat ) );
                
                idlOS::memcpy( sErrorFormat,
                               sFormat,
                               sFormatLen );
                sErrorFormat[sFormatLen] = '\0';
                
                IDE_RAISE( ERR_NOT_RECONGNIZED_FORMAT );
        }
        
        sString       += sLength;
        sStringMaxLen -= sLength;
        
        // 다음 포맷의 시작위치를 가리킴
        sFormat += mtddlget_leng( sScanner );
        
        // get next token
        sToken = mtddllex( sScanner );
    }

    // snprintf에서 '\0' termination을 안 할 수 있으므로
    sString[0] = '\0';

    *aStringLen = sString - aString;

    return IDE_SUCCESS;    

    IDE_EXCEPTION( ERR_NOT_RECONGNIZED_FORMAT )
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_NOT_RECOGNIZED_FORMAT,
                                 sErrorFormat ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
IDE_RC
mtdDateInterface::getMaxCharLength( UChar*       aFormat,
                                    UInt         aFormatLen,
                                    UInt*        aLength)
{
    UInt    sToken;

    // temp variables
    SInt         sBufferCur = 0;
    yyscan_t     sScanner;
    idBool       sInitScanner = ID_FALSE;

    IDE_TEST_RAISE( mtddllex_init ( &sScanner ) != 0, ERR_LEX_INIT_FAILED );
    sInitScanner = ID_TRUE;

    mtddl_scan_bytes( (const char*)aFormat, aFormatLen, sScanner );

    // get first token
    sToken = mtddllex( sScanner );
    while( sToken != 0 )
    {
        switch ( sToken )
        {
            case MTD_DATE_FORMAT_NONE :
                sBufferCur += idlOS::strlen( mtddlget_text(sScanner) );
                break;

            case MTD_DATE_FORMAT_AM_U :
            case MTD_DATE_FORMAT_PM_U :
            case MTD_DATE_FORMAT_AM_UL :
            case MTD_DATE_FORMAT_PM_UL :
            case MTD_DATE_FORMAT_AM_L :
            case MTD_DATE_FORMAT_PM_L :
                sBufferCur += 2; //(AM/PM, am,pm.. Am..)
                break;

            case MTD_DATE_FORMAT_CC :
                sBufferCur += 2;
                break;

            case MTD_DATE_FORMAT_DAY_U :
            case MTD_DATE_FORMAT_DAY_L :
            case MTD_DATE_FORMAT_DAY_UL :
                sBufferCur += 9; // "WEDNESDAY"
                break;
            case MTD_DATE_FORMAT_DDD :
                sBufferCur += 3; // "365"
                break;

            case MTD_DATE_FORMAT_DD :
                sBufferCur += 2; // "31"
                break;

            case MTD_DATE_FORMAT_DY_U :
            case MTD_DATE_FORMAT_DY_UL :
            case MTD_DATE_FORMAT_DY_L :
                sBufferCur +=3; // "MON"
                break;

            case MTD_DATE_FORMAT_D :
                sBufferCur +=1;
                break;

            case MTD_DATE_FORMAT_FF1 :
                sBufferCur += 1;
                break;

            case MTD_DATE_FORMAT_FF2 :
                sBufferCur += 2;
                break;

            case MTD_DATE_FORMAT_FF3 :
                sBufferCur += 3;
                break;

            case MTD_DATE_FORMAT_FF4 :
                sBufferCur += 4;
                break;

            case MTD_DATE_FORMAT_FF5 :
                sBufferCur += 5;
                break;

            case MTD_DATE_FORMAT_FF :
            case MTD_DATE_FORMAT_FF6 :
                sBufferCur += 6;
                break;

            case MTD_DATE_FORMAT_HH :
            case MTD_DATE_FORMAT_HH12 :
            case MTD_DATE_FORMAT_HH24 :
            case MTD_DATE_FORMAT_MI :
            case MTD_DATE_FORMAT_MM :
                sBufferCur +=2;
                break;

            case MTD_DATE_FORMAT_MON_U :
            case MTD_DATE_FORMAT_MON_UL :
            case MTD_DATE_FORMAT_MON_L :
                sBufferCur += 3; //'JAN'
                break;
            case MTD_DATE_FORMAT_MONTH_U :
            case MTD_DATE_FORMAT_MONTH_UL :
            case MTD_DATE_FORMAT_MONTH_L :
                sBufferCur += 9; // "september"
                break;

            case MTD_DATE_FORMAT_Q :
                sBufferCur += 1;
                break;

            case MTD_DATE_FORMAT_RM_U :
            case MTD_DATE_FORMAT_RM_L :
                sBufferCur += 4; //'VIII'
                break;

            case MTD_DATE_FORMAT_SSSSSSSS :
                sBufferCur += 8; //'sec:microsec'
                break;

            case MTD_DATE_FORMAT_SSSSSS :
                sBufferCur += 6; //'sec:microsec'
                break;

            case MTD_DATE_FORMAT_SSSSS :
                sBufferCur += 5; 
                break;

            case MTD_DATE_FORMAT_SS :
                sBufferCur += 2; 
                break;

            case MTD_DATE_FORMAT_WW :
                sBufferCur +=2; // week of year
                break;

            case MTD_DATE_FORMAT_W :
                sBufferCur +=1; // week of month
                break;

            case MTD_DATE_FORMAT_YCYYY : 
                sBufferCur += 5; //20,03
                break;

            case MTD_DATE_FORMAT_YYYY :
            case MTD_DATE_FORMAT_RRRR :
                sBufferCur += 4;
                break;

            case MTD_DATE_FORMAT_YY :
            case MTD_DATE_FORMAT_RR :
                sBufferCur += 2;
                break;

            case MTD_DATE_FORMAT_DOUBLE_QUOTE_STRING :
                sBufferCur += idlOS::strlen( mtddlget_text(sScanner) ) + 1;
                break;

            default:
                IDE_RAISE( ERR_INVALID_LITERAL );
        }

        // get next token
    	sToken = mtddllex( sScanner);
    }

    mtddllex_destroy ( sScanner );
    sInitScanner = ID_FALSE;

    if (sBufferCur != 0)
    {
        sBufferCur ++; // trailing \0
    }
    *aLength = sBufferCur;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_LITERAL_MISMATCH) );
    }
    IDE_EXCEPTION( ERR_LEX_INIT_FAILED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtdDateInterface::getMaxCharLength",
                                  "Lex init failed" ));
    }
    IDE_EXCEPTION_END;

    if ( sInitScanner == ID_TRUE )
    {
        mtddllex_destroy ( sScanner );
    }

    return IDE_FAILURE;
}
*/

IDE_RC
mtdDateInterface::checkYearMonthDayAndSetDateValue(
    mtdDateType  * aDate,
    SShort         aYear,
    UChar          aMonth,
    UChar          aDay)
{
    IDE_TEST( setYear( aDate, aYear ) != IDE_SUCCESS );
    IDE_TEST( setMonth( aDate, aMonth ) != IDE_SUCCESS );

    /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
    if ( ( aYear == 1582 ) &&
         ( aMonth == 10 ) &&
         ( 4 < aDay ) && ( aDay < 15 ) )
    {
        IDE_TEST( setDay( aDate, 15 ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( setDay( aDate, aDay ) != IDE_SUCCESS );

        if ( isLeapYear( aYear ) == ID_TRUE )
        {
            IDE_TEST_RAISE( aDay > mDaysOfMonth[1][aMonth], ERR_INVALID_DAY );
        }
        else
        {
            IDE_TEST_RAISE( aDay > mDaysOfMonth[0][aMonth], ERR_INVALID_DAY );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DAY );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_DAY));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtdStoredValue2MtdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              /*aDestValueOffset*/,
                                       UInt              aLength,
                                       const void      * aValue )
{
/******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdDateType  * sDateValue;

    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sDateValue = (mtdDateType*)aDestValue;
    
    if( aLength == 0 )
    {
        // NULL 데이타
        *sDateValue = mtdDateNull;
    }
    else
    {
        IDE_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );        

        idlOS::memcpy( sDateValue, aValue, aLength );        
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


UInt mtdNullValueSize()
{
/*******************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 *******************************************************************/
    return mtdActualSize( NULL, &mtdDateNull );
}

#if defined(HP_HPUX) || defined(IA64_HP_HPUX)		   
IDE_RC mtdDateInterface::toDate( mtdDateType * aDate,
                                 UChar       * aString,
                                 UInt          aStringLen,
                                 UChar       * aFormat,
                                 UInt          aFormatLen )
{
    idBool   sInitScanner = ID_FALSE;
    yyscan_t sScanner;

    IDE_TEST_RAISE( mtddllex_init ( &sScanner ) != 0, ERR_LEX_INIT_FAILED );
    sInitScanner = ID_TRUE;

    mtddl_scan_bytes( (const char*)aFormat, aFormatLen, sScanner );

    IDE_TEST( mtdDateInterface::toDateInternal( aDate,
                                                aString,
                                                aStringLen,
                                                aFormat,
                                                aFormatLen,
                                                sScanner )
              != IDE_SUCCESS );

    mtddllex_destroy ( sScanner );
    sInitScanner = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LEX_INIT_FAILED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtdDateInterface::toDate",
                                  "Lex init failed" ));
    }
    IDE_EXCEPTION_END;

    if ( sInitScanner == ID_TRUE )
    {
        mtddllex_destroy ( sScanner );
    }
    
    return IDE_FAILURE;
}
#else
IDE_RC mtdDateInterface::toDate( mtdDateType * aDate,
                                 UChar       * aString,
                                 UInt          aStringLen,
                                 UChar       * aFormat,
                                 UInt          aFormatLen )
{
    UInt                     i;
    yyscan_t                 sScanner;
    struct yy_buffer_state * sBufferState[2];
    mtddl_yyguts_t         * sYYGuts;
    SChar                  * sBase;
    UChar                    sFormatBuffer[MTD_FORMAT_SIZE];
    UChar                    sYYBuffer[MTD_YYGUTS_T_SIZE];
    UChar                    sYYBufferState[MTD_YYBUFFER_STATE_SIZE];
    UChar                    sErrorFormat[2];
    YY_BUFFER_STATE          b;

    /* BUG-48113 to_date to_char malloc free 제거 */
    sErrorFormat[0] = '\0';
    IDE_TEST_RAISE( aFormatLen  > ( MTD_FORMAT_SIZE - 2 ), ERR_NOT_RECONGNIZED_FORMAT );

    /* mtddllex_init  */
    sScanner = sYYBuffer;
    sYYGuts = (struct yyguts_t*)sScanner;
    MTDDL_YYGUTS_T_INIT( sYYGuts );
    sYYGuts->yy_init = 1;
    sYYGuts->yy_start = 1;
    sYYGuts->yy_buffer_stack = sBufferState;

    /* mtddl_scan_bytes */
    for ( i = 0; i < aFormatLen + 2; i++ )
    {
        if ( i < aFormatLen )
        {
            sFormatBuffer[i] = aFormat[i];
        }
        else
        {
            sFormatBuffer[i] = '\0';
        }
    }

    sBase = (SChar *)sFormatBuffer;

    /* mtddl_scan_buffer */
    b = (YY_BUFFER_STATE)sYYBufferState;

    MTDDL_YYSTATE_INIT( b );
    b->yy_buf_pos = sBase;
    b->yy_ch_buf  = sBase;
    b->yy_buf_size = aFormatLen;
    b->yy_n_chars = b->yy_buf_size;
    b->yy_is_our_buffer = 1;

    /* mtddl_switch_to_buffer */
    MTDDL_SET_BUFFER( b, sYYGuts );

    IDE_TEST( mtdDateInterface::toDateInternal( aDate,
                                                aString,
                                                aStringLen,
                                                aFormat,
                                                aFormatLen,
                                                sScanner )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_RECONGNIZED_FORMAT );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_NOT_RECOGNIZED_FORMAT,
                                 sErrorFormat ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

#if defined(HP_HPUX) || defined(IA64_HP_HPUX)		   
IDE_RC mtdDateInterface::toChar( mtdDateType * aDate,
                                 UChar       * aString,
                                 UInt        * aStringLen,
                                 SInt          aStringMaxLen,
                                 UChar       * aFormat,
                                 UInt          aFormatLen )
{
    idBool   sInitScanner = ID_FALSE;
    yyscan_t sScanner;

    IDE_TEST_RAISE( mtddllex_init ( &sScanner ) != 0, ERR_LEX_INIT_FAILED );
    sInitScanner = ID_TRUE;

    mtddl_scan_bytes( (const char*)aFormat, aFormatLen, sScanner );

    IDE_TEST( mtdDateInterface::toCharInternal( aDate,
                                                aString,
                                                aStringLen,
                                                aStringMaxLen,
                                                aFormat,
                                                aFormatLen,
                                                sScanner )
              != IDE_SUCCESS );

    mtddllex_destroy ( sScanner );
    sInitScanner = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LEX_INIT_FAILED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtdDateInterface::toDate",
                                  "Lex init failed" ));
    }
    IDE_EXCEPTION_END;

    if ( sInitScanner == ID_TRUE )
    {
        mtddllex_destroy ( sScanner );
    }
    
    return IDE_FAILURE;
}
#else
IDE_RC mtdDateInterface::toChar( mtdDateType * aDate,
                                 UChar       * aString,
                                 UInt        * aStringLen,
                                 SInt          aStringMaxLen,
                                 UChar       * aFormat,
                                 UInt          aFormatLen )
{
    UInt                     i;
    yyscan_t                 sScanner;
    struct yy_buffer_state * sBufferState[2];
    mtddl_yyguts_t         * sYYGuts;
    SChar                  * sBase;
    UChar                    sFormatBuffer[MTD_FORMAT_SIZE];
    UChar                    sYYBuffer[MTD_YYGUTS_T_SIZE];
    UChar                    sYYBufferState[MTD_YYBUFFER_STATE_SIZE];
    UChar                    sErrorFormat[2];
    YY_BUFFER_STATE          b;

    /* BUG-48113 to_date to_char malloc free 제거 */
    sErrorFormat[0] = '\0';
    IDE_TEST_RAISE( aFormatLen  > ( MTD_FORMAT_SIZE - 2 ), ERR_NOT_RECONGNIZED_FORMAT );

    /* mtddllex_init  */
    sScanner = sYYBuffer;
    sYYGuts = (struct yyguts_t*)sScanner;
    MTDDL_YYGUTS_T_INIT( sYYGuts );
    sYYGuts->yy_init = 1;
    sYYGuts->yy_start = 1;
    sYYGuts->yy_buffer_stack = sBufferState;

    /* mtddl_scan_bytes */
    for ( i = 0; i < aFormatLen + 2; i++ )
    {
        if ( i < aFormatLen )
        {
            sFormatBuffer[i] = aFormat[i];
        }
        else
        {
            sFormatBuffer[i] = '\0';
        }
    }

    sBase = (SChar *)sFormatBuffer;

    /* mtddl_scan_buffer */
    b = (YY_BUFFER_STATE)sYYBufferState;

    MTDDL_YYSTATE_INIT( b );
    b->yy_buf_pos = sBase;
    b->yy_ch_buf  = sBase;
    b->yy_buf_size = aFormatLen;
    b->yy_n_chars = b->yy_buf_size;
    b->yy_is_our_buffer = 1;

    /* mtddl_switch_to_buffer */
    MTDDL_SET_BUFFER( b, sYYGuts );

    IDE_TEST( mtdDateInterface::toCharInternal( aDate,
                                                aString,
                                                aStringLen,
                                                aStringMaxLen,
                                                aFormat,
                                                aFormatLen,
                                                sScanner )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_RECONGNIZED_FORMAT );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_DATE_NOT_RECOGNIZED_FORMAT,
                                 sErrorFormat ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

