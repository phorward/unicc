if( ( bottles = prompt( "Enter number of bottles [default=99]" ) ) == "" )
    bottles = 99;

if( integer( bottles ) <= 0 )
{
    print( "Sorry, but the input '" + bottles + "' is invalid." );
    exit( 1 );
}

while( bottles > 0 )
{
    if( bottles > 1 )
        print( bottles + " bottles of beer on the wall, " +
                bottles + " bottles of beer." );
    else
        print( "One bottle of beer on the wall, one bottle of beer." );

    print( "Take one down, pass it around." );
        
    if( ( bottles = bottles - 1 ) == 0 )
        print( "No more bottles of beer on the wall." );
    else if( bottles == 1 )
        print( "One more bottle of beer on the wall." );
    else
        print( bottles + " more bottles of beer on the wall." );
}
