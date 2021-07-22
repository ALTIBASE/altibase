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
 
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1993-1998, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * User-visible pieces of the ANSI C standard I/O package.
 */

#ifndef _STDIO_H
#define	_STDIO_H

#pragma ident	"@(#)stdio.h	1.69	98/07/13 SMI"	/* SVr4.0 2.34.1.2 */

#include <sys/feature_tests.h>
#include <sys/va_list.h>
#include <stdio_tag.h>
#include <stdio_impl.h>

/*
 * If feature test macros are set that enable interfaces that use types
 * defined in <sys/types.h>, get those types by doing the include.
 *
 * Note that in asking for the interfaces associated with this feature test
 * macro one also asks for definitions of the POSIX types.
 */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	_FILEDEFED
#define	_FILEDEFED
typedef	__FILE FILE;
#endif

#ifndef _SIZE_T
#define	_SIZE_T
#if defined(_LP64) || defined(_I32LPx)
typedef unsigned long	size_t;		/* size of something in bytes */
#else
typedef unsigned int	size_t;		/* (historical version) */
#endif
#endif	/* !_SIZE_T */

#if !defined(_LP64) && (_FILE_OFFSET_BITS == 64 || defined(_LARGEFILE64_SOURCE))
/*
 * The following typedefs are adopted from ones in <sys/types.h> (with leading
 * underscores added to avoid polluting the ANSI C name space).  See the
 * commentary there for further explanation.
 */
#if __STDC__ - 0 == 0 && !defined(_NO_LONGLONG)
typedef	long long	__longlong_t;
#else
/* used to reserve space and generate alignment */
typedef union {
	double	_d;
	int	_l[2];
} __longlong_t;
#endif
#endif	/* !_LP64 && _FILE_OFFSET_BITS == 64 || defined(_LARGEFILE64_SOURCE) */

#if defined(_LARGEFILE_SOURCE) || defined(_XPG5)
#ifndef	_OFF_T
#define	_OFF_T
#if defined(_LP64) || _FILE_OFFSET_BITS == 32
typedef long		off_t;
#else
typedef __longlong_t	off_t;
#endif
#ifdef	_LARGEFILE64_SOURCE
#ifdef _LP64
typedef	off_t		off64_t;
#else
typedef __longlong_t	off64_t;
#endif
#endif /* _LARGEFILE64_SOURCE */
#endif /* _OFF_T */
#endif /* _LARGEFILE_SOURCE */

#if defined(_LP64) || _FILE_OFFSET_BITS == 32
typedef long		fpos_t;
#else
typedef __longlong_t	fpos_t;
#endif
#ifdef _LARGEFILE64_SOURCE
#ifdef _LP64
typedef fpos_t		fpos64_t;
#else
typedef __longlong_t	fpos64_t;
#endif
#endif /* _LARGEFILE64_SOURCE */

#ifndef	NULL
#if defined(_LP64) && !defined(__cplusplus)
#define	NULL	0L
#else
#define	NULL	0
#endif
#endif

#define	BUFSIZ	1024

/*
 * XPG4 requires that va_list be defined in <stdio.h> "as described in
 * <stdarg.h>".  ANSI-C and POSIX require that the namespace of <stdio.h>
 * not be polluted with this name.
 */
#if defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4) && !defined(_VA_LIST)
#define	_VA_LIST
typedef	__va_list va_list;
#endif	/* defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4) && ... */

#ifdef	__STDC__
/*
 * Note that the following construct, "#machine(name)", is a non-standard
 * extension to ANSI-C.  It is maintained here to provide compatibility
 * for existing compilations systems, but should be viewed as transitional
 * and may be removed in a future release.  If it is required that this
 * file not contain this extension, edit this file to remove the offending
 * condition.
 *
 * The value of _NFILE is defined in the Processor Specific ABI.  The value
 * is chosen for historical reasons rather than for truly processor related
 * attribute.  Note that the SPARC Processor Specific ABI uses the common
 * UNIX historical value of 20 so it is allowed to fall through.
 */
#if #machine(i386) || defined(__i386)
#define	_NFILE	60	/* initial number of streams: Intel x86 ABI */
#else
#define	_NFILE	20	/* initial number of streams: SPARC ABI and default */
#endif
#else	/* __STDC__ */
#if defined(i386) || defined(__i386)
#define	_NFILE	60	/* initial number of streams: Intel x86 ABI */
#else
#define	_NFILE	20	/* initial number of streams: SPARC ABI and default */
#endif
#endif	/* __STDC__ */

#define	_SBFSIZ	8	/* compatibility with shared libs */

#define	_IOFBF		0000	/* full buffered */
#define	_IOLBF		0100	/* line buffered */
#define	_IONBF		0004	/* not buffered */
#define	_IOEOF		0020	/* EOF reached on read */
#define	_IOERR		0040	/* I/O error from system */

#define	_IOREAD		0001	/* currently reading */
#define	_IOWRT		0002	/* currently writing */
#define	_IORW		0200	/* opened for reading and writing */
#define	_IOMYBUF	0010	/* stdio malloc()'d buffer */

#ifndef EOF
#define	EOF	(-1)
#endif

#define	FOPEN_MAX	_NFILE
#define	FILENAME_MAX    1024	/* max # of characters in a path name */

#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	TMP_MAX		17576	/* 26 * 26 * 26 */

#if defined(__EXTENSIONS__) || __STDC__ - 0 == 0 || \
		defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE)

#define	L_ctermid	9
#define	L_cuserid	9
#endif

#if defined(__EXTENSIONS__) || ((__STDC__ - 0 == 0 && \
		!defined(_POSIX_C_SOURCE))) || defined(_XOPEN_SOURCE)

#define	P_tmpdir	"/var/tmp/"
#endif /* defined(__EXTENSIONS__) || ((__STDC__ - 0 == 0 && ... */

#define	L_tmpnam	25	/* (sizeof(P_tmpdir) + 15) */

#if defined(__STDC__)
extern FILE	__iob[_NFILE];
#define	stdin	(&__iob[0])
#define	stdout	(&__iob[1])
#define	stderr	(&__iob[2])
#else
extern FILE	_iob[_NFILE];
#define	stdin	(&_iob[0])
#define	stdout	(&_iob[1])
#define	stderr	(&_iob[2])
#endif	/* __STDC__ */

#ifndef _STDIO_ALLOCATE
extern unsigned char	 _sibuf[], _sobuf[];
#endif

/* large file compilation environment setup */
#if !defined(_LP64) && _FILE_OFFSET_BITS == 64
#ifdef __PRAGMA_REDEFINE_EXTNAME
#pragma redefine_extname	fopen	fopen64
#pragma redefine_extname	freopen	freopen64
#pragma redefine_extname	tmpfile	tmpfile64
#pragma redefine_extname	fgetpos	fgetpos64
#pragma redefine_extname	fsetpos	fsetpos64
#ifdef	_LARGEFILE_SOURCE
#pragma redefine_extname	fseeko	fseeko64
#pragma redefine_extname	ftello	ftello64
#endif
#else	/* __PRAGMA_REDEFINE_EXTNAME */
#define	fopen			fopen64
#define	freopen			freopen64
#define	tmpfile			tmpfile64
#define	fgetpos			fgetpos64
#define	fsetpos			fsetpos64
#ifdef	_LARGEFILE_SOURCE
#define	fseeko			fseeko64
#define	ftello			ftello64
#endif
#endif	/* __PRAGMA_REDEFINE_EXTNAME */
#endif	/* !_LP64 && _FILE_OFFSET_BITS == 64 */

#ifndef _LP64
extern unsigned char	*_bufendtab[];
extern FILE		*_lastbuf;
#endif

/* In the LP64 compilation environment, all APIs are already large file */
#if defined(_LP64) && defined(_LARGEFILE64_SOURCE)
#ifdef __PRAGMA_REDEFINE_EXTNAME
#pragma	redefine_extname	fopen64		fopen
#pragma	redefine_extname	freopen64	freopen
#pragma	redefine_extname	tmpfile64	tmpfile
#pragma	redefine_extname	fgetpos64	fgetpos
#pragma	redefine_extname	fsetpos64	fsetpos
#ifdef	_LARGEFILE_SOURCE
#pragma	redefine_extname	fseeko64	fseeko
#pragma	redefine_extname	ftello64	ftello
#endif
#else	/* __PRAGMA_REDEFINE_EXTNAME */
#define	fopen64		fopen
#define	freopen64	freopen
#define	tmpfile64	tmpfile
#define	fgetpos64	fgetpos
#define	fsetpos64	fsetpos
#ifdef	_LARGEFILE_SOURCE
#define	fseeko64	fseeko
#define	ftello64	ftello
#endif
#endif	/* __PRAGMA_REDEFINE_EXTNAME */
#endif	/* _LP64 && _LARGEFILE64_SOURCE */

#if defined(__STDC__)

extern int	remove(const char *);
extern int	rename(const char *, const char *);
extern FILE	*tmpfile(void);
extern char	*tmpnam(char *);
#if	defined(__EXTENSIONS__) || defined(_REENTRANT)
extern char	*tmpnam_r(char *);
#endif /* defined(__EXTENSIONS__) || defined(_REENTRANT) */
extern int	fclose(FILE *);
extern int	fflush(FILE *);
extern FILE	*fopen(const char *, const char *);
extern FILE	*freopen(const char *, const char *, FILE *);
extern void	setbuf(FILE *, char *);
#if defined(__EXTENSIONS__) || (__STDC__ == 0 && \
	!defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE))
extern void setbuffer(FILE *, char *, size_t);
extern int setlinebuf(FILE *);
#endif
extern int	setvbuf(FILE *, char *, int, size_t);
/* PRINTFLIKE2 */
extern int	fprintf(FILE *, const char *, ...);
/* SCANFLIKE2 */
extern int	fscanf(FILE *, const char *, ...);
/* PRINTFLIKE1 */
extern int	printf(const char *, ...);
/* SCANFLIKE1 */
extern int	scanf(const char *, ...);
#if defined(__EXTENSIONS__) || (__STDC__ == 0 && \
	!defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE)) || \
	defined(_XPG5)
/* PRINTFLIKE3 */
extern int	snprintf(char *, size_t, const char *, ...);
#endif
/* PRINTFLIKE2 */
extern int	sprintf(char *, const char *, ...);
/* SCANFLIKE2 */
extern int	sscanf(const char *, const char *, ...);
extern int	vfprintf(FILE *, const char *, __va_list);
extern int	vprintf(const char *, __va_list);
#if defined(__EXTENSIONS__) || (__STDC__ == 0 && \
	!defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE)) || \
	defined(_XPG5)
extern int	vsnprintf(char *, size_t, const char *, __va_list);
#endif	/* defined(__EXTENSIONS__) ||  __STDC__ == 0 ... */
extern int	vsprintf(char *, const char *, __va_list);
extern int	fgetc(FILE *);
extern char	*fgets(char *, int, FILE *);
extern int	fputc(int, FILE *);
extern int	fputs(const char *, FILE *);
extern int	getc(FILE *);
extern int	getchar(void);
extern char	*gets(char *);
extern int	putc(int, FILE *);
extern int	putchar(int);
extern int	puts(const char *);
extern int	ungetc(int, FILE *);
extern size_t	fread(void *, size_t, size_t, FILE *);
extern size_t	fwrite(const void *, size_t, size_t, FILE *);
extern int	fgetpos(FILE *, fpos_t *);
extern int	fseek(FILE *, long, int);
extern int	fsetpos(FILE *, const fpos_t *);
extern long	ftell(FILE *);
extern void	rewind(FILE *);
extern void	clearerr(FILE *);
extern int	feof(FILE *);
extern int	ferror(FILE *);
extern void	perror(const char *);

#ifndef	_LP64
extern int	__filbuf(FILE *);
extern int	__flsbuf(int, FILE *);
#endif	/*	_LP64	*/

/*
 * The following are known to POSIX and XOPEN, but not to ANSI-C.
 */
#if defined(__EXTENSIONS__) || __STDC__ == 0 || \
		defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE)

extern FILE	*fdopen(int, const char *);
extern char	*ctermid(char *);
extern int	fileno(FILE *);

#endif	/* defined(__EXTENSIONS__) ||  __STDC__ == 0 ... */

/*
 * The following are known to POSIX.1c, but not to ANSI-C or XOPEN.
 */
#if	defined(__EXTENSIONS__) || defined(_REENTRANT) || \
	    (_POSIX_C_SOURCE - 0 >= 199506L)
extern void	flockfile(FILE *);
extern int	ftrylockfile(FILE *);
extern void	funlockfile(FILE *);
extern int	getc_unlocked(FILE *);
extern int	getchar_unlocked(void);
extern int	putc_unlocked(int, FILE *);
extern int	putchar_unlocked(int);

#endif	/* defined(__EXTENSIONS__) || defined(_REENTRANT).. */

/*
 * The following are known to XOPEN, but not to ANSI-C or POSIX.
 */
#if defined(__EXTENSIONS__) || __STDC__ == 0 || defined(_XOPEN_SOURCE)

extern FILE	*popen(const char *, const char *);
extern char	*cuserid(char *);
extern char	*tempnam(const char *, const char *);
extern int	getopt(int, char *const *, const char *);
#if !defined(_XOPEN_SOURCE)
extern int	getsubopt(char **, char *const *, char **);
#endif /* !defined(_XOPEN_SOURCE) */
extern char	*optarg;
extern int	optind, opterr, optopt;
extern int	getw(FILE *);
extern int	putw(int, FILE *);
extern int	pclose(FILE *);

#endif	/* defined(__EXTENSIONS__) ||  __STDC__ == 0 ... */

/*
 * The following are defined as part of the Large File Summit interfaces.
 */
#if	defined(_LARGEFILE_SOURCE) || defined(_XPG5)
extern int	fseeko(FILE *, off_t, int);
extern off_t	ftello(FILE *);
#endif

/*
 * The following are defined as part of the transitional Large File Summit
 * interfaces.
 */
#if	defined(_LARGEFILE64_SOURCE) && !((_FILE_OFFSET_BITS == 64) && \
	    !defined(__PRAGMA_REDEFINE_EXTNAME))
extern FILE	*fopen64(const char *, const char *);
extern FILE	*freopen64(const char *, const char *, FILE *);
extern FILE	*tmpfile64(void);
extern int	fgetpos64(FILE *, fpos64_t *);
extern int	fsetpos64(FILE *, const fpos64_t *);
extern int	fseeko64(FILE *, off64_t, int);
extern off64_t	ftello64(FILE *);
#endif

#else	/* !defined __STDC__ */

#ifndef	_LP64
#define	_bufend(p)	((fileno(p) < _NFILE) ? _bufendtab[fileno(p)] : \
			(unsigned char *)_realbufend(p))
#define	_bufsiz(p)	(_bufend(p) - (p)->_base)
#endif	/*	_LP64	*/

extern int	remove();
extern int	rename();
extern FILE	*tmpfile();
extern char	*tmpnam();
#if	defined(__EXTENSIONS__) || defined(_REENTRANT)
extern char	*tmpnam_r();
#endif /* defined(__EXTENSIONS__) || defined(_REENTRANT) */
extern int	fclose();
extern int	fflush();
extern FILE	*fopen();
extern FILE	*freopen();
extern void	setbuf();
#if defined(__EXTENSIONS__) || \
	(!defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE))
extern void setbuffer();
extern int setlinebuf();
#endif
extern int	setvbuf();
extern int	fprintf();
extern int	fscanf();
extern int	printf();
extern int	scanf();
extern int	sprintf();
extern int	sscanf();
extern int	vfprintf();
extern int	vprintf();
extern int	vsprintf();
extern int	fgetc();
extern char	*fgets();
extern int	fputc();
extern int	fputs();
extern int	getc();
extern int	getchar();
extern char	*gets();
extern int	putc();
extern int	putchar();
extern int	puts();
extern int	ungetc();
extern size_t	fread();
extern size_t	fwrite();
extern int	fgetpos();
extern int	fseek();
extern int	fsetpos();
extern long	ftell();
extern void	rewind();
extern void	clearerr();
extern int	feof();
extern int	ferror();
extern void	perror();

#ifndef	_LP64
extern int	_filbuf();
extern int	_flsbuf();
#endif	/*	_LP64	*/

#if defined(__EXTENSIONS__) || \
		defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE)
extern FILE	*fdopen();
extern char	*ctermid();
extern int	fileno();
#endif	/* defined(__EXTENSIONS__) ||  defined(_POSIX_C_SOURCE... */

#if	defined(__EXTENSIONS__) || defined(_REENTRANT) || \
	    (_POSIX_C_SOURCE - 0 >= 199506L)
extern void	flockfile();
extern int	ftrylockfile();
extern void	funlockfile();
extern int	getc_unlocked();
extern int	getchar_unlocked();
extern int	putc_unlocked();
extern int	putchar_unlocked();
#endif	/* defined(__EXTENSIONS__) || defined(_REENTRANT).. */

#if defined(__EXTENSIONS__) || defined(_XOPEN_SOURCE)
extern FILE	*popen();
extern char	*cuserid();
extern char	*tempnam();
extern int	getopt();
#if !defined(_XOPEN_SOURCE)
extern int	getsubopt();
#endif /* !defined(_XOPEN_SOURCE) */
extern char	*optarg;
extern int	optind, opterr, optopt;
extern int	getw();
extern int	putw();
extern int	pclose();
#endif	/* defined(__EXTENSIONS__) || defined(_XOPEN_SOURCE) */

#if	defined(_LARGEFILE_SOURCE) || defined(_XPG5)
extern int	fseeko();
extern off_t	ftello();
#endif

#if	defined(_LARGEFILE64_SOURCE) && !((_FILE_OFFSET_BITS == 64) && \
	    !defined(__PRAGMA_REDEFINE_EXTNAME))
extern FILE	*fopen64();
extern FILE	*freopen64();
extern FILE	*tmpfile64();
extern int	fgetpos64();
extern int	fsetpos64();
extern int	fseeko64();
extern off64_t	ftello64();
#endif

#endif	/* __STDC__ */

#if !defined(lint) && !defined(__lint)

#ifndef	_REENTRANT
#ifndef	_LP64
#ifdef	__STDC__
#define	getc(p)		(--(p)->_cnt < 0 ? __filbuf(p) : (int)*(p)->_ptr++)
#define	putc(x, p)	(--(p)->_cnt < 0 ? __flsbuf((x), (p)) \
				: (int)(*(p)->_ptr++ = (unsigned char) (x)))
#else
#define	getc(p)		(--(p)->_cnt < 0 ? _filbuf(p) : (int) *(p)->_ptr++)
#define	putc(x, p)	(--(p)->_cnt < 0 ? \
			_flsbuf((x), (p)) : \
			(int) (*(p)->_ptr++ = (unsigned char) (x)))
#endif	/* __STDC__ */
#endif	/* _LP64 */

#define	getchar()	getc(stdin)
#define	putchar(x)	putc((x), stdout)

#ifndef	_LP64
#define	clearerr(p)	((void)((p)->_flag &= ~(_IOERR | _IOEOF)))
#define	feof(p)		((p)->_flag & _IOEOF)
#define	ferror(p)	((p)->_flag & _IOERR)
#endif	/* _LP64 */

#endif	/* _REENTRANT */

#if	defined(__EXTENSIONS__) || defined(_REENTRANT) || \
	    (_POSIX_C_SOURCE - 0 >= 199506L)
#ifndef	_LP64
#ifdef	__STDC__
#define	getc_unlocked(p)	(--(p)->_cnt < 0 \
					? __filbuf(p) \
					: (int)*(p)->_ptr++)
#define	putc_unlocked(x, p)	(--(p)->_cnt < 0 \
					? __flsbuf((x), (p)) \
					: (int)(*(p)->_ptr++ = \
					(unsigned char) (x)))
#else
#define	getc_unlocked(p)	(--(p)->_cnt < 0 \
					? _filbuf(p) \
					: (int)*(p)->_ptr++)
#define	putc_unlocked(x, p)	(--(p)->_cnt < 0 \
					? _flsbuf((x), (p)) \
					: (int)(*(p)->_ptr++ = \
					(unsigned char) (x)))
#endif	/* __STDC__ */
#endif	/* _LP64 */
#define	getchar_unlocked()	getc_unlocked(stdin)
#define	putchar_unlocked(x)	putc_unlocked((x), stdout)
#endif	/* defined(__EXTENSIONS__) || defined(_REENTRANT).. */

#endif	/* !defined(lint) && !defined(__lint) */

#ifdef	__cplusplus
}
#endif

#endif	/* _STDIO_H */
