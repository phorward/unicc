bool @@prefix_parser::alloc_stack( void )
{
	if( !this->stacksize )
	{
		if( !( this->tos = this->stack = (@@prefix_tok*)malloc(
				UNICC_MALLOCSTEP * sizeof( @@prefix_tok ) ) ) )
		{
			UNICC_OUTOFMEM( this );
			return false;
		}

		this->stacksize = UNICC_MALLOCSTEP;
	}
	else if( ( this->tos - this->stack ) == this->stacksize )
	{
		size_t			size = ( this->tos - this->stack );
		@@prefix_tok*	ptr;

		if( !( ptr = (@@prefix_tok*)realloc( this->stack,
				( this->stacksize + UNICC_MALLOCSTEP )
					* sizeof( @@prefix_tok ) ) ) )
		{
			UNICC_OUTOFMEM( this );

			if( this->stack )
			{
				free( this->stack );
				this->tos = this->stack = NULL;
				this->stacksize = 0;
			}

			return false;
		}

		this->tos = this->stack = ptr;
		this->stacksize += UNICC_MALLOCSTEP;
		this->tos += size;
	}

	return true;
}
