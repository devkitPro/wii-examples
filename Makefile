SUBDIRS:= `ls | egrep -v '^(CVS)$$'`

DATESTRING	:=	$(shell date +%Y)$(shell date +%m)$(shell date +%d)

all:
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then make  -C $$i || { exit 1;} fi; done;

clean:
	@rm -f *.bz2
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then make  -C $$i clean || { exit 1;} fi; done;

install:
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then make  -C $$i install || { exit 1;} fi; done;

dist: clean
	@tar --exclude=*CVS* -cvjf gamecube-examples-$(DATESTRING).tar.bz2 *
