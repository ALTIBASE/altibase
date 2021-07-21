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

#ifndef  _O_ULN_CONFIG_H_
#define  _O_ULN_CONFIG_H_ 1

#include <uln.h>
#include <ulnDataSource.h>

#define  ULN_CONFIG_FILE_PATH_LEN      (1024)
#define  ULN_CONFIG_READ_BUFF_SIZE     (2048)
#define  ULN_CONFIG_FILE_NAME          "altibase_cli.ini"
#define  ULN_CONFIG_COMMENT_CHAR       '#'


// PROJ-1645 UL Config file.
typedef struct ulnConfigFile
{
    acp_std_file_t       mFile;
    ulnDataSourceBucket  mBucketTable[ULN_DATASOURCE_BUCKET_COUNT];
}ulnConfigFile;

ulnDataSource *ulnConfigFileGetDataSource(acp_char_t *aDataSourceName);
ulnDataSource *ulnConfigFileRegisterUrlDataSource(acp_char_t *aUrl);
void           ulnConfigFileDump();
void           ulnConfigFileDumpFromName(acp_char_t *aDataSourceName);     /* BUG-28793 */
void           ulnConfigFileUnLoad();
void           ulnConfigFileLoad();

#endif /* _O_ULN_CONFIG_H_ */
