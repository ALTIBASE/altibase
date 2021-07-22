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
 * $Id: acpPrintf.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_PRINTF_H_)
#define _O_ACP_PRINTF_H_

/**
 * @file
 * @ingroup CoreChar
 * @ingroup CoreFile
 *
 * Altibase Core implementation of standard printf() functions
 *
 * @section intro Introduction
 * The acpPrintf() family of functions produces output according to a @a aFormat
 * as described below.
 * The acpPrintf() and acpVprintf() functions write output to stdout, the
 * standard output stream; acpSnprintf() and acpVsnprintf() write to the string
 * buffer.
 *
 * The functions acpVprintf(), acpVsnprintf() are equivalent to the functions
 * acpPrintf(), acpSnprintf(), respectively, except that they are called with a
 * va_list instead of a variable number of arguments. These functions do not
 * call the va_end macro. Consequently, the value of @a aArgs is undefined
 * after the call. The application should call va_end(@a aArgs) itself
 * afterwards.
 *
 * These functions write the output under the control of a @a aFormat string
 * that specifies how subsequent arguments (or arguments accessed via the
 * variable-length argument facilities of stdarg(3)) are converted for output.
 *
 * @section return_value Return value
 * These functions return #ACP_RC_SUCCESS if success or error code if fail.
 * The acpSnprintf() and acpVsnprintf() functions will write at most @a aSize-1
 * of the characters printed into the output buffer (the @a aSize'th character
 * then gets the terminating '\\0'); if the output was truncated due to this
 * limit then return #ACP_RC_ETRUNC.
 *
 * @section format_string Format of the format string
 * The format string is composed of zero or more directives: ordinary characters
 * (not %), which are copied unchanged to the output stream; and conversion
 * specifications, each of which results in fetching zero or more subsequent
 * arguments. Each conversion specification is introduced by the % character.
 * The arguments must correspond properly (after type promotion) with the
 * conversion specifier. After the %, the following appear in sequence:
 *
 * @subsection argument_access The next argument to access
 * An optional field, consisting of a decimal digit string followed by a $,
 * specifying the next argument to access. If this field is not provied, the
 * argument following the last argument accessed will be used. Arguments are
 * numbered starting at 1. If unaccessed arguments in the format string are
 * interspersed with onces that are accessed the results will be indeterminate.
 *
 * @subsection flag Flags
 * Zero or more of the following flags:
 *
 * <table border="0" cellspacing="4" cellpadding="4">
 * <tr>
 * <td nowrap valign="top">'<b>#</b>'</td>
 * <td>
 * The value should be converted to an 'alternate form'. For @b c, @b d, @b i,
 * @b n, @b p, @b s, and @b u conversions, this option has no effect.
 * For @b o conversions, the precision of the number is increased to force the
 * first character of the output string to a zero (except if a zero value is
 * printed with an explicit precision of zero).
 * For @b x and @b X conversions, a non-zero result has the string '0x' (or '0X'
 * for @b X conversions) prepended to it.
 * For @b a, @b A, @b e, @b E, @b f, @b F, @b g, and @b G conversions, the
 * result will always contain a decimal point, even if no digits follow it
 * (normally, a decimal point appears in the results of those conversions only
 * if a digit follows).
 * For @b g and @b G conversion, trailing zeros are not removed from the result
 * as they whould otherwise be.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top">'<b>0</b>' (zero)</td>
 * <td>
 * Zero padding. For all conversions except @b n, the converted value is padded
 * on the left with zeros rather than blanks. If a precision is given with a
 * numeric conversion (@b d, @b i, @b o, @b u, @b x, and @b X), the @b 0 flag
 * is ignored.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top">'<b>-</b>'</td>
 * <td>
 * A negative field width flag; the converted value is to be left adjusted on
 * the field boundary. Except for @b n conversions, the converted value is
 * padded on the right with blank, rather than on the left with blanks or zeros.
 * A @b - overrides a @b 0 if both are given.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top">' ' (space)</td>
 * <td>
 * A blank should be left before a positive number produced by a signed
 * conversion (@b a, @b A, @b d, @b e, @b E, @b f, @b F, @b g, @b G, or @b i).
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top">'<b>+</b>'</td>
 * <td>
 * A sign must always be placed before a number produced by a signed conversion.
 * A @b + overrides a space if both are used.
 * </td>
 * </tr>
 * </table>
 *
 * @subsection field_width Minimum field width
 * An optional decimal digit string specifying a minimum field width. If the
 * converted value has fewer characters than the field width, it will be padded
 * with spaces on the left (or right, if the left-adjustment flag has been
 * given) to fill out the field width.
 *
 * @subsection precision Precision
 * An optional precision, in the form of a period <b>.</b> followed by an
 * optional digit string. If the digit string is omitted, the precision is taken
 * as zero. This gives the minimum number of digits to appear for @b d, @b i,
 * @b o, @b u, @b x, and @b X conversions, the number of digits to appear after
 * the decimal-point for @b a, @b A, @b e, @b E, @b f, and @b F conversions, the
 * maximum number of significant digits for @b g and @b G conversions, or the
 * maximum number of characters to be printed from a string from @b s
 * conversions.
 *
 * @subsection length_modifier Length modifier
 * An optional length modifier, that specifies the size of the argument. The
 * following length modifiers are valid for the @b d, @b i, @b n, @b o, @b u,
 * @b x, or @b X conversions.
 *
 * <table border="0" cellspacing="3" cellpadding="3">
 * <tr>
 * <td><b>Modifier</b></td>
 * <td><b>d, i</b></td>
 * <td><b>o, u, x, X</b></td>
 * <td><b>n</b></td>
 * </tr>
 * <tr>
 * <td>default</td>
 * <td>#acp_sint32_t</td>
 * <td>#acp_uint32_t</td>
 * <td>#acp_sint32_t *</td>
 * </tr>
 * <tr>
 * <td><b>hh</b></td>
 * <td>#acp_sint8_t</td>
 * <td>#acp_uint8_t</td>
 * <td>#acp_sint8_t *</td>
 * </tr>
 * <tr>
 * <td><b>h</b></td>
 * <td>#acp_sint16_t</td>
 * <td>#acp_uint16_t</td>
 * <td>#acp_sint16_t *</td>
 * </tr>
 * <tr>
 * <td><b>l</b> (ell)</td>
 * <td>#acp_slong_t</td>
 * <td>#acp_ulong_t</td>
 * <td>#acp_slong_t *</td>
 * </tr>
 * <tr>
 * <td><b>ll</b> (ell ell)</td>
 * <td>#acp_sint64_t</td>
 * <td>#acp_uint64_t</td>
 * <td>#acp_sint64_t *</td>
 * </tr>
 * <tr>
 * <td><b>t</b></td>
 * <td>#acp_ptrdiff_t</td>
 * <td>unsigned equivalent to #acp_ptrdiff_t</td>
 * <td>#acp_ptrdiff_t *</td>
 * </tr>
 * <tr>
 * <td><b>z</b></td>
 * <td>#acp_ssize_t</td>
 * <td>#acp_size_t</td>
 * <td>#acp_ssize_t *</td>
 * </tr>
 * </table>
 *
 * The signed type is expected for the @b d, @b i, and @b n conversions, and
 * unsigned type is expected for the @b o, @b u, @b x, and @b X conversions.
 * The @b l (ell) modifier expects the standard long integer type that is 4-byte
 * in 32bit systems, or 8-byte in 64bit systems. The @b ll (ell ell) modifier
 * expects always 8-byte integer type in any system.
 *
 * @subsection conversion_specifier Conversion specifier
 * A character that specifies the type of conversion to be applied
 * The conversion specifiers and their meanings are:
 *
 * <table border="0" cellspacing="4" cellpadding="4">
 * <tr>
 * <td nowrap valign="top"><b>diouxX</b></td>
 * <td>
 * The #acp_sint32_t (or appropriate variant) argument is converted to signed
 * decimal (@b d and @b i), unsigned octal (@b o), unsigned decimal (@b u), or
 * unsigned hexadecimal (@b x and @b X) notation. The letters 'abcdef' are used
 * for @b x conversions; the letters 'ABCDEF' are used for @b X conversions.
 * The precision, if any, gives the minimum number of digits that must appear;
 * if the converted value requires fewer digits, it is padded on the left with
 * zeros.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>eE</b></td>
 * <td>
 * The #acp_double_t argument is rounded and converted in the style
 * [-]d.ddde+-dd where there is one digit before the decimal-point character
 * and the number of digits after it is equal to the precision; if the precision
 * is missing, it is taken as 6; if the precision is zero, no decimal-point
 * character appears. An @b E conversion uses the letter 'E' (rather than 'e')
 * to introduce the exponent. The exponent always contains at least two digits;
 * if the value is zero, the exponent is 00.
 *
 * For @b a, @b A, @b e, @b E, @b f, @b F, @b g, and @b G conversions, positive
 * and negative infinity are represented as inf and -inf respectively when using
 * the lowercase conversion character, and INF and -INF respectively when using
 * the uppercase conversion character. Similarly, NaN is represented as nan when
 * using the lowercase conversion, NAN when using the uppercase conversion.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>fF</b></td>
 * <td>
 * The #acp_double_t argument is rounded and converted to decimal notation in
 * the sylte [-]ddd.ddd where the number of digits after the decimal-point
 * character is equal to the precision specification. If the precision is
 * missing, it is taken as 6; if the precision is explicitly zero, no
 * decimal-point character appears. If a decimal point appears, at least one
 * digit appears before it.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>gG</b></td>
 * <td>
 * The #acp_double_t argument is converted in style @b f or @b e (or @b F or
 * @b E for @b G conversions). The precision specifies the number of significant
 * digits. If the precision is missing, 6 digits are given; if the precision is
 * zero, it is treated as 1. Style @b e is used if the exponent from its
 * conversion is less than -4 or greater than or equal to the precision.
 * Trailing zeros are removed from the fractional part of the result; a decimal
 * point appears only if it is followed by at least one digit.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>aA</b></td>
 * <td>
 * These conversions are supported by GNU libc and BSD libc,
 * but the others do not. acpPrintf() does not support either.
 *
 * In GNU libc and BSD libc, these conversions are for printing double precision
 * floating point to hexadecimal notation in the style [-]0xh.hhhp[+-]d.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>c</b></td>
 * <td>
 * The #acp_char_t argument is converted to a character, and the resulting
 * character is written.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>s</b></td>
 * <td>
 * The #acp_char_t * argument is expected to be a pointer to an array of
 * character type (pointer to a sring). Characters from the array are written
 * up to (but not including) a terminating NUL character; if a precision is
 * specified, no more than the number specified are written. If a precision is
 * given, no null character need be present; if the precision is not specified,
 * or is greater than the size of the array, the array must contain a
 * terminating NUL character.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>S</b></td>
 * <td>
 * The #acp_str_t * argument is expected to be a pointer to a string object.
 * Characters from the string object are written; if a precision is specified,
 * no more than the number specified are written.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>p</b></td>
 * <td>
 * The void * pointer argument is printed in hexadecimal
 * (as if by <b>@%@#lx</b>).
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>n</b></td>
 * <td>
 * The number of characters written so far is stored into the integer indicated
 * by the #acp_sint32_t * (or variant) pointer argument. No arguemnt is
 * converted.
 * </td>
 * </tr>
 * <tr>
 * <td nowrap valign="top"><b>@%</b></td>
 * <td>
 * A '@%' is written. No argument is converted. The complete conversion
 * specification is '@%@%'.
 * </td>
 * </tr>
 * </table>
 *
 * @subsection via_argument Field width or precision via argument
 * A field width or precision, or both, may be indicated by an asterisk '*' or
 * an asterisk followed by one or more decimal digits and a '$' instead of a
 * digit string. In this case, an #acp_sint32_t argument supplies the field
 * width or precision. A negative field width is treated as a left adjustment
 * flag followed by a positive field width; a negative precision is treated as
 * though it were missing. If a single format directive mixes positional (nn$)
 * and non-positional arguments, the results are undefined.
 *
 */

#include <acpStd.h>


ACP_EXTERN_C_BEGIN


/*@printflike@*/
ACP_EXPORT acp_rc_t acpPrintf(const acp_char_t *aFormat, ...);
ACP_EXPORT acp_rc_t acpVprintf(const acp_char_t *aFormat, va_list aArgs);

/*@printflike@*/
ACP_EXPORT acp_rc_t acpFprintf(acp_std_file_t   *aFile,
                               const acp_char_t *aFormat, ...);
ACP_EXPORT acp_rc_t acpVfprintf(acp_std_file_t   *aFile,
                                const acp_char_t *aFormat,
                                va_list           aArgs);

/*@printflike@*/
ACP_EXPORT acp_rc_t acpSnprintf(acp_char_t       *aBuffer,
                                acp_size_t        aSize,
                                const acp_char_t *aFormat, ...);
ACP_EXPORT acp_rc_t acpVsnprintf(acp_char_t       *aBuffer,
                                 acp_size_t        aSize,
                                 const acp_char_t *aFormat,
                                 va_list           aArgs);
ACP_EXPORT acp_rc_t acpSnprintfSize(acp_char_t       *aBuffer,
                                    acp_size_t        aSize,
                                    acp_size_t       *aWritten,
                                    const acp_char_t *aFormat, ...);
ACP_EXPORT acp_rc_t acpVsnprintfSize(acp_char_t       *aBuffer,
                                     acp_size_t        aSize,
                                     acp_size_t       *aWritten,
                                     const acp_char_t *aFormat,
                                     va_list           aArgs);


ACP_EXTERN_C_END


#endif
