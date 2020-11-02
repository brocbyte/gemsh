shell: shell.o promptline.o parser.o launchers.o tokens.o
		cc -o shell shell.o promptline.o parser.o launchers.o tokens.o
shell.o: shell.c shell.h
		cc -c -o shell.o shell.c
promptline.o: promptline.c shell.h
		cc -c -o promptline.o promptline.c
parser.o: parser.c shell.h
		cc -c -o parser.o parser.c
launchers.o: launchers.c shell.h
		cc -c -o launchers.o launchers.c
tokens.o: tokens.c shell.h
		cc -c -o tokens.o tokens.c
clean:
	rm -rf *.o shell
