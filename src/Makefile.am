MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -DPLUGINDIR=\"$(libdir)/lpe\" -I$(top_srcdir)/src/lib

SUBDIRS = lib modes

bin_PROGRAMS = lpe

lpe_SOURCES = \
	highlight.h \
	i18n.h \
	options.h

lpe_LDADD = $(top_srcdir)/src/lib/liblpe.la
lpe_LDFLAGS = @lpe_LDFLAGS@

EXTRA_DIST = templ.c templ.h
