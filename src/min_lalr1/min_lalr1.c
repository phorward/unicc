/*
	min_lalr1 - Experimental Minimalist LALR(1) Parser Generator
	Copyright (C) 2007-2012 by Phorward Software Technologies, Jan Max Meyer
	http://www.phorward-software.com  contact<at>phorward<dash>software<dot>com
	All rights reserved. See LICENSE for more information.
	
	This program represents a modified experimental implementation of the
	LALR(1) parse table construction algorithm for testing and experimental
	purposes.
	
	This program represents a prototype for a simple LALR(1) parser generator
	that matches single characters as terminals, and keywords. The keyword
	features has been implemented very rude in this software (via strcmp ;))

	This prototype is only used for research and development on the LALR(1)
	parser generation algorithms, optimizations and character-based parsing.
	It was used for experimental tests that resultet in the UniCC parser
	generator, which uses min_lalr1 for bootstrapping.

	DO NEVER USE THIS SOFTWARE FOR PRODUCTION PURPOSES!!!
	YOU'RE SEARCHING FOR DOCUMENTATION? LOOK INTO THIS SOURCE ;)
*/

/* for MSVC... */
#define _CRT_SECURE_NO_DEPRECATE 1
#pragma warning( disable : 4047 4024 4996 )

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define uchar unsigned char

typedef struct sym_ SYM;
typedef struct prod_ PROD;
typedef struct item_ ITEM;
typedef struct state_ STATE;
typedef struct tabcol_ TABCOL;

/*
 * Everything in min_lalr1 is constant
 */
#define MAXRHS			128
#define MAXPROD			32
#define MAXSTR			256
#define MAXFIRST		64
#define MAXNUM			512
#define MAXTAB			128

#define S_UNDEF			-1
#define S_NONTERM		0
#define S_TERM			1

#define REDUCE			1
#define SHIFT			2
#define SHIFT_REDUCE	3

#define	ERROR_TOKEN_NAME	"ERROR"


/* Switch features */
#define WITH_ITEM_IN_TABCOL 1
#define HIDE_WHITESPACE_ERRORS ( !getenv( "MIN_LALR1_SHOW_WS_ERRORS" ) )

struct prod_
{
	SYM* 	lhs;
	SYM*	rhs[MAXRHS];
	int		rhs_cnt;
	uchar*	code;
	int		prec;
};

struct sym_
{
	int	id;
	uchar 	name[MAXSTR];
	int	att;
	int 	type;
	PROD*	prod[MAXPROD];
	int	prod_cnt;
	SYM*	first[MAXFIRST];
	int	first_cnt;
	int	nullable;	
	int	prec;
	int	assoc;
	int	whitespace;
	int	lexem;
	int	generated;
};

struct item_
{
	PROD*	prod;
	int	dot_offset;
	SYM*	lookahead[MAXFIRST];
	int	lookahead_cnt;
	ITEM* derived_from;
};
#define SYM_RIGHT_OF_DOT( item ) ( (item).prod->rhs[(item).dot_offset] )

/*
	This source is for C haxX0rs only!
*/


struct tabcol_
{
	int	sym;
	int	act;
	int idx;
	#if WITH_ITEM_IN_TABCOL
	ITEM	item;
	#endif
};

struct state_
{
	int	id;
	ITEM	kernel[MAXPROD];
	int	kernel_cnt;
	ITEM	epsilon[MAXPROD];
	int	epsilon_cnt;
	
	TABCOL	acttab[MAXTAB];
	int	acttab_cnt;
	TABCOL	gototab[MAXTAB];
	int	gototab_cnt;

	int	closed;
	int	done;
};

SYM		sym[ MAXNUM ];
int		sym_cnt = 0;
SYM*	goal	= (SYM*)NULL;
PROD	prod[ MAXNUM ];
int		prod_cnt = 0;
STATE	state[ MAXNUM ];
int		state_cnt = 0;

int		prec_count	= 0;

uchar*	head_code = (uchar*)NULL;
uchar*	body_code = (uchar*)NULL;



/* Flags */
int		distinguish_lexemes	= 0;


char* print_charset( char* charset )
{
	static char out[MAXSTR*5];
	int i,j;
	for( i = 0, j = 0; i < strlen( charset ); i++ )
	{
		if( charset[i] >= 32 && charset[i] <= 128 )
			out[j++] = charset[i];
		else
		{
			sprintf( out + j, "\\%d", charset[i] );
			j += strlen( out+j );
		}
	}
	out[j] = '\0';
	
	return out;
}

void symdef_to_charmap( uchar* def, uchar* map )
{
	uchar charmap[MAXSTR];
	uchar ch;
	int i = 0,
		j,
		neg = 0;
		
	/* fprintf( stderr, "@@@ mapping >%s< ", def ); */
		
	if( def[i] == '^' )
	{
		i++;
		neg = 1;
	}
	
	for( j = 0; j < MAXSTR; j++ )
		charmap[j] = neg ? 1 : 0;
	
	for( ; i < strlen( def ) && i < MAXSTR; i++ )
	{
		if( def[i+1] == '-' )
		{
			if( def[i] > def[i+2] )
			{
				ch = def[i];
				def[i] = def[i+2];
				def[i+2] = ch;
			}
			
			for( ch = def[i]; ch <= def[i+2]; ch++ )
				charmap[ch] = neg ? 0 : 1;
				
			i+=2;
		}
		else
			charmap[def[i]] = neg ? 0 : 1;
	}
	
	for( i = 1, j = 0; i < MAXSTR; i++ )
		if( charmap[i] )
			map[j++] = i;
	
	map[j] = '\0';
	
	/* fprintf( stderr, "to >%s<\n", map ); */
}


#define SYM_ATT_NONE	0
#define SYM_ATT_KEYWORD	1
#define SYM_ATT_ERROR	2

SYM* get_symbol( uchar* n, int t, int auto_create, int att )
{
	int i;

	for( i = 0; i < sym_cnt; i++ )
		if( sym[i].type == t &&
				strcmp( sym[i].name, n ) == 0 &&
					sym[i].att == att )
			return &(sym[i]);
	
	if( auto_create )
	{
		memset( &(sym[sym_cnt]), 0, sizeof( SYM ) );

		sym[sym_cnt].id = sym_cnt+1;
		sym[sym_cnt].type = t;
		
		strcpy( sym[sym_cnt].name, n );
		
		if( t == S_TERM )
			sym[sym_cnt].att = att;
		

		sym_cnt++;
		return &(sym[sym_cnt-1]);
	}
	
	return (SYM*)NULL;
}

#define ASSOC_NONE		0
#define ASSOC_LEFT		1
#define ASSOC_RIGHT		2
#define ASSOC_NOASSOC		3

#define T_UNDEF			-2
#define T_EOF			-1
#define T_NONTERM		0
#define T_TERM			1
#define T_KW			2
#define T_CODE			3
#define T_PIPE			4
#define T_COLON			5
#define T_SEMICOLON		6
#define T_LEFT			7
#define T_RIGHT			8
#define T_MIDDLE		9
#define T_TILDE			10
#define T_DOLLAR		11
#define T_ANYCHAR		12
#define T_NEGATE		13
#define T_EXCLAM		14
#define	T_HASH			15

#define PERROR( txt )	{ fprintf( stderr, "%s (%s)\n", txt, src ); getchar(); exit( -1 ); }
#define LA( x ) ( lookahead == (x) )
#define NEXT	p_scan( &src, &lookahead, &attribute )

int		lookahead	= T_UNDEF;
uchar*	attribute	= (uchar*)NULL;
uchar*	src			= (uchar*)NULL;


void unescape_string( uchar* str )
{
	int i,j;
	
	for( i = 0, j = 0; i < strlen( str ); i++, j++ )
	{
		if( str[i] == '\\' )
		{
			i++;
			switch( str[i] )
			{
				case 'n':
					str[j] = '\n';
					break;
				case 't':
					str[j] = '\t';
					break;
				case 'r':
					str[j] = '\r';
					break;
				case '0':
					str[j] = '\0';
					break;
				default:
					str[j] = str[i];
					break;
			}
		}
	}
	str[j] = '\0';
}

void p_scan( uchar** s, int* la, uchar** att )
{
	uchar*		start,
		*		end;
	int			bcnt		= 0;
	
	if( *att )
	{
		free( *att );
		*att = (uchar*)NULL;
	}
	
	getsym:
	
	while( **s == ' ' || **s == '\t' || **s == '\n' || **s == '\r' )
		(*s)++;
	
	start = end = *s;
	
	if( *end >= 'a' && *end <= 'z'
				|| *end >= 'A' && *end <= 'Z'
					|| *end == '_' )
	{
		while( *end >= 'a' && *end <= 'z'
				|| *end >= 'A' && *end <= 'Z'
					|| *end >= '0' && *end <= '9'
						|| *end == '_' )
		{
			if( *end == '\0' )
				break;

			end++;
			(*s)++;
		}
		
		*la = T_NONTERM;
	}
	else if( *end == '\'' )
	{
		start++;
		do
		{
			end++;
			(*s)++;
						
			if( *end == '\\' )
			{
				end++;
				(*s)++;
			}
			else if( *end == '\'' )
				break;
		}
		while( 1 );
		(*s)++;
		*la = T_TERM;
	}
	else if( *end == '\"' )
	{
		start++;
		do
		{
			end++;
			(*s)++;
						
			if( *end == '\\' )
			{
				end++;
				(*s)++;
			}
			else if( *end == '\"' )
				break;
		}
		while( 1 );
		(*s)++;
		
		*la = T_KW;
	}
	else if( *end == '{' )
	{
		start++;
		
		do
		{
			
			end++;
			(*s)++;
			
			if( *end == '}' )
			{
				if( bcnt == 0 )
					break;
				else
					bcnt--;
			}
			else if( *end == '{' )
				bcnt++;
			else if( *end == '\0' )
				break;
			else if( *end == '\\' )
			{
				end++;
				(*s)++;
			}
		}
		while( 1 );
		
		if( *end != '\0' )
			(*s)++;
		
		*la = T_CODE;
	}
	else if( *end == '|' )
	{		
		(*s)++;
		*la = T_PIPE;
	}
	else if( *end == ':' )
	{
		(*s)++;
		*la = T_COLON;
	}
	else if( *end == ';' )
	{
		(*s)++;
		*la = T_SEMICOLON;
	}
	else if( *end == '^' )
	{
		(*s)++;
		*la = T_MIDDLE;
	}
	else if( *end == '<' )
	{
		(*s)++;
		*la = T_LEFT;
	}
	else if( *end == '>' )
	{
		(*s)++;
		*la = T_RIGHT;
	}
	else if( *end == '~' )
	{
		(*s)++;
		*la = T_TILDE;
	}
	else if( *end == '!' )
	{
		(*s)++;
		*la = T_EXCLAM;
	}
	else if( *end == '$' )
	{
		(*s)++;
		*la = T_DOLLAR;
	}
	else if( *end == '#' )
	{
		(*s)++;
		*la = T_HASH;
	}
	else if( *end == '/' && *(end+1) == '*' )
	{
		while( **s != '\0' )
		{
			if( **s == '*' && *((*s)+1) == '/' )
			{
				(*s) += 2;
				break;
			}
			(*s)++;
		}
		
		goto getsym;
	}
	else if( *end == '\0' )
		*la = T_EOF;
	else
		*la = T_UNDEF;

	*att = (uchar*)malloc( ( end - start + 1 ) * sizeof( uchar ) );
	**att = '\0';

	if( end > start )
	{
		memcpy( *att, start, ( end - start ) * sizeof( uchar ) );
		(*att)[ ( end - start ) * sizeof( uchar ) ] = '\0';
		
		if( *la == T_TERM || *la == T_KW )
		{
			unescape_string( *att );
		}
	}
}


void p_sym( PROD* p )
{
	uchar mapstr[MAXSTR];
	
	if( LA( T_TERM ) || LA( T_KW ) )
	{
		if( LA( T_TERM ) )
			symdef_to_charmap( attribute, mapstr );

		p->rhs[(p->rhs_cnt)++] = get_symbol( LA( T_TERM ) ? mapstr : attribute, S_TERM, 1, ( LA( T_KW ) ? SYM_ATT_KEYWORD : SYM_ATT_NONE ) );
	}
	else if( LA( T_HASH ) )
	{
		NEXT;
		if( LA( T_NONTERM ) && strcmp( attribute, ERROR_TOKEN_NAME ) == 0 )
			p->rhs[(p->rhs_cnt)++] = get_symbol( ERROR_TOKEN_NAME, S_TERM, 1, SYM_ATT_ERROR );
	}
	else
		p->rhs[(p->rhs_cnt)++] = get_symbol( attribute, S_NONTERM, 1, SYM_ATT_NONE );

	NEXT;
}

void p_seq( PROD* p )
{
	while( LA( T_TERM ) || LA( T_KW ) || LA( T_HASH ) || LA( T_NONTERM ) )
		p_sym( p );
}

void p_alt( SYM* lhs )
{
	do
	{
		NEXT;
		
		prod[prod_cnt].lhs = lhs;
		lhs->prod[(lhs->prod_cnt)++] = &(prod[prod_cnt]);

		p_seq( &(prod[prod_cnt]) );
		if( prod[prod_cnt].rhs_cnt == 0 )
			lhs->nullable = 1;
			
		if( LA( T_CODE ) )
		{
			prod[prod_cnt].code = attribute;
			attribute = (uchar*)NULL;
			NEXT;
		}
		/*
		else if( prod[prod_cnt].rhs_cnt > 0 )
			prod[prod_cnt].code = strdup( DEF_ACTION );
		*/
		prod_cnt++;
	}
	while( LA( T_PIPE ) );
}

void p_def( uchar* s )
{
	SYM*	lhs;
	int		body = 0;

	src = s;
	NEXT;

	while( !( LA( T_EOF ) ) )
	{
		if( LA( T_NONTERM ) )
		{
			body = 1;

			lhs = get_symbol( attribute, S_NONTERM, 1, SYM_ATT_NONE );

			NEXT;
			if( LA( T_COLON ) )
				p_alt( lhs );
			else
			{
				PERROR( "Expecting colon (:)" )
				NEXT;
			}
			
			if( !goal )
			{
				goal = lhs;

				if( lhs->prod_cnt > 1 )
				{
					fprintf( stderr, "Error: Invalid goal-symbol '%s' has more than one right-hand side!", lhs->name );
					getchar();
					exit(-1);
				}
			}

			if( !( LA( T_SEMICOLON ) ) )
				PERROR( "Expecting semicolon (;)" )
			NEXT;
		}
		else if( LA( T_LEFT ) || LA( T_RIGHT ) || LA( T_MIDDLE ) )
		{
			int assoc = ASSOC_NONE;
			SYM* sym;
			
			switch( lookahead )
			{
				case T_LEFT:
					assoc = ASSOC_LEFT;
					break;
				case T_RIGHT:
					assoc = ASSOC_RIGHT;
					break;
				case T_MIDDLE:
					assoc = ASSOC_NOASSOC;
					break;
				default:
					break;
			}
			
			prec_count++;
			do
			{
				NEXT;
				if( LA( T_TERM ) || LA( T_KW ) )
					sym = get_symbol( attribute, S_TERM, 1, ( LA( T_KW ) ? SYM_ATT_KEYWORD : SYM_ATT_NONE ) );
				else if( LA( T_NONTERM ) )
					sym = get_symbol( attribute, S_NONTERM, 1, SYM_ATT_NONE );
				else if( !( LA( T_SEMICOLON ) ) )
				{
					PERROR( "Expecting token!" );
					break;
				}
				
				sym->prec = prec_count;
				sym->assoc = assoc;
			}
			while( !( LA( T_SEMICOLON ) ) );
			NEXT;
		}
		else if( LA( T_TILDE ) )
		{
			SYM* sym;

			do
			{
				NEXT;
				if( LA( T_TERM ) || LA( T_KW ) )
					sym = get_symbol( attribute, S_TERM, 1, ( LA( T_KW ) ? SYM_ATT_KEYWORD : SYM_ATT_NONE ) );
				else if( LA( T_NONTERM ) )
					sym = get_symbol( attribute, S_NONTERM, 1, SYM_ATT_NONE );
				else if( !( LA( T_SEMICOLON ) ) )
				{
					PERROR( "Expecting token!" );
					break;
				}

				sym->whitespace = 1;
			}
			while( !( LA( T_SEMICOLON ) ) );
			NEXT;
		}
		else if( LA( T_DOLLAR ) )
		{
			SYM* sym;
			
			do
			{
				NEXT;
				if( LA( T_TERM ) || LA( T_KW ) )
					sym = get_symbol( attribute, S_TERM, 1, ( LA( T_KW ) ? SYM_ATT_KEYWORD : SYM_ATT_NONE ) );
				else if( LA( T_NONTERM ) )
					sym = get_symbol( attribute, S_NONTERM, 1, SYM_ATT_NONE );
				else if( !( LA( T_SEMICOLON ) ) )
				{
					PERROR( "Expecting token!" );
					break;
				}

				sym->lexem = 1;
			}
			while( !( LA( T_SEMICOLON ) ) );
			NEXT;
		}
		else if( LA( T_CODE ) )
		{
			if( body )
			{
				if( !body_code )
				{
					body_code = attribute;
					attribute = (uchar*)NULL;
				}
				else
				{
					body_code = (uchar*)realloc( (uchar*)body_code, ( strlen( body_code ) + strlen( attribute ) + 1 ) * sizeof( uchar ) );
					strcat( body_code, attribute );
				}
			}
			else
			{
				if( !head_code )
				{
					head_code = attribute;
					attribute = (uchar*)NULL;
				}
				else
				{
					head_code = (uchar*)realloc( (uchar*)head_code, ( strlen( head_code ) + strlen( attribute ) + 1 ) * sizeof( uchar ) );
					strcat( head_code, attribute );
				}
			}
			NEXT;
		}
		else if( LA( T_EXCLAM ) )
		{
			do
			{
				NEXT;
				if( LA( T_NONTERM ) )
				{
					#if WITH_ITEM_IN_TABCOL
					if( strcmp( attribute, "dl" ) == 0 )
						distinguish_lexemes = 1;
					else
					#endif
						PERROR( "Unknown flag" );
				}
				else if( !( LA( T_SEMICOLON ) ) )
				{
					PERROR( "Expecting token!" );
					break;
				}

				sym->lexem = 1;
			}
			while( !( LA( T_SEMICOLON ) ) );
			NEXT;
			
		}
		else
			PERROR( "Parse error at definition" )
	}
	
	free( attribute );
}

void printgrm( void )
{
	int i, j, k;

	fprintf( stderr, "-= Grammar overview =-\n" );
	for( i = 0; i < sym_cnt; i++ )
	{
		if( 1 /* sym[i].type == S_NONTERM */ )
		{
			fprintf( stderr, "\n%s [ ", print_charset( sym[i].name ) );
			for( j = 0; j < sym[i].first_cnt; j++ )
			{
				fprintf( stderr, "'%s' ", print_charset( sym[i].first[j]->name ) );
			}
			fprintf( stderr, "] nullable:%d assoc:%c prec:%d lexem:%d\n", sym[i].nullable,
				( sym[i].assoc == ASSOC_LEFT ? 'l' : ( sym[i].assoc == ASSOC_RIGHT ? 'r' : ( sym[i].assoc == ASSOC_NOASSOC ? 'n' : '-' ) ) ),
					sym[i].prec, sym[i].lexem );

			for( j = 0; j < sym[i].prod_cnt; j++ )
			{
				fprintf( stderr, "  %s: ", print_charset( sym[i].name ) );
				for( k = 0; k < sym[i].prod[j]->rhs_cnt; k++ )
				{
					if( sym[i].prod[j]->rhs[k]->type == S_NONTERM )
						fprintf( stderr, "%s ", print_charset( sym[i].prod[j]->rhs[k]->name ) );
					else
						fprintf( stderr, "'%s' ", print_charset( sym[i].prod[j]->rhs[k]->name ) );
					
				}
				fprintf( stderr, "   prec:%d\n", sym[i].prod[j]->prec );
			}
		}
	}

	fprintf( stderr, "\n\n" );
}

void unique_charsets( void )
{
	int		i,j,k,l;
	uchar 	cmap[MAXNUM],
			nmap[MAXNUM],
			xmap[MAXNUM];
	SYM*	csets[MAXNUM];
	int		csets_cnt;
	int		old_prod_cnt;
	int		done_something;
	uchar	name[MAXNUM];
	
	do
	{
		old_prod_cnt = prod_cnt;

		for( i = 0; i < sym_cnt; i++ )
		{
			if( sym[i].type != S_TERM || sym[i].att & SYM_ATT_KEYWORD || sym[i].att & SYM_ATT_ERROR )
				continue;
	
			memset( cmap, 0, MAXNUM * sizeof( uchar ) );
			for( j = 0; j < strlen( sym[i].name ); j++ )
				cmap[ sym[i].name[j] ] = sym[i].id;
			
			for( j = 0; j < sym_cnt; j++ )
			{
				if( sym[j].type != S_TERM || sym[j].att & SYM_ATT_KEYWORD || sym[j].att & SYM_ATT_ERROR )
					continue;
	
				if( j != i )
					for( k = 0; k < strlen( sym[j].name ); k++ )
						if( cmap[ sym[j].name[k] ] == sym[i].id )
							cmap[ sym[j].name[k] ] = sym[j].id;
			}
			
			csets_cnt = 0;
			for( j = 0; j < sym_cnt; j++ )
			{
				if( sym[j].type != S_TERM || sym[j].att & SYM_ATT_KEYWORD || sym[j].att & SYM_ATT_ERROR )
					continue;
				
				done_something = 0;
				memset( nmap, 0, MAXNUM * sizeof( uchar ) );
				
				for( k = 0; k < MAXNUM; k++ )
					if( cmap[k] == sym[j].id )
					{
						nmap[k] = 1;
						done_something = 1;
					}
				
				if( done_something )
				{
					/* Is there already such a set? */
					for( k = 0; k < sym_cnt; k++ )
					{
						if( sym[k].type != S_TERM || sym[k].att & SYM_ATT_KEYWORD || sym[k].att & SYM_ATT_ERROR )
							continue;
	
						memset( xmap, 0, MAXNUM * sizeof( uchar ) );
						for( l = 0; l < strlen( sym[k].name ); l++ )
							xmap[ sym[k].name[l] ] = 1;
							
						if( memcmp( nmap, xmap, MAXNUM * sizeof( uchar ) ) == 0 )
							break;
					}
					
					if( k != i )
					{
						*name = '\0';
						for( k = 0; k < MAXNUM; k++ )
							if( nmap[k] )
								sprintf( name + strlen( name ), "%c", k );
						csets[ csets_cnt++ ] = get_symbol( name, S_TERM, 1, SYM_ATT_NONE );
					}
				}
			}
			
			if( csets_cnt > 0 )
			{
				strcpy( name, sym[i].name );
				memset( &( sym[i] ), 0, sizeof( SYM ) );
							
				sprintf( sym[i].name, "%s#", name );
				sym[i].type = S_NONTERM;
				sym[i].id = i + 1;
				
				for( j = 0; j < csets_cnt; j++ )
				{
					prod[ prod_cnt ].rhs[0] = csets[ j ];
					prod[ prod_cnt ].rhs_cnt = 1;
					prod[ prod_cnt ].lhs = &( sym[i] );
					sym[i].prod[ sym[i].prod_cnt++ ] = &( prod[ prod_cnt ] );
	
					prod_cnt++;
				}
			}
		}
	}
	while( old_prod_cnt < prod_cnt );
}

void rewrite_grammar( void )
{
	int i,j,k;
	uchar n[MAXNUM];
	int old_prod_cnt = prod_cnt;
	int tmp_prod_cnt;

	SYM tmp_sym;
	SYM	* s,
		* ns,
		* ws;
	
	SYM* stack[MAXNUM];
	int stack_cnt = 0;
	int done_cnt = 0;
	SYM* done[MAXNUM];
	int mods_cnt = 0;
	
	memset( &tmp_sym, 0, sizeof( tmp_sym ) );
	
	for( i = 0; i < sym_cnt; i++ )
	{
		if( sym[i].whitespace )
		{
			/* Make "?: X;" */
			prod[prod_cnt].rhs[0] = &(sym[i]);
			prod[prod_cnt].rhs_cnt = 1;
			prod_cnt++;
		}
	}
	
	/* fprintf( stderr, "%d %d\n", old_prod_cnt, prod_cnt ); */
	if( old_prod_cnt < prod_cnt )
	{
		ws = get_symbol( "WS...", S_NONTERM, 1, SYM_ATT_NONE );
		ws->lexem = 1;
		ws->generated = 1;
		ws->whitespace = 1;
		
		tmp_prod_cnt = prod_cnt;
		for( i = old_prod_cnt; i < tmp_prod_cnt; i++ )
		{
			prod[i].lhs = ws;
			ws->prod[ws->prod_cnt++] = &(prod[i]);
			
			/* Make "ws: ws X;" */
			prod[prod_cnt].lhs = ws;
			prod[prod_cnt].rhs[0] = ws;
			prod[prod_cnt].rhs[1] = prod[i].rhs[0];
			prod[prod_cnt].rhs_cnt = 2;
			
			ws->prod[ws->prod_cnt++] = &(prod[prod_cnt]);
			
			prod_cnt++;
		}
		
		/* Make "ws: ___WS;" */
		prod[prod_cnt].rhs[0] = ws;
		prod[prod_cnt].rhs_cnt = 1;
		
		ws = get_symbol( "WS?...", S_NONTERM, 1, SYM_ATT_NONE );
		ws->nullable = 1;
		ws->lexem = 1;
		ws->generated = 1;
		ws->whitespace = 1;
		
		prod[prod_cnt].lhs = ws;
		ws->prod[ws->prod_cnt++] = &(prod[prod_cnt+1]); /* first, add the epsilon! */
		ws->prod[ws->prod_cnt++] = &(prod[prod_cnt]);
		prod_cnt++;
		
		/* Make "ws: ;" */
		prod[prod_cnt].lhs = ws;		
		prod_cnt++;
		
		/*
			Rewrite grammar, part 1:
			Find out all lexeme non-terminals and those
			which belong to them.
		*/
		for( i = 0; i < sym_cnt; i++ )
		{
			if( sym[i].lexem && sym[i].type == S_NONTERM )
			{
				stack[stack_cnt++] = &(sym[i]);
				done[done_cnt++] = &(sym[i]);
			}
		}
		
		while( stack_cnt )
		{
			stack_cnt--;
			s = stack[stack_cnt];
			for( i = 0; i < s->prod_cnt; i++ )
			{
				for( j = 0; j < s->prod[i]->rhs_cnt; j++ )
				{
					if( s->prod[i]->rhs[j]->type == S_NONTERM )
					{
						for( k = 0; k < done_cnt; k++ )
							if( done[k] == s->prod[i]->rhs[j] )
								break;

						if( k == done_cnt )
						{
							done[done_cnt++] = stack[stack_cnt++] = s->prod[i]->rhs[j];
							s->prod[i]->rhs[j]->lexem = 1;
						}
					}
				}
			}
		}
		
		/*
			Rewrite grammar, part 2:
			Find all non-terminals from goal; If there is a call to a
			lexem non-terminal or a terminal, rewrite their rules and
			replace them.
		*/
		if( prod->lhs->lexem == 0 )
		{
			done_cnt = 0;
			
			stack[stack_cnt++] = prod->lhs;
			done[done_cnt++] = prod->lhs;
			
			while( stack_cnt )
			{
				stack_cnt--;
				s = stack[stack_cnt];
				for( i = 0; i < s->prod_cnt; i++ )
				{
					for( j = 0; j < s->prod[i]->rhs_cnt; j++ )
					{
						if( s->prod[i]->rhs[j]->type == S_NONTERM
							&& s->prod[i]->rhs[j]->lexem == 0 )
						{
							for( k = 0; k < done_cnt; k++ )
								if( done[k] == s->prod[i]->rhs[j] )
									break;
	
							if( k == done_cnt )
								done[done_cnt++] = stack[stack_cnt++] = s->prod[i]->rhs[j];
						}
						else if( ( s->prod[i]->rhs[j]->type == S_NONTERM
							&& s->prod[i]->rhs[j]->lexem == 1 )
								|| s->prod[i]->rhs[j]->type == S_TERM 
								&& !( s->prod[i]->rhs[j]->att & SYM_ATT_ERROR ) )
						{
							strcpy( n, s->prod[i]->rhs[j]->name );
							strcat( n, "'" );
							
							if( ( ns = get_symbol( n, S_NONTERM, 0, SYM_ATT_NONE ) ) == (SYM*)NULL )
							{
								ns = get_symbol( n, S_NONTERM, 1, SYM_ATT_NONE );
																
								prod[prod_cnt].lhs = ns;
								prod[prod_cnt].rhs[0] = s->prod[i]->rhs[j];
								prod[prod_cnt].rhs[1] = ws;
								prod[prod_cnt].rhs_cnt = 2;
								
								ns->prod[0] = &(prod[prod_cnt]);
								ns->prod_cnt = 1;
								ns->prec = s->prod[i]->rhs[j]->prec;
								ns->assoc = s->prod[i]->rhs[j]->assoc;
								ns->nullable = s->prod[i]->rhs[j]->nullable;
								ns->generated = 1;

								//prod[prod_cnt].prec = s->prod[i]->rhs[j]->prec;
								
								prod_cnt++;
							}

							s->prod[i]->rhs[j] = ns;
						}
					}
				}
			}
		}
		
		/*
			Rewrite grammar, part 3:
			Make a goal-symbol with leading whitespaces
		*/
		strcpy( n, goal->name );
		strcat( n, "'" );
		
		ns = get_symbol( n, S_NONTERM, 1, SYM_ATT_NONE );
		
		prod[prod_cnt].lhs = ns;
		prod[prod_cnt].rhs[0] = ws;
		prod[prod_cnt].rhs[1] = goal;
		prod[prod_cnt].rhs_cnt = 2;
		
		ns->prod[0] = &(prod[prod_cnt]);
		ns->prod_cnt = 1;
		
		prod_cnt++;
		
		goal = ns;
	}
}

int union_array( SYM** dest, int* dest_cnt, SYM** src, int src_cnt )
{
	int i, j, n = 0;
	for( i = 0; i < src_cnt; i++ )
	{
		for( j = 0; j < *dest_cnt; j++ )
		{
			if( src[i] == dest[j] )
				break;
		}
		
		if( j == *dest_cnt )
		{
			dest[(*dest_cnt)++] = src[i];
			n++;
		}
	}

	return n;
}


void dofirst( void )
{
	int		oldcnt	= 0,
			cnt 	= 0,
			i		= 0,
			j		= 0,
			k		= 0,
			nullable= 0;

	do
	{
		oldcnt = cnt;

		for( i = 0; i < sym_cnt; i++ )
		{
			if( sym[i].type == S_TERM &&
					sym[i].first_cnt == 0 
				&& !( sym[i].att & SYM_ATT_ERROR ) )
			{
				sym[i].first[0] = &(sym[i]);
				sym[i].first_cnt = 1;
				cnt++;
			}
			else if( sym[i].type == S_NONTERM )
				for( j = 0; j < sym[i].prod_cnt; j++ )
				{
					nullable = 0;
					for( k = 0; k < sym[i].prod[j]->rhs_cnt; k++ )
					{
						cnt += union_array( sym[i].first, &(sym[i].first_cnt),
							sym[i].prod[j]->rhs[k]->first, sym[i].prod[j]->rhs[k]->first_cnt );

						nullable = sym[i].prod[j]->rhs[k]->nullable;
						if( !nullable )
							break;
					}

					sym[i].nullable |= nullable;
				}
		}
	}
	while( oldcnt != cnt );
}


int get_rhs_first( ITEM* dest, PROD* p, int off )
{
	int 	i;

	for( i = off; i < p->rhs_cnt; i++ )
	{
		union_array( dest->lookahead, &(dest->lookahead_cnt), p->rhs[i]->first, p->rhs[i]->first_cnt );

		if( !(p->rhs[i]->nullable) )
			break;
	}

	return ( i == p->rhs_cnt ) ? 0 : 1;
}


void print_item( FILE* where, ITEM* item )
{
	int j;
	
	fprintf( where, "  " );
	fprintf( where, "%s: ", print_charset( item->prod->lhs->name ) );

	for( j = 0; j < item->prod->rhs_cnt; j++ )
	{
		if( j == item->dot_offset )
			fprintf( where, "." );
		if( item->prod->rhs[j]->type == S_TERM )
			fprintf( where, "\"%s\" ", print_charset( item->prod->rhs[j]->name ) );
		else
			fprintf( where, "%s ", print_charset( item->prod->rhs[j]->name ) );
	}
	if( j == item->dot_offset )
		fprintf( where, "." );

	if( item->lookahead_cnt > 0 )
	{
		fprintf( where, "\t\t\t," );
		for( j = 0; j < item->lookahead_cnt; j++ )
			fprintf( where, "\"%s\" ", print_charset( item->lookahead[j]->name ) );
	}

	fprintf( where, " prec:%d\n", item->prod->prec );
}


void print_item_set( uchar* what, ITEM* set, int cnt )
{
	int i;

	if( cnt == 0 )
		return;

	fprintf( stderr, "%s (%d):\n", what, cnt );

	for( i = 0; i < cnt; i++ )
		print_item( stderr, &( set[i] ) );

	fprintf( stderr, "\n" );
}


STATE* state_with_same_kernel( ITEM* partition, int partition_cnt )
{
	ITEM	tmp;
	int i,j,k,l;

	for( i = 0; i < state_cnt; i++ )
		if( state[i].kernel_cnt == partition_cnt )
		{
			l = 0;
			for( j = 0; j < state[i].kernel_cnt; j++ )
				for( k = 0; k < partition_cnt; k++ )
					if( state[i].kernel[j].prod == partition[k].prod &&
							state[i].kernel[j].dot_offset == partition[k].dot_offset )
					{
						l++;

						if( 0 )
						{
							memcpy( &tmp, &(partition[j]), sizeof( ITEM ) );
							memcpy( &(partition[j]), &(partition[k]), sizeof( ITEM ) );
							memcpy( &(partition[j]), &tmp, sizeof( ITEM ) );
						}
					}

			if( l == partition_cnt )
				return &(state[i]);
		}

	return (STATE*)NULL;
}

STATE* undone_state( void )
{
	int i;
	for( i = 0; i < state_cnt; i++ )
	{
		if( !(state[i].done) )
			return &(state[i]);
	}

	return (STATE*)NULL;
}

int lalr1_closure( STATE* s )
{
	int 	done_something	= 0, cnt = 0;
	ITEM	closure_set		[MAXNUM];
	ITEM	tmp_closure_set	[MAXNUM];
	ITEM	partition		[MAXNUM];
	ITEM	sort;
	int		closure_set_cnt = 0;
	int		tmp_closure_set_cnt = 0;
	int		partition_cnt	= 0;
	int		i,j,k;
	int		first			= 1;
	SYM*	ts;
	STATE*	st_tmp;

	fprintf( stderr, "\n\n-------------------------------------------------------------------\n" );
	fprintf( stderr, "CLOSING STATE %d\n", s->id );
	fprintf( stderr, "-------------------------------------------------------------------\n" );

	print_item_set( "Kernel set", s->kernel, s->kernel_cnt );

	/* init */
	for( i = 0; i < MAXNUM; i++ )
		memset( &(closure_set[i]), 0, sizeof( ITEM ) );

	s->done = 1;

	/* perform the closure */
	do
	{
		done_something = 0;

		/*
			very, very important:
			first run must occur on kernel set, but closures are done to closure_set!
		*/
		for( i = 0; i < ( first ? s->kernel_cnt : closure_set_cnt ); i++ )
		{
			if( ( first ? s->kernel[i].dot_offset : closure_set[i].dot_offset )
					< ( first ? s->kernel[i].prod->rhs_cnt : closure_set[i].prod->rhs_cnt ) )
			{
				ts = SYM_RIGHT_OF_DOT( ( first ? s->kernel[i] : closure_set[i] ) );

				if( ts->type == S_NONTERM )
				{
					for( j = 0; j < ts->prod_cnt; j++ )
					{
						for( k = 0; k < closure_set_cnt; k++ )
							if( closure_set[k].prod == ts->prod[j] )
								break;

						if( k == closure_set_cnt )
						{
							closure_set[closure_set_cnt].prod = ts->prod[j];
							closure_set[closure_set_cnt].derived_from = ( first ? &( s->kernel[i] ) : &( closure_set[i] ) );
							
							closure_set_cnt++;
							done_something = 1;
						}

						cnt = closure_set[k].lookahead_cnt;
						if( ( first ? s->kernel[i].dot_offset+1 : closure_set[i].dot_offset+1 )
								< ( first ? s->kernel[i].prod->rhs_cnt : closure_set[i].prod->rhs_cnt ) )
						{
							if( get_rhs_first( &(closure_set[k]),
									( first ? s->kernel[i].prod : closure_set[i].prod ),
										( first ? s->kernel[i].dot_offset+1 : closure_set[i].dot_offset+1 )
											 ) == 0 )
								union_array( closure_set[k].lookahead,
									&(closure_set[k].lookahead_cnt),
										( first ? s->kernel[i].lookahead : closure_set[i].lookahead ),
											( first ? s->kernel[i].lookahead_cnt : closure_set[i].lookahead_cnt ) );
						}
						else
						{
							union_array( closure_set[k].lookahead,
								&(closure_set[k].lookahead_cnt),
									( first ? s->kernel[i].lookahead : closure_set[i].lookahead ),
										( first ? s->kernel[i].lookahead_cnt : closure_set[i].lookahead_cnt ) );
						}

						done_something |= ( cnt < closure_set[k].lookahead_cnt ) ? 1 : 0;
					}
				}
			}
		}
		

		first = 0;
	}
	while( done_something );

	/* copy outgoing transitions from kernel to temporary closure set */
	for( i = 0; i < s->kernel_cnt; i++ )
		if( s->kernel[i].prod->rhs_cnt > s->kernel[i].dot_offset )
			memcpy( &(tmp_closure_set[tmp_closure_set_cnt++]), &(s->kernel[i]), sizeof( ITEM ) );

	/* copy the rest of the closure set into the temporary closure set */
	for( i = 0; i < closure_set_cnt; i++ )
		memcpy( &(tmp_closure_set[tmp_closure_set_cnt++]), &(closure_set[i]), sizeof( ITEM ) );

	/* replace closure set with temporary closure set */
	memcpy( closure_set, tmp_closure_set, MAXNUM * sizeof( ITEM ) );
	closure_set_cnt = tmp_closure_set_cnt;

	/* remove epsilons */
	for( i = 0; i < closure_set_cnt; i++ )
	{
		if( closure_set[i].prod->rhs_cnt == 0 )
		{
			for( j = 0; j < s->epsilon_cnt; j++ )
				if( s->epsilon[j].prod == closure_set[i].prod )
						break;

			if( j == s->epsilon_cnt )
				memcpy( &(s->epsilon[s->epsilon_cnt++]), &(closure_set[i]), sizeof( ITEM ) );				
			else
				/* Merging the lookaheads */
				union_array( s->epsilon[j].lookahead, &( s->epsilon[j].lookahead_cnt ),
					closure_set[i].lookahead, closure_set[i].lookahead_cnt );

			for( j = i, k = i+1; j < closure_set_cnt; j++, k++ )
				memcpy( &(closure_set[j]), &(closure_set[k]), sizeof( ITEM ) );

			closure_set_cnt--;
			i--;
		}
	}

	if( closure_set_cnt == 0 )
		fprintf( stderr, "Closure set is empty\n\n" );
	else
		print_item_set( "Closure set", closure_set, closure_set_cnt );

	if( s->epsilon_cnt == 0 )
		fprintf( stderr, "Epsilon set is empty\n\n" );
	else
		print_item_set( "Epsilon set", s->epsilon, s->epsilon_cnt );

	/* create partitions */
	while( closure_set_cnt > 0 )
	{
		for( i = 0; i < MAXNUM; i++ )
			memset( &(partition[i]), 0, sizeof( ITEM ) );

		st_tmp = (STATE*)NULL;
		ts = (SYM*)NULL;
		partition_cnt = 0;

		for( i = 0; i < closure_set_cnt; i++ )
		{
			if( ts == (SYM*)NULL )
				ts = SYM_RIGHT_OF_DOT( closure_set[i] );

			if( SYM_RIGHT_OF_DOT( closure_set[i] ) == ts )
			{
				memcpy( &(partition[partition_cnt]), &(closure_set[i]), sizeof( ITEM ) );
				partition[partition_cnt++].dot_offset++;

				for( j = i, k = i+1; j < closure_set_cnt; j++, k++ )
					memcpy( &(closure_set[j]), &(closure_set[k]), sizeof( ITEM ) );

				closure_set_cnt--;
				i--;
			}
		}
		
		if( partition_cnt == 0 )
			break;
					
		for( i = 0; i < MAXNUM; i++ )
		{
			do
			{
				done_something = 0;

				for( j = 0; j < partition_cnt - 1; j++ )
				{
					if( partition[j].prod > partition[j+1].prod )
					{
						memcpy( &sort, &(partition[j]), sizeof( ITEM ) );
						memcpy( &(partition[j]), &(partition[j+1]), sizeof( ITEM ) );
						memcpy( &(partition[j+1]), &sort, sizeof( ITEM ) );
						
						done_something++;
					}
				}
			}
			while( done_something );
		}
		
		print_item_set( "Partition", partition, partition_cnt );
		
		/*
			Experimental modification on LALR(1) parse table construction:
			
			If we here have a partition that has a single configuration
			
				y -> x T .
			
			where y is the nonterm, x a possible sequence, and T a terminal,
			then don't create a new state from this, but add a SHIFT_REDUCE,
			reduce by the production, shift to no state!
		*/
		for( i = 0; i < partition_cnt; i++ )
		{
			if( partition[i].prod->rhs_cnt == partition[i].dot_offset )
				/* && partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->type == S_TERM  ) */
			{
				if( i > 0 && memcmp( &( partition[i] ), &( partition[i - 1] ), sizeof( ITEM ) ) != 0 )
					break;
			}
			else
				break;
		}
		
		if( i == partition_cnt && !( s->closed ) && 1 )
		{
			fprintf( stderr, "\nAdding SHIFT_REDUCE... on partititon\n" );
			print_item_set( "Partition:", partition, partition_cnt );
			
			if( partition[0].prod->rhs[ partition[0].dot_offset - 1 ]->type == S_TERM )
			{
				s->acttab[s->acttab_cnt].sym = partition[0].prod->rhs[ partition[0].dot_offset - 1 ]->id;
				s->acttab[s->acttab_cnt].act = SHIFT_REDUCE;
				s->acttab[s->acttab_cnt].idx = ( partition[0].prod - prod );

				/* Remember the item, reset the dot to its previous position! */
				#if WITH_ITEM_IN_TABCOL
				memcpy( &( s->acttab[s->acttab_cnt].item ), &( partition[0] ), sizeof( ITEM ) );						
				s->acttab[s->acttab_cnt].item.dot_offset--;
				#endif
	
				s->acttab_cnt++;
			}
			else
			{
				s->gototab[s->gototab_cnt].sym = partition[0].prod->rhs[ partition[0].dot_offset - 1 ]->id;
				s->gototab[s->gototab_cnt].act = SHIFT_REDUCE;
				s->gototab[s->gototab_cnt].idx = ( partition[0].prod - prod );

				s->gototab_cnt++;
			}
		}
		else
		{
			/* If the above fails, create a new state or check for existing one */
			
			if( !(st_tmp = state_with_same_kernel( partition, partition_cnt )) )
			{
				for( i = 0; i < partition_cnt; i++ )
					memcpy( &(state[state_cnt].kernel[i]), &(partition[i]), sizeof( ITEM ) );
	
				state[state_cnt].kernel_cnt = partition_cnt;
				state[state_cnt].id = state_cnt;
				st_tmp = &(state[state_cnt++]);
	
				fprintf( stderr, "Creating new state %d\n", st_tmp->id );
			}
			else
				fprintf( stderr, "Using existing state %d\n", st_tmp->id );

			if( !( s->closed ) )
			{
				for( i = 0; i < partition_cnt; i++ )
				{
					if( partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->type == S_TERM )
					{
						for( j = 0; j < s->acttab_cnt; j++ )
							if( s->acttab[j].sym == partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->id )
								break;
	
						if( j == s->acttab_cnt )
						{
							fprintf( stderr, "adding shift on %s\n", partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->name );
							
							s->acttab[s->acttab_cnt].sym = partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->id;		
							s->acttab[s->acttab_cnt].act = SHIFT;
							s->acttab[s->acttab_cnt].idx = st_tmp->id;
							
							/* Remember the item, reset the dot to its previous position! */
							#if WITH_ITEM_IN_TABCOL
							memcpy( &( s->acttab[s->acttab_cnt].item ), &( partition[i] ), sizeof( ITEM ) );						
							s->acttab[s->acttab_cnt].item.dot_offset--;
							#endif
	
							s->acttab_cnt++;
						}
					}
					else
					{
						for( j = 0; j < s->gototab_cnt; j++ )
							if( s->gototab[j].sym == partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->id )
								break;
								
						if( j == s->gototab_cnt )
						{
							fprintf( stderr, "adding goto on %s\n", partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->name );
							
							s->gototab[s->gototab_cnt].sym = partition[i].prod->rhs[ partition[i].dot_offset - 1 ]->id;
							s->gototab[s->gototab_cnt].act = SHIFT;
							s->gototab[s->gototab_cnt].idx = st_tmp->id;
							
							s->gototab_cnt++;
						}
					}
				}
			}
			

			cnt = 0;
			for( i = 0; i < st_tmp->kernel_cnt; i++ )
				cnt += union_array( st_tmp->kernel[i].lookahead, &(st_tmp->kernel[i].lookahead_cnt),
					partition[i].lookahead, partition[i].lookahead_cnt );
		
			if( cnt > 0 )
				st_tmp->done = 0;
		}
	}

	s->closed = 1;

	return 0;
}

void reduce_item( STATE* s, ITEM* item )
{
	int i, j, k, noresolve;

	for( i = 0; i < item->lookahead_cnt; i++ )
	{
		noresolve = 0;
		for( j = 0; j < s->acttab_cnt; j++ )
		{
			if( s->acttab[j].sym == item->lookahead[i]->id )
				break;
		}

		if( j == s->acttab_cnt )
		{
			s->acttab[s->acttab_cnt].sym = item->lookahead[i]->id;
			s->acttab[s->acttab_cnt].act = REDUCE;
			s->acttab[s->acttab_cnt].idx = ( item->prod - prod );

			#if WITH_ITEM_IN_TABCOL
			memcpy( &( s->acttab[s->acttab_cnt].item ), item, sizeof( ITEM ) );						
			#endif

			s->acttab_cnt++;
		}
		else
		{			
			if( s->acttab[j].act & SHIFT )
			{
				#if WITH_ITEM_IN_TABCOL
				if( distinguish_lexemes && s->acttab[j].item.prod->lhs->lexem )
					continue;
				#endif
				
				if( ( HIDE_WHITESPACE_ERRORS && !( item->prod->lhs->whitespace ) ) || !HIDE_WHITESPACE_ERRORS )
				{
					fprintf( stderr, "State %d: Shift-reduce conflict on \"%s\" with %d (table) and %d (current)\n",
						s->id, item->lookahead[i]->name, s->acttab[j].idx, item->prod - prod );
				
					#if WITH_ITEM_IN_TABCOL
					print_item( stderr, &( s->acttab[j].item ) );
					print_item( stderr, item );
					#endif
			
					fprintf( stderr, "\tprec %d %d\n", item->lookahead[i]->prec, item->prod->prec );
				}

				if( item->lookahead[i]->prec < item->prod->prec
					|| ( item->lookahead[i]->prec == item->prod->prec
					&& item->lookahead[i]->assoc == ASSOC_LEFT ) )
				{
					s->acttab[j].act = REDUCE;
					s->acttab[j].idx = ( item->prod - prod );
				}
				else if( ( item->lookahead[i]->prec == item->prod->prec
					&& item->lookahead[i]->assoc == ASSOC_NOASSOC ) )
				{
					fprintf( stderr, "\tNonassociativity error!\n" );
					if( j < s->acttab_cnt - 1 )
						memcpy( &( s->acttab[j] ), &( s->acttab[ s->acttab_cnt - 1 ] ),
								sizeof( TABCOL ) );

					s->acttab_cnt--;
					
					noresolve = 1;
				}
				
				/*
				if( item->lookahead[i]->prec < item->prod->prec
						|| ( item->lookahead[i]->prec == item->prod->prec
							&& item->lookahead[i]->assoc != ASSOC_RIGHT ) )
				{
					s->acttab[j].act = REDUCE;
					s->acttab[j].idx = ( item->prod - prod );
				}
				*/
				
				if( ( ( HIDE_WHITESPACE_ERRORS && !( item->prod->lhs->whitespace ) ) || !HIDE_WHITESPACE_ERRORS ) && !noresolve )
					fprintf( stderr, "\tresolved to %d %d\n", s->acttab[j].act,  s->acttab[j].act );
			}
			else if( s->acttab[j].act == REDUCE )
			{
				fprintf( stderr, "State %d: Reduce-reduce conflict on \"%s\" with %d (table) and %d (current)\n",
					s->id, item->lookahead[i]->name, s->acttab[j].idx, item->prod - prod );

				#if WITH_ITEM_IN_TABCOL
				print_item( stderr, &( s->acttab[j].item ) );
				print_item( stderr, item );
				#endif
				
				if( s->acttab[j].idx > item->prod - prod )
				{
					s->acttab[j].act = REDUCE;
					s->acttab[j].idx = ( item->prod - prod );
				}

				fprintf( stderr, "\tresolved to %d\n", s->acttab[j].idx );
			}
		}
	}
}

void doclosure( void )
{
	STATE*	s;
	int i, j, found;

	fprintf( stderr, "\n\n###################################################################\n" );

	fprintf( stderr, "-= LALR(1) closure =-\n" );
	
	/* Set production's precedence level to the one of the rightmost terminal! */
	for( i = 0; i < prod_cnt; i++ )
	{
		if( prod[i].prec > 0 )
			continue;

		/* First try to find leftmost terminal */
		found = 0;
		for( j = prod[i].rhs_cnt - 1; j >= 0; j-- )
		{
			if( prod[i].rhs[j]->lexem )
			{
				prod[i].prec = prod[i].rhs[j]->prec;
				found = 1;
				break;
			}
		}
		
		/* If there is no terminal, use rightmost non-terminal with a precedence */
		if( !found )
		{
			for( j = prod[i].rhs_cnt - 1; j >= 0; j-- )
			{
				if( prod[i].rhs[j]->prec > prod[i].prec )
				{
					prod[i].prec = prod[i].rhs[j]->prec;
					break;
				}
			}
		}
	}
		
	/* If nonterminal symbol has a precedence, attach it to all its productions! */
	for( i = 0; i < sym_cnt; i++ )
		if( sym[i].type == S_NONTERM && sym[i].prec > 0 && sym[i].generated == 0 )
		{
			for( j = 0; j < sym[i].prod_cnt; j++ )
				sym[i].prod[j]->prec = sym[i].prec;
		}
	
	/* Print grammar to check rewritten precedences */
	printgrm();
	
	/* Create first kernel seed! */
	s = &(state[state_cnt++]);

	/* s->kernel[0].prod = &(prod[0]); */
	s->kernel[0].prod = goal->prod[0];
	s->kernel[0].lookahead[0] = sym;
	s->kernel[0].derived_from = (ITEM*)NULL;

	s->kernel[0].lookahead_cnt = 1;
	s->id = 0;
	s->kernel_cnt++;

	while( ( s = undone_state() ) != (STATE*)NULL )
		lalr1_closure( s );

	fprintf( stderr, "\n\n" );
	fprintf( stderr, "-= LALR(1) states =-\n" );
	for( i = 0; i < state_cnt; i++ )
	{
		fprintf( stderr, "### State %d ###\n", i );
		print_item_set( "Kernel", state[i].kernel, state[i].kernel_cnt );
		print_item_set( "Epsilon", state[i].epsilon, state[i].epsilon_cnt );
	}
	fprintf( stderr, "\n" );
	
	/* Make the reductions */
	for( i = 0; i < state_cnt; i++ )
	{
		/* fprintf( stderr, "reduce state %d\n", i ); */
		
		for( j = 0; j < state[i].kernel_cnt; j++ )
			if( state[i].kernel[j].dot_offset == state[i].kernel[j].prod->rhs_cnt )
				reduce_item( &( state[i] ), &( state[i].kernel[j] ) );

		for( j = 0; j < state[i].epsilon_cnt; j++ )
			reduce_item( &( state[i] ), &( state[i].epsilon[j] ) );
	}
	
	fprintf( stderr, "-= LALR(1) parse tables =-\n" );
	
	fprintf( stderr, "  Action-Table:\n" );
	for( i = 0; i < state_cnt; i++ )
	{
		fprintf( stderr, "    State %d: ", i );
		for( j = 0; j < state[i].acttab_cnt; j++ )
			fprintf( stderr, "%d,%d,%d ", state[i].acttab[j].sym, state[i].acttab[j].act, state[i].acttab[j].idx );
			
		fprintf( stderr, "\n" );
	}

	fprintf( stderr, "  Goto-Table:\n" );
	for( i = 0; i < state_cnt; i++ )
	{
		fprintf( stderr, "    State %d: ", i );
		for( j = 0; j < state[i].gototab_cnt; j++ )
			fprintf( stderr, "%d,%d,%d ", state[i].gototab[j].sym, state[i].gototab[j].act, state[i].gototab[j].idx );
			
		fprintf( stderr, "\n" );
	}
	
	fprintf( stderr, "  Pop-Table:\n" );
	for( i = 0; i < prod_cnt; i++ )
		fprintf( stderr, "    Production %d: %d, %d {%s}\n", i, prod[i].rhs_cnt, prod[i].lhs->id, prod[i].code );
		
	fprintf( stderr, "###################################################################\n" );
}


void chkgrm( void )
{
	int i, j;
	
	for( i = 0; i < prod_cnt; i++ )
	{
		if( prod[i].rhs_cnt == 1 && prod[i].rhs[0] == prod[i].lhs )
			fprintf( stderr, "WARNING: Circular definition of non-terminal '%s' in rule %d\n\n", prod[i].lhs->name, i );
		else if( prod[i].lhs->nullable )
			for( j = 0; j < prod[i].rhs_cnt; j++ )
			{
				if( !( prod[i].rhs[j]->nullable ) )
					break;
				else if( prod[i].rhs[j] == prod[i].lhs )
				{
					fprintf( stderr, "WARNING: Empty recursion in non-terminal '%s' in rule %d\n\n", prod[i].lhs->name, i );
					break;
				}
			}
	}
}


void print_c_code( void )
{
	int i,j,maxdepth,first;
	int char_map[MAXSTR];

	#define P( txt ) fprintf( stdout, "%s\n", txt );
	#define NL fprintf( stdout, "\n" );
		
	if( head_code )
		P( head_code )
	else
	{
		P( "#include <stdlib.h>" )
		P( "#include <stdio.h>" )
		P( "#include <string.h>" )
	}
	NL
	fprintf( stdout, "#define ERROR %d\n", prod_cnt );
	fprintf( stdout, "#define EOFTOKEN %d\n", ( get_symbol( "", S_TERM, 0, SYM_ATT_NONE ) - sym ) + 1 );
	fprintf( stdout, "#define ERRTOKEN %d\n", ( get_symbol( ERROR_TOKEN_NAME, S_TERM, 0, SYM_ATT_ERROR ) - sym ) + 1 );
	P( "#define MAXSTACK 255" )
	NL
	NL
	
	/* Action-Table */
	maxdepth = 0;
	for( i = 0; i < state_cnt; i++ )
	{
		if( maxdepth < state[i].acttab_cnt )
			maxdepth = state[i].acttab_cnt;
	}
	
	fprintf( stdout, "int act_tab[%d][%d + 1] =\n{\n", state_cnt, maxdepth * 3 );
	for( i = 0; i < state_cnt; i++ )
	{
		fprintf( stdout, "\t/* State %d */ { %d%c ", i, state[i].acttab_cnt * 3, state[i].acttab_cnt ? ',' : ' ' );
		for( j = 0; j < state[i].acttab_cnt; j++ )
			fprintf( stdout, "%d,%d,%d%c ", state[i].acttab[j].sym, state[i].acttab[j].act, state[i].acttab[j].idx,
				(j+1 < state[i].acttab_cnt) ? ',' : ' ' );

		fprintf( stdout, " }%c\n", ( i+1 < state_cnt ) ? ',' : ' ' );
	}
	fprintf( stdout, "};\n\n" );
	
	/* Goto-Table */
	maxdepth = 0;
	for( i = 0; i < state_cnt; i++ )
	{
		if( maxdepth < state[i].gototab_cnt )
			maxdepth = state[i].gototab_cnt;
	}
	
	fprintf( stdout, "int goto_tab[%d][%d + 1] =\n{\n", state_cnt, maxdepth * 3 );
	for( i = 0; i < state_cnt; i++ )
	{
		fprintf( stdout, "\t/* State %d */ { %d%c ", i, state[i].gototab_cnt * 3, state[i].gototab_cnt ? ',' : ' ' );
		for( j = 0; j < state[i].gototab_cnt; j++ )
			fprintf( stdout, "%d,%d,%d%c ", state[i].gototab[j].sym, state[i].gototab[j].act, state[i].gototab[j].idx,
				(j+1 < state[i].gototab_cnt) ? ',' : ' ' );
			
		fprintf( stdout, " }%c\n", ( i+1 < state_cnt ) ? ',' : ' ' );
	}
	fprintf( stdout, "};\n\n" );
	
	/* Pop-Table */
	fprintf( stdout, "int pop_tab[%d][2] =\n{\n", prod_cnt );
	for( i = 0; i < prod_cnt; i++ )
		fprintf( stdout, "\t/* Production %d */ { %d, %d }%c\n", i, prod[i].rhs_cnt, prod[i].lhs->id,
			( i+1 < prod_cnt ) ? ',' : ' ' );
	fprintf( stdout, "};\n\n" );
	
	/* Character symbol map */	
	for( i = 0; i < MAXSTR; i++ )
		char_map[i] = -1;
		
	for( i = 0; i < sym_cnt; i++ )
	{
		if( sym[i].type != S_TERM || sym[i].att & SYM_ATT_KEYWORD || sym[i].att & SYM_ATT_ERROR )
			continue;
			
		if( strlen( sym[i].name ) > 0 )
			for( j = 0; j < strlen( sym[i].name ); j++ )
				char_map[ sym[i].name[j] ] = sym[i].id;
		else /* End-of-string */
			char_map[0] = sym[i].id;
	}
	
	fprintf( stdout, "int char_map[256] =\n{\n\t" );
	for( i = 0; i < MAXSTR; i++ )
	{
		fprintf( stdout, "%d%c ", char_map[i], ( i+1 == MAXSTR ) ? ' ' : ',' );
		
		if( ( i > 0 && i % 20 == 0 ) || i+1 == MAXSTR )
		{
			fprintf( stdout, "\n" );
			if( i+1 < MAXSTR )
				fprintf( stdout, "\t" );
		}
	}
	fprintf( stdout, "};\n\n" );
	
	/* Functions */
	NL
	NL
	P( "int _get_act_entry( int state, int sym, int* act, int* idx )" )
	P( "{" )
	P( "	int i;" )
		
	P( "	for( i = 1; i < act_tab[state][0]; i += 3 )" )
	P( "		if( act_tab[state][i] == sym )" )
	P( "		{" )
	P( "			*act = act_tab[state][i+1];" )
	P( "			*idx = act_tab[state][i+2];" )
	P( "			return 1;" )
	P( "		}" )
	NL
	P( "	return 0;" )
	P( "}" )

	NL
	P( "int _get_goto_entry( int state, int sym, int* act, int* idx )" )
	P( "{" )
	P( "	int i;" )
		
	P( "	for( i = 1; i < goto_tab[state][0]; i += 3 )" )
	P( "		if( goto_tab[state][i] == sym )" )
	P( "		{" )
	P( "			*act = goto_tab[state][i+1];" )
	P( "			*idx = goto_tab[state][i+2];" )
	P( "			return 1;" )
	P( "		}" )
	NL
	P( "	return 0;" )
	P( "}" )

	NL
	P( "int _get_keyword( char* s, int* length )" )
	P( "{" )
	P( "	int match = -1;" )
	P( "	*length = 0;" )
	NL
	
	first = 1;
	for( i = 0; i < sym_cnt; i++ )
	{
		if( sym[i].type == S_TERM && sym[i].att & SYM_ATT_KEYWORD )
		{
			fprintf( stdout, "\t%sif( ", first ? "" : "else " );
			fprintf( stdout, "strncmp( \"%s\", s, %d * sizeof( char ) ) == 0",
				sym[i].name, strlen( sym[i].name ) );
			fprintf( stdout, " )\n\t{\n\t\t*length = %d;\n\t\tmatch = %d;\n\t}\n",
				strlen( sym[i].name ), sym[i].id );
			first = 0;
		}
	}
	NL
	P( "	return match;" )
	P( "}" )
	
	NL
	P( "int _get_symbol( char* s, int* length )" )
	P( "{" )
	P( "	*length = 1;" )
	P( "	return char_map[ *s ];" )
	P( "}" )
	NL
	NL
	
	P( "#ifndef VAL" )
	P( "#define VAL int" )
	P( "#endif" )
	P( "#ifndef SET_CURRENT_CHAR" )
	P( "#define SET_CURRENT_CHAR" )
	P( "#endif" )
	NL
	P( "int parse( char* str )" )
	P( "{" )
	P( "	int	stack[MAXSTACK], sym, kw_sym, act, idx, tos = 0, ok, len, kw_len, length, i;" )
	P( "	VAL vstack[MAXSTACK], ret;" )
	P( "	char* pstr = str;" )
	NL
	P( "	memset( stack, 0, MAXSTACK * sizeof( int ) );" )
	P( "	memset( vstack, 0, MAXSTACK * sizeof( VAL ) );" )
	NL
	P( "	sym = _get_symbol( pstr, &len );" )
	P( "	kw_sym = _get_keyword( pstr, &kw_len );" )
	NL
	P( "	do" )
	P( "	{" )
	P( "		ok = 0;" )
	NL
	P( "		if( kw_sym > -1 )" )
	P( "		{" )
	P( "			ok = _get_act_entry( stack[ tos ], kw_sym, &act, &idx );" )
	P( "			length = kw_len;" )
	P( "		}" )
	NL
	P( "		if( !ok )" )
	P( "		{" )
	P( "			ok = _get_act_entry( stack[ tos ], sym, &act, &idx );" )
	P( "			length = len;" )
	P( "		}" )
	NL
	P( "		#if PDEBUG" );
	P( "		printf( \"state %d, ok = %d, sym = %d (att = >%c<) kw_sym = %d (att = >%.*s<) act = %d idx = %d\\n\", stack[ tos ], ok, sym, *pstr, kw_sym, kw_len, pstr, act, idx );" )
	P( "		for( i = 0; i <= tos; i++ )" )
	P( "			printf( \"%p \", vstack[i] );" )
	P( "		printf( \"\\n\" );" )
	P( "		#endif" );
	NL
	P( "		if( !ok ) /* Error */" )
	P( "		{" )	
	P( "			fprintf( stdout, \"Parse error at pstr = >%s<, att = >%c<, sym = %d, stack[ tos ] = %d\\n\"," )
	P( "				pstr, *pstr, sym, stack[ tos ] );" )
	NL
#if 0
	P( "			while( tos > 0 )" )
	P( "			{" )
	P( "				tos--;" )
	NL

	P( "				if( ( act = _get_act_entry( stack[ tos ], ERRTOKEN, act, &err ) ) != ERROR )" )
	P( "				{" )
	P( "					tos++;" )
	P( "					stack[ tos ] = act;" )
	/* P( "					vstack[ tos ]SET_CURRENT_CHAR = 0;" ) */
	
	P( "					while( 1 )" )
	P( "					{" );
	P( "						act = ERROR;" )
	NL
	P( "						if( kw_sym > -1 )" )
	P( "						{" )
	P( "							act = _get_act_entry( stack[ tos ], kw_sym );" )
	P( "							length = kw_len;" )
	P( "						}" )
	NL
	P( "						if( act == ERROR )" )
	P( "						{" )
	P( "							act = _get_act_entry( stack[ tos ], sym );" )
	P( "							length = len;" )
	P( "						}" )
	NL
	P( "						if( act != ERROR || ( act == ERROR && sym == EOFTOKEN ) )" )
	P( "							break;" )
	NL
	P( "						pstr += length;" )
	P( "						sym = _get_symbol( pstr, &len );" )
	P( "						kw_sym = _get_keyword( pstr, &kw_len );" )
	P( "					}" )
	NL
	P( "					break;" )
	P( "				}" )
	P( "			}" )
	NL
	P( "			if( act != ERROR )" )
	P( "				continue;" )
#endif
	NL
	P( "			fprintf( stdout, \"Aborting error recovery: End of input reached.\\n\" );" )
	P( "			return -1;" )
	P( "		}" )
	NL
	P( "		if( act & 2 ) /* Real Shift */" )
	P( "		{" )
	P( "			#if PDEBUG" );
	P( "			printf( \"Shifting to state %d\\n\", idx );" );
	P( "			#endif" );
	NL
	P( "			tos++;" )
	NL
	P( "			if( act == 3 )" )
	P( "				stack[ tos ] = stack[ tos - 1 ]; /* Push Junk in SHIFT_REDUCE operation! */" )
	P( "			else" )
	P( "				stack[ tos ] = idx;" )
	NL
	P( "			vstack[ tos ]SET_CURRENT_CHAR = *pstr;" )
	NL
	P( "			pstr += length;" )
	P( "			sym = _get_symbol( pstr, &len );" )
	P( "			kw_sym = _get_keyword( pstr, &kw_len );" )
	P( "		}" )
	NL
	P( "		while( act & 1 ) /* Reduce */" )
	P( "		{" )
	
	/*
		Whoa, here I have to watch out:
		Use the rightmost value of the production, not the leftmost!!!
		
		Now it works, phew!
	*/
	
	P( "			memcpy( &ret, &( vstack[ tos - pop_tab[ idx ][0] + 1 ] ), sizeof( VAL ) );" )
	NL
	P( "			#if PDEBUG" );
	P( "			printf( \"Reducing production %d\\n\", idx );" );
	P( "			#endif" );
	P( "			switch( idx )" )
	P( "			{" )
	
	/* Action-Switch */
	for( i = 0; i < prod_cnt; i++ )
	{
		if( prod[i].code )	
		{
			fprintf( stdout, "\t\t\t\tcase %d:\n", i );
			fprintf( stdout, "\t\t\t\t\t{ " );
		
			for( j = 0; j < strlen( prod[i].code ); j++ )
			{
				if( prod[i].code[j] == '#' )
				{
					j++;
					if( prod[i].code[j] == '#' )
						fprintf( stdout, "(ret)" );
					else
						fprintf( stdout, "(vstack[ tos - %d ])",
							prod[i].rhs_cnt - ( prod[i].code[j] - '0' ) );
				}
				else
					fprintf( stdout, "%c", prod[i].code[j] );
			}
				
			fprintf( stdout, " }\n" );
			fprintf( stdout, "\t\t\t\t\tbreak;\n\n" );
		}
	}
	
	P( "			}" )
	NL
	P( "			for( i = tos; i > tos - pop_tab[ idx ][0]; i-- )" )
	P( "				memset( &( vstack[ i ] ), 0, sizeof( VAL ) );" )
	NL
	P( "			tos -= pop_tab[ idx ][0];" )
	P( "			tos++;" )
	P( "			_get_goto_entry( stack[ tos - 1 ], pop_tab[ idx ][1], &act, &idx );" )
	P( "			stack[ tos ] = idx;" )
	P( "			memcpy( &( vstack[ tos ] ), &( ret ), sizeof( VAL ) );" )
	NL
	printf( "			if( act & 1 && idx == %d )\n",  (int)( goal->prod[0] - prod ) );
	P( "				break;" )
	P( "		}" )
	P( "	}" )
	printf( "	while( !( act & 1 && idx == %d ) );\n", (int)( goal->prod[0] - prod  ) );
	NL
	P( "	return 0;" )
	P( "}" )
	NL
	NL
	
	if( body_code )
		P( body_code )
	else
	{	
		P( "int main( int argc, char** argv )" )
		P( "{" )
		P( "	char str[255];" )
		NL
		P( "	while( 1 )" )
		P( "	{" )
		P( "		fprintf( stdout, \">>\" );" )
		P( "		gets( str );" )
		NL
		P( "		if( *str == '\\0' )" )
		P( "			break;" )
		NL
		P( "		parse( str );" )
		P( "	}" )
		P( "	return 0;" )
		P( "}" )
	}

	NL
}

void all_init( void )
{
	int i, j;

	for( i = 0; i < MAXNUM; i++ )
	{
		memset( &(sym[i]), 0, sizeof( SYM ) );
		sym[i].type = S_UNDEF;
		memset( &(prod[i]), 0, sizeof( PROD ) );
		memset( &(state[i]), 0, sizeof( STATE ) );
		for( j = 0; j < MAXPROD; j++ )
		{
			memset( &(state[i].kernel[j]), 0, sizeof( ITEM ) );
			memset( &(state[i].epsilon[j]), 0, sizeof( ITEM ) );
		}
	}

	/* End-of-String symbol! */
	get_symbol( "", S_TERM, 1, SYM_ATT_NONE );
	/* Error symbol */
	get_symbol( ERROR_TOKEN_NAME, S_TERM, 1, SYM_ATT_ERROR );
}

int main( int argc, char** argv )
{
	uchar* src = (uchar*)NULL,
		* sptr;
	FILE* f = (FILE*)NULL;
	
	fprintf( stderr, "min_lalr1 - Minimalist LALR(1) Parser Generator "
			"v0.14.1 [%s]\n", __DATE__ );
	fprintf( stderr, "Copyright (C) 2007-2012 by "
			"Phorward Software Technologies, Jan Max Meyer\n" );
	fprintf( stderr, "http://www.phorward-software.com   "
				"contact<at>phorward<dash>software<dot>com\n" );
	fprintf( stderr, "All rights reserved. See LICENSE for more"
						"information.\n\n" );				
	
	if( argc < 2 )
	{
		fprintf( stderr, "Generates a complete C source parser"
			       " from a BNF-grammar definition.\n" );
		fprintf( stderr, "Debug is written to stderr, output source "
			       "to stdout.\n\n" );
		
		fprintf( stderr, "usage: %s grammar.syn [ >output.c ]\n\n", argv[0] );
		
		fprintf( stderr, "In grammars, use the configuration switches:\n\n"
				 "  ~ sym [ sym ... ]     for whitespace definitions\n"
				 "  $ sym [ sym ... ]     for lexem nonterminal definitions\n"
				 "  ! dl                  for \"distinguish lexemes*\" feature"
			   );
						 
		fprintf( stderr, "\n\nExample:"
						"    ~ ' ' ;\n"

						"            z:   e	{ printf( \"=%%d\\n\", #1 ); };\n"
						"            e:   e '+' t { ## = #1 + #3; } | t;\n"
						"            t:   t '*' f { ## = #1 * #3; } | f;\n"
						"            f:   int | '(' e ')' { ## = #2; } ;\n"
						"            int: '0-9' { ## = #1 - '0'; }\n"
						"         	    | int '0-9' { ## = #1 * 10 + "
							"( #2 - '0' ); } ;\n"
				);
										 
		return 1;
	}
	else
	{
		f = fopen( argv[1], "rb" );
		if( !f )
		{
			fprintf( stderr, "Error: File %s not found.\n", argv[1] );
			return 1;
		}
		
		fseek( f, 0L, SEEK_END );
	
		src = (uchar*)malloc( ( ftell( f ) + 1 ) * sizeof( uchar ) );
		fseek( f, 0L, SEEK_SET );
		
		sptr = src;
		while( !feof( f ) )
		{
			*sptr = fgetc( f );
			sptr++;
		}
		*(sptr-1) = '\0';
		
		all_init();

		p_def( src );
		free( src );
			
		/* Perform grammar revision! */
		rewrite_grammar();
		unique_charsets();
	
		dofirst();
		chkgrm();
		
		doclosure();
		
		print_c_code();
		
		/* Stats */
		fprintf( stderr, "States %d, Symbols %d, Productions %d\n",
				state_cnt, sym_cnt, prod_cnt );
	}

	return 0;
}

