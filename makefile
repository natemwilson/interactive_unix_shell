all: ishlex ishlexd ishlexm

ishlex: lex.c ishlex.c dynarray.c
	gcc217 lex.c ishlex.c dynarray.c -o ishlex
ishlexm: lex.c ishlex.c dynarray.c
	gcc217m lex.c ishlex.c dynarray.c -o ishlexm
ishlexd: lex.c ishlex.c dynarray.c
	gcc217 -g -D NDEBUG lex.c ishlex.c dynarray.c -o ishlexd

ishsyn:
	gcc217 
