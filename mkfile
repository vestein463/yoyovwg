BUILTINS =
me = /usr/fisher
opts =
objlist = base.o readgr.o checkmeta.o factorize.o checkgr.o parse.o earley.o \
	  ftree.o hnmatch.o common.o heap.o fprintf.o

$me/bin/vwg:	$objlist
		cc $opts $objlist
		mv a.out vwg

%.o:		vwg.h %.c
		cc $opts -c -O $stem.c

earley.o:	states.h
ftree.o:	states.h
fprintf.o:	states.h

clean:
		rm -f $objlist
