#include <config.h>
#include <slang.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main ( int argc, char **argv )
{
	int i;
	char *s;
	char *c1="byte_compile_file(\"";

	SLang_init_all ();

	for ( i = 1; i < argc; i++ )
	{
		printf ("Compiling: %s ...", argv[i] );
		s = (char *) malloc ( strlen ( argv[i] ) + strlen ( c1 ) + 6 );
		s[0]='\0';
		strcat(s,c1);
		strcat(s,argv[i]);
		strcat(s,"\",0);");
		SLang_load_string ( s );
		free ( s );
		printf (" done.\n");
	}
	return 0;
}
