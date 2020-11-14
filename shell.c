#include "shell.h"
#include "jobcontrol.h"
job *first_job = NULL;

pid_t shell_pgid;
void jobs_info(job *j);
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

    char tmp_line[MAXLINELEN];
    sprintf(prompt, "<> ");

    while (promptline(prompt, tmp_line, sizeof(tmp_line)) > 0)
    {
        update_status();
        strncpy(line, tmp_line, sizeof(line));
        if ((njobs = parseline(line)) <= 0){
            do_job_notification();
            continue;
        }

        /* запускаем задания */
        job *j;
        for (j = first_job; j; j = j->next)
        {
            if (!j->launched)
                launch_job(j);
        }
#ifdef DEBUG
        {
            job *j;
            for (j = first_job; j; j = j->next)
            {
                printf("new job{\n");
                printf("\tjob->pgid: %d\n", j->pgid);
                printf("\tstdin: %d, stdout: %d, stderr: %d\n", j->stdin, j->stdout, j->stderr);
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
        do_job_notification();
    }
    return 0;
}
