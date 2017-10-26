/*
 * Parser:		@@name
 * Version:		@@version
 * Copyright:	@@copyright
 * Description:	@@description
 *
 * UniCC4C Parser - Version 1.10
 * Copyright (C) by Phorward Software Technologies, Jan Max Meyer
 */

%%%code{
	if( pstrlen( xml_txt( xml_child( parser, "prologue" ) ) ) )
%%%code}
@@prologue
%%%code{
	else
	{
%%%code}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
%%%code{
	}
%%%code}

%%%include defines.h

/* Include parser control block definitions */
#include "@@basename.h"
/* TODO: AST */

@@UNICC4C_print_act_fn()
@@UNICC4C_print_go_fn()

%%%include fn.getinput.c

%%%include fn.clearin.c

@@UNICC4C_print_lex_fn()

%%%include fn.debug.c

%%%include fn.handleerr.c

%%%include fn.parse.c

%%%include fn.main.c

