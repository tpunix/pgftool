
all:
	gcc -Wall -O2 -o dump_pgf dump_pgf.c libpgf.c
	gcc -Wall -O2 -o  mix_pgf save_pgf.c libpgf.c mix_pgf.c
	gcc -Wall -O2 -o  ttf_pgf save_pgf.c libpgf.c ttfont.c -lfreetype -lz
clean:
	rm -f *.o *.exe
