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

#ifndef _O_ULN_CONFIG_H_
#define _O_ULN_CONFIG_H_ 1

/**
 * ULN_USE_GETDATA_EXTENSIONS.
 *
 * 이 상수는 GetData 를 위해서 서버에서 보내 준 데이터 중 사용자가 바인드하지 않은
 * 컬럼의 데이터를 IRD record 에 캐쉬하는 부분과 관련된 코드를 활성화시키는지의 여부를
 * 지정하는 상수이다.
 *
 * 그러나, SQLGetData() 에 ODBC 스펙에서 명시한 제약사항을 그대로 두기로 결정되었고,
 * 그에 따라 fetch protocol 이 바뀌었으며, 아래의 코드가 필요 없게 되었다.
 *
 * 하지만, SQLGetInfo() 에서 SQL_GETDATA_EXTENSIONS 에 관한 정보를 사용자가 얻었을 때
 * 가능한 모든 옵션을 지원하기 위해서는 상기 언급한 캐슁 루틴이 필요하다.
 * 그때를 대비해서 코드를 남겨둔다.
 *
 * 현재(2006. 09. 26일)는 아래의 ULN_USE_GETDATA_EXTENSIONS  를 0 으로 세팅해 둠으로써
 * caching 과 관련된 코드가 하나도 컴파일되지 않도록 조치해 두었다.
 *
 * 물론 현재(2006.09.26) 관련 코드들을 out 시키는 시점의 코드들은 검증되지도,
 * 완성되지도 않은 코드들이다. 개념만 파악해서 재사용 하도록 한다.
 *
 * 필요가 생길 때 제한을 풀고 코드 수정과 디버깅을 하면 되겠다.
 */
#define ULN_USE_GETDATA_EXTENSIONS 0

/*
 * DiagHeader 가 가지는 청크 풀의 단위 크기 및 SP 갯수
 */
#define ULN_MAX_DIAG_MSG_LEN                ACI_MAX_ERROR_MSG_LEN
#define ULN_SIZE_OF_CHUNK_IN_DIAGHEADER     (ACI_SIZEOF(ulnDiagRec) + ULN_MAX_DIAG_MSG_LEN + 1024)
#define ULN_NUMBER_OF_SP_IN_DIAGHEADER      (10)

/*
 * ENV의 청크 풀의 단위 크기 및 SP 의 갯수
 */
#define ULN_SIZE_OF_CHUNK_IN_ENV            (4 * 1024)
#define ULN_NUMBER_OF_SP_IN_ENV             20

/*
 * DBC의 청크 풀의 단위 크기 및 SP 갯수
 */
#define ULN_SIZE_OF_CHUNK_IN_DBC            (32 * 1024)
#define ULN_NUMBER_OF_SP_IN_DBC             20

/*
 * Cache 의 청크풀의 단위크기 및 SP 갯수
 */
#define ULN_SIZE_OF_CHUNK_IN_CACHE          (64 * 1024)
#define ULN_NUMBER_OF_SP_IN_CACHE           4

#endif /* _O_ULN_CONFIG_H_ */
