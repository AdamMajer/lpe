SUBDIRS = data intl man po src

MAINTAINERCLEANFILES = ABOUT-NLS INSTALL Makefile.in aclocal.m4 config.guess \
		     config.h.in config.sub configure install-sh ltconfig \
		     ltmain.sh missing mkinstalldirs stamp-h.in

docdir = $(datadir)/doc/lpe
doc_DATA = AUTHORS BUGS COPYING CUSTOMIZE IDEAS MODES NEWS README TODO

EXTRA_DIST = $(doc_DATA) autogen.sh

dist-hook:
	chmod -R go-w $(distdir)

bin-dist: all
	rm -rf $(PACKAGE)-$(VERSION).bin
	$(MAKE) prefix=`pwd`/$(PACKAGE)-$(VERSION).bin/$(prefix) install
	@if test "x$(GZIP_ENV)" = "x"; then \
		if test "x$(GZIP)" = "x"; then \
			GZIP="--best"; \
		else \
			GZIP_ENV=$(GZIP); \
		fi \
	fi
	GZIP=$(GZIP_ENV) $(TAR) chozf $(PACKAGE)-$(VERSION).bin.tar.gz $(PACKAGE)-$(VERSION).bin
	rm -rf $(PACKAGE)-$(VERSION).bin

cvs-dist: dist
	rm -rf $(PACKAGE)-$(VERSION)CVS
	rm -rf $(PACKAGE)-$(VERSION)
	tar xhozf $(PACKAGE)-$(VERSION).tar.gz
	(cd $(PACKAGE)-$(VERSION); ./configure --enable-maintainer-mode ; make maintainer-clean; cd ..)
	mv $(PACKAGE)-$(VERSION) $(PACKAGE)-$(VERSION)CVS
	@if test "x$(GZIP_ENV)" = "x"; then \
		if test "x$(GZIP)" = "x"; then \
			GZIP_ENV="--best"; \
		else \
			GZIP_ENV="$(GZIP)"; \
		fi \
	fi
	GZIP=$(GZIP_ENV) $(TAR) chozf $(PACKAGE)-$(VERSION)CVS.tar.gz $(PACKAGE)-$(VERSION)CVS
	rm -rf $(PACKAGE)-$(VERSION)CVS
