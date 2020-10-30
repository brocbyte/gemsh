#include "shell.h"

void putJobInForeground(job *j, int cont);
void putJobInBackground(job *j, int cont);

void launch_process(process *p, pid_t pgid, char *infile, char *outfile, char *appfile, int foreground)
{
    /* ������������� �������� ������
     * ���� ���������� pgid == 0, ������ �� ��������� ������ ������� � ������� => ��� ���� pgid ����� ��� pid'� 
     * ���������� ��� ������, ��� ������ ������� � ������� ����� ������� ������ ��������� */
    pid_t pid;
    pid = getpid();
    if (pgid == 0)
        pgid = pid;

    /* �������� ������� ������ �� ������� (��� �� ������ � � �����, ����� �������� �����) */
    setpgid(pid, pgid);

    /* ��������������� ����������� ������� �� ������� (�.�. �� ������������� �� �����) */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    if (foreground)
    {
        /* ������� ������� �� �������� ���� */
        tcsetpgrp(STDIN_FILENO, pgid);
    }
    else
    {
        /* ���� �������, ���� ������������ �� SIGINT,SIGQUIT */
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
    }

    /* ��������� ����� ��� ��������������� */
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

    /* �������, ����� exec �� ���������� p->argv */
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
    /* ��������� ���� ��� ������ ������� � ����� */
    infile = j->infile;
    for (p = j->first_process; p; p = p->next)
    {
        /* ��������� ����� ��� ��������� ������� � ����� */
        if (!p->next)
        {
            outfile = j->outfile;
            appfile = j->appfile;
        }
        else
        {
            /* ����� ����� ���� */
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
            /* pgid ��� ������� ����� ��� ������� �������� � ��������� */
            if (!j->pgid)
                j->pgid = pid;
            /* ��������� ������� ������ �� ����� */
            /* If both the child processes and the shell call setpgid, this ensures that the right things happen no matter which process gets to it first. */
            setpgid(pid, j->pgid);
        }
        /* ����� ����� ���� */
        infile = 0;
    }
    if (j->foreground)
    {
        putJobInForeground(j, 0);
    }
    else
    {
        printf("launched: %d\n", j->pgid);
        putJobInBackground(j, 0);
    }
    /*� ���� ���������� �� ���� ��� ��������. 
    * ������� � ����� ������� �� �������, ��� ��� �������� ���������.
    * ��-�������� ���� ����� ��������, ����� ������� ���������, ������� �� ���� ��������� ��������� + ��������� �������.
    */
}

void putJobInForeground(job *j, int cont)
{
    /* cont - ��� job control */
    tcsetpgrp(STDIN_FILENO, j->pgid);
    if (waitpid(j->first_process->pid, 0, 0) == -1)
    {
        perror("Wait error!");
        exit(1);
    }
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}
void putJobInBackground(job *j, int cont)
{
    /* cont - ��� job control */
    return;
}
