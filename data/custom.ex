% Example ~/.lpe/custom
% by Gergely Nagy <algernon@debian.org>
%

define htw (width)
{
	lpe_set_hard_tab_width (width);
}

define stw (width)
{
	lpe_set_soft_tab_width (width);
}

define bkg (color)
{
	lpe_set_bkg (color);
}

define efill (str)
{
	lpe_set_eof_fill (str);
}

define setcolor ( mode, id, fg, bg )
{
	lpe_set_option ( mode + ".color." + id, fg );
	lpe_set_option ( mode + ".background." + id, bg );
}

%define sc ()
%{
%	lpe_save_settings ();
%}

define so ( option )
{
	lpe_save_option ( option );
}

setcolor ( "lispmode", "comment", "yellow", "red" );
stw ( 2 );
htw ( 4 );
