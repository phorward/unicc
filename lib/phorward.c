/* -MODULE----------------------------------------------------------------------
Phorward C/C++ Library
Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer
https://phorward.info ++ contact<at>phorward<dash>software<dot>com
All rights reserved. See LICENSE for more information.

File:	phorward.c
Usage:	Phorward C/C++ library; Merged by standalone.sh on 2019-10-15
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
the array is freed again. The array's elements may change their heap address
when a chunk reallocation is required.
*/
void parray_init( parray* array, size_t size, size_t chunk )
{
	if( !chunk )
		chunk = PARRAY_DEFAULT_CHUNK;

	memset( array, 0, sizeof( parray ) );
	array->size = size;
	array->chunk = chunk;

	array->sortfn = parray_compare;
}

/** Create a new parray as an object with an element allocation size //size//,
a reallocation-chunk-size of //chunk//.

The returned memory must be released with parray_free().  */
parray* parray_create( size_t size, size_t chunk )
{
	parray*	array;

	array = (parray*)pmalloc( sizeof( parray ) );
	parray_init( array, size, chunk );

	return array;
}

/** Erase a dynamic array.

The array must not be reinitialized after destruction, using parray_init().

//array// is the pointer to the array to be erased. */
void parray_erase( parray* array )
{
	array->bottom = array->top = array->start = pfree( array->start );
	array->count = array->first = array->last = 0;
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
with a macro. It can be left NULL, so no memory will be copied.

The function returns the address of the newly pushed item, and NULL if
the item could not be pushed.
*/
void* parray_push( parray* array, void* item )
{
	/* Is reallocation required? */
	if( !array->count || array->last == array->count )
	{
		array->count += array->chunk;

		if( !( array->start = (void*)prealloc(
				(void*)array->start, array->count * array->size ) ) )
			return NULL;
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
void* parray_reserve( parray* array, size_t n )
{
	if( array->last + n < array->count )
		return array->bottom + n;

	array->count += n + ( n % array->chunk );

	if( !( array->start = (void*)prealloc(
			(void*)array->start, array->count * array->size ) ) )
		return NULL;

	array->bottom = array->start + array->first * array->size;
	array->top = array->start + array->last * array->size;

	return array->bottom + n;
}

/** Pushes and "allocates" an empty element on the array.

This function is just a shortcut to ```parray_push( array, NULL )```,
and the memory of the pushed element is initialized to zero. */
void* parray_malloc( parray* array )
{
	void*	ptr;

	if( !( ptr = parray_push( array, NULL ) ) )
		return ptr;

	memset( ptr, 0, array->size );
	return ptr;
}

/** Unshifts and "allocates" an empty element on the array.

This function is just a shortcut to ```parray_unshift( array, NULL )```,
and the memory of the unshifted element is initialized to zero. */
void* parray_rmalloc( parray* array )
{
	void*	ptr;

	if( !( ptr = parray_unshift( array, NULL ) ) )
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

	/* Within current bounds? */
	if( array->first + offset < array->last )
	{
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

		return slot;
	}

	while( array->first + offset >= array->last )
		if( !( slot = parray_malloc( array ) ) )
			return slot;

	if( item )
		memcpy( slot, item, array->size );

	return slot;
}

/** Remove item on //offset// from array //array//.

The removed item will be copied into //item//, if //item// is not NULL.
The function returns the memory of the removed item (it will contain the
moved up data part or invalid memory, if on the end). */
void* parray_remove( parray* array, size_t offset, void** item )
{
	void*	slot;

	/* Within current bounds? */
	if( array->first + offset >= array->last )
		return NULL;

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

	return slot;
}

/** Removes an element from the end of an array.

The function returns the pointer of the popped item. Because dynamic arrays only
grow and no memory is freed, the returned data pointer is still valid, and will
only be overridden with the next push operation.

//array// is the pointer to array where to pop an item off.

The function returns the address of the popped item, and NULL if the
item could not be popped (e.g. array is empty).
*/
void* parray_pop( parray* array )
{
	if( array->last == array->first )
		/* last is zero, no items on the array */
		return NULL;

	array->top -= array->size;

	return array->start + ( ( --array->last ) * array->size );
}

/** Appends an element to the begin of the array.

The elements memory is copied during the unshift. The item must be of the same
memory size as used at array initialization.

//array// is the pointer to array where to push an item to the beginning.

//item// is the pointer to the memory of the item that should be pushed onto the
array. The caller should cast his type into void, or wrap the push-operation
with a macro. It can be left NULL, so no memory will be copied.

The function returns the address of the newly unshifted item, and NULL
if the item could not be unshifted.
*/
void* parray_unshift( parray* array, void* item )
{
	/* Is reallocation required? */
	if( !array->count || array->first == 0 )
	{
		array->count += array->chunk;

		if( !( array->start = (void*)prealloc(
				(void*)array->start, array->count * array->size ) ) )
			return NULL;

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

	return array->bottom;
}

/** Removes an element from the begin of an array.

The function returns the pointer of the shifted item.
Because dynamic arrays only grow and no memory is freed, the returned data
pointer is still valid, and will only be overridden with the next unshift
operation.

//array// is the pointer to array where to pop an item off.

The function returns the address of the shifted item, and NULL if the
item could not be popped (e.g. array is empty).
*/
void* parray_shift( parray* array )
{
	if( array->last == array->first )
		/* last is zero, no items on the array. */
		return NULL;

	array->bottom += array->size;
	return array->start + array->first++ * array->size;
}

/** Access an element from the array by its offset position from the left.

//array// is the pointer to array where to access the element from.
//offset// is the offset of the element to be accessed from the array's
base address.

Returns the address of the accessed item, and NULL if the item could not
be accessed (e.g. if the array is empty or offset is beyond the top of array).

Use parray_rget() for access items from the end.
*/
void* parray_get( parray* array, size_t offset )
{
	if( array->last == array->first
			|| offset >= ( array->last - array->first ) )
		/* offset defines an item that is out of bounds of the array */
		return NULL;

	return array->start + ( array->first + offset ) * array->size;
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

	if( array->last == array->first
			|| offset >= ( array->last - array->first ) )
		/* offset defines an item that is out of bounds of the array */
		return NULL;

	slot = array->start + ( array->first + offset ) * array->size;

	if( item )
		memcpy( slot, item, array->size );
	else
		memset( slot, 0, array->size );

	return slot;
}

/** Access an element from the array by its offset position from the right.

//array// is the pointer to array where to access the element from.
//offset// is the offset of the element to be accessed from the array's
base address.

Returns the address of the accessed item, and NULL if the item could not
be accessed (e.g. if the array is empty or offset is beyond the bottom of
the array).

Use parray_get() to access items from the begin.
*/
void* parray_rget( parray* array, size_t offset )
{
	return parray_get( array, array->last - offset );
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
	return parray_put( array, array->last - offset, item );
}

/** Iterates over //array//.

Iterates over all items of //array// and calls the function //callback// on
every item. */
void parray_iter( parray* array, parrayfn callback )
{
	void*	ptr;

	parray_for( array, ptr )
		(*callback)( array, ptr );
}

/** Iterates backwards over //array//.

Backwardly iterates over all items of //array// and calls the function
//callback// on every item. */
void parray_riter( parray* array, parrayfn callback )
{
	void*	ptr;

	for( ptr = parray_last( array ); ptr; ptr = parray_prev( array, ptr ) )
		(*callback)( array, ptr );
}

/** Access first element of the array.

Returns the address of the accessed item, and NULL if nothing is in
the array.
*/
void* parray_first( parray* array )
{
	if( array->first == array->last )
		return NULL;

	return array->bottom;
}

/** Access last element of the array.

Returns the address of the accessed item, and NULL if nothing is in
the array.
*/
void* parray_last( parray* array )
{
	if( array->first == array->last )
		return NULL;

	return array->top - array->size;
}

/** Access next element from //ptr// in //array//.

Returns the address of the next element, and NULL if the access gets
out of bounds.
*/
void* parray_next( parray* array, void* ptr )
{
	ptr += array->size;
	if( ptr > parray_last( array ) || ptr < parray_first( array ) )
		return NULL;

	return ptr;
}

/** Access previous element from //ptr// in //array//.

Returns the address of the previous element, and NULL if the access gets
out of bounds.
*/
void* parray_prev( parray* array, void* ptr )
{
	ptr -= array->size;
	if( ptr < parray_first( array ) || ptr > parray_last( array ) )
		return NULL;

	return ptr;
}

/** Swap two elements of an array. */
void* parray_swap( parray* array, size_t pos1, size_t pos2 )
{
	void*	ptr1;
	void*	ptr2;

	if( !( ( ptr1 = parray_get( array, pos1 ) )
			&& ( ptr2 = parray_get( array, pos2 ) ) ) )
		return NULL;

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
void* parray_partof( parray* array, void* ptr )
{
	if( ptr >= (void*)array->bottom && ptr <= (void*)array->top )
		return ptr;

	return NULL;
}

/** Return offset of element //ptr// in array //array//.

Returns the offset of //ptr// in //array//.
The function returns the size of the array (which is an invalid offset)
if //ptr// is not part of //array//.

To check if a pointer belongs to an array, call parray_partof(). */
size_t parray_offset( parray* array, void* ptr )
{
	if( !parray_partof( array, ptr ) )
		return parray_count( array );

	return ( (char*)ptr - ( array->start + ( array->first * array->size ) ) )
				/ array->size;
}

/*TESTCASE:parray_(create|free|push|pop|shift|insert|unshift|remove)


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

	if( dest->size != src->size )
		return 0;

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

	if( !( all->size == from->size && all->comparefn == from->comparefn ) )
		return 0;

	if( !( last = all->last ) )
		return parray_concat( all, from );

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

	return parray_count( all ) - last;
}

/*TESTCASE:parray_union


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


	if( !( left->size == right->size && left->comparefn == right->comparefn ) )
		return -1;

	/* Checking for same element count */
	if( parray_count( right ) < parray_count( left ) )
		return 1;
	else if( parray_count( right ) > parray_count( left ) )
		return -1;

	/* OK, requiring deep check */
	for( p = left->bottom, q = right->bottom;
			p < left->top && q < right->top;
				p += left->size, q += right->size )
	{
		if( ( diff = parray_compare( left, p, q ) ) )
			break;
	}

	return diff;
}

/*TESTCASE:parray_diff


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
sort-function that was set for the array.

To sort the entire array, use parray_sort().

The sort-function can be modified by using parray_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
void parray_subsort( parray* array, size_t from, size_t to )
{
	size_t	a	    = from;
	size_t	b	    = to;
	size_t  ref;

	if( from == to )
		return;

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
}

/** Sorts //list// according to the sort-function that was set for the list.

To sort only parts of a list, use plist_subsort().

The sort-function can be modified by using plist_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
void parray_sort( parray* array )
{
	if( !parray_first( array ) )
		return;

	parray_subsort( array, array->first, array->last - 1 );
}

/*TESTCASE:parray_sort


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
void parray_set_comparefn( parray* array,
			int (*comparefn)( parray*, void*, void* ) )
{
	if( !( array->comparefn = comparefn ) )
		array->comparefn = parray_compare;
}

/** Sets array sort function.

If no sort function is given, the compare function set by parray_set_comparefn()
is used. If even unset, memcmp() will be used. */
void parray_set_sortfn( parray* array,
			int (*sortfn)( parray*, void*, void* ) )
{
	if( !( array->sortfn = sortfn ) )
		array->sortfn = parray_compare;
}


/* Prototype */
static pboolean pccl_ADDRANGE( pccl* ccl, wchar_t begin, wchar_t end );

/* Sort-function required for quick sort */
static int ccl_SORTFUNC( parray* arr, void* a, void* b )
{
	return ((pcrange*)b)->begin - ((pcrange*)a)->begin;
}

/** Constructor function to create a new character-class.

//min// and //max// can either be specified as -1, so the configured default
constants PCCL_MIN and PCCL_MAX will be used. The values can also be inverted.

Returns a pointer to the newly created character-class. This pointer should be
released with pccl_free() when its existence is no longer required.
*/
pccl* pccl_create( int min, int max, char* ccldef )
{
	pccl*	ccl;

	if( min < 0 )
		min = PCCL_MIN;
	if( max < 0 )
		max = PCCL_MAX;

	ccl = (pccl*)pmalloc( sizeof( pccl ) );

	parray_init( &ccl->ranges, sizeof( pcrange ), 0 );
	parray_set_sortfn( &ccl->ranges, ccl_SORTFUNC );

	if( min > max )
	{
		ccl->min = max;
		ccl->max = min;
	}
	else
	{
		ccl->min = min;
		ccl->max = max;
	}

	if( ccldef )
		pccl_parse( ccl, ccldef, FALSE );

	return ccl;
}

/** Checks if the character-classes //l// and //r// are configured
to be in the same character universe and compatible for operations. */
pboolean pccl_compat( pccl* l, pccl* r )
{
	if( !( l && r ) )
		return FALSE;

	if( l->min != r->min && l->max != r->max )
		return FALSE;

	return TRUE;
}

/** Returns the number of range pairs within a character-class.

//ccl// is a pointer to the character-class to be processed.

To retrieve the number of characters in a character-class, use
pccl_count() instead.

Returns the number of pairs the charclass holds.
*/
size_t pccl_size( pccl* ccl )
{
	return parray_count( &ccl->ranges );
}

/** Returns the number of characters within a character-class.

//ccl// is a pointer to the character-class to be processed.

Returns the total number of characters the class is holding.
*/
size_t pccl_count( pccl* ccl )
{
	pcrange*	cr;
	size_t		cnt	= 0;

	if( !ccl )
		return 0;

	parray_for( &ccl->ranges, cr )
		cnt += ( cr->end - cr->begin ) + 1;

	return cnt;
}

/** Duplicates a character-class into a new one.

//ccl// is the pointer to the character-class to be duplicated.

Returns a pointer to the duplicate of //ccl//, or (pcrange)NULL
in error case.
*/
pccl* pccl_dup( pccl* ccl )
{
	pccl* 		dup;
	pcrange*	cr;

	/* Create new, empty ccl */
	dup = pccl_create( ccl->min, ccl->max, (char*)NULL );

	parray_reserve( &dup->ranges, parray_count( &ccl->ranges ) );

	/* Copy elements */
	parray_for( &ccl->ranges, cr )
		pccl_ADDRANGE( dup, cr->begin, cr->end );

	return dup;
}

/* Normalizes a pre-parsed or modified character-class.

Normalization means, that duplicate elements will be removed, the range pairs
are sorted and intersections are resolved. The result is a unique, normalized
character-class to be used for further operations.

//ccl// is the character-class to be normalized.

Returns the number of cycles used for normalization.
*/
static size_t pccl_normalize( pccl* ccl )
{
	pcrange*	l;
	pcrange*	r;
	size_t		count;
	size_t		oldcount	= 0;
	size_t		cycles		= 0;

	while( ( count = pccl_size( ccl ) ) != oldcount )
	{
		oldcount = count;

		/* First sort the character ranges */
		parray_sort( &ccl->ranges );

		/* Then, find intersections and... */
		parray_for( &ccl->ranges, l )
		{
			if( ( r = l + 1 ) < (pcrange*)ccl->ranges.top )
			{
				if( r->begin <= l->end && r->end >= l->begin )
				{
					if( r->end > l->end )
						l->end = r->end;

					parray_remove( &ccl->ranges,
							parray_offset( &ccl->ranges, r ), NULL );
					break;
				}
				else if( l->end + 1 == r->begin )
				{
					l->end = r->end;
					parray_remove( &ccl->ranges,
							parray_offset( &ccl->ranges, r ), NULL );
					break;
				}
			}
		}

		cycles++;
	}

	return cycles;
}

/** Tests a character-class to match a character range.

//ccl// is a pointer to the character-class to be tested.
//begin// is the begin of character-range to be tested.
//end// is the end of character-range to be tested.

Returns TRUE if the entire character range matches the class, and FALSE if not.
*/
pboolean pccl_testrange( pccl* ccl, wchar_t begin, wchar_t end )
{
	pcrange*	cr;

	parray_for( &ccl->ranges, cr )
		if( begin >= cr->begin && end <= cr->end )
			return TRUE;

	return FALSE;
}

/** Tests a character-class if it contains a character.

//ccl// is the pointer to character-class to be tested.
//ch// is the character to be tested.

The function is a shortcut for pccl_testrange().

It returns TRUE, if the character matches the class, and FALSE if not.
*/
pboolean pccl_test( pccl* ccl, wchar_t ch )
{
	return pccl_testrange( ccl, ch, ch );
}

/** Tests for a character in case-insensitive-mode if it matches
a character-class.

//ccl// is the pointer to character-class to be tested.
//ch// is the character to be tested.

The function is a shortcut for pccl_testrange().

It returns TRUE, if the character matches the class, and FALSE if not.
*/
pboolean pccl_instest( pccl* ccl, wchar_t ch )
{
	if( pccl_test( ccl, ch ) )
		return TRUE;

#if UNICODE
	if( iswupper( ch ) )
		ch = towlower( ch );
	else
		ch = towupper( ch );
#else
	if( isupper( ch ) )
		ch = tolower( ch );
	else
		ch = toupper( ch );
#endif

	return pccl_test( ccl, ch );
}

/* Internal function without normalization */
static pboolean pccl_ADDRANGE( pccl* ccl, wchar_t begin, wchar_t end )
{
	pcrange		cr;

	if( begin > end )
	{
		cr.begin = end;
		cr.end = begin;
	}
	else
	{
		cr.begin = begin;
		cr.end = end;
	}

	if( cr.begin < ccl->min )
		cr.begin = ccl->min;

	if( cr.end > ccl->max )
		cr.end = ccl->max;

	if( pccl_testrange( ccl, cr.begin, cr.end ) )
		/* Range already in character-class */
		return TRUE;

	if( cr.begin > ccl->max || cr.end < ccl->min )
		/* Character-range not in the universe of character-class */
		return FALSE;

	return parray_push( &ccl->ranges, &cr ) ? TRUE : FALSE;
}

/** Integrates a character range into a character-class.

//ccl// is the pointer to the character-class to be affected. If //ccl// is
provided as (pccl*)NULL, it will be created by the function.

//begin// is the begin of character range to be integrated.
//end// is the end of character range to be integrated.

If //begin// is greater than //end//, the values will be swapped.
*/
pboolean pccl_addrange( pccl* ccl, wchar_t begin, wchar_t end )
{
	if( !pccl_ADDRANGE( ccl, begin, end ) )
		return FALSE;

	pccl_normalize( ccl );
	return TRUE;
}


/** Integrates a single character into a character-class.

//ccl// is the pointer to the character-class to be affected.
//ch// is the character to be integrated.

The function is a shortcut for pccl_addrange().
*/
pboolean pccl_add( pccl* ccl, wchar_t ch )
{
	return pccl_addrange( ccl, ch, ch );
}

/** Removes a character range from a character-class.

//ccl// is the pointer to the character-class to be affected.
//begin// is the begin of character range to be removed.
//end// is the end of character range to be removed.
*/
pboolean pccl_delrange( pccl* ccl, wchar_t begin, wchar_t end )
{
	pcrange		d;
	pcrange*	r;

	if( begin > end )
	{
		d.begin = end;
		d.end = begin;
	}
	else
	{
		d.begin = begin;
		d.end = end;
	}

	/* Which elements do match? */
	do
	{
		parray_for( &ccl->ranges, r )
		{
			if( d.begin <= r->end && end >= r->begin )
			{
				/* Slitting required? */
				if( d.begin > r->begin && d.end < r->end )
				{
					/* Split current range */
					end = r->end;
					r->end = d.begin - 1;

					if( !pccl_addrange( ccl, d.end + 1, end ) )
						return FALSE;

					break;
				}
				/* Move end of current range */
				else if( d.begin > r->begin )
					r->end = d.begin - 1;
				/* Change begin of current range */
				else if( d.end < r->end )
					r->begin = d.end + 1;
				/* Remove entire range */
				else
				{
					parray_remove( &ccl->ranges,
							parray_offset( &ccl->ranges, r ), NULL );

					r = parray_first( &ccl->ranges );
					break;
				}
			}
		}
	}
	while( r );

	pccl_normalize( ccl );
	return TRUE;
}

/** Removes a character from a character-class.

//ccl// is the pointer to the character-class to be affected.
//ch// is the character to be removed from //ccl//.

The function is a shortcut for pccl_delrange().
*/
pboolean pccl_del( pccl* ccl, wchar_t ch )
{
	return pccl_delrange( ccl, ch, ch );
}

/** Negates all ranges in a character-class.

//ccl// is the pointer to the character-class to be negated.

Returns a pointer to //ccl//.
*/
pccl* pccl_negate( pccl* ccl )
{
	wchar_t		start;
	wchar_t		end;
	pcrange*	r;

	start = end = ccl->min;

	do
	{
		parray_for( &ccl->ranges, r )
		{
			if( end < r->begin )
			{
				start = r->begin;
				r->begin = end;

				end = r->end + 1;
				r->end = start - 1;
			}
			else
			{
				end = r->end + 1;
				parray_remove( &ccl->ranges,
						parray_offset( &ccl->ranges, r ), NULL );
				break;
			}
		}
	}
	while( r );

	if( end < ccl->max )
		pccl_addrange( ccl, end, ccl->max );

	pccl_normalize( ccl );

	return ccl;
}

/** Creates the union of two character-classes and returns the newly created,
normalized character-class.

//ccl// is the pointer to the character-class that will be extended to all
ranges contained in //add//. //add// is the character-class that will be joined
with //ccl//.

The function creates and returns a new character-class that is the union
of //ccl// and //add//.
*/
pccl* pccl_union( pccl* ccl, pccl* add )
{
	pccl*		un;
	pcrange*	r;

	if( !pccl_compat( ccl, add ) )
		/* Incompatible character-classes */
		return NULL;

	un = pccl_dup( ccl );

	parray_for( &add->ranges, r )
		if( !pccl_ADDRANGE( un, r->begin, r->end ) )
			return NULL;

	pccl_normalize( un );

	return un;
}


/** Returns the difference quantity of two character-classes.
All elements from //rem// will be removed from //ccl//, and put into a
new character-class.

//ccl// is the pointer to the first character-class.
//rem// is the pointer to the second character-class.

Returns a new pointer to a copy of //ccl//, without the ranges contained in
//rem//. Returns (pccl*)NULL in case of memory allocation or parameter
error.
*/
pccl* pccl_diff( pccl* ccl, pccl* rem )
{
	pcrange*	r;
	pccl*		diff;

	if( !pccl_compat( ccl, rem ) )
		/* Incompatible character-classes */
		return NULL;

	if( !( diff = pccl_dup( ccl ) ) )
		return diff;

	parray_for( &rem->ranges, r )
		pccl_delrange( diff, r->begin, r->end );

	return diff;
}

/** Checks for differences in two character-classes.

//left// is the pointer to the first character-class.
//right// is the pointer to the second character-class.

Returns a value < 0 if //left// is lower than //right//, 0 if //left// is
equal to //right// or a value > 0 if //left// is greater than //right//.
*/
int pccl_compare( pccl* left, pccl* right )
{
	size_t	ret;

	if( !pccl_compat( left, right ) )
		/* Incompatible character-classes */
		return left->max - right->max;

	if( ( ret = pccl_size( left ) - pccl_size( right ) ) != 0 )
		/* Unequal number of range pairs */
		return ret < 0 ? -1 : 1;

	return parray_diff( &left->ranges, &right->ranges );
}

/** Returns a new character-class with all characters that exist in both
provided character-classes.

//ccl// is the pointer to the first character-class.
//within// is the pointer to the second character-class.

Returns a new character-class containing the intersection of //ccl//
and //within//. If there is no intersection between both character-classes,
the function returns (pccl*)NULL.
*/
pccl* pccl_intersect( pccl* ccl, pccl* within )
{
	pcrange*	r;
	pcrange*	s;
	pccl*		in	= (pccl*)NULL;

	if( !pccl_compat( ccl, within ) )
		/* Incompatible character-classes */
		return NULL;

	parray_for( &ccl->ranges, r )
	{
		parray_for( &within->ranges, s )
		{
			if( s->begin <= r->end && s->end >= r->begin )
			{
				if( !in )
					in = pccl_create( ccl->min, ccl->max, (char*)NULL );

				pccl_addrange( in,
					( r->begin > s->begin ) ? r->begin : s->begin,
					( r->end > s->end ) ? s->end : r->end );
			}
		}
	}

	if( in )
		pccl_normalize( in );

	return in;
}

/** Return a character or a character-range by its offset.

If the function is called only with pointer //from// provided, and //to// as
(wchar_t*)NULL, it writes the character in //offset//th position of the
character-class into from.

If the function is called both with pointer //from// and //to// provided,
it writes the //begin// and //end// character of the character-range in the
//offset//th position of the character-class into //from// and //to//.

If no character or range with the given offset was found, the function
returns FALSE, meaning that the end of the characters is reached.
On success, the function will always return TRUE. */
pboolean pccl_get( wchar_t* from, wchar_t* to, pccl* ccl, size_t offset )
{
	pcrange*	cr;

	if( !to )
	{
		/* Single-character retrieval */

		parray_for( &ccl->ranges, cr )
		{
			if( offset >= ( cr->end - cr->begin ) + 1 )
			{
				/* Offset not in this range */
				offset -= ( cr->end - cr->begin ) + 1;
			}
			else if( offset < ( cr->end - cr->begin ) + 1 )
			{
				/* Offset is within this class */
				*from = cr->begin + offset;
				return TRUE;
			}
		}
	}
	else
	{
		/* Range retrieval */

		if( ( cr = (pcrange*)parray_get( &ccl->ranges, offset ) ) )
		{
			*from = cr->begin;
			*to = cr->end;
			return TRUE;
		}
	}

	/* Offset not available */
	return FALSE;
}

/** Reads a character from a string. The character may consist of one single
character or it may be made up of an escape sequence or UTF-8 character.
The function returns the number of bytes read.

//retc// is the return pointer for the character code of the escaped string.
//str// is the begin pointer of the string at which character parsing begins.
If //escapeseq// is TRUE, the function regards escape sequences, else it ignores
them.

Returns the number of bytes that had been read for the character. */
size_t pccl_parsechar( wchar_t* retc, char *str, pboolean escapeseq )
{
	wchar_t	ch;
	char* 	esc;
    char 	digs[9]		=	"\0\0\0\0\0\0\0\0";
    int		dno 		= 0;
	char*	p			= str;

	if( escapeseq && *p == '\\' )
	{
		p++;

		if( *p >= 'a' &&  *p <= 'z'
			&& ( esc = strchr( "n\nt\tr\rb\bf\fv\va\a", *p ) ) )
		{
			ch = *( esc + 1 );
			p++;
		}
		else if( octal_digit( *p ) )
		{
			do
				digs[dno++] = *( p++ );
			while( octal_digit( *p ) && dno < 3 );

			ch = strtol( digs, (char**)NULL, 8 );
		}
		else if( *p == 'x' )
		{
			p++;
			while( hex_digit( *p ) && dno < 2 )
				digs[ dno++ ] = *( p++ );

			if (dno > 0)
				ch = strtol( digs, (char**)NULL, 16 );
		}
#ifdef UTF8
		else if( *p == 'u' )
		{
			p++;
			while( hex_digit( *p ) && dno < 4 )
				digs[dno++] = *( p++ );

			if( dno > 0 )
				ch = strtol( digs, (char**)NULL, 16 );
		}
		else if( *p == 'U' )
		{
			p++;
			while( hex_digit( *p ) && dno < 8 )
				digs[dno++] = *( p++ );

			if( dno > 0 )
				ch = strtol( digs, (char**)NULL, 16 );
		}
#endif
		else
		{
#ifdef UTF8
			ch = putf8_char( p );
			p += putf8_seqlen( p );
#else
			ch = *( p++ );
#endif
		}
	}
	else
	{
#ifdef UTF8
		ch = putf8_char( p );
		p += putf8_seqlen( p );
#else
		ch = *( p++ );
#endif
	}

	*retc = ch;

    return (size_t)( p - str );
}

/**  Tries to parse a shorthand sequence from a string. This matches the
shorthands \w, \W, \d, \D, \s and \S. If it matches, all characters are
added to //ccl//.

The function returns TRUE in case a shorthand has been parsed. If so,
the pointer //str// is moved the characters consumed.

If no shorthand sequence could be found, it returns FALSE, leaving //ccl//
untouched. */
pboolean pccl_parseshorthand( pccl* ccl, char** str )
{
	pccl*		sh;
	pboolean	neg	= FALSE;
	int			i;
	wchar_t		begin;
	wchar_t		end;

	/* Check for shorthand */
	if( **str != '\\' )
		return FALSE;

	switch( *(*str + 1) )
	{
		/* This solution is ugly and does not support any Unicode features.
			So it would be nice to find out a cooler solution in future. */
		case 'D':
			neg = TRUE;
		case 'd':
			sh = pccl_create( ccl->min, ccl->max, "0-9" );
			break;

		case 'W':
			neg = TRUE;
		case 'w':
			sh = pccl_create( ccl->min, ccl->max, "a-zA-Z_0-9" );
			break;

		case 'S':
			neg = TRUE;
		case 's':
			sh = pccl_create( ccl->min, ccl->max, " \f\n\r\t\v\a" );
			break;

		default:
			/* This is not a supported shorthand! */
			return FALSE;
	}

	if( neg )
		pccl_negate( sh );

	for( i = 0; pccl_get( &begin, &end, sh, i ); i++ )
		pccl_ADDRANGE( ccl, begin, end );

	pccl_free( sh );
	pccl_normalize( ccl );

	*str += 2;
	return TRUE;
}

/** Parses the character-class definition provided in //ccldef// and assigns
this definition to the character-class //ccl//.

If //ccl// is NULL, a new character-class with the PCCL_MIN/PCCL_MAX
configuration will be created.

//ccldef// may contain UTF-8 formatted input. Escape-sequences will be
 interpreted to their correct character representations.

A typical character-class definition simply exists of single characters and
range definitions. For example, "$A-Z#0-9" defines a character-class that
consists of the characters "$#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ".

The parameter //extend// specifies, if the provided character-class overwrites
(//extend// = FALSE) or extends (//extend// = TRUE) the provided
character-class. This means that definitions that already exist in the
character-class, should be erased first or not.

The function returns TRUE on success, and FALSE on an error.
*/
pccl* pccl_parse( pccl* ccl, char* ccldef, pboolean extend )
{
	pccl*		own;
	char*		cclptr;
	wchar_t		begin;
	wchar_t		end;

	if( !ccl )
		ccl = own = pccl_create( 0, 0, NULL );
	else if( !extend )
		pccl_erase( ccl );

	for( cclptr = ccldef; *cclptr; )
	{
		if( pccl_parseshorthand( ccl, &cclptr ) )
			continue;

		cclptr += pccl_parsechar( &begin, cclptr, TRUE );
		end = begin;

		/* Is this a range def? */
		if( *cclptr == '-' )
		{
			cclptr++;
			cclptr += pccl_parsechar( &end, cclptr, TRUE );
		}

		if( !pccl_ADDRANGE( ccl, begin, end ) )
		{
			if( own )
				pccl_free( own );

			return NULL;
		}
	}

	pccl_normalize( ccl );

	return ccl;
}

/** Erases a character-class //ccl//.

The function sets a character-class to zero, as it contains no character range
definitions. The object //ccl// will be still alive. To delete the entire
object, use pccl_free().
*/
void pccl_erase( pccl* ccl )
{
	parray_erase( &ccl->ranges );
	ccl->str = pfree( ccl->str );
}

/** Frees a character-class //ccl// and all its used memory.

The function always returns (pccl*)NULL.
*/
pccl* pccl_free( pccl* ccl )
{
	if( !ccl )
		return (pccl*)NULL;

	pccl_erase( ccl );
	pfree( ccl );

	return (pccl*)NULL;
}

/** Converts a character-class back to a string representation of the
character-class definition, which in turn can be converted back into a
character-class using pccl_create().

//ccl// is the pointer to character-class to be converted.
//escape//, if TRUE, escapes "unprintable" characters in their hexadecimal
representation. If FALSE, it prints all characters, except the zero, which will
be returned as "\0"

Returns a pointer to the generated string that represents the charclass.
The returned pointer belongs to the //ccl// and is managed by the
character-class handling functions, so it should not be freed manually.
*/
char* pccl_to_str( pccl* ccl, pboolean escape )
{
	size_t		len		= 1;
	pcrange*	r;

	ccl->str = pfree( ccl->str );

	/* Examine required size of the resulting string */
	parray_for( &ccl->ranges, r )
		len += ( ( r->begin != r->end ? 1 : 2 )			/* Two or one char? */
					* ( escape ? 10 : 6 ) ) 			/* Escaped or not? */
				+ ( ( r->begin != r->end ) ? 1 : 0 );	/* Range-dash? */

	if( !( ccl->str = (char*)pmalloc( len * sizeof( char ) ) ) )
		return NULL;

	*ccl->str = '\0';

	parray_for( &ccl->ranges, r )
	{
		if( escape )
			putf8_escape_wchar(
					ccl->str + strlen( ccl->str ),
						len - strlen( ccl->str ) - 1, r->begin );
		else
			putf8_toutf8( ccl->str + strlen( ccl->str ),
					len - strlen( ccl->str ) - 1,
						&( r->begin ), 1 );

		if( r->begin != r->end )
		{
			strcat( ccl->str, "-" );

			if( escape )
				putf8_escape_wchar(
						ccl->str + strlen( ccl->str ),
							len - strlen( ccl->str ) - 1, r->end );
			else
				putf8_toutf8( ccl->str + strlen( ccl->str ),
							  len - strlen( ccl->str ) - 1,
							  &( r->end ), 1 );
		}
	}

	return ccl->str ? ccl->str : "";
}

/** Print character-class to output stream.
This function is provided for debug-purposes only.

//stream// is the output stream to dump the character-class to; This can be
left (FILE*)NULL, so //stderr// will be used.
//ccl// is the pointer to character-class

//break_after// defines:
- if < 0 print with pointer info
- if 0 print all into one line
- if > 0 print linewise
-
*/
void pccl_print( FILE* stream, pccl* ccl, int break_after )
{
	pcrange*	r;
	int			cnt			= 0;
	char		outstr[ 2 ] [ 10 + 1 ];

	if( !( ccl ) )
		return;

	if( !stream )
		stream = stderr;

	if( break_after < 0 )
		fprintf( stream, "*** begin of ccl %p ***\n", ccl );

	parray_for( &ccl->ranges, r )
	{
		putf8_toutf8( outstr[0], sizeof( outstr[0] ), &( r->begin ), 1 );

		if( r->begin != r->end )
		{
			putf8_toutf8( outstr[1], sizeof( outstr[1] ), &( r->end ), 1 );
			fprintf( stream, "'%s' [%d] to '%s' [%d] ",
				outstr[0], (int)r->begin, outstr[1], (int)r->end );
		}
		else
			fprintf( stream, "'%s' [%d] ", outstr[0], (int)r->begin );

		if( break_after > 0 && cnt++ % break_after == 0 )
			fprintf( stream, "\n" );
	}

	if( break_after < 0 )
		fprintf( stream, "\n*** end of ccl %p ***\n", ccl );
}

#if 0
int main( int argc, char** argv )
{
	pccl*	c;
	pccl*	d;
	pccl*	e;

	c = pccl_create( 0, 255, "A-Za-z0-9" );
	d = pccl_create( 0, 255, "A-@." );

	/*pccl_addrange( c, 0, 10 );*/
	pccl_print( NULL, c, 1 );
	pccl_print( NULL, d, 1 );

	e = pccl_union( c, d );
	pccl_print( NULL, e, 1 );

	printf( "%s\n", pccl_to_str( e, TRUE ) );

	/*
	pccl_negate( c );
	pccl_delrange( c, 0, 255 );
	pccl_print( c );
	*/

	return 0;
}
#endif

/*TESTCASE


void testcase()
{
	pccl*		c;
	pccl*		d;
	pccl*		e;
	char* 		x;

	c = pccl_create( PCCL_MIN, PCCL_MAX, "^ €A-Z\n" );
	d = pccl_create( PCCL_MIN, PCCL_MAX, "A-NXYZ\n" );

	d = pccl_create( PCCL_MIN, PCCL_MAX,
					 "^alles richtig! :)" );
	pccl_print( stderr, c, 0 );
	pccl_print( stderr, d, 0 );

	e = pccl_union( c, d );
	pccl_print( stderr, e, 0 );

	pccl_negate( e );
	pccl_print( stderr, e, 0 );

	pccl_negate( e );
	pccl_print( stderr, e, 0 );

	pccl_delrange( d, '\0', PCCL_MAX );
	printf( "e = >%s<\n", pccl_to_str( d, TRUE ) );

	pccl_free( c );
	pccl_free( d );
	pccl_free( e );
}
---
todo
*/


/** This functions converts a wide-character string into an UTF-8 string.

The string conversion is performed into dynamically allocated memory.
The function wraps the system function wcstombs(), so set_locale() must be
called before this function works properly.

//str// is the zero-terminated string to be converted to UTF-8.
//freestr// defines if the input-string shall be freed after successful
conversion, if set to TRUE.

Returns the UTF-8 character pendant of //str// as pointer to dynamically
allocated memory.
*/
char* pwcs_to_str( wchar_t* str, pboolean freestr )
{
	size_t	size;
	char*	retstr;

	PROC( "pwcs_to_str" );
	PARMS( "str", "%ls", str );
	PARMS( "freestr", "%s", BOOLEAN_STR( freestr ) );

#ifdef UNICODE
	size = wcstombs( (char*)NULL, str, 0 );
	VARS( "size", "%ld", size );

	if( !( retstr = (char*)pmalloc( ( size + 1 ) * sizeof( char ) ) ) )
	{
		MSG( "Out of memory?" );
		RETURN( (char*)NULL );
	}

	wcstombs( retstr, str, size + 1 );

	if( freestr )
		pfree( str );
#else
	MSG( "Nothing to do, this function is compiled without UNICODE-flag!" );

	if( freestr )
		retstr = str;
	else
		retstr = pstrdup( str );
#endif

	VARS( "retstr", "%s", retstr );
	RETURN( retstr );
}

/** This functions converts an UTF-8-multi-byte string into a Unicode
wide-character string.

The function wraps mbstowcs(), so set_locale() must be done before this
function works properly.

//str// is the zero-terminated multi-byte-character string to be converted
into a wide-character string.
//freestr// if value equals TRUE then //str// will be freed after successfull
conversion.

Returns the wide-character pendant of //str// as pointer to dynamically
allocated memory.
*/
wchar_t* pstr_to_wcs( char* str, pboolean freestr )
{
	size_t	size;
	wchar_t*	retstr;

	PROC( "pstr_to_wcs" );
	PARMS( "str", "%s", str );
	PARMS( "freestr", "%s", BOOLEAN_STR( freestr ) );

#ifdef UNICODE
	size = mbstowcs( (wchar_t*)NULL, str, 0 );
	VARS( "size", "%ld", size );

	if( !( retstr = (wchar_t*)pmalloc( ( size + 1 ) * sizeof( wchar_t ) ) ) )
	{
		MSG( "Out of memory?" );
		RETURN( (wchar_t*)NULL );
	}

	mbstowcs( retstr, str, size + 1 ); /* JMM 23.09.2010 */

	if( freestr )
		pfree( str );
#else
	MSG( "Nothing to do, this function is compiled without UNICODE-flag!" );

	if( freestr )
		retstr = str;
	else
		retstr = pstrdup( str );
#endif

	VARS( "retstr", "%ls", retstr );
	RETURN( retstr );
}

/** Converts a double-value into an allocated string buffer.

//d// is the double value to become converted. Zero-digits behind the decimal
dot will be removed after conversion, so 1.65000 will become "1.65" in its
string representation.

Returns a pointer to the newly allocated string, which contains the
string-representation of the double value. This pointer must be released
by the caller.
*/
char* pdbl_to_str( double d )
{
	char*		trail;
	char		str		[ 128 ];

	PROC( "pdbl_to_str" );
	PARMS( "d", "%lf", d );

	sprintf( str, "%lf", d );
	VARS( "str", "%s", str );

	for( trail = str + strlen( str ) - 1;
			*trail == '0'; trail-- )
		;

	*( trail + 1 ) = '\0';

	VARS( "str", "%s", str );
	RETURN( pstrdup( str ) );
}

#ifdef UNICODE
/** Converts a double-value into an allocated wide-character string buffer.

//d// is the double value to become converted. Zero-digits behind the decimal
dot will be removed after conversion, so 1.65000 will become L"1.65" in its
wide-character string representation.

Returns a pointer to the newly allocated wide-character string, which contains
the string-representation of the double value. This pointer must be released
by the caller.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pdbl_to_wcs( double d )
{
	wchar_t*	trail;
	wchar_t		str		[ 128 ];

	PROC( "pdbl_to_wcs" );
	PARMS( "d", "%lf", d );

	#if _WIN32
	_snwprintf( str, sizeof( str ), L"%lf", d );
	#else
	swprintf( str, sizeof( str ), L"%lf", d );
	#endif

	VARS( "str", "%ls", str );

	for( trail = str + wcslen( str ) - 1;
			*trail == '0'; trail-- )
		;

	*( trail + 1 ) = '\0';

	VARS( "str", "%ls", str );
	RETURN( pwcsdup( str ) );
}
#endif


/*
	Trace is activated in any program if the DEBUG-macro is defined.
	A function which uses trace looks like the following:

	int func( int a, char* b )
	{
		int i;

		PROC( "func" );
		PARMS( "a", "%d", a );
		PARMS( "b", "%s", b );

		MSG( "Performing loop..." );
		for( i = 0; i < a; i++ )
		{
			VARS( "i", "%d", i );
			printf( "%c\n", b[ i ] );
		}

		MSG( "Ok, everything is fine! :)" );
		RETURN( i );
	}
*/

/** Write function entry to trace.

The PROC-macro introduces a new function level, if compiled with trace.

The PROC-macro must be put behind the last local variable declaration and the
first code line, else it won't compile. A PROC-macro must exists within a
function to allow for other trace-macro usages. If PROC() is used within a
function, the macros RETURN() or VOIDRET, according to the function return
value, must be used. If PROC is used without RETURN, the trace output will
output a wrong call level depth.

The parameter //func_name// is a static string for the function name.
*/
/*MACRO:PROC( char* func_name )*/

/** Write function return to trace.

RETURN() can only be used if PROC() is used at the beginning of the function.
For void-functions, use the macro VOIDRET.

//return_value// is return-value of the function.
*/
/*MACRO:RETURN( function_type return_value )*/

/** Write void function return to trace.

VOIDRET can only be used if PROC() is used at the beginning of the function.
For typed functions, use the macro RETURN().
*/
/*MACRO:VOIDRET*/

/** Write parameter content to trace.

The PARMS-macro is used to write parameter names and values to the program
trace. PARMS() should - by definition - only be used right behind PROC().
If the logging of variable values is wanted during a function execution to
trace, the VARS()-macro shall be used.

//param_name// is the name of the parameter
//format// is a printf-styled format placeholder.
//parameter// is the parameter itself.
*/
/*MACRO:PARMS( char* param_name, char* format, param_type parameter )*/

/** Write variable content to trace.

The VARS-macro is used to write variable names and values to the program trace.
For parameters taken to functions, the PARMS()-macro shall be used.

//var_name// is the name of the variable
//format// is a printf-styled format placeholder.
//variable// is the parameter itself.
*/
/*MACRO:VARS( char* var_name, char* format, var_type variable )*/

/** Write a message to trace.

//message// is your message!
*/
/*MACRO:MSG( char* message )*/

/** Write any logging output to trace.

This function is newer than the previous ones, and allows for a printf-like
format string with variable amount of parameters.

//format// is a printf()-like format-string.
//...// parameters in the way they occur in the format-string.
*/
/*MACRO:LOG( char* format, ... )*/

/*NO_DOC*/

static int 		_dbg_level;
static clock_t	_dbg_clock;

/** Print //indent// levels to //f//. */
static void _dbg_indent( void )
{
	int		i;
	char*	traceindent;

	if( ( traceindent = getenv( "TRACEINDENT" ) ) )
	{
		if( strcasecmp( traceindent, "ON" ) != 0 && !atoi( traceindent ) )
			return;
	}

	for( i = 0; i < _dbg_level; i++ )
		fprintf( stderr, "." );
}

/** Check if debug output is wanted. */
pboolean _dbg_trace_enabled( char* file, char* function, char* type )
{
	char*		modules;
	char*		functions;
	char*		types;
	char*		basename;
	int			maxdepth;

	if( !( basename = strrchr( file, PPATHSEP ) ) )
		basename = file;
	else
		basename++;

	/* Find out if this module should be traced */
	if( ( modules = getenv( "TRACEMODULE" ) ) )
	{
		if( strcmp( modules, "*" ) != 0
			&& !strstr( modules, basename ) )
			return FALSE;
	}

	/* Find out if this function should be traced */
	if( ( functions = getenv( "TRACEFUNCTION" ) ) )
	{
		if( strcmp( functions, "*" ) != 0
			&& !strstr( functions, function ) )
			return FALSE;
	}

	/* Find out if this type should be traced */
	if( ( types = getenv( "TRACETYPE" ) ) )
	{
		if( strcmp( types, "*" ) != 0
			&& !strstr( types, type ) )
			return FALSE;
	}

	/* One of both configs must be present! */
	if( !modules && !functions )
		return FALSE;

	if( ( maxdepth = atoi( pstrget( getenv( "TRACEDEPTH" ) ) ) ) > 0
			&& _dbg_level > maxdepth )
		return FALSE;

	return TRUE;
}

/** Output trace message to the error log.

//file// is the filename (__FILE__).
//line// is the line number in file (__LINE__).
//type// is the type of the trace message ("PROC", "RETURN", "VARS" ...).
//function// is the function name that is currently executed
//format// is an optional printf()-like format string.
//...// values according to the format string follow.
*/
void _dbg_trace( char* file, int line, char* type,
					char* function, char* format, ... )
{
	va_list		arg;
	time_t		now;

	/* Check if trace is enabled */
	if( !_dbg_trace_enabled( file, function, type ) )
		return;

	if( type && strcmp( type, "ENTRY" ) == 0 )
		_dbg_level++;

	now = clock();
	if( !_dbg_clock )
		_dbg_clock = now;

	fprintf( stderr, "(%-20s:%5d %lf) ", file, line,
		( (double)( now - _dbg_clock ) / CLOCKS_PER_SEC ) );

	_dbg_clock = now;

	_dbg_indent();
	fprintf( stderr, "%-8s", type ? type : "" );

	if( format && *format )
	{
		va_start( arg, format );
		fprintf( stderr, ": " );
		vfprintf( stderr, format, arg );
		va_end( arg );
	}
	else
		fprintf( stderr, ": %s", function );

	fprintf( stderr, "\n" );

	if( type && strcmp( type, "RETURN" ) == 0 )
		_dbg_level--;

	fflush( stderr );
}

/*COD_ON*/


/*
The plist-object implements

- a double linked-list
- hashable entries
- dynamic stack functionalities
- data object collections
- set functions

into one handy library. It can be used for many powerful tasks, including
symbol tables, functions relating to set theories or as associative arrays.

It serves as the replacement for the older libphorward data structures
LIST (llist), HASHTAB and STACK, which had been widely used and provided in the
past. All usages of them had been substituted by plist now.

The object parray can be used for real stacks for a better runtime performance,
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

	l = (float)plist_count( list ) / (float)list->hashsize;
	load = (int)( l * 100 );

	return load;
}

/* Compare hash-table elements */
static int plist_hash_compare( plist* list, char* l, char* r, size_t n )
{
	int		res;

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

	if( !e->key )
		return FALSE;

	e->hashnext = (plistel*)NULL;
	e->hashprev = (plistel*)NULL;

	if( ! plist_get_by_key( list, e->key ) )
	{
		/* new element, check if we have to resize the map */

		/* check load factor */
		list->load_factor = plist_get_load_factor( list );

		if( list->load_factor > LOAD_FACTOR_HIGH )
		{
			/* hashmap has to be resized. */
			if( !plist_hash_rebuild( list ) )
				return FALSE;

			/* store new load factor */
			list->load_factor = plist_get_load_factor( list );

			return TRUE; /* e has been inserted by plist_hash_rebuild()! */
		}
	}

	bucket = &( list->hash[ plist_hash_index( list, e->key, 0 ) ] );

	if( ! *bucket )
	{
		/* Bucket is empty, chaining start position */
		*bucket = e;
		list->free_hash_entries--;
	}
	else
	{
		/* Chaining into hashed bucket */

		for( he = *bucket; he; he = he->hashnext )
		{
			if( plist_hash_compare( list, he->key, e->key, 0 ) == 0 )
			{
				if( list->flags & PLIST_MOD_UNIQUE )
					return FALSE;

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

	return TRUE;
}

/* Rebuild hash-table */
static pboolean plist_hash_rebuild( plist* list )
{
	plistel*	e;

	if( list->size_index + 1 >=
			( sizeof( table_sizes ) / sizeof( *table_sizes ) ) )
		return FALSE; /* Maximum size is reached. */

	if( list->hash )
		list->hash = pfree( list->hash );

	for( e = plist_first( list ); e; e = plist_next( e ) )
		e->hashnext = (plistel*)NULL;

	list->size_index++;
	list->hashsize = table_sizes[ list->size_index ];

	list->hash_collisions = 0;

	list->hash = (plistel**)pmalloc( list->hashsize * sizeof( plistel* ) );

	for( e = plist_first( list ); e; e = plist_next( e ) )
		plist_hash_insert( list, e );

	return TRUE;
}

/* Drop list element */
static void plistel_drop( plistel* e )
{
	/* TODO: Call element destructor? */
	if( !( e->flags & PLIST_MOD_EXTKEYS )
			&& !( e->flags & PLIST_MOD_PTRKEYS ) )
		e->key = pfree( e->key );
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
void plist_init( plist* list, size_t size, short flags )
{
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
void plist_erase( plist* list )
{
	plistel*	e;
	plistel*	next;

	/* Freeing current list contents */
	for( e = list->first; e; e = next )
	{
		next = e->next;

		plistel_drop( e );
		pfree( e );
	}

	/* Freeing list of unused nodes */
	for( e = list->unused; e; e = next )
	{
		next = e->next;
		pfree( e );
	}

	/* Resetting hash table */
	if( list->hash )
		pfree( list->hash );

	/* Resetting list-object pointers */
	list->first = (plistel*)NULL;
	list->last = (plistel*)NULL;
	list->hash = (plistel**)NULL;
	list->unused = (plistel*)NULL;
	list->count = 0;
}

/** Clear content of the list //list//.

The function has nearly the same purpose as plist_erase(), except that
the entire list is only cleared, but if the list was initialized with
PLIST_MOD_RECYCLE, existing pointers are held for later usage. */
void plist_clear( plist* list )
{
	while( list->first )
		plist_remove( list, list->first );
}

/** Releases all the memory //list// uses and destroys the list object.

The function always returns (plist*)NULL. */
plist* plist_free( plist* list )
{
	if( !list )
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

	/* Rebuild hash-table if necessary */
	if( key && !list->hash && !plist_hash_rebuild( list ) )
		return (plistel*)NULL;

	/* Recycle existing elements? */
	if( list->unused )
	{
		/* Recycle list contains element, will recycle this now */
		e = list->unused;
		list->unused = e->next;
		list->recycled--;
	}
	else
	{
		/* No elements to recycle, allocating a new one */
		e = (plistel*)pmalloc( sizeof( plistel ) + list->size );
	}

	memset( e, 0, sizeof( plistel ) + list->size );

	e->flags = list->flags;

	if( src )
	{
		/* data is provided, copy memory */

		if( list->flags & PLIST_MOD_PTR )
			*( (void**)( e + 1 ) ) = src; /* Pointer mode: just store pointer */
		else
			memcpy( e + 1, src, list->size ); /* Copy memory into element */
	}

	if( !pos )
	{
		/* pos unset, will chain at end of list */
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
		/* Key provided, will insert into hash table */

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
			/* Item collides! */
			plist_remove( list, e );
			return (plistel*)NULL;
		}
	}

	if( list->flags & PLIST_MOD_AUTOSORT )
		plist_sort( list );

	return e;
}

/** Push //src// to end of //list//.

Like //list// would be a stack, //src// is pushed at the end of the list.
This function can only be used for linked lists without the hash-table feature
in use. */
plistel* plist_push( plist* list, void* src )
{
	return plist_insert( list, (plistel*)NULL, (char*)NULL, src );
}

/** Shift //src// at begin of //list//.

Like //list// would be a queue, //src// is shifted at the beginning of the list.
This function can only be used for linked lists without the hash-table feature
in use. */
plistel* plist_shift( plist* list, void* src )
{
	return plist_insert( list, plist_first( list ), (char*)NULL, src );
}

/** Allocates memory for a new element in list //list//, push it to the end and
return the pointer to this.

The function works as a shortcut for plist_access() in combination with
plist_push().
*/
void* plist_malloc( plist* list )
{
	return plist_access( plist_push( list, (void*)NULL ) );
}

/** Allocates memory for a new element in list //list//, shift it at the begin
and return the pointer to this.

The function works as a shortcut for plist_access() in combination with
plist_shift().
*/
void* plist_rmalloc( plist* list )
{
	return plist_access( plist_shift( list, (void*)NULL ) );
}

/** Removes the element //e// from the //list// and frees it or puts
 it into the unused element chain if PLIST_MOD_RECYCLE is flagged. */
void plist_remove( plist* list, plistel* e )
{
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
		/* Recycling element */
		memset( e, 0, sizeof( plistel ) + list->size );

		e->next = list->unused;
		list->unused = e;
		list->recycled++;
	}
	else
	{
		/* Free element */
		pfree( e );
	}

	list->count--;

	/* store new load factor */
	list->load_factor = plist_get_load_factor( list );
}

/** Pop last element to //dest// off the list //list//.

Like //list// would be a stack, the last element of the list is popped and
its content is written to //dest//, if provided at the end of the list.

//dest// can be omitted and given as (void*)NULL, so the last element will
be popped off the list and discards. */
pboolean plist_pop( plist* list, void* dest )
{
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

	/* Iterate trough the buckets */
	for( i = 0; i < list->hashsize; i++ )
	{
		if( !( e = list->hash[ i ] ) )
			continue;

		while( e )
		{
			if( !n )
				return e;

			n--;

			while( ( e = e->hashnext ) )
			{
				if( e && plist_hash_compare(
							list, e->hashprev->key, e->key, 0 ) )
					break;
			}
		}
	}

	return NULL;
}

/** Retrieve list element by its index from the end.

The function returns the //n//th element of the list //list//
from the right. */
plistel* plist_rget( plist* list, size_t n )
{
	plistel*	e;

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

	if( !list->hash )
		return NULL;

	bucket = plist_hash_index( list, key, 0 );

	for( e = list->hash[ bucket ]; e; e = e->hashnext )
		if( plist_hash_compare( list, e->key, key, 0 ) == 0 )
			return e;

	return NULL;
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

	if( !list->hash )
		return NULL;

	bucket = plist_hash_index( list, key, n );

	for( e = list->hash[ bucket ]; e; e = e->hashnext )
		if( plist_hash_compare( list, e->key, key, n ) == 0 )
			return e;

	return NULL;
}

/** Retrieve list element by pointer.

This function returns the list element of the unit within the list //list//
that is the pointer //ptr//.
*/
plistel* plist_get_by_ptr( plist* list, void* ptr )
{
	plistel*	e;

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

	count = dest->count;

	plist_for( src, e )
		if( !plist_insert( dest, NULL, e->key, plist_access( e ) ) )
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

	plist_for( list, e )
		(*callback)( e );
}

/** Iterates backwards over //list//.

Backwardly iterates over all items of //list// and calls the function
//callback// on every item. The callback function receives the plistel-element
pointer of the list element. */
void plist_riter( plist* list, plistelfn callback )
{
	plistel*	e;

	for( e = plist_last( list ); e; e = plist_prev( e ) )
		(*callback)( e );
}

/** Iterates over //list// and accesses every item.

Iterates over all items of //list// and calls the function //callback// on
every item's access. The callback function receives a pointer to the accessed
element. */
void plist_iter_access( plist* list, plistfn callback )
{
	plistel*	e;

	plist_for( list, e )
		(*callback)( plist_access( e ) );
}

/** Iterates backwards over //list//.

Backwardly iterates over all items of //list// and calls the function
//callback// on every  item's access. The callback function receives a pointer
to the accessed element. */
void plist_riter_access( plist* list, plistfn callback )
{
	plistel*	e;

	for( e = plist_last( list ); e; e = plist_prev( e ) )
		(*callback)( plist_access( e ) );
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

	if( !( all && from
		&& all->size == from->size
		&& all->comparefn == from->comparefn ) )
	{
		/* Invalid list configurations,
			size and compare function must be equal! */
		return 0;
	}

	if( !( count = all->count ) )
		return plist_concat( all, from );

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

	return all->count - count;
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

	if( !( left && right
		   && left->size == right->size
		   && left->comparefn == right->comparefn ) )
	{
		/* Invalid list configurations,
			size and compare function must be equal! */
		return -1;
	}

	/* Check number of elements */
	if( right->count < left->count )
		return 1;
	else if( right->count > left->count )
		return -1;

	/* OK, requiring deep check */
	for( p = plist_first( left ), q = plist_first( right );
				p && q; p = plist_next( p ), q = plist_next( q ) )
	{
		if( ( diff = plist_compare( left, p, q ) ) )
		{
			/* Elements are not equal */
			break;
		}
	}

	return diff;
}

/** Sorts //list// between the elements //from// and //to// according to the
sort-function that was set for the list.

To sort the entire list, use plist_sort().

The sort-function can be modified by using plist_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
void plist_subsort( plist* list, plistel* left, plistel* right )
{
	plistel*	i;
	pboolean	mod;

	do
	{
		mod = FALSE;

		for( i = left; i && i->next && i != right; i = i->next )
		{
			if( (*list->sortfn)( list, i, i->next ) > 0 )
			{
				if( i->next == right )
					right = i;

				plist_swap( list, i, i->next );

				i = i->prev;

				mod = TRUE;
			}
		}

		right = right->prev;
	}
	while( mod && right );
}

/** Sorts //list// according to the sort-function that was set for the list.

To sort only parts of a list, use plist_subsort().

The sort-function can be modified by using plist_set_sortfn().

The default sort function sorts the list by content using the memcmp()
standard function. */
void plist_sort( plist* list )
{
	if( !plist_first( list ) )
		return;

	if( list->printfn )
		(*list->printfn)( list );

	plist_subsort( list, plist_first( list ), plist_last( list ) );

	if( list->printfn )
		(*list->printfn)( list );
}

/** Set compare function.

If no compare function is set or NULL is provided, memcmp() will be used
as default fallback. */
void plist_set_comparefn( plist* list,
			int (*comparefn)( plist*, plistel*, plistel* ) )
{
	if( !( list->comparefn = comparefn ) )
		list->comparefn = plist_compare;
}

/** Set sort function.

If no sort function is given, the compare function set by plist_set_comparefn()
is used. If even unset, memcmp() will be used. */
void plist_set_sortfn( plist* list,
			int (*sortfn)( plist*, plistel*, plistel* ) )
{
	if( !( list->sortfn = sortfn ) )
		list->sortfn = plist_compare;
}

/** Set an element dump function. */
void plist_set_printfn( plist* list, void (*printfn)( plist* ) )
{
	list->printfn = printfn;
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
void plist_swap( plist* l, plistel* a, plistel* b )
{
	plistel*	aprev;
	plistel*	anext;
	plistel*	bprev;
	plistel*	bnext;

	if( a == b )
		return;

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
}

/** Return first element of list //l//. */
plistel* plist_first( plist* l )
{
	if( !l )
		return (plistel*)NULL;

	return l->first;
}

/** Return last element of list //l//. */
plistel* plist_last( plist* l )
{
	if( !l )
		return (plistel*)NULL;

	return l->last;
}

/** Return element size of list //l//. */
int plist_size( plist* l )
{
	if( !l )
		return 0;

	return l->size;
}

/** Return element count of list //l//. */
int plist_count( plist* l )
{
	if( !l )
		return 0;

	return l->count;
}

/** Prints some statistics for the hashmap in //list// on stderr. */
void plist_dbgstats( FILE* stream, plist* list )
{
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
}

/*TESTCASE:plist object functions


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


/** Dynamically allocate heap memory.

The function is a wrapper for the system function malloc(), but with memory
initialization to zero, and immediately stops the program if no more memory
can be allocated.

//size// is the size of memory to be allocated, in bytes.

The function returns the allocated heap memory pointer. The returned memory
address should be freed using pfree() after it is not required anymore.
*/
void* pmalloc( size_t size )
{
	void*	ptr;

	if( !( ptr = malloc( size ) ) )
	{
		OUTOFMEM;
		return (void*)NULL;
	}

	memset( ptr, 0, size );
	return ptr;
}

/** Dynamically (re)allocate memory on the heap.

The function wraps the system-function realloc(), but always accepts a
NULL-pointer and immediately stops the program if no more memory can be
allocated.

//oldptr// is the pointer to be reallocated. If this is (void*)NULL,
prealloc() works like a normal call to pmalloc().

//size// is the size of memory to be reallocated, in bytes.

The function returns the allocated heap memory pointer. The returned memory
address should be freed using pfree() after it is not required any more.
*/
void* prealloc( void* oldptr, size_t size )
{
	void*	ptr;

	if( !oldptr )
		return pmalloc( size );

	if( !( ptr = realloc( oldptr, size ) ) )
	{
		OUTOFMEM;
		return (void*)NULL;
	}

	return ptr;
}

/** Free allocated memory.

The function is a wrapper for the system-function free(), but accepts
NULL-pointers and returns a (void*)NULL pointer for direct pointer memory reset.

It could be used this way to immediately reset a pointer to NULL:

``` ptr = pfree( ptr );

//ptr// is the pointer to be freed.

Always returns (void*)NULL.
*/
void* pfree( void* ptr )
{
	if( ptr )
		free( ptr );

	return (void*)NULL;
}

/** Duplicates a memory entry onto the heap.

//ptr// is the pointer to the memory to be duplicated.
//size// is the size of pointer's data storage.

Returns the new pointer to the memory copy. This should be cast back to the
type of //ptr// again.
*/
void* pmemdup( void* ptr, size_t size )
{
	void*	ret;

	if( !( ptr && size ) )
	{
		WRONGPARAM;
		return (void*)NULL;
	}

	ret = pmalloc( size );
	memcpy( ret, ptr, size );

	return ret;
}


#define MAX_SIZE		64
#define MALLOC_STEP		32

/** Dynamically appends a character to a string.

//str// is the pointer to a string to be appended. If this is (char*)NULL,
the string will be newly allocated. //chr// is the character to be appended
to str.

Returns a char*-pointer to the (possibly re-)allocated and appended string.
(char*)NULL is returned if no memory could be (re)allocated. This pointer must
be released with pfree() when its existence is no longer required.
*/
char* pstrcatchar( char* str, char chr )
{
	PROC( "pstrcatchar" );
	PARMS( "str", "%p", str );
	PARMS( "chr", "%d", chr );

	if( !chr )
		return str;

	if( !str )
	{
		MSG( "Allocating new string" );
		str = (char*)pmalloc( ( 1 + 1 ) * sizeof( char ) );

		if( str )
			*str = '\0';
	}
	else
	{
		MSG( "Reallocating existing string" );
		str = (char*)prealloc( (char*)str,
				( pstrlen( str ) + 1 + 1 ) * sizeof( char ) );
	}

	VARS( "str", "%p", str );
	if( !str )
	{
		MSG( "Pointer is null, critical error" );
		exit( 1 );
	}

	sprintf( str + pstrlen( str ), "%c", (int)chr );

	RETURN( str );
}

/*TESTCASE:pstrcatchar


void testcase()
{
	char* str1;

	str1 = pstrdup( "Hello" );
	str1 = pstrcatchar( str1, 'X' );

	printf( "%s", str1 );

	pfree( str1 );
}
---
HelloX
*/


/** Dynamically appends a zero-terminated string to a dynamic string.

//str// is the pointer to a zero-terminated string to be appended.
If this is (char*)NULL, the string is newly allocated.

//append// is the string to be appended at the end of //str//.

//freesrc// frees the pointer provided as //append// automatically by
this function, if set to TRUE.

Returns a char*-pointer to (possibly re-)allocated and appended string.
(char*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL. If //dest// is NULL and //freesrc// is FALSE, the function
automatically returns the pointer //src//. This pointer must be released with
pfree() when its existence is no longer required.
*/
char* pstrcatstr( char* dest, char* src, pboolean freesrc )
{
	PROC( "pstrcatstr" );
	PARMS( "dest", "%p", dest );
	PARMS( "src", "%p", src );
	PARMS( "freesrc", "%s", BOOLEAN_STR( freesrc ) );

	if( src )
	{
		if( !dest )
		{
			if( freesrc )
			{
				dest = src;
				freesrc = FALSE;
			}
			else
				dest = pstrdup( src );
		}
		else
		{
			dest = (char*)prealloc( (char*)dest,
					( pstrlen( dest ) + pstrlen( src ) + 1 )
						* sizeof( char ) );
			strcat( dest, src );
		}

		if( freesrc )
			pfree( src );
	}

	RETURN( dest );
}

/*TESTCASE:pstrcatstr


void testcase()
{
	char* str1;

	str1 = pstrdup( "Hello" );
	str1 = pstrcatstr( str1, "World", FALSE );

	printf( "%s", str1 );

	pfree( str1 );
}
---
HelloWorld
*/

/** Dynamically appends n-characters from one string to another string.

The function works similar to pstrcatstr(), but allows to copy only a maximum
of //n// characters from //append//.

//str// is the pointer to a string to be appended. If this is (char*)NULL,
the string is newly allocated. //append// is the begin of character sequence to
be appended. //n// is the number of characters to be appended to //str//.

Returns a char*-pointer to (possibly re-)allocated and appended string.
(char*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL. This pointer must be released with pfree() when its existence
is no longer required.
*/
char* pstrncatstr( char* str, char* append, size_t n )
{
	size_t	len		= 0;

	PROC( "pstrncatstr" );
	PARMS( "str", "%p", str );
	PARMS( "append", "%p", append );
	PARMS( "n", "%d", n );

	if( append )
	{
		if( !str )
		{
			if( !( str = (char*)pmalloc( ( n + 1 ) * sizeof( char ) ) ) )
				RETURN( (char*)NULL );
		}
		else
		{
			len = pstrlen( str );

			if( !( str = (char*)prealloc( (char*)str,
					( len + n + 1 ) * sizeof( char ) ) ) )
				RETURN( (char*)NULL );
		}

		strncpy( str + len, append, n );
		str[ len + n ] = '\0';
	}

	RETURN( str );
}

/*TESTCASE:pstrncatstr


void testcase()
{
	char* str1;

	str1 = pstrdup( "Hello" );
	str1 = pstrncatstr( str1, "WorldDinosaurus", 5 );

	printf( "%s", str1 );

	pfree( str1 );
}
---
HelloWorld
*/

/** Replace a substring sequence within a string.

//str// is the string to be replaced in. //find// is the substring to be
matched. //replace// is the string to be inserted for each match of the
substring //find//.

Returns a char* containing the allocated string which is the result of replacing
all occurences of //find// with //replace// in //str//.

This pointer must be released with pfree() when its existence is no longer
required.
*/
char* pstrreplace( char* str, char* find, char* replace )
{
	char*			match;
	char*			str_ptr			= str;
	char*			result			= (char*)NULL;
	char*			result_end		= (char*)NULL;
	unsigned long	len;
	unsigned long	rlen;
	unsigned long	size			= 0L;

	PROC( "pstrreplace" );
	PARMS( "str", "%s", str );
	PARMS( "find", "%s", find );
	PARMS( "replace", "%s", replace );

	len = pstrlen( find );
	rlen = pstrlen( replace );

	while( 1 )
	{
		VARS( "str_ptr", "%s", str_ptr );
		if( !( match = strstr( str_ptr, find ) ) )
		{
			size = 0;
			match = str_ptr + pstrlen( str_ptr );
		}
		else
			size = rlen;

		size += (unsigned long)( match - str_ptr );

		VARS( "size", "%ld", size );
		VARS( "match", "%s", match );

		if( !result )
			result = result_end = (char*)pmalloc(
				( size + 1 ) * sizeof( char ) );
		else
		{
			result = (char*)prealloc( (char*)result,
				( result_end - result + size + 1 ) * sizeof( char ) );
			result_end = result + pstrlen( result );
		}

		if( !result )
		{
			MSG( "Ran out of memory!" );
			exit( 1 );
		}

		strncpy( result_end, str_ptr, match - str_ptr );
		result_end += match - str_ptr;
		*result_end = '\0';

		VARS( "result", "%s", result );

		if( !*match )
			break;

		strcat( result_end, replace );
		VARS( "result", "%s", result );

		result_end += rlen;
		str_ptr = match + len;
	}

	RETURN( result );
}

/*TESTCASE:pstrreplace


void testcase()
{
	char* str1;
	char* str2;

	str1 = pstrdup( "Hello World" );
	str2 = pstrreplace( str1, "World", "Universe" );

	printf( "%s", str2 );

	pfree( str1 );
}
---
Hello Universe
*/

/** Duplicate a string in memory.

//str// is the string to be copied in memory. If //str// is provided as NULL,
the function will also return NULL.

Returns a char*-pointer to the newly allocated copy of //str//. This pointer
must be released with pfree() when its existence is no longer required.
*/
char* pstrdup( char* str )
{
	if( !str )
		return (char*)NULL;

	return (char*)pmemdup( str, ( pstrlen( str ) + 1 ) * sizeof( char ) );
}

/** Duplicate //n// characters from a string in memory.

The function mixes the functionalities of strdup() and strncpy().
The resulting string will be zero-terminated.

//str// is the parameter string to be duplicated. If this is provided as
(char*)NULL, the function will also return (char*)NULL.
//n// is the number of characters to be copied and duplicated from //str//.
If //n// is greater than the length of //str//, copying will stop at the zero
terminator.

Returns a char*-pointer to the allocated memory holding the zero-terminated
string duplicate. This pointer must be released with pfree() when its existence
is no longer required.
*/
char* pstrndup( char* str, size_t len )
{
	char*	ret;

	if( !str )
		return (char*)NULL;

	if( pstrlen( str ) < len )
		len = pstrlen( str );

	ret = (char*)pmalloc( ( len + 1 ) * sizeof( char ) );
	strncpy( ret, str, len );
	ret[ len ] = '\0';

	return ret;
}

/** Return length of a string.

//str// is the parameter string to be evaluated. If (char*)NULL, the function
returns 0. pstrlen() is much safer than strlen() because it returns 0 when
a NULL-pointer is provided.

Returns the length of the string //str//.
*/
size_t pstrlen( char* str )
{
	if( !str )
		return 0;

	return strlen( (char*)str );
}

/** Assign a string to a dynamically allocated pointer.

pstrput() manages the assignment of a dynamically allocated string.

//str// is a pointer receiving the target pointer to be (re)allocated. If
//str// already references a string, this pointer will be freed and reassigned
to a copy of //val//.

//val// is the string to be assigned to //str// (as a independent copy).

Returns a pointer to the allocated heap memory on success, (char*)NULL else.
This is the same pointer as returned when calling ``*str``. The returned pointer
must be released with pfree() or another call of pstrput(). Calling pstrput()
as ``pstrput( &p, (char*)NULL );`` is equivalent to ``p = pfree( &p )``.
*/
char* pstrput( char** str, char* val )
{
	if( *str )
	{
		if( val && strcmp( *str, val ) == 0 )
			return *str;

		pfree( *str );
	}

	*str = pstrdup( val );
	return *str;
}

/** Safely reads a string.

//str// is the string pointer to be safely read. If //str// is NULL, the
function returns a pointer to a static address holding an empty string.
*/
char* pstrget( char* str )
{
	if( !str )
		return "";

	return str;
}

/*TESTCASE:pstrput, pstrget


void testcase()
{
	char* s = NULL;

	printf( "%s\n", pstrget( s ) );
	pstrput( &s, "Hello World" );
	printf( "%s\n", pstrget( s ) );
	pstrput( &s, "Jean Luc Picard" );
	printf( "%s\n", pstrget( s ) );

	pfree( s );
}
---

Hello World
Jean Luc Picard
*/

/** String rendering function.

Inserts multiple values dynamically into the according wildcards positions of
a template string. The function can be compared to the function of
pstrreplace(), but allows to replace multiple substrings by multiple replacement
strings.

//tpl// is the template string to be rendered with values.
//...// are the set of values to be inserted into the desired position;

These consist of three values each:

- //char* name// as a wildcard-name
- //char* value// as the replacement value for the wildcard
- //pboolean freeflag// defines if //value// shall be freed after processing
-

Returns an allocated string which is the result of rendering. This string must
be released by pfree() or another function releasing heap memory when its
existence is no longer required.
*/
char* pstrrender( char* tpl, ... )
{
	struct
	{
		char*	wildcard;
		char*	value;
		BOOLEAN	clear;
		char*	_match;
	} values[MAX_SIZE];

	va_list	args;
	int		i;
	int		vcount;
	int		match;
	char*	tpl_ptr			= tpl;
	char*	result			= (char*)NULL;
	long	copy_size;
	long	prev_size;
	long	size			= 0L;

	if( !tpl )
		return (char*)NULL;

	va_start( args, tpl );

	for( vcount = 0; vcount < MAX_SIZE; vcount++ )
	{
		if( !( values[vcount].wildcard = va_arg( args, char* ) ) )
			break;

		values[vcount].value = va_arg( args, char* );
		values[vcount].clear = (pboolean)va_arg( args, int );

		if( !values[vcount].value )
		{
			values[vcount].value = pstrdup( "" );
			values[vcount].clear = TRUE;
		}
	}

	do
	{
		for( i = 0; i < vcount; i++ )
			values[i]._match = strstr( tpl_ptr, values[i].wildcard );

		match = vcount;
		for( i = 0; i < vcount; i++ )
		{
			if( values[i]._match != (char*)NULL )
			{
				if( match == vcount || values[match]._match > values[i]._match )
					match = i;
			}
		}

		prev_size = size;
		if( match < vcount )
		{
			copy_size = (long)( values[match]._match - tpl_ptr );
			size += (long)strlen( values[match].value );
		}
		else
			copy_size = (long)strlen( tpl_ptr );

		size += copy_size;

		if( result )
			result = (char*)prealloc( (char*)result,
				( size + 1 ) * sizeof( char ) );
		else
			result = (char*)pmalloc( ( size + 1 ) * sizeof( char ) );

		memcpy( result + prev_size, tpl_ptr, copy_size * sizeof( char ) );

		if( match < vcount )
			memcpy( result + prev_size + copy_size, values[match].value,
				strlen( values[match].value ) * sizeof( char ) );

		result[size] = '\0';

		if( match < vcount )
			tpl_ptr += copy_size + strlen( values[match].wildcard );
	}
	while( match < vcount );


	for( i = 0; i < vcount; i++ )
		if( values[i].clear )
			free( values[i].value );

	va_end( args );

	return result;
}

/*TESTCASE:String rendering from templates


void testcase()
{
	char* str1;
	char* str2 = "Hello World";

	str1 = pstrrender( "<a href=\"$link\" alt=\"$title\">$title</a>",
			"$link", "https://phorward.info", FALSE,
			"$title", str2, FALSE,
			(char*)NULL );

	printf( "str1 = >%s<\n", str1 );
	pfree( str1 );
}
---
str1 = ><a href="https://phorward.info" alt="Hello World">Hello World</a><
*/


/** Removes whitespace on the left of a string.

//s// is the string to be left-trimmed.

Returns //s//.
*/
char* pstrltrim( char* s )
{
	char*	c;

	if( !( s && *s ) )
		return pstrget( s );

	for( c = s; *c; c++ )
		if( !( *c == ' ' || *c == '\t' || *c == '\r' || *c == '\n' ) )
			break;

	memmove( s, c, ( pstrlen( c ) + 1 ) * sizeof( char ) );

	return s;
}

/** Removes trailing whitespace on the right of a string.

//s// is the string to be right-trimmed.

Returns //s//.
*/
char* pstrrtrim( char* s )
{
	char*	c;

	if( !( s && *s ) )
		return pstrget( s );

	for( c = s + pstrlen( s ) - 1; c > s; c-- )
		if( !( *c == ' ' || *c == '\t' || *c == '\r' || *c == '\n' ) )
			break;

	*( c + 1 ) = '\0';

	return s;
}

/** Removes beginning and trailing whitespace from a string.

//s// is the string to be trimmed.

Returns //s//.
*/
char* pstrtrim( char* s )
{
	if( !( s && *s ) )
		return s;

	return pstrltrim( pstrrtrim( s ) );
}

/** Splits a string at a delimiting token and returns an allocated array of
token reference pointers.

//tokens// is an allocated array of tokenized array values.
Requires a pointer to char**.
//str// is the input string to be tokenized.
//sep// is the token separation substring.
//limit// is the token limit; If set to 0, there is no token limit available,
in which case as many as possible tokens are read.

Returns the number of separated tokens, or -1 on error.
*/
int pstrsplit( char*** tokens, char* str, char* sep, int limit )
{
	char*	next;
	char*	tok		= str;
	int		cnt		= 0;

	PROC( "pstrsplit" );
	PARMS( "tokens", "%p", tokens );
	PARMS( "str", "%s", str );
	PARMS( "sep", "%p", sep );
	PARMS( "limit", "%d", limit );

	if( !( tokens && str && sep && *sep ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( *tokens = (char**)pmalloc( MALLOC_STEP * sizeof( char* ) ) ) )
		RETURN( -1 );

	VARS( "cnt", "%d", cnt );
	VARS( "tok", "%s", tok );
	(*tokens)[ cnt++ ] = tok;

	while( ( next = strstr( tok, sep ) )
			&& ( ( limit > 0 ) ? cnt < limit : 1 ) )
	{
		tok = next + pstrlen( sep );
		VARS( "next", "%s", next );
		*next = '\0';

		if( ( cnt % MAX_SIZE ) == 0 )
		{
			MSG( "realloc required!" );
			if( !( *tokens = (char**)prealloc( (char**)*tokens,
					( cnt + MALLOC_STEP ) * sizeof( char* ) ) ) )
				RETURN( -1 );
		}

		VARS( "cnt", "%d", cnt );
		VARS( "tok", "%s", tok );
		(*tokens)[ cnt++ ] = tok;
	}

	if( limit > 0 )
		*next = '\0';

	RETURN( cnt );
}

/*TESTCASE:String Tokenizer


void testcase()
{
	char 	str[1024];
	int 	i, all;
	char**	tokens;

	strcpy( str, "Hello World, this is a simple test" );
	all = pstrsplit( &tokens, str, " ", 3 );
	printf( "%d\n", all );

	for( i = 0; i < all; i++ )
		printf( "%d: >%s<\n", i, tokens[i] );

	pfree( tokens );
}
---
3
0: >Hello<
1: >World,<
2: >this<
*/

/** Convert a string to upper-case.

//s// acts both as input- and output string.

Returns //s//.
*/
char* pstrupr( char* s )
{
	char*	ptr;

	if( !s )
		return (char*)NULL;

	for( ptr = s; *ptr; ptr++ )
		if( islower( *ptr ) )
			*ptr = toupper( *ptr );

	return s;
}

/** Convert a string to lower-case.

//s// is the acts both as input and output-string.

Returns //s//.
*/
char* pstrlwr( char* s )
{
	char*	ptr;

	if( !s )
		return (char*)NULL;

	for( ptr = s; *ptr; ptr++ )
		if( isupper( *ptr ) )
			*ptr = tolower( *ptr );

	return s;
}

/** Compare a string ignoring case-order.

//s1// is the string to compare with //s2//.
//s2// is the string to compare with //s1//.

Returns 0 if both strings are equal. Returns a value <0 if //s1// is lower than
//s2// or a value >0 if //s1// is greater than //s2//.
*/
int	pstrcasecmp( char* s1, char* s2 )
{
	if( !( s1 && s2 ) )
		return -1;

	for( ; *s1 && *s2 && toupper( *s1 ) == toupper( *s2 ); s1++, s2++ )
		;

	return (int)( toupper( *s1 ) - toupper( *s2 ) );
}

/** Compare two strings ignoring case-order up to a maximum of //n// bytes.

//s1// is the string to compare with //s2//.
//s2// is the string to compare with //s1//.
//n// is the number of bytes to compare.

Returns 0 if both strings are equal. Returns a value <0 if //s1// is less than
//s2// or a value >0 if //s1// is greater than //s2//.
*/
int	pstrncasecmp( char* s1, char* s2, size_t n )
{
	if( !( s1 && s2 && n ) )
	{
		WRONGPARAM;
		return -1;
	}

	for( ; n > 0 && *s1 && *s2 && toupper( *s1 ) == toupper( *s2 );
			s1++, s2++, n-- )
		;

	return (int)( ( !n ) ? 0 : ( toupper( *s1 ) - toupper( *s2 ) ) );
}

/** Converts a string with included escape-sequences back into its natural form.

The following table shows escape sequences which are converted.

|| Sequence | is replaced by |
| \n | newline |
| \t | tabulator |
| \r | carriage-return |
| \b | backspace |
| \f | form feed |
| \a | bell / alert |
| \' | single-quote |
| \" | double-quote |


The replacement is done within the memory bounds of //str// itself, because the
unescaped version of the character requires less space than its previous escape
sequence.

The function always returns its input pointer.

**Example:**
```
char* s = (char*)NULL;

psetstr( &s, "\\tHello\\nWorld!" );
printf( ">%s<\n", pstrunescape( s ) );

s = pfree( s );
```
*/
char* pstrunescape( char* str )
{
	char*	ch;
	char*	esc;
	char*	ptr;

	for( ptr = ch = str; *ch; ch++, ptr++ )
	{
		if( *ch == '\\' && *( ch + 1 ) )
		{
			for( esc = "n\nt\tr\rb\bf\fv\va\a'\'\"\""; *esc; esc += 2 )
				if( *( ch + 1 ) == *esc )
				{
					*ptr = *( esc + 1 );
					ch++;
					break;
				}

			/* TODO: hex-seqs (UTF-8). */
		}
		else
			*ptr = *ch;
	}

	*ptr = '\0';
	return str;
}

/** Implementation and replacement for vasprintf.

//str// is the pointer receiving the result, allocated string pointer.
//fmt// is the format string.
//...// are the parameters according to the placeholders set in //fmt//.

Returns the number of characters written, or -1 in case of an error.
*/
int pvasprintf( char** str, char* fmt, va_list ap )
{
	char*		istr;
	int			ilen;
	int			len;
	va_list		w_ap;

	PROC( "pvasprintf" );
	PARMS( "str", "%p", str );
	PARMS( "fmt", "%s", fmt );
	PARMS( "ap", "%p", ap );

	if( !( istr = (char*)pmalloc( MALLOC_STEP * sizeof( char ) ) ) )
		RETURN( -1 );

	va_copy( w_ap, ap );

	MSG( "Invoking vsnprintf() for the first time" );
	len = vsnprintf( istr, MALLOC_STEP, fmt, w_ap );
	VARS( "len", "%d", len );

	if( len >= 0 && len < MALLOC_STEP )
		*str = istr;
	else if( len == INT_MAX || len < 0 )
	{
		MSG( "ret is negative or too big - can't handle this!" );
		va_end( w_ap );
		RETURN( -1 );
	}
	else
	{
		if( !( istr = prealloc( istr, ( ilen = len + 1 ) * sizeof( char ) ) ) )
		{
			va_end( w_ap );
			RETURN( -1 );
		}

		va_end( w_ap );
		va_copy( w_ap, ap );

		MSG( "Invoking vsnprintf() for the second time" );
		len = vsnprintf( istr, ilen, fmt, w_ap );
		VARS( "len", "%d", len );

		if( len >= 0 && len < ilen )
			*str = istr;
		else
		{
			pfree( istr );
			RETURN( -1 );
		}
	}

	va_end( w_ap );
	RETURN( len );
}

/** Implementation and replacement for asprintf. pasprintf() takes only the
format-string and various arguments. It outputs an allocated string to be freed
later on.

//fmt// is the format string.
//...// are the parameters according to the placeholders set in //fmt//.

Returns a char* Returns the allocated string which contains the format string
with inserted values.
*/
char* pasprintf( char* fmt, ... )
{
	char*	str		= (char*)NULL;
	va_list	args;

	PROC( "pasprintf" );
	PARMS( "fmt", "%s", fmt );

	if( !( fmt ) )
		RETURN( (char*)NULL );

	va_start( args, fmt );
	pvasprintf( &str, fmt, args );
	va_end( args );

	VARS( "str", "%s", str );
	RETURN( str );
}

/*TESTCASE:Self-allocating sprintf extension


void testcase()
{
	char* str = "Hello World";
	char* str1;

	str1 = pasprintf( "current content of str is >%s<, and len is %d",
						str, strlen( str ) );
	printf( "str1 = >%s<\n", str1 );
	pfree( str1 );
}
---
str1 = >current content of str is >Hello World<, and len is 11<
*/


/******************************************************************************
 * FUNCTIONS FOR UNICODE PROCESSING (wchar_t)                                 *
 ******************************************************************************/

#ifdef UNICODE

/** Duplicate a wide-character string in memory.

//str// is the string to be copied in memory. If //str// is provided as NULL,
the function will also return NULL.

Returns a wchar_t*-pointer to the newly allocated copy of //str//. This pointer
must be released with pfree() when its existence is no longer required.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsdup( wchar_t* str )
{
	if( !str )
		return (wchar_t*)NULL;

	return (wchar_t*)pmemdup( str, ( pwcslen( str ) + 1 ) * sizeof( wchar_t ) );
}

/** Appends a character to a dynamic wide-character string.

//str// is the pointer to a wchar_t-string to be appended. If this is
(wchar_t*)NULL, the string is newly allocated. //chr// is the the character
to be appended to str.

Returns a wchar_t* Pointer to (possibly re-)allo- cated and appended string.
(wchar_t*)NULL is returned if no memory could be (re)allocated.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcscatchar( wchar_t* str, wchar_t chr )
{
	PROC( "pwcscatchar" );
	PARMS( "str", "%p", str );
	PARMS( "chr", "%d", chr );

	if( !str )
	{
		MSG( "Allocating new string" );
		str = (wchar_t*)pmalloc( ( 1 + 1 ) * sizeof( wchar_t ) );

		if( str )
			*str = L'\0';
	}
	else
	{
		MSG( "Reallocating existing string" );
		str = (wchar_t*)prealloc( (wchar_t*)str,
				( pwcslen( str ) + 1 + 1) * sizeof( wchar_t ) );
	}

	VARS( "str", "%p", str );
	if( !str )
	{
		MSG( "Pointer is null, critical error" );
		exit( 1 );
	}

	#if _WIN32
	_snwprintf( str + pwcslen( str ), 1, L"%lc", chr );
	#else
	swprintf( str + pwcslen( str ), 1, L"%lc", chr );
	#endif

	RETURN( str );
}

/** Appends a (possibly dynamic) wide-character string to a dynamic
wide-character string.

//str// is the pointer to a wchar_t-string to be appended. If this is
(wchar_t*)NULL, the string is newly allocated.
//append// is the string to be appended.
//freesrc// if true, //append// is free'd automatically by this function.

Returns a wchar_t* Pointer to (possibly re-)allo- cated and appended string.
(wchar_t*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcscatstr( wchar_t* dest, wchar_t* src, pboolean freesrc )
{
	PROC( "pwcscatstr" );
	PARMS( "dest", "%p", dest );
	PARMS( "src", "%p", src );
	PARMS( "freesrc", "%d", freesrc );

	if( src )
	{
		if( !dest )
		{
			if( freesrc )
			{
				dest = src;
				freesrc = FALSE;
			}
			else
				dest = pwcsdup( src );
		}
		else
		{
			dest = (wchar_t*)prealloc( (wchar_t*)dest,
					( pwcslen( dest ) + pwcslen( src ) + 1 )
						* sizeof( wchar_t ) );
			wcscat( dest, src );
		}

		if( freesrc )
			pfree( src );
	}

	RETURN( dest );
}

/** Appends //n// characters from one wide-character string to a dynamic string.

//str// is the pointer to a wchar_t-string to be appended. If this is
(wchar_t*)NULL, the string is newly allocated.
//append// is the begin of character sequence to be appended.
//n// is the number of characters to be appended to str.

Returns a wchar_t* Pointer to (possibly re-)allo- cated and appended string.
(wchar_t*)NULL is returned if no memory could be (re)allocated, or both strings
were NULL.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsncatstr( wchar_t* str, wchar_t* append, size_t n )
{
	size_t	len		= 0;

	PROC( "pwcsncatstr" );
	PARMS( "str", "%p", str );
	PARMS( "str", "%ls", str );
	PARMS( "append", "%p", append );
	PARMS( "append", "%ls", append );
	PARMS( "n", "%d", n );

	if( append )
	{
		if( !str )
		{
			if( !( str = (wchar_t*)pmalloc( ( n + 1 ) * sizeof( wchar_t ) ) ) )
				RETURN( (wchar_t*)NULL );
		}
		else
		{
			len = pwcslen( str );

			VARS( "len", "%d", len );
			VARS( "( len + n + 1 ) * sizeof( wchar_t )",
						"%d", ( len + n + 1 ) * sizeof( wchar_t ) );

			if( !( str = (wchar_t*)prealloc( (wchar_t*)str,
					( len + n + 1 ) * sizeof( wchar_t ) ) ) )
				RETURN( (wchar_t*)NULL );
		}

		wcsncpy( str + len, append, n );
		str[ len + n ] = 0;
	}

	RETURN( str );
}

/** Safe strlen replacement for wide-character.

//str// is the parameter string to be evaluated. If (wchar_t*)NULL,
the function returns 0.

//This function is only available when compiled with -DUNICODE.//
*/
size_t pwcslen( wchar_t* str )
{
	if( !str )
		return 0;

	return wcslen( str );
}

/** Assign a wide-character string to a dynamically allocated pointer.

pwcsput() manages the assignment of an dynamically allocated  wide-chararacter
string.

//str// is a pointer receiving the target pointer to be (re)allocated. If
//str// already references a wide-character string, this pointer will be freed
and reassigned to a copy of //val//.

//val// is the the wide-character string to be assigned to //str//
(as an independent copy).

Returns a pointer to the allocated heap memory on success, (char_t*)NULL else.
This is the same pointer as returned when calling ``*str``. The returned pointer
must be released with pfree() or another call of pwcsput(). Calling pwcsput()
as ``pwcsput( &p, (char*)NULL );`` is equivalent to ``p = pfree( &p )``.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsput( wchar_t** str, wchar_t* val )
{
	if( *str )
	{
		if( val && wcscmp( *str, val ) == 0 )
			return *str;

		pfree( *str );
	}

	*str = pwcsdup( val );
	return *str;
}

/** Safely reads a wide-character string.

//str// is the string pointer to be safely read. If //str// is NULL, the
function returns a pointer to a static address holding an empty string.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsget( wchar_t* str )
{
	if( !str )
		return L"";

	return str;
}

/** Duplicate //n// characters from a wide-character string in memory.

The function mixes the functionalities of wcsdup() and wcsncpy().
The resulting wide-character string will be zero-terminated.

//str// is the parameter wide-character string to be duplicated.
If this is provided as (wchar_t*)NULL, the function will also return
(wchar_t*)NULL.

//n// is the the number of characters to be copied and duplicated from //str//.
If //n// is greater than the length of //str//, copying will stop at the zero
terminator.

Returns a wchar_t*-pointer to the allocated memory holding the zero-terminated
wide-character string duplicate. This pointer must be released with pfree()
when its existence is no longer required.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pwcsndup( wchar_t* str, size_t len )
{
	wchar_t*	ret;

	if( !str )
		return (wchar_t*)NULL;

	if( pwcslen( str ) < len )
		len = pwcslen( str );

	ret = (wchar_t*)pmalloc( ( len + 1 ) * sizeof( wchar_t ) );
	wcsncpy( ret, str, len );
	ret[ len ] = '\0';

	return ret;
}


/** Wide-character implementation of pasprintf().

//str// is the a pointer receiving the resultung, allocated string pointer.
//fmt// is the the format string.
//...// is the parameters according to the placeholders set in //fmt//.

Returns the number of characters written.

//This function is only available when compiled with -DUNICODE.//
*/
int pvawcsprintf( wchar_t** str, wchar_t* fmt, va_list ap )
{
	wchar_t*	istr;
	int			ilen;
	int			len;
	va_list		w_ap;

	PROC( "pvawcsprintf" );
	PARMS( "str", "%p", str );
	PARMS( "fmt", "%ls", fmt );
	PARMS( "ap", "%p", ap );

	if( !( istr = (wchar_t*)pmalloc( ( ilen = MALLOC_STEP + 1 )
				* sizeof( wchar_t ) ) ) )
		RETURN( -1 );

	do
	{
		va_copy( w_ap, ap );

		len =
#ifdef __MINGW32__
		_vsnwprintf
#else
		vswprintf
#endif
		( istr, (size_t)ilen, fmt, w_ap );

		va_end( w_ap );
		VARS( "len", "%d", len );

		if( len < 0 )
		{
			if( !( istr = prealloc( istr, ( ilen = ilen + MALLOC_STEP + 1 )
					* sizeof( wchar_t ) ) ) )
				RETURN( -1 );
		}
	}
	while( len < 0 || len >= ilen );

	*str = istr;

	RETURN( len );
}

/** An implementation of pasprintf() for wide-character wchar_t. pasprintf()
takes only the format-string and various arguments. It outputs an allocated
string to be released with pfree() later on.

//fmt// is the format string.
//...// are the parameters according to the placeholders set in //fmt//.

Returns a wchar_t* Returns the allocated string which cointains the format
string with inserted values.

//This function is only available when compiled with -DUNICODE.//
*/
wchar_t* pawcsprintf( wchar_t* fmt, ... )
{
	wchar_t*	str;
	va_list	args;

	PROC( "pasprintf" );
	PARMS( "fmt", "%ls", fmt );

	if( !( fmt ) )
		RETURN( (wchar_t*)NULL );

	va_start( args, fmt );
	pvawcsprintf( &str, fmt, args );
	va_end( args );

	VARS( "str", "%ls", str );
	RETURN( str );
}

#endif /* UNICODE */

/*TESTCASE:UNICODE functions



void unicode_demo()
{
	wchar_t		mystr		[ 255 ];
	wchar_t*	mydynamicstr;

	setlocale( LC_ALL, "" );

	wcscpy( mystr, L"Yes, w€ cän üse standard C function "
			L"names for Unicode-strings!" );

	printf( "mystr = >%ls<\n", mystr );
	swprintf( mystr, sizeof( mystr ),
			L"This string was %d characters long!",
			pwcslen( mystr ) );
	printf( "mystr = >%ls<\n", mystr );

	mydynamicstr = pwcsdup( mystr );
	mydynamicstr = pwcscatstr( mydynamicstr,
			L" You can see: The functions are"
			L" used the same way as the standard"
			L" char-functions!", FALSE );

	printf( "mydynamicstr = >%ls<\n", mydynamicstr );
	pfree( mydynamicstr );

	mydynamicstr = pawcsprintf( L"This is €uro symbol %ls of %d",
						mystr, sizeof( mystr ) );
	printf( "mydynamicstr = >%ls<\n", mydynamicstr );
	pfree( mydynamicstr );
}
---
mystr = >Yes, w€ cän üse standard C function names for Unicode-strings!<
mystr = >This string was 62 characters long!<
mydynamicstr = >This string was 62 characters long! You can see: The functions are used the same way as the standard char-functions!<
mydynamicstr = >This is €uro symbol This string was 62 characters long! of 1020<
*/


/** Figures out a filepath by searching in a PATH definition.

//filename// is the filename to be searched for.

//directories// is a string specifying the directories to search in.
If this is (char*)NULL, the environment variable PATH will be used and
evaluated by using [getenv() #fn_getenv]. The path can be split with multiple
paths by a character that depends on the current platform
(Unix: ":", Windows: ";").

Returns a static pointer to the absolute path that contains the file specified
as filename, else it will return (char*)NULL.
*/
char* pwhich( char* filename, char* directories )
{
	static char	path	[ BUFSIZ + 1 ];
	char*			start;
	char*			end;

	if( !( filename && *filename ) )
	{
		WRONGPARAM;
		return (char*)NULL;
	}

	if( !directories )
		directories = getenv( "PATH" );

	start = directories;
	while( start && *start )
	{
		if( !( end = strchr( start, PDIRSEP ) ) )
			end = start + pstrlen( start );

		if( ( end - start ) + pstrlen( filename ) <= BUFSIZ )
		{
			sprintf( path, "%.*s%c%s",
				(int)((end - start) - ( (*( end - 1) == PPATHSEP ) ? 1 : 0 )),
					start, PPATHSEP, filename );

			if( pfileexists( path ) )
				return path;
		}

		if( ! *end )
			break;

		start = end + 1;
	}

	return (char*)NULL;
}

/** Returns the basename of a file.

//path// is the file path pointer.

Returns a pointer to the basename, which is a part of //path//. */
char* pbasename( char* path )
{
	char*		basename;

	PROC( "pbasename" );
	PARMS( "path", "%s", path );

	basename = strrchr( path, PPATHSEP );
	VARS( "basename", "%s", basename ? basename+1 : path );
	RETURN( ( basename ) ? ++basename : path );
}

/** Checks for file existence.

//filename// is the path to a file that will be checked.

Returns TRUE on success, FALSE if not. */
pboolean pfileexists( char* filename )
{
	PROC( "pfileexists" );
	PARMS( "filename", "%s", filename );

	if( !
	#ifndef _WIN32
		access( filename, F_OK )
	#else
		access( filename, 0 )
	#endif
	)
	{
		MSG( "File exists!" );
		RETURN( TRUE );
	}

	MSG( "File does not exist!" );
	RETURN( FALSE );
}

/** Maps the content of an entire file into memory.

//cont// is the file content return pointer.
//filename// is the path to file to be mapped

The function returns TRUE on success. */
pboolean pfiletostr( char** cont, char* filename )
{
	FILE*	f;
	char*	c;

	PROC( "pfiletostr" );
	PARMS( "cont", "%p", cont );
	PARMS( "filename", "%s", filename );

	/* Check parameters */
	if( !( cont && filename && *filename ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* Open file */
	if( !( f = fopen( filename, "rb" ) ) )
	{
		MSG( "File could not be opened - wrong path?" );
		RETURN( FALSE );
	}

	/* Allocate memory for file */
	fseek( f, 0L, SEEK_END );
	if( !( c = *cont = (char*)pmalloc( ( ftell( f ) + 1 )
			* sizeof( char ) ) ) )
	{
		MSG( "Unable to allocate required memory" );

		fclose( f );
		RETURN( FALSE );
	}

	/* Read entire file into buffer */
	fseek( f, 0L, SEEK_SET );

	while( !feof( f ) )
		*(c++) = fgetc( f );

	/* Case: File is empty */
	if( c == *cont )
		c++;

	*( c - 1 ) = '\0';

	fclose( f );

	VARS( "Entire file", "%s", *cont );
	RETURN( TRUE );
}

/** Command-line option interpreter.

This function works similar to the getopt() functions of the GNU Standard
Library, but uses a different style of parameter submit.

It supports both short- and long- option-style parameters.

- //opt// is a pointer to a buffer with enough space to store the requested parameter to. For short parameters, this is only one char, for long-parameters the full name. The string will be zero-terminated.
- //param// is a pointer to store a possible parameter value to, if the detected option allows for parameters.
- //next// receives the index in argv of the next evaluated option. It can be left (int*)NULL. It points to the next valid index in argv[] after all parameters have been evaluated. Check it for < argc, to point to valid data.
- //argc// is the argument count as taken from the main() function.
- //argv// are the argument values as taken from the main() function.
- //optstr// contains the possible short-options. This is a string where each character defines an option. If an option takes a parameter, a colon (:) is submitted. E.g. "abc:def:g". The Options "-c" and "-f" will take a parameter that is
returned to param. This parameter can be (char*)NULL.
- //loptstr// contains the possible long-options. This is a string containing all long option names, each separated by a blank. Long options taking parameters have an attached colon (:) to the name.  E.g. "hello world: next" defines three long options, where option 'world' takes one parameter that is returned to param. This parameter can be (char*)NULL.
- //idx// is the index of the requested option, 0 for the first option behind argv[0].
-

The function must be called multiple times to read all command-line parameters
and to react on the parameters.

The function returns 0, if the parameter with the given index was
successfully evaluated. It returns 1, if there are still command-line
parameters, but not as part of options. The parameter //param// will receive
the given pointer. It returns -1 if no more options could be read, or if an
option could not be evaluated (unknown option). In such case, //param// will
hold a string to the option that is unknown to pgetopt().

**Example:**

This is a minimal example showing the usage of pgetopt() in a real program:

```


int main( int argc, char** argv )
{
    int			i;
    int			rc;
    int			next;
    char		opt			[ 10 + 1 ];
    char*		param;

    for( i = 0; ( rc = pgetopt( opt, &param, &next, argc, argv,
                                "ho:", "help output:", i ) ) == 0; i++ )
    {
        if( !strcmp( opt, "help" ) || !strcmp( opt, "h" ) )
            printf( "Help\n" );
        else if( !strcmp( opt, "output" ) || !strcmp( opt, "o" ) )
            printf( "Output = >%s<\n", param );
    }

    if( rc < 0 && param )
    {
        fprintf( stderr, "Unknown option '%s'\n", param );
        return 1;
    }

    for( i = 0; next < argc; next++, i++ )
        printf( "Parameter %d = >%s<\n", i, argv[ next ] );

    return 0;
}
```
*/
int pgetopt( char* opt, char** param, int* next,
				int argc, char** argv, char* optstr,
					char* loptstr, int idx )
{
	BOOLEAN		has_parm;
	BOOLEAN		found;
	BOOLEAN		lopt;
	int			i;
	char*		str;
	char*		pos;
	char*		del;
	static char	optinfo	[ 2+1 ];

	PROC( "pgetopt" );
	PARMS( "opt", "%p", opt );
	PARMS( "param", "%p", param );
	PARMS( "next", "%p", next );
	PARMS( "argc", "%d", argc );
	PARMS( "argv", "%p", argv );
	PARMS( "optstr", "%s", optstr );
	PARMS( "loptstr", "%s", loptstr );
	PARMS( "idx", "%d", idx );

	if( !( opt && param && idx >= 0 && ( optstr || loptstr ) ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	*param = (char*)NULL;
	if( next )
		*next = 1;

	if( argc < 2 )
		RETURN( 1 );

	for( i = 1; i < argc; i++ )
	{
		found = FALSE;
		str = argv[i];
		lopt = FALSE;

		if( *str == '-' )
		{
			str++;
			if( *str == '-' )
			{
				str++;
				lopt = TRUE;
			}
		}
		else
		{
			*param = str;

			if( next )
				*next = i;

			RETURN( 1 );
		}

		if( !lopt && optstr && *optstr )
		{
			while( *str && idx >= 0 )
			{
				found = FALSE;
				for( pos = optstr; *pos; pos++ )
				{
					if( *pos == ':' )
						continue;

					if( *pos == *str )
					{
						if( *( pos + 1 ) == ':' )
						{
							if( argc > ++i )
							{
								if( !idx )
									*param = argv[ i ];
							}

						}

						if( !idx )
						{
							sprintf( opt, "%c", *pos );

							if( next )
								*next = i + 1;

							RETURN( 0 );
						}

						found = TRUE;
						break;
					}
				}

				if( !found )
				{
					sprintf( optinfo, "-%c", *str );
					*param = optinfo;
					RETURN( -1 );
				}

				idx--;
				str++;
			}
		}
		else if( lopt && loptstr && *loptstr )
		{
			for( pos = loptstr; *pos; pos = ( *del ? del + 1 : del ) )
			{
				if( !( del = strstr( pos, " " ) ) )
					del = pos + pstrlen( pos );

				if( del == pos )
					continue;

				if( del > pos && *( del - 1 ) == ':' )
				{
					has_parm = TRUE;
					del--;
				}
				else
					has_parm = FALSE;

				if( strncmp( pos, str, del - pos ) == 0 )
				{
					if( has_parm && argc > ++i )
					{
						if( !idx )
							*param = argv[ i ];
					}

					if( !idx )
					{
						sprintf( opt, "%.*s", (int)( del - pos ), str );

						if( next )
							*next = i + 1;

						RETURN( 0 );
					}

					found = TRUE;
					break;
				}
			}

			if( !found )
			{
				*param = argv[ i ];

				if( next )
					*next = i + 1;

				RETURN( -1 );
			}

			idx--;
		}
		else
			break;
	}

	if( next )
		*next = i;

	RETURN( -1 );
}

/** Reads an entire line from //stream//, storing the address of the buffer
	containing the text into //lineptr//. The buffer is zero-terminated and
	includes the newline character, if one was found.

	This function serves as a platform-independent implementation for POSIX
	getline(), which is wrapped in case of POSIX.
*/
size_t pgetline( char** lineptr, size_t* n, FILE* stream )
{
	#ifndef _WIN32
	return getline( lineptr, n, stream );
	#else
    char*	bufptr = (char*)NULL;
    char*	p = bufptr;
    size_t	size;
    int 	c;

    if( !( lineptr && stream && n ) )
        return -1;

    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if( c == EOF )
        return -1;

    if( !bufptr )
    {
        bufptr = (char*)pmalloc( 128 );
        size = 128;
    }

    p = bufptr;

    while( c != EOF )
    {
        if( ( p - bufptr ) > ( size - 1 ) )
        {
            size = size + 128;
            bufptr = prealloc(bufptr, size);
        }

        *p++ = c;

        if( c == '\n' )
            break;

        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
    #endif
}
/*
  Basic UTF-8 manipulation routines
  by Jeff Bezanson
  placed in the public domain Fall 2005

  This code is designed to provide the utilities you need to manipulate
  UTF-8 as an internal string encoding. These functions do not perform the
  error checking normally needed when handling UTF-8 data, so if you happen
  to be from the Unicode Consortium you will want to flay me alive.
  I do this because error checking can be performed at the boundaries (I/O),
  with these routines reserved for higher performance on data known to be
  valid.
*/



#if _WIN32
#define snprintf _snprintf
#endif

static const size_t offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/** Check for UTF-8 character sequence signature.

The function returns TRUE, if the character //c// is the beginning of a UTF-8
character signature, else FALSE. */
pboolean putf8_isutf( unsigned char c )
{
	return MAKE_BOOLEAN( ( c & 0xC0 ) != 0x80 );
}

/** Returns length of next UTF-8 sequence in a multi-byte character string.

//s// is the pointer to begin of UTF-8 sequence.

Returns the number of bytes used for the next character.
*/
int putf8_seqlen(char *s)
{
#ifdef UTF8
	if( putf8_isutf( (unsigned char)*s ) )
	    return trailingBytesForUTF8[ (unsigned char)*s ] + 1;
#endif
	return 1;
}

/** Return single character (as wide-character value) from UTF-8 multi-byte
character string.

//str// is the pointer to character sequence begin. */
wchar_t putf8_char( char* str )
{
#ifndef UTF8
	return (wchar_t)*str;
#else
	int 	nb;
	wchar_t 	ch = 0;

	switch( ( nb = trailingBytesForUTF8[ (unsigned char)*str ] ) )
	{
        case 3:
			ch += (unsigned char)*str++;
			ch <<= 6;
        case 2:
			ch += (unsigned char)*str++;
			ch <<= 6;
        case 1:
			ch += (unsigned char)*str++;
			ch <<= 6;
        case 0:
			ch += (unsigned char)*str++;
	}

	ch -= offsetsFromUTF8[ nb ];

	return ch;
#endif
}

/** Moves //count// characters ahead in an UTF-8 multi-byte character sequence.

//str// is the pointer to UTF-8 string to begin moving.
//count// is the number of characters to move left.

The function returns the address of the next UTF-8 character sequence after
//count// characters. If the string's end is reached, it will return a
pointer to the zero-terminator.
*/
char* putf8_move( char* str, int count )
{
	for( ; count > 0; count-- )
		str += putf8_seqlen( str );

	return str;
}

/** Read one character from an UTF-8 input sequence.
This character can be escaped, an UTF-8 character or an ordinary ASCII-char.

//chr// is the input- and output-pointer (the pointer is replaced by the pointer
to the next character or escape-sequence within the string).

The function returns the character code of the parsed character.
*/
wchar_t putf8_parse_char( char** ch )
{
	wchar_t	ret;
	PROC( "putf8_parse_char" );
	PARMS( "ch", "%p", ch );

#ifdef UTF8
	if( putf8_char( *ch ) == (wchar_t)'\\' )
	{
		MSG( "Escape sequence detected" );
		(*ch)++;

		VARS( "*ch", "%s", *ch );
		(*ch) += putf8_read_escape_sequence( *ch, &ret );
		VARS( "*ch", "%s", *ch );
	}
	else
	{
		MSG( "No escape sequence, normal UTF-8 character sequence processing" );
		ret = putf8_char( *ch );
		(*ch) += putf8_seqlen( *ch );
	}
#else
	ret = *( (*ch)++ );
#endif

	VARS( "ret", "%d", ret );
	RETURN( ret );
}


/* conversions without error checking
   only works for valid UTF-8, i.e. no 5- or 6-byte sequences
   srcsz = source size in bytes, or -1 if 0-terminated
   sz = dest size in # of wide characters

   returns # characters converted
   dest will always be L'\0'-terminated, even if there isn't enough room
   for all the characters.
   if sz = srcsz+1 (i.e. 4*srcsz+4 bytes), there will always be enough space.
*/
int putf8_toucs(wchar_t *dest, int sz, char *src, int srcsz)
{
    wchar_t ch;
    char *src_end = src + srcsz;
    int nb;
    int i=0;

    while (i < sz-1) {
        nb = trailingBytesForUTF8[(int)*src];
        if (srcsz == -1) {
            if (*src == 0)
                goto done_toucs;
        }
        else {
            if (src + nb >= src_end)
                goto done_toucs;
        }
        ch = 0;
        switch (nb) {
            /* these fall through deliberately */
        case 3: ch += *src++; ch <<= 6;
        case 2: ch += *src++; ch <<= 6;
        case 1: ch += *src++; ch <<= 6;
        case 0: ch += *src++;
        }
        ch -= offsetsFromUTF8[nb];
        dest[i++] = ch;
    }
 done_toucs:
    dest[i] = 0;
    return i;
}

/* srcsz = number of source characters, or -1 if 0-terminated
   sz = size of dest buffer in bytes

   returns # characters converted
   dest will only be '\0'-terminated if there is enough space. this is
   for consistency; imagine there are 2 bytes of space left, but the next
   character requires 3 bytes. in this case we could NUL-terminate, but in
   general we can't when there's insufficient space. therefore this function
   only NUL-terminates if all the characters fit, and there's space for
   the NUL as well.
   the destination string will never be bigger than the source string.
*/
int putf8_toutf8(char *dest, int sz, wchar_t *src, int srcsz)
{
    wchar_t ch;
    int i = 0;
    char *dest_end = dest + sz;

    while (srcsz<0 ? src[i]!=0 : i < srcsz) {
        ch = src[i];
        if (ch < 0x80) {
            if (dest >= dest_end)
                return i;
            *dest++ = (char)ch;
        }
        else if (ch < 0x800) {
            if (dest >= dest_end-1)
                return i;
            *dest++ = (ch>>6) | 0xC0;
            *dest++ = (ch & 0x3F) | 0x80;
        }
        #ifndef _WIN32
        #if UNICODE
        else if (ch < 0x10000) {
            if (dest >= dest_end-2)
                return i;
            *dest++ = (ch>>12) | 0xE0;
            *dest++ = ((ch>>6) & 0x3F) | 0x80;
            *dest++ = (ch & 0x3F) | 0x80;
        }
        else if (ch < 0x110000) {
            if (dest >= dest_end-3)
                return i;
            *dest++ = (ch>>18) | 0xF0;
            *dest++ = ((ch>>12) & 0x3F) | 0x80;
            *dest++ = ((ch>>6) & 0x3F) | 0x80;
            *dest++ = (ch & 0x3F) | 0x80;
        }
        #endif
        #endif

        i++;
    }
    if (dest < dest_end)
        *dest = '\0';
    return i;
}

int putf8_wc_toutf8(char *dest, wchar_t ch)
{
    if (ch < 0x80) {
        dest[0] = (char)ch;
        return 1;
    }
    if (ch < 0x800) {
        dest[0] = (ch>>6) | 0xC0;
        dest[1] = (ch & 0x3F) | 0x80;
        return 2;
    }

    #ifndef _WIN32
    #if UNICODE
    if (ch < 0x10000) {
        dest[0] = (ch>>12) | 0xE0;
        dest[1] = ((ch>>6) & 0x3F) | 0x80;
        dest[2] = (ch & 0x3F) | 0x80;
        return 3;
    }
    if (ch < 0x110000) {
        dest[0] = (ch>>18) | 0xF0;
        dest[1] = ((ch>>12) & 0x3F) | 0x80;
        dest[2] = ((ch>>6) & 0x3F) | 0x80;
        dest[3] = (ch & 0x3F) | 0x80;
        return 4;
    }
    #endif
    #endif

    return 0;
}

/* charnum => byte offset */
int putf8_offset(char *str, int charnum)
{
    int offs=0;

    while (charnum > 0 && str[offs]) {
        (void)(putf8_isutf(str[++offs]) || putf8_isutf(str[++offs]) ||
               putf8_isutf(str[++offs]) || ++offs);
        charnum--;
    }
    return offs;
}

/* byte offset => charnum */
int putf8_charnum(char *s, int offset)
{
    int charnum = 0, offs=0;

    while (offs < offset && s[offs]) {
        (void)(putf8_isutf(s[++offs]) || putf8_isutf(s[++offs]) ||
               putf8_isutf(s[++offs]) || ++offs);
        charnum++;
    }
    return charnum;
}

/* number of characters */
int putf8_strlen(char *s)
{
    int count = 0;

    while( *( s += putf8_seqlen( s ) ) )
        count++;

    return count;
}

/* reads the next utf-8 sequence out of a string, updating an index */
wchar_t putf8_nextchar(char *s, int *i)
{
    wchar_t ch = 0;
    int sz = 0;

    do {
        ch <<= 6;
        ch += s[(*i)++];
        sz++;
    } while (s[*i] && !putf8_isutf(s[*i]));
    ch -= offsetsFromUTF8[sz-1];

    return ch;
}

void putf8_inc(char *s, int *i)
{
    (void)(putf8_isutf(s[++(*i)]) || putf8_isutf(s[++(*i)]) ||
           putf8_isutf(s[++(*i)]) || ++(*i));
}

void putf8_dec(char *s, int *i)
{
    (void)(putf8_isutf(s[--(*i)]) || putf8_isutf(s[--(*i)]) ||
           putf8_isutf(s[--(*i)]) || --(*i));
}

int octal_digit(char c)
{
    return (c >= '0' && c <= '7');
}

int hex_digit(char c)
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f'));
}

/* assumes that src points to the character after a backslash
   returns number of input characters processed */
int putf8_read_escape_sequence(char *str, wchar_t *dest)
{
    wchar_t ch;
    char digs[9]="\0\0\0\0\0\0\0\0";
    int dno=0, i=1;

    ch = (wchar_t)str[0];    /* take literal character */
    if (str[0] == 'n')
        ch = L'\n';
    else if (str[0] == 't')
        ch = L'\t';
    else if (str[0] == 'r')
        ch = L'\r';
    else if (str[0] == 'b')
        ch = L'\b';
    else if (str[0] == 'f')
        ch = L'\f';
    else if (str[0] == 'v')
        ch = L'\v';
    else if (str[0] == 'a')
        ch = L'\a';
    else if (octal_digit(str[0])) {
        i = 0;
        do {
            digs[dno++] = str[i++];
        } while (octal_digit(str[i]) && dno < 3);
        ch = strtol(digs, NULL, 8);
    }
    else if (str[0] == 'x') {
        while (hex_digit(str[i]) && dno < 2) {
            digs[dno++] = str[i++];
        }
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    }
    else if (str[0] == 'u') {
        while (hex_digit(str[i]) && dno < 4) {
            digs[dno++] = str[i++];
        }
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    }
    else if (str[0] == 'U') {
        while (hex_digit(str[i]) && dno < 8) {
            digs[dno++] = str[i++];
        }
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    }
    *dest = ch;

    return i;
}

/* convert a string with literal \uxxxx or \Uxxxxxxxx characters to UTF-8
   example: putf8_unescape(mybuf, 256, "hello\\u220e")
   note the double backslash is needed if called on a C string literal */
int putf8_unescape(char *buf, int sz, char *src)
{
    int c=0, amt;
    wchar_t ch;
    char temp[4];

    while (*src && c < sz) {
        if (*src == '\\') {
            src++;
            amt = putf8_read_escape_sequence(src, &ch);
        }
        else {
            ch = (wchar_t)*src;
            amt = 1;
        }
        src += amt;
        amt = putf8_wc_toutf8(temp, ch);
        if (amt > sz-c)
            break;
        memcpy(&buf[c], temp, amt);
        c += amt;
    }
    if (c < sz)
        buf[c] = '\0';
    return c;
}

int putf8_escape_wchar(char *buf, int sz, wchar_t ch)
{
    if (ch == L'\n')
        return snprintf(buf, sz, "\\n");
    else if (ch == L'\t')
        return snprintf(buf, sz, "\\t");
    else if (ch == L'\r')
        return snprintf(buf, sz, "\\r");
    else if (ch == L'\b')
        return snprintf(buf, sz, "\\b");
    else if (ch == L'\f')
        return snprintf(buf, sz, "\\f");
    else if (ch == L'\v')
        return snprintf(buf, sz, "\\v");
    else if (ch == L'\a')
        return snprintf(buf, sz, "\\a");
    else if (ch == L'\\')
        return snprintf(buf, sz, "\\\\");
    else if (ch < 32 || ch == 0x7f)
        return snprintf(buf, sz, "\\x%hhX", ch);
    #ifndef _WIN32
    #if UNICODE
    else if (ch > 0xFFFF)
        return snprintf(buf, sz, "\\U%.8X", (wchar_t)ch);
    else if (ch >= 0x80 && ch <= 0xFFFF)
        return snprintf(buf, sz, "\\u%.4hX", (unsigned short)ch);
	#endif
	#endif

    return snprintf(buf, sz, "%c", (char)ch);
}

int putf8_escape(char *buf, int sz, char *src, int escape_quotes)
{
    int c=0, i=0, amt;

    while (src[i] && c < sz) {
        if (escape_quotes && src[i] == '"') {
            amt = snprintf(buf, sz - c, "\\\"");
            i++;
        }
        else {
            amt = putf8_escape_wchar(buf, sz - c, putf8_nextchar(src, &i));
        }
        c += amt;
        buf += amt;
    }
    if (c < sz)
        *buf = '\0';
    return c;
}

char *putf8_strchr(char *s, wchar_t ch, int *charn)
{
    int i = 0, lasti=0;
    wchar_t c;

    *charn = 0;
    while (s[i]) {
        c = putf8_nextchar(s, &i);
        if (c == ch) {
            return &s[lasti];
        }
        lasti = i;
        (*charn)++;
    }
    return NULL;
}

char *putf8_memchr(char *s, wchar_t ch, size_t sz, int *charn)
{
    int i = 0, lasti=0;
    wchar_t c;
    int csz;

    *charn = 0;
    while (i < sz) {
        c = csz = 0;
        do {
            c <<= 6;
            c += s[i++];
            csz++;
        } while (i < sz && !putf8_isutf(s[i]));
        c -= offsetsFromUTF8[csz-1];

        if (c == ch) {
            return &s[lasti];
        }
        lasti = i;
        (*charn)++;
    }
    return NULL;
}

int putf8_is_locale_utf8(char *locale)
{
    /* this code based on libutf8 */
    const char* cp = locale;

    for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++) {
        if (*cp == '.') {
            const char* encoding = ++cp;
            for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++)
                ;
            if ((cp-encoding == 5 && !strncmp(encoding, "UTF-8", 5))
                || (cp-encoding == 4 && !strncmp(encoding, "utf8", 4)))
                return 1; /* it's UTF-8 */
            break;
        }
    }
    return 0;
}

/*TESTCASE:UTF-8 functions



void utf8_demo()
{
	char	str		[ 1024 ];
	char*	ptr;

	setlocale( LC_ALL, "" );

	strcpy( str, "Hällö ich bün ein StrÜng€!" );
	\*            0123456789012345678901234567890
	               0        1         2         3
	*\
	printf( "%ld %d\n", pstrlen( str ), putf8_strlen( str ) );

	putf8_unescape( str, sizeof( str ), "\\u20AC" );
	printf( ">%s< %d\n", str, putf8_char( str ) );
}
---
32 26
>€< 8364
*/


/*NO_DOC*/
/* No documentation for the entire module, all here is only used internally. */

/* For Debug */
void pregex_dfa_print( pregex_dfa* dfa )
{
	plistel*		e;
	plistel*		f;
	pregex_dfa_st*	s;
	pregex_dfa_tr*	t;
	int				i;

	plist_for( dfa->states, e )
	{
		s = (pregex_dfa_st*)plist_access( e );
		fprintf( stderr, "*** STATE %d, accepts %d, flags %d",
			plist_offset( plist_get_by_ptr( dfa->states, s ) ),
				s->accept, s->flags );

		if( s->refs )
		{
			fprintf( stderr, " refs" );

			for( i = 0; i < PREGEX_MAXREF; i++ )
				if( s->refs & ( 1 << i ) )
					fprintf( stderr, " %d", i );
		}

		fprintf( stderr, "\n" );

		plist_for( s->trans, f )
		{
			t = (pregex_dfa_tr*)plist_access( f );

			pccl_print( stderr, t->ccl, 0 );
			fprintf( stderr, "-> %d\n", t->go_to );
		}

		fprintf( stderr, "\n" );
	}
}

/* Sort transitions by characters */
static int pregex_dfa_sort_trans( plist* list, plistel* el, plistel* er )
{
	pregex_dfa_tr*	l = (pregex_dfa_tr*)plist_access( el );
	pregex_dfa_tr*	r = (pregex_dfa_tr*)plist_access( er );

	return pccl_compare( l->ccl, r->ccl );
}

/* Sort transitions by characters */
static int pregex_dfa_sort_classes( plist* list, plistel* el, plistel* er )
{
	pccl*	l = (pccl*)plist_access( el );
	pccl*	r = (pccl*)plist_access( er );

	return pccl_compare( l, r );
}

/* Creating a new DFA state */
static pregex_dfa_st* pregex_dfa_create_state( pregex_dfa* dfa )
{
	pregex_dfa_st* 	ptr;

	ptr = (pregex_dfa_st*)plist_malloc( dfa->states );

	ptr->trans = plist_create( sizeof( pregex_dfa_tr ), PLIST_MOD_RECYCLE );
	plist_set_sortfn( ptr->trans, pregex_dfa_sort_trans );

	return ptr;
}

/* Freeing a DFA-state */
static void pregex_dfa_delete_state( pregex_dfa_st* st )
{
	plistel*		e;
	pregex_dfa_tr*	tr;

	plist_for( st->trans, e )
	{
		tr = (pregex_dfa_tr*)plist_access( e );
		pccl_free( tr->ccl );
	}

	st->trans = plist_free( st->trans );
}

/** Allocates an initializes a new pregex_dfa-object for a deterministic
finite state automata that can be used for pattern matching. A DFA
is currently created out of an NFA.

The function pregex_dfa_free() shall be used to destruct a pregex_dfa-object. */
pregex_dfa* pregex_dfa_create( void )
{
	pregex_dfa*		dfa;

	dfa = (pregex_dfa*)pmalloc( sizeof( pregex_dfa ) );
	dfa->states = plist_create( sizeof( pregex_dfa_st ), PLIST_MOD_RECYCLE );

	return dfa;
}

/** Resets a DFA state machine.

The object //dfa// can still be used as fresh, empty object after reset. */
pboolean pregex_dfa_reset( pregex_dfa* dfa )
{
	plistel*		e;

	if( !( dfa ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	while( ( e = plist_first( dfa->states ) ) )
	{
		pregex_dfa_delete_state( (pregex_dfa_st*)plist_access( e ) );
		plist_remove( dfa->states, e );
	}

	return TRUE;
}

/** Frees and resets a DFA state machine.

//dfa// is the pointer to a DFA-machine to be reset.

Always returns (pregex_dfa*)NULL.
*/
pregex_dfa* pregex_dfa_free( pregex_dfa* dfa )
{
	if( !( dfa ) )
		return (pregex_dfa*)NULL;

	pregex_dfa_reset( dfa );

	plist_free( dfa->states );
	pfree( dfa );

	return (pregex_dfa*)NULL;
}

/** Performs a check on all DFA state transitions to figure out the default
transition for every dfa state. The default-transition is only filled, when any
character of the entire character-range results in a transition, and is set to
the transition with the most characters in its class.

For example, the regex "[^\"]*" causes a dfa-state with two transitions:
On '"', go to state x, on every character other than '"', go to state y.
y will be selected as default state.
*/
static void pregex_dfa_default_trans( pregex_dfa* dfa )
{
	plistel*		e;
	plistel*		f;
	pregex_dfa_st*	st;
	pregex_dfa_tr*	tr;
	size_t			max;
	size_t			all;
	size_t			cnt;

	PROC( "pregex_dfa_default_trans" );
	PARMS( "dfa", "%p", dfa );

	plist_for( dfa->states, e )
	{
		st = (pregex_dfa_st*)plist_access( e );

		if( !st->trans )
			continue;

		/* Sort transitions */
		plist_sort( st->trans );

		/* Find default transition */
		max = all = 0;
		plist_for( st->trans, f )
		{
			tr = (pregex_dfa_tr*)plist_access( f );

			if( max < ( cnt = pccl_count( tr->ccl ) ) )
			{
				max = cnt;
				st->def_trans = tr;
			}

			all += cnt;
		}

		if( all <= PCCL_MAX ) /* fixme... */
			st->def_trans = (pregex_dfa_tr*)NULL;
	}

	VOIDRET;
}

/** Collects all references by the NFA-states forming a DFA-state, and puts them
into an dynamically allocated array for later re-use.

//st// is the DFA-state, for which references shall be collected.
*/
static pboolean pregex_dfa_collect_ref( pregex_dfa_st* st, plist* nfa_set )
{
	plistel*		e;
	pregex_nfa_st*	nfa_st;

	PROC( "pregex_dfa_collect_ref" );

	/* Find out number of references */
	MSG( "Searching for references in the NFA transitions" );
	plist_for( nfa_set, e )
	{
		nfa_st = (pregex_nfa_st*)plist_access( e );
		st->refs |= nfa_st->refs;
	}

	RETURN( TRUE );
}

/* Checks for DFA-states with same NFA-epsilon transitions than the specified
	one within the DFA-state machine. If an equal item is found, the offset of
		that DFA-state is returned, else -1. */
static pregex_dfa_st* pregex_dfa_same_transitions(
							pregex_dfa* dfa, plist* trans, parray* sets )
{
	plistel*		e;
	pregex_dfa_st*	ptr;
	plist*			nfa_set;

	plist_for( dfa->states, e )
	{
		ptr = (pregex_dfa_st*)plist_access( e );

		nfa_set = *( (plist**)parray_get( sets, plist_offset( e ) ) );
		if( plist_diff( nfa_set, trans ) == 0 )
			return ptr;
	}

	return (pregex_dfa_st*)NULL;
}

/** Turns a NFA-state machine into a DFA-state machine using the
subset-construction algorithm.

//dfa// is the pointer to the DFA-machine that will be constructed by this
function. The pointer is set to zero before it is used.
//nfa// is the pointer to the NFA-Machine where the DFA-machine should be
constructed from.

Returns the number of DFA states that where constructed.
In case of an error, -1 is returned.
*/
int pregex_dfa_from_nfa( pregex_dfa* dfa, pregex_nfa* nfa )
{
	plist*			transitions;
	plist*			classes;
	plist*			done;
	parray*			sets;

	plist*			nfa_set;
	plist*			current_nfa_set;

	plistel*		e;
	plistel*		f;
	pregex_dfa_tr*	trans;
	pregex_dfa_st*	current;
	pregex_dfa_st*	st;
	pregex_nfa_st*	nfa_st;
	int				state_next	= 0;
	pboolean		changed;
	int				i;
	wchar_t			begin;
	wchar_t			end;
	pccl*			ccl;
	pccl*			test;
	pccl*			del;
	pccl*			subset;

	PROC( "pregex_dfa_from_nfa" );
	PARMS( "dfa", "%p", dfa );
	PARMS( "nfa", "%p", nfa );

	if( !( dfa && nfa ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	/* Initialize */
	classes = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	plist_set_sortfn( classes, pregex_dfa_sort_classes );

	transitions = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	done = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	sets = parray_create( sizeof( plist* ), 0 );

	/* Starting seed */

	if( !( current = pregex_dfa_create_state( dfa ) ) )
		RETURN( -1 );

	nfa_set = plist_create( 0, PLIST_MOD_PTR );
	plist_push( nfa_set, plist_access( plist_first( nfa->states ) ) );
	parray_push( sets, &nfa_set );

	pregex_nfa_epsilon_closure( nfa, nfa_set, (unsigned int*)NULL, (int*)NULL );
	pregex_dfa_collect_ref( current, nfa_set );

	/* Perform algorithm until all states are done */
	while( TRUE )
	{
		MSG( "WHILE" );

		/* Get next undone state */
		plist_for( dfa->states, e )
		{
			current = (pregex_dfa_st*)plist_access( e );
			if( !plist_get_by_ptr( done, current ) )
				break;
		}

		if( !e )
		{
			MSG( "No more undone states found" );
			break;
		}

		plist_push( done, current );
		current->accept = 0;
		current_nfa_set = *( (plist**)parray_get( sets, plist_offset( e ) ) );

		/* Assemble all character sets in the alphabet list */
		plist_erase( classes );

		plist_for( current_nfa_set, e )
		{
			nfa_st = (pregex_nfa_st*)plist_access( e );

			if( nfa_st->accept )
			{
				MSG( "NFA is an accepting state" );
				if( !current->accept || current->accept >= nfa_st->accept )
				{
					MSG( "Copying accept information" );
					current->accept = nfa_st->accept;
					current->flags = nfa_st->flags;
				}
			}

			/* Generate list of character classes */
			if( nfa_st->ccl )
			{
				VARS( "nfa_st->ccl", "%s", pccl_to_str( nfa_st->ccl, TRUE ) );
				MSG( "Adding character class to list" );
				if( !( ccl = pccl_dup( nfa_st->ccl ) ) )
					RETURN( -1 );

				if( !plist_push( classes, ccl ) )
					RETURN( -1 );

				VARS( "plist_count( classes )", "%d", plist_count( classes ) );
			}
		}

		VARS( "current->accept", "%d", current->accept );

		MSG( "Removing intersections within character classes" );
		do
		{
			changed = FALSE;

			plist_for( classes, e )
			{
				ccl = (pccl*)plist_access( e );

				plist_for( classes, f )
				{
					if( e == f )
						continue;

					test = (pccl*)plist_access( f );

					if( pccl_count( ccl ) > pccl_count( test ) )
						continue;

					if( ( subset = pccl_intersect( ccl, test ) ) )
					{
						test = pccl_diff( test, subset );
						pccl_free( subset );

						del = (pccl*)plist_access( f );
						pccl_free( del );

						plist_remove( classes, f );
						plist_push( classes, test );

						changed = TRUE;
					}
				}
			}

			VARS( "changed", "%s", BOOLEAN_STR( changed ) );
		}
		while( changed );

		/* Sort classes */
		plist_sort( classes );

		MSG( "Make transitions on constructed alphabet" );
		/* Make transitions on constructed alphabet */
		plist_for( classes, e )
		{
			ccl = (pccl*)plist_access( e );

			MSG( "Check char class" );
			for( i = 0; pccl_get( &begin, &end, ccl, i ); i++ )
			{
				VARS( "begin", "%d", begin );
				VARS( "end", "%d", end );

				plist_for( current_nfa_set, f )
				{
					if( !plist_push( transitions, plist_access( f ) ) )
						RETURN( -1 );
				}

				if( pregex_nfa_move( nfa, transitions, begin, end ) < 0 )
				{
					MSG( "pregex_nfa_move() failed" );
					break;
				}

				if( pregex_nfa_epsilon_closure( nfa, transitions,
								(unsigned int*)NULL, (int*)NULL ) < 0 )
				{
					MSG( "pregex_nfa_epsilon_closure() failed" );
					break;
				}

				if( !plist_count( transitions ) )
				{
					/* There is no move on this character! */
					MSG( "transition set is empty, will continue" );
					continue;
				}
				else if( ( st = pregex_dfa_same_transitions(
									dfa, transitions, sets ) ) )
				{
					MSG( "State with same transitions exists" );
					/* This transition is already existing in the DFA
						- discard the transition table! */
					plist_erase( transitions );
					state_next = plist_offset(
									plist_get_by_ptr( dfa->states, st ) );

					nfa_set = *( (plist**)parray_get( sets, state_next ) );
					pregex_dfa_collect_ref( st, nfa_set );
				}
				else
				{
					MSG( "Creating new DFA state" );
					/* Create a new DFA as undone with this transition! */
					if( !( st = pregex_dfa_create_state( dfa ) ) )
						RETURN( -1 );

					state_next = plist_count( dfa->states ) - 1;

					nfa_set = plist_dup( transitions );
					plist_erase( transitions );

					pregex_dfa_collect_ref( st, nfa_set );

					parray_push( sets, &nfa_set );
				}

				VARS( "state_next", "%d", state_next );

				/* Find transition entry with same follow state */
				plist_for( current->trans, f )
				{
					trans = (pregex_dfa_tr*)plist_access( f );

					if( trans->go_to == state_next )
						break;
				}

				VARS( "trans", "%p", trans );

				if( !f )
				{
					MSG( "Need to create new transition entry" );

					/* Set the transition into the transition state dfatab... */
					trans = plist_malloc( current->trans );
					trans->ccl = pccl_create( -1, -1, (char*)NULL );
					trans->go_to = state_next;
				}

				VARS( "Add range:begin", "%d", begin );
				VARS( "Add range:end", "%d", end );

				/* Append current range */
				if( !pccl_addrange( trans->ccl, begin, end ) )
					RETURN( -1 );
			}

			pccl_free( ccl );
		}
	}

	/* Clear temporary allocated memory */
	plist_free( done );
	plist_free( classes );
	plist_free( transitions );

	while( parray_count( sets ) )
		plist_free( *( (plist**)parray_pop( sets ) ) );

	parray_free( sets );

	/* Set default transitions */
	pregex_dfa_default_trans( dfa );

	RETURN( plist_count( dfa->states ) );
}

/** Checks for transition equality within two dfa states.

//dfa// is the pointer to DFA state machine.
//groups// is the list of DFA groups
//first// is the first state to check.
//second// is the Second state to check.

Returns TRUE if both states are totally equal, FALSE else.
*/
static pboolean pregex_dfa_equal_states(
	pregex_dfa* dfa, plist* groups,
		pregex_dfa_st* first, pregex_dfa_st* second )
{
	plistel*		e;
	plistel*		f;
	plistel*		g;
	pregex_dfa_tr*	tr	[ 2 ];

	PROC( "pregex_dfa_equal_states" );
	PARMS( "first", "%p", first );
	PARMS( "second", "%p", second );

	if( plist_count( first->trans ) != plist_count( second->trans ) )
	{
		MSG( "Number of transitions is different" );
		RETURN( FALSE );
	}

	for( e = plist_first( first->trans ), f = plist_first( second->trans );
			e && f; e = plist_next( e ), f = plist_next( f ) )
	{
		tr[0] = (pregex_dfa_tr*)plist_access( e );
		tr[1] = (pregex_dfa_tr*)plist_access( f );

		/* Equal Character class selection? */
		if( pccl_compare( tr[0]->ccl, tr[1]->ccl ) )
		{
			MSG( "Character classes are not equal" );
			RETURN( FALSE );
		}

		/* Search for goto-state group equality */
		first = (pregex_dfa_st*)plist_access(
					plist_get( dfa->states, tr[0]->go_to ) );
		second = (pregex_dfa_st*)plist_access(
					plist_get( dfa->states, tr[1]->go_to ) );

		plist_for( groups, g )
		{
			if( plist_get_by_ptr( (plist*)plist_access( g ), first )
				&& !plist_get_by_ptr( (plist*)plist_access( g ), second ) )
			{
				MSG( "Transition state are in different groups\n" );
				RETURN( FALSE );
			}
		}
	}

	RETURN( TRUE );
}

/** Minimizes a DFA to lesser states by grouping equivalent states to new
states, and transforming transitions to them.

//dfa// is the pointer to the DFA-machine that will be minimized. The content of
//dfa// will be replaced with the reduced machine.

Returns the number of DFA states that where constructed in the minimized version
of //dfa//. Returns -1 on error.
*/
int pregex_dfa_minimize( pregex_dfa* dfa )
{
	pregex_dfa_st*	dfa_st;
	pregex_dfa_st*	grp_dfa_st;
	pregex_dfa_tr*	ent;

	plist*			min_states;
	plist*			group;
	plist*			groups;
	plist*			newgroup;

	plistel*		e;
	plistel*		f;
	plistel*		g;
	plistel*		first;
	plistel*		next;
	plistel*		next_next;
	int				i;
	pboolean		changes		= TRUE;

	PROC( "pregex_dfa_minimize" );
	PARMS( "dfa", "%p", dfa );

	if( !dfa )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	groups = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );

	MSG( "First, all states are grouped by accepting id" );
	plist_for( dfa->states, e )
	{
		dfa_st = (pregex_dfa_st*)plist_access( e );

		plist_for( groups, f )
		{
			group = (plist*)plist_access( f );
			grp_dfa_st = (pregex_dfa_st*)plist_access( plist_first( group ) );

			if( grp_dfa_st->accept == dfa_st->accept )
				break;
		}

		if( !f )
		{
			group = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
			if( !plist_push( group, dfa_st ) )
				RETURN( -1 );

			if( !plist_push( groups, group ) )
				RETURN( -1 );
		}
		else if( !plist_push( group, dfa_st ) )
			RETURN( -1 );
	}

	MSG( "Perform the algorithm" );
	while( changes )
	{
		MSG( "LOOP" );
		VARS( "groups", "%d", plist_count( groups ) );
		changes = FALSE;

		plist_for( groups, e )
		{
			MSG( "Examining group" );
			newgroup = (plist*)NULL;

			group = (plist*)plist_access( e );

			first = plist_first( group );

			dfa_st = (pregex_dfa_st*)plist_access( first );
			next_next = plist_next( first );

			while( ( next = next_next ) )
			{
				next_next = plist_next( next );
				grp_dfa_st = (pregex_dfa_st*)plist_access( next );

				VARS( "dfa_st", "%d", plist_offset(
										plist_get_by_ptr(
											dfa->states, dfa_st ) ) );
				VARS( "grp_dfa_st", "%d", plist_offset(
										plist_get_by_ptr(
											dfa->states, grp_dfa_st ) ) );
				VARS( "dfa_st->trans", "%p", dfa_st->trans );
				VARS( "grp_dfa_st->trans", "%p", grp_dfa_st->trans );

				/* Check for state equality */
				if( !pregex_dfa_equal_states( dfa, groups,
							dfa_st, grp_dfa_st ) )
				{
					MSG( "States aren't equal" );
					MSG( "Removing state from current group" );
					plist_remove( group, plist_get_by_ptr(
											group, grp_dfa_st ) );

					if( !plist_count( group ) )
						plist_remove( groups,
							plist_get_by_ptr( groups, first ) );

					MSG( "Appending state into new group" );

					if( !newgroup )
					{
						if( !( newgroup =
								plist_create( 0,
									PLIST_MOD_PTR | PLIST_MOD_RECYCLE ) ) )
							RETURN( -1 );
					}

					if( !plist_push( newgroup, grp_dfa_st ) )
						RETURN( -1 );
				}
			}

			VARS( "newgroup", "%p", newgroup );
			if( newgroup )
			{
				MSG( "Engaging new group into list of groups" );
				if( !plist_push( groups, newgroup ) )
					RETURN( -1 );

				changes = TRUE;
			}
		}

		VARS( "changes", "%s", BOOLEAN_STR( changes ) );
	}

	/* Now that we have all groups, reduce each group to one state */
	plist_for( groups, e )
	{
		group = (plist*)plist_access( e );
		grp_dfa_st = (pregex_dfa_st*)plist_access( plist_first( group ) );

		plist_for( grp_dfa_st->trans, f )
		{
			ent = (pregex_dfa_tr*)plist_access( f );

			dfa_st = (pregex_dfa_st*)plist_access(
										plist_get( dfa->states, ent->go_to ) );

			for( g = plist_first( groups ), i = 0;
					g; g = plist_next( g ), i++ )
			{
				if( plist_get_by_ptr( (plist*)plist_access( g ), dfa_st ) )
				{
					ent->go_to = i;
					break;
				}
			}
		}
	}

	/* Put leading group states into new, minimized dfa state machine */
	min_states = plist_create( sizeof( pregex_dfa_st ), PLIST_MOD_RECYCLE );

	plist_for( groups, e )
	{
		group = (plist*)plist_access( e );

		grp_dfa_st = (pregex_dfa_st*)plist_access( plist_first( group ) );

		/* Delete all states except the first one in the group */
		for( f = plist_next( plist_first( group ) ); f; f = plist_next( f ) )
		{
			dfa_st = (pregex_dfa_st*)plist_access( f );
			grp_dfa_st->refs |= dfa_st->refs;
			pregex_dfa_delete_state( dfa_st );
		}

		/* Push the first one into the minimized list */
		plist_push( min_states, grp_dfa_st );

		/* Free this group */
		plist_free( group );
	}

	plist_free( groups );

	/* Replace states by minimized list */
	plist_free( dfa->states );
	dfa->states = min_states;

	/* Set default transitions */
	pregex_dfa_default_trans( dfa );

	RETURN( plist_count( dfa->states ) );
}

/* !!!OBSOLETE!!! */
/** Tries to match a pattern using a DFA state machine.

//dfa// is the DFA state machine to be executed.
//str// is the test string where the DFA should work on.
//len// is the length of the match, -1 on error or no match.

//flags// are the flags to modify the DFA state machine matching behavior.

Returns 0, if no match was found, else the id of the bestmost (=longest) match.
*/
int pregex_dfa_match( pregex_dfa* dfa, char* str, size_t* len,
		int* mflags, prange** ref, int* ref_count, int flags )
{
	pregex_dfa_st*	dfa_st;
	pregex_dfa_st*	next_dfa_st;
	pregex_dfa_st*	last_accept = (pregex_dfa_st*)NULL;
	pregex_dfa_tr*	ent;
	plistel*		e;
	char*			pstr		= str;
	size_t			plen		= 0;
	wchar_t			ch;

	PROC( "pregex_dfa_match" );
	PARMS( "dfa", "%p", dfa );

	if( flags & PREGEX_RUN_WCHAR )
		PARMS( "str", "%ls", str );
	else
		PARMS( "str", "%s", str );

	PARMS( "len", "%p", len );
	PARMS( "mflags", "%p", mflags );
	PARMS( "ref", "%p", ref );
	PARMS( "ref_count", "%p", ref_count );
	PARMS( "flags", "%d", flags );

	if( !( dfa && str && len ) )
	{
		WRONGPARAM;
		RETURN( 0 );
	}

	/* Initialize! */
	if( mflags )
		*mflags = PREGEX_FLAG_NONE;

	/*
	if( !pregex_ref_init( ref, ref_count, dfa->ref_count, flags ) )
		RETURN( 0 );
	*/

	*len = 0;
	dfa_st = (pregex_dfa_st*)plist_access( plist_first( dfa->states ) );

	while( dfa_st )
	{
		MSG( "At begin of loop" );

		VARS( "dfa_st->accept", "%d", dfa_st->accept );
		if( dfa_st->accept )
		{
			MSG( "This state has an accept" );
			last_accept = dfa_st;
			*len = plen;

			if(	( last_accept->flags & PREGEX_FLAG_NONGREEDY )
					|| ( flags & PREGEX_RUN_NONGREEDY ) )
			{
				MSG( "This match is not greedy, so matching will stop now" );
				break;
			}
		}

		/*
		MSG( "Handling References" );
		if( ref && dfa->ref_count && dfa_st->ref_cnt )
		{
			VARS( "Having ref_cnt", "%d", dfa_st->ref_cnt );

			for( i = 0; i < dfa_st->ref_cnt; i++ )
			{
				VARS( "i", "%d", i );
				VARS( "State", "%d", plist_offset(
										plist_get_by_ptr(
											dfa->states, dfa_st ) ) );
				VARS( "dfa_st->ref[i]", "%d", dfa_st->ref[ i ] );

				pregex_ref_update( &( ( *ref )[ dfa_st->ref[ i ] ] ),
									pstr, plen );
			}
		}
		*/

		/* Get next character */
		if( flags & PREGEX_RUN_WCHAR )
		{
			VARS( "pstr", "%ls", (wchar_t*)pstr );
			ch = *((wchar_t*)pstr);
			pstr += sizeof( wchar_t );

			if( flags & PREGEX_RUN_DEBUG )
				fprintf( stderr, "reading wchar_t >%lc< %d\n", ch, ch );
		}
		else
		{
			VARS( "pstr", "%s", pstr );
#ifdef UTF8
			ch = putf8_char( pstr );
			pstr += putf8_seqlen( pstr );
#else
			ch = *pstr++;
#endif

			if( flags & PREGEX_RUN_DEBUG )
				fprintf( stderr, "reading char >%c< %d\n", ch, ch );
		}

		VARS( "ch", "%d", ch );

		next_dfa_st = (pregex_dfa_st*)NULL;
		plist_for( dfa_st->trans, e )
		{
			ent = (pregex_dfa_tr*)plist_access( e );

			if( pccl_test( ent->ccl, ch ) )
			{
				MSG( "Having a character match!" );
				next_dfa_st = (pregex_dfa_st*)plist_access(
									plist_get( dfa->states, ent->go_to ) );
				break;
			}
		}

		if( !next_dfa_st )
		{
			MSG( "No transitions match!" );
			break;
		}

		plen++;
		VARS( "plen", "%ld", plen );

		dfa_st = next_dfa_st;
	}

	MSG( "Done scanning" );

	if( mflags && last_accept )
	{
		*mflags = last_accept->flags;
		VARS( "*mflags", "%d", *mflags );
	}

	VARS( "*len", "%d", *len );
	VARS( "last_accept->accept", "%d",
		( last_accept ? last_accept->accept : 0 ) );

	RETURN( ( last_accept ? last_accept->accept : 0 ) );
}

/* DFA Table Structure */

/** Compiles the significant state table of a pregex_dfa-structure into a
two-dimensional array //dfatab//.

//dfatab// is a pointer to a variable that receives the allocated DFA state machine, where
each row forms a state that is made up of columns described in the table below.

|| Column / Index | Content |
| 0 | Total number of columns in the current row |
| 1 | Match ID if > 0, or 0 if the state is not an accepting state |
| 2 | Match flags (anchors, greedyness, (PREGEX_FLAG_*)) |
| 3 | Reference flags; The index of the flagged bits defines the number of \
reference |
| 4 | Default transition from the current state. If there is no transition, \
its value is set to the number of all states. |
| 5 | Transition: from-character |
| 6 | Transition: to-character |
| 7 | Transition: Goto-state |
| ... | more triples follow for each transition |


Example for a state machine that matches the regular expression ``@[a-z0-9]+``
that has match 1 and no references:

```
8 0 0 0 3 64 64 2
11 1 0 0 3 48 57 1 97 122 1
11 0 0 0 3 48 57 1 97 122 1
```

Interpretation:

```
00: col= 8 acc= 0 flg= 0 ref= 0 def= 3 tra=064(@);064(@):02
01: col=11 acc= 1 flg= 0 ref= 0 def= 3 tra=048(0);057(9):01 tra=097(a);122(z):01
02: col=11 acc= 0 flg= 0 ref= 0 def= 3 tra=048(0);057(9):01 tra=097(a);122(z):01
```

A similar dump like this interpretation above will be printed to stderr by the
function when //dfatab// is provided as (long***)NULL.

The pointer assigned to //dfatab// must be freed after usage using a for-loop:

```
for( i = 0; i < dfatab_cnt; i++ )
	pfree( dfatab[i] );

pfree( dfatab );
```

The function returns the number of states of //dfa//, or -1 in error case.
*/
int pregex_dfa_to_dfatab( wchar_t*** dfatab, pregex_dfa* dfa )
{
	wchar_t**		trans;
	pregex_dfa_st*	st;
	pregex_dfa_tr*	tr;
	int				i;
	int				j;
	int				k;
	int				cnt;
	plistel*		e;
	plistel*		f;
	wchar_t			from;
	wchar_t			to;
	char			pr_from	[ 10 + 1 ];
	char			pr_to	[ 10 + 1 ];

	PROC( "pregex_dfa_to_dfatab" );
	PARMS( "dfatab", "%p", dfatab );
	PARMS( "dfa", "%p", dfa );

	if( !( dfa ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( trans = (wchar_t**)pmalloc( plist_count( dfa->states )
										* sizeof( wchar_t* ) ) ) )
		RETURN( -1 );

	for( i = 0, e = plist_first( dfa->states ); e; e = plist_next( e ), i++ )
	{
		VARS( "state( i )", "%d", i );
		st = (pregex_dfa_st*)plist_access( e );

		/*
			Row formatting:

			Number-of-columns;Accept;Flags;Ref-Flags;Default-Goto;

			Then, dynamic columns follow:
			Ref;...;From-Char;To-Char;Goto;...

			The rest consists of triples containing
			"From-Char;To-Char;Goto" each.
		*/

		MSG( "Examine required number of columns" );
		for( cnt = 5, f = plist_first( st->trans ); f; f = plist_next( f ) )
		{
			tr = (pregex_dfa_tr*)plist_access( f );
			if( st->def_trans == tr )
				continue;

			cnt += pccl_size( tr->ccl ) * 3;
		}

		VARS( "required( cnt )", "%d", cnt );

		trans[ i ] = (wchar_t*)pmalloc( cnt * sizeof( wchar_t ) );

		trans[ i ][ 0 ] = cnt;
		trans[ i ][ 1 ] = st->accept;
		trans[ i ][ 2 ] = st->flags;
		trans[ i ][ 3 ] = st->refs;
		trans[ i ][ 4 ] = st->def_trans ? st->def_trans->go_to :
												plist_count( dfa->states );

		MSG( "Fill columns" );
		for( j = 5, f = plist_first( st->trans ); f; f = plist_next( f ) )
		{
			tr = (pregex_dfa_tr*)plist_access( f );
			if( st->def_trans == tr )
				continue;

			for( k = 0; pccl_get( &from, &to, tr->ccl, k ); k++ )
			{
				trans[ i ][ j++ ] = from;
				trans[ i ][ j++ ] = to;
				trans[ i ][ j++ ] = tr->go_to;
			}
		}
	}

	/* Print it to stderr? */
	if( !dfatab )
	{
		for( i = 0; i < plist_count( dfa->states ); i++ )
		{
			fprintf( stderr, "%02d: col=%2d acc=%2d flg=%2x ref=%2d def=%2d",
								i, trans[i][0], trans[i][1], trans[i][2],
									trans[i][3], trans[i][4] );

			for( j = 5; j < trans[i][0]; j += 3 )
			{
				if( isprint( trans[i][j] ) )
					sprintf( pr_from, "(%lc)", trans[i][j] );
				else
					*pr_from = '\0';

				if( isprint( trans[i][j+1] ) )
					sprintf( pr_to, "(%lc)", trans[i][j+1] );
				else
					*pr_to = '\0';


				fprintf( stderr, " tra=%03d%s;%03d%s:%02d",
					trans[i][j], pr_from,
						trans[i][j+1], pr_to,
							trans[i][j+2] );
			}

			fprintf( stderr, "\n" );

			pfree( trans[i] );
		}

		pfree( trans );
	}
	else
		*dfatab = trans;

	RETURN( plist_count( dfa->states ) );
}


/*COD_ON*/



/** Performs a regular expression match on a string, and returns an array of
matches via prange-structures, which hold pointers to the begin- and
end-addresses of all matches.

//regex// is the regular expression pattern to be processed.

//str// is the string on which the pattern will be executed.

//flags// are for regular expression compile- and runtime-mode switching.
Several of them can be used with the bitwise or-operator (|).

//matches// is the array of results to the matched substrings within //str//,
provided as parray-object existing of one prange-object for every match.
It is optional. //matches// must be released with parray_free() after its usage.

Returns the number of matches, which is the number of result entries in the
returned array //matches//. If the value is negative, an error occurred.
*/
int pregex_qmatch( char* regex, char* str, int flags, parray** matches )
{
	int			count;
	pregex*		re;

	PROC( "pregex_qmatch" );
	PARMS( "regex", "%s", pstrget( regex ) );
	PARMS( "str", "%s", pstrget( str ) );
	PARMS( "flags", "%d", flags );
	PARMS( "matches", "%p", matches );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( re = pregex_create( regex, flags ) ) )
		RETURN( -1 );

	count = pregex_findall( re, str, matches );
	pregex_free( re );

	VARS( "count", "%d", count );
	RETURN( count );
}

/** Performs a regular expression search on a string and uses the expression as
separator; All strings that where split are returned as //matches//-array.

//regex// is the regular expression pattern to be processed.

//str// is the string on which the pattern will be executed.

//flags// are for regular expression compile- and runtime-mode switching.
Several of them can be used with the bitwise or-operator (|).

//matches// is the array of results to the matched substrings within //str//,
provided as parray-object existing of one prange-object for every match.
It is optional. //matches// must be released with parray_free() after its usage.

Returns the number of split substrings, which is the number of result entries in
the returned array //matches//. If the value is negative, an error occured.
*/
int pregex_qsplit( char* regex, char* str, int flags, parray** matches )
{
	int			count;
	pregex*		re;

	PROC( "pregex_qsplit" );
	PARMS( "regex", "%s", pstrget( regex ) );
	PARMS( "str", "%s", pstrget( str ) );
	PARMS( "flags", "%d", flags );
	PARMS( "matches", "%p", matches );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( re = pregex_create( regex, flags ) ) )
		RETURN( -1 );

	count = pregex_splitall( re, str, matches );
	pregex_free( re );

	VARS( "count", "%d", count );
	RETURN( count );
}

/** Replaces all matches of a regular expression pattern within a string with
the replacement. Backreferences can be used with ``$x`` for each opening bracket
within the regular expression.

//regex// is the regular expression pattern to be processed.

//str// is the string on which the pattern will be executed.

//replace// is the string that will be inserted as replacement for each pattern
match. ``$x`` back-references can be used.

//flags// are for regular expression compile- and runtime-mode switching.
Several of them can be used with the bitwise or-operator (|).

Returns an allocated pointer to the generated string with the replacements.
This string must be released after its existence is no longer required by the
caller using pfree().
*/
char* pregex_qreplace( char* regex, char* str, char* replace, int flags )
{
	char*			ret;
	pregex*			re;

	PROC( "pregex_qreplace" );
	PARMS( "regex", "%s", pstrget( regex ) );
	PARMS( "str", "%s", pstrget( str ) );
	PARMS( "replace", "%s", pstrget( replace ) );
	PARMS( "flags", "%d", flags );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

	if( !( re = pregex_create( regex, flags ) ) )
	{
		pregex_free( re );
		RETURN( (char*)NULL );
	}

	if( !( ret = pregex_replace( re, str, replace ) ) )
	{
		pregex_free( re );
		RETURN( (char*)NULL );
	}

	pregex_free( re );

	VARS( "ret", "%s", ret );
	RETURN( ret );
}


/** Constructor function to create a new plex object.

//flags// can be a combination of compile- and runtime-flags and are merged
with special compile-time flags provided for each pattern.

|| Flag | Usage |
| PREGEX_COMP_WCHAR | The regular expressions are provided as wchar_t. |
| PREGEX_COMP_NOANCHORS | Ignore anchor tokens, handle them as normal \
characters |
| PREGEX_COMP_NOREF | Don't compile references. |
| PREGEX_COMP_NONGREEDY | Compile all patterns to be forced nongreedy. |
| PREGEX_COMP_NOERRORS | Don't report errors, and try to compile as much as \
possible |
| PREGEX_COMP_INSENSITIVE | Parse regular expressions as case insensitive. |
| PREGEX_COMP_STATIC | The regular expressions passed should be converted 1:1 \
as if it were a string-constant. Any regex-specific symbols will be ignored \
and taken as if escaped. |
| PREGEX_RUN_WCHAR | Run regular expressions with wchar_t as input. |
| PREGEX_RUN_NOANCHORS | Ignore anchors while processing the lexer. |
| PREGEX_RUN_NOREF | Don't create references. |
| PREGEX_RUN_NONGREEDY | Force run lexer nongreedy. |
| PREGEX_RUN_DEBUG | Debug mode; output some debug info to stderr. |


On success, the function returns the allocated pointer to a plex-object.
This must be freed later using plex_free().
*/
plex* plex_create( int flags )
{
	plex*	lex;

	PROC( "plex_create" );
	PARMS( "flags", "%d", flags );

	lex = (plex*)pmalloc( sizeof( plex ) );
	lex->ptns = plist_create( 0, PLIST_MOD_PTR );
	lex->flags = flags;

	RETURN( lex );
}

/** Destructor function for a plex-object.

//lex// is the pointer to a plex-structure that will be released.

Always returns (plex*)NULL.
*/
plex* plex_free( plex* lex )
{
	plistel*	e;

	PROC( "plex_free" );
	PARMS( "lex", "%p", lex );

	if( !lex )
		RETURN( (plex*)NULL );

	plist_for( lex->ptns, e )
		pregex_ptn_free( (pregex_ptn*)plist_access( e ) );

	plist_free( lex->ptns );

	plex_reset( lex );
	pfree( lex );

	RETURN( (plex*)NULL );
}

/** Resets the DFA state machine of a plex-object //lex//. */
pboolean plex_reset( plex* lex )
{
	int		i;

	PROC( "plex_reset" );
	PARMS( "lex", "%p", lex );

	if( !lex )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* Drop out the dfatab */
	for( i = 0; i < lex->trans_cnt; i++ )
		pfree( lex->trans[ i ] );

	lex->trans_cnt = 0;
	lex->trans = pfree( lex->trans );

	RETURN( TRUE );
}

/** Prepares the DFA state machine of a plex-object //lex// for execution. */
pboolean plex_prepare( plex* lex )
{
	plistel*	e;
	pregex_nfa*	nfa;
	pregex_dfa*	dfa;

	PROC( "plex_prepare" );
	PARMS( "lex", "%p", lex );

	if( !lex )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !plist_count( lex->ptns ) )
	{
		MSG( "Can't construct a DFA from nothing!" );
		RETURN( FALSE );
	}

	plex_reset( lex );

	/* Create a NFA from patterns */
	nfa = pregex_nfa_create();

	plist_for( lex->ptns, e )
		if( !pregex_ptn_to_nfa( nfa, (pregex_ptn*)plist_access( e ) ) )
		{
			pregex_nfa_free( nfa );
			RETURN( FALSE );
		}

	/* Create a minimized DFA from NFA */
	dfa = pregex_dfa_create();

	if( pregex_dfa_from_nfa( dfa, nfa ) <= 0
			|| pregex_dfa_minimize( dfa ) <= 0 )
	{
		pregex_nfa_free( nfa );
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	pregex_nfa_free( nfa );

	/* Debug */
	if( lex->flags & PREGEX_RUN_DEBUG )
		pregex_dfa_to_dfatab( NULL, dfa );

	/* Compile significant DFA table into dfatab array */
	if( ( lex->trans_cnt = pregex_dfa_to_dfatab( &lex->trans, dfa ) ) <= 0 )
	{
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	pregex_dfa_free( dfa );

	RETURN( TRUE );
}

/** Defines and parses a regular expression pattern into the plex-object.

//pat// is the regular expression string, or a pointer to a pregex_ptn*
structure in case PREGEX_COMP_PTN is flagged.

//match_id// must be a token match ID, a value > 0. The lower the match ID is,
the higher precedence takes the appended expression when there are multiple
matches.

//flags// may ONLY contain compile-time flags, and is combined with the
compile-time flags of the plex-object provided at plex_create().

|| Flag | Usage |
| PREGEX_COMP_WCHAR | The regular expressions are provided as wchar_t. |
| PREGEX_COMP_NOANCHORS | Ignore anchor tokens, handle them as normal \
characters |
| PREGEX_COMP_NOREF | Don't compile references. |
| PREGEX_COMP_NONGREEDY | Compile all patterns to be forced nongreedy. |
| PREGEX_COMP_NOERRORS | Don't report errors, and try to compile as much as \
possible |
| PREGEX_COMP_INSENSITIVE | Parse regular expressions as case insensitive. |
| PREGEX_COMP_STATIC | The regular expressions passed should be converted 1:1 \
as if it were a string-constant. Any regex-specific symbols will be ignored \
and taken as if escaped. |
| PREGEX_COMP_PTN | The regular expression passed already is a pattern, and \
shall be integrated. |


Returns a pointer to the pattern object that just has been added. This allows
for changing e.g. the accept flag later on. In case of an error, the value
returned is NULL.
*/
pregex_ptn* plex_define( plex* lex, char* pat, int match_id, int flags )
{
	pregex_ptn*	ptn;

	PROC( "plex_define" );
	PARMS( "lex", "%p", lex );
	PARMS( "pat", "%s", flags & PREGEX_COMP_PTN ? "(PREGEX_COMP_PTN)" : pat );
	PARMS( "match_id", "%d", match_id );
	PARMS( "flags", "%d", flags );

	if( !( lex && pat && match_id > 0 ) )
	{
		WRONGPARAM;
		RETURN( (pregex_ptn*)NULL );
	}

	if( flags & PREGEX_COMP_PTN )
		ptn = pregex_ptn_dup( (pregex_ptn*)pat );
	else if( !pregex_ptn_parse( &ptn, pat, lex->flags | flags ) )
		RETURN( (pregex_ptn*)NULL );

	PARMS( "ptn->accept", "%p", ptn->accept );

	ptn->accept = match_id;
	plist_push( lex->ptns, ptn );

	plex_reset( lex );
	RETURN( ptn );
}


/** Performs a lexical analysis using the object //lex// on pointer //start//.

If a token can be matched, the function returns the related id of the matching
pattern, and //end// receives the pointer to the last matched character.

The function returns 0 in case that there was no direct match.
The function plex_next() ignores unrecognized symbols and directly moves to the
next matching pattern.
*/
int plex_lex( plex* lex, char* start, char** end )
{
	int		i;
	int		state		= 0;
	int		next_state;
	char*	match		= (char*)NULL;
	char*	ptr			= start;
	wchar_t	ch			= ' ';
	int		id			= 0;

	PROC( "plex_lex" );
	PARMS( "lex", "%p", lex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( lex && start ) )
	{
		WRONGPARAM;
		RETURN( 0 );
	}

	if( !lex->trans_cnt && !plex_prepare( lex ) )
		RETURN( 0 );

	memset( lex->ref, 0, PREGEX_MAXREF * sizeof( prange ) );

	while( ch && state >= 0 )
	{
		/* State accepts? */
		if( lex->trans[ state ][ 1 ] )
		{
			MSG( "This state accepts the input" );
			match = ptr;
			id = lex->trans[ state ][ 1 ];

			if( lex->flags & PREGEX_RUN_NONGREEDY
				|| lex->trans[ state ][ 2 ] & PREGEX_FLAG_NONGREEDY )
				break;
		}

		/* References */
		if( lex->trans[ state ][ 3 ] )
		{
			for( i = 0; i < PREGEX_MAXREF; i++ )
			{
				if( lex->trans[ state ][ 3 ] & ( 1 << i ) )
				{
					if( !lex->ref[ i ].start )
						lex->ref[ i ].start = ptr;

					lex->ref[ i ].end = ptr;
				}
			}
		}

		/* Get next character */
		if( lex->flags & PREGEX_RUN_WCHAR )
		{
			VARS( "pstr", "%ls", (wchar_t*)ptr );
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
			VARS( "pstr", "%s", ptr );

			if( ( lex->flags & PREGEX_RUN_UCHAR ) )
				ch = (unsigned char)*ptr++;
			else
			{
#ifdef UTF8
				ch = putf8_char( ptr );
				ptr += putf8_seqlen( ptr );
#else
				ch = *ptr++;
#endif
			}
		}

		/* Initialize default transition */
		next_state = lex->trans[ state ][ 4 ];

		/* Find transition according to current character */
		for( i = 5; i < lex->trans[ state ][ 0 ]; i += 3 )
		{
			if( lex->trans[ state ][ i ] <= ch
					&& lex->trans[ state ][ i + 1 ] >= ch )
			{
				next_state = lex->trans[ state ][ i + 2 ];
				break;
			}
		}

		if( next_state == lex->trans_cnt )
			break;

		if( lex->flags & PREGEX_RUN_DEBUG )
		{
			if( lex->flags & PREGEX_RUN_WCHAR )
				fprintf( stderr,
					"state %d, wchar_t %d (>%lc<), next state %d\n",
						state, ch, ch, next_state );
			else
				fprintf( stderr,
					"state %d, char %d (>%c<), next state %d\n",
						state, ch, ch, next_state );
		}

		state = next_state;
	}

	if( match )
	{
		if( end )
			*end = match;

		/*
		for( i = 0; i < PREGEX_MAXREF; i++ )
			if( lex->ref[ i ].start )
				fprintf( stderr, "%2d: >%.*s<\n",
					i, lex->ref[ i ].end - lex->ref[ i ].start,
						lex->ref[ i ].start );
		*/

		RETURN( id );
	}

	RETURN( 0 );
}


/** Performs lexical analysis using //lex// from begin of pointer //start//, to
the next matching token.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the plex-object).

If a token can be matched, the function returns the pointer to the position
where the match starts at. //id// receives the id of the matching pattern,
//end// receives the end pointer of the match, when provided. //id// and //end//
can be omitted by providing NULL-pointers.

The function returns (char*)NULL in case that there is no match.
*/
char* plex_next( plex* lex, char* start, unsigned int* id, char** end )
{
	wchar_t		ch;
	char*		ptr 	= start;
	char*		lptr;
	int			i;
	int			mid;

	PROC( "plex_next" );
	PARMS( "lex", "%p", lex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( lex && start ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

	if( !lex->trans_cnt && !plex_prepare( lex ) )
		RETURN( (char*)NULL );

	do
	{
		lptr = ptr;

		/* Get next character */
		if( lex->flags & PREGEX_RUN_WCHAR )
		{
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
#ifdef UTF8
			ch = putf8_char( ptr );
			ptr += putf8_seqlen( ptr );
#else
			ch = *ptr++;
#endif
		}

		if( !ch )
			break;

		/* Find a transition according to current character */
		for( i = 5; i < lex->trans[ 0 ][ 0 ]; i += 3 )
			if( ( ( lex->trans[ 0 ][ 4 ] < lex->trans_cnt )
					|| ( lex->trans[ 0 ][ i ] <= ch
							&& lex->trans[ 0 ][ i + 1 ] >= ch ) )
					&& ( mid = plex_lex( lex, lptr, end ) ) )
					{
						if( id )
							*id = mid;

						RETURN( lptr );
					}
	}
	while( ch );

	RETURN( (char*)NULL );
}

/** Tokenizes the string beginning at //start// using the lexical analyzer
//lex//.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the plex-object).

The function initializes and fills the array //matches//, if provided, with
items of size prange. It returns the total number of matches.
*/
size_t plex_tokenize( plex* lex, char* start, parray** matches )
{
	char*			end;
	unsigned int	id;
	prange*	r;

	PROC( "plex_tokenize" );
	PARMS( "lex", "%p", lex );
	PARMS( "start", "%s", start );
	PARMS( "matches", "%p", matches );

	if( !( lex && start ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( matches )
		*matches = (parray*)NULL;

	while( start && *start )
	{
		if( !( start = plex_next( lex, start, &id, &end ) ) )
			break;

		if( matches )
		{
			if( !*matches )
				*matches = parray_create( sizeof( prange ), 0 );

			r = (prange*)parray_malloc( *matches );
			r->id = id;
			r->start = start;
			r->end = end;
		}

		start = end;
	}

	RETURN( *matches ? parray_count( *matches ) : 0 );
}

/* plex to dot */

static void write_edge( FILE* stream, wchar_t from, wchar_t to )
{
	if( iswprint( from ) )
		fprintf( stream, "&#x%x;", from );
	else
		fprintf( stream, "0x%x", from );

	if( to != from )
	{
		if( iswprint( to ) )
			fprintf( stream, " - &#x%x;",to);
		else
			fprintf( stream, " - 0x%x",to);
	}
}

/** Dumps the DFA of a //lex// lexer object into a DOT-formatted graph output.

The graph can be made visible with tools like Graphviz
(http://www.graphviz.org/) and similar.

//stream// is the output stream to be used. This is stdout when NULL is
provided.

//lex// is the plex object, which DFA shall be dumped.
 */
void plex_dump_dot( FILE* stream, plex* lex )
{
	int i, j, dst;

	PROC( "plex_dump_dot" );
	PARMS( "stream", "%p", stream );
	PARMS( "lex", "%p", lex );

	if( !stream )
		stream = stderr;

	if( !lex )
	{
		WRONGPARAM;
		VOIDRET;
	}

	if( !lex->trans_cnt && !plex_prepare( lex ) )
		VOIDRET;

	/* write start of graph */
	fprintf( stream,"digraph {\n" );
	fprintf( stream, "  rankdir=LR;\n" );
	fprintf( stream, "  node [shape = circle];\n" );

	for( i = 0; i < lex->trans_cnt; i++ )
	{
		/* size = lex->trans[i][0]; */
		fprintf( stream, "  n%d [", i );

		/* change shape and label of node if state is a final state */
		if( lex->trans[i][1] > 0 )
			fprintf( stream, "shape=doublecircle," );

		fprintf( stream,
					"label = \" n%d\\nmatch_flags = %d\\nref_flags = %d\\n",
						i, lex->trans[i][2], lex->trans[i][3] );

		if( lex->trans[i][1] > 0 )
			fprintf( stream, "id = %d\\n", lex->trans[i][1] );

		fprintf( stream, "\"];\n" );

		/* default transition */
		if( lex->trans[i][4] != lex->trans_cnt )
			fprintf( stream, "  n%d -> n%d [style=bold];\n",
									i, lex->trans[i][4] );

		/* a state with size < 5 is a final state:
				it has no outgoing transitions */
		if( lex->trans[i][0] > 5 )
		{
			j = 5;

			while( TRUE )
			{
				fprintf( stream, "  n%d -> n%d [label = <",
										i, lex->trans[i][j+2] );

				write_edge( stream, lex->trans[i][j], lex->trans[i][j+1] );
				dst = lex->trans[i][j+2];

				j += 3;

				while( TRUE )
				{
					/*  no more transitions to write */
					if( j >= lex->trans[i][0] )
					{
						fprintf( stream, ">];\n" );
						break;
					}
					else if( lex->trans[i][j+2] == dst )
					{
						fprintf( stream, "<br/>" );
						write_edge( stream, lex->trans[i][j],
												lex->trans[i][j+1] );
						j += 3;
						continue;
					}

					/* no more transitions to write */
					fprintf( stream, ">];\n" );
					break;
				}

				if( j >= lex->trans[i][0] )
					break;
			}
		}
	}

	fprintf( stream, "}\n" );
	VOIDRET;
};


/** Initializes a lexer context //ctx// for lexer //lex//.

Lexer contexts are objects holding state and semantics information on a current
lexing process. */
plexctx* plexctx_init( plexctx* ctx, plex* lex )
{
	PROC( "plexctx_init" );
	PARMS( "ctx", "%p", ctx );
	PARMS( "lex", "%p", lex );

	if( !( ctx && lex ) )
	{
		WRONGPARAM;
		RETURN( (plexctx*)NULL );
	}

	memset( ctx, 0, sizeof( plexctx ) );
	ctx->lex = lex;

	RETURN( ctx );
}

/** Creates a new lexer context for lexer //par//.

lexer contexts are objects holding state and semantics information on a
current parsing process. */
plexctx* plexctx_create( plex* lex )
{
	plexctx*	ctx;

	PROC( "plexctx_create" );
	PARMS( "lex", "%p", lex );

	if( !lex )
	{
		WRONGPARAM;
		RETURN( (plexctx*)NULL );
	}

	ctx = (plexctx*)pmalloc( sizeof( plexctx ) );
	plexctx_init( ctx, lex );

	RETURN( ctx );
}

/** Resets the lexer context object //ctx//. */
plexctx* plexctx_reset( plexctx* ctx )
{
	if( !ctx )
		return (plexctx*)NULL;

	ctx->state = 0;
	ctx->handle = 0;

	return ctx;
}

/** Frees the lexer context object //ctx//. */
plexctx* plexctx_free( plexctx* ctx )
{
	if( !ctx )
		return (plexctx*)NULL;

	pfree( ctx );

	return (plexctx*)NULL;
}

/** Performs a lexical analysis using the object //lex// using context //ctx//
and character //ch//. */
pboolean plexctx_lex( plexctx* ctx, wchar_t ch )
{
	int		i;
	int		next_state;

	PROC( "plexctx_lex" );
	PARMS( "ctx", "%p", ctx );
	PARMS( "ch", "%d", ch );

	if( !( ctx && ctx->lex ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	if( !ctx->lex->trans_cnt && !plex_prepare( ctx->lex ) )
		RETURN( FALSE );

	if( !( ctx->state >= 0 && ctx->state < ctx->lex->trans_cnt ) )
	{
		MSG( "Invalid state" );
		RETURN( FALSE );
	}

	/* State accepts? */
	if( ctx->lex->trans[ ctx->state ][ 1 ] )
	{
		MSG( "This state accepts the input" );
		ctx->handle = ctx->lex->trans[ ctx->state ][ 1 ];

		if( ctx->lex->flags & PREGEX_RUN_NONGREEDY
			|| ctx->lex->trans[ ctx->state ][ 2 ] & PREGEX_FLAG_NONGREEDY )
			RETURN( FALSE );
	}

	/* References */
	/* todo
	if( lex->trans[ ctx->state ][ 3 ] )
	{
		for( i = 0; i < PREGEX_MAXREF; i++ )
		{
			if( lex->trans[ ctx->state ][ 3 ] & ( 1 << i ) )
			{
				if( !ctx->ref[ i ].start )
					ctx->ref[ i ].start = ptr;

				lex->ref[ i ].end = ptr;
			}
		}
	}
	*/

	/* Initialize default transition */
	next_state = ctx->lex->trans[ ctx->state ][ 4 ];

	/* Find transition according to current character */
	for( i = 5; i < ctx->lex->trans[ ctx->state ][ 0 ]; i += 3 )
	{
		if( ctx->lex->trans[ ctx->state  ][ i ] <= ch
				&& ctx->lex->trans[ ctx->state  ][ i + 1 ] >= ch )
		{
			next_state = ctx->lex->trans[ ctx->state ][ i + 2 ];
			break;
		}
	}

	if( next_state < ctx->lex->trans_cnt )
	{
		MSG( "Having transition" );

		if( ctx->lex->flags & PREGEX_RUN_WCHAR )
			LOG( "state %d, wchar_t %d (>%lc<), next state %d\n",
					ctx->state, ch, ch, next_state );
		else
			LOG( "state %d, char %d (>%c<), next state %d\n",
					ctx->state, ch, ch, next_state );
	}
	else
		MSG( "No more transitions" );

	ctx->state = next_state;

	RETURN( MAKE_BOOLEAN( ctx->state < ctx->lex->trans_cnt ) );
}


/*NO_DOC*/
/* No documentation for the entire module, all here is only interally used. */

/** Performs an anchor checking within a string. It is used by the internal
matching functions for NFA and DFA state machines.

//all// is the entire string. This can be equal to //str//, but is required to
perform valid line-begin anchor checking. If //all// is (char*)NULL, //str//
is assumed to be //all//.
//str// is the position pointer of the current match within //all//.
//len// is the length of the matched string, in characters.
//anchors// is the anchor configuration to be checked for the string.
//flags// is the flags configuration, e. g. for wide-character enabled anchor
checking.

Returns TRUE, if all anchors flagged as //anchors// match, else FALSE.
*/
pboolean pregex_check_anchors( char* all, char* str, size_t len,
										int anchors, int flags )
{
	wchar_t	ch;
	int		charsize = sizeof( char );

	PROC( "pregex_check_anchors" );
	if( flags & PREGEX_RUN_WCHAR )
	{
		PARMS( "all", "%ls", all );
		PARMS( "str", "%ls", str );
	}
	else
	{
		PARMS( "all", "%s", all );
		PARMS( "str", "%s", str );
	}
	PARMS( "anchors", "%d", anchors );
	PARMS( "flags", "%d", flags );

	/* Perform anchor checkings? */
	if( flags & PREGEX_RUN_NOANCHORS )
	{
		MSG( "Anchor checking is disabled through flags, or not required" );
		RETURN( TRUE );
	}

	/* Check parameter integrity */
	if( !( str || len ) )
	{
		MSG( "Not enough or wrong parameters!" );
		RETURN( FALSE );
	}

	if( !all )
		all = str;

	if( flags & PREGEX_RUN_WCHAR )
		charsize = sizeof( wchar_t );

	/* Begin of line anchor */
	if( anchors & PREGEX_FLAG_BOL )
	{
		MSG( "Begin of line anchor is set" );
		if( all < str )
		{
			if( flags & PREGEX_RUN_WCHAR )
			{
				VARS( "str-1", "%ls", (wchar_t*)( str - 1 ) );
				ch = *( (wchar_t*)( str - 1 ) );
			}
			else
			{
				VARS( "str-1", "%s", str-1 );
				ch = *( str - 1 );
			}

			VARS( "ch", "%lc", ch );
			if( ch != '\n' && ch != '\r' )
				RETURN( FALSE );
		}
	}

	/* End of Line anchor */
	if( anchors & PREGEX_FLAG_EOL )
	{
		MSG( "End of line anchor is set" );
		if( ( ch = *( str + ( len * charsize ) ) ) )
		{
			VARS( "ch", "%lc", ch );
			if( ch != '\n' && ch != '\r' )
				RETURN( FALSE );
		}
	}

	/* Begin of word anchor */
	if( anchors & PREGEX_FLAG_BOW )
	{
		MSG( "Begin of word anchor is set" );
		if( all < str )
		{
			if( flags & PREGEX_RUN_WCHAR )
			{
				VARS( "str-1", "%ls", (wchar_t*)( str - 1 ) );
				ch = *( (wchar_t*)( str - 1 ) );
			}
			else
			{
				VARS( "str-1", "%s", str-1 );
				ch = *( str - 1 );
			}

			VARS( "ch", "%lc", ch );
			if( iswalnum( ch ) || ch == '_' )
				RETURN( FALSE );
		}
	}

	/* End of word anchor */
	if( anchors & PREGEX_FLAG_EOW )
	{
		MSG( "End of word anchor is set" );
		if( ( ch = *( str + ( len * charsize ) ) ) )
		{
			VARS( "ch", "%lc", ch );
			if( iswalnum( ch ) || ch == '_' )
				RETURN( FALSE );
		}
	}

	MSG( "All anchor tests where fine!" );
	RETURN( TRUE );
}

/*COD_ON*/



/*NO_DOC*/
/* No documentation for the entire module, all here is only interally used. */

/** Creates a new NFA-state within an NFA state machine. The function first
checks if there are recyclable states in //nfa//. If so, the state is re-used
and re-configured, else a new state is allocated in memory.

//nfa// is the structure pointer to the NFA to process.
//chardef// is an optional charset definition for the new state. If this is
(char*)NULL, then a new epsilon state is created.
//flags// defines the modifier flags that belong to the chardef-parameter.

Returns a pointer to pregex_nfa_st, defining the newly created state. The
function returns (pregex_nfa_st*)NULL on error case.
*/
pregex_nfa_st* pregex_nfa_create_state(
	pregex_nfa* nfa, char* chardef, int flags )
{
	pregex_nfa_st* 	ptr;

	PROC( "pregex_nfa_create_state" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "chardef", "%s", chardef ? chardef : "NULL" );

	/* Get new element */
	ptr = plist_malloc( nfa->states );

	/* Define character edge? */
	if( chardef )
	{
		MSG( "Required to parse chardef" );
		VARS( "chardef", "%s", chardef );

		if( !( ptr->ccl = pccl_create( -1, -1, chardef ) ) )
		{
			MSG( "Out of memory error" );
			RETURN( (pregex_nfa_st*)NULL );
		}

		/* Is case-insensitive flag set? */
		if( flags & PREGEX_COMP_INSENSITIVE )
		{
			pccl*		iccl;
			int			i;
			wchar_t		ch;
			wchar_t		cch;

			iccl = pccl_dup( ptr->ccl );

			MSG( "PREGEX_COMP_INSENSITIVE set" );
			for( i = 0; pccl_get( &ch, (wchar_t*)NULL, ptr->ccl, i ); i++ )
			{
				VARS( "ch", "%d", ch );
#ifdef UNICODE
				if( iswupper( ch ) )
					cch = towlower( ch );
				else
					cch = towupper( ch );
#else
				if( isupper( ch ) )
					cch = tolower( ch );
				else
					cch = toupper( ch );
#endif
				VARS( "cch", "%d", cch );
				if( ch == cch )
					continue;

				if( !pccl_add( iccl, cch ) )
					RETURN( (pregex_nfa_st*)NULL );
			}

			pccl_free( ptr->ccl );
			ptr->ccl = iccl;
		}

		VARS( "ptr->ccl", "%p", ptr->ccl );
	}

	RETURN( ptr );
}

/** Prints an NFA for debug purposes on a output stream.

//stream// is the output stream where to print the NFA to.
//nfa// is the NFA state machine structure to be printed.
*/
void pregex_nfa_print( pregex_nfa* nfa )
{
	plistel*		e;
	pregex_nfa_st*	s;
	int				i;

	fprintf( stderr, " no next next2 accept flags refs\n" );
	fprintf( stderr, "----------------------------------------------------\n" );

	plist_for( nfa->states, e )
	{
		s = (pregex_nfa_st*)plist_access( e );

#define GETOFF( l, s ) 	(s) ? plist_offset( plist_get_by_ptr( l, s ) ) : -1

		fprintf( stderr, "#%2d %4d %5d %6d %5d",
			GETOFF( nfa->states, s ),
			GETOFF( nfa->states, s->next ),
			GETOFF( nfa->states, s->next2 ),
			s->accept, s->flags );

		for( i = 0; i < PREGEX_MAXREF; i++ )
			if( s->refs & ( 1 << i ) )
				fprintf( stderr, " %d", i );

		fprintf( stderr, "\n" );
#undef GETOFF

		if( s->ccl )
			pccl_print( stderr, s->ccl, 0 );

		fprintf( stderr, "\n\n" );
	}

	fprintf( stderr, "----------------------------------------------------\n" );
}

/* Function for deep debugging, not used now. */
/*
static void print_nfa_state_list( char* info, pregex_nfa* nfa, plist* list )
{
	plistel*		e;
	pregex_nfa_st*	st;

	plist_for( list, e )
	{
		st = (pregex_nfa_st*)plist_access( e );
		fprintf( stderr, "%s: #%d %s\n",
			info,
			plist_offset( plist_get_by_ptr( nfa->states, st ) ),
			st->ccl ? pccl_to_str( st->ccl, TRUE ) : "EPSILON" );
	}
}
*/

/** Allocates an initializes a new pregex_nfa-object for a nondeterministic
finite state automata that can be used for pattern matching or to construct
a determinisitic finite automata out of.

The function pregex_nfa_free() shall be used to destruct a pregex_dfa-object. */
pregex_nfa* pregex_nfa_create( void )
{
	pregex_nfa*		nfa;

	nfa = (pregex_nfa*)pmalloc( sizeof( pregex_nfa ) );
	nfa->states = plist_create( sizeof( pregex_nfa_st ), PLIST_MOD_RECYCLE );

	return nfa;
}

/** Reset a pregex_nfa state machine.

The object can still be used after reset. */
pboolean pregex_nfa_reset( pregex_nfa* nfa )
{
	pregex_nfa_st*	st;

	PROC( "pregex_nfa_free" );
	PARMS( "nfa", "%p", nfa );

	if( !nfa )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	MSG( "Resetting states" );
	while( plist_first( nfa->states ) )
	{
		st = (pregex_nfa_st*)plist_access( plist_first( nfa->states ) );
		pccl_free( st->ccl );

		plist_remove( nfa->states, plist_first( nfa->states ) );
	}

	RETURN( TRUE );
}

/** Releases a pregex_nfa state machine.

//nfa// is the pointer to the NFA state machine structure. All allocated
elements of this structure as well as the structure itself will be freed.

The function always returns (pregex_nfa*)NULL.
*/
pregex_nfa* pregex_nfa_free( pregex_nfa* nfa )
{
	PROC( "pregex_nfa_free" );
	PARMS( "nfa", "%p", nfa );

	if( !nfa )
		RETURN( (pregex_nfa*)NULL );

	MSG( "Clearing states" );
	pregex_nfa_reset( nfa );

	MSG( "Dropping memory" );
	plist_free( nfa->states );
	pfree( nfa );

	RETURN( (pregex_nfa*)NULL );
}

/** Performs a move operation on a given input character from a set of NFA
states.

//nfa// is the state machine.
//hits// must be the pointer to an inizialized list of pointers to
pregex_nfa_st* that both contains the input seed for the move operation and
also receives all states that can be reached on the given
character-range. States from the input configuration will be removed, states
from the move operation will be inserted.
//from// is the character-range begin from which the move-operation should be
processed on.
//to// is the character-range end until the move-operation should be processed.

Returns the number of elements in //result//, or -1 on error.
*/
int pregex_nfa_move( pregex_nfa* nfa, plist* hits, wchar_t from, wchar_t to )
{
	plistel*		first;
	plistel*		end;
	pregex_nfa_st*	st;

	PROC( "pregex_nfa_move" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "hits", "%p", hits );
	PARMS( "from", "%d", from );
	PARMS( "to", "%d", to );

	if( !( nfa && hits ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( !( end = plist_last( hits ) ) )
	{
		MSG( "Nothing to do, hits is empty" );
		RETURN( 0 );
	}

	/* Loop through the input items */
	do
	{
		first = plist_first( hits );
		st = (pregex_nfa_st*)plist_access( first );
		VARS( "st", "%p", st );

		/* Not an epsilon edge? */
		if( st->ccl )
		{
			/* Fine, test for range! */
			if( pccl_testrange( st->ccl, from, to ) )
			{
				MSG( "State matches range!" );
				plist_push( hits, st->next );
			}
		}

		plist_remove( hits, first );
	}
	while( first != end );

	VARS( "plist_count( hits )", "%d", plist_count( hits ) );
	RETURN( plist_count( hits ) );
}

/** Performs an epsilon closure from a set of NFA states.

//nfa// is the NFA state machine
//closure// is a list of input NFA states, which will be extended on the closure
after the function returned.
//accept// is the return pointer which receives possible information about a
pattern match. This parameter is optional, and can be left empty by providing
(unsigned int*)NULL.
//flags// is the return pointer which receives possible flagging about a
pattern match. This parameter is optional, and can be left empty by providing
(int*)NULL.

Returns a number of elements in //input//.
*/
int pregex_nfa_epsilon_closure( pregex_nfa* nfa, plist* closure,
									unsigned int* accept, int* flags )
{
	pregex_nfa_st*	top;
	pregex_nfa_st*	next;
	pregex_nfa_st*	last_accept	= (pregex_nfa_st*)NULL;
	plist*			stack;
	short			i;

	PROC( "pregex_nfa_epsilon_closure" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "closure", "%p", closure );
	PARMS( "accept", "%p", accept );

	if( !( nfa && closure ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( accept )
		*accept = 0;
	if( flags )
		*flags = 0;

	stack = plist_dup( closure );

	/* Loop through the items */
	while( plist_pop( stack, &top ) )
	{
		if( accept && top->accept
				&& ( !last_accept
					|| last_accept->accept > top->accept ) )
			last_accept = top;

		if( !top->ccl )
		{
			for( i = 0; i < 2; i++ )
			{
				next = ( !i ? top->next : top->next2 );
				if( next && !plist_get_by_ptr( closure, next ) )
				{
					plist_push( closure, next );
					plist_push( stack, next );
				}
			}
		}
		else if( top->next2 )
		{
			/* This may not happen! */
			fprintf( stderr,
				"%s, %d: Impossible character-node "
					"has two outgoing transitions!\n", __FILE__, __LINE__ );
			exit( 1 );
		}
	}

	stack = plist_free( stack );

	if( accept && last_accept )
	{
		*accept = last_accept->accept;
		VARS( "*accept", "%d", *accept );
	}

	if( flags && last_accept )
	{
		*flags = last_accept->flags;
		VARS( "*flags", "%d", *flags );
	}

	VARS( "Closed states", "%d", plist_count( closure ) );
	RETURN( plist_count( closure ) );
}

/* !!!OBSOLETE!!! */
/** Tries to match a pattern using a NFA state machine.

//nfa// is the NFA state machine to be executed.
//str// is a test string where the NFA should work on.
//len// receives the length of the match, -1 on error or no match.

//mflags// receives the match flags configuration of the matching state, if it
provides flags. If this is (int*)NULL, any match state configurations will
be ignored.

//ref// receives a return array of references; If this pointer is not NULL, the
function will allocate memory for a reference array. This array is only
allocated if the following dependencies are met:
# The NFA has references
# //ref_count// is zero
# //ref// points to a prange*

//ref_count// receives the number of references. This value MUST be zero, if the
function should allocate refs. A positive value indicates the number of elements
in //ref//, so the array can be re-used in multiple calls.

//flags// are the flags to modify the NFA state machine matching behavior.

Returns 0, if no match was found, else the number of the match that was found
relating to a pattern in //nfa//.
*/
int pregex_nfa_match( pregex_nfa* nfa, char* str, size_t* len, int* mflags,
		prange** ref, int* ref_count, int flags )
{
	plist*			res;
	char*			pstr		= str;
	int				plen		= 0;
	int				last_accept = 0;
	wchar_t			ch;
	unsigned int	accept;
	int				aflags;

	PROC( "pregex_nfa_match" );
	PARMS( "nfa", "%p", nfa );

	if( flags & PREGEX_RUN_WCHAR )
		PARMS( "str", "%ls", str );
	else
		PARMS( "str", "%s", str );

	PARMS( "len", "%p", len );
	PARMS( "mflags", "%p", mflags );
	PARMS( "ref", "%p", ref );
	PARMS( "ref_count", "%p", ref_count );
	PARMS( "flags", "%d", flags );

	if( !( nfa && str ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	/*
	if( !pregex_ref_init( ref, ref_count, nfa->ref_count, flags ) )
		RETURN( -1 );
	*/

	*len = 0;
	if( mflags )
		*mflags = PREGEX_FLAG_NONE;

	res = plist_create( 0, PLIST_MOD_PTR | PLIST_MOD_RECYCLE );
	plist_push( res, plist_access( plist_first( nfa->states ) ) );

	/* Run the engine! */
	while( plist_count( res ) )
	{
		MSG( "Performing epsilon closure" );
		if( pregex_nfa_epsilon_closure( nfa, res, &accept, &aflags ) < 0 )
		{
			MSG( "pregex_nfa_epsilon_closure() failed" );
			break;
		}

		MSG( "Handling References" );
		/*
		if( ref && nfa->ref_count )
		{
			plist_for( res, e )
			{
				st = (pregex_nfa_st*)plist_access( e );
				if( st->ref > -1 )
				{
					MSG( "Reference found" );
					VARS( "State", "%d",
							plist_offset(
								plist_get_by_ptr( nfa->states, st ) ) );
					VARS( "st->ref", "%d", st->ref );

					pregex_ref_update( &( ( *ref )[ st->ref ] ), pstr, plen );
				}
			}
		}
		*/

		VARS( "accept", "%d", accept );
		if( accept )
		{
			if( flags & PREGEX_RUN_DEBUG )
			{
				if( flags & PREGEX_RUN_WCHAR )
					fprintf( stderr, "accept %d, len %d >%.*ls<\n",
						accept, plen, plen, (wchar_t*)str );
				else
					fprintf( stderr, "accept %d, len %d >%.*s<\n",
						accept, plen, plen, str );
			}

			MSG( "New accepting state takes place!" );
			last_accept = accept;
			*len = plen;

			if( mflags )
				*mflags = aflags;

			VARS( "last_accept", "%d", last_accept );
			VARS( "*len", "%d", *len );

			if(	( aflags & PREGEX_FLAG_NONGREEDY )
					|| ( flags & PREGEX_RUN_NONGREEDY ) )
			{
				if( flags & PREGEX_RUN_DEBUG )
					fprintf( stderr, "greedy set, match terminates\n" );

				MSG( "Greedy is set, will stop recognition with this match" );
				break;
			}
		}

		if( flags & PREGEX_RUN_WCHAR )
		{
			MSG( "using wchar_t" );
			VARS( "pstr", "%ls", (wchar_t*)pstr );

			ch = *((wchar_t*)pstr);
			pstr += sizeof( wchar_t );

			if( flags & PREGEX_RUN_DEBUG )
				fprintf( stderr, "reading wchar_t >%lc< %d\n", ch, ch );
		}
		else
		{
			MSG( "using char" );
			VARS( "pstr", "%s", pstr );
#ifdef UTF8
			ch = putf8_char( pstr );
			pstr += putf8_seqlen( pstr );
#else
			ch = *pstr++;
#endif

			if( flags & PREGEX_RUN_DEBUG )
				fprintf( stderr, "reading char >%c< %d\n", ch, ch );
		}

		VARS( "ch", "%d", ch );
		VARS( "ch", "%lc", ch );

		if( pregex_nfa_move( nfa, res, ch, ch ) < 0 )
		{
			MSG( "pregex_nfa_move() failed" );
			break;
		}

		plen++;
		VARS( "plen", "%ld", plen );
	}

	plist_free( res );

	VARS( "*len", "%d", *len );
	VARS( "last_accept", "%d", last_accept );
	RETURN( last_accept );
}

/** Builds or extends a NFA state machine from a string. The string is simply
converted into a state machine that exactly matches the desired string.

//nfa// is the pointer to the NFA state machine to be extended.
//str// is the sequence of characters to be converted one-to-one into //nfa//.
//flags// are flags for regular expresson processing.
//acc// is the acceping identifier that is returned on pattern match.

Returns TRUE on success.
*/
pboolean pregex_nfa_from_string( pregex_nfa* nfa, char* str, int flags, int acc )
{
	pregex_nfa_st*	nfa_st;
	pregex_nfa_st*	append_to;
	pregex_nfa_st*	first_nfa_st;
	pregex_nfa_st*	prev_nfa_st;
	char*			pstr;
	wchar_t			ch;

	PROC( "pregex_nfa_from_string" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "str", "%s", str );
	PARMS( "flags", "%d", flags );
	PARMS( "acc", "%d", acc );

	if( !( nfa && str ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* For wide-character execution, copy string content */
	if( flags & PREGEX_COMP_WCHAR )
	{
		if( !( str = pwcs_to_str( (wchar_t*)str, FALSE ) ) )
			RETURN( FALSE );
	}

	/* Find node to integrate into existing machine */
	for( append_to = (pregex_nfa_st*)plist_access( plist_first( nfa->states ) );
		append_to && append_to->next2; append_to = append_to->next2 )
			; /* Find last first node ;) ... */

	/* Create first state - this is an epsilon node */
	if( !( first_nfa_st = prev_nfa_st =
			pregex_nfa_create_state( nfa, (char*)NULL, flags ) ) )
		RETURN( FALSE );

	for( pstr = str; *pstr; )
	{
		/* Then, create all states that form the string */
		MSG( "Adding new state" );
		VARS( "pstr", "%s", pstr );
		if( !( nfa_st = pregex_nfa_create_state(
				nfa, (char*)NULL, flags ) ) )
			RETURN( FALSE );

		ch = putf8_parse_char( &pstr );
		VARS( "ch", "%d", ch );

		nfa_st->ccl = pccl_create( -1, -1, (char*)NULL );

		if( !( nfa_st->ccl && pccl_add( nfa_st->ccl, ch ) ) )
			RETURN( FALSE );

		/* Is case-insensitive flag set? */
		if( flags & PREGEX_COMP_INSENSITIVE )
		{
#ifdef UNICODE
			MSG( "UNICODE mode, trying to convert" );
			if( iswupper( ch ) )
				ch = towlower( ch );
			else
				ch = towupper( ch );
#else
			MSG( "non UNICODE mode, trying to convert" );
			if( isupper( ch ) )
				ch = (char)tolower( (int)ch );
			else
				ch = (char)toupper( (int)ch );
#endif

			MSG( "Case-insensity set, new character evaluated is:" );
			VARS( "ch", "%d", ch );
			if( !pccl_add( nfa_st->ccl, ch ) )
				RETURN( FALSE );
		}

		prev_nfa_st->next = nfa_st;
		prev_nfa_st = nfa_st;
	}

	/* Add accepting node */
	VARS( "acc", "%d", acc );
	if( !( nfa_st = pregex_nfa_create_state( nfa,
			(char*)NULL, flags ) ) )
		RETURN( FALSE );

	nfa_st->accept = acc;
	prev_nfa_st->next = nfa_st;

	/* Append to existing machine, if required */
	VARS( "append_to", "%p", append_to );
	if( append_to )
		append_to->next2 = first_nfa_st;

	/* Free copied string, in wide character mode */
	if( flags & PREGEX_COMP_WCHAR )
		pfree( str );

	RETURN( TRUE );
}

/*COD_ON*/


/* Local prototypes */
static pboolean parse_alter( pregex_ptn** ptn, char** pstr, int flags );

/* Create a pattern object of type //type//; Internal constructor! */
static pregex_ptn* pregex_ptn_CREATE( pregex_ptntype type )
{
	pregex_ptn*		ptn;

	ptn = (pregex_ptn*)pmalloc( sizeof( pregex_ptn ) );
	ptn->type = type;

	return ptn;
}

/** Constructs and parses a new pregex_ptn-structure from //pat//.

This function is a shortcut for a call to pregex_ptn_parse().
pregex_ptn_create() directly takes //pat// as its input and returns the parsed
pregex_ptn structure which represents the internal representation of the
regular expression //pat//.

//flags// provides a combination of compile-time modifier flags
(PREGEX_COMP_...) if wanted, or 0 (PREGEX_FLAG_NONE) if no flags should be
used.

Returns an allocated pregex_ptn-node which must be freed using pregex_ptn_free()
when it is not used anymore.
*/
pregex_ptn* pregex_ptn_create( char* pat, int flags )
{
	pregex_ptn*		ptn;

	PROC( "pregex_ptn_create" );
	PARMS( "pat", "%s", pat );
	PARMS( "flags", "%d", flags );

	if( !( pat && *pat ) )
	{
		WRONGPARAM;
		RETURN( (pregex_ptn*)NULL );
	}

	if( !pregex_ptn_parse( &ptn, pat, flags ) )
	{
		MSG( "Parse error" );
		RETURN( (pregex_ptn*)NULL );
	}

	RETURN( ptn );
}

/** Constructs a character-class pattern.

//ccl// is the pointer to a character class. This pointer is not duplicated,
and will be directly assigned to the object.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_char( pccl* ccl )
{
	pregex_ptn*		pattern;

	if( !( ccl ) )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_CHAR );
	pattern->ccl = ccl;

	return pattern;
}

/** Constructs a pattern for a static string.

//str// is the input string to be converted.
//flags// are optional flags for wide-character support.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_string( char* str, int flags )
{
	char*		ptr;
	wchar_t		ch;
	pregex_ptn*	chr;
	pregex_ptn*	seq		= (pregex_ptn*)NULL;
	pccl*		ccl;

	PROC( "pregex_ptn_create_string" );
	PARMS( "str", "%s", str );
	PARMS( "flags", "%d", flags );

	/* Check parameters */
	if( !( str ) )
	{
		WRONGPARAM;
		RETURN( (pregex_ptn*)NULL );
	}

	/* Convert string to UTF-8, if in wide-character mode */
	if( flags & PREGEX_COMP_WCHAR )
	{
		if( !( str = pwcs_to_str( (wchar_t*)str, FALSE ) ) )
			RETURN( (pregex_ptn*)NULL );
	}

	/* Loop through the string */
	for( ptr = str; *ptr; )
	{
		VARS( "ptr", "%s", ptr );
		ch = putf8_parse_char( &ptr );

		VARS( "ch", "%d", ch );

		ccl = pccl_create( -1, -1, (char*)NULL );

		if( !( ccl && pccl_add( ccl, ch ) ) )
		{
			pccl_free( ccl );
			RETURN( (pregex_ptn*)NULL );
		}

		/* Is case-insensitive flag set? */
		if( flags & PREGEX_COMP_INSENSITIVE )
		{
#ifdef UNICODE
			MSG( "UNICODE mode, trying to convert" );
			if( iswupper( ch ) )
				ch = towlower( ch );
			else
				ch = towupper( ch );
#else
			MSG( "non UNICODE mode, trying to convert" );
			if( isupper( ch ) )
				ch = (char)tolower( (int)ch );
			else
				ch = (char)toupper( (int)ch );
#endif

			MSG( "Case-insensity set, new character evaluated is:" );
			VARS( "ch", "%d", ch );

			if( !pccl_add( ccl, ch ) )
			{
				pccl_free( ccl );
				RETURN( (pregex_ptn*)NULL );
			}
		}

		if( !( chr = pregex_ptn_create_char( ccl ) ) )
		{
			pccl_free( ccl );
			RETURN( (pregex_ptn*)NULL );
		}

		if( ! seq )
			seq = chr;
		else
			seq = pregex_ptn_create_seq( seq, chr, (pregex_ptn*)NULL );
	}

	/* Free duplicated string */
	if( flags & PREGEX_COMP_WCHAR )
		pfree( str );

	RETURN( seq );
}

/** Constructs a sub-pattern (like with parantheses).

//ptn// is the pattern that becomes the sub-ordered pattern.

Returns a pregex_ptn-node which can be child of another pattern construct
or part of a sequence.
*/
pregex_ptn* pregex_ptn_create_sub( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_SUB );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs a sub-pattern as backreference (like with parantheses).

//ptn// is the pattern that becomes the sub-ordered pattern.

Returns a pregex_ptn-node which can be child of another pattern construct
or part of a sequence.
*/
pregex_ptn* pregex_ptn_create_refsub( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_REFSUB );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs alternations of multiple patterns.

//left// is the first pattern of the alternation.
//...// are multiple pregex_ptn-pointers follow which become part of the
alternation. The last node must be specified as (pregex_ptn*)NULL.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence. If there is only //left// assigned without other alternation
patterns, //left// will be returned back.
*/
pregex_ptn* pregex_ptn_create_alt( pregex_ptn* left, ...  )
{
	pregex_ptn*		pattern;
	pregex_ptn*		alter;
	va_list			alt;

	if( !( left ) )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	va_start( alt, left );

	while( ( alter = va_arg( alt, pregex_ptn* ) ) )
	{
		pattern = pregex_ptn_CREATE( PREGEX_PTN_ALT );
		pattern->child[0] = left;
		pattern->child[1] = alter;

		left = pattern;
	}

	va_end( alt );

	return left;
}

/** Constructs a kleene-closure repetition, allowing for multiple or none
repetitions of the specified pattern.

//ptn// is the pattern that will be configured for kleene-closure.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_kle( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_KLE );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs an positive-closure, allowing for one or multiple specified
pattern.

//ptn// is the pattern to be configured for positive closure.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_pos( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_POS );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs an optional-closure, allowing for one or none specified pattern.

//ptn// is the pattern to be configured for optional closure.

Returns a pregex_ptn-node which can be child of another pattern construct or
part of a sequence.
*/
pregex_ptn* pregex_ptn_create_opt( pregex_ptn* ptn )
{
	pregex_ptn*		pattern;

	if( !ptn )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	pattern = pregex_ptn_CREATE( PREGEX_PTN_OPT );
	pattern->child[0] = ptn;

	return pattern;
}

/** Constructs a sequence of multiple patterns.

//first// is the beginning pattern of the sequence. //...// follows as parameter
list of multiple patterns that become part of the sequence. The last pointer
must be specified as (pregex_ptn*)NULL to mark the end of the list.

Always returns the pointer to //first//.
*/
pregex_ptn* pregex_ptn_create_seq( pregex_ptn* first, ... )
{
	pregex_ptn*		prev	= first;
	pregex_ptn*		next;
	va_list			chain;

	if( !first )
	{
		WRONGPARAM;
		return (pregex_ptn*)NULL;
	}

	va_start( chain, first );

	while( ( next = va_arg( chain, pregex_ptn* ) ) )
	{
		for( ; prev->next; prev = prev->next )
			;

		/* Alternatives must be put in brackets */
		if( next->type == PREGEX_PTN_ALT )
			next = pregex_ptn_create_sub( next );

		prev->next = next;
		prev = next;
	}

	va_end( chain );

	return first;
}

/** Duplicate //ptn// into a stand-alone 1:1 copy. */
pregex_ptn* pregex_ptn_dup( pregex_ptn* ptn )
{
	pregex_ptn*		start	= (pregex_ptn*)NULL;
	pregex_ptn*		prev	= (pregex_ptn*)NULL;
	pregex_ptn*		dup;

	PROC( "pregex_ptn_dup" );
	PARMS( "ptn", "%p", ptn );

	while( ptn )
	{
		dup = pregex_ptn_CREATE( ptn->type );

		if( !start )
			start = dup;
		else
			prev->next = dup;

		if( ptn->type == PREGEX_PTN_CHAR )
			dup->ccl = pccl_dup( ptn->ccl );

		if( ptn->child[0] )
			dup->child[0] = pregex_ptn_dup( ptn->child[0] );

		if( ptn->child[1] )
			dup->child[1] = pregex_ptn_dup( ptn->child[1] );

		dup->accept = ptn->accept;
		dup->flags = ptn->flags;

		prev = dup;
		ptn = ptn->next;
	}

	RETURN( start );
}

/** Releases memory of a pattern including all its subsequent and following
patterns.

//ptn// is the pattern object to be released.

Always returns (pregex_ptn*)NULL.
*/
pregex_ptn* pregex_ptn_free( pregex_ptn* ptn )
{
	pregex_ptn*		next;

	if( !ptn )
		return (pregex_ptn*)NULL;

	do
	{
		if( ptn->type == PREGEX_PTN_CHAR )
			pccl_free( ptn->ccl );

		if( ptn->child[0] )
			pregex_ptn_free( ptn->child[0] );

		if( ptn->child[1] )
			pregex_ptn_free( ptn->child[1] );

		pfree( ptn->str );

		next = ptn->next;
		pfree( ptn );

		ptn = next;
	}
	while( ptn );

	return (pregex_ptn*)NULL;
}

/** A debug function to print a pattern's hierarchical structure to stderr.

//ptn// is the pattern object to be printed.
//rec// is the recursion depth, set this to 0 at initial call.
*/
void pregex_ptn_print( pregex_ptn* ptn, int rec )
{
	int			i;
	char		gap		[ 100+1 ];
	static char	types[][20]	= { "PREGEX_PTN_NULL", "PREGEX_PTN_CHAR",
								"PREGEX_PTN_SUB", "PREGEX_PTN_REFSUB",
								"PREGEX_PTN_ALT",
								"PREGEX_PTN_KLE", "PREGEX_PTN_POS",
									"PREGEX_PTN_OPT"
							};

	for( i = 0; i < rec; i++ )
		gap[i] = '.';
	gap[i] = 0;

	do
	{
		fprintf( stderr, "%s%s", gap, types[ ptn->type ] );

		if( ptn->type == PREGEX_PTN_CHAR )
			fprintf( stderr, " %s", pccl_to_str( ptn->ccl, FALSE ) );

		fprintf( stderr, "\n" );

		if( ptn->child[0] )
			pregex_ptn_print( ptn->child[0], rec + 1 );

		if( ptn->child[1] )
		{
			if( ptn->type == PREGEX_PTN_ALT )
				fprintf( stderr, "%s\n", gap );

			pregex_ptn_print( ptn->child[1], rec + 1 );
		}

		ptn = ptn->next;
	}
	while( ptn );
}

/* Internal function for pregex_ptn_to_regex() */
static void pregex_char_to_REGEX( char* str, int size,
 				wchar_t ch, pboolean escape, pboolean in_range )
{
	if( ( !in_range && ( strchr( "|()[]*+?", ch ) || ch == '.' ) ) ||
			( in_range && ch == ']' ) )
		sprintf( str, "\\%c", (char)ch );
	else if( escape )
		putf8_escape_wchar( str, size, ch );
	else
		putf8_toutf8( str, size, &ch, 1 );
}

/* Internal function for pregex_ptn_to_regex() */
static void pccl_to_REGEX( char** str, pccl* ccl )
{
	int				i;
	wchar_t			cfrom;
	char			from	[ 40 + 1 ];
	wchar_t			cto;
	char			to		[ 20 + 1 ];
	pboolean		range	= FALSE;
	pboolean		neg		= FALSE;
	pboolean		is_dup	= FALSE;

	/*
	 * If this caracter class contains PCCL_MAX characters, then simply
	 * print a dot.
	 */
	if( pccl_count( ccl ) > PCCL_MAX )
	{
		*str = pstrcatchar( *str, '.' );
		return;
	}

	/* Better negate if more than 128 chars */
	if( pccl_count( ccl ) > 128 )
	{
		ccl = pccl_dup( ccl );
		is_dup = TRUE;

		neg = TRUE;
		pccl_negate( ccl );
	}

	/* Char-class or single char? */
	if( neg || pccl_count( ccl ) > 1 )
	{
		range = TRUE;
		*str = pstrcatchar( *str, '[' );
		if( neg )
			*str = pstrcatchar( *str, '^' );
	}

	/*
	 * If ccl contains a dash,
	 * this must be printed first!
	 */
	if( pccl_test( ccl, '-' ) )
	{
		if( !is_dup )
		{
			ccl = pccl_dup( ccl );
			is_dup = TRUE;
		}

		*str = pstrcatchar( *str, '-' );
		pccl_del( ccl, '-' );
	}

	/* Iterate ccl... */
	for( i = 0; pccl_get( &cfrom, &cto, ccl, i ); i++ )
	{
		pregex_char_to_REGEX( from, (int)sizeof( from ), cfrom, TRUE, range );

		if( cfrom < cto )
		{
			pregex_char_to_REGEX( to, (int)sizeof( to ), cto, TRUE, range );
			sprintf( from + strlen( from ), "%s%s",
				cfrom + 1 < cto ? "-" : "", to );
		}

		*str = pstrcatstr( *str, from, FALSE );
	}

	if( range )
		*str = pstrcatchar( *str, ']' );

	if( is_dup )
		pccl_free( ccl );
}

/* Internal function for pregex_ptn_to_regex() */
static pboolean pregex_ptn_to_REGEX( char** regex, pregex_ptn* ptn )
{
	if( !( regex && ptn ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	while( ptn )
	{
		switch( ptn->type )
		{
			case PREGEX_PTN_NULL:
				return TRUE;

			case PREGEX_PTN_CHAR:
				pccl_to_REGEX( regex, ptn->ccl );
				break;

			case PREGEX_PTN_SUB:
			case PREGEX_PTN_REFSUB:
				*regex = pstrcatchar( *regex, '(' );

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, ')' );
				break;

			case PREGEX_PTN_ALT:
				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '|' );

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 1 ] ) )
					return FALSE;

				break;

			case PREGEX_PTN_KLE:

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '*' );
				break;

			case PREGEX_PTN_POS:

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '+' );
				break;

			case PREGEX_PTN_OPT:

				if( !pregex_ptn_to_REGEX( regex, ptn->child[ 0 ] ) )
					return FALSE;

				*regex = pstrcatchar( *regex, '?' );
				break;

			default:
				MISSINGCASE;
				return FALSE;
		}

		ptn = ptn->next;
	}

	return TRUE;
}

/** Turns a regular expression pattern back into a regular expression string.

//ptn// is the pattern object to be converted into a regex.

The returned pointer is dynamically allocated but part of //ptn//, so it should
not be freed by the caller. It is automatically freed when the pattern object
is released.
*/
char* pregex_ptn_to_regex( pregex_ptn* ptn )
{
	if( !( ptn ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	ptn->str = pfree( ptn->str );

	if( !pregex_ptn_to_REGEX( &ptn->str, ptn ) )
	{
		ptn->str = pfree( ptn->str );
		return (char*)NULL;
	}

	return ptn->str;
}

/* Internal recursive processing function for pregex_ptn_to_nfa() */
static pboolean pregex_ptn_to_NFA( pregex_nfa* nfa, pregex_ptn* pattern,
	pregex_nfa_st** start, pregex_nfa_st** end, int* ref_count )
{
	pregex_nfa_st*	n_start	= (pregex_nfa_st*)NULL;
	pregex_nfa_st*	n_end	= (pregex_nfa_st*)NULL;
	int				ref		= 0;

	if( !( pattern && nfa && start && end ) )
	{
		WRONGPARAM;
		return FALSE;
	}

	*end = *start = (pregex_nfa_st*)NULL;

	while( pattern )
	{
		switch( pattern->type )
		{
			case PREGEX_PTN_NULL:
				return TRUE;

			case PREGEX_PTN_CHAR:
				n_start = pregex_nfa_create_state( nfa, (char*)NULL, 0 );
				n_end = pregex_nfa_create_state( nfa, (char*)NULL, 0 );

				n_start->ccl = pccl_dup( pattern->ccl );
				n_start->next = n_end;
				break;

			case PREGEX_PTN_REFSUB:
				if( !ref_count || ( ref = ++(*ref_count) ) > PREGEX_MAXREF )
					ref = 0;
				/* NO break! */

			case PREGEX_PTN_SUB:
				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 0 ], &n_start, &n_end, ref_count ) )
					return FALSE;

				if( ref )
				{
					n_start->refs |= 1 << ref;
					n_end->refs |= 1 << ref;
				}

				break;

			case PREGEX_PTN_ALT:
			{
				pregex_nfa_st*	a_start;
				pregex_nfa_st*	a_end;

				n_start = pregex_nfa_create_state( nfa, (char*)NULL, 0 );
				n_end = pregex_nfa_create_state( nfa, (char*)NULL, 0 );

				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 0 ], &a_start, &a_end, ref_count ) )
					return FALSE;

				n_start->next = a_start;
				a_end->next= n_end;

				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 1 ], &a_start, &a_end, ref_count ) )
					return FALSE;

				n_start->next2 = a_start;
				a_end->next= n_end;
				break;
			}

			case PREGEX_PTN_KLE:
			case PREGEX_PTN_POS:
			case PREGEX_PTN_OPT:
			{
				pregex_nfa_st*	m_start;
				pregex_nfa_st*	m_end;

				n_start = pregex_nfa_create_state( nfa, (char*)NULL, 0 );
				n_end = pregex_nfa_create_state( nfa, (char*)NULL, 0 );

				if( !pregex_ptn_to_NFA( nfa,
						pattern->child[ 0 ], &m_start, &m_end, ref_count ) )
					return FALSE;

				/* Standard chain linking */
				n_start->next = m_start;
				m_end->next = n_end;

				switch( pattern->type )
				{
					case PREGEX_PTN_KLE:
						/*
								 ____________________________
								|                            |
								|                            v
							n_start -> m_start -> m_end -> n_end
										  ^         |
										  |_________|
						*/
						n_start->next2 = n_end;
						m_end->next2 = m_start;
						break;

					case PREGEX_PTN_POS:
						/*
								n_start -> m_start -> m_end -> n_end
											  ^         |
											  |_________|
						*/
						m_end->next2 = m_start;
						break;

					case PREGEX_PTN_OPT:
						/*
									_____________________________
								   |                             |
								   |                             v
								n_start -> m_start -> m_end -> n_end
						*/
						n_start->next2 = n_end;
						break;

					default:
						break;
				}
				break;
			}

			default:
				MISSINGCASE;
				return FALSE;
		}

		if( ! *start )
		{
			*start = n_start;
			*end = n_end;
		}
		else
		{
			/*
				In sequences, when start has already been assigned,
				end will always be the pointer to an epsilon node.
				So the previous end can be replaced by the current
				start, to minimize the usage of states.

				This only if the state does not contain any additional
				informations (e.g. references).
			*/
			if( !( (*end)->refs ) )
			{
				memcpy( *end, n_start, sizeof( pregex_nfa_st ) );

				plist_remove( nfa->states,
					plist_get_by_ptr( nfa->states, n_start ) );
			}
			else
				(*end)->next = n_start;

			*end = n_end;
		}

		pattern = pattern->next;
	}

	return TRUE;
}

/** Converts a pattern-structure into a NFA state machine.

//nfa// is the NFA state machine structure that receives the compiled result of
the pattern. This machine will be extended to the pattern if it already contains
states. //nfa// must be previously initialized!

//ptn// is the pattern structure that will be converted and extended into
the NFA state machine.

//flags// are compile-time flags.

Returns TRUE on success.
*/
pboolean pregex_ptn_to_nfa( pregex_nfa* nfa, pregex_ptn* ptn )
{
	pregex_nfa_st*	start;
	pregex_nfa_st*	end;
	pregex_nfa_st*	first;
	pregex_nfa_st*	n_first;
	int				ref_count	= 0;

	PROC( "pregex_ptn_to_nfa" );
	PARMS( "nfa", "%p", nfa );
	PARMS( "ptn", "%p", ptn );

	if( !( nfa && ptn ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* Find last first node ;) ... */
	for( n_first = (pregex_nfa_st*)plist_access( plist_first( nfa->states ) );
		n_first && n_first->next2; n_first = n_first->next2 )
			;

	/* Create first epsilon node */
	if( !( first = pregex_nfa_create_state( nfa, (char*)NULL, 0 ) ) )
		RETURN( FALSE );

	/* Turn pattern into NFA */
	if( !pregex_ptn_to_NFA( nfa, ptn, &start, &end, &ref_count ) )
		RETURN( FALSE );

	/* start is next of first */
	first->next = start;

	/* Chaining into big state machine */
	if( n_first )
		n_first->next2 = first;

	/* end becomes the accepting state */
	end->accept = ptn->accept;
	end->flags = ptn->flags;

	RETURN( TRUE );
}

/** Converts a pattern-structure into a DFA state machine.

//dfa// is the DFA state machine structure that receives the compiled result of
the pattern. //dfa// must be initialized!
//ptn// is the pattern structure that will be converted and extended into
the DFA state machine.

Returns TRUE on success.
*/
pboolean pregex_ptn_to_dfa( pregex_dfa* dfa, pregex_ptn* ptn )
{
	pregex_nfa*	nfa;

	PROC( "pregex_ptn_to_dfa" );
	PARMS( "dfa", "%p", dfa );
	PARMS( "ptn", "%p", ptn );

	if( !( dfa && ptn ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	pregex_dfa_reset( dfa );
	nfa = pregex_nfa_create();

	if( !pregex_ptn_to_nfa( nfa, ptn ) )
	{
		pregex_nfa_free( nfa );
		RETURN( FALSE );
	}

	if( pregex_dfa_from_nfa( dfa, nfa ) < 0 )
	{
		pregex_nfa_free( nfa );
		RETURN( FALSE );
	}

	pregex_nfa_free( nfa );

	if( pregex_dfa_minimize( dfa ) < 0 )
	{
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	RETURN( TRUE );
}

/** Converts a pattern-structure into a DFA state machine //dfatab//.

//ptn// is the pattern structure that will be converted into a DFA state
machine.

//dfatab// is a pointer to a variable that receives the allocated DFA state machine, where
each row forms a state that is made up of columns described in the table below.

|| Column / Index | Content |
| 0 | Total number of columns in the current row |
| 1 | Match ID if > 0, or 0 if the state is not an accepting state |
| 2 | Match flags (anchors, greedyness, (PREGEX_FLAG_*)) |
| 3 | Reference flags; The index of the flagged bits defines the number of \
reference |
| 4 | Default transition from the current state. If there is no transition, \
its value is set to the number of all states. |
| 5 | Transition: from-character |
| 6 | Transition: to-character |
| 7 | Transition: Goto-state |
| ... | more triples follow for each transition |


Example for a state machine that matches the regular expression ``@[a-z0-9]+``
that has match 1 and no references:

```
8 0 0 0 3 64 64 2
11 1 0 0 3 48 57 1 97 122 1
11 0 0 0 3 48 57 1 97 122 1
```

Interpretation:

```
00: col= 8 acc= 0 flg= 0 ref= 0 def= 3 tra=064(@);064(@):02
01: col=11 acc= 1 flg= 0 ref= 0 def= 3 tra=048(0);057(9):01 tra=097(a);122(z):01
02: col=11 acc= 0 flg= 0 ref= 0 def= 3 tra=048(0);057(9):01 tra=097(a);122(z):01
```

A similar dump like this interpretation above will be printed to stderr by the
function when //dfatab// is provided as (long***)NULL.

The pointer assigned to //dfatab// must be freed after usage using a for-loop:

```
for( i = 0; i < dfatab_cnt; i++ )
	pfree( dfatab[i] );

pfree( dfatab );
```

Returns the number of rows in //dfatab//, or a negative value in error case.
*/
int pregex_ptn_to_dfatab( wchar_t*** dfatab, pregex_ptn* ptn )
{
	pregex_dfa*	dfa;
	int			states;

	PROC( "pregex_ptn_to_dfatab" );
	PARMS( "dfatab", "%p", dfatab );
	PARMS( "ptn", "%p", ptn );

	if( !( ptn ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	dfa = pregex_dfa_create();

	if( !pregex_ptn_to_dfa( dfa, ptn ) )
	{
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	if( ( states = pregex_dfa_to_dfatab( dfatab, dfa ) ) < 0 )
	{
		pregex_dfa_free( dfa );
		RETURN( FALSE );
	}

	pregex_dfa_free( dfa );

	VARS( "states", "%d", states );
	RETURN( states );
}

/** Parse a regular expression pattern string into a pregex_ptn structure.

//ptn// is the return pointer receiving the root node of the generated pattern.

//str// is the pointer to the string which contains the pattern to be parsed. If
PREGEX_COMP_WCHAR is assigned in //flags//, this pointer must be set to a
wchar_t-array holding a wide-character string.

//flags// provides compile-time modifier flags (PREGEX_COMP_...).

Returns TRUE on success.
*/
pboolean pregex_ptn_parse( pregex_ptn** ptn, char* str, int flags )
{
	char*			ptr;
	int				aflags	= 0;

	PROC( "pregex_ptn_parse" );
	PARMS( "ptn", "%p", ptn );
	PARMS( "str", "%s", str );
	PARMS( "flags", "%d", flags );

	if( !( ptn && str ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	/* If PREGEX_COMP_STATIC is set, parsing is not required! */
	if( flags & PREGEX_COMP_STATIC )
	{
		MSG( "PREGEX_COMP_STATIC" );

		if( !( *ptn = pregex_ptn_create_string( str, flags ) ) )
			RETURN( FALSE );

		RETURN( TRUE );
	}

	/* Copy input string - this is required,
		because of memory modification during the parse */
	if( flags & PREGEX_COMP_WCHAR )
	{
		if( !( ptr = str = pwcs_to_str( (wchar_t*)str, FALSE ) ) )
			RETURN( FALSE );
	}
	else
	{
		if( !( ptr = str = pstrdup( str ) ) )
			RETURN( FALSE );
	}

	VARS( "ptr", "%s", ptr );

	/* Parse anchor at begin of regular expression */
	if( !( flags & PREGEX_COMP_NOANCHORS ) )
	{
		MSG( "Anchors at begin" );

		if( *ptr == '^' )
		{
			aflags |= PREGEX_FLAG_BOL;
			ptr++;
		}
		else if( !strncmp( ptr, "\\<", 2 ) )
			/* This is a GNU-like extension */
		{
			aflags |= PREGEX_FLAG_BOW;
			ptr += 2;
		}
	}

	/* Run the recursive descent parser */
	MSG( "Starting the parser" );
	VARS( "ptr", "%s", ptr );

	if( !parse_alter( ptn, &ptr, flags ) )
		RETURN( FALSE );

	VARS( "ptr", "%s", ptr );

	/* Parse anchor at end of regular expression */
	if( !( flags & PREGEX_COMP_NOANCHORS ) )
	{
		MSG( "Anchors at end" );
		if( !strcmp( ptr, "$" ) )
			aflags |= PREGEX_FLAG_EOL;
		else if( !strcmp( ptr, "\\>" ) )
			/* This is a GNU-style extension */
			aflags |= PREGEX_FLAG_EOW;
	}

	/* Force nongreedy matching */
	if( flags & PREGEX_COMP_NONGREEDY )
		aflags |= PREGEX_FLAG_NONGREEDY;

	/* Free duplicated string */
	pfree( str );

	/* Assign flags */
	(*ptn)->flags = aflags;

	RETURN( TRUE );
}

/******************************************************************************
 *      RECURSIVE DESCENT PARSER FOR REGULAR EXPRESSIONS FOLLOWS HERE...      *
 ******************************************************************************/

static pboolean parse_char( pregex_ptn** ptn, char** pstr, int flags )
{
	pccl*		ccl;
	pregex_ptn*	alter;
	wchar_t		single;
	char		restore;
	char*		zero;
	pboolean	neg		= FALSE;

	PROC( "parse_char" );
	VARS( "**pstr", "%c", **pstr );

	switch( **pstr )
	{
		case '(':
			MSG( "Sub expression" );
			(*pstr)++;

			if( !parse_alter( &alter, pstr, flags ) )
				RETURN( FALSE );

			if( flags & PREGEX_COMP_NOREF &&
					!( *ptn = pregex_ptn_create_sub( alter ) ) )
				RETURN( FALSE );
			else if( !( *ptn = pregex_ptn_create_refsub( alter ) ) )
				RETURN( FALSE );

			/* Report error? */
			if( **pstr != ')' && !( flags & PREGEX_COMP_NOERRORS ) )
			{
				fprintf( stderr, "Missing closing bracket in regex\n" );
				RETURN( FALSE );
			}

			(*pstr)++;
			break;

		case '.':
			MSG( "Any character" );

			ccl = pccl_create( -1, -1, (char*)NULL );

			if( !( ccl && pccl_addrange( ccl, PCCL_MIN + 1, PCCL_MAX ) ) )
			{
				pccl_free( ccl );
				RETURN( FALSE );
			}

			if( !( *ptn = pregex_ptn_create_char( ccl ) ) )
			{
				pccl_free( ccl );
				RETURN( FALSE );
			}

			(*pstr)++;
			break;

		case '[':
			MSG( "Character-class definition" );

			/* Find next UNESCAPED! closing bracket! */
			for( zero = *pstr + 1; *zero && *zero != ']'; zero++ )
				if( *zero == '\\' && *(zero + 1) )
					zero++;

			if( *zero )
			{
				restore = *zero;
				*zero = '\0';

				if( (*pstr) + 1 < zero && *((*pstr)+1) == '^' )
				{
					neg = TRUE;
					(*pstr)++;
				}

				if( !( ccl = pccl_create( -1, -1, (*pstr) + 1 ) ) )
					RETURN( FALSE );

				if( neg )
					pccl_negate( ccl );

				if( !( *ptn = pregex_ptn_create_char( ccl ) ) )
				{
					pccl_free( ccl );
					RETURN( FALSE );
				}

				*zero = restore;
				*pstr = zero + 1;
				break;
			}
			/* No break here! */

		default:
			MSG( "Default case" );

			if( !( ccl = pccl_create( -1, -1, (char*)NULL ) ) )
			{
				OUTOFMEM;
				RETURN( FALSE );
			}

			if( !pccl_parseshorthand( ccl, pstr ) )
			{
				*pstr += pccl_parsechar( &single, *pstr, TRUE );

				MSG( "Singe character" );
				VARS( "single", "%c", single );
				VARS( "**pstr", "%c", **pstr );

				if( !pccl_add( ccl, single ) )
					RETURN( FALSE );
			}
			else
			{
				MSG( "Shorthand parsed" );
				VARS( "**pstr", "%c", **pstr );
			}

			VARS( "ccl", "%s", pccl_to_str( ccl, TRUE ) );

			if( !( *ptn = pregex_ptn_create_char( ccl ) ) )
			{
				pccl_free( ccl );
				RETURN( FALSE );
			}

			break;
	}

	RETURN( TRUE );
}

static pboolean parse_factor( pregex_ptn** ptn, char** pstr, int flags )
{
	PROC( "parse_factor" );

	if( !parse_char( ptn, pstr, flags ) )
		RETURN( FALSE );

	VARS( "**pstr", "%c", **pstr );

	switch( **pstr )
	{
		case '*':
		case '+':
		case '?':

			MSG( "Modifier matched" );

			switch( **pstr )
			{
				case '*':
					MSG( "Kleene star" );
					*ptn = pregex_ptn_create_kle( *ptn );
					break;
				case '+':
					MSG( "Positive plus" );
					*ptn = pregex_ptn_create_pos( *ptn );
					break;
				case '?':
					MSG( "Optional quest" );
					*ptn = pregex_ptn_create_opt( *ptn );
					break;

				default:
					break;
			}

			if( ! *ptn )
				RETURN( FALSE );

			(*pstr)++;
			break;

		default:
			break;
	}

	RETURN( TRUE );
}

static pboolean parse_sequence( pregex_ptn** ptn, char** pstr, int flags )
{
	pregex_ptn*	next;

	PROC( "parse_sequence" );

	if( !parse_factor( ptn, pstr, flags ) )
		RETURN( FALSE );

	while( !( **pstr == '|' || **pstr == ')' || **pstr == '\0' ) )
	{
		if( !( flags & PREGEX_COMP_NOANCHORS ) )
		{
			if( !strcmp( *pstr, "$" ) || !strcmp( *pstr, "\\>" ) )
				break;
		}

		if( !parse_factor( &next, pstr, flags ) )
			RETURN( FALSE );

		*ptn = pregex_ptn_create_seq( *ptn, next, (pregex_ptn*)NULL );
	}

	RETURN( TRUE );
}

static pboolean parse_alter( pregex_ptn** ptn, char** pstr, int flags )
{
	pregex_ptn*	seq;

	PROC( "parse_alter" );

	if( !parse_sequence( ptn, pstr, flags ) )
		RETURN( FALSE );

	while( **pstr == '|' )
	{
		MSG( "Alternative" );
		(*pstr)++;

		if( !parse_sequence( &seq, pstr, flags ) )
			RETURN( FALSE );

		if( !( *ptn = pregex_ptn_create_alt( *ptn, seq, (pregex_ptn*)NULL ) ) )
			RETURN( FALSE );
	}

	RETURN( TRUE );
}



/** Constructor function to create a new pregex object.

//pat// is a string providing a regular expression pattern.
//flags// can be a combination of compile- and runtime-flags.

|| Flag | Usage |
| PREGEX_COMP_WCHAR | The regular expression //pat// is provided as wchar_t. |
| PREGEX_COMP_NOANCHORS | Ignore anchor tokens, handle them as normal \
characters |
| PREGEX_COMP_NOREF | Don't compile references. |
| PREGEX_COMP_NONGREEDY | Compile regex to be forced non-greedy. |
| PREGEX_COMP_NOERRORS | Don't report errors, and try to compile as much as \
possible |
| PREGEX_COMP_INSENSITIVE | Parse regular expression as case insensitive. |
| PREGEX_COMP_STATIC | The regular expression passed should be converted 1:1 \
as it where a string-constant. Any regex-specific symbols will be ignored and \
taken as they where escaped. |
| PREGEX_RUN_WCHAR | Run regular expression with wchar_t as input. |
| PREGEX_RUN_NOANCHORS | Ignore anchors while processing the regex. |
| PREGEX_RUN_NOREF | Don't create references. |
| PREGEX_RUN_NONGREEDY | Force run regular expression non-greedy. |
| PREGEX_RUN_DEBUG | Debug mode; output some debug to stderr. |


On success, the function returns the allocated pointer to a pregex-object.
This must be freed later using pregex_free().
*/
pregex* pregex_create( char* pat, int flags )
{
	pregex*			regex;
	pregex_ptn*		ptn;

	PROC( "pregex_create" );
	PARMS( "pat", "%s", pat );
	PARMS( "flags", "%d", flags );

	if( !pregex_ptn_parse( &ptn, pat, flags ) )
		RETURN( (pregex*)NULL );

	ptn->accept = 1;

	regex = (pregex*)pmalloc( sizeof( pregex ) );
	regex->ptn = ptn;
	regex->flags = flags;

	/* Generate a dfatab */
	if( ( regex->trans_cnt = pregex_ptn_to_dfatab( &regex->trans, ptn ) ) < 0 )
		RETURN( pregex_free( regex ) );

	/* Print dfatab */
	/* pregex_ptn_to_dfatab( (wchar_t***)NULL, ptn ); */

	RETURN( regex );
}

/** Destructor function for a pregex-object.

//regex// is the pointer to a pregex-structure that will be released.

Returns always (pregex*)NULL.
*/
pregex* pregex_free( pregex* regex )
{
	int		i;

	PROC( "pregex_free" );
	PARMS( "regex", "%p", regex );

	if( !regex )
		RETURN( (pregex*)NULL );

	/* Freeing the pattern definitions */
	regex->ptn = pregex_ptn_free( regex->ptn );

	/* Drop out the dfatab */
	for( i = 0; i < regex->trans_cnt; i++ )
		pfree( regex->trans[ i ] );

	pfree( regex->trans );
	pfree( regex );

	RETURN( (pregex*)NULL );
}

/** Tries to match the regular expression //regex// at pointer //start//.

If the expression can be matched, the function returns TRUE and //end// receives
the pointer to the last matched character. */
pboolean pregex_match( pregex* regex, char* start, char** end )
{
	int		i;
	int		state		= 0;
	int		next_state;
	char*	match		= (char*)NULL;
	char*	ptr			= start;
	wchar_t	ch;

	PROC( "pregex_match" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( FALSE );
	}

	memset( regex->ref, 0, PREGEX_MAXREF * sizeof( prange ) );

	while( state >= 0 )
	{
		/* State accepts? */
		if( regex->trans[ state ][ 1 ] )
		{
			MSG( "This state accepts the input" );
			match = ptr;

			if( ( regex->flags & PREGEX_RUN_NONGREEDY
					|| regex->trans[ state ][ 2 ] & PREGEX_FLAG_NONGREEDY ) )
			{
				if( regex->flags & PREGEX_RUN_DEBUG )
					fprintf( stderr,
						"state %d accepted %d, end of recognition\n",
							state, regex->trans[ state ][ 1 ] );

				break;
			}
		}

		/* References */
		if( regex->trans[ state ][ 3 ] )
		{
			for( i = 0; i < PREGEX_MAXREF; i++ )
			{
				if( regex->trans[ state ][ 3 ] & ( 1 << i ) )
				{
					if( !regex->ref[ i ].start )
						regex->ref[ i ].start = ptr;

					regex->ref[ i ].end = ptr;
				}
			}
		}

		/* Get next character */
		if( regex->flags & PREGEX_RUN_WCHAR )
		{
			VARS( "pstr", "%ls", (wchar_t*)ptr );
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
			VARS( "pstr", "%s", ptr );

			if( ( regex->flags & PREGEX_RUN_UCHAR ) )
				ch = (unsigned char)*ptr++;
			else
			{
#ifdef UTF8
				ch = putf8_char( ptr );
				ptr += putf8_seqlen( ptr );
#else
				ch = *ptr++;
#endif
			}
		}

		if( !ch )
			break;

		/* Initialize default transition */
		next_state = regex->trans[ state ][ 4 ];

		/* Find transition according to current character */
		for( i = 5; i < regex->trans[ state ][ 0 ]; i += 3 )
		{
			if( regex->trans[ state ][ i ] <= ch
					&& regex->trans[ state ][ i + 1 ] >= ch )
			{
				next_state = regex->trans[ state ][ i + 2 ];
				break;
			}
		}

		if( next_state == regex->trans_cnt )
			break;

		if( regex->flags & PREGEX_RUN_DEBUG )
		{
			if( regex->flags & PREGEX_RUN_WCHAR )
				fprintf( stderr,
					"state %d, wchar_t %d (>%lc<), next state %d\n",
						state, ch, ch, next_state );
			else
				fprintf( stderr,
					"state %d, char %d (>%c<), next state %d\n",
						state, ch, ch, next_state );
		}

		state = next_state;
	}

	if( match )
	{
		if( end )
			*end = match;

		/*
		for( i = 0; i < PREGEX_MAXREF; i++ )
			if( regex->ref[ i ].start )
				fprintf( stderr, "%2d: >%.*s<\n",
					i, regex->ref[ i ].end - regex->ref[ i ].start,
						regex->ref[ i ].start );
		*/

		RETURN( TRUE );
	}

	RETURN( FALSE );
}

/** Find a match for the regular expression //regex// from begin of pointer
//start//.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).

If the expression can be matched, the function returns the pointer to the
position where the match begins. //end// receives the end pointer of the match,
when provided.

The function returns (char*)NULL in case that there is no match.
*/
char* pregex_find( pregex* regex, char* start, char** end )
{
	wchar_t		ch;
	char*		ptr 	= start;
	char*		lptr;
	int			i;

	PROC( "pregex_find" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

	do
	{
		lptr = ptr;

		/* Get next character */
		if( regex->flags & PREGEX_RUN_WCHAR )
		{
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
			if( ( regex->flags & PREGEX_RUN_UCHAR ) )
				ch = (unsigned char)*ptr++;
			else
			{
#ifdef UTF8
				ch = putf8_char( ptr );
				ptr += putf8_seqlen( ptr );
#else
				ch = *ptr++;
#endif
			}
		}

		if( !ch )
			break;

		/* Find a transition according to current character */
		for( i = 5; i < regex->trans[ 0 ][ 0 ]; i += 3 )
			if( ( ( regex->trans[ 0 ][ 4 ] < regex->trans_cnt )
					|| ( regex->trans[ 0 ][ i ] <= ch
							&& regex->trans[ 0 ][ i + 1 ] >= ch ) )
					&& pregex_match( regex, lptr, end ) )
						RETURN( lptr );
	}
	while( ch );

	RETURN( (char*)NULL );
}

/** Find all matches for the regular expression //regex// from begin of pointer
//start//, and optionally return matches as an array.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).

The function fills the array //matches//, if provided, with items of size
prange. It returns the total number of matches.
*/
int pregex_findall( pregex* regex, char* start, parray** matches )
{
	char*			end;
	int				count	= 0;
	prange*	r;

	PROC( "pregex_findall" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "matches", "%p", matches );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( matches )
		*matches = (parray*)NULL;

	while( ( start = pregex_find( regex, start, &end ) ) )
	{
		if( matches )
		{
			if( ! *matches )
				*matches = parray_create( sizeof( prange ), 0 );

			r = (prange*)parray_malloc( *matches );
			r->id = 1;
			r->start = start;
			r->end = end;
		}

		start = end;
		count++;
	}

	RETURN( count );
}

/** Returns the range between string //start// and the next match of //regex//.

This function can be seen as a "negative match", so the substrings that are
not part of the match will be returned.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).
//end// receives the last position of the string before the regex.
//next// receives the pointer of the next split element behind the matched
substring, so //next// should become the next //start// when pregex_split() is
called in a loop.

The function returns (char*)NULL in case there is no more string to split, else
it returns //start//.
*/
char* pregex_split( pregex* regex, char* start, char** end, char** next )
{
	wchar_t		ch;
	char*		ptr 	= start;
	char*		lptr;
	int			i;

	PROC( "pregex_split" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "end", "%p", end );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

	if( next )
		*next = (char*)NULL;

	do
	{
		lptr = ptr;

		/* Get next character */
		if( regex->flags & PREGEX_RUN_WCHAR )
		{
			ch = *( (wchar_t*)ptr );
			ptr += sizeof( wchar_t );
		}
		else
		{
			if( ( regex->flags & PREGEX_RUN_UCHAR ) )
				ch = (unsigned char)*ptr++;
			else
			{
#ifdef UTF8
				ch = putf8_char( ptr );
				ptr += putf8_seqlen( ptr );
#else
				ch = *ptr++;
#endif
			}
		}

		if( !ch )
			break;

		/* Find a transition according to current character */
		for( i = 5; i < regex->trans[ 0 ][ 0 ]; i += 3 )
			if( regex->trans[ 0 ][ i ] <= ch
				&& regex->trans[ 0 ][ i + 1 ] >= ch
					&& pregex_match( regex, lptr, next ) )
						ch = 0;
	}
	while( ch );

	if( lptr > start )
	{
		if( end )
			*end = lptr;

		RETURN( start );
	}

	RETURN( (char*)NULL );
}

/** Split a string at all matches of the regular expression //regex// from
begin of pointer //start//, and optionally returns the substrings found as an
array.

//start// has to be a zero-terminated string or wide-character string (according
to the configuration of the pregex-object).

The function fills the array //matches//, if provided, with items of size
prange. It returns the total number of matches.
*/
int pregex_splitall( pregex* regex, char* start, parray** matches )
{
	char*			end;
	char*			next;
	int				count	= 0;
	prange*	r;

	PROC( "pregex_splitall" );
	PARMS( "regex", "%p", regex );
	PARMS( "start", "%s", start );
	PARMS( "matches", "%p", matches );

	if( !( regex && start ) )
	{
		WRONGPARAM;
		RETURN( -1 );
	}

	if( matches )
		*matches = (parray*)NULL;

	while( start )
	{
		if( pregex_split( regex, start, &end, &next ) && matches )
		{
			if( ! *matches )
				*matches = parray_create( sizeof( prange ), 0 );

			r = (prange*)parray_malloc( *matches );
			r->start = start;
			r->end = end;
		}

		count++;

		start = next;
	}

	RETURN( count );
}

/** Replaces all matches of a regular expression object within a string //str//
with //replacement//. Backreferences in //replacement// can be used with //$x//
for each opening bracket within the regular expression.

//regex// is the pregex-object used for pattern matching.
//str// is the string on which //regex// will be executed.
//replacement// is the string that will be inserted as the replacement for each
match of a pattern described in //regex//. The notation //$x// can be used for
backreferences, where x is the offset of opening brackets in the pattern,
beginning at 1.

The function returns the string with the replaced elements, or (char*)NULL
in case of an error.
*/
char* pregex_replace( pregex* regex, char* str, char* replacement )
{
	prange*	refer;
	char*			ret			= (char*)NULL;
	char*			sstart		= str;
	char*			start;
	char*			end;
	char*			replace;
	char*			rprev;
	char*			rpstr;
	char*			rstart;
	int				ref;

	PROC( "pregex_replace" );
	PARMS( "regex", "%p", regex );

	if( !( regex && str ) )
	{
		WRONGPARAM;
		RETURN( (char*)NULL );
	}

#ifdef DEBUG
	if( regex && regex->flags & PREGEX_RUN_WCHAR )
	{
		PARMS( "str", "%ls", pstrget( str ) );
		PARMS( "replacement", "%ls", pstrget( replacement ) );
	}
	else
	{
		PARMS( "str", "%s", pstrget( str ) );
		PARMS( "replacement", "%s", pstrget( replacement ) );
	}
#endif

	MSG( "Starting loop" );

	while( TRUE )
	{
		if( !( start = pregex_find( regex, sstart, &end ) ) )
		{
			if( regex->flags & PREGEX_RUN_WCHAR )
				start = (char*)( (wchar_t*)sstart +
									wcslen( (wchar_t*)sstart ) );
			else
				start = sstart + strlen( sstart );

			end = (char*)NULL;
		}

		if( start > sstart )
		{
			MSG( "Extending string" );
			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				if( !( ret = (char*)pwcsncatstr(
								(wchar_t*)ret, (wchar_t*)sstart,
									( (wchar_t*)start - (wchar_t*)sstart ) ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
			else
			{
				if( !( ret = pstrncatstr( ret, sstart, start - sstart ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
		}

		if( !end )
			break;

		MSG( "Constructing a replacement" );
		if( !( regex->flags & PREGEX_RUN_NOREF ) )
		{
			rprev = rpstr = replacement;
			replace = (char*)NULL;

			while( *rpstr )
			{
				VARS( "*rpstr", "%c", *rpstr );
				if( *rpstr == '$' )
				{
					rstart = rpstr;

					if( regex->flags & PREGEX_RUN_WCHAR )
					{
						wchar_t*		_end;
						wchar_t*		_rpstr = (wchar_t*)rpstr;

						MSG( "Switching to wide-character mode" );

						if( iswdigit( *( ++_rpstr ) ) )
						{
							ref = wcstol( _rpstr, &_end, 0 );

							VARS( "ref", "%d", ref );
							VARS( "end", "%ls", _end );

							/* Skip length of the number */
							_rpstr = _end;

							/* Extend first from prev of replacement */
							if( !( replace = (char*)pwcsncatstr(
									(wchar_t*)replace, (wchar_t*)rprev,
										(wchar_t*)rstart - (wchar_t*)rprev ) ) )
							{
								OUTOFMEM;
								RETURN( (char*)NULL );
							}

							VARS( "replace", "%ls", replace );
							VARS( "ref", "%d", ref );

							if( ref > 0 && ref < PREGEX_MAXREF )
								refer = &( regex->ref[ ref ] );
							/* TODO: Ref $0 */
							else
								refer = (prange*)NULL;

							if( refer )
							{
								MSG( "There is a reference!" );
								VARS( "refer->start", "%ls",
										(wchar_t*)refer->start );
								VARS( "refer->end", "%ls",
										(wchar_t*)refer->start );

								if( !( replace = (char*)pwcsncatstr(
										(wchar_t*)replace,
											(wchar_t*)refer->start,
												(wchar_t*)refer->end -
													(wchar_t*)refer->start ) ) )
								{
									OUTOFMEM;
									RETURN( (char*)NULL );
								}
							}

							VARS( "replace", "%ls", (wchar_t*)replace );
							rprev = rpstr = (char*)_rpstr;
						}
						else
							rpstr = (char*)_rpstr;
					}
					else
					{
						char*		_end;

						MSG( "Byte-character mode (Standard)" );

						if( isdigit( *( ++rpstr ) ) )
						{
							ref = strtol( rpstr, &_end, 0 );

							VARS( "ref", "%d", ref );
							VARS( "end", "%s", _end );

							/* Skip length of the number */
							rpstr = _end;

							/* Extend first from prev of replacement */
							if( !( replace = pstrncatstr( replace,
										rprev, rstart - rprev ) ) )
							{
								OUTOFMEM;
								RETURN( (char*)NULL );
							}

							VARS( "replace", "%s", replace );
							VARS( "ref", "%d", ref );

							if( ref > 0 && ref < PREGEX_MAXREF )
								refer = &( regex->ref[ ref ] );
							/* TODO: Ref $0 */
							else
								refer = (prange*)NULL;

							if( refer )
							{
								MSG( "There is a reference!" );
								VARS( "refer->start", "%ls",
										(wchar_t*)refer->start );
								VARS( "refer->end", "%ls",
										(wchar_t*)refer->end );

								if( !( replace = (char*)pstrncatstr(
											replace, refer->start,
												refer->end - refer->start ) ) )
								{
									OUTOFMEM;
									RETURN( (char*)NULL );
								}
							}

							VARS( "replace", "%s", replace );
							rprev = rpstr;
						}
					}
				}
				else
					rpstr += ( regex->flags & PREGEX_RUN_WCHAR )
								? sizeof( wchar_t ) : 1;
			}

			VARS( "rpstr", "%p", rpstr );
			VARS( "rprev", "%p", rprev );

#ifdef DEBUG
			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				VARS( "rpstr", "%ls", (wchar_t*)replace );
				VARS( "rprev", "%ls", (wchar_t*)rprev );
			}
			else
			{
				VARS( "rpstr", "%s", replace );
				VARS( "rprev", "%s", rprev );
			}
#endif

			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				if( rpstr != rprev &&
						!( replace = (char*)pwcscatstr(
								(wchar_t*)replace, (wchar_t*)rprev, FALSE ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
			else
			{
				if( rpstr != rprev && !( replace = pstrcatstr(
											replace, rprev, FALSE ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
		}
		else
			replace = replacement;

		if( replace )
		{
#ifdef DEBUG
			if( regex->flags & PREGEX_RUN_WCHAR )
				VARS( "replace", "%ls", (wchar_t*)replace );
			else
				VARS( "replace", "%s", replace );
#endif
			if( regex->flags & PREGEX_RUN_WCHAR )
			{
				if( !( ret = (char*)pwcsncatstr(
						(wchar_t*)ret, (wchar_t*)replace,
							pwcslen( (wchar_t*)replace ) ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}
			else
			{
				if( !( ret = pstrncatstr( ret, replace, pstrlen( replace ) ) ) )
				{
					OUTOFMEM;
					RETURN( (char*)NULL );
				}
			}

			if( !( regex->flags & PREGEX_RUN_NOREF ) )
				pfree( replace );
		}

		sstart = end;
	}

#ifdef DEBUG
	if( regex->flags & PREGEX_RUN_WCHAR )
		VARS( "ret", "%ls", (wchar_t*)ret );
	else
		VARS( "ret", "%s", ret );
#endif

	RETURN( ret );
}
