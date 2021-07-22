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
 
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static void
print_char (
size_t       n_bytes
, const char * block
, const char * fmt_string )
{
	size_t i;
	for (i = n_bytes; i > 0; i--)
	{
		unsigned int tmp = *(const unsigned char *) block;
		printf (fmt_string, tmp);
		block += sizeof (unsigned char);
	}
}

static void
print_short (
size_t       n_bytes
, const char * block
, const char * fmt_string )
{
	size_t i;
	for (i = n_bytes / sizeof (unsigned short); i > 0; i--)
	{
		unsigned int tmp = *(const unsigned short *) block;
		printf (fmt_string, tmp);
		block += sizeof (unsigned short);
	}
}

static void
print_ascii (
size_t       n_bytes
, const char * block
, const char * unused_fmt_string )
{
	size_t       i;
	const char * s;
	char         buf[5];

	for (i = n_bytes; i > 0; i--)
	{
		unsigned int c = *(const unsigned char *) block;

		switch (c)
		{
		case '\0'  : 
			s = "\\0"; 
			break;
		case '\007': 
			s = "\\a"; 
			break;
		case '\b'  : 
			s = "\\b"; 
			break;
		case '\f'  : 
			s = "\\f"; 
			break;
		case '\n'  : 
			s = "\\n"; 
			break;
		case '\r'  : 
			s = "\\r"; 
			break;
		case '\t'  : 
			s = "\\t"; 
			break;
		case '\v'  : 
			s = "\\v"; 
			break;
		default:
			sprintf (buf, ((isascii (c) && isprint (c)) ? " %c" : "%02x"), c);
			s = (const char *) buf;
		}
		printf (" %2s", s);

		block += sizeof (unsigned char);
	}
}

static void
format_address(
uintmax_t address
, char      c
, int       address_pad_len
)
{
	char buf[((sizeof (uintmax_t) * 8 + 8 - 1) / 3) + 2];
	char const *pbound;
	char *p = buf + sizeof buf;

	*--p   = '\0';
	*--p   = c;
	pbound = p - address_pad_len;

	do { 
		*--p = "0123456789abcdef"[address & 0xF]; 
	} while ((address >>= 4) != 0);

	while (pbound < p) { 
		*--p = '0'; 
	}

	fputs (p, stdout);
}

static void
write_block (
uintmax_t    current_offset
, size_t       n_bytes
, const char * prev_block
, const char * curr_block
, int          address_pad_len
, size_t       bytes_per_block
)
{
	static int first           = 1;
	static int prev_pair_equal = 0;

	if ( !first 
	    && n_bytes == bytes_per_block
	    && (memcmp (prev_block, curr_block, bytes_per_block) == 0))
	{
		if (prev_pair_equal)
		{
			/* The two preceding blocks were equal, and the current
			                 block is the same as the last one, so print nothing.  */
		}
		else
		{
			printf ("*\n");
			prev_pair_equal = 1;
		}
	}
	else
	{
		prev_pair_equal = 0;

		format_address (current_offset, '\0',address_pad_len);
		/* print_short(n_bytes, curr_block, " %04x"); */
		print_char(n_bytes, curr_block, " %02x");
		putchar ('\n');

		printf ("%*s", address_pad_len, "");
		print_ascii (n_bytes, curr_block, NULL);
		putchar ('\n');
	}
	first = 0;
}

static int
dump (
int          address_pad_len
, uintmax_t    n_bytes_to_skip
, uintmax_t    end_offset
, size_t       bytes_per_block
, FILE       * in_stream
)
{
	char      * block[2];
	uintmax_t   current_offset;
	int         idx;
	int         err;
	size_t      n_bytes_read;
	size_t      n_needed;

	block[0] = (char *) malloc (bytes_per_block);
	block[1] = (char *) malloc (bytes_per_block);

	current_offset = n_bytes_to_skip;

	idx = 0;
	err = 0;
	while (1)
	{
		if (current_offset >= end_offset)
		{
			n_bytes_read = 0;
			break;
		}
		n_needed     = MIN (end_offset - current_offset, (uintmax_t) bytes_per_block);
		n_bytes_read = fread (block[idx], 1, n_needed, in_stream);
		if (n_bytes_read < bytes_per_block) { 
			break; 
		}

		assert (n_bytes_read == bytes_per_block);
		write_block (current_offset,n_bytes_read,block[!idx],block[idx],address_pad_len,bytes_per_block);

		current_offset += n_bytes_read;
		idx             = !idx;
	}

	if (n_bytes_read > 0)
	{
		int    l_c_m;
		size_t bytes_to_write;

		l_c_m = 2;

		/* Make bytes_to_write the smallest multiple of l_c_m that
		               is at least as large as n_bytes_read.  */
		bytes_to_write = l_c_m * ((n_bytes_read + l_c_m - 1) / l_c_m);

		memset (block[idx] + n_bytes_read, 0, bytes_to_write - n_bytes_read);
		write_block (current_offset, bytes_to_write,block[!idx], block[idx],address_pad_len,bytes_per_block);
		current_offset += n_bytes_read;
	}
	format_address (current_offset,'\n',address_pad_len);

	return err;
}

void
usage(void)
{
    printf("Usage: hd [-j BYTES] [-N BYTES] filename\n -j BYTES skip BYTES input bytes first\n -N BYTES limit dump to BYTES(hexa) input bytes\n The format of BYTES should be hexa starting with 0x\n Example: hd -j 0x2000 -N 0x10000 system001.dbf");
}

int
main (int argc, char **argv)
{
	int          c;
	size_t       i;
	int          err;
	char const * input_filename;
	uintmax_t    max_bytes_to_format;
	int          address_pad_len;
	uintmax_t    n_bytes_to_skip;
	size_t       bytes_per_block;
	FILE       * in_stream;

	err = 0;

	n_bytes_to_skip     = 0x0;
	max_bytes_to_format = 0x1000;
	address_pad_len     = 6;
	bytes_per_block     = 32;

	while ((c = getopt(argc, argv, "j:N:")) != EOF)
	{
		switch (c)
		{
		case 0  : 
			break;
		case 'j': 
			n_bytes_to_skip     = strtol(optarg, NULL, 16); 
			break;
		case 'N': 
			max_bytes_to_format = strtol(optarg, NULL, 16); 
			break;
		default : 
                        usage();
			break;
		}
	}

        if(argc - optind != 1)
        {
		fprintf (stderr, "%s\n", "argument error");
                usage();
		err = 1;
		goto cleanup;
        }

	input_filename = argv[optind];

	in_stream = fopen (input_filename, "rb");
	if (in_stream == NULL)
	{
		fprintf (stderr, "%s\n", input_filename);
		err = 1;
		goto cleanup;
	}

	if (fseeko (in_stream, n_bytes_to_skip, SEEK_CUR) != 0)
	{
		fprintf (stderr, "%s\n", input_filename);
		err = 1;
		goto cleanup;
	}

	err = dump(  address_pad_len
	    , n_bytes_to_skip
	    , n_bytes_to_skip + max_bytes_to_format
	    , bytes_per_block
	    , in_stream );

cleanup:
	;

	exit (err == 0 ? 0 : 1);
}

