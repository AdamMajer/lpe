MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS = -avoid-version -module -export-dynamic -rpath $(liblpedir)

liblpedir = $(libdir)/lpe

liblpe_LTLIBRARIES = @MODES@

EXTRA_LTLIBRARIES = \
	cmode.la cppmode.la perlmode.la htmlmode.la sgmlmode.la \
	mailmode.la javamode.la lispmode.la

cmode_la_SOURCES = cmode.c
cppmode_la_SOURCES = cppmode.c
perlmode_la_SOURCES = perlmode.c
htmlmode_la_SOURCES = htmlmode.c
sgmlmode_la_SOURCES = sgmlmode.c
mailmode_la_SOURCES = mailmode.c
javamode_la_SOURCES = javamode.c
lispmode_la_SOURCES = lispmode.c
