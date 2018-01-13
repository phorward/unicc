bool @@prefix_parser::get_go( void )
{
	for( int i = 1; i < this->go[ this->tos->state ][0] * 3; i += 3 )
	{
		if( this->go[ this->tos->state ][i] == this->lhs )
		{
			this->act = this->go[ this->tos->state ][ i + 1 ];
			this->idx = this->go[ this->tos->state ][ i + 2 ];
			return true;
		}
	}

	return false;
}
