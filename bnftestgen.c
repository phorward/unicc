Symbol*	n_opt_sequence;
Symbol*	n_inline;
Symbol*	n_sequence;
Symbol*	n_assocdef;
Symbol*	t_Regex;
Symbol*	n_alternation[ 2 ];
Symbol*	n_definition;
Symbol*	t_Function;
Symbol*	t_Flag_goal;
Symbol*	t_Identifier;
Symbol*	t_Flag_ignore;
Symbol*	n_rule;
Symbol*	t_Int;
Symbol*	n_modifier;
Symbol*	n_pos_terminal;
Symbol*	n_grammar[ 2 ];
Symbol*	n_pos_grammar;
Symbol*	n_colon;
Symbol*	t_CCL;
Symbol*	n_opt_Flag_goal;
Symbol*	n_opt_emits;
Symbol*	_t_noname[ 16 ];
Symbol*	n_terminal;
Symbol*	n_symbol;
Symbol*	t_Token;
Symbol*	n_emits;
Symbol*	n_pos_alternation;
Symbol*	t_String;

/* Symbols */

t_Flag_goal = sym_create( g, "Flag_goal" );
t_Flag_goal->str = "$";
t_Flag_goal->emit = t_Flag_goal->name;

_t_noname[ 0 ] = sym_create( g, ":=" );
_t_noname[ 0 ]->str = ":=";

_t_noname[ 1 ] = sym_create( g, ":" );
_t_noname[ 1 ]->str = ":";

_t_noname[ 2 ] = sym_create( g, "=" );
_t_noname[ 2 ]->str = "=";

_t_noname[ 3 ] = sym_create( g, "(" );
_t_noname[ 3 ]->str = "(";

_t_noname[ 4 ] = sym_create( g, ")" );
_t_noname[ 4 ]->str = ")";

_t_noname[ 5 ] = sym_create( g, "*" );
_t_noname[ 5 ]->str = "*";

_t_noname[ 6 ] = sym_create( g, "+" );
_t_noname[ 6 ]->str = "+";

_t_noname[ 7 ] = sym_create( g, "?" );
_t_noname[ 7 ]->str = "?";

_t_noname[ 8 ] = sym_create( g, "|" );
_t_noname[ 8 ]->str = "|";

_t_noname[ 9 ] = sym_create( g, "@" );
_t_noname[ 9 ]->str = "@";

_t_noname[ 10 ] = sym_create( g, "<" );
_t_noname[ 10 ]->str = "<";

_t_noname[ 11 ] = sym_create( g, ">" );
_t_noname[ 11 ]->str = ">";

_t_noname[ 12 ] = sym_create( g, "^" );
_t_noname[ 12 ]->str = "^";

_t_noname[ 13 ] = sym_create( g, (char*)NULL );
_t_noname[ 13 ]->ptn = pregex_ptn_create( "[\\t\\n\\r ]+", 0 );
_t_noname[ 13 ]->flags.whitespace = TRUE;

_t_noname[ 14 ] = sym_create( g, (char*)NULL );
_t_noname[ 14 ]->ptn = pregex_ptn_create( "//[^\\n]*\\n", 0 );
_t_noname[ 14 ]->flags.whitespace = TRUE;

_t_noname[ 15 ] = sym_create( g, (char*)NULL );
_t_noname[ 15 ]->ptn = pregex_ptn_create( "/\\*([^*]|\\*[^/])*\\*/|//[^\\n]*\\n|#[^\\n]*\\n", 0 );
_t_noname[ 15 ]->flags.whitespace = TRUE;

t_Identifier = sym_create( g, "Identifier" );
t_Identifier->ptn = pregex_ptn_create( "[A-Z_a-z][0-9A-Z_a-z]*", 0 );
t_Identifier->emit = t_Identifier->name;

t_CCL = sym_create( g, "CCL" );
t_CCL->ptn = pregex_ptn_create( "\\[(\\\\.|[^\\\\\\]])*\\]", 0 );
t_CCL->emit = t_CCL->name;

t_String = sym_create( g, "String" );
t_String->ptn = pregex_ptn_create( "'[^']*'", 0 );
t_String->emit = t_String->name;

t_Token = sym_create( g, "Token" );
t_Token->ptn = pregex_ptn_create( "\"[^\"]*\"", 0 );
t_Token->emit = t_Token->name;

t_Regex = sym_create( g, "Regex" );
t_Regex->ptn = pregex_ptn_create( "/(\\\\.|[^/\\\\])*/", 0 );
t_Regex->emit = t_Regex->name;

t_Int = sym_create( g, "Int" );
t_Int->ptn = pregex_ptn_create( "[0-9]+", 0 );
t_Int->emit = t_Int->name;

t_Function = sym_create( g, "Function" );
t_Function->ptn = pregex_ptn_create( "[A-Z_a-z][0-9A-Z_a-z]*\\(\\)", 0 );
t_Function->emit = t_Function->name;

t_Flag_ignore = sym_create( g, "Flag_ignore" );
t_Flag_ignore->ptn = pregex_ptn_create( "%(ignore|skip)", 0 );
t_Flag_ignore->emit = t_Flag_ignore->name;

n_colon = sym_create( g, "colon" );

n_emits = sym_create( g, "emits" );
n_emits->emit = n_emits->name;

n_terminal = sym_create( g, "terminal" );

n_inline = sym_create( g, "inline" );
n_inline->emit = n_inline->name;

n_alternation[ 0 ] = sym_create( g, "alternation" );

n_symbol = sym_create( g, "symbol" );

n_modifier = sym_create( g, "modifier" );

n_sequence = sym_create( g, "sequence" );

n_rule = sym_create( g, "rule" );
n_rule->emit = n_rule->name;

n_opt_sequence = sym_create( g, "opt_sequence" );

n_opt_emits = sym_create( g, "opt_emits" );

n_alternation[ 1 ] = sym_create( g, "alternation'" );

n_pos_alternation = sym_create( g, "pos_alternation'" );

n_definition = sym_create( g, "definition" );
n_definition->emit = n_definition->name;

n_opt_Flag_goal = sym_create( g, "opt_Flag_goal" );

n_assocdef = sym_create( g, "assocdef" );

n_pos_terminal = sym_create( g, "pos_terminal" );

n_grammar[ 0 ] = sym_create( g, "grammar" );
g->goal = n_grammar[ 0 ];

n_grammar[ 1 ] = sym_create( g, "grammar'" );

n_pos_grammar = sym_create( g, "pos_grammar'" );

/* Productions */

prod_create( g, n_colon /* colon */,
	_t_noname[ 0 ], /* ":=" */
	(Symbol*)NULL
)->emit = "emitsdef";

prod_create( g, n_colon /* colon */,
	_t_noname[ 1 ], /* ":" */
	(Symbol*)NULL
);

prod_create( g, n_emits /* emits */,
	_t_noname[ 2 ], /* "=" */
	t_Identifier, /* /[A-Z_a-z][0-9A-Z_a-z]*\/ */
	(Symbol*)NULL
);

prod_create( g, n_terminal /* terminal */,
	t_CCL, /* /\\[(\\\\.|[^\\\\\\]])*\\]/ */
	(Symbol*)NULL
);
prod_create( g, n_terminal /* terminal */,
	t_String, /* /'[^']*'/ */
	(Symbol*)NULL
);
prod_create( g, n_terminal /* terminal */,
	t_Token, /* /"[^"]*"/ */
	(Symbol*)NULL
);
prod_create( g, n_terminal /* terminal */,
	t_Regex, /* /\/(\\\\.|[^\/\\\\])*\// */
	(Symbol*)NULL
);
prod_create( g, n_terminal /* terminal */,
	t_Function, /* /[A-Z_a-z][0-9A-Z_a-z]*\\(\\)/ */
	(Symbol*)NULL
);

prod_create( g, n_inline /* inline */,
	_t_noname[ 3 ], /* "(" */
	n_alternation[ 0 ], /* alternation */
	_t_noname[ 4 ], /* ")" */
	(Symbol*)NULL
);

prod_create( g, n_symbol /* symbol */,
	t_Identifier, /* /[A-Z_a-z][0-9A-Z_a-z]*\/ */
	(Symbol*)NULL
);
prod_create( g, n_symbol /* symbol */,
	n_terminal, /* terminal */
	(Symbol*)NULL
);
prod_create( g, n_symbol /* symbol */,
	n_inline, /* inline */
	(Symbol*)NULL
);

prod_create( g, n_modifier /* modifier */,
	n_symbol, /* symbol */
	_t_noname[ 5 ], /* "*" */
	(Symbol*)NULL
)->emit = "kle";

prod_create( g, n_modifier /* modifier */,
	n_symbol, /* symbol */
	_t_noname[ 6 ], /* "+" */
	(Symbol*)NULL
)->emit = "pos";

prod_create( g, n_modifier /* modifier */,
	n_symbol, /* symbol */
	_t_noname[ 7 ], /* "?" */
	(Symbol*)NULL
)->emit = "opt";

prod_create( g, n_modifier /* modifier */,
	n_symbol, /* symbol */
	(Symbol*)NULL
);

prod_create( g, n_sequence /* sequence */,
	n_sequence, /* sequence */
	n_modifier, /* modifier */
	(Symbol*)NULL
);
prod_create( g, n_sequence /* sequence */,
	n_modifier, /* modifier */
	(Symbol*)NULL
);

prod_create( g, n_rule /* rule */,
	n_opt_sequence, /* opt_sequence */
	n_opt_emits, /* opt_emits */
	(Symbol*)NULL
);

prod_create( g, n_opt_sequence /* opt_sequence */,
	n_sequence, /* sequence */
	(Symbol*)NULL
);
prod_create( g, n_opt_sequence /* opt_sequence */,
	(Symbol*)NULL
);

prod_create( g, n_opt_emits /* opt_emits */,
	n_emits, /* emits */
	(Symbol*)NULL
);
prod_create( g, n_opt_emits /* opt_emits */,
	(Symbol*)NULL
);

prod_create( g, n_alternation[ 0 ] /* alternation */,
	n_rule, /* rule */
	n_pos_alternation, /* pos_alternation' */
	(Symbol*)NULL
)->emit = "alternation";


prod_create( g, n_alternation[ 1 ] /* alternation' */,
	_t_noname[ 8 ], /* "|" */
	n_rule, /* rule */
	(Symbol*)NULL
);

prod_create( g, n_pos_alternation /* pos_alternation' */,
	n_pos_alternation, /* pos_alternation' */
	n_alternation[ 1 ], /* alternation' */
	(Symbol*)NULL
);
prod_create( g, n_pos_alternation /* pos_alternation' */,
	n_alternation[ 1 ], /* alternation' */
	(Symbol*)NULL
);

prod_create( g, n_alternation[ 0 ] /* alternation */,
	n_rule, /* rule */
	(Symbol*)NULL
);

prod_create( g, n_definition /* definition */,
	_t_noname[ 9 ], /* "@" */
	t_Identifier, /* /[A-Z_a-z][0-9A-Z_a-z]*\/ */
	n_opt_Flag_goal, /* opt_Flag_goal */
	n_colon, /* colon */
	n_alternation[ 0 ], /* alternation */
	(Symbol*)NULL
);

prod_create( g, n_opt_Flag_goal /* opt_Flag_goal */,
	t_Flag_goal, /* "$" */
	(Symbol*)NULL
);
prod_create( g, n_opt_Flag_goal /* opt_Flag_goal */,
	(Symbol*)NULL
);

prod_create( g, n_definition /* definition */,
	t_Flag_ignore, /* /%(ignore|skip)/ */
	n_terminal, /* terminal */
	n_opt_emits, /* opt_emits */
	(Symbol*)NULL
);

prod_create( g, n_assocdef /* assocdef */,
	_t_noname[ 10 ], /* "<" */
	n_pos_terminal, /* pos_terminal */
	(Symbol*)NULL
)->emit = "assoc_left";


prod_create( g, n_pos_terminal /* pos_terminal */,
	n_pos_terminal, /* pos_terminal */
	n_terminal, /* terminal */
	(Symbol*)NULL
);
prod_create( g, n_pos_terminal /* pos_terminal */,
	n_terminal, /* terminal */
	(Symbol*)NULL
);

prod_create( g, n_assocdef /* assocdef */,
	_t_noname[ 11 ], /* ">" */
	n_pos_terminal, /* pos_terminal */
	(Symbol*)NULL
)->emit = "assoc_right";

prod_create( g, n_assocdef /* assocdef */,
	_t_noname[ 12 ], /* "^" */
	n_pos_terminal, /* pos_terminal */
	(Symbol*)NULL
)->emit = "assoc_none";


prod_create( g, n_grammar[ 0 ] /* grammar */,
	n_pos_grammar, /* pos_grammar' */
	(Symbol*)NULL
);

prod_create( g, n_grammar[ 1 ] /* grammar' */,
	n_definition, /* definition */
	(Symbol*)NULL
);
prod_create( g, n_grammar[ 1 ] /* grammar' */,
	n_assocdef, /* assocdef */
	(Symbol*)NULL
);

prod_create( g, n_pos_grammar /* pos_grammar' */,
	n_pos_grammar, /* pos_grammar' */
	n_grammar[ 1 ], /* grammar' */
	(Symbol*)NULL
);
prod_create( g, n_pos_grammar /* pos_grammar' */,
	n_grammar[ 1 ], /* grammar' */
	(Symbol*)NULL
);

