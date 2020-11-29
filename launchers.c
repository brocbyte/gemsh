#include "shell.h"
#include "jobcontrol.h"
void jobs_info();
void fg_builtin(process *p);
void bg_builtin(process *p);
void cd_builtin(process *p);
// clear ; ls | grep shell

void launch_process(process *p, pid_t pgid, int infile, int outfile, int foreground)
{
    /* ������������� �������� ������
     * ���� ���������� pgid == 0, ������ �� ��������� ������ ������� � ������� => ��� ���� pgid ����� ��� pid'� 
     * ���������� ��� ������, ��� ������ ������� � ������� ����� ������� ������ ��������� */
    pid_t pid;
    pid = getpid();
    p->pid = pid;
    if (pgid == 0)
        pgid = pid;

    /* �������� ������� ������ �� ������� (��� �� ������ � � �����, ����� �������� �����) */
    setpgid(pid, pgid);

    if (foreground)
    {
        /* ������� ������� �� �������� ���� */
        if (tcsetpgrp(STDIN_FILENO, pgid) == -1)
            perror("tsetpgrp err");
    }

    /* ��������������� ����������� ������� �� ������� (�.�. �� ������������� �� �����) */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    /* ��������������� */
    if (infile != STDIN_FILENO)
    {
        if (dup2(infile, STDIN_FILENO) == -1)
        {
            perror("Dup infile");
        }
        if (close(infile) == -1)
        {
            perror("Close infile");
        }
    }
    if (outfile != STDOUT_FILENO)
    {
        if (dup2(outfile, STDOUT_FILENO) == -1)
        {
            perror("Dup outfile");
        }
        if (close(outfile) == -1)
        {
            perror("Close outfile");
        }
    }
    if (p->stderrno != STDERR_FILENO)
    {
        if (dup2(p->stderrno, STDERR_FILENO) == -1)
        {
            perror("Dup errfile");
        }
        if (close(p->stderrno) == -1)
        {
            perror("Close errfile");
        }
    }
    /* �������, ����� exec �� ���������� p->argv */
    execvp(p->argv[0], p->argv);
    perror(p->argv[0]);
    /* exec error */
    exit(1);
}

void launch_job(job *j)
{
    int pipesUsed = 0;
    process *p;
    pid_t pid;
    int mypipe[2], infile, outfile;
    //char *infile, *outfile, *appfile;
    /* ��������� ���� ��� ������ ������� � ����� */
    infile = j->stdinno;
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
        if (strcmp(p->argv[0], "cd") == 0)
        {
            cd_builtin(p);
            p->completed = 1;
            return;
        }
        /* ��������� ����� ��� ��������� ������� � ����� */
        if (p->next)
        {
            /* ���������������! */
            if (pipe(mypipe) < 0)
            {
                perror("pipe");
                exit(1);
            }
            pipesUsed = 1;
            outfile = mypipe[1];
        }
        else
        {
            outfile = j->stdoutno;
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
            if (infile != mypipe[0] && pipesUsed)
            {
                if (close(mypipe[0]) == -1)
                {
                    perror("infile");
                }
            }
            if (outfile != mypipe[1] && outfile != j->stdoutno && pipesUsed)
            {
                /* ������ �� �� ���� �����-������?.. */
                if (close(mypipe[1]) == -1)
                {
                    perror("outfile");
                }
            }
            launch_process(p, j->pgid, infile, outfile, j->foreground);
        }
        else
        {
            /* parent process code */
            p->pid = pid;
            /* pgid ��� ������� ����� pid ������� �������� � ��������� */
            if (!j->pgid)
                j->pgid = pid;
            /* ��������� ������� ������ �� ����� */
            /* If both the child processes and the shell call setpgid, this ensures that the right things happen no matter which process gets to it first. */
            setpgid(pid, j->pgid);
        }
        /* Clean up after pipes.  */
        if (infile != j->stdinno)
            close(infile);
        if (outfile != j->stdoutno)
            close(outfile);
        if (p->stderrno != STDERR_FILENO)
            close(p->stderrno);
        infile = mypipe[0];
    }
    /* ������� ���� ��������� �������� ������ ����� */
    if (j->stdinno != STDIN_FILENO)
        close(j->stdinno);
    if (j->stdoutno != STDOUT_FILENO)
        close(j->stdoutno);

    j->launched = 1;
    if (j->foreground)
    {
        put_job_in_foreground(j, 0);
    }
    else
    {
        printf("[%d]\n", j->pgid);
        put_job_in_background(j, 0);
    }
}
