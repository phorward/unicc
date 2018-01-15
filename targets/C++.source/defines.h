/* Wide character processing enabled? */
#ifndef UNICC_WCHAR
#define UNICC_WCHAR					0
#endif

/* UTF-8 processing enabled? */
#if !UNICC_WCHAR
#ifndef UNICC_UTF8
#	define UNICC_UTF8				1
#endif
#else
#	ifdef UNICC_UTF8
#	undef UNICC_UTF8
#	endif
#	define UNICC_UTF8				0
#endif

/* UNICC_CHAR is used as character type for internal processing */
#ifndef UNICC_CHAR
#if UNICC_UTF8 || UNICC_WCHAR
#	define UNICC_CHAR				wchar_t
#	define UNICC_CHAR_FORMAT		"%S"
#else
#	define UNICC_CHAR				char
#	define UNICC_CHAR_FORMAT		"%s"
#endif
#endif /* UNICC_CHAR */

/* UNICC_SCHAR defines the character type for semantic action procession */
#ifndef UNICC_SCHAR
#if UNICC_WCHAR
#	define UNICC_SCHAR				wchar_t
#	define UNICC_SCHAR_FORMAT		"%S"
#else
#	define UNICC_SCHAR				char
#	define UNICC_SCHAR_FORMAT		"%s"
#endif
#endif /* UNICC_SCHAR */

/* Debug level */
#ifndef UNICC_DEBUG
#define UNICC_DEBUG				0
#endif

/* Stack debug switch */
#ifndef UNICC_STACKDEBUG
#define UNICC_STACKDEBUG		0
#endif

/* Parse error macro */
#ifndef UNICC_PARSE_ERROR
#define UNICC_PARSE_ERROR( parser ) \
	fprintf( stderr, "line %d, column %d: syntax error on symbol %d, token '" \
		UNICC_SCHAR_FORMAT "'\n", \
	parser->line, parser->column, parser->sym, parser->get_lexem() )
#endif

/* Input buffering clean-up */
#ifndef UNICC_CLEARIN
#define UNICC_CLEARIN( parser )	parser->clear_input()
#endif

/*TODO:*/
#ifndef UNICC_NO_INPUT_BUFFER
#define UNICC_NO_INPUT_BUFFER	0
#endif

/* Memory allocation step size for dynamic stack- and buffer allocation */
#ifndef UNICC_MALLOCSTEP
#define UNICC_MALLOCSTEP		128
#endif

/* Call this when running out of memory during memory allocation */
#ifndef UNICC_OUTOFMEM
#define UNICC_OUTOFMEM			fprintf( stderr, \
									"Fatal error, ran out of memory\n" ), \
								exit( 1 )
#endif

#ifdef UNICC_PARSER
#undef UNICC_PARSER
#endif
#define UNICC_PARSER			"@@prefix" "debug"

/* Don't change next three defines below! */
#ifndef UNICC_ERROR
#define UNICC_ERROR				0
#endif
#ifndef UNICC_SHIFT
#define UNICC_SHIFT				2
#endif
#ifndef UNICC_REDUCE
#define UNICC_REDUCE			1
#endif

/* Error delay after recovery */
#ifndef UNICC_ERROR_DELAY
#define UNICC_ERROR_DELAY		3
#endif

/* Enable/Disable terminal selection in semantic actions */
#ifndef UNICC_SEMANTIC_TERM_SEL
#define UNICC_SEMANTIC_TERM_SEL	0
#endif

#define UNICC_GETINPUT             getchar()
