CC=gcc -g -Wall

all: altex

altex: altex.c altex-utilitaire.o
	$(CC) -o altex altex-utilitaire.o `pkg-config --cflags cairo` altex.c `pkg-config --libs cairo freetype2` -lm 

altex-utilitaire.o: altex-utilitaire.c altex-utilitaire.h
	$(CC) `pkg-config --cflags cairo` -c altex-utilitaire.c `pkg-config --libs cairo freetype2`

clean:
	rm altex-utilitaire.o altex

