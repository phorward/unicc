/* -MODULE----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2014 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_list.c
Author:	Jan Max Meyer
Usage:	Management functions for simple linked-lists.

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

/*
	The linked list structure LIST (llist) came from libphorward, but
	had been replaced there by the much more powerful plist objects.
	
	UniCC is the only program that still makes use of these older lists,
	and a refactoring to use plist is too expensive.
*/

#include "p_global.h"

/** Pushes a pointer of any type to a linked list of pointers. Therefore, the
list can act as a stack when using the function list_pop() to pop items off
this "stack". If not used as a stack, list_push() simply appends a node to a
linked list of nodes.

//list// is the pointer to the element list where the element should be pushed
on. If this is (LIST*)NULL, the item acts as the first element of the list.
//ptr// is the pointer to the element to be pushed on the list.

Returns a pointer to the first item of the linked list of elements.
*/
LIST* list_push( LIST* list, void* ptr )
{
	LIST*	elem;
	LIST*	item;

	if( ( elem = (LIST*)pmalloc( sizeof( LIST ) ) ) )
	{
		elem->pptr = ptr;
		elem->next = (LIST*)NULL;

		if( !list )
			list = elem;
		else
		{
			item = list;
			while( item->next )
				item = item->next;

			item->next = elem;
		}
	}

	return list;
}

/** Pops the last element off a linked-list of pointers.

//list// is the pointer to the element list where the element should be popped
off. If this is (LIST*)NULL, nothing occurs (then, the stack is empty).
//ptr// is the pointer to be filled with the pointer that was stored on the
popped element of the list. If this is (void**)NULL, the element will be removed
from the list and not operated anymore.

Returns a pointer to the first item of the linked list of elements.
If the last element is popped, (LIST*)NULL is returned.
*/
LIST* list_pop( LIST* list, void** ptr )
{
	LIST*	item;
	LIST*	prev	= (LIST*)NULL;

	if( !list )
	{
		if( ptr )
			*ptr = (void*)NULL;

		return (LIST*)NULL;
	}
	else
	{
		item = list;
		while( item->next )
		{
			prev = item;
			item = item->next;
		}

		if( prev )
			prev->next = (LIST*)NULL;

		if( ptr )
			*ptr = item->pptr;

		if( item == list )
			list = (LIST*)NULL;

		pfree( item );

		item = (LIST*)NULL;
	}

	return list;
}

/** Removes an item from a linked list. Instead as list_pop(), list_remove() can
remove an item anywhere in the linked list.

//list// is the pointer to the begin of the element list where the element
should be popped off. If this is (LIST*)NULL, nothing occurs.
//ptr// is the pointer to be searched for. The element with this pointer will be
removed from the list.

Returns a pointer to the updated begin of the list, or (LIST*)NULL if the last
item was removed.
*/
LIST* list_remove( LIST* list, void* ptr )
{
	LIST*	item;
	LIST*	prev	= (LIST*)NULL;

	if( !ptr )
		return list;

	for( item = list; item; item = item->next )
	{
		if( item->pptr == ptr )
		{
			if( !prev )
				list = item->next;
			else
				prev->next = item->next;

			pfree( item );
			break;
		}

		prev = item;
	}

	return list;
}

/** Frees a linked list.

//list// is the linked list to be freed.

Returns always (LIST*)NULL.
*/
LIST* list_free( LIST* list )
{
	LIST*	next	= (LIST*)NULL;
	LIST*	item;

	item = list;
	while( item )
	{
		next = item->next;
		pfree( item );

		item = next;
	}

	return (LIST*)NULL;
}

/** Prints the items of a linked list of elements for debug purposes.
This function expects a callback-function which then performs the output of the
pointer that is associated with the list element.

//list// is the linked list to be printed. //(*callback)(void*)// is the
callback function that is called for each element.
*/
void list_print( LIST* list, void (*callback)( void* ) )
{
	LIST*	item	= list;

	while( item )
	{
		callback( list->pptr );
		item = item->next;
	}
}

/** Duplicates a list in a 1:1 copy.

//src// is the linked list to be copied.

Returns a pointer to the copy if //src//.
*/
LIST* list_dup( LIST* src )
{
	LIST*	item;
	LIST*	tar		= (LIST*)NULL;

	for( item = src; item; item = item->next )
		tar = list_push( tar, item->pptr );

	return tar;
}

/** Searches for a pointer in a linked list.

//list// is the linked list where //ptr// should be searched in.
//ptr// is the pointer to be searched in the linked list.

Returns -1 if the desired item was not found, else the offset of the element
from the lists begin, 0 is the first element.
*/
int list_find( LIST* list, void* ptr )
{
	LIST*	item;
	int		cnt		= 0;

	if( !ptr )
		return -1;

	for( item = list; item; item = item->next )
	{
		if( item->pptr == ptr )
			return cnt;

		cnt++;
	}

	return -1;
}

/** Returns the pointer of the desired offset from the linked list.

//list// is the linked list to get //cnt// from.
//cnt// is the offset of the item starting from the lists first element that
should be returned.

Returns a pointer of the desired position, (void*)NULL if the position is
not in the list (if //cnt// goes over the end of the list).
*/
void* list_getptr( LIST* list, int cnt )
{
	LIST*	item;

	if( cnt < 0 )
		return (void*)NULL;

	for( item = list; item; item = item->next )
	{
		if( cnt == 0 )
			return item->pptr;

		cnt--;
	}

	return (void*)NULL;
}

/** Compares two lists if they contain the same pointers.

//first// is the first linked list to be compared. //second// is the second
linked list to be compated with //first//.

Returns 1 if indifferent, 0 if different
*/
int list_diff( LIST* first, LIST* second )
{
	int		first_cnt	= 0;
	int		second_cnt	= 0;
	LIST*	item;

	for( item = first; item; item = item->next )
		first_cnt++;
	for( item = second; item; item = item->next )
		second_cnt++;

	if( first_cnt == second_cnt && first_cnt > 0 )
	{
		for( item = first; item; item = item->next )
		{
			first_cnt--;
			if( list_find( second, item->pptr ) >= 0 )
				second_cnt--;
		}

		if( first_cnt != second_cnt  )
			return 0;

		return 1;
	}

	return 0;
}

/** Unions two list to a huger new one.

//first// is the first linked list. //second// is the second linked list that
will be unioned into //first//.

Returns the extended list //first//, which is the union of //first// and
//second//. The elements of //second// are copied.
*/
LIST* list_union( LIST* first, LIST* second )
{
	LIST*	ret;
	LIST*	current;

	if( first != (LIST*)NULL )
	{
		ret = first;
		for( current = second; current; current = current->next )
			if( list_find( ret, current->pptr ) == -1 )
				ret = list_push( ret, current->pptr );
	}
	else
		ret = list_dup( second );

	return ret;
}

/** Counts the elements in a list.

//list// is the list start point which items should be counted.

Returns the number of items contained by the list.
*/
int list_count( LIST* list )
{
	int		count		= 0;

	for( ; list; list = list->next )
		count++;

	return count;
}

/** Checks if a list contains a subset of another list.

//list// is the first linked list. //subset// is the possible subset list.

Returns TRUE if there is a subset, else FALSE.
*/
pboolean list_subset( LIST* list, LIST* subset )
{
	LIST*	current	= (LIST*)NULL;

	if( list )
	{
		for( current = subset; current; current = current->next )
			if( list_find( list, current->pptr ) < 0 )
				break;

		if( !current )
			return TRUE;
	}

	return FALSE;
}

/** Sorts a list according to a callback-function using the bubble-sort
algorithm.

//list// is the linked list to be sorted. //int (*sf)(void*,void*)// is a
pointer to callback-function to compare items. The functions shall return a
value lower than 0 if a<b, greater 0 if a>b and 0 if a==b.

Returns the new beginning pointer to the sorted list //list//.
*/
LIST* list_sort( LIST* list, int (*sf)( void*, void* ) )
{
	LIST*		current;
	int			ret;
	BOOLEAN		changes;
	void*		tmp;

	if( !sf )
	{
		WRONGPARAM;
		return list; /* Can't sort anything */
	}

	do
	{
		changes = FALSE;

		for( current = list; list_next( current );
				current = list_next( current ) )
		{
			if( ( ret = (*sf)( list_access( current ),
							list_access( list_next( current ) ) ) ) < 0 )
			{
				tmp = list_access( list_next( current ) );
				current->next->pptr = list_access( current );
				current->pptr = tmp;

				changes = TRUE;
			}
			else if( ret > 0 )
			{
				tmp = list_access( current );
				current->pptr = list_access( list_next( current ) );
				current->next->pptr = tmp;

				changes = TRUE;
			}
		}
	}
	while( changes );

	return list;
}
