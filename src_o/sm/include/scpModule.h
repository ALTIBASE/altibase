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
 *
 * $Id: scpModule.h $
 *
 **********************************************************************/

#ifndef _O_SCE_MODULE_H_
#define _O_SCE_MODULE_H_ 1

#include <smiDef.h>

//export를 시작한다.
typedef IDE_RC (*scpBeginExport)( idvSQL              * aStatistics,
                                  void               ** aHandle,
                                  smiDataPortHeader   * aHeader,
                                  SChar               * aObjName,
                                  SChar               * aDirectory,
                                  SLong                 aSplit );

//Row를 하나 Write한다.
typedef IDE_RC (*scpWrite)( idvSQL         * aStatistics,
                            void           * aHandle,
                            smiValue       * aValueList );

// Lob을 쓸 준비를 한다.
typedef IDE_RC (*scpPrepareLob)( idvSQL         * aStatistics,
                                 void           * aHandle,
                                 UInt             aLobLength );

// Lob을 기록한다.
typedef IDE_RC (*scpWriteLob)( idvSQL         * aStatistics,
                               void           * aHandle,
                               UInt             aLobPieceLength,
                               UChar          * aLobPieceValue );

// Lob을 기록을 완료한다.
typedef IDE_RC (*scpFinishLobWriting)( idvSQL         * aStatistics,
                                       void           * aHandle );

// Export를 종료한다.
typedef IDE_RC (*scpEndExport)( idvSQL         * aStatistics,
                                void           * aHandle );

//import를 시작한다. 헤더를 읽어 Header에 넣는다.
typedef IDE_RC (*scpBeginImport)( idvSQL              * aStatistics,
                                  void               ** aHandle,
                                  smiDataPortHeader   * aHeader,
                                  SLong                 aFirstRowSeq,
                                  SLong                 aLastRowSeq,
                                  SChar               * aObjName,
                                  SChar               * aDirectory );

//row들을 읽는다.
typedef IDE_RC (*scpRead)( idvSQL         * aStatistics,
                           void           * aHandle,
                           smiRow4DP     ** aRows,
                           UInt           * aRowCount );

//Lob의 총 길이를 반환한다.
typedef IDE_RC (*scpReadLobLength)( idvSQL         * aStatistics,
                                    void           * aHandle,
                                    UInt           * aLength );

//Lob을 읽는다.
typedef IDE_RC (*scpReadLob)( idvSQL         * aStatistics,
                              void           * aHandle,
                              UInt           * aLobPieceLength,
                              UChar         ** aLobPieceValue );

//Lob을 읽기를 완료한다.
typedef IDE_RC (*scpFinishLobReading)( idvSQL         * aStatistics,
                                       void           * aHandle );


//import를 종료한다.
typedef IDE_RC (*scpEndImport)( idvSQL         * aStatistics,
                                void           * aHandle );

typedef struct scpModule
{
    UInt                mType;
    scpBeginExport      mBeginExport;
    scpWrite            mWrite;
    scpPrepareLob       mPrepareLob;
    scpWriteLob         mWriteLob;
    scpFinishLobWriting mFinishLobWriting;
    scpEndExport        mEndExport;
    scpBeginImport      mBeginImport;
    scpRead             mRead;
    scpReadLobLength    mReadLobLength;
    scpReadLob          mReadLob;
    scpFinishLobReading mFinishLobReading;
    scpEndImport        mEndImport;
} scpModule;

#endif /*_O_SCE_MODULE_H_ */
