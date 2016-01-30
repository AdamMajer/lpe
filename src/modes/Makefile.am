MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS = -I$(top_srcdir)/src

liblpedir = $(libdir)/lpe

liblpe_LTLIBRARIES = @MODES@

EXTRA_LTLIBRARIES = \
	cmode.la cppmode.la perlmode.la htmlmode.la sgmlmode.la \
	mailmode.la javamode.la lispmode.la

cmode_la_SOURCES = cmode.c
cmode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
cppmode_la_SOURCES = cppmode.c
cppmode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
perlmode_la_SOURCES = perlmode.c
perlmode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
htmlmode_la_SOURCES = htmlmode.c
htmlmode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
sgmlmode_la_SOURCES = sgmlmode.c
sgmlmode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
mailmode_la_SOURCES = mailmode.c
mailmode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
javamode_la_SOURCES = javamode.c
javamode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
lispmode_la_SOURCES = lispmode.c
lispmode_la_LDFLAGS = -no-undefined -avoid-version -module -export-dynamic
