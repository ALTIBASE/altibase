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
 * $Id:$
 **********************************************************************/

#ifndef _O_SMI_TABLEBACKUP_DEF
#define _O_SMI_TABLEBACKUP_DEF 1

#define SMI_BACKUP_FILE_BUFFER_SIZE (1024 * 1024)

typedef struct smiTableBackupColumnInfo
{
    const smiColumn *mSrcColumn;    // src table smiColumn
    const smiColumn *mDestColumn;   // dest table smiColumn
    idBool           mIsSkip;
} smiTableBackupColumnInfo;

class smiTableBackup
{
/* Function */
public:
    IDE_RC initialize(const void* aTable,
                      SChar * aBackupFileName);
    IDE_RC destroy();

    IDE_RC backup(smiStatement* aStatement,
                  const void*   aTable,
                  idvSQL       *aStatistics);

    IDE_RC restore(void                  * aTrans,
                   const void            * aDstTable,
                   smOID                   aTableOID,
                   smiColumnList         * aSrcColumnList,
                   smiColumnList         * aBothColumnList,
                   smiValue              * aNewRow,
                   idBool                  aIsUndo,
                   smiAlterTableCallBack * aCallBack);

    static IDE_RC dump(SChar *aFilename);

    // BUG-20048
    static IDE_RC isBackupFile(SChar *aFilename, idBool *aCheck);

    smiTableBackup(){};
    virtual ~smiTableBackup(){};

private:
    IDE_RC create();

    IDE_RC getBackupFileTail(smcBackupFileTail *aBackupFileTail);
    IDE_RC appendBackupFileHeader();
    IDE_RC appendBackupFileTail(smOID aTableOID, ULong aFileSize, UInt aRowMaxSize);

    IDE_RC appendRow(smiTableCursor *aTableCursor,
                     smiColumn     **aArrLobColumn,
                     UInt            aLobColumnCnt,
                     SChar          *aRowPtr,
                     UInt           *aRowSize);

    IDE_RC appendLobColumn(idvSQL*          aStatistics,
                           smiTableCursor  *aTableCursor,
                           UChar           *aBuffer,
                           UInt             aBufferSize,
                           const smiColumn *aLobColumn,
                           UInt             aLobLen,
                           UInt             aLobFlag,
                           SChar           *aRow);

    IDE_RC waitForSpace(UInt aNeedSpace);
    IDE_RC checkBackupFileIsValid(smOID aTableOID, smcBackupFileTail *aFileTail);

    IDE_RC readRowFromBackupFile( smiTableBackupColumnInfo * aArrColumnInfo,
                                  UInt                       aColumnCnt,
                                  smiValue                 * aArrValue,
                                  smiAlterTableCallBack    * aCallBack );

    IDE_RC readColumnFromBackupFile(const smiColumn *aColumn,
                                    smiValue        *aValue,
                                    UInt            *aColumnOffset);

    IDE_RC insertLobRow( void                       * aTrans,
                         void                       * aHeader,
                         smiTableBackupColumnInfo   * aArrLobColumnInfo,
                         UInt                         aLobColumnCnt,
                         SChar                      * aRowPtr,
                         UInt                         aAddOIDFlag );

    IDE_RC skipReadColumn(const smiColumn *aColumn);
    IDE_RC skipReadLobColumn(const smiColumn *aColumn);

    IDE_RC readNullRowAndSet(
        void                     * aTrans,
        void                     * aTableHeader,
        smiTableBackupColumnInfo * aArrColumnInfo,
        UInt                       aColumnCnt,
        smiTableBackupColumnInfo * aArrLobColumnInfo,
        UInt                       aLobColumnCnt,
        smiValue                 * aValueList,
        smiAlterTableCallBack    * aCallBack );

    IDE_RC skipNullRow(smiColumnList * aColumnList);

    IDE_RC makeColumnList4Res(const void                * aDstTableHeader,
                              smiColumnList             * aSrcColumnList,
                              smiColumnList             * aDstColumnList,
                              smiTableBackupColumnInfo ** aArrColumnInfo,
                              UInt                      * aColumnCnt,
                              smiTableBackupColumnInfo ** aArrLobColumnInfo,
                              UInt                      * aLobColumnCnt);

    IDE_RC destColumnList4Res( void *aArrColumn,
                               void *aArrLobColumn );

    IDE_RC appendTableColumnInfo();
    IDE_RC skipColumnInfo();

    /* inline function */
    inline IDE_RC writeToBackupFile(void *aBuffer, UInt aSize);
    inline IDE_RC prepareAccessFile(idBool aIsWrite);
    inline IDE_RC initBuffer(UInt aBufferSize);
    inline IDE_RC destBuffer();
    inline IDE_RC finishAccessFile();

/* Member */
private:
    smcTableBackupFile mFile;   //Backup File
    UChar*             mRestoreBuffer; //buffer
    ULong              mOffset; //전체파일에서 read, write offset
    UInt               mRestoreBufferSize; //할당된 버퍼의 크기.
    smcTableHeader    *mTableHeader; //BackupFile에 저장할, 저장된
                                     //테이블 헤더
    smiColumn        **mArrColumn;
};

/***********************************************************************
 * Description : File에 데이타를 기록한다.
 *
 * aBuffer - [IN] 버퍼의 크기.
 * aSize   - [IN] Write할 데이타의 크기.
 ***********************************************************************/
IDE_RC smiTableBackup::writeToBackupFile(void *aBuffer, UInt aSize)
{
    /* BUG-15751: NULL Row를 기록하다 서버사망. 왜냐하면 Varchar의 Null Value
       는 길이가 0이어서 smcTableBackupFile의 write에서 길이가 0이상인것을
       Check하고 있어서 기록할 데이타의 길이가 0이면 IDE_ASSERT에서 서버가
       죽습니다.*/
    if(aSize > 0 )
    {
        while (1)
        {
            errno = 0;
            if (mFile.write(mOffset, (SChar*)aBuffer, aSize) == IDE_SUCCESS)
            {
                break;
            }
            IDE_TEST(waitForSpace(aSize) != IDE_SUCCESS);
        }
    }

    mOffset += aSize;
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : File에 데이타를 기록하기위해 File을 open하고 mOffset을
 *               초기화한다.
 *
 * aIsWrite - [IN] write: ID_TRUE, read:ID_FALSE
 ***********************************************************************/
IDE_RC smiTableBackup::prepareAccessFile(idBool aIsWrite)
{
    IDE_TEST(mFile.open(aIsWrite) != IDE_SUCCESS);
    mOffset = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : File close를 수행한다.
 ***********************************************************************/
IDE_RC smiTableBackup::finishAccessFile()
{
    IDE_TEST(mFile.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aBufferSize크기만큼의 버퍼를 할당한다.
 *
 * aBufferSize - [IN] 할당받을 버퍼의 크기.
 ***********************************************************************/
IDE_RC smiTableBackup::initBuffer(UInt aBufferSize)
{
    IDE_ASSERT(mRestoreBuffer == NULL);

    //BUG-25640
    //Variable 또는 Lob 칼럼만 존재하며, 값이 Null일 경우, Row의 Value만 저장하는 mRestoreBuffer 또한 Null일 수 있다.
    if( aBufferSize != 0 )
    {
        IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMC,
                                   1,
                                   aBufferSize,
                                   (void**)&mRestoreBuffer,
                                   IDU_MEM_FORCE)
                 != IDE_SUCCESS);
    }

    mRestoreBufferSize = aBufferSize;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 할당받은 Buffer를 Free한다.
 *
 ***********************************************************************/
IDE_RC smiTableBackup::destBuffer()
{
    //BUG-25640
    //Variable 또는 Lob 칼럼만 존재하며, 값이 Null일 경우, Row의 Value만 저장하는 mRestoreBuffer 또한 Null일 수 있다.
    if( mRestoreBuffer != NULL )
    {
        IDE_TEST(iduMemMgr::free(mRestoreBuffer) != IDE_SUCCESS);
    }
    mRestoreBuffer       = NULL;
    mOffset       = 0;
    mRestoreBufferSize   = 0;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

#endif
