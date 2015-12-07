/* -HEADER----------------------------------------------------------------------
UniCC LALR(1) Parser Generator
Copyright (C) 2006-2015 by Phorward Software Technologies, Jan Max Meyer
http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com

File:	p_proto.h (created on 30.01.2007)
Author:	Jan Max Meyer
Usage:	Prototype declarations

You may use, modify and distribute this software under the terms and conditions
of the Artistic License, version 2. Please see LICENSE for more information.
----------------------------------------------------------------------------- */

#ifndef P_PROTO_H
#define P_PROTO_H

/* p_first.c */
void p_first( LIST* symbols );
int p_rhs_first( LIST** first, LIST* rhs );

/* p_lalr_gen.c */
void p_generate_tables( PARSER* parser );
void p_detect_default_productions( PARSER* parser );

/* p_error.c */
void p_error( PARSER* parser, int err_id, int err_style, ... );

/* p_mem.c */
SYMBOL* p_get_symbol( PARSER* p, void* dfn, int type, BOOLEAN create );
void p_free_symbol( SYMBOL* sym );
PROD* p_create_production( PARSER* p, SYMBOL* lhs );
void p_append_to_production( PROD* p, SYMBOL* sym, char* name );
void p_free_production( PROD* prod );
ITEM* p_create_item( STATE* st, PROD* p, LIST* lookahead );
void p_free_item( ITEM* it );
STATE* p_create_state( PARSER* p );
void p_free_state( STATE* st );
TABCOL* p_create_tabcol( SYMBOL* sym, short action, int index, ITEM* item );
void p_free_tabcol( TABCOL* act );
TABCOL* p_find_tabcol( LIST* row, SYMBOL* sym );
OPT* p_create_opt( plist* options, char* opt, char* def );
plist* p_free_opts( plist* options );
PARSER* p_create_parser( void );
void p_free_parser( PARSER* parser );
VTYPE* p_find_vtype( PARSER* p, char* name );
VTYPE* p_create_vtype( PARSER* p, char* name );
void p_free_vtype( VTYPE* vt );

/* p_integrity.c */
BOOLEAN p_undef_or_unused( PARSER* parser );
BOOLEAN p_regex_anomalies( PARSER* parser );
BOOLEAN p_stupid_productions( PARSER* parser );

/* p_string.c */
char* p_int_to_str( int val );
char* p_long_to_str( long val );
char* p_str_no_whitespace( char* str );

/* p_util.c */
char* p_derivation_name( char* name, char append_char );
int p_unescape_char( char* str, char** strfix );
SYMBOL* p_find_base_symbol( SYMBOL* sym );
char* p_gen_c_identifier( char* str, BOOLEAN to_upper );

/* p_rewrite.c */
void p_rewrite_grammar( PARSER* parser );
void p_unique_charsets( PARSER* parser );
void p_fix_precedences( PARSER* parser );
void p_inherit_fixiations( PARSER* parser );
void p_inherit_vtypes( PARSER* parser );
void p_setup_single_goal( PARSER* parser );
void p_symbol_order( PARSER* parser );
void p_charsets_to_ptn( PARSER* parser );

/* p_virtual.c */
SYMBOL* p_positive_closure( PARSER* parser, SYMBOL* base );
SYMBOL* p_kleene_closure( PARSER* parser, SYMBOL* base );
SYMBOL* p_optional_closure( PARSER* parser, SYMBOL* base );

/* p_main.c */
char* p_version( BOOLEAN long_version );

/* p_debug.c */
void p_print_symbol( FILE* stream, SYMBOL* sym );
void p_dump_grammar( FILE* stream, PARSER* parser );
void p_dump_symbols( FILE* stream, PARSER* parser );
void p_dump_item_set( FILE* stream, char* title, LIST* list );
void p_dump_lalr_states( FILE* stream, PARSER* parser );
void p_dump_productions( FILE* stream, PARSER* parser );
void p_dump_production( FILE* stream, PROD* prod,
		BOOLEAN with_lhs, BOOLEAN semantics );

/* p_parse.c / p_parse.syn */
int p_parse( PARSER* p, char* src );

/* p_keywords.c */
void p_keywords_to_dfa( PARSER* parser );
void p_single_lexer( PARSER* parser );
pregex_dfa* p_find_equal_dfa( PARSER* parser, pregex_dfa* ndfa );
void p_symbol_to_nfa( PARSER* parser, pregex_nfa* nfa, SYMBOL* sym );

/* p_list.c */
LIST* list_push( LIST* list, void* ptr );
LIST* list_pop( LIST* list, void** ptr );
LIST* list_remove( LIST* list, void* ptr );
LIST* list_free( LIST* list );
void list_print( LIST* list, void (*callback)( void* ) );
LIST* list_dup( LIST* src );
int list_find( LIST* list, void* ptr );
void* list_getptr( LIST* list, int cnt );
int list_diff( LIST* first, LIST* second );
LIST* list_union( LIST* first, LIST* second );
int list_count( LIST* list );
pboolean list_subset( LIST* list, LIST* subset );
LIST* list_sort( LIST* list, int (*sf)( void*, void* ) );

/* p_build.c */
void p_build_code( PARSER* parser );
char* p_build_action( PARSER* parser, GENERATOR* g,
			PROD* p, char* base, BOOLEAN def_code );
char* p_build_scan_action( PARSER* parser, GENERATOR* g, SYMBOL* s,
			char* base );
char* p_escape_for_target( GENERATOR* g, char* str, BOOLEAN clear );
char* p_mkproduction_str( PROD* p );
BOOLEAN p_load_generator( PARSER* parser, GENERATOR* g, char* genfile );

/* p_xml.c */
void p_build_xml( PARSER* parser, BOOLEAN finished );

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

#endif
