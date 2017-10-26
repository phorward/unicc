/* -HEADER----------------------------------------------------------------------
UniCC4C
Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com

File:	unicc4c.h
Author:	Jan Max Meyer
Usage:	Module Header
----------------------------------------------------------------------------- */

#ifndef UNICC4C_H
#define UNICC4C_H

/*
 * Includes
 */
#include <pbasis.h>
#include <pregex.h>
#include <pstring.h>

/*
 * Defines
 */
#define UNICC4C_VERSION	"0.1"
 
#define OUTOFMEM

#define	VARPREF	"@@"
#define UNICC4C	"UNICC4C"

#define STATIC	"UNICC_STATIC"
#define REDUCE	1
#define SHIFT 	2

/*
 * Typedefs
 */
typedef struct _astnode	ASTNODE;

/*
 * Structs
 */

typedef struct
{
	uchar*		type;
	uchar*		name;
} ASTNODEVAR;
 
struct _astnode
{
	uchar*		name;
	ASTNODE*	parent;
	
	BOOLEAN		abstract;

	uchar*		def;
};
 
typedef struct
{
} OPT;

/*
 * Function Prototypes
 */
#ifndef MAKE_PROTOTYPES
#include "proto.h"
#endif

#endif
