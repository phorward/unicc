# UniCC Standard C Parser Template
# Copyright (C) 2006-2016 by Phorward Software Technologies, Jan Max Meyer
# http://www.phorward-software.com ++ contact<<AT>>phorward-software<<DOT>>com
# ------------------------------------------------------------------------------
# AWK-Script to assemble skeleton parser templates both for XML-based
# UniCC standard-template engine as well as UniCC4C parser generator.
# The parsers both produced directly by UniCC and UniCC4C share the same
# code base; UniCC4C provides more possibilities and extensitive generation
# options for parsers, e.g. hard-coded parse tables, abstract syntax tree
# handling and traversion, etc.

BEGIN		{
				on_gen = 1
				tab = 1
				print_as_code = 0
				line = 0

				if( genwhat == "" )
					genwhat = "UNICC4C"
			}

			{
				line++
			}

/%%%ifgen/	{
				if( $2 == genwhat )
					on_gen = 1
				else
					on_gen = 0

				print_as_code = 0
				next
			}

/%%%end/	{
				on_gen = 1
				print_as_code = 0
				next
			}

/%%%code}/	{
				if( on_gen )
					print_as_code = 0
				next
			}

/%%%code{/	{
				if( on_gen )
					print_as_code = 1
			}

/%%%code/	{
				if( on_gen )
				{
					gsub( $1, "" )
					$0 = trim( $0 )

					if( trim( $0 ) == "}" )
						tab--;

					print tabs() $0

					if( trim( $0 ) == "{" )
						tab++;
				}
				next
			}

/%%%tabs/	{
				tab = $2
				next
			}

/%%%line/	{
				print "#line " line
				next
			}

/%%%include/{
				system( ARGV[0] " -v genwhat=" genwhat " -f mkparser.awk " $2 )
				next
			}

			{
				if( !on_gen )
					next

				if( print_as_code )
				{
					print
					next
				}

				if( genwhat == "STDTPL" )
				{
					if( substr( FILENAME, length( FILENAME ) - 3 ) != ".xml" )
					{
						gsub( "&", "\\&amp;" )
						gsub( /[<]/, "\\&lt;" )
						gsub( /[>]/, "\\&gt;" )
					}
					print
				}
				else
				{
					if( trim( $0 ) == "" )
						print tabs() "EL( &out );"
					else
					{
						gsub( /\\/, "\\\\" )
						gsub( /\"/, "\\\"" )
						gsub( /%/, "%%" )
						print tabs() "L( &out, \"" $0 "\" );"
					}
				}
			}

function tabs()
{
	rt = ""
	for( i = 1; i <= tab; i++ )
		rt = rt "\t"

	return rt
}

function ltrim( s )
{
	sub( /^[ \t\n]+/, "", s )
	return s
}

function rtrim( s )
{
	sub( /[ \t\n]+$/, "", s )
	return s
}

function trim( s )
{
	return rtrim( ltrim( s ) )
}

