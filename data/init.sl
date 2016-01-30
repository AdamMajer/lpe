% init.sl - basic functions for lpe configuration handling
% (C) 2000 Chris Smith
% Written by Gergely Nagy <algernon@debian.org>
%

if ( lpe_exists ( "/usr/share/lpe/conv.sl" ) )
	evalfile ( "/usr/share/lpe/conv.sl" );

if ( lpe_exists ( "/etc/lperc" ) )
	evalfile ( "/etc/lperc" );

if ( LPE_CONFIG_FILE != NULL & lpe_exists ( LPE_CONFIG_FILE ) )
 	evalfile ( LPE_CONFIG_FILE );
