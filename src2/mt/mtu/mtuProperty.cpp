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
 
 
/*****************************************************************************
 * $Id: mtuProperty.cpp 87881 2020-06-29 08:04:01Z kclee $
 ****************************************************************************/
#include <idl.h>
#include <ide.h>
#include <idp.h>
#include <mtuProperty.h>
#include <mtc.h>
#include <mtlTerritory.h>
#include <mte.h>
#include <mtz.h>

#ifdef ALTIBASE_PRODUCT_XDB
mtuProperties  * mtuProperty::mSharedProperty;
#else
mtuProperties    mtuProperty::mStaticProperty;
#endif

#ifdef ALTIBASE_PRODUCT_XDB

IDE_RC mtuProperty::initProperty( idvSQL *aStatistics )
{
    IDE_ASSERT( allocShm( aStatistics ) == IDE_SUCCESS );

    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        IDE_TEST( load() != IDE_SUCCESS );
        IDE_TEST( setupUpdateCallback() != IDE_SUCCESS );
    }
    else
    {
        // DAEMON 이 아닌 경우에는 load 를 해서는 안됨
        IDE_TEST( setupUpdateCallback() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtuProperty::finalProperty( idvSQL *aStatistics )
{
    IDE_ASSERT( freeShm( aStatistics ) == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC mtuProperty::allocShm( idvSQL *aStatistics )
{
    idShmAddr         sAddr4StProps;
    idrSVP            sVSavepoint;
    iduShmTxInfo    * sShmTxInfo = NULL;

    sAddr4StProps  = IDU_SHM_NULL_ADDR;
    sShmTxInfo     = IDV_WP_GET_THR_INFO( aStatistics );

    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        idrLogMgr::setSavepoint( sShmTxInfo, &sVSavepoint );

        IDE_TEST( iduShmMgr::allocMem( aStatistics,
                                       sShmTxInfo,
                                       IDU_MEM_MT,
                                       ID_SIZEOF( mtuProperties ),
                                       (void **)&mSharedProperty )
                  != IDE_SUCCESS );

        mSharedProperty->mAddrSelf = iduShmMgr::getAddr( mSharedProperty );

        iduShmMgr::setMetaBlockAddr(
            (iduShmMetaBlockType)IDU_SHM_META_MT_PROPERTIES_BLOCK,
            mSharedProperty->mAddrSelf );
        
        IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                         sShmTxInfo,
                                         &sVSavepoint )
                  != IDE_SUCCESS );
    }
    else
    {
        /* try to attach at shared memory. */
        sAddr4StProps =
            iduShmMgr::getMetaBlockAddr( IDU_SHM_META_MT_PROPERTIES_BLOCK );

        /* attach shared memory to local memory. */
        IDE_ASSERT( sAddr4StProps != IDU_SHM_NULL_ADDR );

        mSharedProperty = (mtuProperties *)IDU_SHM_GET_ADDR_PTR( sAddr4StProps );

        IDE_ASSERT( mSharedProperty != NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, sShmTxInfo, &sVSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC mtuProperty::freeShm( idvSQL *aStatistics )
{
    idrSVP           sVSavepoint;
    iduShmTxInfo   * sShmTxInfo = NULL;

    IDE_ASSERT( aStatistics != NULL );

    sShmTxInfo = IDV_WP_GET_THR_INFO( aStatistics );

    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        idrLogMgr::setSavepoint( sShmTxInfo, &sVSavepoint );

        IDE_TEST( iduShmMgr::freeMem( aStatistics,
                                      sShmTxInfo,
                                      &sVSavepoint,
                                      mSharedProperty)
                  != IDE_SUCCESS );

        IDE_ASSERT( idrLogMgr::commit2Svp( aStatistics,
                                           sShmTxInfo,
                                           &sVSavepoint )
                    == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, sShmTxInfo, &sVSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

#else

IDE_RC mtuProperty::initProperty( idvSQL */*aStatistics*/ )
{
    IDE_TEST( load() != IDE_SUCCESS );
    IDE_TEST( setupUpdateCallback() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtuProperty::finalProperty( idvSQL */*aStatistics*/ )
{
    return IDE_SUCCESS;
}

#endif

IDE_RC
mtuProperty::load()
{
    SInt    sNlsTerritory   = -1;
    SInt    sNlsISOCurrency = -1;
    idBool  sIsValid;

    IDE_ASSERT(idp::read("DEFAULT_DATE_FORMAT",
                         MTU_PROPERTY(mDateFormat)) == IDE_SUCCESS );

    IDE_ASSERT(idp::read("NLS_COMP",
                         & MTU_PROPERTY(mNlsCompMode) ) == IDE_SUCCESS );

    IDE_ASSERT(idp::read("NLS_NCHAR_CONV_EXCP",
                         & MTU_PROPERTY(mNlsNcharConvExcp)) == IDE_SUCCESS );
                 
    // PROJ-1579 NCHAR
    IDE_ASSERT(idp::read("NLS_NCHAR_LITERAL_REPLACE",
                         & MTU_PROPERTY(mNlsNcharLiteralReplace)) == IDE_SUCCESS );
    // BUG-34342
    IDE_ASSERT(idp::read("ARITHMETIC_OPERATION_MODE",
                         & MTU_PROPERTY(mArithmeticOpMode)) == IDE_SUCCESS );
 
    /* PROJ-2208 Multi Currency */
    IDE_ASSERT(idp::read("NLS_TERRITORY",
                         MTU_PROPERTY(mNlsTerritory) ) == IDE_SUCCESS );

    IDE_TEST( mtlTerritory::searchNlsTerritory(MTU_PROPERTY(mNlsTerritory), &sNlsTerritory)
              != IDE_SUCCESS );

    if ( sNlsTerritory < 0 )
    {
        IDE_TEST( mtlTerritory::setNlsTerritoryName( MTL_TERRITORY_DEFAULT,
                                                     MTU_PROPERTY(mNlsTerritory) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2208 Multi Currency */
    IDE_ASSERT(idp::read("NLS_ISO_CURRENCY", MTU_PROPERTY(mNlsISOCurrency) )
               == IDE_SUCCESS );

    IDE_TEST( mtlTerritory::searchISOCurrency(MTU_PROPERTY(mNlsISOCurrency), &sNlsISOCurrency)
              != IDE_SUCCESS );

    if ( sNlsISOCurrency < 0 )
    {
        IDE_TEST( idp::update( NULL, "NLS_ISO_CURRENCY", MTU_PROPERTY(mNlsTerritory), 0, NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2208 NLS_CURRENCY의 Default 설정은 캐릭터 셋 문제로
     * 추후에 loadNLSCurrencyProperty 에서 설정한다.
     */
    IDE_ASSERT(idp::read("NLS_CURRENCY", MTU_PROPERTY(mNlsCurrency) )
               == IDE_SUCCESS );

    /* PROJ-2208 Multi Currency */
    IDE_ASSERT(idp::read("NLS_NUMERIC_CHARACTERS", MTU_PROPERTY(mNlsNumChar) )
               == IDE_SUCCESS );

    IDE_TEST( mtlTerritory::validNlsNumericChar( MTU_PROPERTY(mNlsNumChar), &sIsValid )
              != IDE_SUCCESS );
    if ( sIsValid == ID_FALSE )
    {
        IDE_TEST( mtlTerritory::setNlsNumericChar( sNlsTerritory,
                                                   MTU_PROPERTY(mNlsNumChar) )
                  != IDE_SUCCESS );
        IDE_TEST( idp::update( NULL,"NLS_NUMERIC_CHARACTERS",
                               MTU_PROPERTY(mNlsNumChar), 0, NULL )
                  != IDE_SUCCESS );

    }
    else
    {
        /* Nothing to do */
    }
    
    /* PROJ-2209 DBTIMEZONE sysdate 를 위한 서버의 OS timezone을 구한다. */
    MTU_PROPERTY(mOSTimezoneSecond) = mtc::getSystemTimezoneSecond();
    (void)mtc::getSystemTimezoneString( MTU_PROPERTY(mOSTimezoneString) );

    IDE_ASSERT( idp::read( (const SChar*) "TIME_ZONE",
                           MTU_PROPERTY(mDBTimezoneString) ) == IDE_SUCCESS );

    if ( idlOS::strCaselessMatch( MTU_PROPERTY(mDBTimezoneString), "OS_TZ" ) == 0 )
    {
        MTU_PROPERTY(mDBTimezoneSecond) = MTU_PROPERTY(mOSTimezoneSecond);
        (void)mtc::getSystemTimezoneString( MTU_PROPERTY(mDBTimezoneString) );
    }
    else
    {
        IDE_TEST ( mtz::getTimezoneSecondAndString( MTU_PROPERTY(mDBTimezoneString),
                                                    &MTU_PROPERTY(mDBTimezoneSecond),
                                                    MTU_PROPERTY(mDBTimezoneString) )
                   != IDE_SUCCESS );
    }

    /* PROJ-1753 One Pass Like */
    IDE_ASSERT(idp::read("__LIKE_OP_USE_OLD_MODULE", &MTU_PROPERTY(mLikeOpUseOldModule) )
               == IDE_SUCCESS );

    // BUG-38416 count(column) to count(*)
    IDE_ASSERT(idp::read("OPTIMIZER_COUNT_COLUMN_TO_COUNT_ASTAR",
                         &MTU_PROPERTY(mCountColumnToCountAStar) )
               == IDE_SUCCESS );

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    IDE_ASSERT( idp::read( (const SChar*) "LOB_OBJECT_BUFFER_SIZE",
                           & MTU_PROPERTY(mLOBObjectBufSize) ) == IDE_SUCCESS );

    // BUG-37247
    IDE_ASSERT(idp::read("GROUP_CONCAT_PRECISION", &MTU_PROPERTY(mGroupConcatPrecision) )
               == IDE_SUCCESS );
    
    // BUG-38101
    IDE_ASSERT(idp::read("CASE_SENSITIVE_PASSWORD", &MTU_PROPERTY(mCaseSensitivePassword) )
               == IDE_SUCCESS );

    // BUG-38842
    IDE_ASSERT(idp::read("CLOB_TO_VARCHAR_PRECISION", &MTU_PROPERTY(mClob2VarcharPrecision) )
               == IDE_SUCCESS );

    // BUG-41194
    IDE_ASSERT(idp::read("IEEE754_DOUBLE_TO_NUMERIC_FAST_CONVERSION",
                         &MTU_PROPERTY(mDoubleToNumericFastConversion) )
               == IDE_SUCCESS );

    // BUG-41555 DBMS PIPE
    IDE_ASSERT(idp::read("MSG_QUEUE_PERMISSION",
                         &MTU_PROPERTY(mMsgQueuePermission) )
               == IDE_SUCCESS );

    /* BUG-46267 */
    IDE_ASSERT( idp::read( "NUMBER_CONVERSION_MODE",
                           & MTU_PROPERTY( mNumberConversionMode ) )
               == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mtuProperty::setupUpdateCallback()
{
    IDE_TEST(idp::setupAfterUpdateCallback(
                 (const SChar*)"DEFAULT_DATE_FORMAT",
                 mtuProperty::changeDEFAULT_DATE_FORMAT) != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                 (const SChar*)"NLS_NCHAR_CONV_EXCP",
                 mtuProperty::changeNLS_NCHAR_CONV_EXCP) != IDE_SUCCESS);
                 
    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"NLS_TERRITORY",
                 mtuProperty::changeNLS_TERRITORY) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"NLS_ISO_CURRENCY",
                 mtuProperty::changeNLS_ISO_CURRENCY) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"NLS_CURRENCY",
                 mtuProperty::changeNLS_CURRENCY) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"NLS_NUMERIC_CHARACTERS",
                 mtuProperty::changeNLS_NUMERIC_CHARACTERS) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"OPTIMIZER_COUNT_COLUMN_TO_COUNT_ASTAR",
                 mtuProperty::changeCOUNT_COLUMN_TO_COUNT_ASTAR) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"GROUP_CONCAT_PRECISION",
                 mtuProperty::changeGROUP_CONCAT_PRECISION) != IDE_SUCCESS);
    
    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"CASE_SENSITIVE_PASSWORD",
                 mtuProperty::changeCASE_SENSITIVE_PASSWORD) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"CLOB_TO_VARCHAR_PRECISION",
                 mtuProperty::changeCLOB_TO_VARCHAR_PRECISION) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"IEEE754_DOUBLE_TO_NUMERIC_FAST_CONVERSION",
                 mtuProperty::changeDOUBLE_TO_NUMERIC_FAST_CONV) != IDE_SUCCESS);

    // BUG-41555 DBMS PIPE
    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"MSG_QUEUE_PERMISSION",
                 mtuProperty::changeMSG_QUEUE_PERMISSION) != IDE_SUCCESS);

    IDE_TEST(idp::setupBeforeUpdateCallback(
                 (const SChar*)"ARITHMETIC_OPERATION_MODE",
                 mtuProperty::changeARITHMETIC_OPERATION_MODE) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mtuProperty::changeDEFAULT_DATE_FORMAT(  idvSQL * /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    DATE_FORMAT 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/
    idlOS::strcpy(MTU_PROPERTY(mDateFormat), (SChar*)aNewValue);
            
    return IDE_SUCCESS;
}

IDE_RC
mtuProperty::changeNLS_NCHAR_CONV_EXCP(  idvSQL * /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1579 NCHAR
 *    NLS_NCHAR_CONV_EXCP의 변경을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/
    idlOS::memcpy( &MTU_PROPERTY(mNlsNcharConvExcp),
                   aNewValue,
                   ID_SIZEOF(UInt) );
            
    return IDE_SUCCESS;
}

/**
 * PROJ-2208 loadNLSCurrencyProperty
 *
 *  DB가 mtuProperty:load가 호출될 때는 현제 chararchter set을 알 수 없기 때문에
 *  추후에 현제 character set이 알수 있는 시점에서 호출된다.
 */
IDE_RC mtuProperty::loadNLSCurrencyProperty( void )
{
    SInt    sNlsTerritory = -1;
    idBool  sIsValid;

    IDE_TEST( mtlTerritory::validCurrencySymbol( MTU_PROPERTY(mNlsCurrency), &sIsValid )
              != IDE_SUCCESS );
    
    if ( sIsValid == ID_FALSE )
    {
        IDE_TEST( mtlTerritory::searchNlsTerritory( MTU_PROPERTY(mNlsTerritory), &sNlsTerritory )
                  != IDE_SUCCESS );

        IDE_TEST( mtlTerritory::setNlsCurrency( sNlsTerritory,
                                                MTU_PROPERTY(mNlsCurrency) )
                  != IDE_SUCCESS );
        IDE_TEST( idp::update( NULL, "NLS_CURRENCY", MTU_PROPERTY(mNlsCurrency), 0, NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

/**
 * PROJ-2208 changeNLS_TERRITORY
 *
 *  alter system으로 인해 Property 값이 변경 될 경우 Territory 값이 변경 가능할 경우
 *  Property 값을 변경한다.
 */
IDE_RC mtuProperty::changeNLS_TERRITORY( idvSQL * /* aStatistics */,
                                         SChar *,
                                         void  *,
                                         void  * aNewValue,
                                         void  * )
{
    SChar * sValue        = ( SChar * )aNewValue;
    SInt    sNlsTerritory = -1;
    SInt    sLen          = 0;

    IDE_TEST( mtlTerritory::searchNlsTerritory( sValue, &sNlsTerritory )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sNlsTerritory < 0, ERR_INVALID_CHARACTER )
    sLen = idlOS::strlen( sValue );

    idlOS::memcpy( MTU_PROPERTY(mNlsTerritory), sValue, sLen );
    MTU_PROPERTY(mNlsTerritory)[sLen] = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_CHARACTER )
    {
        IDE_TEST( ideSetErrorCode( mtERR_ABORT_INVALID_CHARACTER ));
    }

    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

/**
 * PROJ-2208 changeNLS_ISO_CURRENCY
 *
 *  alter system으로 인해 Property 값이 변경 될 경우 ISO Currency 값이 변경
 *  가능할 경우 Property 값을 변경한다. ISO Currency의 경우 변경을 territory value
 *  값으로 변경을 한다.
 */
IDE_RC mtuProperty::changeNLS_ISO_CURRENCY( idvSQL * /* aStatistics */,
                                            SChar *,
                                            void  *,
                                            void  * aNewValue,
                                            void  * )
{
    SChar * sValue          = ( SChar * )aNewValue;
    SInt    sNlsISOCurrency = -1;
    SInt    sLen            = 0;

    IDE_TEST( mtlTerritory::searchISOCurrency( sValue, &sNlsISOCurrency )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sNlsISOCurrency < 0, ERR_INVALID_CHARACTER );

    sLen = idlOS::strlen( sValue );

    idlOS::memcpy( MTU_PROPERTY(mNlsISOCurrency), sValue, sLen );
    MTU_PROPERTY(mNlsISOCurrency)[sLen] = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_CHARACTER )
    {
        IDE_TEST( ideSetErrorCode( mtERR_ABORT_INVALID_CHARACTER ));
    }

    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

/**
 * PROJ-2208 changeNLS_CURRENCY
 *
 *  alter system으로 인해 Property 값이 변경 될 경우 Currency 값이 변경
 *  가능할 경우 Property 값을 변경한다.
 */
IDE_RC mtuProperty::changeNLS_CURRENCY(  idvSQL * /* aStatistics */, SChar *, 
                                        void  *,
                                        void  * aNewValue,
                                        void  * )
{
    SChar * sValue = ( SChar * )aNewValue;
    SInt    sLen   = 0;

    IDE_TEST( mtlTerritory::checkCurrencySymbol( sValue )
              != IDE_SUCCESS );
    
    sLen = idlOS::strlen( sValue );
    
    if ( sLen > MTL_TERRITORY_CURRENCY_LEN )
    {
        sLen = MTL_TERRITORY_CURRENCY_LEN;
    }
    else
    {
        /* Nothgin to do */
    }

    idlOS::memcpy( MTU_PROPERTY(mNlsCurrency), sValue, sLen );
    MTU_PROPERTY(mNlsCurrency)[sLen] = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

/**
 * PROJ-2208 changeNLS_NUMERIC_CHARACTERS
 *
 *  alter system으로 인해 Property 값이 변경 될 경우 NLS_NUMERIC_CHARACTERS 값이 변경
 *  가능할 경우 Property 값을 변경한다.
 */
IDE_RC mtuProperty::changeNLS_NUMERIC_CHARACTERS( idvSQL * /* aStatistics */,
                                                  SChar *,
                                                  void  *,
                                                  void  * aNewValue,
                                                  void  * )
{
    SChar * sValue = ( SChar * )aNewValue;

    IDE_TEST( mtlTerritory::checkNlsNumericChar( sValue )
              != IDE_SUCCESS );
    
    idlOS::memcpy( MTU_PROPERTY(mNlsNumChar), sValue, MTL_TERRITORY_NUMERIC_CHAR_LEN );
    MTU_PROPERTY(mNlsNumChar)[MTL_TERRITORY_NUMERIC_CHAR_LEN] = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

// BUG-37247
IDE_RC mtuProperty::changeGROUP_CONCAT_PRECISION( idvSQL *, SChar *,
                                                  void  *,
                                                  void  * aNewValue,
                                                  void  * )
{
    idlOS::memcpy( &MTU_PROPERTY(mGroupConcatPrecision),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-38416 count(column) to count(*)
IDE_RC mtuProperty::changeCOUNT_COLUMN_TO_COUNT_ASTAR(
                        idvSQL * /* aStatistics */,
                        SChar *,
                        void  *,
                        void  * aNewValue,
                        void  * )
{
    idlOS::memcpy( &MTU_PROPERTY(mCountColumnToCountAStar),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-38101
IDE_RC mtuProperty::changeCASE_SENSITIVE_PASSWORD( idvSQL *, SChar *,
                                                   void  *,
                                                   void  * aNewValue,
                                                   void  * )
{
    idlOS::memcpy( &MTU_PROPERTY(mCaseSensitivePassword),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-38842
IDE_RC mtuProperty::changeCLOB_TO_VARCHAR_PRECISION( idvSQL *, SChar *,
                                                     void  *,
                                                     void  * aNewValue,
                                                     void  * )
{
    idlOS::memcpy( &MTU_PROPERTY(mClob2VarcharPrecision),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-41194
IDE_RC mtuProperty::changeDOUBLE_TO_NUMERIC_FAST_CONV( idvSQL *, SChar *,
                                                       void  *,
                                                       void  * aNewValue,
                                                       void  * )
{
    idlOS::memcpy( &MTU_PROPERTY(mDoubleToNumericFastConversion),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-41555 DBMS PIPE
IDE_RC mtuProperty::changeMSG_QUEUE_PERMISSION( idvSQL *,
                                                SChar *,
                                                void  *,
                                                void  * aNewValue,
                                                void  * )
{
    idlOS::memcpy( &MTU_PROPERTY(mMsgQueuePermission),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-41944
IDE_RC mtuProperty::changeARITHMETIC_OPERATION_MODE( idvSQL *,
                                                     SChar *,
                                                     void  *,
                                                     void  * aNewValue,
                                                     void  * )
{
    idlOS::memcpy( &MTU_PROPERTY(mArithmeticOpMode),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}
