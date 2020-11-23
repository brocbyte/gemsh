#include "shell.h"
int job_is_completed(job *j);
int job_is_stopped(job *j);
void put_job_in_foreground(job *j, int cont);
void put_job_in_background(job *j, int cont);
void wait_for_job(job *j);
void update_status();

void mark_job_as_running(job *j)
{
    process *p;

    for (p = j->first_process; p; p = p->next)
        p->stopped = 0;
    j->notified = 0;
}

void continue_job(job *j, int foreground)
{
    int stopped = job_is_stopped(j);
    mark_job_as_running(j);
    if (foreground)
        put_job_in_foreground(j, stopped);
    else
        put_job_in_background(j, stopped);
}

void jobs_info()
{
    job *j;
    for (j = first_job; j; j = j->next)
    {
        if (j->builtin || !j->launched)
            continue;
        printf("# [%d]: ", j->pgid);
        if (job_is_stopped(j) && !job_is_completed(j))
        {
            printf("stopped\n");
            j->notified = 1;
        }
        else if (!job_is_completed(j))
        {
            printf("active\n");
        }
        else
        {
            printf("completed\n");
            j->notified = 1;
        }
    }
}
/* flag == 0: find last stopped background process 
   flag == 1: find last backgrounded process */
job *find_job(process *p, int flag)
{
    if (p->argv[1] == 0)
    {
        /* w/o argument: find the most recent backgrounded job */
        job *j, *jRecorded = 0;
        for (j = first_job; j; j = j->next)
        {
            if (j->builtin || !j->launched)
                continue;
            if (flag == 0)
            {
                if (!job_is_completed(j) && job_is_stopped(j))
                {
                    jRecorded = j;
                }
            }
            else
            {
                if (!job_is_completed(j))
                {
                    jRecorded = j;
                }
            }
        }
        return jRecorded;
    }
    else
    {
        job *j, *jRecorded = 0;
        for (j = first_job; j; j = j->next)
        {
            if (j->builtin || !j->launched)
                continue;
            char pgid[20];
            sprintf(pgid, "%d", j->pgid);
            if (strcmp(pgid, p->argv[1]) == 0)
            {
                if (flag == 0)
                {
                    if (!job_is_completed(j) && job_is_stopped(j))
                    {
                        jRecorded = j;
                    }
                }
                else
                {
                    if (!job_is_completed(j))
                    {
                        jRecorded = j;
                    }
                }
            }
        }
        return jRecorded;
    }
}
void fg_builtin(process *p)
{
    job *j = find_job(p, 1);
    if (j)
    {
        fprintf(stderr, "[%d]: continued in foreground\n", j->pgid);
        continue_job(j, 1);
    }
    else
    {
        fprintf(stderr, "no such job\n");
    }
}
void bg_builtin(process *p)
{
    job *j = find_job(p, 0);
    if (j)
    {
        fprintf(stderr, "[%d]: continued in background\n", j->pgid);
        continue_job(j, 0);
    }
    else
    {
        fprintf(stderr, "no such job\n");
    }
}

void cd_builtin(process *p)
{
    if(p->argv[1] == 0){
        fprintf(stderr, "cd w/o args is not allowed :(\n");
        return;
    }
    if (chdir(p->argv[1]) == -1)
    {
        perror("chdir");
    }
}
