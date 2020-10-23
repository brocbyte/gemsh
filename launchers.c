#include "shell.h"

void launch_process(process *p, pid_t pgid, int infile, int outfile, int foreground)
{
    pid_t pid;

    if (infile != STDIN_FILENO)
    {
        if (dup2(infile, 0) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(infile);
    }

    if (outfile != STDOUT_FILENO)
    {
        if (dup2(outfile, 1) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(outfile);
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
    int infile, outfile;
    /* ��������� ���� ��� ������ ������� � ����� */
    infile = j->in;
    for (p = j->first_process; p; p = p->next)
    {
        /* ��������� ����� ��� ��������� ������� � ����� */
        if (!p->next)
        {
            outfile = j->out;
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
            launch_process(p, j->pgid, infile, outfile, j->foreground);
        }
        else
        {
            /* parent process */
            p->pid = pid;
            int status;
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("Wait error!");
                exit(1);
            }
        }
        /* ����� ����� ���� */
        infile = 0;
    }
    /*� ���� ���������� �� ���� ��� ��������. 
    * ������� � ����� ������� �� �������, ��� ��� �������� ���������.
    * ��-�������� ���� ����� ��������, ����� ������� ���������, ������� �� ���� ��������� ��������� + ��������� �������,
    * ����������� ����� ������.
    */
    if (j->in != STDIN_FILENO)
        close(j->in);
    if (j->out != STDOUT_FILENO)
        close(j->out);
}