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

/**
 * acp.h 를 include 하고 alticore 함수를 호출하지 않은 이유는 alticore 의
 * socket handle 은 캡슐화되어 있고 현재로서 C 코드가 간단하기 때문이다.
 */
#include <acp.h>
#include <Altibase_jdbc_driver_JniExt.h>

JNIEXPORT jlong JNICALL Java_Altibase_jdbc_driver_JniExt_getTcpiLastDataRecv(
        JNIEnv  *aEnv,
        jobject  aObject,
        jint     aSocketDescriptor)
{
    int             sRet;
    socklen_t       sOptionLen = sizeof(struct tcp_info);
    struct tcp_info sTcpInfo;
    jclass          sJavaExceptionClass;
    char            sExceptionMessage[256];

    ACP_UNUSED(aObject);

    sRet = getsockopt(aSocketDescriptor,
                      SOL_TCP,
                      TCP_INFO,
                      &sTcpInfo,
                      &sOptionLen);


    if (sRet < 0)
    {
        sJavaExceptionClass = (*aEnv)->FindClass(aEnv, "java/io/IOException");
        if (sJavaExceptionClass == NULL)
        {
            sJavaExceptionClass = (*aEnv)->FindClass(aEnv, "java/lang/NoClassDefFoundError");
            if (sJavaExceptionClass == NULL)
            {
                assert(0);
            }
        }
        else
        {
            snprintf(sExceptionMessage, sizeof(sExceptionMessage),
                    "getsockopt(SOL_TCP, TCP_INFO, ...), errno = %d",
                    errno);

            (*aEnv)->ThrowNew(aEnv, sJavaExceptionClass, sExceptionMessage);
        }
    }

    return sTcpInfo.tcpi_last_data_recv;
}

