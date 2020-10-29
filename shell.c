#include "shell.h"

job *first_job = NULL;

int main()
{
    /* TODO проверить, нужные ли здесь сигналы */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    
    char line[MAXLINELEN]; /*  allow large command lines  */
    int njobs;
    char prompt[50]; // строка-приглашение на ввод

    sprintf(prompt, "<> ");

    while (promptline(prompt, line, sizeof(line)) > 0) // читаем sizeof(line) байт
    {
        if ((njobs = parseline(line)) <= 0)
            continue;

#ifdef DEBUG
        {
            job *j;
            for (j = first_job; j; j = j->next)
            {
                printf("new job{\n");
                printf("\tinfile: %s, outfile: %s, appfile: %s\n", j->infile, j->outfile, j->appfile);
                process *p;
                for (p = j->first_process; p; p = p->next)
                {
                    printf("\tnew process{\n");
                    for (int i = 0; i < p->nargs; i++)
                        printf("\t\t\"%s\"\n", p->argv[i]);
                    printf("\t}\n");
                }
                printf("\n}\n");
            }
        }
#endif

        /* запускаем задания */
        job *j;
        for (j = first_job; j; j = j->next)
        {
            launch_job(j);
        }
        /* тупое подбирание завершившихся процессов. По-нормальному, надо делать job control и внутри него проверять, закончились ли задания */
        int r = waitpid(-1, (int *)0, WNOHANG);
        while (r > 0)
        {
            r = waitpid(-1, (int *)0, WNOHANG);
        }
    }
    return 0;
}
