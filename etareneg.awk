# etareneg: Script to in-place generate & substitute code segments in C programs

BEGIN				{
						in_gen = 0
					}

/\/\*GENERATE/		{
						print

						if( in_gen )
							next

						in_gen = 1

						gsub( "\\/\\*GENERATE", "" )
						gsub( "\\*\\/$", "" )

						system( $0 )

						next
					}

/\/\*ETARENEG/		{
						print
						in_gen = 0

						next
					}

					{
						if( !in_gen )
							print
					}
