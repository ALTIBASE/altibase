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

#ifndef _O_ULN_PROPERTIES_H_
#define _O_ULN_PROPERTIES_H_ 1

#include <ulnPrivate.h>

#define IPCDA_DEF_CLIENT_SLEEP_TIME 400
#define IPCDA_DEF_CLIENT_EXP_CNT    0
#define IPCDA_DEF_MESSAGEQ_TIMEOUT  100

struct ulnProperties {
    ACP_STR_DECLARE_DYNAMIC(mHomepath);
    ACP_STR_DECLARE_DYNAMIC(mConfFilepath);
    ACP_STR_DECLARE_DYNAMIC(mUnixdomainFilepath);
    ACP_STR_DECLARE_DYNAMIC(mIpcFilepath);
    ACP_STR_DECLARE_DYNAMIC(mIPCDAFilepath);
    acp_uint32_t  mIpcDaClientSleepTime;
    acp_uint32_t  mIpcDaClientExpireCount;
    acp_uint32_t  mIpcDaClientMessageQTimeout;
};

/**
 *  ulnPropertiesCreate
 *
 *  @aProperties : ulnProperties의 객체
 *
 *  서버 환경파일을 읽어 aProperties에 저장한다.
 *
 *  우선순위
 *  ALTIBASE 환경변수 > altibase.properties > 코드에 설정된 값
 */
void ulnPropertiesCreate(ulnProperties *aProperties);

/**
 *  ulnPropertiesDestroy
 *
 *  @aProperties : ulnProperties의 객체
 *
 *  aProperties의 메모리를 반환한다.
 */
void ulnPropertiesDestroy(ulnProperties *aProperties);

/**
 *  ulnPropertiesExpandValues
 *
 *  @aProperties : ulnProperties의 객체
 *  @aDest       : 타겟 버퍼
 *  @aDestSize   : 타켓 버퍼 사이즈
 *  @aSrc        : 소스 버퍼
 *  @aSrcLen     : 소스 문자열 길이
 *
 *  ?를 $ALTIBASE_HOME으로 확장한다.
 */
ACI_RC ulnPropertiesExpandValues( ulnProperties *aProperties,
                                  acp_char_t    *aDest,
                                  acp_sint32_t   aDestSize,
                                  acp_char_t    *aSrc,
                                  acp_sint32_t   aSrcLen );

#endif /* _O_ULN_PROPERTIES_H_ */
