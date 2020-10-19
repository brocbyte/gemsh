#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"
static char *blankskip(register char *);

/* 
    TODO = {
            перенаправление на нескольких файлах, 
            пайпы,
            globbing,
            стрелки,
            табы
            }
*/
int parseline(char *line)
{
    int nargs;
    int ncmds;
    register char *s;
    char aflg = 0;
    int rval;
    register int i;
    static char delim[] = " \t|&<>;\n";

    /* initialize  */

    nargs = ncmds = rval = 0;
    s = line;
    rval = 0;
    s = line;
    for (i = 0; i < MAXCMDS; i++){
        cmds[i].cmdflag = 0;
        cmds[i].infile = cmds[i].outfile = cmds[i].appfile = (char *)NULL;
        cmds[i].bkgrnd = 0;
        cmds[i].cmdargs[0] = (char *)NULL;
    }

    while (*s)
    {
        aflg = 0;
        s = blankskip(s);
        if (!*s)
            break;
        
        /*  handle <, >, |, &, and ;  */
        switch (*s)
        {
            case '&':
                /**
                 * Текущая команда должна выполняться в фоне.
                 * Конец текущей команды.
                */
                ++cmds[ncmds].bkgrnd;
                *s++ = '\0';
                ++ncmds;
                nargs = 0;
                break;
            case '>':
                /**
                 * Нормальный shell при множественных перенаправлениях складывает источники и копирует во все выходы
                */
                if (*(s + 1) == '>')
                {
                    ++aflg;
                    *s++ = '\0';
                }
                *s++ = '\0';
                s = blankskip(s);
                if (!*s)
                {
                    fprintf(stderr, "syntax error\n");
                    return (-1);
                }
                if (aflg)
                    cmds[ncmds].appfile = s;
                else
                    cmds[ncmds].outfile = s;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
            case '<':
                *s++ = '\0';
                s = blankskip(s);
                if (!*s)
                {
                    fprintf(stderr, "syntax error\n");
                    return (-1);
                }
                cmds[ncmds].infile = s;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
            case '|':
                if (nargs == 0)
                {
                    fprintf(stderr, "syntax error\n");
                    return (-1);
                }
                cmds[ncmds++].cmdflag |= OUTPIP;
                cmds[ncmds].cmdflag |= INPIP;
                *s++ = '\0';
                nargs = 0;
                break;
            case ';':
                *s++ = '\0';
                ++ncmds;
                nargs = 0;
                break;
            default:
                /*  a command argument  */
                if (nargs == 0) /* next command */
                    rval = ncmds + 1;
                cmds[ncmds].cmdargs[nargs++] = s;
                cmds[ncmds].cmdargs[nargs] = (char*)NULL;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
        } /*  close switch  */
    }     /* close while  */

    #ifdef DEBUG
    {
        printf("app file: %s\n", appfile);
        printf("out file: %s\n", outfile);
        printf("inp file: %s\n", infile);
    }
    #endif
    /*  error check  */

    /*
    *  The only errors that will be checked for are
    *  no command on the right side of a pipe
    *  no command to the left of a pipe is checked above
    */
    if (cmds[ncmds - 1].cmdflag & OUTPIP)
    {
        if (nargs == 0)
        {
            fprintf(stderr, "syntax error\n");
            return (-1);
        }
    }
    return (rval);
}

static char *blankskip(register char *s)
{
    while (isspace(*s) && *s)
        ++s;
    return (s);
}
