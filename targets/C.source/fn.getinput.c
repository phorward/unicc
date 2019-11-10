UNICC_STATIC UNICC_CHAR @@prefix_get_input( @@prefix_pcb* pcb, unsigned int offset )
{
#if UNICC_DEBUG	> 2
	fprintf( stderr, "%s: get input: pcb->buf + offset = %p pcb->bufend = %p\n",
				UNICC_PARSER, pcb->buf + offset, pcb->bufend );
#endif

	while( pcb->buf + offset >= pcb->bufend )
	{
#if UNICC_DEBUG	> 2
			fprintf( stderr, "%s: get input: requiring more input\n",
					UNICC_PARSER );
#endif
		if( !pcb->buf )
		{
			pcb->bufend = pcb->buf = (UNICC_CHAR*)malloc(
				( UNICC_MALLOCSTEP + 1 ) * sizeof( UNICC_CHAR ) );

			if( !pcb->buf )
			{
				UNICC_OUTOFMEM( pcb );
				return 0;
			}

			*pcb->buf = 0;
		}
		else if( *pcb->buf && !( ( pcb->bufend - pcb->buf ) %
					UNICC_MALLOCSTEP ) )
		{
			unsigned int 	size	= (unsigned int)( pcb->bufend - pcb->buf );
			UNICC_CHAR*		buf;

			if( !( buf = (UNICC_CHAR*)realloc( pcb->buf,
						( size + UNICC_MALLOCSTEP + 1 )
							* sizeof( UNICC_CHAR ) ) ) )
			{
				UNICC_OUTOFMEM( pcb );
				free( pcb->buf );

				return 0;
			}

			pcb->buf = buf;
			pcb->bufend = pcb->buf + size;
		}

		if( pcb->is_eof || ( *( pcb->bufend ) = (UNICC_CHAR)UNICC_GETINPUT )
									== pcb->eof )
		{
#if UNICC_DEBUG	> 2
			fprintf( stderr, "%s: get input: can't get more input, "
						"end-of-file reached\n", UNICC_PARSER );
#endif
			pcb->is_eof = 1;
			return pcb->eof;
		}
#if UNICC_DEBUG	> 2
		fprintf( stderr, "%s: get input: read char >%c< %d\n",
					UNICC_PARSER, (char)*( pcb->bufend ), *( pcb->bufend ) );
#endif

#if UNICC_DEBUG	> 2
		fprintf( stderr, "%s: get input: reading character >%c< %d\n",
					UNICC_PARSER, (char)*( pcb->bufend ), *( pcb->bufend ) );
#endif

		*( ++pcb->bufend ) = 0;
	}

#if UNICC_DEBUG	> 2
	{
		UNICC_CHAR*		chptr;

		fprintf( stderr, "%s: get input: offset = %d\n",
					UNICC_PARSER, offset );
		fprintf( stderr, "%s: get input: buf = >" UNICC_CHAR_FORMAT "<\n",
					UNICC_PARSER, pcb->buf );
		fprintf( stderr, "%s: get input: returning %d\n",
					UNICC_PARSER, *( pcb->buf + offset ) );
	}
#endif

	return pcb->buf[ offset ];
}
