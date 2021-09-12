#pragma once
#include <stddef.h>
#include <stdint.h>


#if defined (__aarch64__) || defined (__amd64__) || defined (_M_AMD64)
/* unpack csv newline search */
#define CSV_UNPACK_64_SEARCH
#endif

const char* csv_read_next_column(char* row, char delim, char quote, char escape, char ** context)
{
	/* return properly escaped CSV col
	 * RFC: [https://tools.ietf.org/html/rfc4180]
	 */
	char* p = (*context) ? (*context) : row;
	char* d = p; /* destination */
	char* b = p; /* begin */
	int quoted = 0; /* idicates quoted string */

	quoted = *p == quote;
	if (quoted)
		p++;

	for (; *p; p++, d++)
	{
		/* double quote is present if (1) */
		int dq = 0;

		/* skip escape */
		if (*p == escape && p[1])
			p++;

		/* skip double-quote */
		if (*p == quote && p[1] == quote)
		{
			dq = 1;
			p++;
		}

		/* check if we should end */
		if (quoted && !dq)
		{
			if (*p == quote)
				break;
		}
		else if (*p == delim)
		{
			break;
		}

		/* copy if required */
		if (d != p)
			*d = *p;
	}

	if (!*p)
	{
		/* nothing to do */
		if (p == b)
			return NULL;

		(*context) = p;
	}
	else
	{
		/* end reached, skip */
		*d = '\0';
		if (quoted)
		{
			for (p++; *p; p++)
				if (*p == delim)
					break;

			if (*p)
				p++;

			(*context) = p;
		}
		else
		{
			(*context) = p + 1;
		}
	}
	return b;
}



#define CSV_QUOTE_BR(c, n, quotes) \
	do \
		if (c##n == quote)                              \
			(quotes)++;                           \
		else if (c##n == '\n' && !((quotes) & 1)) \
			return p + n;                               \
	while (0)


static char* csv_search_linefeed(char* p, size_t size, char quote, size_t * quotes)
{
	/* TODO: this can be greatly optimized by
	 * using modern SIMD instructions, but for now
	 * we only fetch 8Bytes "at once"
	 */
	char* end = p + size;
	//char quote = handle->quote;

#ifdef CSV_UNPACK_64_SEARCH
	uint64_t* pd = (uint64_t*)p;
	uint64_t* pde = pd + (size / sizeof(uint64_t));

	for (; pd < pde; pd++)
	{
		/* unpack 64bits to 8x8bits */
		char c0, c1, c2, c3, c4, c5, c6, c7;
		p = (char*)pd;
		c0 = p[0];
		c1 = p[1];
		c2 = p[2];
		c3 = p[3];
		c4 = p[4];
		c5 = p[5];
		c6 = p[6];
		c7 = p[7];

		CSV_QUOTE_BR(c, 0, *quotes);
		CSV_QUOTE_BR(c, 1, *quotes);
		CSV_QUOTE_BR(c, 2, *quotes);
		CSV_QUOTE_BR(c, 3, *quotes);
		CSV_QUOTE_BR(c, 4, *quotes);
		CSV_QUOTE_BR(c, 5, *quotes);
		CSV_QUOTE_BR(c, 6, *quotes);
		CSV_QUOTE_BR(c, 7, *quotes);
	}
	p = (char*)pde;
#endif

	for (; p < end; p++)
	{
		char c0 = *p;
		CSV_QUOTE_BR(c, 0, *quotes);
	}

	return NULL;
}



static void csv_terminate_line(char* p, size_t size)
{
	/* we do support standard POSIX LF sequence
	 * and Windows CR LF sequence.
	 * old non POSIX Mac OS CR is not supported.
	 */
	char* res = p;
	if (size >= 2 && p[-1] == '\r')
		--res;

	*res = 0;
}

