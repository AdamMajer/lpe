MAINTAINERCLEANFILES = lpe.1 Makefile.in

lpe.1: $(top_srcdir)/man/lpe.1.en
	cp $(top_srcdir)/man/lpe.1.en $(top_builddir)/man/lpe.1

clean-local:
	rm -f lpe.1

man_MANS = lpe.1

EXTRA_MANPAGES = lpe.1.bg lpe.1.cs lpe.1.de lpe.1.es lpe.1.fr lpe.1.pl \
		 lpe.1.ru

EXTRA_DIST = $(EXTRA_MANPAGES) lpe.1.en

install-data-hook:
	for m in $(EXTRA_MANPAGES); do \
		$(mkinstalldirs) $(DESTDIR)/$(mandir)/`echo $$m | sed -e "s,^.*\.1\.\(.*\)$$,\1/man1,"`; \
		$(INSTALL_DATA) $(top_srcdir)/man/$$m \
			$(DESTDIR)/$(mandir)/`echo $$m | sed -e "s,^\(.*\)\.1\.\(.*\)$$,\2/man1/\1.1,"`; \
	done
