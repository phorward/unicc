#if UNICC_SYNTAXTREE
/* Parse tree node */
typedef struct @@prefix_SYNTREE @@prefix_syntree;

struct @@prefix_SYNTREE
{
	@@prefix_tok		symbol;
	UNICC_SCHAR*		token;

	@@prefix_syntree*	parent;
	@@prefix_syntree*	child;
	@@prefix_syntree*	prev;
	@@prefix_syntree*	next;
}; 
#endif
