bool @@prefix_parser::get_act( void )
{
	for( int i = 1; i < this->actions[ this->tos->state ][0] * 3; i += 3 )
	{
		if( this->actions[ this->tos->state ][i] == this->sym )
		{
			if( ( this->act = this->actions[ this->tos->state ][i+1] )
					== UNICC_ERROR )
				return 0; /* Force parse error! */

			this->idx = this->actions[ this->tos->state ][i+2];
			return true;
		}
	}

	/* Default production */
	if( ( this->idx = this->def_prod[ this->tos->state ] ) > -1 )
	{
		this->act = 1; /* Reduce */
		return true;
	}

	return false;
}
