/* Stack Token */
typedef struct
{
	@@prefix_vtype			value;
	@@prefix_ast*			node;

	const @@prefix_syminfo*	symbol;

	int						state;
	unsigned int			line;
	unsigned int			column;
} @@prefix_tok;

