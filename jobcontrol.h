#ifndef JOB_CONTROL_HEAD
#define JOB_CONTROL_HEAD
int mark_process_status(pid_t pid, int status);
void update_status(void);
void wait_for_job(job *j);
void do_job_notification(void);
int job_is_completed(job *j);
int job_is_stopped(job *j);
void reportJob(job * j, char const * s);
void free_job(job * j);
void put_job_in_foreground(job *j, int cont);
void put_job_in_background(job *j, int cont);
#endif
