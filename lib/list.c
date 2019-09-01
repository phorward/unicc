/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	list.c
Usage:	An improved, double linked, optionally hashed list collection object.
----------------------------------------------------------------------------- */

#include "phorward.h"

/*
The plist-object implements

- a double linked-list
- hashable entries
- dynamic stack functionalities
- data object collections
- set functions

into one handy library. It can be used for many powerful tasks, including
symbol tables, functions relating to set theories or as associative arrays.

It serves as the replacement for the older libphorward data structrues
LIST (llist), HASHTAB and STACK, which had been widely used and provided in the
past. All usages of them had been substituted by plist now.

The object pstack can be used for real stacks for a better runtime performance,
because they allocate arrays of memory instead of unfixed, linked elements.
*/

/* Local prototypes */
static pboolean plist_hash_rebuild( plist* list );

/* Local variables & defines */

/* Table size definitions (using non mersenne primes for lesser collisions) */
static const int table_sizes[] = {
	61,      131, 	  257,     509,
	1021,    2053,    4099,    8191,
	16381,   32771,   65537,   131071,
	262147,  524287,  1048573, 2097143,
	4194301, 8388617
};

/* Load factor */
#define	LOAD_FACTOR_HIGH	75	/* resize on 75% load factor
										to avoid collisions */

/* Calculates load factor of the map */
static int plist_get_load_factor( plist* list )
{
	int 	load = 0;
	float	l = 0.00;

	if ( !list )
	{
		WRONGPARAM;
		return -1;
	}

	l = (float)plist_count( list ) / (float)list->hashsize;
	load = (int)( l * 100 );

	return load;
}

/* Compare hash-table elements */
static int plist_hash_compare( plist* list, char* l, char* r, size_t n )
{
	int		res;

	if( !( list && l && r ) )
	{
		WRONGPARAM;
		return -1;
	}

	if( list->flags & PLIST_MOD_PTRKEYS )
		res = (int)( l - r );
	else
	{
		if( !n && list->flags & PLIST_MOD_WCHAR )
			res = wcscmp( (wchar_t*)l, (wchar_t*)r );
		else if( !n )
			res = strcmp( l, r );
		else if( list->flags & PLIST_MOD_WCHAR )
			res = wcsncmp( (wchar_t*)l, (wchar_t*)r, n );
		else
			res = strncmp( l, r, n );
	}

	return res;
}

/* Get hash table index */
static size_t plist_hash_index( plist* list, char* key, size_t n )
{
	size_t hashval	= 5381L;

	if( !( list ) )
	{
		WRONGPARAM;
		return 0;
	}

	if( list->flags & PLIST_MOD_PTRKEYS )
		hashval = (size_t)key;
	else
	{
		#ifdef UNICODE
		if( !n && list->flags & PLIST_MOD_WCHAR )
		{
			wchar_t*	wkey	= (wchar_t*)key;

			while( *wkey )
				hashval += ( hashval << 7 ) + *( wkey++ );
		}
		else
		#endif
		if( !n )
			while( *key )
				hashval += ( hashval << 7 ) + *( key++ );
		#ifdef UNICODE
		else if( list->flags & PLIST_MOD_WCHAR )
		{
			wchar_t*	wkey	= (wchar_t*)key;

			while( n && *wkey )
			{
				hashval += ( hashval << 7 ) + *( wkey++ );
				n--;
			}
		}
		#endif
		else
			while( n && *key )
			{
				hashval += ( hashval << 7 ) + *( key++ );
				n--;
			}
	}

	return hashval % list->hashsize;
}

/* Insert a plist entry node into the hash-table via its key. */
static pboolean plist_hash_insert( plist* list, plistel* e )
{
	plistel*	he;
	plistel**	bucket;

	PROC( "plist_hash_insert" );
	PARMS( "list", "%p", list );
	PARMS( "e", "%p", e );

	if( !( list && e ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !e->key )
		RETURN( FALSE );

	e->hashnext = (plistel*)NULL;
	e->hashprev = (plistel*)NULL;

	if( ! plist_get_by_key( list, e->key ) )
	{
		/* new element, check if we have to resize the map */

		/* check load factor */
		list->load_factor = plist_get_load_factor( list );
		VARS( "load_factor", "%d", list->load_factor );

		if( list->load_factor > LOAD_FACTOR_HIGH )
		{
			MSG( "hashmap has to be resized." );
			if( !plist_hash_rebuild( list ) )
			{
				RETURN( FALSE );
			}

			/* store new load factor */
			list->load_factor = plist_get_load_factor( list );

			VARS( "load_factor", "%d", list->load_factor );
			RETURN( TRUE ); /* e has been inserted by plist_hash_rebuild()! */
		}
	}

	bucket = &( list->hash[ plist_hash_index( list, e->key, 0 ) ] );

	if( ! *bucket )
	{
		MSG( "Bucket is empty, chaining start position" );
		VARS( "e->key", "%s", e->key );

		*bucket = e;
		list->free_hash_entries--;
	}
	else
	{
		MSG( "Chaining into hash" );

		for( he = *bucket; he; he = he->hashnext )
		{
			VARS( "he->key", "%s", he->key );
			VARS( "e->key", "%s", e->key );

			if( plist_hash_compare( list, he->key, e->key, 0 ) == 0 )
			{
				if( list->flags & PLIST_MOD_UNIQUE )
					RETURN( FALSE );

				if( list->flags & PLIST_MOD_KEEPKEYS )
				{
					while( he->hashnext )
						he = he->hashnext;

					e->hashprev = he;
					he->hashnext = e;
				}
				else
				{
					if( he == *bucket )
						*bucket = e;

					/* he will become hidden with e */
					e->hashnext = he;
					e->hashprev = he->hashprev;
					he->hashprev = e;
				}

				break;
			}
			else if( !he->hashnext )
			{
				he->hashnext = e;
				e->hashprev = he;
				break;
			}
		}

		list->hash_collisions++;
	}

	/* store new load factor */
	list->load_factor = plist_get_load_factor( list );
	VARS( "load_factor", "%d", list->load_factor );

	RETURN( TRUE );
}

/* Rebuild hash-table */
static pboolean plist_hash_rebuild( plist* list )
{
	plistel*	e;

	PROC( "plist_hash_rebuild" );
	PARMS( "list", "%p", list );

	if( !list )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( list->size_index + 1 >= ( sizeof( table_sizes) / sizeof( *table_sizes ) ) )
	{
		MSG( "Maximum size is reached." );
		RETURN( FALSE );
	}

	if( list->hash )
		list->hash = pfree( list->hash );

	for( e = plist_first( list ); e; e = plist_next( e ) )
		e->hashnext = (plistel*)NULL;

	list->size_index++;
	list->hashsize = table_sizes[ list->size_index ];
	VARS( "new list->hashsize", "%ld", list->hashsize );

	list->hash_collisions = 0;

	list->hash = (plistel**)pmalloc( list->hashsize * sizeof( plistel* ) );

	for( e = plist_first( list ); e; e = plist_next( e ) )
		plist_hash_insert( list, e );

	RETURN( TRUE );
}

/* Drop list element */
static pboolean plistel_drop( plistel* e )
{
	PROC( "plistel_drop" );

	if( !( e ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* TODO: Call element destructor? */
	if( !( e->flags & PLIST_MOD_EXTKEYS )
			&& !( e->flags & PLIST_MOD_PTRKEYS ) )
		e->key = pfree( e->key );

	RETURN( TRUE );
}

/* Compare elements of a list */
static int plist_compare( plist* list, plistel* l, plistel* r )
{
	if( list->comparefn )
		return (*list->comparefn)( list, l, r );

	return -memcmp( l + 1, r + 1, list->size );
}

/** Initialize the list //list// with an element allocation size //size//.

//flags// defines an optional flag configuration that modifies the behavior
of the linked list and hash table usage. */
pboolean plist_init( plist* list, size_t size, short flags )
{
	if( !( list ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	/* A size of zero causes a pointer list allocation,
		so the flag must also be set. */
	if( size == 0 )
		flags |= PLIST_MOD_PTR;

	if( flags & PLIST_MOD_PTR && size < sizeof( void* ) )
		size = sizeof( void* );

	memset( list, 0, sizeof( plist ) );

	list->flags = flags;
	list->size = size;

	list->hashsize = table_sizes[ list->size_index ];

	list->sortfn = plist_compare;

	list->load_factor = 0;
	list->free_hash_entries = list->hashsize;
	list->hash_collisions = 0;

	return TRUE;
}

/** Create a new plist as an object with an element allocation size //size//.
Providing a //size// of 0 causes automatic configuration of PLIST_MOD_PTR.

//flags// defines an optional flag configuration that modifies the behavior
of the linked list and hash table usage. The flags can be merged together using
bitwise or (|).

Possible flags are:
- **PLIST_MOD_NONE** for no special flagging.
- **PLIST_MOD_PTR** to use the plist-object in pointer-mode: Each \
plistel-element cointains only a pointer to an object in the memory and \
returns this, instead of copying from or into pointers.
- **PLIST_MOD_RECYCLE** to configure that elements that are removed during \
list usage will be reused later.
- **PLIST_MOD_AUTOSORT** to automatically sort elements on insert operations.
- **PLIST_MOD_EXTKEYS** to configure that string pointers to hash-table key\
values are stored elsewhere, so the plist-module only uses the original \
pointers instead of copying them.
- **PLIST_MOD_PTRKEYS** disables string keys and uses the pointer/value \
provided as key directly.
- **PLIST_MOD_KEEPKEYS** holds the correct element insertation sequence. \
In case of a key collision, the inserted element is inserted __behind__ the \
colliding element rather than __before__.
- **PLIST_MOD_UNIQUE** to disallow hash-table-key collisions, so elements with \
a key that already exist in the object will be rejected.
- **PLIST_MOD_WCHAR** to handle all key values as wide-character strings.
-

Use plist_free() to erase and release the returned list object. */
plist* plist_create( size_t size, short flags )
{
	plist*	list;

	list = (plist*)pmalloc( sizeof( plist ) );
	plist_init( list, size, flags );

	return list;
}

/** Creates an independent copy of //list// and returns it.

All elements of //list// are duplicated and stand-alone. */
plist* plist_dup( plist* list )
{
	plist*		dup;

	if( !list )
	{
		WRONGPARAM;
		return (plist*)NULL;
	}

	dup = plist_create( list->size, list->flags );
	dup->comparefn = list->comparefn;
	dup->sortfn = list->sortfn;
	dup->printfn = list->printfn;

	plist_concat( dup, list );

	return dup;
}

/** Erase all allocated content of the list //list//.

The object //list// will be still alive, but must be re-configured
using plist_init(). */
pboolean plist_erase( plist* list )
{
	plistel*	e;
	plistel*	next;

	PROC( "plist_erase" );

	if( !( list ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	MSG( "Freeing current list contents" );
	for( e = list->first; e; e = next )
	{
		next = e->next;

		plistel_drop( e );
		pfree( e );
	}

	MSG( "Freeing list of unused nodes" );
	for( e = list->unused; e; e = next )
	{
		next = e->next;
		pfree( e );
	}

	MSG( "Resetting hash table" );
	if( list->hash )
		pfree( list->hash );

	MSG( "Resetting list-object pointers" );
	list->first = (plistel*)NULL;
	list->last = (plistel*)NULL;
	list->hash = (plistel**)NULL;
	list->unused = (plistel*)NULL;
	list->count = 0;

	RETURN( TRUE );
}

/** Clear content of the list //list//.

The function has nearly the same purpose as plist_erase(), except that
the entire list is only cleared, but if the list was initialized with
PLIST_MOD_RECYCLE, existing pointers are held for later usage. */
pboolean plist_clear( plist* list )
{
	PROC( "plist_clear" );

	if( !( list ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	while( list->first )
		plist_remove( list, list->first );

	RETURN( TRUE );
}

/** Releases all the memory //list// uses and destroys the list object.

The function always returns (plist*)NULL. */
plist* plist_free( plist* list )
{
	if( !( list ) )
		return (plist*)NULL;

	plist_erase( list );
	pfree( list );

	return (plist*)NULL;
}

/** Insert //src// as element to the list //list// __before__ position //pos//.

If //pos// is NULL, the new element will be attached to the end of the
list.

If //key// is not NULL, the element will also be added to the lists hash table.

If //src// is NULL, a zero-initialized element is inserted into the list for
further processing.
*/
plistel* plist_insert( plist* list, plistel* pos, char* key, void* src )
{
	plistel*	e;

	PROC( "plist_insert" );
	PARMS( "list", "%p", list );
	PARMS( "pos", "%p", pos );
	PARMS( "key", "%s", key );
	PARMS( "src", "%p", src );

	if( !( list ) )
	{
		WRONGPARAM;
		RETURN( (plistel*)NULL );
	}

	/* Rebuild hash-table if necessary */
	if( key && !list->hash && !plist_hash_rebuild( list ) )
		RETURN( (plistel*)NULL );

	/* Recycle existing elements? */
	if( list->unused )
	{
		MSG( "Recycle list contains element, recycling" );
		e = list->unused;
		list->unused = e->next;
		memset( e, 0, sizeof( plistel ) + list->size );
		list->recycled--;
	}
	else
	{
		MSG( "Allocating new element" );
		VARS( "size", "%d", sizeof( plistel ) + list->size );

		e = (plistel*)pmalloc( sizeof( plistel ) + list->size );
	}

	e->flags = list->flags;

	if( src )
	{
		MSG( "data is provided, will copy memory" );
		VARS( "sizeof( plistel )", "%d", sizeof( plistel ) );
		VARS( "size", "%d", list->size );

		if( list->flags & PLIST_MOD_PTR )
		{
			MSG( "Pointer mode, just store the pointer" );
			*( (void**)( e + 1 ) ) = src;
		}
		else
		{
			LOG( "Copying %ld bytes", list->size );
			memcpy( e + 1, src, list->size );
		}
	}

	if( !pos )
	{
		MSG( "pos unset, will chain at end of list" );
		if( !( pos = plist_last( list ) ) )
			list->first = e;
		else
		{
			pos->next = e;
			e->prev = pos;
		}

		list->last = e;
	}
	else
	{
		if( ( e->prev = pos->prev ) )
			e->prev->next = e;
		else
			list->first = e;

		e->next = pos;
		pos->prev = e;
	}

	list->count++;

	if( key )
	{
		MSG( "Key provided, will insert into hash table" );
		VARS( "key", "%s", key );

		if( list->flags & PLIST_MOD_EXTKEYS
				|| list->flags & PLIST_MOD_PTRKEYS )
			e->key = key;
		#if UNICODE
		else if( list->flags & PLIST_MOD_WCHAR )
			e->key = (char*)pwcsdup( (wchar_t*)key );
		#endif
		else
			e->key = pstrdup( key );

		if( !plist_hash_insert( list, e ) )
		{
			MSG( "This item collides!" );
			plist_remove( list, e );
			RETURN( (plistel*)NULL );
		}
	}

	if( list->flags & PLIST_MOD_AUTOSORT )
		plist_sort( list );

	VARS( "list->count", "%d", list->count );
	RETURN( e );
}

/** Push //src// to end of //list//.

Like //list// would be a stack, //src// is pushed at the end of the list.
This function can only be used for linked lists without the hash-table feature
in use. */
plistel* plist_push( plist* list, void* src )
{
	if( !( list ) )
	{
		WRONGPARAM;
		return (plistel*)NULL;
	}

	return plist_insert( list, (plistel*)NULL, (char*)NULL, src );
}

/** Shift //src// at begin of //list//.

Like //list// would be a queue, //src// is shifted at the beginning of the list.
This function can only be used for linked lists without the hash-table feature
in use. */
plistel* plist_shift( plist* list, void* src )
{
	if( !( list ) )
	{
		WRONGPARAM;
		return (plistel*)NULL;
	}

	return plist_insert( list, plist_first( list ), (char*)NULL, src );
}

/** Allocates memory for a new element in list //list//, push it to the end and
return the pointer to this.

The function works as a shortcut for plist_access() in combination with
plist_push().
*/
void* plist_malloc( plist* list )
{
	if( !( list ) )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	return plist_access( plist_push( list, (void*)NULL ) );
}

/** Allocates memory for a new element in list //list//, shift it at the begin
and return the pointer to this.

The function works as a shortcut for plist_access() in combination with
plist_shift().
*/
void* plist_rmalloc( plist* list )
{
	if( !( list ) )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	return plist_access( plist_shift( list, (void*)NULL ) );
}

/** Removes the element //e// from the //list// and frees it or puts
 it into the unused element chain if PLIST_MOD_RECYCLE is flagged. */
pboolean plist_remove( plist* list, plistel* e )
{
	PROC( "plist_remove" );

	if( !( list && e ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( e->prev )
		e->prev->next = e->next;
	else
		list->first = e->next;

	if( e->next )
		e->next->prev = e->prev;
	else
		list->last = e->prev;

	if( e->hashprev )
		e->hashprev->hashnext = e->hashnext;
	else if( list->hash && e->key )
	{
		list->hash[ plist_hash_index( list, e->key, 0 ) ] = e->hashnext;
		list->free_hash_entries++;
	}

	/* Drop element contents */
	plistel_drop( e );

	/* Put unused node into unused list or free? */
	if( list->flags & PLIST_MOD_RECYCLE )
	{
		LOG( "Will recycle current element, resetting %d bytes",
			sizeof( plistel ) + list->size );

		memset( e, 0, sizeof( plistel ) + list->size );

		e->next = list->unused;
		list->unused = e;
		list->recycled++;

		MSG( "Element is now discarded, for later usage" );
	}
	else
	{
		MSG( "Freeing current element" );
		pfree( e );
		MSG( "Element gone" );
	}

	list->count--;

	/* store new load factor */
	list->load_factor = plist_get_load_factor( list );
	VARS( "load_factor", "%d<", list->load_factor );

	RETURN( TRUE );
}

/** Pop last element to //dest// off the list //list//.

Like //list// would be a stack, the last element of the list is popped and
its content is written to //dest//, if provided at the end of the list.

//dest// can be omitted and given as (void*)NULL, so the last element will
be popped off the list and discards. */
pboolean plist_pop( plist* list, void* dest )
{
	if( !( list ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( !list->last )
	{
		if( dest )
		{
			/* Zero dest if there is no more item */
			if( list->flags & PLIST_MOD_PTR )
				*( (void**)dest ) = (void*)NULL;
			else
				memset( dest, 0, list->size );
		}

		return FALSE;
	}

	if( dest )
	{
		if( list->flags & PLIST_MOD_PTR )
			*( (void**)dest ) = plist_access( list->last );
		else
			memcpy( dest, plist_access( list->last ), list->size );
	}

	plist_remove( list, list->last );
	return TRUE;
}

/** Take first element of //list// and write it to //dest//.

Like //list// would be a queue, the first element of the list is taken and
its content is written to //dest//.

//dest// can be omitted and given as (void*)NULL, so the first element from
//list// will be taken and discarded. */
pboolean plist_unshift( plist* list, void* dest )
{
	if( !( list ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( !list->first )
	{
		if( dest )
		{
			/* Zero dest if there is no more item */
			if( list->flags & PLIST_MOD_PTR )
				*( (void**)dest ) = (void*)NULL;
			else
				memset( dest, 0, list->size );
		}

		return FALSE;
	}

	if( dest )
	{
		if( list->flags & PLIST_MOD_PTR )
			*( (void**)dest ) = plist_access( list->first );
		else
			memcpy( dest, plist_access( list->first ), list->size );
	}

	plist_remove( list, list->first );
	return TRUE;
}

/** Retrieve list element by its index from the begin.

The function returns the //n//th element of the list //list//. */
plistel* plist_get( plist* list, size_t n )
{
	plistel*	e;

	if( !( list ) )
	{
		WRONGPARAM;
		return (plistel*)NULL;
	}

	for( e = plist_first( list ); e && n;
			e = plist_next( e ), n-- )
		;

	return e;
}

/** Retrieve list keys by their index from the begin.

The function returns the //n//th key within the list //list//. */
plistel* plist_getkey( plist* list, size_t n )
{
	int			i;
	plistel*	e;

	PROC( "plist_getkey" );
	PARMS( "list", "%p", list );
	PARMS( "n", "%ld", n );

	if( !( list ) )
	{
		WRONGPARAM;
		RETURN( (plistel*)NULL );
	}

	/* Iterate trough the buckets */
	for( i = 0; i < list->hashsize; i++ )
	{
		VARS( "i", "%d", i );

		if( !( e = list->hash[ i ] ) )
			continue;

		while( e )
		{
			VARS( "n", "%ld", n );
			if( !n )
				RETURN( e );

			n--;

			while( ( e = e->hashnext ) )
			{
				if( !( list->flags & PLIST_MOD_PTRKEYS ) )
				{
					if( list->flags & PLIST_MOD_WCHAR )
					{
						VARS( "e->hashprev->key", "%ls", e->hashprev->key );
						VARS( "e->key", "%ls", e->key );
					}
					else
					{
						VARS( "e->hashprev->key", "%s", e->hashprev->key );
						VARS( "e->key", "%s", e->key );
					}
				}

				if( e && plist_hash_compare(
							list, e->hashprev->key, e->key, 0 ) )
					break;
			}
		}
	}

	RETURN( (plistel*)NULL );
}

/** Retrieve list element by its index from the end.

The function returns the //n//th element of the list //list//
from the right. */
plistel* plist_rget( plist* list, size_t n )
{
	plistel*	e;

	if( !( list ) )
	{
		WRONGPARAM;
		return (plistel*)NULL;
	}

	for( e = plist_last( list ); e && n;
			e = plist_prev( e ), n-- )
		;

	return e;
}

/** Retrieve list element by hash-table //key//.

This function tries to fetch a list entry plistel from list //list//
with the key //key//.
*/
plistel* plist_get_by_key( plist* list, char* key )
{
	int			bucket;
	plistel*	e;

	PROC( "plist_get_by_key" );
	PARMS( "list", "%p", list );
	PARMS( "key", "%p", key );

	if( !( list && key ) )
	{
		WRONGPARAM;
		RETURN( (plistel*)NULL );
	}

	if( !list->hash )
		RETURN( (plistel*)NULL );

	bucket = plist_hash_index( list, key, 0 );
	VARS( "bucket", "%d", bucket );

	for( e = list->hash[ bucket ]; e; e = e->hashnext )
	{
		VARS( "e", "%p", e );
		VARS( "e->key", "%p", e->key );

		if( plist_hash_compare( list, e->key, key, 0 ) == 0 )
		{
			MSG( "Key matches" );
			RETURN( e );
		}
	}

	RETURN( e );
}

/** Retrieve list element by hash-table //key//,
where key is limited by //n// bytes.

This function tries to fetch a list entry plistel from list //list//
with the key //key// over a size of //n// bytes.
*/
plistel* plist_get_by_nkey( plist* list, char* key, size_t n )
{
	int			bucket;
	plistel*	e;

	PROC( "plist_get_by_nkey" );
	PARMS( "list", "%p", list );
	PARMS( "key", "%p", key );
	PARMS( "n", "%ld", n );

	if( !( list && key && n ) )
	{
		WRONGPARAM;
		RETURN( (plistel*)NULL );
	}

	if( !list->hash )
		RETURN( (plistel*)NULL );

	bucket = plist_hash_index( list, key, n );
	VARS( "bucket", "%d", bucket );

	for( e = list->hash[ bucket ]; e; e = e->hashnext )
	{
		VARS( "e", "%p", e );
		VARS( "e->key", "%p", e->key );

		if( plist_hash_compare( list, e->key, key, n ) == 0 )
		{
			MSG( "Key matches" );
			RETURN( e );
		}
	}

	RETURN( e );
}

/** Retrieve list element by pointer.

This function returns the list element of the unit within the list //list//
that is the pointer //ptr//.
*/
plistel* plist_get_by_ptr( plist* list, void* ptr )
{
	plistel*	e;

	if( !( list && ptr ) )
	{
		WRONGPARAM;
		return (plistel*)NULL;
	}

	for( e = plist_first( list ); e; e = plist_next( e ) )
		if( plist_access( e ) == ptr )
			return e;

	return (plistel*)NULL;
}

/** Concats the elements of list //src// to the elements of list //dest//.

The function will not run if both lists have different element size settings.

The function returns the number of elements added to //dest//. */
size_t plist_concat( plist* dest, plist* src )
{
	plistel*	e;
	size_t		count;

	if( !( dest && src && dest->size == src->size ) )
	{
		WRONGPARAM;
		return 0;
	}

	count = dest->count;

	plist_for( src, e )
		if( !plist_insert( dest, (plistel*)NULL, e->key, plist_access( e ) ) )
			break;

	return dest->count - count;
}

/** Iterates over //list//.

Iterates over all items of //list// and calls the function //callback// on
every item. The callback function receives the plistel-element pointer of
the list element. */
void plist_iter( plist* list, plistelfn callback )
{
	plistel*	e;

	PROC( "plist_iter" );
	PARMS( "list", "%p", list );
	PARMS( "callback", "%p", callback );

	if( !( list && callback ) )
	{
		WRONGPARAM;
		VOIDRET;
	}

	plist_for( list, e )
		(*callback)( e );

	VOIDRET;
}

/** Iterates backwards over //list//.

Backwardly iterates over all items of //list// and calls the function
//callback// on every item. The callback function receives the plistel-element
pointer of the list element. */
void plist_riter( plist* list, plistelfn callback )
{
	plistel*	e;

	PROC( "plist_riter" );
	PARMS( "list", "%p", list );
	PARMS( "callback", "%p", callback );

	if( !( list && callback ) )
	{
		WRONGPARAM;
		VOIDRET;
	}

	for( e = plist_last( list ); e; e = plist_prev( e ) )
		(*callback)( e );

	VOIDRET;
}

/** Iterates over //list// and accesses every item.

Iterates over all items of //list// and calls the function //callback// on
every item's access. The callback function receives a pointer to the accessed
element. */
void plist_iter_access( plist* list, plistfn callback )
{
	plistel*	e;

	PROC( "plist_iter_access" );
	PARMS( "list", "%p", list );
	PARMS( "callback", "%p", callback );

	if( !( list && callback ) )
	{
		WRONGPARAM;
		VOIDRET;
	}

	plist_for( list, e )
		(*callback)( plist_access( e ) );

	VOIDRET;
}

/** Iterates backwards over //list//.

Backwardly iterates over all items of //list// and calls the function
//callback// on every  item's access. The callback function receives a pointer
to the accessed element. */
void plist_riter_access( plist* list, plistfn callback )
{
	plistel*	e;

	PROC( "plist_riter_access" );
	PARMS( "list", "%p", list );
	PARMS( "callback", "%p", callback );

	if( !( list && callback ) )
	{
		WRONGPARAM;
		VOIDRET;
	}

	for( e = plist_last( list ); e; e = plist_prev( e ) )
		(*callback)( plist_access( e ) );

	VOIDRET;
}

/** Unions elements from list //from// into list //all//.

An element is only added to //all//, if there exists no equal element with the
same size and content.

The function will not run if both lists have different element size settings.

The function returns the number of elements added to //from//. */
size_t plist_union( plist* all, plist* from )
{
	size_t		count;
	plistel*    last;
	plistel*	p;
	plistel*	q;

	PROC( "plist_union" );
	PARMS( "all", "%p", all );
	PARMS( "from", "%p", from );

	if( !( all && from
		&& all->size == from->size
		&& all->comparefn == from->comparefn ) )
	{
		WRONGPARAM;
		RETURN( 0 );
	}

	if( !( count = all->count ) )
		RETURN( plist_concat( all, from ) );

	last = plist_last( all );

	plist_for( from, p )
	{
		for( q = plist_first( all ); q; q = q == last ? NULL : plist_next( q ) )
			if( plist_compare( all, p, q ) == 0 )
				break;

		if( !q )
			if( !plist_push( all, plist_access( p ) ) )
				break;
	}

	VARS( "added", "%ld", all->count - count );
	RETURN( all->count - count );
}

/** Tests the contents (data parts) of the list //left// and the list //right//
for equal elements.

The function returns a value < 0 if //left// is lower //right//, a value > 0
if //left// is greater //right// and a value == 0 if //left// is equal to
//right//. */
int plist_diff( plist* left, plist* right )
{
	plistel*	p;
	plistel*	q;
	int			diff;

	PROC( "plist_diff" );
	PARMS( "left", "%p", left );
	PARMS( "right", "%p", right );

	if( !( left && right
		   && left->size == right->size
		   && left->comparefn == right->comparefn ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( right->count < left->count )
		RETURN( 1 );
	else if( right->count > left->count )
		RETURN( -1 );

	MSG( "OK, requiring deep check" );

	for( p = plist_first( left ), q = plist_first( right );
				p && q; p = plist_next( p ), q = plist_next( q ) )
	{
		if( ( diff = plist_compare( left, p, q ) ) )
		{
			MSG( "Elements are not equal" );
			break;
		}
	}

	VARS( "diff", "%d", diff );
	RETURN( diff );
}

/** Sorts //list// between the elements //from// and //to// according to the
sort-function that was set for the list.

To sort the entire list, use plist_sort().

The sort-function can be modified by using plist_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
pboolean plist_subsort( plist* list, plistel* from, plistel* to )
{
	plistel*	a	= from;
	plistel*	b	= to;
	plistel*	e;
	plistel*	ref;

	int			i	= 0;
	int			j	= 0;

	if( !( list && from && to ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( from == to )
		return TRUE;

	while( a != b )
	{
		j++;

		if( !( a = a->next ) )
		{
			WRONGPARAM;
			return FALSE;
		}
	}

	a = from;
	ref = from;

	do
	{
		while( ( *list->sortfn )( list, a, ref ) > 0 )
		{
			i++;
			a = a->next;
		}

		while( ( *list->sortfn )( list, ref, b ) > 0 )
		{
			j--;
			b = b->prev;
		}

		if( i <= j )
		{
			if( from == a )
				from = b;
			else if( from == b )
				from = a;

			if( to == a )
				to = b;
			else if( to == b )
				to = a;

			plist_swap( list, a, b );

			e = a;
			a = b->next;
			b = e->prev;

			i++;
			j--;
		}
	}
	while( i <= j );

	if( ( b != from ) && ( b != from->prev ) )
		plist_subsort( list, from, b );

	if( ( a != to ) && ( a != to->next ) )
		plist_subsort( list, a, to );

	return TRUE;
}

/** Sorts //list// according to the sort-function that was set for the list.

To sort only parts of a list, use plist_subsort().

The sort-function can be modified by using plist_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
pboolean plist_sort( plist* list )
{
	pboolean	ret;

	if( !( list ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( !plist_first( list ) )
		return TRUE;

	if( list->printfn )
		(*list->printfn)( list );

	ret = plist_subsort( list, plist_first( list ), plist_last( list ) );

	if( list->printfn )
		(*list->printfn)( list );

	return ret;
}

/** Set compare function.

If no compare function is set or NULL is provided, memcmp() will be used
as default fallback. */
pboolean plist_set_comparefn( plist* list,
			int (*comparefn)( plist*, plistel*, plistel* ) )
{
	PROC( "plist_set_comparefn" );
	PARMS( "list", "%p", list );
	PARMS( "compare_fn", "%p", comparefn );

	if( !( list ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !( list->comparefn = comparefn ) )
		list->comparefn = plist_compare;

	RETURN( TRUE );
}

/** Set sort function.

If no sort function is given, the compare function set by plist_set_comparefn()
is used. If even unset, memcmp() will be used. */
pboolean plist_set_sortfn( plist* list,
			int (*sortfn)( plist*, plistel*, plistel* ) )
{
	PROC( "plist_set_sortfn" );
	PARMS( "list", "%p", list );
	PARMS( "sortfn", "%p", sortfn );

	if( !( list ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !( list->sortfn = sortfn ) )
		list->sortfn = plist_compare;

	RETURN( TRUE );
}

/** Set an element dump function. */
pboolean plist_set_printfn( plist* list, void (*printfn)( plist* ) )
{
	PROC( "plist_set_printfn" );
	PARMS( "list", "%p", list );
	PARMS( "printfn", "%p", printfn );

	if( !( list ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	list->printfn = printfn;

	RETURN( TRUE );
}

/** Access data-content of the current element //e//. */
void* plist_access( plistel* e )
{
	if( !( e ) )
		return (void*)NULL;

	/* Dereference pointer list differently */
	if( e->flags & PLIST_MOD_PTR )
		return *((void**)( e + 1 ));

	return (void*)( e + 1 );
}

/** Access key-content of the current element //e//. */
char* plist_key( plistel* e )
{
	if( !( e ) )
		return (char*)NULL;

	return e->key;
}

/** Access next element of current unit //u//. */
plistel* plist_next( plistel* u )
{
	if( !( u ) )
		return (plistel*)NULL;

	return u->next;
}

/** Access previous element of current unit //u//. */
plistel* plist_prev( plistel* u )
{
	if( !( u ) )
		return (plistel*)NULL;

	return u->prev;
}

/** Access next element with same hash value of current unit //u//. */
plistel* plist_hashnext( plistel* u )
{
	if( !( u ) )
		return (plistel*)NULL;

	return u->hashnext;
}

/** Access previous element with same hash value of a current unit //u//. */
plistel* plist_hashprev( plistel* u )
{
	if( !( u ) )
		return (plistel*)NULL;

	return u->hashprev;
}

/** Return the offset of the unit //u// within the list it belongs to. */
int plist_offset( plistel* u )
{
	int		off		= 0;

	while( ( u = plist_prev( u ) ) )
		off++;

	return off;
}

/** Swaps the positions of the list elements //a// and //b// with each
other. The elements must be in the same plist object, else the function
returns FALSE. */
pboolean plist_swap( plist* l, plistel* a, plistel* b )
{
	plistel*	aprev;
	plistel*	anext;
	plistel*	bprev;
	plistel*	bnext;

	if( !( l && a && b ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( a == b )
		return TRUE;

	/* Retrieve pointers */
	aprev = a->prev;
	anext = a->next;
	bprev = b->prev;
	bnext = b->next;

	/* a next */
	if( anext == b )
	{
		a->prev = b;
		b->next = a;
	}
	else if( ( b->next = anext ) )
		anext->prev = b;

	/* a prev */
	if( aprev == b )
	{
		a->next = b;
		b->prev = a;
	}
	else if( ( b->prev = aprev ) )
		aprev->next = b;

	/* b next */
	if( bnext == a )
	{
		b->prev = a;
		a->next = b;
	}
	else if( ( a->next = bnext ) )
		bnext->prev = a;

	/* b prev */
	if( bprev == a )
	{
		b->next = a;
		a->prev = b;
	}
	else if( ( a->prev = bprev ) )
		bprev->next = a;

	/* first */
	if( a == l->first )
		l->first = b;
	else if( b == l->first )
		l->first = a;

	/* last */
	if( a == l->last )
		l->last = b;
	else if( b == l->last )
		l->last = a;

	return TRUE;
}

/** Return first element of list //l//. */
plistel* plist_first( plist* l )
{
	if( !( l ) )
		return (plistel*)NULL;

	return l->first;
}

/** Return last element of list //l//. */
plistel* plist_last( plist* l )
{
	if( !( l ) )
		return (plistel*)NULL;

	return l->last;
}

/** Return element size of list //l//. */
int plist_size( plist* l )
{
	if( !( l ) )
		return 0;

	return l->size;
}

/** Return element count of list //l//. */
int plist_count( plist* l )
{
	if( !( l ) )
		return 0;

	return l->count;
}

/** Prints some statistics for the hashmap in //list// on stderr. */
void plist_dbgstats( FILE* stream, plist* list )
{
	PROC( "plist_dbgstats" );
	PARMS( "stream", "%p", stream );
	PARMS( "list", "%p", list );

	if( !list )
	{
		WRONGPARAM;
		VOIDRET;
	}

	if( !stream )
		stream = stderr;

	fprintf( stream, "list statistics\n" );
	fprintf( stream, "=================================\n" );
	fprintf( stream, "element size:\t %7zd\n", list->size );
	fprintf( stream, "# of elements:\t %7ld\n", list->count );
	fprintf( stream, "# of recycled (unused) elements:\t %7ld\n", list->recycled );
	fprintf( stream, "\nhashmap statistics\n" );
	fprintf( stream, "---------------------------------\n" );
	fprintf( stream, "# of max. buckets:\t %7d\n", list->hashsize );
	fprintf( stream, "# of empty buckets:\t %7d\n", list->free_hash_entries );
	fprintf( stream, "load factor %%:\t\t %7d\n", list->load_factor );
	fprintf( stream, "# of collisions:\t %7d\n", list->hash_collisions );
	fprintf( stream, "\n" );

	VOIDRET;
}

/*TESTCASE:plist object functions
#include <phorward.h>

typedef struct
{
	char	firstname	[ 30 + 1 ];
	char	lastname	[ 30 + 1 ];
} person;

static void dump( plist* list )
{
	person*		pp;
	plistel*	e;

	for( e = plist_first( list ); e; e = plist_next( e ) )
	{
		pp = (person*)plist_access( e );
		printf( "- %s, %s\n", pp->lastname, pp->firstname );
	}
	printf( "%d elements\n", plist_count( list ) );
}

static void dump_by_key( plist* list, char* key )
{
	person*		pp;
	plistel*	e;

	if( !( e = plist_get_by_key( list, key ) ) )
	{
		printf( "<No record found matching '%s'>\n", key );
		return;
	}

	pp = (person*)plist_access( e );
	printf( "%s => %s, %s\n", key, pp->lastname, pp->firstname );
}

static int by_name( plist* list, plistel* a, plistel* b )
{
	person*	ap = plist_access( a );
	person*	bp = plist_access( b );

	return strcmp( ap->lastname, bp->lastname );
}

void testcase( void )
{
	person		p;
	plist*		my;
	plistel*	e;

	// Initialize
	my = plist_create( sizeof( person ), PLIST_MOD_RECYCLE );

	strcpy( p.firstname, "Melinda" );
	strcpy( p.lastname, "Smith" );
	plist_insert( my, NULL, "Smith", &p );

	strcpy( p.firstname, "Brenda" );
	strcpy( p.lastname, "Brandon" );
	plist_insert( my, NULL, "Brandon", &p );

	strcpy( p.firstname, "Monique" );
	strcpy( p.lastname, "Joli" );
	e = plist_insert( my, NULL, "Joli", &p );

	strcpy( p.firstname, "Susan" );
	strcpy( p.lastname, "Mueller" );
	plist_insert( my, NULL, "Mueller", &p );

	dump( my );

	// Sort list by name
	plist_set_sortfn( my, by_name );
	plist_sort( my );

	// Print content
	dump( my );

	// Find by key
	dump_by_key( my, "Joli" );

	// Remove entry
	plist_remove( my, e );

	// Find again by key
	dump_by_key( my, "Joli" );

	// Add more data - first element will be recycled.
	strcpy( p.firstname, "Rei" );
	strcpy( p.lastname, "Ayanami" );
	plist_insert( my, NULL, "Ayanami", &p );
	dump_by_key( my, "Ayanami" );

	// Add data with same key, test collision
	strcpy( p.firstname, "Throttle" );
	strcpy( p.lastname, "Full" );
	plist_insert( my, NULL, "Ayanami", &p );

	// Sort list by name again
	plist_sort( my );

	// Now print and get by key
	dump( my );
	dump_by_key( my, "Ayanami" );

	plist_erase( my );
	dump( my );

	my = plist_free( my );
}

--------------------------------------------------------------------------------
- Smith, Melinda
- Brandon, Brenda
- Joli, Monique
- Mueller, Susan
4 elements
- Smith, Melinda
- Mueller, Susan
- Joli, Monique
- Brandon, Brenda
4 elements
Joli => Joli, Monique
<No record found matching 'Joli'>
Ayanami => Ayanami, Rei
- Smith, Melinda
- Mueller, Susan
- Full, Throttle
- Brandon, Brenda
- Ayanami, Rei
5 elements
Ayanami => Full, Throttle
0 elements
TESTCASE*/

/*TESTCASE:plist behavior with PLIST_MOD_PTR
#include <phorward.h>

void testcase( void )
{
	plist*		mylist; // This is list object
	plistel*	e; 		// e, for list iteration
	char*		values[] = { "Hello", "World", "out there!" };
	char*		tmp;

	mylist = plist_create( sizeof( char* ), PLIST_MOD_RECYCLE | PLIST_MOD_PTR );

	// Create the list.
	plist_push( mylist, (void*)values[0] );
	plist_push( mylist, (void*)values[1] );
	plist_push( mylist, (void*)values[2] );

	printf( "%ld\n", values[0] - values[0] );
	printf( "%ld\n", values[1] - values[0] );
	printf( "%ld\n", values[2] - values[0] );

	// Let's iterate it.
	printf( "mylist contains %d items\n", plist_count( mylist ) );
	for( e = plist_first( mylist ); e; e = plist_next( e ) )
		printf( "%s(%ld) ", (char*)plist_access( e ),
								(char*)plist_access( e ) - values[0] );

	// Now, we remove one element (identified by its pointer)
	//    and iterate the list again
	plist_remove( mylist, plist_get_by_ptr( mylist, (void*)values[1] ) );
	printf( "\nmylist contains now %d items\n", plist_count( mylist ) );

	// The macro plist_for() expands into a for-loop like above...
	plist_for( mylist, e )
	printf( "%s(%ld) ", (char*)plist_access( e ),
							(char*)plist_access( e ) - values[0] );

	printf( "\n" );

	plist_pop( mylist, (void*)&tmp );
	printf( "tmp = %ld >%s<\n", tmp - values[0], tmp );

	// Free the entire list
	mylist = plist_free( mylist );
}
--------------------------------------------------------------------------------
0
6
12
mylist contains 3 items
Hello(0) World(6) out there!(12)
mylist contains now 2 items
Hello(0) out there!(12)
tmp = 12 >out there!<
TESTCASE*/
