# Macros
CC = gcc217
# CC = gcc217m
# CFLAGS = -g -O2
# CFLAGS = -g
# CFLAGS = -D NDEBUG
# CFLAGS = -D NDEBUG -O
CFLAGS = -fprofile-arcs -ftest-coverage -g 

# Dependency rules for non-file targets
all: ishlex ishsyn ish

clobber: clean
	rm -f *~ \#*\#
clean:
	rm -f *.o

# Dependency rules for executable files
ishlex: ishlex.o lex.o dynarray.o
	$(CC) $(CFLAGS) ishlex.o lex.o dynarray.o -o $@

ishsyn: ishsyn.o lex.o dynarray.o command.o
	$(CC) $(CFLAGS) ishsyn.o lex.o dynarray.o command.o -o $@

ish: ish.o lex.o dynarray.o command.o
	$(CC) $(CFLAGS) ish.o lex.o dynarray.o command.o -o $@

# Dependency rules for projects object files
ishlex.o: ishlex.c ish.h lex.h dynarray.h
	$(CC) $(CFLAGS) -c $<

ishsyn.o: ishsyn.c ish.h lex.h dynarray.h
	$(CC) $(CFLAGS) -c $<

ish.o: ish.c ish.h lex.h command.h dynarray.h
	$(CC) $(CFLAGS) -c $<

lex.o: lex.c lex.h ish.h dynarray.h
	$(CC) $(CFLAGS) -c $<

command.o: command.c command.h ish.h lex.h dynarray.h
	$(CC) $(CFLAGS) -c $<

dynarray.o: dynarray.c dynarray.h
	$(CC) $(CFLAGS) -c $<

# this line makes the Makefile a dependency,
# so that when you switch debuggin modes you
# dont have to manually touch symtable.h
dynarray.h: Makefile
	touch dynarray.h
