# $Id: dist.mk,v 1.283 2001/12/22 13:08:07 tom Exp $
# Makefile for creating ncurses distributions.
#
# This only needs to be used directly as a makefile by developers, but
# configure mines the current version number out of here.  To move
# to a new version number, just edit this file and run configure.
#
SHELL = /bin/sh

# These define the major/minor/patch versions of ncurses.
NCURSES_MAJOR = 5
NCURSES_MINOR = 2
NCURSES_PATCH = 20011222

# We don't append the patch to the version, since this only applies to releases
VERSION = $(NCURSES_MAJOR).$(NCURSES_MINOR)

DUMP	= lynx -dump
DUMP2	= $(DUMP) -nolist

GNATHTML= `type -p gnathtml || type -p gnathtml.pl`

# man2html 3.0.1 is a Perl script which assumes that pages are fixed size.
# Not all man programs agree with this assumption; some use half-spacing, which
# has the effect of lengthening the text portion of the page -- so man2html
# would remove some text.  The man program on Redhat 6.1 appears to work with
# man2html if we set the top/bottom margins to 6 (the default is 7).
MAN2HTML= man2html -botm=6 -topm=6 -cgiurl '$$title.$$section$$subsection.html'

ALL	= ANNOUNCE doc/html/announce.html doc/ncurses-intro.doc doc/hackguide.doc manhtml adahtml

all :	$(ALL)

dist:	$(ALL)
	(cd ..;  tar cvf ncurses-$(VERSION).tar `sed <ncurses-$(VERSION)/MANIFEST 's/^./ncurses-$(VERSION)/'`;  gzip ncurses-$(VERSION).tar)

distclean:
	rm -f $(ALL) subst.tmp subst.sed MANIFEST.tmp

# Don't mess with announce.html.in unless you have lynx available!
doc/html/announce.html: announce.html.in
	sed 's,@VERSION@,$(VERSION),' <announce.html.in > $@

ANNOUNCE : doc/html/announce.html
	$(DUMP) doc/html/announce.html > $@

doc/ncurses-intro.doc: doc/html/ncurses-intro.html
	$(DUMP2) doc/html/ncurses-intro.html > $@
doc/hackguide.doc: doc/html/hackguide.html
	$(DUMP2) doc/html/hackguide.html > $@

MANPROG	= tbl | nroff -man

manhtml: MANIFEST
	@rm -f doc/html/man/*.html
	@mkdir -p doc/html/man
	@rm -f subst.tmp ;
	@for f in man/*.[0-9]*; do \
	   m=`basename $$f` ;\
	   x=`echo $$m | awk -F. '{print $$2;}'` ;\
	   xu=`echo $$x | dd conv=ucase 2>/dev/null` ;\
	   if [ "$${x}" != "$${xu}" ]; then \
	     echo "s/$${xu}/$${x}/g" >> subst.tmp ;\
	   fi ;\
	done
	# change some things to make weblint happy:
	@echo 's/<B>/<STRONG>/g'     >> subst.tmp
	@echo 's/<\/B>/<\/STRONG>/g' >> subst.tmp
	@echo 's/<I>/<EM>/g'         >> subst.tmp
	@echo 's/<\/I>/<\/EM>/g'     >> subst.tmp
	@echo 's/<\/TITLE>/<\/TITLE><link rev=made href="mailto:bug-ncurses@gnu.org">/' >> subst.tmp
	@sort < subst.tmp | uniq > subst.sed
	@rm -f subst.tmp
	@for f in man/*.[0-9]* ; do \
	   m=`basename $$f` ;\
	   T=`egrep '^.TH' $$f|sed -e 's/^.TH //' -e s'/"//g' -e 's/[ 	]\+$$//'` ; \
	   g=$${m}.html ;\
	   if [ -f doc/html/$$g ]; then chmod +w doc/html/$$g; fi;\
	   echo "Converting $$m to HTML" ;\
	   echo '<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">' > doc/html/man/$$g ;\
	   echo '<!-- ' >> doc/html/man/$$g ;\
	   egrep '^.\\"[^#]' $$f | \
	   	sed	-e 's/\$$/@/g' \
			-e 's/^.../  */' \
			-e 's/</\&lt;/g' \
			-e 's/>/\&gt;/g' \
	   >> doc/html/man/$$g ;\
	   echo '-->' >> doc/html/man/$$g ;\
	   man/edit_man.sh editing /usr/man man $$f | $(MANPROG) | tr '\255' '-' | $(MAN2HTML) -title "$$T" | \
	   sed -f subst.sed |\
	   sed -e 's/"curses.3x.html"/"ncurses.3x.html"/g' \
	   >> doc/html/man/$$g ;\
	done
	@rm -f subst.sed
	@sed -e "\%./doc/html/man/%d" < MANIFEST > MANIFEST.tmp
	@find ./doc/html/man -type f -print >> MANIFEST.tmp
	@chmod u+w MANIFEST
	@sort -u < MANIFEST.tmp > MANIFEST
	@rm -f MANIFEST.tmp

#
# Please note that this target can only be properly built if the build of the
# Ada95 subdir has been done.  The reason is, that the gnathtml tool uses the
# .ali files generated by the Ada95 compiler during the build process.  These
# .ali files contain cross referencing information required by gnathtml.
adahtml: MANIFEST
	if [ ! -z "$(GNATHTML)" ]; then \
	  (cd ./Ada95/gen ; make html) ;\
	  sed -e "\%./doc/html/ada/%d" < MANIFEST > MANIFEST.tmp ;\
	  find ./doc/html/ada -type f -print >> MANIFEST.tmp ;\
	  sort -u < MANIFEST.tmp > MANIFEST ;\
	  rm -f MANIFEST.tmp ;\
	fi

# Prepare distribution for version control
vcprepare:
	find . -type d -exec mkdir {}/RCS \;

# Write-lock almost all files not under version control.
ADA_EXCEPTIONS=$(shell eval 'a="\\\\\|";for x in Ada95/gen/terminal*.m4; do echo -n $${a}Ada95/ada_include/`basename $${x} .m4`; done')
EXCEPTIONS = 'announce.html$\\|ANNOUNCE\\|misc/.*\\.doc\\|man/terminfo.5\\|lib_gen.c'$(ADA_EXCEPTIONS)
writelock:
	for x in `grep -v $(EXCEPTIONS) MANIFEST`; do if [ ! -f `dirname $$x`/RCS/`basename $$x`,v ]; then chmod a-w $${x}; fi; done

# This only works on a clean source tree, of course.
MANIFEST:
	-rm -f $@
	touch $@
	find . -type f -print |sort | fgrep -v .lsm |fgrep -v .spec >$@

TAGS:
	etags */*.[ch]

# Makefile ends here
