#if UNICC_STACKDEBUG
void @@prefix_parser::dbg_stack( FILE* out, @@prefix_tok* stack, @@prefix_tok* tos )
{
    fprintf( out, "%s: Stack Dump: ", UNICC_PARSER );

    for( ; stack <= tos; stack++ )
    {
        fprintf( out, "%d%s%s%s ", stack->state,
            stack->symbol ? " (" : "",
            stack->symbol ? stack->symbol->name : "",
            stack->symbol ? ")" : "" );
    }

    fprintf( out, "\n" );
}
#endif /* UNICC_STACKDEBUG */
