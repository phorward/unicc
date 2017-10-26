/*
 * This is a generated file - manual editing is not recommended!
 */

/* gen.c */
uchar* gen( void );

/* gen.act.c */
void print_act_tab( uchar* tab );
uchar* print_act_fn( uchar* fn );

/* gen.go.c */
void print_go_tab( uchar* tab );
uchar* print_go_fn( uchar* fn );

/* gen.lex.c */
uchar* print_lex_fn( uchar* fn );

/* gen.misc.c */
void print_symbol_name_tab( uchar* tab );
void print_whitespace_tab( uchar* tab );

/* gen.prod.c */
void print_prod_len_tab( uchar* tab );
void print_prod_lhs_tab( uchar* tab );
void print_def_prod_tab( uchar* tab );
uchar* print_actions( void );

/* main.c */
void copyright( void );
void usage( uchar* progname );
BOOLEAN get_command_line( int argc, char** argv );
int main( int argc, char** argv );

/* output.c */
void EL( char** output );
void TABS( char** output, int i );
void PRINT( uchar** output, uchar* fmt, va_list args );
void L( uchar** output, uchar* txt, ... );
void TL( uchar** output, int i, uchar* txt, ... );
void T( uchar** output, int i, uchar* txt, ... );

/* util.c */
int get_error_sym( void );
int get_value_type( uchar* type_name );

