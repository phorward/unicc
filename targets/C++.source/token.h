/* Stack Token */
typedef struct
{
	@@prefix_vtype		value;
	@@prefix_ast*		node;

	const
	@@prefix_syminfo*	symbol;

	int					state;

	size_t				line;
	size_t				column;
} @@prefix_tok;

