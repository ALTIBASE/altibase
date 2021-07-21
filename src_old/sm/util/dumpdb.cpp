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
 

/***********************************************************************
 * $Id: dumpdb.cpp 42163 2011-09-27 07:14:10Z elcarim $
 *
 * dumpdb:
 * Database( DRDB DBFile, MRDB CheckpointImage, Loganchor, LogFile)등을
 * Dump하여 분석하고 정보를 출력해주는 도구 입니다.
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtd.h>
#include <mtl.h>
#include <sti.h>
#include <smi.h>
#include <smu.h>
#include <smr.h>
#include <smxDef.h>
#include <smn.h>
#include <smp.h>
#include <sdb.h>
#include <sdp.h>
#include <sdnManager.h>
#include <smmExpandChunk.h>
#include <mmm.h>

#define SMUTIL_TEMP_BUFFER_SIZE (65536) /* hexa 덤프 등을 위한 버퍼 크기 */
#define SMUTIL_ALL_SPACEID      (SC_MAX_SPACE_COUNT) /* Default */
#define SMUTIL_ALL_TABLEOID     (SM_NULL_OID)        /* Default */

typedef enum {
    SMUTIL_JOB_NONE                = -1, /* Job을 설정하지 않은 경우 */
    SMUTIL_JOB_META                = 0,  /* MetaPage를 Dump합니다. */
    SMUTIL_JOB_TABLESPACE          = 1,  /* Tablespace정보를 dump합니다. */
    SMUTIL_JOB_TABLESPACE_FLI      = 2,  /* FreeListInfo를 보여줍니다. */
    SMUTIL_JOB_TABLESPACE_FREEPAGE = 3,  /* FreePageList를 출력합니다. */
    SMUTIL_JOB_TABLE               = 4,  /* Catalog or NormalTable을 조회 */
    SMUTIL_JOB_TABLE_ALLOCPAGE     = 5,  /* Table이 가진 Page 목록입니다. */
    SMUTIL_JOB_PAGE                = 6,  /* Page하나를 Dump합니다. */
    SMUTIL_JOB_INCREMENTAL_BACKUP  = 7,  /* incremental backup 파일의 MetaPage를 Dump합니다. */
    SMUTIL_JOB_MAX                 = 8
} smuJob;

/* Input Parameter */
idBool              gDisplayHeader     = ID_TRUE;  /* Banner 출력여부 */
idBool              gInvalidArgs       = ID_FALSE;
idBool              gDisplaySimple     = ID_TRUE;  /* Hexa,value값 출력여부 */

smuJob              gJob               = SMUTIL_JOB_NONE;
UInt                gPingPong          = 0;
smOID               gOID               = SMUTIL_ALL_TABLEOID;
scSpaceID           gSID               = SMUTIL_ALL_SPACEID ;
scPageID            gPID               = 0 ;

/* LogAnchor에 기록된 File 대신, 다른 CheckpointImage Fileㅇ르 지정하는 경우
 * 사용됩니다. */
SChar             * gTargetFN          = NULL;
SChar               gTBSName[ SM_MAX_FILE_NAME+1 ];

/* Global variable for access checkpointimage */
void             ** gPCH[SC_MAX_SPACE_COUNT];
iduFile             gFile;
iduMemPool          gPageMemPool;


/* 사용법을 출력합니다. */
void     usage();
/* 인자를 분석합니다. */
void     parseArgs( int    &aArgc, char **&aArgv );
/* Job인자의 입력값을 분석합니다. */
smuJob   parseJob( char * aStr );

/* CheckpointImage File을 명시적으로 설정했을 경우, 그 이름의 구성을 분석*/
void parseDBFileName(SChar * aDBFileName,
                     SChar * aTBSName );

/* Memory를 읽어 Hexa로 출력합니다. */
void     printMemToHexPtr( UChar * aMemPtr, UInt aSize );

/* BIT, BYTE, NIBBLE등을 Dump합니다. (MT에 Dump함수가 없습니다. */
IDE_RC   encodeBinarySelf( const mtdModule  * aModule,
                           UInt               aLength,
                           void             * aValue );
/* Column의 값을 출력합니다. */
IDE_RC printColumnValue( smOID      aTableOID,
                         UInt       aColumnID,
                         SChar    * aValue,
                         SInt       aPrintComma );
/* Column들을 모두 출력합니다. */
IDE_RC printColumns( smOID         aTableOID,
                     UInt          aColumnCount,
                     SChar       * aValue );
/* Column의 Scheme를 출력합니다. */
void printColumnScheme( void * aTableHeader );
/* TableHeader를 출력합니다. */
void printTableHeader( void    * aTableHeader,
                       idBool    aDisplaySimple,
                       SChar   * aTempBuffer,
                       UInt      aTempBufferLength );
/* PrintRecord했을때, 그 값들을 삽입하는 용도로 바로 사용될 수 없음을 알림 */
void printWarning4Record();
/* Record 하나를 Value로 Dump합니다. */
void printRecord( smcTableHeader   * aTableHeader,
                  smpSlotHeader    * aSlotHeaderPtr,
                  SChar            * sTempBuffer,
                  UInt               sTempBufferLength );

/* Dump용 PCH를 초기화하고 마무리합니다. */
void initPCH();
void finalizePCH();

/* CheckpointImageFile을 읽기 위한 DatabaseFile 객체를 할당/삭제합니다. */
IDE_RC allocDbf( scSpaceID         aSpaceID, 
                 scPageID          aPageID, 
                 smmDatabaseFile * aDB );
IDE_RC destroyDbf( smmDatabaseFile *aDB );

/* smmManager::getPersPagePtr의 Callback 함수입니다. */
IDE_RC getPersPagePtr( scSpaceID    aSpaceID, 
                       scPageID     aPageID, 
                       void      ** aPage );

/* Tablespace의 기초적인 정보에 대해 초기화합니다. */
IDE_RC initAllMemTBS();

/* Job에 따라 각종 정보를 출력합니다. */
IDE_RC dumpMemBase( smmMemBase * aMemBase);
IDE_RC dumpMemMeta( scSpaceID    aSpaceID, smmChkptImageHdr * aDfHdr );
IDE_RC dumpMemFreePageList( smmTBSNode * aTBSNode );
IDE_RC dumpMemFLI( smmTBSNode * aTBSNode );
void dumpTablespace( scSpaceID aSID );
void dumpTableRecord( smOID aOID );
void dumpTableAllocPageList( smOID aOID );
void dumpPage( scSpaceID aSID, scPageID aPID );
void dumpMemPage( scSpaceID aSID, scPageID aPID );
void dumpDiskPage( scSpaceID aSID, scPageID aPID );
void dumpIncrementalBackupMeta();

void usage()
{
    idlOS::printf( 
            "\n"
            "%-6s:  dumpdb {-j job_number } "
            "[-i pingpong_number] [-o] [-f file_name] [-s] [-p] [-d]\n"
            "  %-4s: %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "%18"ID_UINT32_FMT" : %-25s %s\n"
            "  %-4s: %s\n"
            "  %-4s: %s\n"
            "  %-4s: %s\n"
            "  %-4s: %s\n"
            "  %-4s: %s\n"
            "  %-4s: %s\n",
            "Usage",
            "-j", "specify job",
            SMUTIL_JOB_META,"META","-s -f",
            SMUTIL_JOB_TABLESPACE,"TABLESPACE","-s -f",
            SMUTIL_JOB_TABLESPACE_FLI,"TABLESPACE-FLI","-s -d",
            SMUTIL_JOB_TABLESPACE_FREEPAGE,"TABLESPACE-FREE-PAGE-LIST","-s",
            SMUTIL_JOB_TABLE,"TABLE","-o -d",
            SMUTIL_JOB_TABLE_ALLOCPAGE,"TABLE-ALLOC-PAGE-LIST","-o",
            SMUTIL_JOB_PAGE,"PAGE","-s -p -d",
            SMUTIL_JOB_INCREMENTAL_BACKUP,"INCREMENTAL_BACKUP_META","-f",
            "-i", "p[I]ngpong number(0,1)",
            "-o", "[O]id",
            "-f", "CheckpointImage [F]ile name",
            "-s", "table[S]pace id",
            "-p", "[P]age id",
            "-d", "[D]etail"
                );
    idlOS::printf( "\n" );

    idlOS::printf("example)\n"
                  "a. dumpdb -j 1\n"
                  "b. dumpdb -j 1 -s 0\n"
                  "c. dumpdb -j 2\n"
                  "d. dumpdb -j 3\n"
                  "e. dumpdb -j 4\n"
                  "f. dumpdb -j 4 -d\n"
                  "g. dumpdb -j 4 -o 65536\n"
                  "h. dumpdb -j 5 -o 65536\n"
                  "i. dumpdb -j 6 -s 0 -p 4\n"
                  "j. dumpdb -j 7 -f SYS_TBS_MEM_DATA-0-0.ibak\n");
}

smuJob   parseJob( char * aStr )
{
    static const SChar sJobList[SMUTIL_JOB_MAX][64] = {
        "META",
        "TABLESPACE",
        "TABLESPACE-FLI",
        "TABLESPACE-FREE-PAGE-LIST",
        "TABLE",
        "TABLE-ALLOC-PAGE-LIST",
        "PAGE" };
    smuJob             sRet = SMUTIL_JOB_NONE;
    UInt               i;


    if( ('0' <= aStr[0] ) && ( aStr[0] <='9' ) )
    {
        sRet = (smuJob)idlOS::atoi( optarg );
    }
    else
    {
        for( i = 0 ; i < SMUTIL_JOB_MAX ; i ++ )
        {
            if( idlOS::strncasecmp( aStr, sJobList[i], 64 ) == 0 )
            {
                sRet = (smuJob)i;
                break;
            }
            else
            {
                /* nothing to do ... */
            }
        }
    }

    return sRet;
}

void parseArgs( int &aArgc, char **&aArgv )
{
    SInt          sOpr;
    const SChar * sOpt="i:o:dj:xs:p:f:";

    while( (sOpr = idlOS::getopt(aArgc, aArgv, sOpt )) != EOF )
    {
        switch( sOpr )
        {
        case 'f':
            gTargetFN= optarg;
            break;
        case 'i':
            gPingPong = idlOS::atoi( optarg );
            break;
        case 'o':
            gOID = idlOS::atoi( optarg );
            break;
        case 's':
            gSID = idlOS::atoi( optarg );
            break;
        case 'p':
            gPID = idlOS::atoi( optarg );
            break;
        case 'd':
            gDisplaySimple   = ID_FALSE;
            break;
        case 'j':
            gJob = parseJob( optarg );
            break;
        case 'x':
            gDisplayHeader   = ID_FALSE;
            break;
        default:
            gInvalidArgs = ID_TRUE;
            break;
        }
    }

    if( gJob == SMUTIL_JOB_NONE )
    {
        gInvalidArgs = ID_TRUE;
    }

    return ;
}

void     printMemToHexPtr( UChar * aMemPtr, UInt aSize )
{
    SChar sBuffer[ SMUTIL_TEMP_BUFFER_SIZE ];
    UInt  sCurSize;
    UInt  sOffset;

    sOffset = 0;
    while( sOffset < aSize )
    {
        sCurSize = aSize - sOffset;
        if( sCurSize >= SMUTIL_TEMP_BUFFER_SIZE/4 )
        {
            sCurSize = SMUTIL_TEMP_BUFFER_SIZE/4;
        }
        ideLog::ideMemToHexStr( aMemPtr,
                                sCurSize,
                                ( IDE_DUMP_FORMAT_PIECE_4BYTE  |  \
                                  IDE_DUMP_FORMAT_ADDR_NONE | \
                                  IDE_DUMP_FORMAT_BODY_HEX |      \
                                  IDE_DUMP_FORMAT_CHAR_ASCII ),
                                sBuffer,
                                SMUTIL_TEMP_BUFFER_SIZE );
        idlOS::printf( "%s\n",sBuffer );
        sOffset += sCurSize;
    }
}

IDE_RC encodeBinarySelf( const mtdModule  * aModule,
                         UInt               aLength,
                         void             * aValue )
{
    UInt     sBitSize;
    UInt     sRemainBit;
    UChar  * sBody;
    UChar    sSrc;
    UChar    sValue;
    UInt     sMTLength;
    UInt     i;

    
    switch( aModule->id )
    {
        // Byte, Nibble 모두 Hexa형태로, 즉 16진수로 출력하기 ??문에
        // 둘다 4Bit단위로 Shift되어 출력한다.
    case MTD_NIBBLE_ID :
        sBitSize = 4;
        sMTLength = ((mtdNibbleType*)aValue)->length;
        break;
    case MTD_BYTE_ID :
    case MTD_VARBYTE_ID :
        sBitSize = 4;
        // 4bit씩 하기 때문에, 8Bit단위인 Byte Length는 *2
        sMTLength = (((mtdByteType*)aValue)->length) * 2;
        break;
    case MTD_BIT_ID :
    case MTD_VARBIT_ID :
        sBitSize = 1;
        sMTLength = ((mtdBitType*)aValue)->length;
        break;
    default:
        IDE_TEST( 1 );
        break;
    }

    sBody    = ((UChar*)aValue) + aModule->headerSize();
    aLength -= aModule->headerSize();


    idlOS::printf( "%s\'",aModule->names->string );

    /* - Nibble,Byte 
     *  X  >> 4
     *  ABCDEFGHI >> FGHI0000 >> getNextValue
     *
     * - Bit, Varbit
     *  X  >> 7
     *  ABCDEFGHI >> BCDEFGHI0 >> CDEFGHI00 .... >> I0000000 >> getNextValue */

    sSrc       = *sBody;
    sRemainBit = 8;
    for( i=0; i<sMTLength; i++)
    {
        sValue = sSrc >> ( 8 - sBitSize);
        sSrc <<= sBitSize;
        sRemainBit -= sBitSize;
        if( sRemainBit == 0 )
        {
            sBody ++;
            sSrc       = *sBody;
            sRemainBit = 8;
        }

        idlOS::printf( "%"ID_XINT32_FMT, sValue );


    }
    idlOS::printf("\'");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC printColumnValue( smOID      aTableOID,
                         UInt       aColumnID,
                         SChar    * aValue,
                         SInt       aPrintComma )
{
    smcTableHeader   * sTableHeader;
    mtcColumn        * sColumn;
    const mtdModule  * sModule;
    SChar            * sValuePtr;
    UInt               sLength;
    SChar              sBuffer[ SMUTIL_TEMP_BUFFER_SIZE ];
    UInt               sStringLength = SMUTIL_TEMP_BUFFER_SIZE;
    idBool             sIsNull = ID_FALSE;
    IDE_RC             sReturn;

    /* column 정보 및 MTD module 획득 */
    IDE_TEST( smcTable::getTableHeaderFromOID( aTableOID, (void**)&sTableHeader ) != IDE_SUCCESS);
    sColumn = (mtcColumn*)smcTable::getColumn( (void*)sTableHeader, aColumnID );
    IDE_TEST( mtd::moduleById( &sModule, sColumn->type.dataTypeId )
                               != IDE_SUCCESS );


    /* Variable Column이면 */
    if ( ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
           == SMI_COLUMN_TYPE_VARIABLE ) ||
         ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
           == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
    {
        sValuePtr = (SChar*)smiGetVarColumn( aValue,
                                             &sColumn->column,
                                             &sLength );
    }
    else
    {
        sValuePtr = aValue + sColumn->column.offset;
        sLength   = sColumn->column.size;
    }

    // CHECK NULL
    if( sValuePtr == NULL )
    {
        sIsNull = ID_TRUE;
    }
    else
    {
        if( sModule->isNull( sColumn, 
                             sValuePtr )
            == ID_TRUE )
        {
            sIsNull = ID_TRUE;
        }
        else
        {
            sIsNull = ID_FALSE;
        }
    }
    if( sIsNull == ID_TRUE )
    {
        idlOS::printf("NULL");
        IDE_CONT( NORMAL_DONE );
    }
    else
    {
        /* nothing to do ... */
    }


    /* Geo, BLOB, CLOB */
    if( ( sModule->id == MTD_GEOMETRY_ID ) ||
        ( sModule->id == MTD_BLOB_ID ) ||
        ( sModule->id == MTD_CLOB_ID ) )
    {
        idlOS::printf("[undisplayable type]");
        printMemToHexPtr( (UChar*)sValuePtr,
                          sLength );
        IDE_CONT( NORMAL_DONE );
    }
    else
    {
        /* nothing to do ... */
    }

    /* Binary Type */
    if( ( sModule->id == MTD_BIT_ID ) ||
        ( sModule->id == MTD_VARBIT_ID ) ||
        ( sModule->id == MTD_NIBBLE_ID ) ||
        ( sModule->id == MTD_BYTE_ID ) ||
        ( sModule->id == MTD_VARBYTE_ID ) )
    {
        IDE_TEST( encodeBinarySelf( (const mtdModule*) sModule,
                                    sLength,
                                    (void*)sValuePtr )
                  != IDE_SUCCESS );
        IDE_CONT( NORMAL_DONE );
    }
    else
    {
        /* nothing to do ... */
    }

    IDE_TEST( sModule->encode( NULL,
                               (void*)sValuePtr,
                               sLength,
                               (UChar*)SM_DUMP_VALUE_DATE_FMT,
                               idlOS::strlen( SM_DUMP_VALUE_DATE_FMT ),
                               (UChar*)sBuffer,
                               &sStringLength,
                               &sReturn )
              != IDE_SUCCESS );

    /* Date Type */
    if( sModule->id == MTD_DATE_ID )
    {
        idlOS::printf("TO_DATE('%s','%s')",
                      sBuffer,
                      SM_DUMP_VALUE_DATE_FMT);
    }
    else
    {
        idlOS::printf("'%s'",sBuffer );
    }

    IDE_EXCEPTION_CONT( NORMAL_DONE );

    if( aPrintComma == ID_TRUE )
    {
        printf(",");
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC printColumns( smOID         aTableOID,
                     UInt          aColumnCount,
                     SChar       * aValue )
{
    UInt i;

    IDE_TEST_CONT( aColumnCount == 0, SKIP );

    idlOS::printf( "(" );
    for( i = 0 ; i < aColumnCount ; i ++ )
    {
        IDE_TEST( printColumnValue( aTableOID,
                                    i,  /* Column ID */
                                    aValue,
                                    ( i < aColumnCount - 1 ) )/*print comma?*/
                  != IDE_SUCCESS );
    }
    idlOS::printf(")\n");

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void printColumnScheme( void * aTableHeader )
{
    mtcColumn        * sColumn;
    const mtdModule  * sModule;
    UInt               i;

    idlOS::printf("Column:\n\n");
    for( i = 0 ; 
         i < smcTable::getColumnCount( aTableHeader ) ; 
         i ++)
    {
        /* Module 및 Column 획득 */
        sColumn = (mtcColumn*)smcTable::getColumn( 
            aTableHeader, i );
        IDE_ASSERT( mtd::moduleById( &sModule, 
                                     sColumn->type.dataTypeId )
                    == IDE_SUCCESS );

        /* Column의 이름 출력 */
        idlOS::printf( "%s",
                       sModule->names->string );

        /* Precision, scale에 대한 정보를 분류하여 출력. */
        switch ( sModule->flag & MTD_CREATE_PARAM_MASK )
        {
        case MTD_CREATE_PARAM_NONE:                  /* ex) Integer */
            //nothing to do ...
            break;
        case MTD_CREATE_PARAM_PRECISION:             /* ex) char */
            idlOS::printf( "(%"ID_UINT32_FMT")",
                           sColumn->precision );
            break;
        case MTD_CREATE_PARAM_PRECISIONSCALE:        /* ex) Numeric */
            idlOS::printf( "(%"ID_UINT32_FMT",%"ID_UINT32_FMT")",
                           sColumn->precision,
                           sColumn->scale );
            break;
        }

        if( gDisplaySimple == ID_FALSE )
        {
            idlOS::printf( "\n"
                           "id              : %"ID_UINT32_FMT"\n"
                           "flag            : %"ID_UINT32_FMT"\n"
                           "offset          : %"ID_UINT32_FMT"\n"
                           "vcInOutBaseSize : %"ID_UINT32_FMT"\n"
                           "size            : %"ID_UINT32_FMT"\n"
                           "colSpace        : %"ID_UINT32_FMT"\n"
                           "colSeg.mSpaceID : %"ID_UINT32_FMT"\n"
                           "colSeg.mPageID  : %"ID_UINT32_FMT"\n"
                           "colSeg.mOffset  : %"ID_UINT32_FMT"\n\n",
                           sColumn->column.id,
                           sColumn->column.flag,
                           sColumn->column.offset,
                           sColumn->column.vcInOutBaseSize,
                           sColumn->column.size,
                           sColumn->column.colSpace,
                           sColumn->column.colSeg.mSpaceID,
                           sColumn->column.colSeg.mPageID,
                           sColumn->column.colSeg.mOffset );
        }
        else
        {
            /* 마지막 칼럼이 아니면 , 로 구분함 */
            if( i < smcTable::getColumnCount( aTableHeader ) - 1 )
            {
                idlOS::printf(",");
            }
            else
            {
                idlOS::printf("\n");
            }
        }
    }
}

void printTableHeader( void    * aTableHeader,
                       idBool    aDisplaySimple,
                       SChar   * aTempBuffer,
                       UInt      aTempBufferLength )
{
    smpSlotHeader  * sSlotHeaderPtr;
    smnIndexHeader * sIndexHeader;
    UInt             i;

    /* TableHeader를 바탕으로 TableHeader의 SlotHeader를 찾는다. */
    sSlotHeaderPtr = (smpSlotHeader*)
        (((UChar*)aTableHeader) - SMP_SLOT_HEADER_SIZE);

    aTempBuffer[0] = '\0';
    smcTable::dumpTableHeaderByBuffer( (smcTableHeader*)aTableHeader,
                                       ID_FALSE,        // dump binary
                                       aDisplaySimple,  // display table
                                       aTempBuffer,
                                       aTempBufferLength );
    idlOS::printf( "%s\n", aTempBuffer);

    /* Table이고, 상세히 출력해야 한다면 */
    if( ( aDisplaySimple == ID_FALSE ) &&
        ( smcTable::getTableType( sSlotHeaderPtr ) == 
          SMC_TABLE_NORMAL ) )
    {
        idlOS::printf("\n----------------------------------------"
                      "----------------------------------------\n" );

        /* display column scheme */
        printColumnScheme( aTableHeader );
        idlOS::printf("\n"
                      "----------------------------------------"
                      "----------------------------------------\n");

        /* display index scheme */
        for( i = 0 ; 
             i < smcTable::getIndexCount( aTableHeader ) ; 
             i ++)
        {
            sIndexHeader = (smnIndexHeader*) smcTable::getTableIndex(
                aTableHeader, i );
            IDE_ASSERT( smnManager::dumpCommonHeader( 
                            sIndexHeader,
                            aTempBuffer,
                            aTempBufferLength )
                        == IDE_SUCCESS );
            idlOS::printf("%s"
                          "----------------------------------------"
                          "----------------------------------------\n",
                          aTempBuffer);
        }
        idlOS::printf("========================================"
                      "========================================\n" );
    }
}

void printWarning4Record()
{
    idlOS::printf("Display Record:\n" );
    idlOS::printf( 
        "\n!!Warning!! The following output cannot be used with iSQL "
        "in its current form. (Please check and change one or more "
        "column types as appropriate.)\n" );
    idlOS::printf("\n========================================"
                  "========================================\n" );
}

void printRecord( smcTableHeader   * aTableHeader,
                  smpSlotHeader    * aSlotHeaderPtr,
                  SChar            * aTempBuffer,
                  UInt               aTempBufferLength )
{
    SChar            * sSlotValue;
    smSCN              sSCN;

    sSCN = aSlotHeaderPtr->mCreateSCN;

    /* 지워진 Value는 Dump하지 않음 */
    if( ( ( SMP_SLOT_IS_DROP( aSlotHeaderPtr ) ) ||
          ( SM_SCN_IS_DELETED( sSCN ) == ID_TRUE ) ) &&
        ( gDisplaySimple == ID_TRUE ) )
    {
        return;
    }
    else
    {
        /* nothing do to ... */
    }

    aTempBuffer[0] = '\0';

    /* Catalog Table일 경우, Record가 일반 Record가 아닌 tableheader이다. */
    if( aTableHeader == smmManager::m_catTableHeader )
    {
        if( gDisplaySimple == ID_TRUE )
        {
            smpFixedPageList::dumpSlotHeaderByBuffer( aSlotHeaderPtr,
                                                      ID_TRUE, // display table
                                                      aTempBuffer,
                                                      aTempBufferLength );
            idlOS::printf("%s",aTempBuffer);
        }
        else
        {
            /* nothing do to ... */
        }

        aTempBuffer[0] = '\0';
        sSlotValue     = (SChar*)(aSlotHeaderPtr + 1);
        smpFixedPageList::dumpSlotHeaderByBuffer( aSlotHeaderPtr,
                                                  gDisplaySimple,  
                                                  aTempBuffer,
                                                  aTempBufferLength );
        printTableHeader( (void*)sSlotValue,
                          gDisplaySimple,
                          aTempBuffer,
                          aTempBufferLength );
    }
    else
    {
        /* 그냥 Record를 출력한다 */
        if( gDisplaySimple == ID_FALSE )
        {
            smpFixedPageList::dumpSlotHeaderByBuffer( aSlotHeaderPtr,
                                                      ID_TRUE, // display table
                                                      aTempBuffer,
                                                      aTempBufferLength );
            idlOS::printf("%s\n",aTempBuffer);
        }
        else
        {
            /* nothing do to ... */
        }

        printColumns( aTableHeader->mSelfOID,
                      aTableHeader->mColumnCount,
                      (SChar*)aSlotHeaderPtr );
    }
}

void initPCH()
{
    IDE_ASSERT( gPageMemPool.initialize(
                    IDU_MEM_SM_SMU,
                    (SChar*)"TEMP_MEMORY_POOL",
                    1, /* MemList Count */
                    SM_PAGE_SIZE,
                    smuProperty::getTempPageChunkCount(),
                    IDU_AUTOFREE_CHUNK_LIMIT,           /* ChunkLimit */
                    ID_TRUE,                            /* UseMutex */
                    IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,    /* AlignByte */
                    ID_FALSE,                           /* ForcePooling */
                    ID_TRUE,                            /* GarbageCollection */
                    ID_TRUE,                            /* HWCacheLine */
                    IDU_MEMPOOL_TYPE_LEGACY             /* mempool type */)
                 == IDE_SUCCESS);
}     

void finalizePCH()
{
    UInt i;
    UInt j;

    for( i = 0 ; i < SC_MAX_SPACE_COUNT ; i ++ )
    {
        if( gPCH[ i ] != NULL )
        {
            for( j = 0 ; 
                 j < smuProperty::getMaxDBSize() / SM_PAGE_SIZE ; 
                 j ++ )
            {
                if( gPCH[ i ][ j ] != NULL )
                {
                    gPageMemPool.memfree( gPCH[ i ][ j ] );
                }
                else
                {
                    /* nothing do to ... */
                }
            }
            IDE_ASSERT( iduMemMgr::free( gPCH[ i ] ) == IDE_SUCCESS);
        }
        else
        {
            /* nothing do to ... */
        }
    }
    IDE_ASSERT( gPageMemPool.destroy() == IDE_SUCCESS);
}

IDE_RC allocDbf( scSpaceID         aSpaceID, 
                 scPageID          aPageID, 
                 smmDatabaseFile * aDB )
{
    smmTBSNode      * sTBSNode;
    SChar             sDBFileName[SM_MAX_FILE_NAME];
    SChar           * sDBFileDir;
    UInt              sFileNo;
    idBool            sFound;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sTBSNode ) != IDE_SUCCESS );

    if ( aPageID != SM_SPECIAL_PID )
    {
	    /* PID를 바탕으로, Split된 파일 중 몇번째 파일을 읽을지 선택 */
	    sFileNo = smmManager::getDbFileNo( sTBSNode, aPageID );
    }
    else
    {
	    sFileNo = 0;
    }

    IDE_TEST( aDB->initialize( aSpaceID,
                               gPingPong,
                               sFileNo ) != IDE_SUCCESS );
    if( gTargetFN == NULL )
    {
        sFound = smmDatabaseFile::findDBFile( sTBSNode,
                                              gPingPong,
                                              sFileNo,
                                              (SChar*)sDBFileName,
                                              &sDBFileDir);
        IDE_ASSERT( sFound == ID_TRUE );

        aDB->setDir(sDBFileDir);
    }
    else
    {
        /* 특정 파일을 강제로 설정했을 경우, 그 파일로 이름 설정함 */
        idlOS::memset( sDBFileName, 0, ID_SIZEOF(sDBFileName) );
	/* BUG-39169 [sm-util] The result of dumpdb and dumpla is different
	 * with checkpoint image files's create SCN
	 * 지정된 파일 이름을 sTBSNode, gPingPong, sFileNo로 복원하려 하였으나
	 * Database File Header Meta 값만을 읽어오는 루트에서
	 * aPageID 값을 0으로 호출하여 파일 이름을 복원하지 못하는 문제가 발생함
	 * 해당 루트에서 PageID를 SM_SPECIAL_PID를 사용하여 호출하여
	 * gTargetFN을 이용하여 파일이름을 복원하지 않고 그냥 사용하도록 함
	 * 그 외 루틴에서는 기존 루트를 타도록 함 */
	if( aPageID != SM_SPECIAL_PID ) {
		idlOS::snprintf( sDBFileName,
				SM_MAX_FILE_NAME,
				SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG_FILENUM,
				gTBSName,
				gPingPong,
				sFileNo );
	}
	else
	{
		idlOS::snprintf( sDBFileName,
				SM_MAX_FILE_NAME,
				"%s",
				gTargetFN );
	}

        aDB->setDir( (SChar*)"." );
    }

    aDB->setFileName(sDBFileName);

    IDE_TEST( aDB->open() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC destroyDbf( smmDatabaseFile * aDB)
{
    IDE_TEST( aDB->destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC getPersPagePtr( scSpaceID    aSpaceID, 
                       scPageID     aPageID, 
                       void      ** aPage )
{
    smmDatabaseFile   sDb;
    smmTBSNode      * sTBSNode;

    /* 최초의 PCH Array 생성 */
    if( gPCH[ aSpaceID ] == NULL )
    {
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                                     smuProperty::getMaxDBSize() 
                                        / SM_PAGE_SIZE,
                                     ID_SIZEOF(smmPCH *),
                                     (void**)&gPCH[ aSpaceID ] ) 
                  != IDE_SUCCESS );

    }
    else
    {
        /* nothing to do ... */
    }

    /* 최초의 해당 Page read */
    if( gPCH[ aSpaceID ][ aPageID ] == NULL )
    {
        IDE_TEST( gPageMemPool.alloc( (void**)&(gPCH[ aSpaceID ][ aPageID ]) )
                  != IDE_SUCCESS );

        IDE_TEST( allocDbf( aSpaceID, aPageID, &sDb ) != IDE_SUCCESS );


        IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                            (void**)&sTBSNode )
                  != IDE_SUCCESS );
        IDE_TEST( sDb.readPageWithoutCheck( sTBSNode,
                                            aPageID,
                                            (UChar*)
                                            gPCH[aSpaceID][aPageID] )
                  != IDE_SUCCESS );

        IDE_TEST( destroyDbf( &sDb ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    (*aPage) = gPCH[ aSpaceID ][ aPageID ];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC dumpMemBase(smmMemBase * aMemBase)
{
    UInt i;

    idlOS::printf( "[BEGIN MEMBASE]\n"
                   "mDBname                : %s\n"
                   "mProductSignature      : %s\n"
                   "mDBFileSignature       : %s\n"
                   "mVersionID             : 0x%"ID_XINT32_FMT"\n"
                   "mCompileBit            : %"ID_UINT32_FMT"\n"
                   "mBigEndian             : %"ID_UINT32_FMT"\n"
                   "mLogSize               : %"ID_vULONG_FMT"\n"
                   "mDBFilePageCount       : %"ID_vULONG_FMT"\n"
                   "mTxTBLSize             : %"ID_UINT32_FMT"\n"
                   "mDBFileCount[0]        : %"ID_UINT32_FMT"\n"
                   "mDBFileCount[1]        : %"ID_UINT32_FMT"\n"
                   "mDBCharSet             : %s\n"
                   "mNationalCharSet       : %s\n"
                   "mTimestamp             : %"ID_UINT64_FMT"\n"
                   "mAllocPersPageCount    : %"ID_vULONG_FMT"\n"
                   "mExpandChunkPageCnt    : %"ID_vULONG_FMT"\n"
                   "mCurrentExpandChunkCnt : %"ID_vULONG_FMT"\n"
                   "mSystemSCN             : %"ID_UINT64_FMT"\n"
                   "mFreePageListCount     : %"ID_UINT32_FMT"\n",
                   aMemBase->mDBname,
                   aMemBase->mProductSignature,
                   aMemBase->mDBFileSignature,
                   aMemBase->mVersionID,
                   aMemBase->mCompileBit,
                   aMemBase->mBigEndian,
                   aMemBase->mLogSize,
                   aMemBase->mDBFilePageCount,
                   aMemBase->mTxTBLSize,
                   aMemBase->mDBFileCount[0],
                   aMemBase->mDBFileCount[1],
                   aMemBase->mDBCharSet,
                   aMemBase->mNationalCharSet,
                   aMemBase->mTimestamp,
                   aMemBase->mAllocPersPageCount,
                   aMemBase->mExpandChunkPageCnt,
                   aMemBase->mCurrentExpandChunkCnt,
                   SM_SCN_TO_LONG( aMemBase->mSystemSCN ),
                   aMemBase->mFreePageListCount );

    // 각각의 Free Page List의 정보를 출력한다.
    for ( i=0; i< aMemBase->mFreePageListCount; i++ )
    {
        idlOS::printf( "     [%02"ID_UINT32_FMT"] "
                       "FreePageCount: %"ID_vULONG_FMT" "
                       "ID: %"ID_vULONG_FMT"\n",
                       i,
                       aMemBase->mFreePageLists[i].mFreePageCount,
                       aMemBase->mFreePageLists[i].mFirstFreePageID );
    }
    idlOS::printf( "[END MEMBASE]\n\n" );

    return IDE_SUCCESS;

}

IDE_RC dumpMemFreePageList( smmTBSNode * aTBSNode )
{
    smmMemBase  * sMemBase;
    scPageID      sPID       = SC_NULL_PID;
    UInt          sPageCount = 0;
    scPageID      sNextPID   = 0;
    UInt          sPageSeq   = 0;
    const SChar * sMsg = "";
    idBool        sIsValid   = ID_TRUE ;
    UInt          i;

    sMemBase = aTBSNode->mMemBase;

    idlOS::printf( "[DUMP FREE PAGE LISTS]\n"
                   "FREE PAGE LIST COUNT: %"ID_UINT32_FMT"\n",
                   sMemBase->mFreePageListCount );

    for( i = 0 ; i < sMemBase->mFreePageListCount ; i ++ )
    {
        sPID       = sMemBase->mFreePageLists[i].mFirstFreePageID;
        sPageCount = sMemBase->mFreePageLists[i].mFreePageCount;

        // print title
        idlOS::printf( "FREE PAGE LIST #%02"ID_UINT32_FMT"\n"
                       "  - First PID: %"ID_UINT32_FMT"\n"
                       "  - Page Count: %"ID_vULONG_FMT"\n"
                       "  - PID List:\n",
                       i,
                       sPID,
                       sPageCount );

        // Seq | PID
        idlOS::printf( "    %7s  %11s\n", "Seq", "PID" );

        for( sPageSeq = 0 ; 
             sPID != SM_NULL_PID ; 
             sPageSeq ++, sPID = sNextPID )
        {
            idlOS::printf( "    %7"ID_vULONG_FMT"  %11"ID_UINT32_FMT"\n",
                           sPageSeq, sPID );

            if( smmExpandChunk::isDataPageID( aTBSNode, sPID ) != ID_TRUE )
            {
                sMsg = "This page isn't data page.";
                sIsValid = ID_FALSE;
                break;
            }

            IDE_TEST( smmExpandChunk::getNextFreePage( aTBSNode, 
                                                       sPID, 
                                                       &sNextPID) 
                      != IDE_SUCCESS );

            if( sNextPID == SMM_FLI_ALLOCATED_PID )
            {
                sMsg = "This page is already allocated.";
                sIsValid = ID_FALSE;
                break;
            }

            sPID = sNextPID;

        }
    }

    if( (sIsValid == ID_TRUE) && (sPageSeq != sPageCount) )
    {
        sMsg = "There is a mismatch in free page count.";
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do ... */
    }

    if( sIsValid != ID_TRUE )
    {
        idlOS::printf( "========================================\n" );
        idlOS::printf( "FREE PAGE LIST IS INVALID: %s\n", sMsg );
        idlOS::printf( "========================================\n" );
        idlOS::printf( "* Page Count: %"ID_vULONG_FMT"\n"
                       "* PID:        %"ID_UINT32_FMT"\n"
                       "* Next PID:   %"ID_UINT32_FMT"\n",
                       sPageCount,
                       sPID,
                       sNextPID );
        idlOS::printf( "========================================\n" );
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dumpMemFLI( smmTBSNode * aTBSNode )
{
    scPageID      sFstDataPID;
    scPageID      sFstChunkPID;
    scPageID      sFLIPageID;
    SChar       * sFLIPageAddr;
    UInt          sFLIPageCnt;
    UInt          sFLISlotCnt;
    smmFLISlot  * sSlotBase;
    smmFLISlot  * sSlotAddr;
    vULong        sChunkPageCnt;
    UInt          sChunkNo;
    UInt          sSlotSeq;
    UInt          sSlotMax;
    UInt          i;
    UInt          j;

    sChunkPageCnt = aTBSNode->mChunkPageCnt;
    
    sFLIPageCnt   = smmExpandChunk::getChunkFLIPageCnt(aTBSNode);
    sFLISlotCnt   = smmExpandChunk::getFLISlotCount();

    if( gDisplaySimple == ID_TRUE )
    {
        sSlotMax = sChunkPageCnt - sFLIPageCnt;
    }
    else
    {
        sSlotMax = sFLISlotCnt;
    }

    idlOS::printf( "[DUMP FLI PAGES]\n"
                   "SMM_DATABASE_META_PAGE_CNT:      %"ID_vULONG_FMT"\n"
                   "CURRENT_EXPAND_CHUNK_PAGE_COUNT: %"ID_vULONG_FMT"\n"
                   "CHUNK_PAGE_COUNT:                %"ID_vULONG_FMT"\n"
                   "FLI_PAGE_COUNT:                  %"ID_UINT32_FMT"\n"
                   "FLI_SLOT_COUNT:                  %"ID_UINT32_FMT"\n",
                   SMM_DATABASE_META_PAGE_CNT,
                   aTBSNode->mMemBase->mCurrentExpandChunkCnt,
                   sChunkPageCnt,
                   sFLIPageCnt,
                   sFLISlotCnt );

    for( sChunkNo = 0 ; 
         sChunkNo < aTBSNode->mMemBase->mCurrentExpandChunkCnt ;
         sChunkNo++ )
    {
    // print title
        idlOS::printf( "CHUNK NO #%02"ID_vULONG_FMT"\n", sChunkNo );
        idlOS::printf( "  - FLIPage Count: %"ID_UINT32_FMT"\n", sFLIPageCnt );

        for( i = 0; i < sFLIPageCnt; i++ )
        {
            sFstChunkPID  = (sChunkNo * sChunkPageCnt)
                + SMM_DATABASE_META_PAGE_CNT;
            sFstDataPID   = sFstChunkPID + sFLIPageCnt;

            sFLIPageID = sFstChunkPID + i;
            idlOS::printf( "  - FLIPage #%02"ID_UINT32_FMT""
                           "(PID: %"ID_UINT32_FMT")\n",
                           i,
                           sFLIPageID );

            IDE_TEST( getPersPagePtr( aTBSNode->mHeader.mID,
                                      sFLIPageID,
                                      (void**)&sFLIPageAddr )
                      != IDE_SUCCESS );

            sSlotBase = (smmFLISlot *)
                ( sFLIPageAddr + aTBSNode->mFLISlotBase );

            // SlotNum | PID | NextFreePID | Used
            idlOS::printf( "    %7s  %11s  %11s  %4s\n", 
                           "SlotNum", "PID", "NextFreePID", "ValidSlot" );
            for( j = 0; j < sSlotMax; j++ )
            {
                sSlotAddr = sSlotBase + j;
                sSlotSeq  = ( i * sFLISlotCnt ) + j ;

                idlOS::printf("    %7"ID_UINT32_FMT"  "
                              "%11"ID_UINT32_FMT"  "
                              "%11"ID_UINT32_FMT"  %4s\n",
                              j,
                              sFstDataPID + sSlotSeq,
                              sSlotAddr->mNextFreePageID,
                              (sFLIPageCnt + sSlotSeq) < 
                                sChunkPageCnt ? "Y" : "N" );
            }
        }
    }

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC dumpMemMeta( scSpaceID    aSpaceID, smmChkptImageHdr * aDfHdr )
{
    smmDatabaseFile    sDB;
    smmChkptImageHdr   sDfHdr;
    UInt               sBackupType;
    time_t             sTime;
    struct tm          sBackupBegin;
    struct tm          sBackupEnd;

    idlOS::memset( &sBackupBegin, 0 , ID_SIZEOF( struct tm ) );
    idlOS::memset( &sBackupEnd, 0 , ID_SIZEOF( struct tm ) );

    if( aDfHdr == NULL )
    {
        IDE_TEST( allocDbf( aSpaceID,
                            SM_SPECIAL_PID, // PID ( load specific databasefile )
                            &sDB )
                  != IDE_SUCCESS );

        IDE_TEST( sDB.readChkptImageHdr( &sDfHdr ) != IDE_SUCCESS );

        IDE_TEST( destroyDbf( &sDB ) != IDE_SUCCESS );
    }
    else
    {
        idlOS::memcpy( &sDfHdr, aDfHdr, ID_SIZEOF( smmChkptImageHdr ));
    }

    idlOS::printf("[BEGIN CHECKPOINT IMAGE HEADER]\n"
                  "Binary DB Version             "
                  "[ %"ID_xINT32_FMT".%"ID_xINT32_FMT".%"ID_xINT32_FMT" ]\n",
                   ((sDfHdr.mSmVersion & SM_MAJOR_VERSION_MASK) >> 24),
                   ((sDfHdr.mSmVersion & SM_MINOR_VERSION_MASK) >> 16),
                   (sDfHdr.mSmVersion  & SM_PATCH_VERSION_MASK) );

    idlOS::printf("Redo LSN       [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                  sDfHdr.mMemRedoLSN.mFileNo,
                  sDfHdr.mMemRedoLSN.mOffset);

    idlOS::printf("Create LSN     [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                  sDfHdr.mMemCreateLSN.mFileNo,
                  sDfHdr.mMemCreateLSN.mOffset);

    /* PROJ-2133 incremental backup */
    idlOS::printf("DataFileDescSlot ID"
                  "           [ %"ID_UINT32_FMT
                  ", %"ID_UINT32_FMT" ]\n",
                  sDfHdr.mDataFileDescSlotID.mBlockID,
                  sDfHdr.mDataFileDescSlotID.mSlotIdx);

    idlOS::printf( "\n  [BEGIN BACKUPFILE INFORMATION]\n\n" );

    sTime =  (time_t)sDfHdr.mBackupInfo.mBeginBackupTime;
    idlOS::localtime_r( (time_t *)&sTime, &sBackupBegin );
    idlOS::printf( "        Begin Backup Time             [ %04"ID_UINT32_FMT"_%02"ID_UINT32_FMT"_%02"ID_UINT32_FMT"",
                   sBackupBegin.tm_year + 1900,
                   sBackupBegin.tm_mon  + 1,
                   sBackupBegin.tm_mday );
    idlOS::printf( " %02"ID_UINT32_FMT":%02"ID_UINT32_FMT":%02"ID_UINT32_FMT" ]\n",
                   sBackupBegin.tm_hour,
                   sBackupBegin.tm_min,
                   sBackupBegin.tm_sec );

    sTime =  (time_t)sDfHdr.mBackupInfo.mEndBackupTime;
    idlOS::localtime_r( (time_t *)&sTime, &sBackupEnd );
    idlOS::printf( "        End Backup Time               [ %04"ID_UINT32_FMT"_%02"ID_UINT32_FMT"_%02"ID_UINT32_FMT"",
                   sBackupEnd.tm_year + 1900,
                   sBackupEnd.tm_mon  + 1,
                   sBackupEnd.tm_mday );
    idlOS::printf( " %02"ID_UINT32_FMT":%02"ID_UINT32_FMT":%02"ID_UINT32_FMT" ]\n",
                   sBackupEnd.tm_hour,
                   sBackupEnd.tm_min,
                   sBackupEnd.tm_sec );

    idlOS::printf( "        IBChunk Count                 [ %"ID_UINT32_FMT" ]\n",
                   sDfHdr.mBackupInfo.mIBChunkCNT );

    switch( sDfHdr.mBackupInfo.mBackupTarget )
    {
        case SMRI_BI_BACKUP_TARGET_NONE:
            idlOS::printf( "        Backup Target                 [ NONE ]\n" );
            break;
        case SMRI_BI_BACKUP_TARGET_DATABASE:
            idlOS::printf( "        Backup Target                 [ DATABASE ]\n" );
            break;
        case SMRI_BI_BACKUP_TARGET_TABLESPACE:
            idlOS::printf( "        Backup Target                 [ TABLESPACE ]\n" );
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    switch( sDfHdr.mBackupInfo.mBackupLevel )
    {
        case SMI_BACKUP_LEVEL_0:
            idlOS::printf( "        Backup Level                  [ LEVEL0 ]\n" );
            break;
        case SMI_BACKUP_LEVEL_1:
            idlOS::printf( "        Backup Level                  [ LEVEL1 ]\n" );
            break;
        case SMI_BACKUP_LEVEL_NONE:
            idlOS::printf( "        Backup Level                  [ NONE ]\n" );
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    sBackupType = sDfHdr.mBackupInfo.mBackupType;
    idlOS::printf( "        Backup Type                   [ " );
    if( (sBackupType & SMI_BACKUP_TYPE_FULL) != 0 )   
    {
        idlOS::printf( "FULL" );
        sBackupType &= ~SMI_BACKUP_TYPE_FULL;
        if( sBackupType != 0 )
        {
            idlOS::printf( ", " );
        }
        
    }
    if( (sBackupType & SMI_BACKUP_TYPE_DIFFERENTIAL) != 0 )   
    {
        idlOS::printf( "DIFFERENTIAL" );
    }
    if( (sBackupType & SMI_BACKUP_TYPE_CUMULATIVE) != 0 )   
    {
        idlOS::printf( "CUMULATIVE" );
    }
    idlOS::printf( " ]\n" );

    idlOS::printf( "        TableSpace ID                 [ %"ID_UINT32_FMT" ]\n",
                   sDfHdr.mBackupInfo.mSpaceID );

    idlOS::printf( "        File ID                       [ %"ID_UINT32_FMT" ]\n",
                   sDfHdr.mBackupInfo.mFileID );

    idlOS::printf( "        Backup Tag Name               [ %s ]\n",
                   sDfHdr.mBackupInfo.mBackupTag );

    idlOS::printf( "        Backup File Name              [ %s ]\n",
                   sDfHdr.mBackupInfo.mBackupFileName );

    idlOS::printf( "\n  [END BACKUPFILE INFORMATION]\n" );

    idlOS::printf("[END CHECKPOINT IMAGE HEADER]\n\n");


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dumpTableRecord( smOID aOID )
{
    smpSlotHeader    * sSlotHeaderPtr;
    smcTableHeader   * sTableHeader;
    SChar            * sCurPtr;
    SChar            * sNxtPtr;
    SChar            * sTempBuffer;

    /* Allocation memory ( dump buffer ) */
    IDE_ASSERT( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                                 1,
                                 IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuffer ) 
              == IDE_SUCCESS );

    /* get Target Table */
    if( aOID != SMUTIL_ALL_TABLEOID )
    {
        IDE_ASSERT( smcTable::getTableHeaderFromOID( 
                aOID, (void**)&sTableHeader ) == IDE_SUCCESS );
    }
    else
    {
        /* Dump Catalog */
        sTableHeader = (smcTableHeader *)smmManager::m_catTableHeader;
    }
    sCurPtr = NULL;

    /* display scheme */
    if( sTableHeader == smmManager::m_catTableHeader ) 
    {
        /* Catalog면, Record들이 Tableheader이다. */
        if( gDisplaySimple == ID_TRUE )
        {
            idlOS::printf(
"SCN                 Next                PID       Offset    "
"TID          Used  " );
            idlOS::printf(
"Type       Flag         TableType     ObjType  "
"SelfOID     NullOID     ColSize ColCnt  SID MaxRow");
        }
        else
        {
            /* nothing to do ... */
        }
    }
    else
    {
        printTableHeader( sTableHeader,
                          ID_FALSE, /* display simple */
                          sTempBuffer, 
                          IDE_DUMP_DEST_LIMIT );

        if( gDisplaySimple == ID_FALSE )
        {
            idlOS::printf("\n");
            idlOS::printf(
"SCN                 Next                PID       Offset    "
"TID          Used  " );
        }
        else
        {
            /* nothing to do ... */
        }
    }
    printWarning4Record();
    while(1)
    {
        IDE_ASSERT( smcRecord::nextOIDall( sTableHeader,
                                           sCurPtr,
                                           & sNxtPtr )
                    == IDE_SUCCESS );
        if( sNxtPtr == NULL )
        {
            break;
        }
        else
        {
            sSlotHeaderPtr = (smpSlotHeader *)sNxtPtr;
            sCurPtr = sNxtPtr;

            printRecord( sTableHeader, 
                         sSlotHeaderPtr, 
                         sTempBuffer, 
                         IDE_DUMP_DEST_LIMIT );
        }
    }

    IDE_ASSERT( iduMemMgr::free( sTempBuffer ) == IDE_SUCCESS );
}

void dumpTableAllocPageList( smOID aOID )
{
    smcTableHeader    * sTH;
    scPageID            sPageID;
    smpPersPageHeader * sPersPageHeaderPtr;
    UInt                i;

    idlOS::printf( "# OID : %"ID_vULONG_FMT"\n", aOID );

    /* get Target Table */
    IDE_TEST_RAISE( smcTable::getTableHeaderFromOID( 
            aOID, (void**)&sTH ) != IDE_SUCCESS,
        invalid_argument );
    
    IDE_TEST_RAISE( ( sTH->mType != SMC_TABLE_NORMAL ) ||
                    ( ( sTH->mFlag != SMI_TABLE_META ) &&
                      ( sTH->mFlag != SMI_TABLE_MEMORY ) ),
                    invalid_argument );

    IDE_TEST_RAISE( smcTable::getTableOID( sTH ) != aOID,
                    invalid_argument );

    for( i = 0; i < SMP_PAGE_LIST_COUNT; i++ )
    {
        if( sTH->mFixedAllocList.mMRDB[i].mPageCount > 0 )
        {
            idlOS::printf(
                "[Fixed Alloc Page List %3"ID_UINT32_FMT"]\t"
                "SlotSize:%"ID_UINT32_FMT" SlotCount:%"ID_UINT32_FMT"\n",
                i,
                sTH->mFixed.mMRDB.mSlotSize,
                sTH->mFixed.mMRDB.mSlotCount );
            idlOS::printf( "%-7s %-7s %-7s %-7s %-7s %-7s\n",
                           "SELF",
                           "TYPE",
                           "PREV",
                           "NEXT",
                           "LISTID",
                           "TBLOID" );
            sPageID = smpAllocPageList::getFirstAllocPageID(
                              &sTH->mFixedAllocList.mMRDB[i] );
            while( sPageID != SM_NULL_PID )
            {
                IDE_ASSERT( getPersPagePtr( 
                                smcTable::getTableSpaceID( (void*)sTH ),
                                sPageID,
                                (void**)&sPersPageHeaderPtr )
                            == IDE_SUCCESS );

                idlOS::printf( "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT"\n",
                               (sPersPageHeaderPtr)->mSelfPageID,
                               (sPersPageHeaderPtr)->mType,
                               (sPersPageHeaderPtr)->mPrevPageID,
                               (sPersPageHeaderPtr)->mNextPageID,
                               (sPersPageHeaderPtr)->mAllocListID,
                               (sPersPageHeaderPtr)->mTableOID );

                sPageID = (sPersPageHeaderPtr)->mNextPageID;
            }
        }
    }
    for( i = 0; i < SMP_PAGE_LIST_COUNT; i++ )
    {
        if( sTH->mVarAllocList.mMRDB[i].mPageCount > 0 )
        {
            idlOS::printf("[Variable Alloc Page List %3"ID_UINT32_FMT"]\n",i);
            idlOS::printf( "%-7s %-7s %-7s %-7s %-7s %-7s\n",
                           "SELF",
                           "TYPE",
                           "PREV",
                           "NEXT",
                           "LISTID",
                           "TBLOID" );
            sPageID = smpAllocPageList::getFirstAllocPageID(
                              &sTH->mVarAllocList.mMRDB[i] );
            while( sPageID != SM_NULL_PID )
            {
                IDE_ASSERT( getPersPagePtr( 
                                smcTable::getTableSpaceID( (void*)sTH ),
                                sPageID,
                                (void**)&sPersPageHeaderPtr )
                            == IDE_SUCCESS );

                idlOS::printf( "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT" "
                               "%-7"ID_UINT32_FMT"\n",
                               (sPersPageHeaderPtr)->mSelfPageID,
                               (sPersPageHeaderPtr)->mType,
                               (sPersPageHeaderPtr)->mPrevPageID,
                               (sPersPageHeaderPtr)->mNextPageID,
                               (sPersPageHeaderPtr)->mAllocListID,
                               (sPersPageHeaderPtr)->mTableOID );

                sPageID = (sPersPageHeaderPtr)->mNextPageID;
            }
        }

    }

    return;

    IDE_EXCEPTION( invalid_argument )
    {
        (void)usage();
    }

    IDE_EXCEPTION_END;

    return;
}

void dumpMemPage( scSpaceID aSID, scPageID aPID )
{
    smpPersPage      * sPagePtr;
    SChar            * sTempBuf;
    smOID              sTableOID;
    smcTableHeader   * sTableHeader;
    smpSlotHeader    * sSlot;
    UInt               sFixedSlotSize;
    UInt               i;

    IDE_ASSERT( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                   ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                   (void**)&sTempBuf )
                == IDE_SUCCESS );

    IDE_ASSERT( smmManager::getPersPagePtr( aSID,
                                            aPID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    /* Fix도 Var도 아니면, Unformatted page */
    if( ( SMP_GET_PERS_PAGE_TYPE( sPagePtr ) != SMP_PAGETYPE_FIX ) &&
        ( SMP_GET_PERS_PAGE_TYPE( sPagePtr ) != SMP_PAGETYPE_VAR ) )
    {
        ideLog::ideMemToHexStr( (UChar*)sPagePtr,
                                SM_PAGE_SIZE,
                                IDE_DUMP_FORMAT_NORMAL,
                                sTempBuf,
                                IDE_DUMP_DEST_LIMIT );

        idlOS::printf( "Unformated page\n%s\n", sTempBuf );
    }
    else
    {
        sTableOID = sPagePtr->mHeader.mTableOID;

        IDE_ASSERT( smcTable::getTableHeaderFromOID( 
                sTableOID, (void**)&sTableHeader ) == IDE_SUCCESS );

        if( SMP_GET_PERS_PAGE_TYPE( sPagePtr ) == SMP_PAGETYPE_FIX )
        {
            sFixedSlotSize = sTableHeader->mFixed.mMRDB.mSlotSize;
            smpFixedPageList::dumpFixedPageByBuffer( 
                (UChar*)sPagePtr,
                sFixedSlotSize,
                sTempBuf,
                IDE_DUMP_DEST_LIMIT );
            idlOS::printf("%s\n", sTempBuf );

            printWarning4Record();
            for( i = 0 ; i < sTableHeader->mFixed.mMRDB.mSlotCount ; i ++ )
            {
                sSlot = (smpSlotHeader*)( &(sPagePtr->mBody[0]) 
                                          + i * sFixedSlotSize );

                printRecord( sTableHeader, 
                             sSlot, 
                             sTempBuf, 
                             IDE_DUMP_DEST_LIMIT );
            }
        }
        else
        {
            smpVarPageList::dumpVarPageByBuffer( (UChar*)sPagePtr,
                                                 sTempBuf,
                                                 IDE_DUMP_DEST_LIMIT );

            idlOS::printf("%s\n", sTempBuf );

        }
    }
    IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
}
void dumpIncrementalBackupMeta()
{
    iduFile             sFile;
    smmChkptImageHdr    sChkptImageHdr;
    
    IDE_ASSERT( sFile.initialize( IDU_MEM_MMU,
                                1,
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              == IDE_SUCCESS );

    IDE_ASSERT( sFile.setFileName( gTargetFN ) == IDE_SUCCESS );
    IDE_ASSERT( sFile.open() == IDE_SUCCESS );
    IDE_ASSERT( sFile.read( NULL,
                            SM_DBFILE_METAHDR_PAGE_OFFSET,
                            (void *)&sChkptImageHdr,
                            ID_SIZEOF(smmChkptImageHdr),
                            NULL )
              == IDE_SUCCESS );

    IDE_ASSERT( sFile.close() == IDE_SUCCESS );
    IDE_ASSERT( sFile.destroy() == IDE_SUCCESS );

    if( (sChkptImageHdr.mBackupInfo.mBeginBackupTime != 0) ||
        (sChkptImageHdr.mBackupInfo.mEndBackupTime != 0) )
    {
        idlOS::printf("\n");
        IDE_ASSERT( dumpMemMeta( 0, &sChkptImageHdr ) 
                    == IDE_SUCCESS );
    }
    else
    {
        idlOS::printf("%s is not Incremental Backup File\n", gTargetFN );
    }
}

void dumpDiskPage( scSpaceID aSID, scPageID aPID )
{

    UChar              * sPagePtr;
    SChar              * sTempBuf;
    sdpPhyPageHdr      * sPhyPageHdr;
    UChar              * sSlotDirPtr;
    UChar              * sSlot;
    UShort               sSlotNum;
    UShort               sSlotCount;
    UChar                sRowValue [ IDE_DUMP_DEST_LIMIT ];
    idBool               sIsRowDeleted;
    idBool               sIsPageLatchReleased;
    smOID                sTableOID;
    smcTableHeader     * sTableHeader;
    smiFetchColumnList * sFetchColumnList;
    UInt                 sMaxRowSize = 0;


    IDE_ASSERT( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                   ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                   (void**)&sTempBuf )
                == IDE_SUCCESS );


    IDE_ASSERT( sdbBufferMgr::fixPageByPID( NULL, // idvSQL
                                            aSID,
                                            aPID,
                                            &sPagePtr )
                == IDE_SUCCESS );

    /* page를 Hexa code로 dump하여 출력한다. */
    if( ideLog::ideMemToHexStr( (UChar*)sPagePtr, 
                                SD_PAGE_SIZE,
                                IDE_DUMP_FORMAT_NORMAL,
                                sTempBuf,
                                IDE_DUMP_DEST_LIMIT )
        == IDE_SUCCESS )
    {
        idlOS::printf("%s\n", sTempBuf );
    }
    else
    {
        /* nothing to do ... */
    }

    /* PhyPageHeader를 dump하여 출력한다. */
    if( sdpPhyPage::dumpHdr( (UChar*) sPagePtr,
                             sTempBuf,
                             IDE_DUMP_DEST_LIMIT )
        == IDE_SUCCESS )
    {
        idlOS::printf("%s", sTempBuf );
    }
    else
    {
        /* nothing to do ... */
    }

    sPhyPageHdr = sdpPhyPage::getHdr( sPagePtr );
    if( sdpPhyPage::getPageType( sPhyPageHdr ) == SDP_PAGE_DATA )
    {
        printWarning4Record();

        sTableOID = sdpPhyPage::getTableOID( sPagePtr );
        IDE_ASSERT( smcTable::getTableHeaderFromOID( 
                sTableOID, (void**)&sTableHeader ) == IDE_SUCCESS );

        IDE_ASSERT( smcTable::initRowTemplate( NULL,
                                               sTableHeader,
                                               NULL )
                    == IDE_SUCCESS );

        IDE_ASSERT( sdnManager::makeFetchColumnList( sTableHeader,
                                                     &sFetchColumnList,
                                                     &sMaxRowSize )
                    == IDE_SUCCESS );


        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPhyPageHdr );
        sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

        for( sSlotNum=0; sSlotNum < sSlotCount; sSlotNum++ )
        {
            if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotNum)
                == ID_TRUE )
            {
                continue;
            }

            IDE_ASSERT( sdpSlotDirectory::getPagePtrFromSlotNum(sSlotDirPtr, 
                                                              sSlotNum,
                                                              &sSlot)
                        == IDE_SUCCESS) ;
            IDE_ASSERT( sdcRow::fetch( NULL, // Statistics,
                                       NULL, // aMtx
                                       NULL, // SP
                                       NULL, // aTrans
                                       aSID,
                                       (UChar*)sSlot,
                                       ID_TRUE, /* aIsPersSlot */
                                       SDB_SINGLE_PAGE_READ,
                                       sFetchColumnList,
                                       SMI_FETCH_VERSION_LAST,
                                       SD_NULL_RID, /* TssRID, */
                                       NULL,        /* SCN, */
                                       NULL,        /* InfiniteSCN, */
                                       NULL,        /* aIndexInfo4Fetch */
                                       NULL,        /* aLobInfo4Fetch */
                                       sTableHeader->mRowTemplate,
                                       (UChar*)&sRowValue,
                                       &sIsRowDeleted,
                                       &sIsPageLatchReleased ) == IDE_SUCCESS );

            if( sIsRowDeleted == ID_TRUE )
            {
                idlOS::printf("[DELETED] ");
            }

            printColumns( sTableHeader->mSelfOID,
                          sTableHeader->mColumnCount,
                          (SChar*)sRowValue );
        }


        IDE_ASSERT( sdnManager::destFetchColumnList( sFetchColumnList ) 
                    == IDE_SUCCESS );

        IDE_ASSERT( smcTable::destroyRowTemplate( sTableHeader ) == IDE_SUCCESS );
    }

    IDE_ASSERT( sdbBufferMgr::unfixPage( NULL, sPagePtr )
                == IDE_SUCCESS );

    IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );

}

void dumpPage( scSpaceID aSID, scPageID aPID )
{
    IDE_TEST_RAISE( ( aSID == SMUTIL_ALL_SPACEID ) ||
                    ( sctTableSpaceMgr::isExistingTBS( aSID ) == ID_FALSE ),
                    invalid_argument );

    if( sctTableSpaceMgr::isMemTableSpace( aSID ) == ID_TRUE )
    {
        /* Memory Tablespace */
        dumpMemPage( aSID, aPID );
    }
    else
    {
        if( sctTableSpaceMgr::isDiskTableSpace( aSID ) == ID_TRUE )
        {
            /* Disk Tablespace */
            dumpDiskPage( aSID, aPID );
        }
        else
        {
            /* nothing to do ... */
        }
    }

    return;

    IDE_EXCEPTION( invalid_argument )
    {
        (void)usage();
    }

    IDE_EXCEPTION_END;

    return;
}

int main( int aArgc, char *aArgv[] )
{
    /*************************************************************
     * Initialize
     *************************************************************/
    parseArgs( aArgc, aArgv );

    /* LogAnchor에 있는 TBS가 아닌 특정 파일을 직접 설정할때 */
    if( gTargetFN != NULL )
    {
        IDE_TEST_RAISE( idlOS::access( gTargetFN, F_OK) != 0,
                        err_unable_to_access_db_file );

        parseDBFileName( gTargetFN, gTBSName );
    }

    if( gDisplayHeader == ID_TRUE)
    {
        smuUtility::outputUtilityHeader("dumpdb");
    }

    IDE_TEST_RAISE( gInvalidArgs == ID_TRUE , err_invalid_arguments );

    // Spatio-Temporal 관련 DataType, Conversion, Function 등록
    IDE_TEST( smiSmUtilInit( &mmm::mSmiGlobalCallBackList  ) != IDE_SUCCESS );
    IDE_TEST( sti::addExtMT_Module() != IDE_SUCCESS );
    IDE_TEST( mtc::initialize( NULL ) != IDE_SUCCESS );

    initPCH();
    smmManager::setCallback4Util( getPersPagePtr );
    // mt 관련 초기화 중 BLob, Clob의 Converting과 관련해 의미없는
    // 오류를 만들어 에러 메시지를 보기 어렵게 한다.
    //      - blob to int, blob to char 등이 선언되어 있지 않아
    //        그것이 불가능하다는 ide오류가 쌓인다.
    // 따라서 이 시점에서 초기화 해준다.
    ideClearError();

    /*************************************************************
     * Body
     *************************************************************/
    if( gJob == SMUTIL_JOB_INCREMENTAL_BACKUP )
    {
        IDE_TEST_RAISE( idlOS::access( gTargetFN, F_OK) != 0,
                        err_unable_to_access_db_file );

        dumpIncrementalBackupMeta();
    }
    else
    {
        initAllMemTBS();
        switch( gJob )
        {
        case SMUTIL_JOB_META:
        case SMUTIL_JOB_TABLESPACE:
        case SMUTIL_JOB_TABLESPACE_FLI:
        case SMUTIL_JOB_TABLESPACE_FREEPAGE:
            dumpTablespace( gSID );
            break;
        case SMUTIL_JOB_TABLE:
            dumpTableRecord( gOID );
            break;
        case SMUTIL_JOB_TABLE_ALLOCPAGE:
            dumpTableAllocPageList( gOID );
            break;
        case SMUTIL_JOB_PAGE:
            dumpPage( gSID, gPID );
            break;
        default:
            break;
        }
    }

    /*************************************************************
     * finalize
     *************************************************************/
    finalizePCH();
    IDE_TEST( mtc::finalize( NULL ) != IDE_SUCCESS );
    IDE_TEST( smiSmUtilShutdown( NULL ) != IDE_SUCCESS );
    idlOS::printf( "\nDump complete.\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_arguments );
    {
        (void)usage();
    }
    IDE_EXCEPTION(err_unable_to_access_db_file) ;
    {
        idlOS::printf( "\nUnable to access DB File specified by -f option\n" );
    }
    IDE_EXCEPTION_END;

    smuUtility::outputMsg( ideGetErrorMsg(ideGetErrorCode()) );

    return IDE_FAILURE;
}

void dumpTablespace( scSpaceID aSID )
{
    smmTBSNode       * sCurTBS;
    scSpaceID          sCurSID;
    const SChar      * sTypeName[]={"SMI_MEMORY_SYSTEM_DICTIONARY",
                                    "SMI_MEMORY_SYSTEM_DATA",
                                    "SMI_MEMORY_USER_DATA",
                                    "SMI_DISK_SYSTEM_DATA",
                                    "SMI_DISK_USER_DATA",
                                    "SMI_DISK_SYSTEM_TEMP",
                                    "SMI_DISK_USER_TEMP",
                                    "SMI_DISK_SYSTEM_UNDO",
                                    "SMI_VOLATILE_USER_DATA" };


    sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurTBS != NULL )
    {
        sCurSID = sCurTBS->mHeader.mID;
        if( ( aSID == sCurSID ) || 
            ( aSID == SMUTIL_ALL_SPACEID ) )
        {
            idlOS::printf( 
                "----------------------------------------"
                "----------------------------------------\n");
            idlOS::printf( "[%4"ID_UINT32_FMT"]%s - %s State:%"ID_UINT32_FMT"\n",
                           sCurTBS->mHeader.mID,
                           sCurTBS->mHeader.mName,
                           sTypeName[ sCurTBS->mHeader.mType ],
                           sCurTBS->mHeader.mState );

            if( sctTableSpaceMgr::isMemTableSpace( sCurTBS->mHeader.mID ) 
                == ID_TRUE )
            {
                switch( gJob )
                {
                case SMUTIL_JOB_META:
                    IDE_ASSERT( dumpMemMeta( sCurSID, NULL ) == IDE_SUCCESS );
                    break;
                case SMUTIL_JOB_TABLESPACE:
                    IDE_ASSERT( dumpMemBase( sCurTBS->mMemBase ) 
                                == IDE_SUCCESS );
                    break;
                case SMUTIL_JOB_TABLESPACE_FLI:
                    IDE_ASSERT( dumpMemFLI( sCurTBS ) 
                                == IDE_SUCCESS );
                    break;
                case SMUTIL_JOB_TABLESPACE_FREEPAGE:
                    IDE_ASSERT( dumpMemFreePageList( sCurTBS ) 
                                == IDE_SUCCESS );
                    break;
                default:
                    break;
                }
            }
            else
            {
                /* 메모리만 취급함 */
            }
        }
        else
        {
            /* 출력할 필요 없음 */
        }
        sCurTBS = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sCurTBS->mHeader.mID );
    }
}

IDE_RC initAllMemTBS()
{
    smmTBSNode        * sTBSNode;
    void              * sPagePtr;

    /* Target을 설정했으면, 특정 TBS를 대상으로 설정한다. */
    if( gTargetFN != NULL )
    {
        IDE_TEST(sctTableSpaceMgr::findSpaceNodeByName(
                gTBSName,
                (void**)& sTBSNode,
                ID_FALSE ) != IDE_SUCCESS );

        gSID = sTBSNode->mHeader.mID;
    }

    sTBSNode =(smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();

    idlOS::printf( "initialize dumpdb..." );
    while( sTBSNode != NULL )
    {
        if( sctTableSpaceMgr::isMemTableSpace( sTBSNode->mHeader.mID )
             == ID_TRUE )
        {

            IDE_TEST( getPersPagePtr( sTBSNode->mHeader.mID,
                                      SMM_MEMBASE_PAGEID, // PID
                                      &sPagePtr )
                      != IDE_SUCCESS );
            IDE_TEST( smmManager::setupBasePageInfo( sTBSNode,
                                                     (UChar *)sPagePtr )
                      != IDE_SUCCESS);
        }

        sTBSNode = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sTBSNode->mHeader.mID );
    }
    idlOS::printf( "done\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Parse DB File Name String
    DB File 이름의  Format : "TableSpaceName-PingPong-FileNo"

    [IN] aDBFileName - Parsing할 DB File의 이름

    [OUT] aTBSName - DB File의 이름
 */
void parseDBFileName(SChar * aDBFileName,
                     SChar * aTBSName )
{
    SChar   sFileName[ SM_MAX_FILE_NAME+1 ];

    IDE_ASSERT( aDBFileName != NULL );
    IDE_ASSERT( aTBSName    != NULL );

    idlOS::strcpy( sFileName, aDBFileName );

    idlOS::strcpy( aTBSName, idlOS::strtok( sFileName, "-") );
}

