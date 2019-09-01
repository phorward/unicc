/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	utf8.c
Author:	Jeff Bezanson, contributions by Jan Max Meyer
Usage:	Some UTF-8 utility functions.
----------------------------------------------------------------------------- */

/*
  Basic UTF-8 manipulation routines
  by Jeff Bezanson
  placed in the public domain Fall 2005

  This code is designed to provide the utilities you need to manipulate
  UTF-8 as an internal string encoding. These functions do not perform the
  error checking normally needed when handling UTF-8 data, so if you happen
  to be from the Unicode Consortium you will want to flay me alive.
  I do this because error checking can be performed at the boundaries (I/O),
  with these routines reserved for higher performance on data known to be
  valid.
*/

#include "phorward.h"

#if _WIN32
#define snprintf _snprintf
#endif

static const size_t offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/** Check for UTF-8 character sequence signature.

The function returns TRUE, if the character //c// is the beginning of a UTF-8
character signature, else FALSE. */
pboolean putf8_isutf( unsigned char c )
{
	return MAKE_BOOLEAN( ( c & 0xC0 ) != 0x80 );
}

/** Returns length of next UTF-8 sequence in a multi-byte character string.

//s// is the pointer to begin of UTF-8 sequence.

Returns the number of bytes used for the next character.
*/
int putf8_seqlen(char *s)
{
#ifdef UTF8
	if( putf8_isutf( (unsigned char)*s ) )
	    return trailingBytesForUTF8[ (unsigned char)*s ] + 1;
#endif
	return 1;
}

/** Return single character (as wide-character value) from UTF-8 multi-byte
character string.

//str// is the pointer to character sequence begin. */
wchar_t putf8_char( char* str )
{
#ifndef UTF8
	return (wchar_t)*str;
#else
	int 	nb;
	wchar_t 	ch = 0;

	switch( ( nb = trailingBytesForUTF8[ (unsigned char)*str ] ) )
	{
        case 3:
			ch += (unsigned char)*str++;
			ch <<= 6;
        case 2:
			ch += (unsigned char)*str++;
			ch <<= 6;
        case 1:
			ch += (unsigned char)*str++;
			ch <<= 6;
        case 0:
			ch += (unsigned char)*str++;
	}

	ch -= offsetsFromUTF8[ nb ];

	return ch;
#endif
}

/** Moves //count// characters ahead in an UTF-8 multi-byte character sequence.

//str// is the pointer to UTF-8 string to begin moving.
//count// is the number of characters to move left.

The function returns the address of the next UTF-8 character sequence after
//count// characters. If the string's end is reached, it will return a
pointer to the zero-terminator.
*/
char* putf8_move( char* str, int count )
{
	for( ; count > 0; count-- )
		str += putf8_seqlen( str );

	return str;
}

/** Read one character from an UTF-8 input sequence.
This character can be escaped, an UTF-8 character or an ordinary ASCII-char.

//chr// is the input- and output-pointer (the pointer is replaced by the pointer
to the next character or escape-sequence within the string).

The function returns the character code of the parsed character.
*/
wchar_t putf8_parse_char( char** ch )
{
	wchar_t	ret;
	PROC( "putf8_parse_char" );
	PARMS( "ch", "%p", ch );

#ifdef UTF8
	if( putf8_char( *ch ) == (wchar_t)'\\' )
	{
		MSG( "Escape sequence detected" );
		(*ch)++;

		VARS( "*ch", "%s", *ch );
		(*ch) += putf8_read_escape_sequence( *ch, &ret );
		VARS( "*ch", "%s", *ch );
	}
	else
	{
		MSG( "No escape sequence, normal UTF-8 character sequence processing" );
		ret = putf8_char( *ch );
		(*ch) += putf8_seqlen( *ch );
	}
#else
	ret = *( (*ch)++ );
#endif

	VARS( "ret", "%d", ret );
	RETURN( ret );
}


/* conversions without error checking
   only works for valid UTF-8, i.e. no 5- or 6-byte sequences
   srcsz = source size in bytes, or -1 if 0-terminated
   sz = dest size in # of wide characters

   returns # characters converted
   dest will always be L'\0'-terminated, even if there isn't enough room
   for all the characters.
   if sz = srcsz+1 (i.e. 4*srcsz+4 bytes), there will always be enough space.
*/
int putf8_toucs(wchar_t *dest, int sz, char *src, int srcsz)
{
    wchar_t ch;
    char *src_end = src + srcsz;
    int nb;
    int i=0;

    while (i < sz-1) {
        nb = trailingBytesForUTF8[(int)*src];
        if (srcsz == -1) {
            if (*src == 0)
                goto done_toucs;
        }
        else {
            if (src + nb >= src_end)
                goto done_toucs;
        }
        ch = 0;
        switch (nb) {
            /* these fall through deliberately */
        case 3: ch += *src++; ch <<= 6;
        case 2: ch += *src++; ch <<= 6;
        case 1: ch += *src++; ch <<= 6;
        case 0: ch += *src++;
        }
        ch -= offsetsFromUTF8[nb];
        dest[i++] = ch;
    }
 done_toucs:
    dest[i] = 0;
    return i;
}

/* srcsz = number of source characters, or -1 if 0-terminated
   sz = size of dest buffer in bytes

   returns # characters converted
   dest will only be '\0'-terminated if there is enough space. this is
   for consistency; imagine there are 2 bytes of space left, but the next
   character requires 3 bytes. in this case we could NUL-terminate, but in
   general we can't when there's insufficient space. therefore this function
   only NUL-terminates if all the characters fit, and there's space for
   the NUL as well.
   the destination string will never be bigger than the source string.
*/
int putf8_toutf8(char *dest, int sz, wchar_t *src, int srcsz)
{
    wchar_t ch;
    int i = 0;
    char *dest_end = dest + sz;

    while (srcsz<0 ? src[i]!=0 : i < srcsz) {
        ch = src[i];
        if (ch < 0x80) {
            if (dest >= dest_end)
                return i;
            *dest++ = (char)ch;
        }
        else if (ch < 0x800) {
            if (dest >= dest_end-1)
                return i;
            *dest++ = (ch>>6) | 0xC0;
            *dest++ = (ch & 0x3F) | 0x80;
        }
        #ifndef _WIN32
        #if UNICODE
        else if (ch < 0x10000) {
            if (dest >= dest_end-2)
                return i;
            *dest++ = (ch>>12) | 0xE0;
            *dest++ = ((ch>>6) & 0x3F) | 0x80;
            *dest++ = (ch & 0x3F) | 0x80;
        }
        else if (ch < 0x110000) {
            if (dest >= dest_end-3)
                return i;
            *dest++ = (ch>>18) | 0xF0;
            *dest++ = ((ch>>12) & 0x3F) | 0x80;
            *dest++ = ((ch>>6) & 0x3F) | 0x80;
            *dest++ = (ch & 0x3F) | 0x80;
        }
        #endif
        #endif

        i++;
    }
    if (dest < dest_end)
        *dest = '\0';
    return i;
}

int putf8_wc_toutf8(char *dest, wchar_t ch)
{
    if (ch < 0x80) {
        dest[0] = (char)ch;
        return 1;
    }
    if (ch < 0x800) {
        dest[0] = (ch>>6) | 0xC0;
        dest[1] = (ch & 0x3F) | 0x80;
        return 2;
    }

    #ifndef _WIN32
    #if UNICODE
    if (ch < 0x10000) {
        dest[0] = (ch>>12) | 0xE0;
        dest[1] = ((ch>>6) & 0x3F) | 0x80;
        dest[2] = (ch & 0x3F) | 0x80;
        return 3;
    }
    if (ch < 0x110000) {
        dest[0] = (ch>>18) | 0xF0;
        dest[1] = ((ch>>12) & 0x3F) | 0x80;
        dest[2] = ((ch>>6) & 0x3F) | 0x80;
        dest[3] = (ch & 0x3F) | 0x80;
        return 4;
    }
    #endif
    #endif

    return 0;
}

/* charnum => byte offset */
int putf8_offset(char *str, int charnum)
{
    int offs=0;

    while (charnum > 0 && str[offs]) {
        (void)(putf8_isutf(str[++offs]) || putf8_isutf(str[++offs]) ||
               putf8_isutf(str[++offs]) || ++offs);
        charnum--;
    }
    return offs;
}

/* byte offset => charnum */
int putf8_charnum(char *s, int offset)
{
    int charnum = 0, offs=0;

    while (offs < offset && s[offs]) {
        (void)(putf8_isutf(s[++offs]) || putf8_isutf(s[++offs]) ||
               putf8_isutf(s[++offs]) || ++offs);
        charnum++;
    }
    return charnum;
}

/* number of characters */
int putf8_strlen(char *s)
{
    int count = 0;

    while( *( s += putf8_seqlen( s ) ) )
        count++;

    return count;
}

/* reads the next utf-8 sequence out of a string, updating an index */
wchar_t putf8_nextchar(char *s, int *i)
{
    wchar_t ch = 0;
    int sz = 0;

    do {
        ch <<= 6;
        ch += s[(*i)++];
        sz++;
    } while (s[*i] && !putf8_isutf(s[*i]));
    ch -= offsetsFromUTF8[sz-1];

    return ch;
}

void putf8_inc(char *s, int *i)
{
    (void)(putf8_isutf(s[++(*i)]) || putf8_isutf(s[++(*i)]) ||
           putf8_isutf(s[++(*i)]) || ++(*i));
}

void putf8_dec(char *s, int *i)
{
    (void)(putf8_isutf(s[--(*i)]) || putf8_isutf(s[--(*i)]) ||
           putf8_isutf(s[--(*i)]) || --(*i));
}

int octal_digit(char c)
{
    return (c >= '0' && c <= '7');
}

int hex_digit(char c)
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f'));
}

/* assumes that src points to the character after a backslash
   returns number of input characters processed */
int putf8_read_escape_sequence(char *str, wchar_t *dest)
{
    wchar_t ch;
    char digs[9]="\0\0\0\0\0\0\0\0";
    int dno=0, i=1;

    ch = (wchar_t)str[0];    /* take literal character */
    if (str[0] == 'n')
        ch = L'\n';
    else if (str[0] == 't')
        ch = L'\t';
    else if (str[0] == 'r')
        ch = L'\r';
    else if (str[0] == 'b')
        ch = L'\b';
    else if (str[0] == 'f')
        ch = L'\f';
    else if (str[0] == 'v')
        ch = L'\v';
    else if (str[0] == 'a')
        ch = L'\a';
    else if (octal_digit(str[0])) {
        i = 0;
        do {
            digs[dno++] = str[i++];
        } while (octal_digit(str[i]) && dno < 3);
        ch = strtol(digs, NULL, 8);
    }
    else if (str[0] == 'x') {
        while (hex_digit(str[i]) && dno < 2) {
            digs[dno++] = str[i++];
        }
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    }
    else if (str[0] == 'u') {
        while (hex_digit(str[i]) && dno < 4) {
            digs[dno++] = str[i++];
        }
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    }
    else if (str[0] == 'U') {
        while (hex_digit(str[i]) && dno < 8) {
            digs[dno++] = str[i++];
        }
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    }
    *dest = ch;

    return i;
}

/* convert a string with literal \uxxxx or \Uxxxxxxxx characters to UTF-8
   example: putf8_unescape(mybuf, 256, "hello\\u220e")
   note the double backslash is needed if called on a C string literal */
int putf8_unescape(char *buf, int sz, char *src)
{
    int c=0, amt;
    wchar_t ch;
    char temp[4];

    while (*src && c < sz) {
        if (*src == '\\') {
            src++;
            amt = putf8_read_escape_sequence(src, &ch);
        }
        else {
            ch = (wchar_t)*src;
            amt = 1;
        }
        src += amt;
        amt = putf8_wc_toutf8(temp, ch);
        if (amt > sz-c)
            break;
        memcpy(&buf[c], temp, amt);
        c += amt;
    }
    if (c < sz)
        buf[c] = '\0';
    return c;
}

int putf8_escape_wchar(char *buf, int sz, wchar_t ch)
{
    if (ch == L'\n')
        return snprintf(buf, sz, "\\n");
    else if (ch == L'\t')
        return snprintf(buf, sz, "\\t");
    else if (ch == L'\r')
        return snprintf(buf, sz, "\\r");
    else if (ch == L'\b')
        return snprintf(buf, sz, "\\b");
    else if (ch == L'\f')
        return snprintf(buf, sz, "\\f");
    else if (ch == L'\v')
        return snprintf(buf, sz, "\\v");
    else if (ch == L'\a')
        return snprintf(buf, sz, "\\a");
    else if (ch == L'\\')
        return snprintf(buf, sz, "\\\\");
    else if (ch < 32 || ch == 0x7f)
        return snprintf(buf, sz, "\\x%hhX", ch);
    #ifndef _WIN32
    #if UNICODE
    else if (ch > 0xFFFF)
        return snprintf(buf, sz, "\\U%.8X", (wchar_t)ch);
    else if (ch >= 0x80 && ch <= 0xFFFF)
        return snprintf(buf, sz, "\\u%.4hX", (unsigned short)ch);
	#endif
	#endif

    return snprintf(buf, sz, "%c", (char)ch);
}

int putf8_escape(char *buf, int sz, char *src, int escape_quotes)
{
    int c=0, i=0, amt;

    while (src[i] && c < sz) {
        if (escape_quotes && src[i] == '"') {
            amt = snprintf(buf, sz - c, "\\\"");
            i++;
        }
        else {
            amt = putf8_escape_wchar(buf, sz - c, putf8_nextchar(src, &i));
        }
        c += amt;
        buf += amt;
    }
    if (c < sz)
        *buf = '\0';
    return c;
}

char *putf8_strchr(char *s, wchar_t ch, int *charn)
{
    int i = 0, lasti=0;
    wchar_t c;

    *charn = 0;
    while (s[i]) {
        c = putf8_nextchar(s, &i);
        if (c == ch) {
            return &s[lasti];
        }
        lasti = i;
        (*charn)++;
    }
    return NULL;
}

char *putf8_memchr(char *s, wchar_t ch, size_t sz, int *charn)
{
    int i = 0, lasti=0;
    wchar_t c;
    int csz;

    *charn = 0;
    while (i < sz) {
        c = csz = 0;
        do {
            c <<= 6;
            c += s[i++];
            csz++;
        } while (i < sz && !putf8_isutf(s[i]));
        c -= offsetsFromUTF8[csz-1];

        if (c == ch) {
            return &s[lasti];
        }
        lasti = i;
        (*charn)++;
    }
    return NULL;
}

int putf8_is_locale_utf8(char *locale)
{
    /* this code based on libutf8 */
    const char* cp = locale;

    for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++) {
        if (*cp == '.') {
            const char* encoding = ++cp;
            for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++)
                ;
            if ((cp-encoding == 5 && !strncmp(encoding, "UTF-8", 5))
                || (cp-encoding == 4 && !strncmp(encoding, "utf8", 4)))
                return 1; /* it's UTF-8 */
            break;
        }
    }
    return 0;
}

/*TESTCASE:UTF-8 functions
#include <phorward.h>
#include <locale.h>

void utf8_demo()
{
	char	str		[ 1024 ];
	char*	ptr;

	setlocale( LC_ALL, "" );

	strcpy( str, "Hällö ich bün ein StrÜng€!" );
	\*            0123456789012345678901234567890
	               0        1         2         3
	*\
	printf( "%ld %d\n", pstrlen( str ), putf8_strlen( str ) );

	putf8_unescape( str, sizeof( str ), "\\u20AC" );
	printf( ">%s< %d\n", str, putf8_char( str ) );
}
---
32 26
>€< 8364
*/
