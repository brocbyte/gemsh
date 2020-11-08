shell: shell.o promptline.o parser.o launchers.o tokens.o jobcontrol.o builtins.o
		cc -o shell shell.o promptline.o parser.o launchers.o tokens.o jobcontrol.o builtins.o
shell.o: shell.c shell.h jobcontrol.h
		cc -c -o shell.o shell.c
promptline.o: promptline.c shell.h jobcontrol.h
		cc -c -o promptline.o promptline.c
parser.o: parser.c shell.h jobcontrol.h
		cc -c -o parser.o parser.c
launchers.o: launchers.c shell.h jobcontrol.h
		cc -c -o launchers.o launchers.c
tokens.o: tokens.c shell.h jobcontrol.h
		cc -c -o tokens.o tokens.c
jobcontrol.o: jobcontrol.c shell.h jobcontrol.h
		cc -c -o jobcontrol.o jobcontrol.c
builtin.o: builtins.c shell.h jobcontrol.h
		cc -c -o builtins.o builtins.c
clean:
	rm -rf *.o shell
