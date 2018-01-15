// Parser class
class @@prefix_parser
{
	private:
		// --- Tables ---

		// Action Table
		const int actions[ @@number-of-states ][ @@deepest-action-row * 3 + 1 ] =
		{
@@action-table
		};

		// GoTo Table
		const int go[ @@number-of-states ][ @@deepest-goto-row * 3 + 1 ] =
		{
@@goto-table
		};

		// Default productions per state
		const int def_prod[ @@number-of-states ] =
		{
			@@default-productions
		};

		#if !@@mode
		// DFA selection table
		const int dfa_select[ @@number-of-states ] =
		{
			@@dfa-select
		};
		#endif

		#if @@number-of-dfa-machines
		// DFA index table
		const int dfa_idx[ @@number-of-dfa-machines ][ @@deepest-dfa-index-row ] =
		{
@@dfa-index
		};

		// DFA transition chars
		const int dfa_chars[ @@size-of-dfa-characters * 2 ] =
		{
			@@dfa-char
		};

		// DFA transitions
		const int dfa_trans[ @@size-of-dfa-characters ] =
		{
			@@dfa-trans
		};

		// DFA acception states
		const int dfa_accept[ @@number-of-dfa-machines ][ @@deepest-dfa-accept-row ] =
		{
@@dfa-accept
		};

		#endif

		// Symbol information table
		const @@prefix_syminfo symbols[ @@number-of-symbols ] =
		{
@@symbols
		};

		// Production information table
		const @@prefix_prodinfo productions[ @@number-of-productions ] =
		{
@@productions
		};

		// --- Runtime ---

		// Stack
		@@prefix_tok*	stack;
		@@prefix_tok*	tos;

		// Stack size
		size_t			stacksize;

		// Values
		@@prefix_vtype	ret;
		@@prefix_vtype	test;

		// State
		int				act;
		int				idx;
		int				lhs;

		// Lookahead
		int				sym;
		int				old_sym;
		size_t			len;

		// Input buffering
		UNICC_SCHAR*	lexem;
		UNICC_CHAR*		buf;
		UNICC_CHAR*		bufend;
		UNICC_CHAR*		bufsize;

		// Lexical analysis
		UNICC_CHAR		next;
		bool			is_eof;

		// Error handling
		int				error_delay;
		int				error_count;

		size_t			line;
		size_t			column;

		// User-defined components
		@@pcb

		// Functions

		//fn.clearin.cpp
		void clear_input(void);

		//fn.debug.cpp
		#if UNICC_STACKDEBUG
		void dbg_stack( FILE* out, @@prefix_tok* stack, @@prefix_tok* tos )
		#endif

		//fn.getact.cpp
		bool get_act( void );

		//fn.getgo.cpp
		bool get_go( void );

		//fn.getinput.cpp
		UNICC_CHAR get_input( size_t offset );

		//fn.getsym.cpp
		bool get_sym( void );

		//fn.handleerr.cpp
		bool handle_error( FILE* @@prefix_dbg );

		//fn.lex.cpp
		#if @@number-of-dfa-machines
		void lex( void );
		#endif

		//fn.stack.cpp
		bool alloc_stack( void );

		//fn.unicode.cpp
		UNICC_SCHAR* get_lexem( void );

	public:
		// EOF behavior
		UNICC_CHAR			eof;

		// Abstract syntax tree
		@@prefix_ast*		ast;

		//fn.parse.cpp
		@@goal-type parse( void );

		//fn.ast.cpp
		@@prefix_ast* ast_free( @@prefix_ast* node );
		@@prefix_ast* ast_create( const char* emit, UNICC_SCHAR* token );
		void ast_print( FILE* stream, @@prefix_ast* node );
};
