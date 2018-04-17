/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2018 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	debug.c
Author:	Jan Max Meyer
Usage:	Debug and trace functionalities
----------------------------------------------------------------------------- */

#include "unicc.h"

extern	BOOLEAN		first_progress;

/** Prints a single symbol.

//stream// is the stream where to dump the grammar to. If this is
(FILE*)NULL, output is written to stderr.
//sym// is the symbol pointer to be printed.
*/
void print_symbol( FILE* stream, SYMBOL* sym )
{
	if( !stream )
		stream = stderr;

	if( sym->type == SYM_CCL_TERMINAL )
		fprintf( stream, "'%s'", p_ccl_to_str( sym->ccl, TRUE ) );
	else if( sym->type == SYM_REGEX_TERMINAL && sym->keyword )
		fprintf( stream, "\"%s\"", sym->name );
	else if( sym->type == SYM_REGEX_TERMINAL && !( sym->keyword ) )
		fprintf( stream, "@%s", sym->name );
	else if( sym->type == SYM_SYSTEM_TERMINAL )
		fprintf( stream, "%s", sym->name );
	else
		fprintf( stream, "%s", sym->name );
}

/** Dumps the analyzed grammar and all its symbols to a desired file or stream
in an ASCII-based view.

//stream// is the stream where to dump the grammar to. If this is
(FILE*)NULL, output is written to stderr.
//parser// is the parser information structure pointer.
*/
void dump_grammar( FILE* stream, PARSER* parser )
{
	plistel*	e;
	plistel*	f;

	SYMBOL*		s		= (SYMBOL*)NULL;
	SYMBOL*		sym		= (SYMBOL*)NULL;
	PROD*		p		= (PROD*)NULL;

	if( !stream )
		stream = stderr;

	if( first_progress )
		fprintf( stream, "\n" );

	fprintf( stream, "\n%s%sGRAMMAR\n\n",
		( parser->p_name ? parser->p_name : "" ),
			( parser->p_name ? ": " : "" ) );

	plist_for( parser->symbols, e )
	{
		s = (SYMBOL*)plist_access( e );

		if( s->type == SYM_NON_TERMINAL )
		{
			fprintf( stream, "    " );
			print_symbol( stream, s );
			fprintf( stream, " " );

			/* Printing the FIRST-set */
			fprintf( stream, "[ " );

			plist_for( s->first, f )
			{
				sym = (SYMBOL*)plist_access( f );

				print_symbol( stream, sym );
				fprintf( stream, " " );
			}

			if( s->type == SYM_NON_TERMINAL )
			{
				fprintf( stream, "] lexem:%d prec:%d assoc:%c v:%s\n", s->lexem,
					s->prec, ( ( s->assoc == ASSOC_LEFT ) ? 'L' :
					( ( s->assoc == ASSOC_RIGHT ) ? 'R' : 'N' ) ),
						( s->vtype ) ? s->vtype->int_name : "(null)" );

				/* Printing the productions */
				plist_for( s->productions, f )
				{
					p = (PROD*)plist_access( f );

					fprintf( stream, "      (%d) -> ", p->id );
					dump_production( stream, p, FALSE, FALSE );
				}
			}
			else
			{
				fprintf( stream, "] prec:%d assoc:%c\n",
					s->prec, ( ( s->assoc == ASSOC_LEFT ) ? 'L' :
					( ( s->assoc == ASSOC_RIGHT ) ? 'R' : 'N' ) ) );
			}

			fprintf( stream, "\n" );
		}
	}

	first_progress = FALSE;
}

/** Dumps the analyzed grammar symbols.

//stream// is the stream where to dump the grammar to. If this is (FILE*)NULL,
output is written to stderr.
//parser// is the parser information structure pointer.
*/
void dump_symbols( FILE* stream, PARSER* parser )
{
	plistel*	e;
	SYMBOL*		s		= (SYMBOL*)NULL;

	if( !stream )
		stream = stderr;

	if( first_progress )
		fprintf( stream, "\n" );

	fprintf( stream, "\n%s%sSYMBOLS\n\n",
		( parser->p_name ? parser->p_name : "" ),
			( parser->p_name ? ": " : "" ) );

	plist_for( parser->symbols, e )
	{
		s = (SYMBOL*)plist_access( e );

		fprintf( stream, "    " );
		fprintf( stream, "%c%d: ",
			( IS_TERMINAL( s ) ? 'T' : 'N' ), s->id );

		print_symbol( stream, s );

		fprintf( stream, " [" );

		switch( s->type )
		{
			case SYM_NON_TERMINAL:
				fprintf( stream, "non-terminal" );
				break;

			case SYM_CCL_TERMINAL:
				fprintf( stream, "terminal: character class" );
				break;

			case SYM_REGEX_TERMINAL:
				if( s->keyword )
					fprintf( stream, "terminal: string" );
				else
					fprintf( stream, "terminal: regular expression" );

				if( s->greedy )
					fprintf( stream, "(greedy)" );
				else
					fprintf( stream, "(non-greedy)" );
				break;

			case SYM_SYSTEM_TERMINAL:
				fprintf( stream, "system terminal" );
				break;

			default:
				fprintf( stream, "undefined" );
				break;
		}

		fprintf( stream, "]" );

		if( s->vtype )
			fprintf( stream, " <%s>", s->vtype->int_name );

		fprintf( stream, "\n" );
	}

	fprintf( stream, "\n" );

	first_progress = FALSE;
}

/** Dumps an item set.

//stream// is the stream where to dump the grammar to. If this is (FILE*)NULL,
output is written to stderr.
//title// is the title of the item set.
//list// is the item set linked-list itself.
*/
void dump_item_set( FILE* stream, char* title, LIST* list )
{
	ITEM*		it		= (ITEM*)NULL;
	LIST*		i		= (LIST*)NULL;
	plistel*	e;
	SYMBOL*		sym		= (SYMBOL*)NULL;
	int			cnt		= 0;

	if( !stream )
		stream = stderr;

	if( list != (LIST*)NULL )
	{
		/* if( first_progress )
			fprintf( stream, "\n\n" ); */

		if( title )
			fprintf( stream, "\n%s\n", title );

		for( i = list; i; i = i->next )
		{
			it = i->pptr;


			fprintf( stream, "    (%d) %s -> ",
				it->prod->id, it->prod->lhs->name );

			cnt = 0;

			plist_for( it->prod->rhs, e )
			{
				sym = (SYMBOL*)plist_access( e );

				if( cnt == it->dot_offset )
					fprintf( stream, "." );

				print_symbol( stream, sym );
				fprintf( stream, " " );

				cnt++;
			}

			if( cnt == it->dot_offset )
			{
				fprintf( stream, "." );
				fprintf( stream, "      { " );
				plist_for( it->lookahead, e )
				{
					sym = (SYMBOL*)plist_access( e );

					print_symbol( stream, sym );
					fprintf( stream, " " );
				}
				fprintf( stream, "}" );
			}

			fprintf( stream, "\n" );
		}
	}

	first_progress = FALSE;
}

/** Dumps the LALR(1) states.

//stream// is the stream where to dump the grammar to. If this is (FILE*)NULL,
output is written to stdout.
//parser// is the parser information structure.
*/
void dump_lalr_states( FILE* stream, PARSER* parser )
{
	STATE*	st;

	if( !stream )
		stream = stderr;

	if( first_progress )
		fprintf( stream, "\n\n" );
	first_progress = FALSE;

	fprintf( stream, "\n%s%sLALR(1) STATES\n\n",
		( parser->p_name ? parser->p_name : "" ),
			( parser->p_name ? ": " : "" ) );

	parray_for( parser->states, st )
	{
		fprintf( stream, "  State %d:\n", st->state_id );
		dump_item_set( stream, "Kernel:", st->kernel );
		dump_item_set( stream, "Epsilon:", st->epsilon );
		fprintf( stream, "\n" );
	}
}

/** Dumps all productions.

//stream// is the stream where to dump the productions to. If this is
(FILE*)NULL, output is written to stderr.
//parser// is the parser information structure pointer.
*/
void dump_productions( FILE* stream, PARSER* parser )
{
	PROD*		p;
	plistel*	e;

	if( !stream )
		stream = stderr;

	if( first_progress )
		fprintf( stream, "\n" );
	fprintf( stream, "%s%sPRODUCTIONS\n\n",
		( parser->p_name ? parser->p_name : "" ),
			( parser->p_name ? ": " : "" ) );

	plist_for( parser->productions, e )
	{
		p = (PROD*)plist_access( e );
		dump_production( stream, p, TRUE, TRUE );
		fprintf( stream, "\n" );
	}

	first_progress = FALSE;
}

/** Dumps one production.

//stream// is the stream where to dump the productions to. If this is
(FILE*)NULL, output is written to stderr.
//prod// is the pointer to the production to be printed.
//with_lhs// prints production's lhs and a "->" between rhs
//semantics// is the If TRUE, production-related semantics are even printed.
*/
void dump_production( FILE* stream, PROD* p,
	BOOLEAN with_lhs, BOOLEAN semantics )
{
	plist*		l			= p->rhs;
	plistel*	e;
	BOOLEAN		embedded 	= FALSE;
	SYMBOL*		sym;

	if( !stream )
		stream = stderr;

	if( with_lhs )
	{
		fprintf( stream, "    (%d) ", p->id );

		plist_for( p->all_lhs, e )
		{
			sym = (SYMBOL*)plist_access( e );
			fprintf( stream, "%s ", sym->name );
		}

		fprintf( stream, "-> " );
	}

	if( semantics && plist_count( p->sem_rhs ) )
	{
		l = p->sem_rhs;
		embedded = TRUE;
	}

	plist_for( l, e )
	{
		if( embedded && plist_count( l ) > plist_count( p->rhs ) )
			fprintf( stream, "<<" );
		else
		{
			sym = (SYMBOL*)plist_access( e );
			print_symbol( stream, sym );
		}

		if( semantics && plist_key( e ) )
			fprintf( stream, ":%s", plist_key( e ) );

		fprintf( stream, " " );
	}

	if( p->code && semantics )
		fprintf( stream, " [*%s*]", p->code );

	fprintf( stream, "\n" );
}

