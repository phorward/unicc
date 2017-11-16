/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com ++ unicc<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	first.c
Author:	Jan Max Meyer
Usage:	First set computation
----------------------------------------------------------------------------- */

#include "unicc.h"

/** Computes the FIRST()-set for all symbols that are within the global table
of symbols.

//symbols// is the list of symbols where the first computation should be done
for.
*/
void compute_first( PARSER* parser )
{
	plistel*	e;
	plistel*	f;
	plistel*	g;
	PROD*		p			= (PROD*)NULL;
	SYMBOL* 	sym			= (SYMBOL*)NULL;
	SYMBOL*		s			= (SYMBOL*)NULL;
	int			nullable	= FALSE;
	int			cnt			= 0;
	int			prev_cnt;

	/*
		23.08.2008	Jan Max Meyer

		Uhh bad bug resolved here, in grammars like

		x$ -> y ;
		y -> y 'a' | ;

		The first set of y and x was always empty!
		Very, very bad...now it's resolved, due
		uncommenting "if( !k )" in one line below...
		Prototype generator "min_lalr1" did it the
		correct way
	*/
	do
	{
		prev_cnt = cnt;
		cnt = 0;

		plist_for( parser->symbols, e )
		{
			s = (SYMBOL*)plist_access( e );

			if( s->type == SYM_NON_TERMINAL )
			{
				plist_for( s->productions, f )
				{
					nullable = FALSE;

					p = (PROD*)plist_access( f );

					if( plist_count( p->rhs ) > 0 )
					{
						plist_for( p->rhs, g )
						{
							sym = (SYMBOL*)plist_access( g );

							plist_union( s->first, sym->first );
							nullable = sym->nullable;

							if( !nullable )
								break;
						}
					}
					else
						nullable = TRUE;

					s->nullable |= nullable;
				}
			}

			cnt += plist_count( s->first );
		}
	}
	while( prev_cnt != cnt );
}


/** Performs a right-hand side FIRST()-set seek. This is an individual
FIRST()-set to be created within a LR(1) closure as lookahead set.

//first// is where the individual FIRST() set is stored to.
//rhs// is the right-hand side to be processed; This is a pointer to an
element on the right-hand side, and is used as starting point.

Returns TRUE if the whole right-hand side is possibly nullable, FALSE else.
*/
int seek_rhs_first( plist* first, plistel* rhs )
{
	SYMBOL*		sym		= (SYMBOL*)NULL;

	for( ; rhs; rhs = plist_next( rhs ) )
	{
		sym = (SYMBOL*)plist_access( rhs );

		if( IS_TERMINAL( sym ) )
		{
			if( !plist_get_by_ptr( first, sym ) )
				plist_push( first, sym );

			break;
		}
		else
		{
			plist_union( first, sym->first );

			if( !( sym->nullable ) )
				break;
		}
	}

	return sym->nullable;
}

