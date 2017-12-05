/* Typedef for symbol information table */
typedef struct
{
	char*			name;
	short			type;
	UNICC_BOOLEAN	lexem;
	UNICC_BOOLEAN	whitespace;
	UNICC_BOOLEAN	greedy;
} @@prefix_syminfo;
