MAINTAINERCLEANFILES = Makefile.in init.sl init.slc conv.slc

slang_compiled = init.slc conv.slc
slang_sources = $(srcdir)/init.sl $(srcdir)/conv.sl

noinst_PROGRAMS = comp

comp_SOURCES = comp.c
comp_LDADD = @SLANG_LIB@

lpe_datadir = $(datadir)/lpe
lpe_data_DATA = $(slang_sources) $(slang_compiled)

exdir = $(datadir)/doc/lpe/examples
ex_DATA = custom.ex

EXTRA_DIST = $(ex_DATA) $(slang_sources) init.sl.in

init.sl: init.sl.in Makefile
	sed -e 's^\@SYSCONFDIR\@^$(sysconfdir)^g' \
	    -e 's^\@DATADIR\@^$(datadir)^g' \
		< $(srcdir)/init.sl.in > init.sl.tmp \
	&& mv init.sl.tmp init.sl

%.slc: %.sl comp
	$(top_builddir)/data/comp $<
