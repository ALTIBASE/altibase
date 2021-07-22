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
 * PROJ-1915 : Off-line Replicator
 * Off-line 센더를 위한 LFGMgr 오직 로그 읽기 기능 만을 수행 한다.
 *
 **********************************************************************/

#ifndef _O_SMR_REMOTE_LFG_MGR_H_
#define _O_SMR_REMOTE_LFG_MGR_H_ 1

#include <idu.h>
#include <iduMemoryHandle.h>
#include <smDef.h>
#include <smrDef.h>

typedef struct smrRemoteLogMgrInfo{
    /* 첫번째 로그 파일 번호 */
    UInt    mFstFileNo;
    /* 마지막 로그 파일 번호 */
    UInt    mEndFileNo;
    /* 마지막 기록된 로그 LstLSN*/
    smLSN    mLstLSN;
}smrRemoteLogMgrInfo;

class smrRemoteLogMgr
{
public :
    smrRemoteLogMgr();
    ~smrRemoteLogMgr();

    /*
     * aLogFileSize - off-line Log File Size
     * aLogDirPath  - LogDirPath array
     * aNotDecomp   - 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부 (압축된 형태로 반환)
     */
    IDE_RC initialize( ULong    aLogFileSize,
                       SChar ** aLogDirPath,
                       idBool   aNotDecomp );

    /* 로그 그룹 관리자 해제 */
    IDE_RC destroy();

    /* 마지막 LSN값을 구한다. */
    void getLstLSN( smLSN * aLstLSN );

    /* 특정 로그파일의 첫번째 로그레코드의 Head를 File로부터 직접 읽는다 */
    IDE_RC readFirstLogHeadFromDisk( smLSN      * aLSN,
                                     smrLogHead * aLogHead,
                                     idBool     * aIsValid );

    /* aLogFile을 Close한다. */
    IDE_RC closeLogFile(smrLogFile * aLogFile);

    /* aLSN이 가리키는 로그파일의 첫번째 Log 의 Head를 읽는다 */
    IDE_RC readFirstLogHead( smLSN      * aLSN,
                             smrLogHead * aLogHead,
                             idBool     * aIsValid );

    /*
      aFirstFileNo에서 aEndFileNo사이의
      모든 LogFile을 조사해서 aMinSN보다 작은 SN을 가지는 로그를
      첫번째로 가지는 LogFile No를 구해서 aNeedFirstFileNo에 넣어준다.
    */
    IDE_RC getFirstNeedLFN( smLSN        aMinLSN,
                            const UInt   aFirstFileNo,
                            const UInt   aEndFileNo,
                            UInt       * aNeedFirstFileNo );

    /* 특정 LSN의 log record와 해당 log record가 속한 로그 파일을 리턴한다. */
    IDE_RC readLog( iduMemoryHandle * aDecompBufferHandle,
                    smLSN           * aLSN,
                    idBool            aIsCloseLogFile,
                    smrLogFile     ** aLogFile,
                    smrLogHead      * aLogHead,
                    SChar          ** aLogPtr,
                    UInt            * aLogSizeAtDisk );

    /* readLog에 Valid 검사 까지 한다.  */
    IDE_RC readLogAndValid( iduMemoryHandle * aDecompBufferHandle,
                            smLSN           * aLSN,
                            idBool            aIsCloseLogFile,
                            smrLogFile     ** aLogFile,
                            smrLogHead      * aLogHeadPtr,
                            SChar          ** aLogPtr,
                            idBool          * aIsValid,
                            UInt            * aLogSizeAtDisk );

    /*
       aFileNo에 해당하는 Logfile을 open한후
       aLogFilePtr에 open된 logfile의 pointer를 넘겨준다..
     */
    IDE_RC openLogFile( UInt          aFileNo,
                        idBool        aIsWrite,
                        smrLogFile ** aLogFilePtr );

    /* Check LogDir Exist */
    IDE_RC checkLogDirExist(void);

    /* aIndex에 해당하는 로그 경로를 리턴 한다. */
    SChar* getLogDirPath();

    /* aIndex에 로그 경로를 세팅 한다. */
    void   setLogDirPath(SChar * aDirPath);

    /* 로그 파일 사이즈를 리턴 한다. */
    ULong  getLogFileSize(void);

    /* 로그 파일 사이즈를 설정 한다. */
    void   setLogFileSize(ULong aLogFileSize);

    /* 로그 파일 존재 유무 검사 */
    IDE_RC isLogFileExist(UInt aFileNo, idBool * aIsExist);

    /* mRemoteLogMgrs 정보를 채운다. */
    IDE_RC setRemoteLogMgrsInfo();

    /* 최소 파일 번호, 최대 파일 번호 */
    IDE_RC setFstFileNoAndEndFileNo(UInt * aFstFileNo,
                                    UInt * aEndFileNo);

    /* 모든 로그 파일 번호에서 최초 파일 번호를 리턴 한다. */
    void   getFirstFileNo(UInt * aFileNo);

    /* logfileXXX에서 XXX를 번호를 반환 한다. */
    UInt   chkLogFileAndGetFileNo(SChar  * aFileName,
                                  idBool * aIsLogFile);

private :

    /* smrLogFile 객체 할당을 위한 memory pool */
    iduMemPool mMemPool;

    /* 로그 파일 사이즈 */
    ULong      mLogFileSize;

    /* 로그 파일 경로 */
    SChar    * mLogDirPath;

    /* 로그파일매니저 */
    smrRemoteLogMgrInfo mRemoteLogMgrs;

    /* BUG-46944 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부 (압축된 형태로 반환)*/
    idBool mNotDecomp;
};
#endif /* _O_SMR_REMOTE_LFG_MGR_H_ */

