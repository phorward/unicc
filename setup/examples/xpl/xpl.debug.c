#include "xpl.h"

static char opcodes[][3+1] = 
{
    "NOP", "CAL", "LIT", "LOD", "STO", "DUP", "DRP", "JMP", "JPC",
    "EQU", "NEQ", "LOT", "LEQ", "GRT", "GEQ",
    "ADD", "SUB", "MUL", "DIV"
};

extern xpl_fn   xpl_buildin_functions[];

void xpl_dump( xpl_program* prog, xpl_runtime* rt )
{
    int         i;
    xpl_cmd*    cmd;
    
    for( i = 0; i < prog->program_cnt; i++ )
    {
        cmd = &( prog->program[i] );
        
        fprintf( stderr, "%s%03d: %s ", 
                    ( rt && rt->ip == cmd ) ? ">" : " ",
                        i, opcodes[ cmd->op ] );
        
        if( cmd->op == XPL_JMP || cmd->op == XPL_JPC )
            fprintf( stderr, "%03d", cmd->param );
        else if( cmd->op == XPL_LIT )
            fprintf( stderr, "%d (%s%s%s)", cmd->param,
                        prog->literals[ cmd->param ]->type == XPL_STRINGVAL ?
                            "\"" : "",
                        xpl_value_get_string( prog->literals[ cmd->param ] ),
                        prog->literals[ cmd->param ]->type == XPL_STRINGVAL ?
                            "\"" : "" );
        else if( cmd->op == XPL_LOD || cmd->op == XPL_STO )
            fprintf( stderr, "%d => %s", cmd->param, 
                        prog->variables[ cmd->param ] );
        else if( cmd->op == XPL_CAL )
            fprintf( stderr, "%d => %s()", cmd->param,
                        xpl_buildin_functions[ cmd->param ].name );         
        
        fprintf( stderr, "\n" );
    }
}
