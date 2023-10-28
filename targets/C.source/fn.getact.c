UNICC_STATIC int @@prefix_get_act( @@prefix_pcb* pcb )
{
    int i;

    for( i = 1; i < @@prefix_act[ pcb->tos->state ][0] * 3; i += 3 )
    {
        if( @@prefix_act[ pcb->tos->state ][i] == pcb->sym )
        {
            if( ( pcb->act = @@prefix_act[ pcb->tos->state ][i+1] )
                    == UNICC_ERROR )
                return 0; /* Force parse error! */

            pcb->idx = @@prefix_act[ pcb->tos->state ][i+2];
            return 1;
        }
    }

    /* Default production */
    if( ( pcb->idx = @@prefix_def_prod[ pcb->tos->state ] ) > -1 )
    {
        pcb->act = 1; /* Reduce */
        return 1;
    }

    return 0;
}
