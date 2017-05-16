all: ishlex ishlexd ishlexm ishsyn ishsynm ishsynd ish ishd ishm

ishlex: lex.c ishlex.c dynarray.c
	gcc217 -D NDEBUG lex.c ishlex.c dynarray.c -o ishlex
ishlexm: lex.c ishlex.c dynarray.c
	gcc217m lex.c ishlex.c dynarray.c -o ishlexm
ishlexd: lex.c ishlex.c dynarray.c
	gcc217 -g lex.c ishlex.c dynarray.c -o ishlexd

ishsyn: command.c ishsyn.c dynarray.c lex.c
	gcc217 -D NDEBUG command.c ishsyn.c dynarray.c lex.c -o ishsyn
ishsynm: command.c ishsyn.c dynarray.c lex.c
	gcc217m command.c ishsyn.c dynarray.c lex.c -o ishsynm
ishsynd: command.c ishsyn.c dynarray.c lex.c
	gcc217 -g command.c ishsyn.c dynarray.c lex.c -o ishsynd 

ish: ish.c lex.c command.c dynarray.c
	gcc217 -D NDEBUG ish.c lex.c command.c dynarray.c -o ish

ishd: ish.c lex.c command.c dynarray.c
	gcc217 -g ish.c lex.c command.c dynarray.c -o ishd

ishm: ish.c lex.c command.c dynarray.c
	gcc217m ish.c lex.c command.c dynarray.c -o ishm
