#include "shell.h"
#include "jobcontrol.h"
#include <errno.h>
int mark_process_status(pid_t pid, int status)
{
    job *j;
    process *p;
    //fprintf(stderr, "mark proc status %d\n", pid);
    if (pid > 0)
    {
        /* Update the record for the process.  */
        for (j = first_job; j; j = j->next)
            for (p = j->first_process; p; p = p->next)
                if (p->pid == pid)
                {
                    p->status = status;
                    if (WIFSTOPPED(status))
                    {
                        p->stopped = 1;
                        //fprintf(stderr, "looks like %d stopped\n", pid);
                    }
                    else
                    {
                        //fprintf(stderr, "looks like %d completed\n", pid);
                        p->completed = 1;
                        if (WIFSIGNALED(status))
                            fprintf(stderr, "\n[%d]: terminated by signal %d.\n", (int)pid, WTERMSIG(p->status));
                    }
                    return 0;
                }
        fprintf(stderr, "No child process %d.\n", pid);
        return -1;
    }

    else if (pid == 0 || errno == ECHILD)
        /* No processes ready to report.  */
        return -1;
    else
    {
        /* Other weird errors.  */
        perror("waitpid");
        return -1;
    }
}

void update_status()
{
    int status;
    pid_t pid;
    /* вытаскиваем менявшиеся статусы процессов */
    do
    {
        pid = waitpid((pid_t)-1, &status, WUNTRACED | WNOHANG);
    } while (!mark_process_status(pid, status));
}

/* Check for processes that have status information available,
   blocking until all processes in the given job have reported.  */

void wait_for_job(job *j)
{
    int status;
    pid_t pid;

    do
    {
        pid = waitpid(-j->pgid, &status, WUNTRACED);
    } while (!mark_process_status(pid, status) && !job_is_stopped(j) && !job_is_completed(j));
    //fprintf(stderr, "%d: p->stopped: %d, p->completed: %d\n", j->pgid, j->first_process->stopped, j->first_process->completed);
    if (job_is_stopped(j) && !job_is_completed(j))
    {
        fprintf(stderr, "\n[%d]: stopped\n", j->pgid);
        j->notified = 1;
    }
}

/* Notify the user about stopped or terminated jobs.
   Delete terminated jobs from the active job list.  */

void do_job_notification()
{
    job *j, *jlast, *jnext;

    /* Ищем процесс с обновленным статусом, регистрируем его */
    update_status();

    jlast = NULL;
    for (j = first_job; j; j = jnext)
    {
        jnext = j->next;

        /* If all processes have completed, tell the user the job has
         completed and delete it from the list of active jobs.  */
        if (job_is_completed(j))
        {
            if (!j->foreground)
                reportJob(j, "completed");
            if (jlast)
                jlast->next = jnext;
            else
                first_job = jnext;
            free_job(j);
        }

        /* Notify the user about stopped jobs,
         marking them so that we won't do this more than once.  */
        else if (job_is_stopped(j) && !j->notified)
        {
            //if (!j->foreground)
            reportJob(j, "stopped");
            j->notified = 1;
            jlast = j;
        }

        /* Don't say anything about jobs that are still running.  */
        else
            jlast = j;
    }
}

int job_is_completed(job *j)
{
    process *p;
    for (p = j->first_process; p; p = p->next)
    {
        if (!p->completed)
            return 0;
    }
    return 1;
}
int job_is_stopped(job *j)
{
    process *p;
    for (p = j->first_process; p; p = p->next)
    {
        if (!p->completed && !p->stopped)
            return 0;
    }
    return 1;
}

void reportJob(job *j, char const *s)
{
    /* TODO индексация заданий */
    fprintf(stderr, "[%d]: %s\n", j->pgid, s);
}

void free_job(job *j)
{
    process *p, *tmp;
    for (p = j->first_process; p; p = tmp)
    {
        tmp = p->next;
        free(p);
    }
    free(j);
}

void put_job_in_foreground(job *j, int cont)
{
    j->foreground = 1;
    /* cont - для job control */
    tcsetpgrp(STDIN_FILENO, j->pgid);
    /* Send the job a continue signal, if necessary.  */
    if (cont)
    {
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
    }
    wait_for_job(j);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}
void put_job_in_background(job *j, int cont)
{
    j->foreground = 0;
    if (cont)
    {
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
    }
}
