MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = modes

bin_PROGRAMS = lpe

lpe_SOURCES = \
	buffer.c buffer.h \
	cfg-core.c cfg-core.h \
	cfg.c cfg.h \
	exports.c exports.h \
	genmode.c genmode.h \
	help.c help.h \
	highlight.h \
	i18n.h \
	input.c input.h \
	lpe.c lpe.h \
	lpecomm.c lpecomm.h \
	minibuf.c minibuf.h \
	mode-utils.c mode-utils.h \
	options.h \
	screen.c screen.h \
	strfuncs.c strfuncs.h \
	common.c common.h

INCLUDES = -DDATADIR=\"$(datadir)/lpe\"

lpe_LDADD = @LIBINTL@ @INTLOBJS@ @DL_LIB@ @SLANG_LIB@ -lncurses
lpe_LDFLAGS = @lpe_LDFLAGS@
lpe_DEPENDENCIES = @lpe_DEPS@ @INTLOBJS@

if USE_INCLUDED_LIBINTL
AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/lpe" -I$(top_srcdir)/intl
else
AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\"
endif

EXTRA_DIST = templ.c templ.h
