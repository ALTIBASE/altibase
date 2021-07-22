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
 
/*******************************************************************************
 * $Id: testAcpChar.c 3576 2008-11-12 06:29:21Z sjkim $
 ******************************************************************************/

#include <act.h>
#include <acpChar.h>

#if !defined(ALINT)
#include <ctype.h>
#endif


static acp_char_t gAcpCharToLowerTable[] =
{
    -128, -127, -126, -125, -124, -123, -122, -121, -120, -119, -118, -117,
    -116, -115, -114, -113, -112, -111, -110, -109, -108, -107, -106, -105,
    -104, -103, -102, -101, -100, -99,  -98,  -97,  -96,  -95,  -94,  -93,
    -92,  -91,  -90,  -89,  -88,  -87,  -86,  -85,  -84,  -83,  -82,  -81,
    -80,  -79,  -78,  -77,  -76,  -75,  -74,  -73,  -72,  -71,  -70,  -69,
    -68,  -67,  -66,  -65,  -64,  -63,  -62,  -61,  -60,  -59,  -58,  -57,
    -56,  -55,  -54,  -53,  -52,  -51,  -50,  -49,  -48,  -47,  -46,  -45,
    -44,  -43,  -42,  -41,  -40,  -39,  -38,  -37,  -36,  -35,  -34,  -33,
    -32,  -31,  -30,  -29,  -28,  -27,  -26,  -25,  -24,  -23,  -22,  -21,
    -20,  -19,  -18,  -17,  -16,  -15,  -14,  -13,  -12,  -11,  -10,  -9,
    -8,   -7,   -6,   -5,   -4,   -3,   -2,   -1,   0,    1,    2,    3,
    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,   15,
    16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,
    28,   29,   30,   31,
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+',
    ',', '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ':', ';', '<', '=', '>', '?', '@', 'a', 'b', 'c',
    'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[',
    '\\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
    't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',  127
};

static acp_char_t gAcpCharToUpperTable[] =
{
    -128, -127, -126, -125, -124, -123, -122, -121, -120, -119, -118, -117,
    -116, -115, -114, -113, -112, -111, -110, -109, -108, -107, -106, -105,
    -104, -103, -102, -101, -100, -99,  -98,  -97,  -96,  -95,  -94,  -93,
    -92,  -91,  -90,  -89,  -88,  -87,  -86,  -85,  -84,  -83,  -82,  -81,
    -80,  -79,  -78,  -77,  -76,  -75,  -74,  -73,  -72,  -71,  -70,  -69,
    -68,  -67,  -66,  -65,  -64,  -63,  -62,  -61,  -60,  -59,  -58,  -57,
    -56,  -55,  -54,  -53,  -52,  -51,  -50,  -49,  -48,  -47,  -46,  -45,
    -44,  -43,  -42,  -41,  -40,  -39,  -38,  -37,  -36,  -35,  -34,  -33,
    -32,  -31,  -30,  -29,  -28,  -27,  -26,  -25,  -24,  -23,  -22,  -21,
    -20,  -19,  -18,  -17,  -16,  -15,  -14,  -13,  -12,  -11,  -10,  -9,
    -8,   -7,   -6,   -5,   -4,   -3,   -2,   -1,   0,    1,    2,    3,
    4,    5,    6,    7,    8,     9,   10,   11,   12,   13,   14,   15,
    16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,
    28,   29,   30,   31,
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+',
    ',', '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ':', ';', '<', '=', '>', '?', '@', 'A', 'B', 'C',
    'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[',
    '\\', ']', '^', '_', '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
    'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '~',  127
};

static acp_bool_t gAcpCharIsAlnumTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '0', '1', '2', '3', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '4', '5', '6', '7', */
    ACP_TRUE,  ACP_TRUE,  ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '@', 'A', 'B', 'C', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'D', 'E', 'F', 'G', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'H', 'I', 'J', 'K', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'L', 'M', 'N', 'O', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'P', 'Q', 'R', 'S', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'T', 'U', 'V', 'W', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '`', 'a', 'b', 'c', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'd', 'e', 'f', 'g', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'h', 'i', 'j', 'k', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'l', 'm', 'n', 'o', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'p', 'q', 'r', 's', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  't', 'u', 'v', 'w', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsAlphaTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '0', '1', '2', '3', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '4', '5', '6', '7', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '@', 'A', 'B', 'C', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'D', 'E', 'F', 'G', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'H', 'I', 'J', 'K', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'L', 'M', 'N', 'O', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'P', 'Q', 'R', 'S', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'T', 'U', 'V', 'W', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '`', 'a', 'b', 'c', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'd', 'e', 'f', 'g', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'h', 'i', 'j', 'k', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'l', 'm', 'n', 'o', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'p', 'q', 'r', 's', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  't', 'u', 'v', 'w', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsAsciiTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  0,     1,    2,    3, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  4,     5,    6,    7, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  8,     9,   10,   11, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  12,   13,   14,   15, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  16,   17,   18,   19, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  20,   21,   22,   23, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  24,   25,   26,   27, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  28,   29,   30,   31, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ' ', '!', '"', '#', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '$', '%', '&', '\'', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '(', ')', '*', '+', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ',', '-', '.', '/', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '0', '1', '2', '3', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '4', '5', '6', '7', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '8', '9', ':', ';', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '<', '=', '>', '?', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '@', 'A', 'B', 'C', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'D', 'E', 'F', 'G', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'H', 'I', 'J', 'K', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'L', 'M', 'N', 'O', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'P', 'Q', 'R', 'S', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'T', 'U', 'V', 'W', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'X', 'Y', 'Z', '[', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '\\', ']', '^', '_', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '`', 'a', 'b', 'c', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'd', 'e', 'f', 'g', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'h', 'i', 'j', 'k', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'l', 'm', 'n', 'o', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'p', 'q', 'r', 's', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  't', 'u', 'v', 'w', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'x', 'y', 'z', '{', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE   /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsCntrlTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  0,     1,    2,    3, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  4,     5,    6,    7, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  8,     9,   10,   11, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  12,   13,   14,   15, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  16,   17,   18,   19, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  20,   21,   22,   23, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  24,   25,   26,   27, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '0', '1', '2', '3', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '4', '5', '6', '7', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '@', 'A', 'B', 'C', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'D', 'E', 'F', 'G', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'H', 'I', 'J', 'K', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'L', 'M', 'N', 'O', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'P', 'Q', 'R', 'S', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'T', 'U', 'V', 'W', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '`', 'a', 'b', 'c', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'd', 'e', 'f', 'g', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'h', 'i', 'j', 'k', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'l', 'm', 'n', 'o', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'p', 'q', 'r', 's', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  't', 'u', 'v', 'w', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_TRUE   /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsDigitTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '0', '1', '2', '3', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '4', '5', '6', '7', */
    ACP_TRUE,  ACP_TRUE,  ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '@', 'A', 'B', 'C', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'D', 'E', 'F', 'G', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'H', 'I', 'J', 'K', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'L', 'M', 'N', 'O', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'P', 'Q', 'R', 'S', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'T', 'U', 'V', 'W', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '`', 'a', 'b', 'c', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'd', 'e', 'f', 'g', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'h', 'i', 'j', 'k', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'l', 'm', 'n', 'o', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'p', 'q', 'r', 's', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  't', 'u', 'v', 'w', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsGraphTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ' ', '!', '"', '#', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '$', '%', '&', '\'', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '(', ')', '*', '+', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ',', '-', '.', '/', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '0', '1', '2', '3', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '4', '5', '6', '7', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '8', '9', ':', ';', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '<', '=', '>', '?', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '@', 'A', 'B', 'C', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'D', 'E', 'F', 'G', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'H', 'I', 'J', 'K', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'L', 'M', 'N', 'O', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'P', 'Q', 'R', 'S', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'T', 'U', 'V', 'W', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'X', 'Y', 'Z', '[', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '\\', ']', '^', '_', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '`', 'a', 'b', 'c', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'd', 'e', 'f', 'g', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'h', 'i', 'j', 'k', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'l', 'm', 'n', 'o', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'p', 'q', 'r', 's', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  't', 'u', 'v', 'w', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'x', 'y', 'z', '{', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsLowerTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '0', '1', '2', '3', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '4', '5', '6', '7', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '@', 'A', 'B', 'C', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'D', 'E', 'F', 'G', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'H', 'I', 'J', 'K', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'L', 'M', 'N', 'O', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'P', 'Q', 'R', 'S', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'T', 'U', 'V', 'W', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '`', 'a', 'b', 'c', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'd', 'e', 'f', 'g', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'h', 'i', 'j', 'k', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'l', 'm', 'n', 'o', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'p', 'q', 'r', 's', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  't', 'u', 'v', 'w', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsPrintTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ' ', '!', '"', '#', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '$', '%', '&', '\'', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '(', ')', '*', '+', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ',', '-', '.', '/', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '0', '1', '2', '3', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '4', '5', '6', '7', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '8', '9', ':', ';', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '<', '=', '>', '?', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '@', 'A', 'B', 'C', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'D', 'E', 'F', 'G', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'H', 'I', 'J', 'K', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'L', 'M', 'N', 'O', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'P', 'Q', 'R', 'S', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'T', 'U', 'V', 'W', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'X', 'Y', 'Z', '[', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '\\', ']', '^', '_', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '`', 'a', 'b', 'c', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'd', 'e', 'f', 'g', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'h', 'i', 'j', 'k', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'l', 'm', 'n', 'o', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'p', 'q', 'r', 's', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  't', 'u', 'v', 'w', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'x', 'y', 'z', '{', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsPunctTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ' ', '!', '"', '#', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '$', '%', '&', '\'', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '(', ')', '*', '+', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  ',', '-', '.', '/', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '0', '1', '2', '3', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '4', '5', '6', '7', */
    ACP_FALSE, ACP_FALSE, ACP_TRUE,  ACP_TRUE,  /*  '8', '9', ':', ';', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '<', '=', '>', '?', */
    ACP_TRUE,  ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '@', 'A', 'B', 'C', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'D', 'E', 'F', 'G', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'H', 'I', 'J', 'K', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'L', 'M', 'N', 'O', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'P', 'Q', 'R', 'S', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'T', 'U', 'V', 'W', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_TRUE,  /*  'X', 'Y', 'Z', '[', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '\\', ']', '^', '_', */
    ACP_TRUE,  ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '`', 'a', 'b', 'c', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'd', 'e', 'f', 'g', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'h', 'i', 'j', 'k', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'l', 'm', 'n', 'o', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'p', 'q', 'r', 's', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  't', 'u', 'v', 'w', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_TRUE,  /*  'x', 'y', 'z', '{', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsSpaceTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  8,     9,   10,   11, */
    ACP_TRUE,  ACP_TRUE,  ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_TRUE,  ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '0', '1', '2', '3', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '4', '5', '6', '7', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '@', 'A', 'B', 'C', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'D', 'E', 'F', 'G', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'H', 'I', 'J', 'K', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'L', 'M', 'N', 'O', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'P', 'Q', 'R', 'S', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'T', 'U', 'V', 'W', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '`', 'a', 'b', 'c', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'd', 'e', 'f', 'g', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'h', 'i', 'j', 'k', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'l', 'm', 'n', 'o', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'p', 'q', 'r', 's', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  't', 'u', 'v', 'w', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsUpperTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '0', '1', '2', '3', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '4', '5', '6', '7', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '@', 'A', 'B', 'C', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'D', 'E', 'F', 'G', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'H', 'I', 'J', 'K', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'L', 'M', 'N', 'O', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'P', 'Q', 'R', 'S', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  'T', 'U', 'V', 'W', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '`', 'a', 'b', 'c', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'd', 'e', 'f', 'g', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'h', 'i', 'j', 'k', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'l', 'm', 'n', 'o', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'p', 'q', 'r', 's', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  't', 'u', 'v', 'w', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE  /*  '|', '}', '~',  127 */
};

static acp_bool_t gAcpCharIsXdigitTable[] =
{
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -128, -127, -126, -125, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -124, -123, -122, -121, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -120, -119, -118, -117, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -116, -115, -114, -113, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -112, -111, -110, -109, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -108, -107, -106, -105, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -104, -103, -102, -101, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -100, -99,  -98,  -97, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -96,  -95,  -94,  -93, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -92,  -91,  -90,  -89, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -88,  -87,  -86,  -85, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -84,  -83,  -82,  -81, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -80,  -79,  -78,  -77, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -76,  -75,  -74,  -73, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -72,  -71,  -70,  -69, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -68,  -67,  -66,  -65, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -64,  -63,  -62,  -61, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -60,  -59,  -58,  -57, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -56,  -55,  -54,  -53, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -52,  -51,  -50,  -49, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -48,  -47,  -46,  -45, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -44,  -43,  -42,  -41, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -40,  -39,  -38,  -37, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -36,  -35,  -34,  -33, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -32,  -31,  -30,  -29, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -28,  -27,  -26,  -25, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -24,  -23,  -22,  -21, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -20,  -19,  -18,  -17, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -16,  -15,  -14,  -13, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -12,  -11,  -10,  -9, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -8,   -7,   -6,   -5, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  -4,   -3,   -2,   -1, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  0,     1,    2,    3, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  4,     5,    6,    7, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  8,     9,   10,   11, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  12,   13,   14,   15, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  16,   17,   18,   19, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  20,   21,   22,   23, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  24,   25,   26,   27, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  28,   29,   30,   31, */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ' ', '!', '"', '#', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '$', '%', '&', '\'', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '(', ')', '*', '+', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  ',', '-', '.', '/', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '0', '1', '2', '3', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '4', '5', '6', '7', */
    ACP_TRUE,  ACP_TRUE,  ACP_FALSE, ACP_FALSE, /*  '8', '9', ':', ';', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '<', '=', '>', '?', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '@', 'A', 'B', 'C', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'D', 'E', 'F', 'G', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'H', 'I', 'J', 'K', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'L', 'M', 'N', 'O', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'P', 'Q', 'R', 'S', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'T', 'U', 'V', 'W', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'X', 'Y', 'Z', '[', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  '\\', ']', '^', '_', */
    ACP_FALSE, ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  /*  '`', 'a', 'b', 'c', */
    ACP_TRUE,  ACP_TRUE,  ACP_TRUE,  ACP_FALSE, /*  'd', 'e', 'f', 'g', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'h', 'i', 'j', 'k', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'l', 'm', 'n', 'o', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'p', 'q', 'r', 's', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  't', 'u', 'v', 'w', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE, /*  'x', 'y', 'z', '{', */
    ACP_FALSE, ACP_FALSE, ACP_FALSE, ACP_FALSE  /*  '|', '}', '~',  127 */
};


static acp_char_t *gAcpCharToLower  = gAcpCharToLowerTable  + 128;
static acp_char_t *gAcpCharToUpper  = gAcpCharToUpperTable  + 128;
static acp_bool_t *gAcpCharIsAlnum  = gAcpCharIsAlnumTable  + 128;
static acp_bool_t *gAcpCharIsAlpha  = gAcpCharIsAlphaTable  + 128;
static acp_bool_t *gAcpCharIsAscii  = gAcpCharIsAsciiTable  + 128;
static acp_bool_t *gAcpCharIsCntrl  = gAcpCharIsCntrlTable  + 128;
static acp_bool_t *gAcpCharIsDigit  = gAcpCharIsDigitTable  + 128;
static acp_bool_t *gAcpCharIsGraph  = gAcpCharIsGraphTable  + 128;
static acp_bool_t *gAcpCharIsLower  = gAcpCharIsLowerTable  + 128;
static acp_bool_t *gAcpCharIsPrint  = gAcpCharIsPrintTable  + 128;
static acp_bool_t *gAcpCharIsPunct  = gAcpCharIsPunctTable  + 128;
static acp_bool_t *gAcpCharIsSpace  = gAcpCharIsSpaceTable  + 128;
static acp_bool_t *gAcpCharIsUpper  = gAcpCharIsUpperTable  + 128;
static acp_bool_t *gAcpCharIsXdigit = gAcpCharIsXdigitTable + 128;


#define TEST_WITH_RESULT_TABLE(aFunc, aTable, aArg)             \
    do                                                          \
    {                                                           \
        ACT_CHECK_DESC(aFunc(aArg) == aTable[aArg],             \
                       (#aFunc "(%d) should return %d but %d",  \
                        aArg,                                   \
                        aTable[aArg],                           \
                        aFunc(aArg)));                          \
    } while (0)

#define TEST_WITH_STD_CTYPE_VALUE(aAcpFunc, aStdFunc, aArg)     \
    do                                                          \
    {                                                           \
        ACT_CHECK_DESC(aAcpFunc(aArg) == aStdFunc(aArg),        \
                       (#aAcpFunc "(%d) should return %d "      \
                        "but %d",                               \
                        aArg,                                   \
                        aStdFunc(aArg),                         \
                        aAcpFunc(aArg)));                       \
    } while (0)

#define TEST_WITH_STD_CTYPE_BOOL(aAcpFunc, aStdFunc, aArg)              \
    do                                                                  \
    {                                                                   \
        ACT_CHECK_DESC(aAcpFunc(aArg) ==                                \
                       (aStdFunc(aArg) ? ACP_TRUE : ACP_FALSE),         \
                       (#aAcpFunc "(%d) should return %d "              \
                        "but %d",                                       \
                        aArg,                                           \
                        (aStdFunc(aArg) ? ACP_TRUE : ACP_FALSE),        \
                        aAcpFunc(aArg)));                               \
    } while (0)

static void testAll(void)
{
    acp_sint32_t i;

    ACT_CHECK(acpCharToLower(0) == 0);
    ACT_CHECK(acpCharToUpper(0) == 0);

    for (i = ACP_SINT8_MIN; i <= ACP_SINT8_MAX; i++)
    {
        TEST_WITH_RESULT_TABLE(acpCharToLower, gAcpCharToLower, i);
        TEST_WITH_RESULT_TABLE(acpCharToUpper, gAcpCharToUpper, i);
        TEST_WITH_RESULT_TABLE(acpCharIsAlnum, gAcpCharIsAlnum, i);
        TEST_WITH_RESULT_TABLE(acpCharIsAlpha, gAcpCharIsAlpha, i);
        TEST_WITH_RESULT_TABLE(acpCharIsAscii, gAcpCharIsAscii, i);
        TEST_WITH_RESULT_TABLE(acpCharIsCntrl, gAcpCharIsCntrl, i);
        TEST_WITH_RESULT_TABLE(acpCharIsDigit, gAcpCharIsDigit, i);
        TEST_WITH_RESULT_TABLE(acpCharIsGraph, gAcpCharIsGraph, i);
        TEST_WITH_RESULT_TABLE(acpCharIsLower, gAcpCharIsLower, i);
        TEST_WITH_RESULT_TABLE(acpCharIsPrint, gAcpCharIsPrint, i);
        TEST_WITH_RESULT_TABLE(acpCharIsPunct, gAcpCharIsPunct, i);
        TEST_WITH_RESULT_TABLE(acpCharIsSpace, gAcpCharIsSpace, i);
        TEST_WITH_RESULT_TABLE(acpCharIsUpper, gAcpCharIsUpper, i);
        TEST_WITH_RESULT_TABLE(acpCharIsXdigit, gAcpCharIsXdigit, i);
    }
}

static void testAscii(void)
{
    acp_sint32_t i;

    for (i = 0; i <= 127; i++)
    {
#if !defined(ALINT)
        /* ------------------------------------------------
         * This area is for cross checking of the API result
         * comparing with the native function like tolower..
         * so, we disabled alint checking.
         * ----------------------------------------------*/
        
        TEST_WITH_STD_CTYPE_VALUE(acpCharToLower, tolower, i);
        TEST_WITH_STD_CTYPE_VALUE(acpCharToUpper, toupper, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsAlnum, isalnum, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsAlpha, isalpha, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsAscii, isascii, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsCntrl, iscntrl, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsDigit, isdigit, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsGraph, isgraph, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsLower, islower, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsPrint, isprint, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsPunct, ispunct, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsSpace, isspace, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsUpper, isupper, i);
        TEST_WITH_STD_CTYPE_BOOL(acpCharIsXdigit, isxdigit, i);
#endif        
    }
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testAll();
    testAscii();

    ACT_TEST_END();

    return 0;
}
