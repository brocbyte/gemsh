#include "shell.h"

void launch_process(process *p, pid_t pgid, char *infile, char *outfile, char *appfile, int foreground)
{
    pid_t pid;
    pid = getpid();
    /* если переданное pgid == 0, значит мы запускаем первый процесс в задании => для него pgid равен его pid'у */
    if (pgid == 0)
        pgid = pid;
    /* назначем потомку группу из потомка */
    setpgid(pid, pgid);
    
    /* открываем файлы для перенаправления */
    int infileno, outfileno;

    if (infile)
    {
        if ((infileno = open(infile, O_RDONLY)) == -1)
        {
            perror(infile);
            exit(1);
        }
        if (dup2(infileno, STDIN_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(infileno);
    }

    if (outfile)
    {
        if ((outfileno = open(outfile, O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1)
        {
            perror(outfile);
            exit(1);
        }
        if (dup2(outfileno, STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(outfileno);
    }
    if (appfile)
    {
        if ((outfileno = open(appfile, O_CREAT | O_APPEND | O_WRONLY, 0644)) == -1)
        {
            perror(appfile);
            exit(1);
        }
        if (dup2(outfileno, STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(outfileno);
    }

    execvp(p->argv[0], p->argv);
    perror(p->argv[0]);
    /* exec error */
    exit(1);
}

void launch_job(job *j)
{
    process *p;
    pid_t pid;
    char *infile, *outfile, *appfile;
    /* назначить вход для первой команды в пайпе */
    infile = j->infile;
    for (p = j->first_process; p; p = p->next)
    {
        /* назначить выход для последней команды в пайпе */
        if (!p->next)
        {
            outfile = j->outfile;
            appfile = j->appfile;
        }
        else
        {
            /* здесь будет пайп */
        }

        pid = fork();
        if (pid == -1)
        {
            /* fork error */
            perror("fork");
            exit(1);
        }
        else if (pid == 0)
        {
            /* child process */
            launch_process(p, j->pgid, infile, outfile, appfile, j->foreground);
        }
        else
        {
            /* parent process code */
            p->pid = pid;
            /* pgid для задания равен как первого процесса в конвейере */
            if (!j->pgid)
                j->pgid = pid;
            /* назначаем потомку группу из шелла */
            /* If both the child processes and the shell call setpgid, this ensures that the right things happen no matter which process gets to it first. */
            setpgid(pid, j->pgid);

            int status;
            if (j->foreground)
            {
                if (waitpid(pid, &status, 0) == -1)
                {
                    perror("Wait error!");
                    exit(1);
                }
            }
            else
            {
                printf("%d\n", pid);
            }
        }
        /* здесь будет пайп */
        infile = 0;
    }
    /*В этой реализации мы ждем все процессы. 
    * Поэтому к этому моменту мы уверены, что все процессы завершены.
    * По-хорошему надо будет смотреть, какие задания завершены, чистить за ними структуры процессов + структуры заданий.
    */
}
