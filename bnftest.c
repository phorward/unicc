#include "unicc.h"

Grammar*	res;
int			prec	= 0;

/* Derive name from basename */
static char* derive_name( Grammar* gram, char* base )
{
	int             i;
	static char		deriv   [ ( NAMELEN * 2 ) + 1 ];

	sprintf( deriv, "%s%c", base, DERIVCHAR );

	for( i = 0; strlen( deriv ) < ( NAMELEN * 2 ); i++ )
	{
		if( !sym_get_by_name( gram, deriv ) )
			return pstrdup( deriv );

		sprintf( deriv + strlen( deriv ), "%c", DERIVCHAR );
	}

	return (char*)NULL;
}

void traverse( Ast_eval type, AST_node* node )
{
	if( type != AST_EVAL_BOTTOMUP )
		return;

	{
		char	buf		[ BUFSIZ + 1 ];

		PROC( "traverse" );

		VARS( "emit", "%s", node->emit );

		if( node->len )
		{
			sprintf( buf, "%.*s", (int)node->len, node->start );
			VARS( "buf", "%s", buf );
		}
		else
			*buf = '\0';

		if( !strcmp( node->emit, "Identifier" ) )
			node->ptr = strdup( buf );
		else if( !strcmp( node->emit, "variable" ) )
		{
			if( !( node->ptr = (void*)sym_get_by_name(
						res, node->child->ptr ) ) )
			{
				Symbol* sym;

				sym = sym_create( res, node->child->ptr );
				sym->flags.freename = TRUE;

				node->ptr = (void*)sym;
			}
			else
				pfree( node->child->ptr );
		}
		else if( !strcmp( node->emit, "CCL" )
					|| !strcmp( node->emit, "String" )
						|| !strcmp( node->emit, "Token" )
							|| !strcmp( node->emit, "Regex" ) )
		{
			if( !( node->ptr = (void*)sym_get_by_name( res, buf ) ) )
			{
				Symbol* sym;

				sym = sym_create( res, pstrdup( buf ) );
				sym->flags.freeemit = TRUE;
				sym->flags.nameless = TRUE;

				if( !strcmp( node->emit, "CCL" )
					|| !strcmp( node->emit, "String" )
						|| !strcmp( node->emit, "Token" )
							|| !strcmp( node->emit, "Regex" ) )
				{
					memmove( buf, buf + 1, node->len - 1 );
					buf[ node->len - 2 ] = '\0';

					if( !strcmp( node->emit, "CCL" ) )
						sym->ccl = pccl_create( -1, -1, buf );
					else if( !strcmp( node->emit, "String" )
								|| !strcmp( node->emit, "Token" ) )
						sym->str = strdup( buf );
					else if( !strcmp( node->emit, "Regex" ) )
						sym->ptn = pregex_ptn_create(
										buf, PREGEX_COMP_NOERRORS );

					if( !strcmp( node->emit, "Token" ) )
						sym->emit = sym->str;
				}

				node->ptr = (void*)sym;
			}
		}
		else if( !strcmp( node->emit, "kle" ) )
			node->ptr = (void*)sym_mod_kleene( node->child->ptr );
		else if( !strcmp( node->emit, "pos" ) )
			node->ptr = (void*)sym_mod_positive( node->child->ptr );
		else if( !strcmp( node->emit, "opt" ) )
			node->ptr = (void*)sym_mod_optional( node->child->ptr );
		else if( !strcmp( node->emit, "definition" )
					|| !strcmp( node->emit, "inline" ) )
		{
			Symbol*		sym;
			Production*	prod;
			AST_node*	pnode;
			char*		emits		= (char*)NULL;

			if( !strcmp( node->emit, "inline" ) )
			{
				for( pnode = node; pnode; pnode = pnode->parent )
				{
					VARS( "pnode->emit", "%s", pnode->emit );

					if( !strcmp( pnode->emit, "definition" ) )
					{
						sym = (Symbol*)pnode->child->ptr;
						break;
					}
				}

				VARS( "sym->name", "%s", sym->name );

				node->ptr = sym = sym_create( res,
									derive_name( res, sym->name ) );

				VARS( "sym->name", "%s", sym->name );

				sym->flags.freename = TRUE;
				sym->flags.nameless = TRUE;
				sym->flags.defined = TRUE;
				sym->flags.generated = TRUE;

				node = node->child;
			}
			else
			{
				sym = (Symbol*)node->child->ptr;
				VARS( "sym->name", "%s", sym->name );

				node = node->child->next;

				if( node && !strcmp( node->emit, "Flag_goal" ) )
				{
					res->goal = sym;
					node = node->next;
				}

				if( node && !strcmp( node->emit, "emitsdef" ) )
				{
					emits = sym->name;
					node = node->next;
				}
			}

			while( node )
			{
				prod = prod_create( res, sym, NULL );
				prod->emit = emits;

				for( pnode = node->child; pnode; pnode = pnode->next )
				{
					if( !strcmp( pnode->emit, "emits" ) )
					{
						prod->emit = pnode->ptr;
						prod->flags.freeemit = TRUE;
						break;
					}

					prod_append( prod, pnode->ptr );
				}

				node = node->next;
			}
		}
		else if( !strcmp( node->emit, "Flag_ignore" )
					|| !strcmp( node->emit, "assoc_left" )
						|| !strcmp( node->emit, "assoc_right" )
							|| !strcmp( node->emit, "assoc_none" ) )
		{
			Symbol*		sym;
			AST_node*	snode;

			if( !strncmp( node->emit, "assoc_", 6 ) )
				prec++;

			for( snode = node->child; snode; snode = snode->next )
			{
				sym = (Symbol*)snode->ptr;

				if( !strcmp( node->emit, "assoc_left" ) )
				{
					sym->assoc = ASSOC_LEFT;
					sym->prec = prec;
				}
				else if( !strcmp( node->emit, "assoc_right" ) )
				{
					sym->assoc = ASSOC_RIGHT;
					sym->prec = prec;
				}
				else if( !strcmp( node->emit, "assoc_none" ) )
				{
					sym->assoc = ASSOC_NONE;
					sym->prec = prec;
				}
				else if( !strcmp( node->emit, "Flag_ignore" ) )
					sym->flags.whitespace = TRUE;
			}
		}

		VOIDRET;
	}
}


int main( int argc, char** argv )
{
	Grammar*	g;
	Parser*		p;
	AST_node*	a;

	PROC( "main" );

	g = gram_create();

	#include "bnftestgen.c"

	/*
	GRAMMAR_DUMP( g );
	printf( "%s\n", gram_to_bnf( g ) );
	*/

	gram_prepare( g );

	p = par_create( g );

	if( argc > 1 && par_parse( &a, p, argv[1] ) )
	{
		ast_dump_short( stdout, a );

		res = gram_create();
		/* res->flags.debug = TRUE; */

		ast_eval( a, traverse );

		printf( "%s\n", gram_to_bnf( res ) );
		GRAMMAR_DUMP( res );
	}

	RETURN( 0 );
}
