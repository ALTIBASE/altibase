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
 * $Id: acpChar.c 937 2007-10-04 18:14:00Z shsuh $
 ******************************************************************************/

#include <acpChar.h>


static acpCharClass gAcpCharClassTable[256] =
{
    0,                                                                /* -128 */
    0,                                                                /* -127 */
    0,                                                                /* -126 */
    0,                                                                /* -125 */
    0,                                                                /* -124 */
    0,                                                                /* -123 */
    0,                                                                /* -122 */
    0,                                                                /* -121 */
    0,                                                                /* -120 */
    0,                                                                /* -119 */
    0,                                                                /* -118 */
    0,                                                                /* -117 */
    0,                                                                /* -116 */
    0,                                                                /* -115 */
    0,                                                                /* -114 */
    0,                                                                /* -113 */
    0,                                                                /* -112 */
    0,                                                                /* -111 */
    0,                                                                /* -110 */
    0,                                                                /* -109 */
    0,                                                                /* -108 */
    0,                                                                /* -107 */
    0,                                                                /* -106 */
    0,                                                                /* -105 */
    0,                                                                /* -104 */
    0,                                                                /* -103 */
    0,                                                                /* -102 */
    0,                                                                /* -101 */
    0,                                                                /* -100 */
    0,                                                                 /* -99 */
    0,                                                                 /* -98 */
    0,                                                                 /* -97 */
    0,                                                                 /* -96 */
    0,                                                                 /* -95 */
    0,                                                                 /* -94 */
    0,                                                                 /* -93 */
    0,                                                                 /* -92 */
    0,                                                                 /* -91 */
    0,                                                                 /* -90 */
    0,                                                                 /* -89 */
    0,                                                                 /* -88 */
    0,                                                                 /* -87 */
    0,                                                                 /* -86 */
    0,                                                                 /* -85 */
    0,                                                                 /* -84 */
    0,                                                                 /* -83 */
    0,                                                                 /* -82 */
    0,                                                                 /* -81 */
    0,                                                                 /* -80 */
    0,                                                                 /* -79 */
    0,                                                                 /* -78 */
    0,                                                                 /* -77 */
    0,                                                                 /* -76 */
    0,                                                                 /* -75 */
    0,                                                                 /* -74 */
    0,                                                                 /* -73 */
    0,                                                                 /* -72 */
    0,                                                                 /* -71 */
    0,                                                                 /* -70 */
    0,                                                                 /* -69 */
    0,                                                                 /* -68 */
    0,                                                                 /* -67 */
    0,                                                                 /* -66 */
    0,                                                                 /* -65 */
    0,                                                                 /* -64 */
    0,                                                                 /* -63 */
    0,                                                                 /* -62 */
    0,                                                                 /* -61 */
    0,                                                                 /* -60 */
    0,                                                                 /* -59 */
    0,                                                                 /* -58 */
    0,                                                                 /* -57 */
    0,                                                                 /* -56 */
    0,                                                                 /* -55 */
    0,                                                                 /* -54 */
    0,                                                                 /* -53 */
    0,                                                                 /* -52 */
    0,                                                                 /* -51 */
    0,                                                                 /* -50 */
    0,                                                                 /* -49 */
    0,                                                                 /* -48 */
    0,                                                                 /* -47 */
    0,                                                                 /* -46 */
    0,                                                                 /* -45 */
    0,                                                                 /* -44 */
    0,                                                                 /* -43 */
    0,                                                                 /* -42 */
    0,                                                                 /* -41 */
    0,                                                                 /* -40 */
    0,                                                                 /* -39 */
    0,                                                                 /* -38 */
    0,                                                                 /* -37 */
    0,                                                                 /* -36 */
    0,                                                                 /* -35 */
    0,                                                                 /* -34 */
    0,                                                                 /* -33 */
    0,                                                                 /* -32 */
    0,                                                                 /* -31 */
    0,                                                                 /* -30 */
    0,                                                                 /* -29 */
    0,                                                                 /* -28 */
    0,                                                                 /* -27 */
    0,                                                                 /* -26 */
    0,                                                                 /* -25 */
    0,                                                                 /* -24 */
    0,                                                                 /* -23 */
    0,                                                                 /* -22 */
    0,                                                                 /* -21 */
    0,                                                                 /* -20 */
    0,                                                                 /* -19 */
    0,                                                                 /* -18 */
    0,                                                                 /* -17 */
    0,                                                                 /* -16 */
    0,                                                                 /* -15 */
    0,                                                                 /* -14 */
    0,                                                                 /* -13 */
    0,                                                                 /* -12 */
    0,                                                                 /* -11 */
    0,                                                                 /* -10 */
    0,                                                                 /* -9  */
    0,                                                                 /* -8  */
    0,                                                                 /* -7  */
    0,                                                                 /* -6  */
    0,                                                                 /* -5  */
    0,                                                                 /* -4  */
    0,                                                                 /* -3  */
    0,                                                                 /* -2  */
    0,                                                                 /* -1  */
    ACP_CHAR_CNTRL,                                                    /* 0   */
    ACP_CHAR_CNTRL,                                                    /* 1   */
    ACP_CHAR_CNTRL,                                                    /* 2   */
    ACP_CHAR_CNTRL,                                                    /* 3   */
    ACP_CHAR_CNTRL,                                                    /* 4   */
    ACP_CHAR_CNTRL,                                                    /* 5   */
    ACP_CHAR_CNTRL,                                                    /* 6   */
    ACP_CHAR_CNTRL,                                                    /* 7   */
    ACP_CHAR_CNTRL,                                                    /* 8   */
    ACP_CHAR_CNTRL | ACP_CHAR_SPACE,                                   /* 9   */
    ACP_CHAR_CNTRL | ACP_CHAR_SPACE,                                   /* 10  */
    ACP_CHAR_CNTRL | ACP_CHAR_SPACE,                                   /* 11  */
    ACP_CHAR_CNTRL | ACP_CHAR_SPACE,                                   /* 12  */
    ACP_CHAR_CNTRL | ACP_CHAR_SPACE,                                   /* 13  */
    ACP_CHAR_CNTRL,                                                    /* 14  */
    ACP_CHAR_CNTRL,                                                    /* 15  */
    ACP_CHAR_CNTRL,                                                    /* 16  */
    ACP_CHAR_CNTRL,                                                    /* 17  */
    ACP_CHAR_CNTRL,                                                    /* 18  */
    ACP_CHAR_CNTRL,                                                    /* 19  */
    ACP_CHAR_CNTRL,                                                    /* 20  */
    ACP_CHAR_CNTRL,                                                    /* 21  */
    ACP_CHAR_CNTRL,                                                    /* 22  */
    ACP_CHAR_CNTRL,                                                    /* 23  */
    ACP_CHAR_CNTRL,                                                    /* 24  */
    ACP_CHAR_CNTRL,                                                    /* 25  */
    ACP_CHAR_CNTRL,                                                    /* 26  */
    ACP_CHAR_CNTRL,                                                    /* 27  */
    ACP_CHAR_CNTRL,                                                    /* 28  */
    ACP_CHAR_CNTRL,                                                    /* 29  */
    ACP_CHAR_CNTRL,                                                    /* 30  */
    ACP_CHAR_CNTRL,                                                    /* 31  */
    ACP_CHAR_PRINT | ACP_CHAR_SPACE,                                   /* ' ' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '!' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '"' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '#' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '$' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '%' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '&' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                 /* '\'' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '(' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* ')' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '*' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '+' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* ',' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '-' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '.' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '/' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '0' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '1' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '2' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '3' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '4' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '5' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '6' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '7' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '8' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_DIGIT | ACP_CHAR_XDGIT, /* '9' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* ':' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* ';' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '<' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '=' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '>' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '?' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '@' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER | ACP_CHAR_XDGIT, /* 'A' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER | ACP_CHAR_XDGIT, /* 'B' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER | ACP_CHAR_XDGIT, /* 'C' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER | ACP_CHAR_XDGIT, /* 'D' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER | ACP_CHAR_XDGIT, /* 'E' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER | ACP_CHAR_XDGIT, /* 'F' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'G' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'H' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'I' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'J' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'K' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'L' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'M' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'N' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'O' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'P' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'Q' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'R' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'S' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'T' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'U' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'V' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'W' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'X' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'Y' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_UPPER,                  /* 'Z' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '[' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                 /* '\\' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* ']' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '^' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '_' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '`' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER | ACP_CHAR_XDGIT, /* 'a' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER | ACP_CHAR_XDGIT, /* 'b' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER | ACP_CHAR_XDGIT, /* 'c' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER | ACP_CHAR_XDGIT, /* 'd' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER | ACP_CHAR_XDGIT, /* 'e' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER | ACP_CHAR_XDGIT, /* 'f' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'g' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'h' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'i' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'j' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'k' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'l' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'm' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'n' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'o' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'p' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'q' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'r' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 's' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 't' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'u' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'v' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'w' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'x' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'y' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_LOWER,                  /* 'z' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '{' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '|' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '}' */
    ACP_CHAR_GRAPH | ACP_CHAR_PRINT | ACP_CHAR_PUNCT,                  /* '~' */
    ACP_CHAR_CNTRL,                                                    /* 127 */
};

static acpCharClass *gAcpCharClass = gAcpCharClassTable + 128;


ACP_EXPORT acpCharClass acpCharClassOf(acp_char_t aChar)
{
    return gAcpCharClass[(acp_sint8_t)aChar];
}
