#ifndef XPL_H
#define XPL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

/* Virtual machine values */
typedef enum
{
    XPL_NULLVAL,                            /* Nothing/Undefined */
    XPL_INTEGERVAL,                         /* Integer type */
    XPL_FLOATVAL,                           /* Float type */
    XPL_STRINGVAL                           /* String type */
} xpl_datatype;

typedef struct
{
    xpl_datatype    type;                   /* Value type */
    
    union
    {
        int         i;
        float       f;
        char*       s;
    } value;                                /* Value storage union */
    
    char*           strval;                 /* Temporary string value
                                                representation pointer */
} xpl_value;

/* Functions */
typedef struct
{
    char*           name;                   /* Function name */
    int             min;                    /* Minimal parameter count */
    int             max;                    /* Maximal parameter count */
    xpl_value*      (*fn)( int, xpl_value** ); /* Function callback pointer */
} xpl_fn;

/* Virtual machine opcodes */
typedef enum
{
    XPL_NOP,                                /* No operation */

    XPL_CAL,                                /* Call function */
    XPL_LIT,                                /* Load literal value */
    XPL_LOD,                                /* Load value from variable */
    XPL_STO,                                /* Store value into variable */
    XPL_DUP,                                /* Duplicate stack item */
    XPL_DRP,                                /* Drop stack item */
    XPL_JMP,                                /* Jump to address */
    XPL_JPC,                                /* Jump to address if false */

    XPL_EQU,                                /* Check for equal */
    XPL_NEQ,                                /* Check for not-equal */
    XPL_LOT,                                /* Check for lower-than */
    XPL_LEQ,                                /* Check for lower-equal */
    XPL_GRT,                                /* Check for greater-than */
    XPL_GEQ,                                /* Check for greater-equal */

    XPL_ADD,                                /* Add or join two values */
    XPL_SUB,                                /* Substract two values */
    XPL_MUL,                                /* Multiply two values */
    XPL_DIV                                 /* Divide two values */
} xpl_op;

/* Command description */
typedef struct
{
    xpl_op      op;                         /* Opcode */
    int         param;                      /* Parameter */
} xpl_cmd;

/* Program structure */
typedef struct
{
    xpl_cmd*    program;                    /* Virtual machine program */
    int         program_cnt;                /* Numer of elements in program */

    xpl_value** literals;                   /* Array of literal objects */
    int         literals_cnt;               /* Number of elements in literals */

    char**      variables;                  /* The variable symbol table. The
                                                index of the variable name
                                                represents the address in the
                                                variables-member of the
                                                runtime data structure.
                                            */
    int         variables_cnt;              /* Number of elements
                                                in variables */

} xpl_program;

/* Runtime structure */
typedef struct
{
    xpl_value** variables;                  /* Array of objects representing
                                                the variable values. The
                                                number of objects is in
                                                the corresponding xpl_program
                                                structure in the member
                                                variables_cnt
                                            */

    xpl_value** stack;                      /* Array representing the value
                                                stack */
    int         stack_cnt;                  /* Defines the top-of-stack, and
                                                the numbers of elements
                                                resisting.
                                            */
    
    xpl_cmd*    ip;                         /* Instruction Pointer to the 
                                                currently executed code
                                                address.
                                            */
} xpl_runtime;

/* Some general defines */
#define XPL_MALLOCSTEP                      256

/* Import function prototypes */
#include "xpl.proto.h"

#endif
