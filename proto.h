/* bnf.c */
pboolean gram_from_bnf( Grammar* g, char* src );

/* grammar.c */
Symbol* sym_create( Grammar* g, char* name );
Symbol* sym_free( Symbol* sym );
Symbol* sym_drop( Symbol* sym );
Symbol* sym_get( Grammar* g, unsigned int n );
Symbol* sym_get_by_name( Grammar* g, char* name );
Symbol* sym_get_nameless_term_by_def( Grammar* g, char* name );
Production* sym_getprod( Symbol* sym, size_t n );
char* sym_to_str( Symbol* sym );
Symbol* sym_mod_positive( Symbol* sym );
Symbol* sym_mod_optional( Symbol* sym );
Symbol* sym_mod_kleene( Symbol* sym );
Production* prod_create( Grammar* g, Symbol* lhs, ... );
Production* prod_free( Production* p );
Production* prod_get( Grammar* g, size_t n );
pboolean prod_append( Production* p, Symbol* sym );
int prod_remove( Production* p, Symbol* sym );
Symbol* prod_getfromrhs( Production* p, int off );
char* prod_to_str( Production* p );
Grammar* gram_create( void );
pboolean gram_prepare( Grammar* g );
void __dbg_gram_dump( char* file, int line, char* function, char* name, Grammar* g );
char* gram_to_bnf( Grammar* grm );
pboolean gram_dump_json( FILE* stream, Grammar* grm );
Grammar* gram_free( Grammar* g );

/* lr.c */
pboolean lr_build( unsigned int* cnt, unsigned int*** dfa, Grammar* grm );

/* parse.c */
AST_node* ast_create( char* emit, Symbol* sym, Production* prod, AST_node* child );
AST_node* ast_free( AST_node* node );
int ast_len( AST_node* node );
AST_node* ast_get( AST_node* node, int n );
AST_node* ast_select( AST_node* node, char* emit, int n );
void ast_eval( AST_node* ast, Ast_evalfn func );
void ast_dump( FILE* stream, AST_node* ast );
void ast_dump_short( FILE* stream, AST_node* ast );
void __dbg_ast_dump( char* file, int line, char* function, char* name, AST_node* ast );
void ast_dump_json( FILE* stream, AST_node* ast );
void ast_dump_tree2svg( FILE* stream, AST_node* ast );
#if 0
void ast_dump_pvm( pvmprog* prog, AST_node* ast );
#endif
Parser* par_create( Grammar* g );
Parser* par_free( Parser* par );
plex* par_autolex( Parser* p );
pboolean par_dump_json( FILE* stream, Parser* par );
Parser_ctx* parctx_init( Parser_ctx* ctx, Parser* par );
Parser_ctx* parctx_create( Parser* par );
Parser_ctx* parctx_reset( Parser_ctx* ctx );
Parser_ctx* parctx_free( Parser_ctx* ctx );
Parser_stat parctx_next( Parser_ctx* ctx, Symbol* sym );
Parser_stat parctx_next_by_name( Parser_ctx* ctx, char* name );
Parser_stat parctx_next_by_idx( Parser_ctx* ctx, unsigned int idx );
pboolean par_parse( AST_node** root, Parser* par, char* start );

