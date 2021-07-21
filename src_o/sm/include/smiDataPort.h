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
 * $Id: smiDataPort.h 
 * 
 * Proj-2059 DB Upgrade 기능
 * Server 중심적으로 데이터를 가져오고 넣는 기능

 **********************************************************************/

#ifndef __O_SMI_DATAPORT_H__
#define __O_SMI_DATAPORT_H__ 1_

/* - Version Up을 위한 Guide.
 *
 *  여기서 말하는 Version UP은 파일의 구조가 변경되는 경우 입니다.
 * 만약 파일의 구조가 변경될 필요가 없는 경우, 모듈의 데이터 처리
 * 방법만 달라지는 경우는 상관 없습니다.
 *
 *  또한 파일의 추가는 새로운 Attribute의 추가만을 허용합니다. 기존
 * 항목의 삭제는 허용치 않습니다. 하위호환성을 위함입니다.
 *
 *  현재 기능은 높은 버전의 DBMS가 낮은 버전의 파일을 읽을 수 있도록
 * 설계되어 있습니다. 반대는 되지 않습니다.
 *
 *
 *
 *  Version업을 위한 절차는 다음과 같습니다.
 *
 *  1. 최신 Version Number 올림
 *    smiDef.h에 정의된 SMI_DATAPORT_VERSION_LATEST를 상승시킵니다.
 * 
 * ex)   
 * #define SMI_DATAPORT_VERSION_2        (2)
 * 
 * #define SMI_DATAPORT_VERSION_BEGIN    (1)
 * #define SMI_DATAPORT_VERSION_LATEST   (SMI_DATAPORT_VERSION_2)
 * 
 * 
 * 
 * 
 * 
 *  2. 세부 헤더의 버전업
 *     세부 헤더들은 다음과 같습니다.
 *
 *     공통헤더 - gSmiDataPortHeaderDesc : smiDataPort.cpp 
 *     파일헤더 - gSCPfFileHeaderDesc : scpfModule.cpp        
 *     테이블헤더 - gQsfTableHeaderDesc : qsfDataPort.cpp 
 *     칼럼헤더 - gQsfColumnHeaderDesc : qsfDataPort.cpp 
 *     파티션헤더 - gQsfPartitionHeaderDesc : qsfDataPort.cpp 
 *
 *     블럭헤더 - gSCPfBlockHeaderDesc : scpfModule.cpp
 *
 *
 *
 *  1) 해당 헤더에 새 항목이 추가
 *       HeaderDesc에 새 버전을 추가하고, ColumnDesc도 추가하여 연결
 *     합니다.
 *         
 *     ex)   
 *     smiDataPortHeaderColDesc  gSmiDataPortHeaderColDescV2[]=                  
 *     {                                                                           
 *         {                                                                       
 *             (SChar*)"VERSION_UP_TEST",                                          
 *             SMI_DATAPORT_HEADER_OFFSETOF( smiDataPortHeader , mTestValue ),   
 *             SMI_DATAPORT_HEADER_SIZEOF( smiDataPortHeader , mTestValue ),     
 *             1, NULL,   // Default                                               
 *             SMI_DATAPORT_HEADER_TYPE_INTEGER                                   
 *         },                                                                      
 *         {                                                                       
 *             NULL,                                                               
 *             0,                                                                  
 *             0,                                                                  
 *             0,0,                                                                
 *             0                                                                   
 *         }                                                                       
 *     };                                                                          
 *         
 *    smiDataPortHeaderDesc gSmiDataPortHeaderDesc[ SMI_DATAPORT_VERSION_COUNT ]=
 *    ...
 *        {                                                                 
 *            (SChar*)"COMMON_HEADER_V2",                                   
 *            ID_SIZEOF( smiDataPortHeader ),                              
 *            (smiDataPortHeaderColDesc*)gSmiDataPortHeaderColDescV2,     
 *            gSmiDataPortHeaderValidation                                 
 *        }                                                                 
 *    };
 *
 *
 *  2) Default값 및 Module 수정
 *    추가된 항목은 이전 버전에는 존재치 않는 값입니다. 따라서 이전 버전을 위해
 *  Default값을 설정합니다. 
 *
 *    만약 Default값만으로도 하위 버전을 읽기 충분하다면 상관없지만, 다른 처리가
 * 필요할 경우 Default를 특정한 값으로 정의 후 Module에서 처리해줘야 합니다.
 *
 *
 *
 *  3) 새 항목 추가가 없는 헤더일 경우
 *     Header Desc에 Null로 설정하면 충분합니다.
 *    
 *  {
 *      (SChar*)"BLOCK_HEADER_V2",
 *      ID_SIZEOF( scpfBlockInfo),
 *    <<NULL,>>
 *      gSCPfBlockHeaderValidation
 *  }
 *
 */


#include <smiDef.h>
#include <smuList.h>

class smiDataPort
{
public:
    /************************************************************
     * Common
     ************************************************************/
    static IDE_RC findHandle( SChar          * aName,
                              void          ** aHandle );

    /************************************************************
     * Export
     ************************************************************/
    //export를 시작한다.
    static IDE_RC beginExport( idvSQL               * aStatistics, 
                               void                ** aHandle,
                               smiDataPortHeader    * aHeader,
                               SChar                * aJobName, 
                               SChar                * aObjectName, 
                               SChar                * aDirectory, 
                               UInt                   aType,
                               SLong                  aSplit );

    //Row를 하나 Write한다.
    static IDE_RC write( idvSQL          * aStatistics, 
                         void           ** aHandle,
                         smiValue        * aValueList );

    // Lob을 쓸 준비를 한다.
    static IDE_RC prepareLob( idvSQL          * aStatistics, 
                              void           ** aHandle,
                              UInt              aLobLength );

    // Lob을 기록한다.
    static IDE_RC writeLob( idvSQL          * aStatistics,
                            void           ** aHandle,
                            UInt              aLobPieceLength,
                            UChar           * aLobPieceValue );

    // Lob의 기록이 완료되었다.
    static IDE_RC finishLobWriting( idvSQL          * aStatistics,
                                    void           ** aHandle );

    // Export를 종료한다.
    static IDE_RC endExport( idvSQL          * aStatistics,
                             void           ** aHandle );



    /************************************************************
     * Import
     ************************************************************/
    //import를 시작한다. 헤더를 읽는다.
    static IDE_RC beginImport( idvSQL               * aStatistics, 
                               void                ** aHandle,
                               smiDataPortHeader    * aHeader,
                               SChar                * aJobName, 
                               SChar                * aObjectName, 
                               SChar                * aDirectory, 
                               UInt                   aType,
                               SLong                  aFirstRowSeq,
                               SLong                  aLastRowSeq );

    //row들을 읽는다.
    static IDE_RC read( idvSQL          * aStatistics, 
                        void           ** aHandle,
                        smiRow4DP      ** aRows,
                        UInt            * aRowCount );

    //Lob의 총 길이를 반환한다.
    static IDE_RC readLobLength( idvSQL          * aStatistics, 
                                 void           ** aHandle,
                                 UInt            * aLength );

    //Lob을 읽는다.
    static IDE_RC readLob( idvSQL          * aStatistics, 
                           void           ** aHandle,
                           UInt            * aLobPieceLength,
                           UChar          ** aLobPieceValue );

    // Lob 읽기가  완료되었다.
    static IDE_RC finishLobReading( idvSQL          * aStatistics,
                                    void           ** aHandle );

    //import를 종료한다.
    static IDE_RC endImport( idvSQL          * aStatistics,
                             void           ** aHandle );


    /****************************************************************
     * HeaderDescriptor
     *
     * Structure에 있는 Member Variable을 Endian 상관 없이
     * 읽고 쓸 수 있도록, Format을 정의한 Header 기록용 함수
     ****************************************************************/
    /* 원본 Header의 크기를 반환한다. */
    static UInt getHeaderSize( smiDataPortHeaderDesc   * aDesc,
                               UInt                      aVersion);

    /* Encoding된 Header의 크기를 구한다. */
    static UInt getEncodedHeaderSize( smiDataPortHeaderDesc   * aDesc,
                                      UInt                      aVersion);


    /* Desc의 Min/Max설정을 바탕으로 Validation을 수행한다. */
    static IDE_RC validateHeader( smiDataPortHeaderDesc   * aDesc,
                                  UInt                      aVersion,
                                  void                    * aData );

    /* Desc에 따라 write한다. */
    static IDE_RC writeHeader( smiDataPortHeaderDesc   * aDesc,
                               UInt                      aVersion,
                               void                    * aData,
                               UChar                   * aDestBuffer,
                               UInt                    * aOffset,
                               UInt                      aDestBufferSize );

    /* Desc에 따라 Read한다. */
    static IDE_RC readHeader( smiDataPortHeaderDesc   * aDesc,
                              UInt                      aVersion,
                              UChar                   * aSourceBuffer,
                              UInt                    * aOffset,
                              UInt                      aSourceBufferSize,
                              void                    * aDestData );

    /* Header를 Dump한다. */
    static IDE_RC dumpHeader( smiDataPortHeaderDesc   * aDesc,
                              UInt                      aVersion,
                              void                    * aSourceData,
                              UInt                      aFlag,
                              SChar                   * aOutBuf ,
                              UInt                      aOutSize );

    /**************************************************************
     * DataPort File의 헤더/블럭/Row를 출력한다.
     **************************************************************/
    static IDE_RC dumpFileHeader( SChar * aFileName,
                                  SChar * aDirectory,
                                  idBool  aDetail );

    static IDE_RC dumpFileBlocks( SChar    * aFileName,
                                  SChar    * aDirectory,
                                  idBool     aHexa,
                                  SLong      aFirstBlock,
                                  SLong      aLastBlock );

    static IDE_RC dumpFileRows( SChar    * aFileName,
                                SChar    * aDirectory,
                                SLong      aFirst,
                                SLong      aLast );


};

#endif /* __O_SMI_DATAPORT_H__ */

