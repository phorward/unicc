/* Abstract Syntax Tree */
typedef struct @@prefix_AST @@prefix_ast;

struct @@prefix_AST
{
	char*			emit;
	UNICC_SCHAR*	token;

	@@prefix_ast*	parent;
	@@prefix_ast*	child;
	@@prefix_ast*	prev;
	@@prefix_ast*	next;
};
