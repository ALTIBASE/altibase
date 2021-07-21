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
 
/***********************************************************************
 * $Id: idnCharSet.h 80575 2017-07-21 07:06:35Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_IDNCHARSET_H_
#define  _O_IDNCHARSET_H_  1

#define IDN_MAX_CHAR_SET_LEN ((UInt)40)

// ASCII 문자인지 체크
#define IDN_IS_ASCII(c)       ( (((c) & ~0x7F) == 0) ? ID_TRUE : ID_FALSE )

#define IDN_IS_UTF16_ASCII_PTR(s) ( ( ( *(UChar*)s == 0 ) && ( (*((UChar*)s+1) & ~0x7F) == 0 ) ) ? ID_TRUE : ID_FALSE )

// 캐릭터 셋 변환 시, 변환할 문자가 없는 경우 나타내는 문자
// ASCII의 경우 '?'이다.
// WE8ISO8859P1의 경우 거꾸로 물음표(0xBF)가 default_replace_character이다.
// (현재 WE8ISO8859P1는 지원하지 않으므로 고려하지 않는다.)
#define IDN_ASCII_DEFAULT_REPLACE_CHARACTER ((UChar)(0x3F))

// space character ' '
#define IDN_ASCII_SPACE ((UChar)(0x20))

#define IDN_IS_UTF16_ALNUM_PTR(s)                                   \
(                                                                   \
  (                                                                 \
    ( *(UChar*)s == 0 )                                             \
     &&                                                             \
    (                                                               \
        (((*((UChar*)s+1)) >= 'a') && ((*((UChar*)s+1)) <= 'z')) || \
        (((*((UChar*)s+1)) >= 'A') && ((*((UChar*)s+1)) <= 'Z')) || \
        (((*((UChar*)s+1)) >= '0') && ((*((UChar*)s+1)) <= '9'))    \
    )                                                               \
  ) ? ID_TRUE : ID_FALSE                                            \
)


// PROJ-1579 NCHAR
// 지원하는 캐릭터셋의 id를 지정한다. 
// conversion matrix와 연관이 있기 때문에 0부터 순서대로 번호를 매겨야 함
typedef enum idnCharSetList
{
    IDN_ASCII_ID = 0,
    IDN_KSC5601_ID,
    IDN_MS949_ID,
    IDN_EUCJP_ID,
    IDN_SHIFTJIS_ID,
    IDN_MS932_ID,   /* PROJ-2590 [기능성] CP932 database character set 지원 */
    IDN_BIG5_ID,
    IDN_GB231280_ID,
    /* PORJ-2414 [기능성] GBK, CP936 character set 추가 */
    IDN_MS936_ID,
    IDN_UTF8_ID,
    IDN_UTF16_ID,
    IDN_MAX_CHARSET_ID
} idnCharSetList;

typedef enum idnCharFeature
{
    IDN_CF_UNKNOWN,
    IDN_CF_NEG_TRAIL,
    IDN_CF_POS_TRAIL,
    IDN_CF_SJIS
}idnCharFeature;

typedef struct Summary16 
{
  UShort indx; // index into big table
  UShort used; // bitmask of used entries
} Summary16;

#endif /* _O_IDNCHARSET_H_ */
 
