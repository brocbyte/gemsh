#include "shell.h"
#include "jobcontrol.h"
void jobs_info();
void fg_builtin(process *p);
void bg_builtin(process *p);

void launch_process(process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground)
{
    /* устанавливаем процессу группу
     * если переданное pgid == 0, значит мы запускаем первый процесс в задании => для него pgid равен его pid'у 
     * фактически это значит, что первый процесс в задании будет лидером группы процессов */
    pid_t pid;
    pid = getpid();
    p->pid = pid;
    if (pgid == 0)
        pgid = pid;

    /* назначем потомку группу из потомка (так же делаем и в шелле, чтобы избежать гонок) */
    setpgid(pid, pgid);

    /* восстанавливаем стандартные реакции на сигналы (т.к. мы наследовались от шелла) */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    if (foreground)
    {
        /* выводим процесс на передний план */
        tcsetpgrp(STDIN_FILENO, pgid);
    }
    /* else
    {
        если фоновый, надо предохранить от SIGINT,SIGQUIT 
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
    } */

    /* int infileno, outfileno;
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
    } */

    /* открываем файлы для перенаправления */
    if (infile != STDIN_FILENO)
    {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if (outfile != STDOUT_FILENO)
    {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    if (errfile != STDERR_FILENO)
    {
        dup2(errfile, STDERR_FILENO);
        close(errfile);
    }

    /* наконец, зовем exec по аргументам p->argv */
    execvp(p->argv[0], p->argv);
    perror(p->argv[0]);
    /* exec error */
    exit(1);
}

void launch_job(job *j)
{
    process *p;
    pid_t pid;
    int mypipe[2], infile, outfile;
    //char *infile, *outfile, *appfile;
    /* назначить вход для первой команды в пайпе */
    infile = j->stdin;
    for (p = j->first_process; p; p = p->next)
    {
        if (strcmp(p->argv[0], "jobs") == 0)
        {
            jobs_info();
            p->completed = 1;
            return;
        }
        if (strcmp(p->argv[0], "fg") == 0)
        {
            fg_builtin(p);
            p->completed = 1;
            return;
        }
        if (strcmp(p->argv[0], "bg") == 0)
        {
            bg_builtin(p);
            p->completed = 1;
            return;
        }
        /* назначить выход для последней команды в пайпе */
        if (p->next)
        {
            if (pipe(mypipe) < 0)
            {
                perror("pipe");
                exit(1);
            }
            outfile = mypipe[1];
        }
        else
        {
            outfile = j->stdout;
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
            launch_process(p, j->pgid, infile, outfile, j->stderr, j->foreground);
        }
        else
        {
            /* parent process code */
            p->pid = pid;
            /* pgid для задания равен pid первого процесса в конвейере */
            if (!j->pgid)
                j->pgid = pid;
            /* назначаем потомку группу из шелла */
            /* If both the child processes and the shell call setpgid, this ensures that the right things happen no matter which process gets to it first. */
            setpgid(pid, j->pgid);
        }
        /* Clean up after pipes.  */
        if (infile != j->stdin)
            close(infile);
        if (outfile != j->stdout)
            close(outfile);
        infile = mypipe[0];
    }
    /* кажется надо почистить открытые шеллом файлы */
    if (j->stdin != STDIN_FILENO)
        close(j->stdin);
    if (j->stdout != STDOUT_FILENO)
        close(j->stdout);
    if (j->stderr != STDERR_FILENO)
        close(j->stderr);

    j->launched = 1;
    if (j->foreground)
    {
        put_job_in_foreground(j, 0);
    }
    else
    {
        printf("launched: %d\n", j->pgid);
        put_job_in_background(j, 0);
    }
    /*В этой реализации мы ждем все процессы. 
    * Поэтому к этому моменту мы уверены, что все процессы завершены.
    * По-хорошему надо будет смотреть, какие задания завершены, чистить за ними структуры процессов + структуры заданий.
    */
}
