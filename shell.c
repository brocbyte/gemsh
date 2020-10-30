#include "shell.h"

job *first_job = NULL;

pid_t shell_pgid;

int main()
{
    shell_pgid = getpgrp();
    /* командный интерпретатор должен игнорировать сигналы, связанные с job control */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    char line[MAXLINELEN]; /*  allow large command lines  */
    int njobs;
    char prompt[50]; // строка-приглашение на ввод

    sprintf(prompt, "<> ");

    while (promptline(prompt, line, sizeof(line)) > 0)
    {
        //tcsetpgrp (0, shell_pgid);
        if ((njobs = parseline(line)) <= 0)
            continue;

        /* запускаем задания */
        job *j;
        for (j = first_job; j; j = j->next)
        {
            launch_job(j);
        }
#ifdef DEBUG
        {
            printf("shell_pgid: %d\n", shell_pgid);
            job *j;
            for (j = first_job; j; j = j->next)
            {
                printf("new job{\n");
                printf("\tjob->pgid: %d\n", j->pgid);
                printf("\tinfile: %s, outfile: %s, appfile: %s\n", j->infile, j->outfile, j->appfile);
                process *p;
                for (p = j->first_process; p; p = p->next)
                {
                    printf("\tnew process{\n");
                    printf("\t\tprocess pid: %d\n", p->pid);
                    for (int i = 0; i < p->nargs; i++)
                        printf("\t\t\"%s\"\n", p->argv[i]);
                    printf("\t}\n");
                }
                printf("\n}\n");
            }
        }
#endif
        /* тупое подбирание завершившихся процессов. По-нормальному, надо делать job control и внутри него проверять, закончились ли задания */
        int r = waitpid(-1, (int *)0, WNOHANG);
        while (r > 0)
        {
            r = waitpid(-1, (int *)0, WNOHANG);
        }
    }
    return 0;
}
