#include "shell.h"

void launch_process(process *p, pid_t pgid, char *infile, char *outfile, char *appfile, int foreground)
{
    pid_t pid;
    pid = getpid();
    /* ���� ���������� pgid == 0, ������ �� ��������� ������ ������� � ������� => ��� ���� pgid ����� ��� pid'� */
    if (pgid == 0)
        pgid = pid;
    /* �������� ������� ������ �� ������� */
    setpgid(pid, pgid);
    
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
        /* ����� ����� ���� */
        infile = 0;
    }
    /*� ���� ���������� �� ���� ��� ��������. 
    * ������� � ����� ������� �� �������, ��� ��� �������� ���������.
    * ��-�������� ���� ����� ��������, ����� ������� ���������, ������� �� ���� ��������� ��������� + ��������� �������.
    */
}
