% conv.sl - convenience functions for lpe configuration handling
% (C) 2000 Chris Smith
% Written by Gergely Nagy <algernon@debian.org>
%

define lpe_set_bkg ( color )
{
	lpe_set_option ( "global.background.default", color );
}

define lpe_set_color ( id, color )
{
	lpe_set_option ( "global.color." + id,  color );
}

define lpe_set_eof_fill ( str )
{
	lpe_set_option ( "global.general.eof_fill", str );
}

define lpe_set_hard_tab_width ( width )
{
	lpe_set_option ( "global.general.hard_tab_width", width );
}

define lpe_set_soft_tab_width ( width )
{
	lpe_set_option ( "global.general.soft_tab_width", width );
}

define lpe_save_option ( option )
{
	variable fp, fname, v;

	if ( getenv ("HOME") == NULL )
		return;

	fname = getenv("HOME") + "/.lpe/custom";
	mkdir ( getenv("HOME") + "/.lpe", 0755 );

%	% Commented out, because if the directory does not exist yet
%	% this one stops with an error.
%
%	if ( errno != EEXIST )
%	{
%		fprintf ( stderr, "mkdir %s failed: %s", getenv ("HOME") + "/.lpe", errno_string ( errno ) );
%		return;
%	}

	fp = fopen ( fname, "a+" );
	if ( fp == NULL ) 
	{
		fprintf ( stderr, "%s could not be opened", fname );
		return;
	}

	fprintf ( fp, "\n%% Automatically written by lpe_save_option\n");
	fprintf ( fp, "%% %s\n", time() );

	fprintf ( fp, "lpe_set_option ( \"%s\", \"%s\" );\n", option, _lpe_cfg_core_get_str ( option ) );

	fclose ( fp );
}
