/* Typedef for symbol information table */
typedef struct
{
    const char*	name;
    const char*	emit;
    short		type;
    bool		lexem;
    bool		whitespace;
    bool		greedy;
} @@prefix_syminfo;
