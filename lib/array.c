/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	array.c
Author:	Jan Max Meyer
Usage:	Universal, dynamic array management functions.
----------------------------------------------------------------------------- */

#include "phorward.h"

#ifndef PARRAY_DEFAULT_CHUNK
#define PARRAY_DEFAULT_CHUNK		128			/* Default chunk size */
#endif

/* Compare elements of a list */
static int parray_compare( parray* array, void* l, void* r )
{
	if( array->comparefn )
		return (*array->comparefn)( array, l, r );

	return -memcmp( l, r, array->size );
}

/** Performs an array initialization.

//array// is the pointer to the array to be initialized.

//size// defines the size of one array element, in bytes.
This should be evaluated using the sizeof()-macro.

//chunk// defines the chunk size, when an array-(re)allocation will be
performed. If, e.g. this is set to 128, then, if the 128th item is created
within the array, a realloction is done. Once allocated memory remains until
the array is freed again.
*/
pboolean parray_init( parray* array, size_t size, size_t chunk )
{
	PROC( "parray_init" );
	PARMS( "array", "%p", array );
	PARMS( "size", "%ld", size );
	PARMS( "chunk", "%ld", chunk );

	if( !( array && size > 0 ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( chunk <= 0 )
		chunk = PARRAY_DEFAULT_CHUNK;

	memset( array, 0, sizeof( parray ) );
	array->size = size;
	array->chunk = chunk;

	array->sortfn = parray_compare;

	RETURN( TRUE );
}

/** Create a new parray as an object with an element allocation size //size//,
a reallocation-chunk-size of //chunk//.

The returned memory must be released with parray_free().  */
parray* parray_create( size_t size, size_t chunk )
{
	parray*	array;

	if( size <= 0 )
	{
		WRONGPARAM;
		return (parray*)NULL;
	}

	array = (parray*)pmalloc( sizeof( parray ) );
	parray_init( array, size, chunk );

	return array;
}

/** Erase a dynamic array.

The array must not be reinitialized after destruction, using parray_init().

//array// is the pointer to the array to be erased. */
pboolean parray_erase( parray* array )
{
	PROC( "parray_free" );
	PARMS( "array", "%p", array );

	if( !array )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	array->bottom = array->top = array->start = pfree( array->start );
	array->count = array->first = array->last = 0;

	RETURN( TRUE );
}

/** Releases all the memory //array// uses and destroys the array object.

The function always returns (parray*)NULL. */
parray* parray_free( parray* array )
{
	if( !array )
		return (parray*)NULL;

	parray_erase( array );
	pfree( array );

	return (parray*)NULL;
}

/** Appends an element to the end of the array.

The element's memory is copied during the push. The item must be of the same
memory size as used at array initialization.

//array// is the pointer to array where to push an item on.

//item// is the pointer to the memory of the item that should be pushed onto the
array. The caller should cast his type into void, or wrap the push-operation
with a macro. It can be left (void*)NULL, so no memory will be copied.

The function returns the address of the newly pushed item, and (void*)NULL if
the item could not be pushed.
*/
void* parray_push( parray* array, void* item )
{
	if( !( array ) )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	/* Is reallocation required? */
	if( !array->count || array->last == array->count )
	{
		array->count += array->chunk;

		if( !( array->start = (void*)prealloc(
				(void*)array->start, array->count * array->size ) ) )
			return (void*)NULL;
	}

	array->bottom = array->start + array->first * array->size;
	array->top = array->start + ++array->last * array->size;

	/* Copy item into last of array */
	if( item )
		memcpy( array->top - array->size, item, array->size );

	return array->top - array->size;
}

/** Reserves memory for //n// items in //array//.

This function is only used to assume that no memory reallocation is done when
the next //n// items are inserted/malloced. */
pboolean parray_reserve( parray* array, size_t n )
{
	PROC( "parray_reserve" );
	PARMS( "array", "%p", array );
	PARMS( "n", "%ld", n );

	if( !( array && n > 0 ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( array->last + n < array->count )
		RETURN( TRUE );

	array->count += n + ( n % array->chunk );

	if( !( array->start = (void*)prealloc(
			(void*)array->start, array->count * array->size ) ) )
		RETURN( FALSE );

	array->bottom = array->start + array->first * array->size;
	array->top = array->start + array->last * array->size;

	RETURN( TRUE );
}

/** Pushes and "allocates" an empty element on the array.

This function is just a shortcut to ```parray_push( array, (void*)NULL )```,
and the memory of the pushed element is initialized to zero. */
void* parray_malloc( parray* array )
{
	void*	ptr;

	PROC( "parray_malloc" );
	PARMS( "array", "%p", array );

	if( !( array ) )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	if( !( ptr = parray_push( array, (void*)NULL ) ) )
		return ptr;

	memset( ptr, 0, array->size );
	return ptr;
}

/** Unshifts and "allocates" an empty element on the array.

This function is just a shortcut to ```parray_unshift( array, (void*)NULL )```,
and the memory of the unshifted element is initialized to zero. */
void* parray_rmalloc( parray* array )
{
	void*	ptr;

	PROC( "parray_rmalloc" );
	PARMS( "array", "%p", array );

	if( !( array ) )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	if( !( ptr = parray_unshift( array, (void*)NULL ) ) )
		return ptr;

	memset( ptr, 0, array->size );
	return ptr;
}

/** Insert item //item// at //offset// into array //array//.
Items right to //offset// will move up.

Gap space between the offset is filled with zero elements;
Handle with care! */
void* parray_insert( parray* array, size_t offset, void* item )
{
	void*	slot;

	PROC( "parray_insert" );
	PARMS( "array", "%p", array );
	PARMS( "offset", "%ld", offset );
	PARMS( "item", "%p", item );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	/* Within current bounds? */
	if( array->first + offset < array->last )
	{
		MSG( "offset within bounds, inserting" );

		/* Allocate one item for moving up */
		parray_malloc( array );

		slot = array->start + ( array->first + offset ) * array->size;

		/* Move up existing items right of offset */
		memmove( slot + array->size, slot,
					( array->last - 1 -
						( array->first + offset ) ) * array->size );

		/* Put new element */
		if( item )
			memcpy( slot, item, array->size );
		else
			memset( slot, 0, array->size );

		RETURN( slot );
	}

	while( array->first + offset >= array->last )
	{
		if( !( slot = parray_malloc( array ) ) )
		{
			MSG( "Out of mem?" );
			RETURN( slot );
		}
	}

	if( item )
		memcpy( slot, item, array->size );

	RETURN( slot );
}

/** Remove item on //offset// from array //array//.

The removed item will be copied into //item//, if //item// is not NULL.
The function returns the memory of the removed item (it will contain the
moved up data part or invalid memory, if on the end). */
void* parray_remove( parray* array, size_t offset, void** item )
{
	void*	slot;

	PROC( "parray_remove" );
	PARMS( "array", "%p", array );
	PARMS( "offset", "%ld", offset );
	PARMS( "item", "%p", item );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	/* Within current bounds? */
	if( array->first + offset >= array->last )
	{
		MSG( "Index out of bounds." );
		RETURN( (void*)NULL );
	}

	slot = array->start + ( array->first + offset ) * array->size;

	if( item )
		memcpy( *item, slot, ( array->first + offset ) * array->size );

	/* Move existing items to the left */
	if( array->first + offset + 1 < array->last )
		memmove( slot, slot + array->size,
				( array->last - ( array->first + offset + 1 ) )
					* array->size );

	array->last--;
	array->top -= array->size;

	RETURN( slot );
}

/** Removes an element from the end of an array.

The function returns the pointer of the popped item. Because dynamic arrays only
grow and no memory is freed, the returned data pointer is still valid, and will
only be overridden with the next push operation.

//array// is the pointer to array where to pop an item off.

The function returns the address of the popped item, and (void*)NULL if the
item could not be popped (e.g. array is empty).
*/
void* parray_pop( parray* array )
{
	PROC( "parray_pop" );
	PARMS( "array", "%p", array );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	if( array->last == array->first )
	{
		MSG( "last is zero, no items on the array." );
		RETURN( (void*)NULL );
	}

	array->top -= array->size;

	RETURN( array->start + ( ( --array->last ) * array->size ) );
}

/** Appends an element to the begin of the array.

The elements memory is copied during the unshift. The item must be of the same
memory size as used at array initialization.

//array// is the pointer to array where to push an item to the beginning.

//item// is the pointer to the memory of the item that should be pushed onto the
array. The caller should cast his type into void, or wrap the push-operation
with a macro. It can be left (void*)NULL, so no memory will be copied.

The function returns the address of the newly unshifted item, and (void*)NULL
if the item could not be unshifted.
*/
void* parray_unshift( parray* array, void* item )
{
	PROC( "parray_unshift" );
	PARMS( "array", "%p", array );
	PARMS( "item", "%p", item );

	if( !( array ) )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	/* Is reallocation required? */
	if( !array->count || array->first == 0 )
	{
		array->count += array->chunk;

		if( !( array->start = (void*)prealloc(
				(void*)array->start, array->count * array->size ) ) )
			RETURN( (void*)NULL );

		array->first = array->chunk;

		if( array->last > 0 )
			memmove( array->start + array->first * array->size,
						array->start, array->last * array->size );

		array->last += array->first;

		array->bottom = array->start + array->first * array->size;
		array->top = array->start + array->last * array->size;
	}

	array->bottom -= array->size;
	array->first--;

	/* Copy item into last of array */
	if( item )
		memcpy( array->bottom, item, array->size );

	RETURN( array->bottom );
}

/** Removes an element from the begin of an array.

The function returns the pointer of the shifted item.
Because dynamic arrays only grow and no memory is freed, the returned data
pointer is still valid, and will only be overridden with the next unshift
operation.

//array// is the pointer to array where to pop an item off.

The function returns the address of the shifted item, and (void*)NULL if the
item could not be popped (e.g. array is empty).
*/
void* parray_shift( parray* array )
{
	PROC( "parray_shift" );
	PARMS( "array", "%p", array );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	if( array->last == array->first )
	{
		MSG( "last is zero, no items on the array." );
		RETURN( (void*)NULL );
	}

	array->bottom += array->size;
	RETURN( array->start + array->first++ * array->size );
}

/** Access an element from the array by its offset position from the left.

//array// is the pointer to array where to access the element from.
//offset// is the offset of the element to be accessed from the array's
base address.

Returns the address of the accessed item, and (void*)NULL if the item could not
be accessed (e.g. if the array is empty or offset is beyond the last of array).

Use parray_rget() for access items from the end.
*/
void* parray_get( parray* array, size_t offset )
{
	PROC( "parray_get" );
	PARMS( "array", "%p", array );
	PARMS( "offset", "%d", offset );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	if( array->last == array->first
			|| offset >= ( array->last - array->first ) )
	{
		MSG( "offset defines an item that is out of bounds of the array" );
		RETURN( (void*)NULL );
	}

	RETURN( array->start + ( array->first + offset ) * array->size );
}

/** Put an element //item// at position //offset// of array //array//.

//array// is the pointer to array where to put the element to.
//offset// is the offset of the element to be set.
//item// is a pointer to the memory that will be copied into the
position at //offset//. If this is NULL, the position at //offset// will be
set to zero.

Returns the address of the item in the array, or NULL if the desired offset
is out of the array bounds.
*/
void* parray_put( parray* array, size_t offset, void* item )
{
	void*	slot;

	PROC( "parray_put" );
	PARMS( "array", "%p", array );
	PARMS( "offset", "%d", offset );
	PARMS( "item", "%p", item );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	if( array->last == array->first
			|| offset >= ( array->last - array->first ) )
	{
		MSG( "Index out of bounds" );
		RETURN( (void*)NULL );
	}

	slot = array->start + ( array->first + offset ) * array->size;

	if( item )
		memcpy( slot, item, array->size );
	else
		memset( slot, 0, array->size );

	RETURN( slot );
}

/** Access an element from the array by its offset position from the right.

//array// is the pointer to array where to access the element from.
//offset// is the offset of the element to be accessed from the array's
base address.

Returns the address of the accessed item, and (void*)NULL if the item could not
be accessed (e.g. if the array is empty or offset is beyond the bottom of
the array).

Use parray_get() to access items from the begin.
*/
void* parray_rget( parray* array, size_t offset )
{
	PROC( "parray_rget" );
	PARMS( "array", "%p", array );
	PARMS( "offset", "%d", offset );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	RETURN( parray_get( array, array->last - offset ) );
}

/** Put an element //item// at position //offset// from the right of
array //array//.

//array// is the pointer to array where to put the element to.
//offset// is the offset of the element to be set.
//item// is a pointer to the memory that will be copied into the
position at //offset//. If this is NULL, the position at //offset// will be
set to zero.

Returns the address of the item in the array, or NULL if the desired offset
is out of the array bounds.
*/
void* parray_rput( parray* array, size_t offset, void* item )
{
	PROC( "parray_rput" );
	PARMS( "array", "%p", array );
	PARMS( "offset", "%d", offset );
	PARMS( "item", "%p", item );

	if( !array )
	{
		WRONGPARAM;
		RETURN( (void*)NULL );
	}

	RETURN( parray_put( array, array->last - offset, item ) );
}

/** Iterates over //array//.

Iterates over all items of //array// and calls the function //callback// on
every item. */
void parray_iter( parray* array, parrayfn callback )
{
	void*	ptr;

	PROC( "parray_iter" );
	PARMS( "array", "%p", array );
	PARMS( "callback", "%p", callback );

	if( !( array && callback ) )
	{
		WRONGPARAM;
		VOIDRET;
	}

	for( ptr = parray_first( array ); ptr; ptr = parray_next( array, ptr ) )
		(*callback)( array, ptr );

	VOIDRET;
}

/** Iterates backwards over //array//.

Backwardly iterates over all items of //array// and calls the function
//callback// on every item. */
void parray_riter( parray* array, parrayfn callback )
{
	void*	ptr;

	PROC( "parray_riter" );
	PARMS( "array", "%p", array );
	PARMS( "callback", "%p", callback );

	if( !( array && callback ) )
	{
		WRONGPARAM;
		VOIDRET;
	}

	for( ptr = parray_last( array ); ptr; ptr = parray_prev( array, ptr ) )
		(*callback)( array, ptr );

	VOIDRET;
}

/** Access first element of the array.

Returns the address of the accessed item, and (void*)NULL if nothing is in
the array.
*/
void* parray_first( parray* array )
{
	if( !array )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	if( array->first == array->last )
		return (void*)NULL;

	return array->bottom;
}

/** Access last element of the array.

Returns the address of the accessed item, and (void*)NULL if nothing is in
the array.
*/
void* parray_last( parray* array )
{
	if( !array )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	if( array->first == array->last )
		return (void*)NULL;

	return array->top - array->size;
}

/** Access next element from //ptr// in //array//.

Returns the address of the next element, and (void*)NULL if the access gets
out of bounds.
*/
void* parray_next( parray* array, void* ptr )
{
	if( !( array && ptr ) )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	ptr += array->size;
	if( ptr > parray_last( array ) || ptr < parray_first( array ) )
		return (void*)NULL;

	return ptr;
}

/** Access previous element from //ptr// in //array//.

Returns the address of the previous element, and (void*)NULL if the access gets
out of bounds.
*/
void* parray_prev( parray* array, void* ptr )
{
	if( !( array && ptr ) )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	ptr -= array->size;
	if( ptr < parray_first( array ) || ptr > parray_last( array ) )
		return (void*)NULL;

	return ptr;
}

/** Swap two elements of an array. */
void* parray_swap( parray* array, size_t pos1, size_t pos2 )
{
	void*	ptr1;
	void*	ptr2;

	if( !( array ) )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	if( !( ( ptr1 = parray_get( array, pos1 ) )
			&& ( ptr2 = parray_get( array, pos2 ) ) ) )
		return (void*)NULL;

	if( ptr1 == ptr2 )
		return ptr1;

	parray_push( array, ptr1 );
	parray_put( array, pos1, ptr2 );
	parray_put( array, pos2, parray_pop( array ) );

	return ptr1;
}

/** Returns the number of elements in a array. */
size_t parray_count( parray* array )
{
	if( !array )
		return 0;

	return array->last - array->first;
}

/** Returns TRUE, if //ptr// is an element of array //array//. */
pboolean parray_partof( parray* array, void* ptr )
{
	if( !( array && ptr ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( ptr >= parray_first( array ) && ptr <= parray_last( array ) )
		return TRUE;

	return FALSE;
}

/** Return offset of element //ptr// in array //array//.
Returns the offset of //ptr// in //array//.
The function returns the size of the array (which is an invalid offset)
if //ptr// is not part of //array//.

To check if a pointer belongs to an array, call parray_partof(). */
size_t parray_offset( parray* array, void* ptr )
{
	if( !( array && ptr ) )
	{
		WRONGPARAM;
		return 0;
	}

	if( !parray_partof( array, ptr ) )
		return parray_count( array );

	return ( (char*)ptr - ( array->start + ( array->first * array->size ) ) )
				/ array->size;
}

/*TESTCASE:parray_(create|free|push|pop|shift|insert|unshift|remove)
#include <phorward.h>

typedef struct
{
	char	firstname	[ 30 + 1 ];
	char	lastname	[ 30 + 1 ];
} person;

static void dump( parray* a )
{
	int		i;
	person*	p;

	printf( "first = %ld last = %ld count = %ld\n",
				a->first, a->last, a->count );

	printf( "-- %d Elements --\n", parray_count( a ) );

	for( i = 0; ( p = (person*)parray_get( a, i ) ); i++ )
		printf( "%02d) %s %s\n", i, p->firstname, p->lastname );

	printf( "-- %d Elements --\n", parray_count( a ) );
}

void testcase( void )
{
	person 	x;
	person*	p;
	parray*	a;

	a = parray_create( sizeof( person ), 0 );

	strcpy( x.lastname, "Zufall" );
	strcpy( x.firstname, "Reiner" );
	parray_push( a, (void*)&x );

	dump( a );

	strcpy( x.lastname, "Pfahl" );
	strcpy( x.firstname, "Martha" );
	p = (person*)parray_insert( a, 5, (void*)&x );

	dump( a );
	printf( "%ld\n", parray_offset( a, p - 10 ) );

	strcpy( x.lastname, "Racho" );
	strcpy( x.firstname, "Volker" );
	parray_unshift( a, (void*)&x );

	dump( a );

	strcpy( x.lastname, "Pete" );
	strcpy( x.firstname, "Dieter" );
	parray_unshift( a, (void*)&x );

	dump( a );

	parray_remove( a, 1, (void**)NULL );

	dump( a );

	a = parray_free( a );
}

--------------------------------------------------------------------------------
first = 0 last = 1 count = 128
-- 1 Elements --
00) Reiner Zufall
-- 1 Elements --
first = 0 last = 6 count = 128
-- 6 Elements --
00) Reiner Zufall
01)
02)
03)
04)
05) Martha Pfahl
-- 6 Elements --
6
first = 127 last = 134 count = 256
-- 7 Elements --
00) Volker Racho
01) Reiner Zufall
02)
03)
04)
05)
06) Martha Pfahl
-- 7 Elements --
first = 126 last = 134 count = 256
-- 8 Elements --
00) Dieter Pete
01) Volker Racho
02) Reiner Zufall
03)
04)
05)
06)
07) Martha Pfahl
-- 8 Elements --
first = 126 last = 133 count = 256
-- 7 Elements --
00) Dieter Pete
01) Reiner Zufall
02)
03)
04)
05)
06) Martha Pfahl
-- 7 Elements --
TESTCASE*/

/** Concats the elements of array //src// to the elements of array //dest//.

The function will not run if both arrays have different element size settings.

The function returns the number of elements added to //dest//. */
size_t parray_concat( parray* dest, parray* src )
{
	size_t		count;
	char*       p;

	if( !( dest && src && dest->size == src->size ) )
	{
		WRONGPARAM;
		return 0;
	}

	count = dest->count;

	/* fixme: This can be done much faster! */
	for( p = src->bottom; p && p < src->top; p += src->size )
		if( !parray_push( dest, p ) )
			break;

	return dest->count - count;
}

/** Unions elements from array //from// into array //all//.

An element is only added to //all//, if there exists no equal element with the
same size and content.

The function will not run if both arrays have different element size settings.

The function returns the number of elements added to //from//. */
size_t parray_union( parray* all, parray* from )
{
	size_t		last;
	char*		top;
	char*       p;
	char*       q;

	PROC( "parray_union" );
	PARMS( "all", "%p", all );
	PARMS( "from", "%p", from );

	if( !( all && from
		&& all->size == from->size
		&& all->comparefn == from->comparefn ) )
	{
		WRONGPARAM;
		RETURN( 0 );
	}

	if( !( last = all->last ) )
		RETURN( parray_concat( all, from ) );

	for( p = from->bottom; p < from->top; p += from->size )
	{
		top = all->bottom + last * all->size;

		for( q = all->bottom; q < top; q += all->size )
			if( parray_compare( all, p, q ) == 0 )
				break;

		if( q == top )
			if( !parray_push( all, p ) )
				break;
	}

	VARS( "added", "%ld", parray_count( all ) - last );
	RETURN( parray_count( all ) - last );
}

/*TESTCASE:parray_union
#include <phorward.h>

void dump( parray* a, void* p )
{
	printf( "%c%s", *((char*)p), p == parray_last( a ) ? "\n" : "" );
}

void testcase()
{
	parray  a;
	parray  b;

	parray_init( &a, sizeof( char ), 0 );
	parray_init( &b, sizeof( char ), 0 );

	parray_push( &a, "a" );
	parray_push( &a, "b" );
	parray_push( &a, "c" );

	parray_push( &b, "a" );
	parray_push( &b, "d" );

	parray_iter( &a, dump );
	parray_iter( &b, dump );

	printf( "%ld\n", parray_union( &b, &a ) );

	parray_iter( &a, dump );
	parray_iter( &b, dump );
}
---
abc
ad
2
abc
adbc
*/

/** Tests the contents (data parts) of the array //left// and the array //right//
for equal elements.

The function returns a value < 0 if //left// is lower //right//, a value > 0
if //left// is greater //right// and a value == 0 if //left// is equal to
//right//. */
int parray_diff( parray* left, parray* right )
{
	int		diff;
	char*   p;
	char*   q;

	PROC( "parray_diff" );
	PARMS( "left", "%p", left );
	PARMS( "right", "%p", right );

	if( !( left && right
			&& left->size == right->size
			&& left->comparefn == right->comparefn ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	MSG( "Checking for same element count" );
	if( parray_count( right ) < parray_count( left ) )
		RETURN( 1 );
	else if( parray_count( right ) > parray_count( left ) )
		RETURN( -1 );

	MSG( "OK, requiring deep check" );

	for( p = left->bottom, q = right->bottom;
			p < left->top && q < right->top;
				p += left->size, q += right->size )
	{
		if( ( diff = parray_compare( left, p, q ) ) )
		{
			MSG( "Elements are not equal" );
			break;
		}
	}

	VARS( "diff", "%d", diff );
	RETURN( diff );
}

/*TESTCASE:parray_diff
#include <phorward.h>

void dump( parray* a, void* p )
{
	printf( "%c%s", *((char*)p), p == parray_last( a ) ? "\n" : "" );
}

void testcase()
{
	parray  a;
	parray  b;

	parray_init( &a, sizeof( char ), 0 );
	parray_init( &b, sizeof( char ), 0 );

	parray_push( &a, "a" );
	parray_push( &a, "b" );

	parray_push( &b, "a" );
	parray_push( &b, "b" );

	parray_iter( &a, dump );
	parray_iter( &b, dump );

	printf( "%d\n", parray_diff( &a, &b ) );

	parray_push( &b, "c" );

	printf( "%d\n", parray_diff( &a, &b ) );

	parray_push( &a, "c" );

	printf( "%d\n", parray_diff( &a, &b ) );

	parray_shift( &b );

	parray_iter( &a, dump );
	parray_iter( &b, dump );

	printf( "%d\n", parray_diff( &a, &b ) );

	parray_shift( &a );
	parray_pop( &b );

	parray_iter( &a, dump );
	parray_iter( &b, dump );

	printf( "%d\n", parray_diff( &a, &b ) );
	printf( "%d\n", parray_diff( &b, &a ) );
}
---
ab
ab
0
-1
0
abc
bc
1
bc
b
1
-1
*/

/** Sorts //array// between the elements //from// and //to// according to the
sort-function that was set for the list.

To sort the entire array, use parray_sort().

The sort-function can be modified by using parray_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
pboolean parray_subsort( parray* array, size_t from, size_t to )
{
	size_t	a	    = from;
	size_t	b	    = to;
	size_t  ref;

	if( !( array ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( from == to )
		return TRUE;

	if( from > to )
	{
		ref = from;
		from = to;
		to = ref;
	}

	a = ref = from;

	do
	{
		while( ( *array->sortfn )(
			array, parray_get( array, a ), parray_get( array, ref ) )
				> 0 )
			a++;

		while( ( *array->sortfn )(
			array, parray_get( array, ref ), parray_get( array, b ) )
				> 0 )
			b--;

		if( a <= b )
		{
			parray_swap( array, a, b );
			a++;

			if( b )
				b--;
		}
	}
	while( a <= b );

	if( ( b != from ) && ( b != from - 1 ) )
		parray_subsort( array, from, b );

	if( ( a != to ) && ( a != to + 1 ) )
		parray_subsort( array, a, to );

	return TRUE;
}

/** Sorts //list// according to the sort-function that was set for the list.

To sort only parts of a list, use plist_subsort().

The sort-function can be modified by using plist_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
pboolean parray_sort( parray* array )
{
	if( !( array ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	if( !parray_first( array ) )
		return TRUE;

	return parray_subsort( array, array->first, array->last - 1 );
}

/*TESTCASE:parray_sort
#include <phorward.h>

void dump( parray* a, void* p )
{
	printf( "%c%s", *((char*)p), p == parray_last( a ) ? "\n" : "" );
}

int sort( parray* a, void* p, void* q )
{
	int ret = toupper( *((char*)q) ) - toupper( *((char*)p) );

	if( ret == 0 )
		return *((char*)q) - *((char*)p);

	return ret;
}

void testcase()
{
	parray  a;
	parray  b;

	parray_init( &a, sizeof( char ), 0 );
	parray_init( &b, sizeof( char ), 0 );

	parray_push( &a, "c" );
	parray_push( &a, "d" );
	parray_push( &a, "a" );
	parray_push( &a, "b" );
	parray_push( &a, "b" );
	parray_push( &a, "B" );
	parray_push( &a, "k" );
	parray_push( &a, "e" );
	parray_push( &a, "A" );
	parray_push( &a, "x" );

	parray_concat( &b, &a );

	parray_iter( &a, dump );
	parray_sort( &a );
	parray_iter( &a, dump );

	parray_set_sortfn( &b, sort );

	parray_iter( &b, dump );
	parray_sort( &b );
	parray_iter( &b, dump );
}
---
cdabbBkeAx
ABabbcdekx
cdabbBkeAx
AaBbbcdekx
*/

/** Sets array compare function.

If no compare function is set or NULL is provided, memcmp() will be used
as default fallback. */
pboolean parray_set_comparefn( parray* array,
			int (*comparefn)( parray*, void*, void* ) )
{
	PROC( "parray_set_comparefn" );
	PARMS( "array", "%p", array );
	PARMS( "compare_fn", "%p", comparefn );

	if( !( array ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !( array->comparefn = comparefn ) )
		array->comparefn = parray_compare;

	RETURN( TRUE );
}

/** Sets array sort function.

If no sort function is given, the compare function set by parray_set_comparefn()
is used. If even unset, memcmp() will be used. */
pboolean parray_set_sortfn( parray* array,
			int (*sortfn)( parray*, void*, void* ) )
{
	PROC( "parray_set_sortfn" );
	PARMS( "array", "%p", array );
	PARMS( "sortfn", "%p", sortfn );

	if( !( array ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !( array->sortfn = sortfn ) )
		array->sortfn = parray_compare;

	RETURN( TRUE );
}

