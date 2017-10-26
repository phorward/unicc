#ifndef UNICC_GETINPUT

#if UNICC_UTF8
static int offsets_utf8[ 6 ] =
{
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static int trailbyte_utf8[ 256 ] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

UNICC_STATIC UNICC_CHAR @@prefix_utf8_getchar( int (*getfn)() )
{
	UNICC_CHAR	ch	= 0;
	int 		nb;
	int			c;
	
	if( !getfn )
		getfn = getchar;

	switch( ( nb = trailbyte_utf8[ ( c = (*getfn)() ) ] ) )
	{
        case 3:
			ch += c;
			ch <<= 6;
			c = (*getfn)();
        case 2:
			ch += c;
			ch <<= 6;
			c = (*getfn)();
        case 1:
			ch += c;
			ch <<= 6;
			c = (*getfn)();
        case 0:
			ch += c;
			break;
	}
	
	ch -= offsets_utf8[ nb ];
#if UNICC_DEBUG	> 3
	fprintf( stderr, "%s: getchar: %d\n", UNICC_PARSER, ch );
#endif
	return ch;
}
#define UNICC_GETINPUT		@@prefix_utf8_getchar( getchar )

#else
#define UNICC_GETINPUT		getchar()
#endif

#endif
