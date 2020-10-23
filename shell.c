#include "shell.h"

int main()
{
    int i;
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
                printf("\tstdin: %d, stdout: %d\n", j->in, j->out);
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
        for (j = first_job; j; j = j->next){
            launch_job(j);
        }
    }
    return 0;
}