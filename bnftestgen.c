Symbol*	n_opt_sequence;
Symbol*	n_inline;
Symbol*	n_sequence;
Symbol*	n_assocdef;
Symbol*	t_Regex;
Symbol*	n_goal;
Symbol*	n_alternation[ 2 ];
Symbol*	n_variable;
Symbol*	n_definition[ 2 ];
Symbol*	t_Identifier;
Symbol*	n_rule;
Symbol*	n_modifier;
Symbol*	n_pos_terminal;
Symbol*	n_grammar[ 2 ];
Symbol*	n_pos_grammar;
Symbol*	n_colon;
Symbol*	t_CCL;
Symbol*	t_ignoreskip;
Symbol*	n_opt_emits;
Symbol*	_t_noname[ 17 ];
Symbol*	n_terminal;
Symbol*	n_symbol;
Symbol*	t_Token;
Symbol*	n_emits;
Symbol*	n_pos_alternation;
Symbol*	n_pos_definition;
Symbol*	n_opt_goal;
Symbol*	t_String;

/* Symbols */

_t_noname[ 0 ] = sym_create( g, "$" );
_t_noname[ 0 ]->str = "$";

_t_noname[ 1 ] = sym_create( g, ":=" );
_t_noname[ 1 ]->str = ":=";

_t_noname[ 2 ] = sym_create( g, ":" );
_t_noname[ 2 ]->str = ":";

_t_noname[ 3 ] = sym_create( g, "=" );
_t_noname[ 3 ]->str = "=";

_t_noname[ 4 ] = sym_create( g, "(" );
_t_noname[ 4 ]->str = "(";

_t_noname[ 5 ] = sym_create( g, ")" );
_t_noname[ 5 ]->str = ")";

_t_noname[ 6 ] = sym_create( g, "*" );
_t_noname[ 6 ]->str = "*";

_t_noname[ 7 ] = sym_create( g, "+" );
_t_noname[ 7 ]->str = "+";

_t_noname[ 8 ] = sym_create( g, "?" );
_t_noname[ 8 ]->str = "?";

_t_noname[ 9 ] = sym_create( g, "|" );
_t_noname[ 9 ]->str = "|";

_t_noname[ 10 ] = sym_create( g, "@" );
_t_noname[ 10 ]->str = "@";

_t_noname[ 11 ] = sym_create( g, "<" );
_t_noname[ 11 ]->str = "<";

_t_noname[ 12 ] = sym_create( g, ">" );
_t_noname[ 12 ]->str = ">";

_t_noname[ 13 ] = sym_create( g, "^" );
_t_noname[ 13 ]->str = "^";

_t_noname[ 14 ] = sym_create( g, (char*)NULL );
_t_noname[ 14 ]->ptn = pregex_ptn_create( "[\\t\\n\\r ]+", 0 );
_t_noname[ 14 ]->flags.whitespace = TRUE;

_t_noname[ 15 ] = sym_create( g, (char*)NULL );
_t_noname[ 15 ]->ptn = pregex_ptn_create( "//[^\\n]*\\n", 0 );
_t_noname[ 15 ]->flags.whitespace = TRUE;

_t_noname[ 16 ] = sym_create( g, (char*)NULL );
_t_noname[ 16 ]->ptn = pregex_ptn_create( "/\\*([^*]|\\*[^/])*\\*/|//[^\\n]*\\n|#[^\\n]*\\n", 0 );
_t_noname[ 16 ]->flags.whitespace = TRUE;

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

t_ignoreskip = sym_create( g, "%(ignore|skip)" );
t_ignoreskip->ptn = pregex_ptn_create( "%(ignore|skip)", 0 );

n_goal = sym_create( g, "goal" );
n_goal->emit = n_goal->name;

n_colon = sym_create( g, "colon" );

n_emits = sym_create( g, "emits" );
n_emits->emit = n_emits->name;

n_terminal = sym_create( g, "terminal" );

n_inline = sym_create( g, "inline" );
n_inline->emit = n_inline->name;

n_alternation[ 0 ] = sym_create( g, "alternation" );

n_variable = sym_create( g, "variable" );
n_variable->emit = n_variable->name;

n_symbol = sym_create( g, "symbol" );

n_modifier = sym_create( g, "modifier" );

n_sequence = sym_create( g, "sequence" );

n_rule = sym_create( g, "rule" );
n_rule->emit = n_rule->name;

n_opt_sequence = sym_create( g, "opt_sequence" );

n_opt_emits = sym_create( g, "opt_emits" );

n_alternation[ 1 ] = sym_create( g, "alternation'" );

n_pos_alternation = sym_create( g, "pos_alternation'" );

n_definition[ 0 ] = sym_create( g, "definition" );

n_opt_goal = sym_create( g, "opt_goal" );

n_definition[ 1 ] = sym_create( g, "definition'" );

n_pos_definition = sym_create( g, "pos_definition'" );

n_assocdef = sym_create( g, "assocdef" );

n_pos_terminal = sym_create( g, "pos_terminal" );

n_grammar[ 0 ] = sym_create( g, "grammar" );
g->goal = n_grammar[ 0 ];

n_grammar[ 1 ] = sym_create( g, "grammar'" );

n_pos_grammar = sym_create( g, "pos_grammar'" );

/* Productions */

prod_create( g, n_goal /* goal */,
	_t_noname[ 0 ], /* "$" */
	(Symbol*)NULL
);

prod_create( g, n_colon /* colon */,
	_t_noname[ 1 ], /* ":=" */
	(Symbol*)NULL
)->emit = "emitsdef";

prod_create( g, n_colon /* colon */,
	_t_noname[ 2 ], /* ":" */
	(Symbol*)NULL
);

prod_create( g, n_emits /* emits */,
	_t_noname[ 3 ], /* "=" */
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

prod_create( g, n_inline /* inline */,
	_t_noname[ 4 ], /* "(" */
	n_alternation[ 0 ], /* alternation */
	_t_noname[ 5 ], /* ")" */
	(Symbol*)NULL
);

prod_create( g, n_variable /* variable */,
	t_Identifier, /* /[A-Z_a-z][0-9A-Z_a-z]*\/ */
	(Symbol*)NULL
);

prod_create( g, n_symbol /* symbol */,
	n_variable, /* variable */
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
	_t_noname[ 6 ], /* "*" */
	(Symbol*)NULL
)->emit = "kle";

prod_create( g, n_modifier /* modifier */,
	n_symbol, /* symbol */
	_t_noname[ 7 ], /* "+" */
	(Symbol*)NULL
)->emit = "pos";

prod_create( g, n_modifier /* modifier */,
	n_symbol, /* symbol */
	_t_noname[ 8 ], /* "?" */
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
);

prod_create( g, n_alternation[ 1 ] /* alternation' */,
	_t_noname[ 9 ], /* "|" */
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

prod_create( g, n_definition[ 0 ] /* definition */,
	_t_noname[ 10 ], /* "@" */
	n_variable, /* variable */
	n_opt_goal, /* opt_goal */
	n_colon, /* colon */
	n_alternation[ 0 ], /* alternation */
	(Symbol*)NULL
)->emit = "definition";


prod_create( g, n_opt_goal /* opt_goal */,
	n_goal, /* goal */
	(Symbol*)NULL
);
prod_create( g, n_opt_goal /* opt_goal */,
	(Symbol*)NULL
);

prod_create( g, n_definition[ 0 ] /* definition */,
	t_ignoreskip, /* /%(ignore|skip)/ */
	n_pos_definition, /* pos_definition' */
	(Symbol*)NULL
)->emit = "ignore";


prod_create( g, n_definition[ 1 ] /* definition' */,
	n_terminal, /* terminal */
	(Symbol*)NULL
);
prod_create( g, n_definition[ 1 ] /* definition' */,
	n_variable, /* variable */
	(Symbol*)NULL
);

prod_create( g, n_pos_definition /* pos_definition' */,
	n_pos_definition, /* pos_definition' */
	n_definition[ 1 ], /* definition' */
	(Symbol*)NULL
);
prod_create( g, n_pos_definition /* pos_definition' */,
	n_definition[ 1 ], /* definition' */
	(Symbol*)NULL
);

prod_create( g, n_assocdef /* assocdef */,
	_t_noname[ 11 ], /* "<" */
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
	_t_noname[ 12 ], /* ">" */
	n_pos_terminal, /* pos_terminal */
	(Symbol*)NULL
)->emit = "assoc_right";

prod_create( g, n_assocdef /* assocdef */,
	_t_noname[ 13 ], /* "^" */
	n_pos_terminal, /* pos_terminal */
	(Symbol*)NULL
)->emit = "assoc_none";


prod_create( g, n_grammar[ 0 ] /* grammar */,
	n_pos_grammar, /* pos_grammar' */
	(Symbol*)NULL
);

prod_create( g, n_grammar[ 1 ] /* grammar' */,
	n_definition[ 0 ], /* definition */
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

