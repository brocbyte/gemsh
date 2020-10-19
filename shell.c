#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include "shell.h"
#include <stdlib.h>
#include <fcntl.h>

struct command cmds[MAXCMDS];
#define DEBUG
int main(int argc, char *argv[])
{
    register int i;
    char line[1024]; /*  allow large command lines  */
    int ncmds;
    char prompt[50]; // строка-приглашение на ввод

    /* PLACE SIGNAL CODE HERE */

    sprintf(prompt, "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) // читаем sizeof(line) байт
    {
        /* until eof  */
        if ((ncmds = parseline(line)) <= 0)
            continue; /* read next line */

#ifdef DEBUG
        {
            int i, j;
            for (i = 0; i < ncmds; i++)
            {
                fprintf(stderr, "Debug info : {\n");
                for (j = 0; cmds[i].cmdargs[j] != (char *)NULL; j++)
                    fprintf(stderr, "\tcmd[%d].cmdargs[%d] = %s\n", i, j, cmds[i].cmdargs[j]);
                fprintf(stderr, "\tcmds[%d].cmdflag = %o\n", i, cmds[i].cmdflag);
                if (cmds[i].infile)
                    fprintf(stderr, "\tcmds[%d].infile = %s\n", i, cmds[i].infile);
                if (cmds[i].outfile)
                    fprintf(stderr, "\tcmds[%d].outfile = %s\n", i, cmds[i].outfile);
                if (cmds[i].appfile)
                    fprintf(stderr, "\tcmds[%d].appfile = %s\n", i, cmds[i].appfile);
                fprintf(stderr, "\t%s\n", (cmds[i].bkgrnd == 1 ? "background" : "foreground"));
                fprintf(stderr, "}\n");
            }
        }
#endif

        for (i = 0; i < ncmds; i++)
        {
            pid_t pid;
            int status;
            switch (pid = fork())
            {
            case -1: // ошибка вызова fork
                perror(prompt);
                exit(1);
            case 0:  // находимся в child process
                if (cmds[i].infile != (char *)NULL)
                {
                    int newIn;
                    if ((newIn = open(cmds[i].infile, O_RDONLY)) == -1)
                    {
                        perror(cmds[i].infile);
                        exit(1);
                    }
                    if (dup2(newIn, 0) == -1)
                        exit(1);
                    close(newIn);
                }
                if (cmds[i].outfile != (char *)NULL)
                {
                    int newOut;
                    if ((newOut = open(cmds[i].outfile, O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1)
                    {
                        perror(cmds[i].outfile);
                        exit(1);
                    }
                    if (dup2(newOut, 1) == -1)
                        exit(1);
                    close(newOut);
                }
                if (cmds[i].appfile != (char *)NULL)
                {
                    int newOut;
                    if ((newOut = open(cmds[i].appfile, O_CREAT | O_APPEND | O_WRONLY, 0644)) == -1)
                    {
                        perror(cmds[i].appfile);
                        exit(1);
                    }
                    if (dup2(newOut, 1) == -1)
                        exit(1);
                    close(newOut);
                }
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                perror(cmds[i].cmdargs[0]);
                exit(1); // ошибка вызова exec
            }
            // находимся в parent process
            if (!cmds[i].bkgrnd)
            {
                if (waitpid(pid, &status, 0) == -1)
                {
                    perror("Ошибка вызова wait");
                    exit(1);
                }
            }
            else
            {
                printf("bkgrnd pid: %d\n", pid);
            }
            int r = waitpid(-1, (int *)0, WNOHANG);
            while (r > 0)
                r = waitpid(-1, (int *)0, WNOHANG);
        }
    } /* close while */
}

/* PLACE SIGNAL CODE HERE */
