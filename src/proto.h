/* build.c */
char* escape_for_target( GENERATOR* g, char* str, BOOLEAN clear );
char* build_action( PARSER* parser, GENERATOR* g, PROD* p, char* base, BOOLEAN def_code );
char* build_scan_action( PARSER* parser, GENERATOR* g, SYMBOL* s, char* base );
char* mkproduction_str( PROD* p );
BOOLEAN load_generator( PARSER* parser, GENERATOR* g, char* genfile );
void build_code( PARSER* parser );

/* buildxml.c */
void build_xml( PARSER* parser, BOOLEAN finished );

/* debug.c */
void print_symbol( FILE* stream, SYMBOL* sym );
void dump_grammar( FILE* stream, PARSER* parser );
void dump_symbols( FILE* stream, PARSER* parser );
void dump_item_set( FILE* stream, char* title, LIST* list );
void dump_lalr_states( FILE* stream, PARSER* parser );
void dump_productions( FILE* stream, PARSER* parser );
void dump_production( FILE* stream, PROD* p, BOOLEAN with_lhs, BOOLEAN semantics );

/* error.c */
void print_error( PARSER* parser, ERRORCODE err_id, int err_style, ... );

/* first.c */
void compute_first( PARSER* parser );
int seek_rhs_first( plist* first, plistel* rhs );

/* integrity.c */
BOOLEAN find_undef_or_unused( PARSER* parser );
BOOLEAN check_regex_anomalies( PARSER* parser );
BOOLEAN check_stupid_productions( PARSER* parser );

/* lalr.c */
void generate_tables( PARSER* parser );
void detect_default_productions( PARSER* parser );

/* lex.c */
void merge_symbols_to_dfa( PARSER* parser );
void construct_single_lexer( PARSER* parser );
pregex_dfa* find_equal_dfa( PARSER* parser, pregex_dfa* ndfa );
void nfa_from_symbol( PARSER* parser, pregex_nfa* nfa, SYMBOL* sym );

/* list.c */
LIST* list_push( LIST* list, void* ptr );
LIST* list_pop( LIST* list, void** ptr );
LIST* list_remove( LIST* list, void* ptr );
LIST* list_free( LIST* list );
LIST* list_dup( LIST* src );
int list_count( LIST* list );
int list_find( LIST* list, void* ptr );
void* list_getptr( LIST* list, int cnt );
LIST* list_union( LIST* first, LIST* second );

/* main.c */
char* print_version( BOOLEAN long_version );
void print_copyright( FILE* stream );
void print_usage( FILE* stream, char* progname );
BOOLEAN get_command_line( int argc, char** argv, char** filename, char** output, PARSER* parser );

/* mem.c */
SYMBOL* get_symbol( PARSER* p, void* dfn, int type, BOOLEAN create );
void free_symbol( SYMBOL* sym );
PROD* create_production( PARSER* p, SYMBOL* lhs );
void append_to_production( PROD* p, SYMBOL* sym, char* name );
void free_production( PROD* prod );
ITEM* create_item( PROD* p );
void free_item( ITEM* it );
STATE* create_state( PARSER* p );
void free_state( STATE* st );
TABCOL* create_tabcol( SYMBOL* sym, short action, int idx, ITEM* item );
void free_tabcol( TABCOL* act );
TABCOL* find_tabcol( LIST* row, SYMBOL* sym );
OPT* create_opt( plist* options, char* opt, char* def );
plist* free_opts( plist* options );
PARSER* create_parser( void );
void free_parser( PARSER* parser );
VTYPE* find_vtype( PARSER* p, char* name );
VTYPE* create_vtype( PARSER* p, char* name );
void free_vtype( VTYPE* vt );

/* parse.c */
int parse_grammar( PARSER* p, char* filename, char* src );

/* rewrite.c */
void rewrite_grammar( PARSER* parser );
void unique_charsets( PARSER* parser );
void fix_precedences( PARSER* parser );
void inherit_fixiations( PARSER* parser );
void inherit_vtypes( PARSER* parser );
void setup_single_goal( PARSER* parser );
void charsets_to_ptn( PARSER* parser );
void symbol_orders( PARSER* parser );

/* string.c */
char* int_to_str( int val );
char* long_to_str( long val );
char* str_no_whitespace( char* str );

/* utils.c */
char* derive_name( char* name, char append_char );
int unescape_char( char* str, char** strfix );
SYMBOL* find_base_symbol( SYMBOL* sym );
char* c_identifier( char* str, BOOLEAN to_upper );

/* virtual.c */
SYMBOL* positive_closure( PARSER* parser, SYMBOL* base );
SYMBOL* kleene_closure( PARSER* parser, SYMBOL* base );
SYMBOL* optional_closure( PARSER* parser, SYMBOL* base );

/* xml.c */
XML_T xml_child( XML_T xml, char* name );
XML_T xml_idx( XML_T xml, int idx );
char* xml_attr( XML_T xml, char* attr );
long xml_int_attr( XML_T xml, char* attr );
double xml_float_attr( XML_T xml, char* attr );
XML_T xml_vget( XML_T xml, va_list ap );
XML_T xml_get( XML_T xml, ... );
char ** xml_pi( XML_T xml, char* target );
char* xml_decode( char* s, char ** ent, char t );
char* xml_str2utf8( char ** s, size_t* len );
void xml_free_attr( char ** attr );
XML_T xml_parse_str( char* s, size_t len );
XML_T xml_parse_fp( FILE* fp );
XML_T xml_parse_file( char* file );
char* xml_ampencode( char* s, size_t len, char ** dst, size_t* dlen, size_t* max, short a );
char* xml_toxml( XML_T xml );
void xml_free( XML_T xml );
char* xml_error( XML_T xml );
XML_T xml_new( char* name );
XML_T xml_insert( XML_T xml, XML_T dest, size_t off );
XML_T xml_add_child( XML_T xml, char* name, size_t off );
XML_T xml_set_txt( XML_T xml, char* txt );
XML_T xml_set_attr( XML_T xml, char* name, char* value );
XML_T xml_set_int_attr( XML_T xml, char* name, long value );
XML_T xml_set_float_attr( XML_T xml, char* name, double value );
XML_T xml_set_flag( XML_T xml, short flag );
int xml_count( XML_T xml );
int xml_count_all( XML_T xml );
XML_T xml_cut( XML_T xml );

