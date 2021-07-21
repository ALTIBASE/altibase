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
 
#include <idsCrypt.h>
#include <ideLog.h>

UInt idsCrypt::i64c( UInt i )
{
#define IDE_FN "UInt idsCrypt::i64c( UInt i )"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("i=%d", i));

    if( i <= 0 )            return ( '.' );
    if( i == 1 )            return ( '/' );
    if( i >=  2 && i < 12 ) return ( '0' - 2 + i );
    if( i >= 12 && i < 38 ) return ( 'A' - 12 + i );
    if( i >= 38 && i < 63 ) return ( 'a' - 38 + i );
    return ( 'z' );

#undef IDE_FN
}

UChar* idsCrypt::makeSalt( void )
{

#define IDE_FN "UChar* idsCrypt::makeSalt( void )"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    time_t now;

    if (idlOS::time( &now ) < 0)
    {
#if !defined(VC_WINCE)
        x_ += idlOS::getpid() + clock();
#else
        x_ += idlOS::getpid();
#endif
    }
    else
    {
#if !defined(VC_WINCE)
        x_ += now + idlOS::getpid() + clock();
#else
        x_ += now + idlOS::getpid();
#endif
    }
    
    salt_[0] = i64c( ( ( x_ >> 18 ) ^ ( x_ >> 6 ) ) & 0x3F );
    salt_[1] = i64c( ( ( x_ >> 12 ) ^ x_ ) & 0x3F );
    salt_[2] = '\0';

    return salt_;


#undef IDE_FN
}

idsCrypt::idsCrypt()
{
#define IDE_FN "idsCrypt::idsCrypt()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    x_ = 0;
#undef IDE_FN
}

UChar* idsCrypt::crypt( UChar* passwd, UChar* salt )
{
#define IDE_FN "UChar* idsCrypt::crypt( UChar* passwd, UChar* salt )"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#if defined(ITRON)
    return (UChar *)"empty";
#else
    UChar* result;

    if( salt == NULL ){
	salt = makeSalt();
    }

    idlOS::memset(&crypt_data_, 0, sizeof(crypt_data_));

    result = idlOS::crypt(passwd, salt, &crypt_data_);

    if( result != NULL )
    {
        idlOS::strcpy( (char*)result_, (char*)result );
    }

    return ( result != NULL ? result_ : result );
#endif

#undef IDE_FN
}

/* BUG-33207 Limitation for the length of input password should be shortened */
UInt idsCrypt::getCryptSize()
{
#define IDE_FN "idsCrypt::getCryptSize()"
#if defined(VC_WIN32) || defined(WRS_VXWORKS) || ( ( defined(SPARC_SOLARIS) || defined(X86_SOLARIS) ) && (OS_MAJORVER == 2) && (OS_MINORVER > 7) ) || defined(ANDROID) || defined(SYMBIAN)
    return 11;
#else
    return 8;
#endif

#undef IDE_FN
}
