/* Action Table */
UNICC_STATIC int @@prefix_act[ @@number-of-states ][ @@deepest-action-row * 3 + 1 ] =
{
@@action-table
};

/* GoTo Table */
UNICC_STATIC int @@prefix_go[ @@number-of-states ][ @@deepest-goto-row * 3 + 1 ] =
{
@@goto-table
};

/* Default productions per state */
UNICC_STATIC int @@prefix_def_prod[ @@number-of-states ] =
{
    @@default-productions
};

#if !@@mode
/* DFA selection table */
UNICC_STATIC int @@prefix_dfa_select[ @@number-of-states ] =
{
    @@dfa-select
};
#endif

#if @@number-of-dfa-machines
/* DFA index table */
UNICC_STATIC int @@prefix_dfa_idx[ @@number-of-dfa-machines ][ @@deepest-dfa-index-row ] =
{
@@dfa-index
};

/* DFA transition chars */
UNICC_STATIC int @@prefix_dfa_chars[ @@size-of-dfa-characters * 2 ] =
{
    @@dfa-char
};

/* DFA transitions */
UNICC_STATIC int @@prefix_dfa_trans[ @@size-of-dfa-characters ] =
{
    @@dfa-trans
};

/* DFA acception states */
UNICC_STATIC int @@prefix_dfa_accept[ @@number-of-dfa-machines ][ @@deepest-dfa-accept-row ] =
{
@@dfa-accept
};

#endif

/* Symbol information table */
UNICC_STATIC @@prefix_syminfo @@prefix_symbols[] =
{
@@symbols
};

/* Production information table */
UNICC_STATIC @@prefix_prodinfo @@prefix_productions[] =
{
@@productions
};
