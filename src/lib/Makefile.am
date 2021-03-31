MAINTAINERCLEANFILES = Makefile.in

if USE_INCLUDED_LIBINTL
AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/lpe" -I$(top_srcdir)/intl
else
AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\"
endif

AM_CPPFLAGS += -DDATADIR=\"$(datadir)/lpe\" -DPLUGINDIR=\"$(libdir)/lpe\" \
	-I$(top_srcdir)/src

noinst_LTLIBRARIES = liblpe.la

liblpe_la_SOURCES = \
	buffer.c buffer.h \
	cfg-core.c cfg-core.h \
	cfg.c cfg.h \
	common.c common.h \
	exports.c exports.h \
	genmode.c genmode.h \
	help.c help.h \
	input.c input.h \
	lpecomm.c lpecomm.h \
	lpe.c lpe.h \
	minibuf.c minibuf.h \
	mode-utils.c mode-utils.h \
	strfuncs.c strfuncs.h \
	screen.c screen.h

liblpe_la_LIBADD = @LIBINTL@ @INTLOBJS@ @DL_LIB@ @SLANG_LIB@ -lncurses -ltinfo
liblpe_la_LDFLAGS = @lpe_LDFLAGS@ -no-undefined -static
liblpe_la_DEPENDENCIES = @lpe_DEPS@ @INTLOBJS@
