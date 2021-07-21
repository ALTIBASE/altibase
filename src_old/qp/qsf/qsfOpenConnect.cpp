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
 * $Id: qsfOpenConnect.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 *     TCP socket OPEN_CONNECT 함수
 *
 * Syntax :
 *    OPEN_CONNECT( remote_host VARCHAR, remote_port integer,
                    connectionTimeout integer, txbuffersize );
 *    RETURN CONNECT_TYPE
 *
 **********************************************************************/

#include <qsf.h>
#include <qsxEnv.h>
#include <qcuSessionObj.h>
#include <qcuProperty.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;
extern mtdModule mtsConnect;

static mtcName qsfFunctionName[1] = {
    { NULL, 12, (void*)"OPEN_CONNECT" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfOpenConnectModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};

IDE_RC qsfCalculate_OpenConnect( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_OpenConnect,
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
    const mtdModule* sModules[4] =
    {
        &mtdVarchar, // host
        &mtdInteger, // port
        &mtdInteger, // connection timeout
        &mtdInteger, // tx buffer size
    };

    const mtdModule* sModule = &mtsConnect;

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
}

IDE_RC qsfCalculate_OpenConnect( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     open_connect calculate
 *
 * Implementation :
 ***********************************************************************/
    
    qcStatement     * sStatement;
    qcSession       * sSession;
    mtsConnectType  * sConnectType;
    mtdCharType     * sRemoteHost;
    mtdIntegerType    sPort;
    mtdIntegerType    sConnectTimeOut;
    mtdIntegerType    sTxBufferSize;
    qcSessionObjInfo *sSessionObjInfo;
    PDL_SOCKET        sSocket = PDL_INVALID_SOCKET;
    
    SChar             sRemoteHostStr[IDL_IP_ADDR_MAX_LEN + 1];
    SChar             sRemotePortStr[IDL_IP_PORT_MAX_LEN + 1];

    struct addrinfo   sHints;
    struct addrinfo * sAddrInfo = NULL;
    SInt              sRet = 0;
    PDL_Time_Value    sConnTimeoutVal;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;
    
    sSession = sStatement->spxEnv->mSession;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
             != IDE_SUCCESS );
    
    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        sConnectType = (mtsConnectType*)aStack[0].value;
        
        // remote host
        sRemoteHost = (mtdCharType*)aStack[1].value;
        
        IDE_TEST_RAISE( sRemoteHost->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );
        
        idlOS::strncpy( sRemoteHostStr,
                        (SChar*)sRemoteHost->value,
                        sRemoteHost->length );
        sRemoteHostStr[sRemoteHost->length] = '\0';
        
        // port no
        sPort = *(mtdIntegerType*)aStack[2].value;
        IDE_TEST_RAISE( ( sPort > 65535 ) ||
                        ( sPort < 0 ),
                        ERR_PORT );
        
        idlOS::snprintf( sRemotePortStr, IDL_IP_PORT_MAX_LEN + 1, "%"ID_UINT32_FMT"", sPort );
        
        // connect time out
        if ( aStack[3].column->module->isNull( aStack[3].column,
                                               aStack[3].value ) == ID_TRUE )
        {
            sConnectTimeOut = 0;
        }
        else
        {
            sConnectTimeOut = *(mtdIntegerType*)aStack[3].value;
            if ( sConnectTimeOut < 0 )
            {
                sConnectTimeOut = 0;
            }
            else
            {
                /* Nothing to do */
            }
        }
        sConnTimeoutVal.initialize( sConnectTimeOut/1000000, sConnectTimeOut%1000000 );
        
        // txBufferSize
        if ( aStack[4].column->module->isNull( aStack[4].column,
                                               aStack[4].value ) == ID_TRUE )
        {
            sTxBufferSize = 2048;
        }
        else
        {
            sTxBufferSize = *(mtdIntegerType*)aStack[4].value;
            if ( sTxBufferSize < 2048 )
            {
                sTxBufferSize = 2048;
            }
            else
            {
                /* Nothing to do */
            }
            
            if ( sTxBufferSize > QSF_TX_MAX_BUFFER_SIZE )
            {
                sTxBufferSize = QSF_TX_MAX_BUFFER_SIZE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        
        idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo) );
        sHints.ai_socktype = SOCK_STREAM;
        sHints.ai_family   = AF_INET;
#ifdef AI_NUMERICSERV
        sHints.ai_flags    = AI_NUMERICSERV|AI_NUMERICHOST;
#endif
        sRet = idlOS::getaddrinfo( sRemoteHostStr,
                                   sRemotePortStr,
                                   &sHints,
                                   &sAddrInfo);
        
        if ( (sRet != 0) || (sAddrInfo == NULL) )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
        
        // stream socket
        sSocket = idlOS::socket( sAddrInfo->ai_family, SOCK_STREAM, 0 );
        
        // tx buffer setting
        if ( idlOS::setsockopt( sSocket,
                                SOL_SOCKET,
                                SO_SNDBUF,
                                (SChar*)&sTxBufferSize,
                                ID_SIZEOF(sTxBufferSize) ) < 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
        
        // tcp connect
        sSessionObjInfo = (qcSessionObjInfo*)(sSession->mQPSpecific.mSessionObj);
        IDE_TEST_RAISE( sSessionObjInfo->connectionList.count >= QCU_CONNECT_TYPE_OPEN_LIMIT,
                        ERR_OEPN_LIMIT );
        
        if ( idlVA::connect_timed_wait( sSocket,
                                        sAddrInfo->ai_addr,
                                        sAddrInfo->ai_addrlen,
                                        &sConnTimeoutVal ) < 0 )
         {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
        
        // qcusession add open connect
        IDE_TEST( qcuSessionObj::addConnection( sSessionObjInfo,
                                                sSocket,
                                                &sConnectType->connectionNodeKey )
                  != IDE_SUCCESS );
    }
    
    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    if ( sAddrInfo != NULL )
    {
        idlOS::freeaddrinfo( sAddrInfo );
        sAddrInfo = NULL;
    }
    else
    {
        /* Nothing to do */
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_IPADDRESS );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSF_INVALID_IPADDRESS ) );
    }
    IDE_EXCEPTION( ERR_PORT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSF_INVALID_PORT ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION( ERR_OEPN_LIMIT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_CONNECT_TYPE_OPEN_LIMIT_EXCEED ) );
    }

    IDE_EXCEPTION_END;
    
    if ( sSocket != PDL_INVALID_SOCKET )
    {
        idlOS::closesocket( sSocket );
    }
    else
    {
        /* Nothing to do */
    }
    
    if ( sAddrInfo != NULL )
    {
        idlOS::freeaddrinfo( sAddrInfo );
        sAddrInfo = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    
    return IDE_FAILURE;
}

