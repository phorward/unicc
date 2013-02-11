/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator 
Copyright (C) 2006-2013 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_first.c
Author:	Jan Max Meyer
Usage:	First set computation

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

/*
 * Includes
 */
#include "p_global.h"
#include "p_proto.h"
#include "p_error.h"

/*
 * Global variables
 */


/*
 * Functions
 */


/* -FUNCTION--------------------------------------------------------------------
	Function:		p_first()
	
	Author:			Jan Max Meyer
	
	Usage:			Computes the FIRST()-set for all symbols that are within the
					global table of symbols.
					
	Parameters:		LIST*		symbols		List of symbols where the first
											computation should be done for.
	
	Returns:		void
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
	23.08.2008	Jan Max Meyer	Uhh bad bug resolved here, in grammars like
								
								x$ -> y ;
								y -> y 'a' | ;

								The first set of y and x was always empty!
								Very, very bad...now it's resolved, due
								uncommenting "if( !k )" in one line below...
								Prototype generator "min_lalr1" did it the
								correct way.
----------------------------------------------------------------------------- */
void p_first( LIST* symbols )
{
	LIST*		i			= (LIST*)NULL;
	LIST*		j			= (LIST*)NULL;
	LIST*		k			= (LIST*)NULL;
	PROD*		p			= (PROD*)NULL;
	SYMBOL* 	sym			= (SYMBOL*)NULL;
	SYMBOL*		s			= (SYMBOL*)NULL;
	int			nullable	= FALSE;
	int			f			= 0;
	int			pf			= 0;

	do
	{
		pf = f;
		f = 0;
		
		for( i = symbols; i; i = i->next )
		{
			s = (SYMBOL*)( i->pptr );
			if( s->type == SYM_NON_TERMINAL )
			{
				for( j = s->productions; j; j = j->next )
				{
					nullable = FALSE;
					
					p = (PROD*)( j->pptr );

					if( p->rhs )
					{
						for( k = p->rhs; k; k = k->next )
						{
							sym = (SYMBOL*)( k->pptr );

							s->first = list_union( s->first, sym->first );
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
			
			f += list_count( s->first );
		}
	}
	while( pf != f );
}


/* -FUNCTION--------------------------------------------------------------------
	Function:		p_rhs_first()
	
	Author:			Jan Max Meyer
	
	Usage:			Performs a right-hand side FIRST()-set seek.
					This is an individual FIRST()-set to be created within a
					LR(1) closure as lookahead set.
					
	Parameters:		LIST*			first		Pointer where the individual
												FIRST() set is stored to.
													
					LIST*			rhs			The right-hand side; This can
												pointer to an element on the
												right-hand side, and is used
												as starting point.
	
	Returns:		int							TRUE if the whole right-hand
												side is possibly nullable,
												FALSE else.
  
	~~~ CHANGES & NOTES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Date:		Author:			Note:
----------------------------------------------------------------------------- */
int p_rhs_first( LIST** first, LIST* rhs )
{
	SYMBOL*		sym		= (SYMBOL*)NULL;
	
	for( ; rhs; rhs = rhs->next )
	{
		sym = rhs->pptr;
		
		if( IS_TERMINAL( sym ) )
		{
			if( list_find( *first, sym ) == -1 )
				*first = list_push( *first, sym );
			break;
		}
		else
		{
			*first = list_union( *first, sym->first );
		
			if( !( sym->nullable ) )
				break;
		}
	}
	
	return sym->nullable;
}

