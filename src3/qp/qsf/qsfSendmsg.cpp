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
 * $Id: qsfSendmsg.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <qsf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxUtil.h>
#include <qcuSessionObj.h>

#define QSF_SENDMSG_MAX (2048)

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 7, (void*)"SENDMSG" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfSendmsgModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_Sendmsg( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_Sendmsg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC qsfEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule* sModules[4] =
    {
        &mtdVarchar,
        &mtdInteger,
        &mtdVarchar,
        &mtdInteger
    };

    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 4,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // return값은 Integer
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsfCalculate_Sendmsg( 
                     mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*        aInfo,
                     mtcTemplate* aTemplate )
{
    qcStatement     *sStatement;
    qcSession       *sSession;
    mtdCharType     *sAddrArgument;
    mtdIntegerType   sPortArgument;
    mtdCharType     *sMsgArgument;
    mtdIntegerType   sTTLArgument;
    mtdIntegerType  *sReturnValue;
    SChar            sAddrBuffer[IDL_IP_ADDR_MAX_LEN];
    SChar            sPortStr[IDL_IP_PORT_MAX_LEN];
    PDL_SOCKET       sSocket = PDL_INVALID_SOCKET;
    struct addrinfo  sHints;
    struct addrinfo *sAddrInfo = NULL;
    UChar            sTTL;
    SInt             sMsgLen;
    SInt             sRet = 0;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;
    
    sSession = sStatement->spxEnv->mSession;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) ||
        (aStack[4].column->module->isNull( aStack[4].column,
                                           aStack[4].value ) == ID_TRUE) )
    {
        // error. value argument is invalid.
        IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
    }
    else
    {
        sAddrArgument = (mtdCharType*)aStack[1].value;
        sPortArgument = *(mtdIntegerType*)aStack[2].value;
        sMsgArgument = (mtdCharType*)aStack[3].value;
        sTTLArgument = *(mtdIntegerType*)aStack[4].value;

        sReturnValue = (mtdIntegerType*)aStack[0].value;

        // value range validation
        // ip가 제대로 되었는지 검사(길이는 IDL_IP_ADDR_MAX_LEN 이하)
        // port범위가 제대로 되었는지 검사(1025 ~ 65535)
        // 메시지 길이가 <=QSF_SENDMSG_MAX인지 검사
        // ttl값이 0 ~ 255인지 검사(멀티캐스트 주소에 보낼때만 적용됨)
        IDE_TEST_RAISE( sAddrArgument->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );
        IDE_TEST_RAISE( ( sPortArgument > 65535 ) ||
                        ( sPortArgument < 1025 ),
                        ERR_PORT );
        IDE_TEST_RAISE( sMsgArgument->length > QSF_SENDMSG_MAX,
                        ERR_MSGLEN );
        IDE_TEST_RAISE( ( sTTLArgument > 255 ) ||
                        ( sTTLArgument < 0 ),
                        ERR_TTL );

        idlOS::strncpy( sAddrBuffer,
                        (SChar*)sAddrArgument->value,
                        sAddrArgument->length );

        sAddrBuffer[sAddrArgument->length] = '\0';

        idlOS::sprintf(sPortStr, "%"ID_UINT32_FMT"", sPortArgument);
       
        idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo));
        sHints.ai_socktype = SOCK_DGRAM;
        sHints.ai_family   = AF_UNSPEC;
#ifdef AI_NUMERICSERV
        sHints.ai_flags    = AI_NUMERICSERV;
#endif

        sRet = idlOS::getaddrinfo(sAddrBuffer, sPortStr,
                                  &sHints, &sAddrInfo);
       
        // invalid address인 경우 sRet != 0 or s
        IDE_TEST_RAISE( ((sRet != 0) || (sAddrInfo == NULL)) , ERR_IPADDRESS );


        // socket 가져옴
        IDE_TEST( qcuSessionObj::getSendSocket(
                      &sSocket,
                      (qcSessionObjInfo*)(sSession->mQPSpecific.mSessionObj),
                      sAddrInfo->ai_family) );
        
        IDE_TEST_RAISE( sSocket < 0, ERR_SOCKET );

        // TTL값 setting
        sTTL = (UChar)sTTLArgument;

        if( idlOS::setsockopt( sSocket,
                               IPPROTO_IP,
                               IP_MULTICAST_TTL,
                               (SChar*)&sTTL,
                               ID_SIZEOF(sTTL) ) < 0 )
        {
            IDE_RAISE( ERR_SOCKET );
        }
        else
        {
            // Nothing to do.
        }
        
        sMsgLen = idlOS::sendto( sSocket,
                                 (SChar*)sMsgArgument->value,
                                 sMsgArgument->length,
                                 0,
                                 (struct sockaddr *)sAddrInfo->ai_addr,
                                 sAddrInfo->ai_addrlen );
        IDE_TEST_RAISE( sMsgLen < 0, ERR_SENDMSG );
                
        *sReturnValue = sMsgLen;

        idlOS::freeaddrinfo(sAddrInfo);
        sAddrInfo = NULL;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_IPADDRESS );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_INVALID_IPADDRESS));
    }
    IDE_EXCEPTION( ERR_PORT );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_INVALID_PORT));
    }
    IDE_EXCEPTION( ERR_TTL );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_INVALID_TTL));
    }
    IDE_EXCEPTION( ERR_SOCKET );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_SOCKET_FAILED));
    }
    IDE_EXCEPTION( ERR_MSGLEN );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }
    IDE_EXCEPTION( ERR_SENDMSG );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_SENDMSG_FAILED));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    if(sAddrInfo != NULL)
    {
        idlOS::freeaddrinfo(sAddrInfo);
        sAddrInfo = NULL;
    }
   
    return IDE_FAILURE;
    
#undef IDE_FN
}



 
