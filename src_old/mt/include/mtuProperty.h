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
 * $Id: mtuProperty.h 83565 2018-07-24 09:07:14Z andrew.shin $
 *
 * MT에서 사용하는 System Property에 대한 정의
 * A4에서 제공하는 Property 관리자를 이용하여 처리한다.
 *
 *        
 * 
 **********************************************************************/

#ifndef _O_MTU_PROPERTY_H
#define _O_MTU_PROPERTY_H 1

#include <idl.h>
#include <idp.h>
#include <mtc.h>

// BUG-41253 MT shared property
#ifdef ALTIBASE_PRODUCT_XDB
#define MTU_PROPERTY( aProperty ) (mtuProperty::mSharedProperty->aProperty)
#else
#define MTU_PROPERTY( aProperty ) (mtuProperty::mStaticProperty.aProperty)
#endif

#define MTU_DEFAULT_DATE_FORMAT       ( MTU_PROPERTY(mDateFormat) )
#define MTU_DEFAULT_DATE_FORMAT_LEN   (idlOS::strlen(MTU_DEFAULT_DATE_FORMAT))

#define MTU_NLS_COMP_MODE             ( MTU_PROPERTY(mNlsCompMode) )

// PROJ-1579 NCHAR
#define MTU_NLS_NCHAR_CONV_EXCP       ( MTU_PROPERTY(mNlsNcharConvExcp) )
#define MTU_NLS_NCHAR_LITERAL_REPLACE ( MTU_PROPERTY(mNlsNcharLiteralReplace) )

// BUG-34342
#define MTU_ARITHMETIC_OP_MODE        ( MTU_PROPERTY(mArithmeticOpMode) )

/* PROJ-2208 Multi Currency */
#define MTU_NLS_TERRITORY     ( MTU_PROPERTY(mNlsTerritory) )
#define MTU_NLS_TERRITORY_LEN ( idlOS::strlen(MTU_NLS_TERRITORY) )
#define MTU_NLS_ISO_CURRENCY  ( MTU_PROPERTY(mNlsISOCurrency) )
#define MTU_NLS_ISO_CURRENCY_LEN ( idlOS::strlen(MTU_NLS_ISO_CURRENCY) )
#define MTU_NLS_CURRENCY      ( MTU_PROPERTY(mNlsCurrency) )
#define MTU_NLS_CURRENCY_LEN  ( idlOS::strlen(MTU_NLS_CURRENCY) )
#define MTU_NLS_NUM_CHAR      ( MTU_PROPERTY(mNlsNumChar) )
#define MTU_NLS_NUM_CHAR_LEN  ( idlOS::strlen(MTU_NLS_NUM_CHAR) )

/* PROJ-2209 DBTIMEZONE */
#define MTU_DB_TIMEZONE_STRING        ( MTU_PROPERTY(mDBTimezoneString) )
#define MTU_DB_TIMEZONE_STRING_LEN    ( idlOS::strlen(MTU_DB_TIMEZONE_STRING) )
#define MTU_DB_TIMEZONE_SECOND        ( MTU_PROPERTY(mDBTimezoneSecond) )

#define MTU_OS_TIMEZONE_STRING        ( MTU_PROPERTY(mOSTimezoneString) )
#define MTU_OS_TIMEZONE_STRING_LEN    ( idlOS::strlen(MTU_OS_TIMEZONE_STRING) )
#define MTU_OS_TIMEZONE_SECOND        ( MTU_PROPERTY(mOSTimezoneSecond) )

// PROJ 1753 One Pass Like
#define MTU_LIKE_OP_USE_MODULE ( MTU_PROPERTY(mLikeOpUseOldModule) )

// BUG-38416 count(column) to count(*)
#define MTU_COUNT_COLUMN_TO_COUNT_ASTAR ( MTU_PROPERTY(mCountColumnToCountAStar) )

#define MTU_LIKE_USE_NEW_MODULE ( (UInt)0)
#define MTU_LIKE_USE_OLD_MODULE ( (UInt)1)

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
#define MTU_LOB_OBJECT_BUFFER_SIZE  ( MTU_PROPERTY(mLOBObjectBufSize) )

// BUG-37247
#define MTU_GROUP_CONCAT_PRECISION  ( MTU_PROPERTY(mGroupConcatPrecision) )

// BUG-38101
#define MTU_CASE_SENSITIVE_PASSWORD    ( MTU_PROPERTY(mCaseSensitivePassword) )

// BUG-38842
#define MTU_CLOB_TO_VARCHAR_PRECISION  ( MTU_PROPERTY(mClob2VarcharPrecision) )

// BUG-41194
#define MTU_DOUBLE_TO_NUMERIC_FAST_CONV ( MTU_PROPERTY(mDoubleToNumericFastConversion) )

// BUG-41555 DBMS PIPE
#define MTU_MSG_QUEUE_PERMISSION ( MTU_PROPERTY(mMsgQueuePermission) )

/* BUG-46267 */
#define MTU_NUMBER_CONVERSION_MODE ( MTU_PROPERTY( mNumberConversionMode ) )

/* BUG-46267 */
#define MTU_NUMBER_CONVERSION_DISABLE ( (UInt)0 )
#define MTU_NUMBER_CONVERSION_ENABLE  ( (UInt)1 )

typedef enum
{
    MTU_NLS_COMP_BINARY = 0,
    MTU_NLS_COMP_ANSI
} mtuNlsCompMode;

// BUG-41253 MT shared property
typedef struct mtuProperties
{
    idShmAddr          mAddrSelf;

    //-----------------------------------
    // mt properties
    //-----------------------------------
    
    SChar              mDateFormat[IDP_MAX_VALUE_LEN];
    mtuNlsCompMode     mNlsCompMode;

    // PROJ-1579 NCHAR
    UInt               mNlsNcharConvExcp;
    UInt               mNlsNcharLiteralReplace;

    // BUG-34342
    mtcArithmeticOpMode  mArithmeticOpMode;
    
    /* PROJ-2208 Multi Currency */
    SChar              mNlsTerritory[IDP_MAX_VALUE_LEN];
    SChar              mNlsISOCurrency[IDP_MAX_VALUE_LEN];
    SChar              mNlsCurrency[IDP_MAX_VALUE_LEN];
    SChar              mNlsNumChar[IDP_MAX_VALUE_LEN];
    
    /* PROJ -1753 One Pass LIKE */
    UInt               mLikeOpUseOldModule;

    // BUG-38416 count(column) to count(*)
    UInt               mCountColumnToCountAStar;

    /* PROJ-2209 DBTIMEZONE */
    SChar              mDBTimezoneString[MTC_TIMEZONE_NAME_LEN + 1];
    SLong              mDBTimezoneSecond;
	
    SChar              mOSTimezoneString[MTC_TIMEZONE_NAME_LEN + 1];
    SLong              mOSTimezoneSecond;

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    UInt               mLOBObjectBufSize;

    // BUG-37247
    UInt               mGroupConcatPrecision;

    // BUG-38101
    UInt               mCaseSensitivePassword;
    
    // BUG-38842
    UInt               mClob2VarcharPrecision;
    
    // BUG-41194
    UInt               mDoubleToNumericFastConversion;

    // BUG-41555 DBMS PIPE
    UInt               mMsgQueuePermission;

    /* BUG-46267 */
    UInt               mNumberConversionMode;
} mtuProperties;
    
class mtuProperty
{
public:
#ifdef ALTIBASE_PRODUCT_XDB
    static mtuProperties  * mSharedProperty;
#else
    static mtuProperties    mStaticProperty;
#endif
    
public:
    static IDE_RC initProperty( idvSQL * aStatistics );
    static IDE_RC finalProperty( idvSQL * aStatistics );
    
    // System 구동 시 관련 Property를 Loading 함.
    static IDE_RC load();

    // System 구동 시 관련 Property의 callback 을 등록함
    static IDE_RC setupUpdateCallback();

#ifdef ALTIBASE_PRODUCT_XDB
    static IDE_RC allocShm( idvSQL * aStatistics );
    static IDE_RC freeShm( idvSQL * aStatistics );
#endif

    //----------------------------------------------
    // Writable Property를 위한 Call Back 함수들
    //----------------------------------------------

    static IDE_RC changeDEFAULT_DATE_FORMAT( idvSQL * aStatistics,
                                             SChar*,
                                             void *,
                                             void *  aNewValue,
                                             void * );

    // PROJ-1579 NCHAR
    static IDE_RC changeNLS_NCHAR_CONV_EXCP( idvSQL * aStatistics,
                                             SChar*,
                                             void *,
                                             void *  aNewValue,
                                             void * );

    /* PROJ-2208 Multi Currency */
    static IDE_RC loadNLSCurrencyProperty( void );

    static IDE_RC changeNLS_TERRITORY( idvSQL * aStatistics,
                                       SChar *,
                                       void  *,
                                       void  * aNewValue,
                                       void  * );

    static IDE_RC changeNLS_ISO_CURRENCY( idvSQL * aStatistics, 
                                          SChar *,
                                          void  *,
                                          void  * aNewValue,
                                          void  * );

    static IDE_RC changeNLS_CURRENCY( idvSQL * aStatistics, 
                                      SChar *,
                                      void  *,
                                      void  * aNewValue,
                                      void  * );

    static IDE_RC changeNLS_NUMERIC_CHARACTERS( idvSQL * aStatistics,
                                                SChar *,
                                                void  *,
                                                void  * aNewValue,
                                                void  * );

    static IDE_RC changeGROUP_CONCAT_PRECISION( idvSQL * aStatistics,
                                                SChar *,
                                                void  *,
                                                void  * aNewValue,
                                                void  * );

    static IDE_RC changeCOUNT_COLUMN_TO_COUNT_ASTAR( idvSQL * aStatistics,
                                                     SChar *,
                                                     void  *,
                                                     void  * aNewValue,
                                                     void  * );

    static IDE_RC changeCASE_SENSITIVE_PASSWORD( idvSQL * aStatistics,
                                                 SChar *,
                                                 void  *,
                                                 void  * aNewValue,
                                                 void  * );

    static IDE_RC changeCLOB_TO_VARCHAR_PRECISION( idvSQL * aStatistics,
                                                   SChar *,
                                                   void  *,
                                                   void  * aNewValue,
                                                   void  * );

    static IDE_RC changeDOUBLE_TO_NUMERIC_FAST_CONV( idvSQL * aStatistics,
                                                     SChar *,
                                                     void  *,
                                                     void  * aNewValue,
                                                     void  * );
    
    // BUG-41555 DBMS PIPE    
    static IDE_RC changeMSG_QUEUE_PERMISSION( idvSQL * aStatistics,
                                              SChar *,
                                              void  *,
                                              void  * aNewValue,
                                              void  * );
    
    static IDE_RC changeARITHMETIC_OPERATION_MODE( idvSQL * aStatistics,
                                                   SChar *,
                                                   void  *,
                                                   void  * aNewValue,
                                                   void  * );
};

#endif /* _O_MTU_PROPERTY_H */

