# $Id$

LDLIBS= -lm -ldl -llua -llualib

# CFLAGS= -g3 -Wall -fprofile-arcs -ftest-coverage
CFLAGS= -g3 -Wall -ansi -pedantic

default: pptest puptest

pptest: pluto.o pptest.o

puptest: pluto.o puptest.o

test: pptest puptest pptest.lua puptest.lua
	./pptest
	./puptest

clean:
	-rm *.o *.bb *.bbg *.da *.gcov pptest puptest test.plh
