#include "shell.h"

int initTokensTable(char *line);
int tokensIsEmpty();
token *tokensGetNextElement();
token *tokensCheckNextElement();
void tokensSkipElement();

static job *parseJob();
static process *parseProcess(char **infile, char **outfile, char **appfile, char **errfile);

int isControlSymbol(char c)
{
    return (c == '|' || c == ';' || c == '&' || c == '>' || c == '<');
}

/**
 * ��������� ����: job1 [&;] job2 [&;] ...
 * ������� '&', ';', �.�. ��� �������� ������� ��� ���������� �������
 * ��������������: ������ ������� ������ "�����������" ������ �� ������, ������� ��� �������
*/
int parseline(char *line)
{
    int tokens = initTokensTable(line);
    if (tokens == -1)
        return 0;

    job *currentJob;
    for (currentJob = first_job; currentJob && currentJob->next; currentJob = currentJob->next)
        ;

    /* ������� ��������� ������ - ���������� ������� */
    while (!tokensIsEmpty())
    {
        token *t = tokensCheckNextElement();
        switch (t->type)
        {
        case AMPERSAND:
            if (currentJob)
            {
                currentJob->foreground = 0;
                tokensSkipElement();
            }
            else
            {
                /* �������� ������� ������� �������, ������� �� ����� �� ���������� */
                fprintf(stderr, "syntax error\n");
                return 0;
            }
            break;
        case SEMICOLON:
            tokensSkipElement();
            break;
        default:
            if (!currentJob)
            {
                if((first_job = currentJob = parseJob()) == 0){
                    return 0;
                }
            }
            else
            {
                if((currentJob->next = parseJob()) == 0){
                    return 0;
                }
                currentJob = currentJob->next;
            }
            break;
        }
    }

    return tokens;
}

/**
 * ��������� ����: "proc1 | proc2 | ... | procn"
 * ������� '|', �.�. ��� �������� ������������ ��-� ��� �������
 * ������������ <> ��� ������� � ���������� �������� � �����??
*/
static job *parseJob()
{
    char *infile = 0, *outfile = 0, *appfile = 0, *errfile = 0;
    job *j = (job *)malloc(sizeof(job));
    j->next = 0;
    j->stdinno = STDIN_FILENO;
    j->stdoutno = STDOUT_FILENO;
    j->stderrno = STDERR_FILENO;
    j->foreground = 1;
    j->pgid = 0;
    j->launched = 0;
    j->notified = 0;
    j->command = 0;
    j->builtin = 0;
    /* tmodes??? */
    process *currentProcess = j->first_process = 0;

    token *t = tokensCheckNextElement();
    /* ������� ��������� ������� - ������ ;& ��� ���������� ������� */
    while (!tokensIsEmpty() && t->type != AMPERSAND && t->type != SEMICOLON)
    {
        switch (t->type)
        {
        case PIPE:
            tokensSkipElement();
            if (currentProcess && !j->builtin)
            {
                /* ��������� �� �����, ������� ����� ��� ��� */
                /* ������ �� ����� ����������� ������ ���� ������ ������� */
                if (!(!tokensIsEmpty() && t->type != AMPERSAND && t->type != SEMICOLON))
                {
                    fprintf(stderr, "syntax error\n");
                    return 0;
                }
                // currentProcess->next = parseProcess(0, &outfile, &appfile);
                if((currentProcess->next = parseProcess(&infile, &outfile, &appfile, &errfile)) == 0) {
                    return 0;
                }
                currentProcess = currentProcess->next;
            }
            else
            {
                fprintf(stderr, "syntax error\n");
                return 0;
            }
            break;
        default:
            /* ��� ������ ������� */
            if((currentProcess = j->first_process = parseProcess(&infile, &outfile, &appfile, &errfile)) == 0) {
                return 0;
            }
            if (strcmp(j->first_process->argv[0], "jobs") == 0 ||
                strcmp(j->first_process->argv[0], "fg") == 0 ||
                strcmp(j->first_process->argv[0], "bg") == 0 ||
                strcmp(j->first_process->argv[0], "cd") == 0)
            {
                j->builtin = 1;
            }
            break;
        }
        t = tokensCheckNextElement();
    }
    /* ����� �� �������� ��������� �����. ������ ������� �� ����, �� ������� ����� ���� �� �������.
       �� ��������� �� ������������ ������� � ������� ����� ������. */
    if (infile)
    {
        if ((j->stdinno = open(infile, O_RDONLY | O_CLOEXEC)) == -1)
        {
            perror(infile);
            exit(1);
        }
    }
    if (outfile)
    {
        if ((j->stdoutno = open(outfile, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, 0644)) == -1)
        {
            perror(outfile);
            exit(1);
        }
    }
    else if (appfile)
    {
        if ((j->stdoutno = open(appfile, O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, 0644)) == -1)
        {
            perror(appfile);
            exit(1);
        }
    }
    if(errfile)
    {
        if ((j->stderrno = open(errfile, O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1)
        {
            perror(errfile);
            exit(1);
        }
    }
    return j;
}

/**
 * ��������� ����: "arg0 arg1 ... argn [<,>,>>]"
 * ���� infile != 0, ���� ����� �������� <
 * ���������� ��� outfile, appfile, errfile.
 * ���� �������� 0, �� ����������� ��������������� ��� ���� - syntax error
*/
static process *parseProcess(char **infile, char **outfile, char **appfile, char **errfile)
{
    process *p = (process *)malloc(sizeof(process));
    p->nargs = 0;
    p->next = 0;
    p->argv[0] = (char *)NULL;
    p->completed = 0;
    p->stopped = 0;
    token *t = tokensCheckNextElement();
    while (!tokensIsEmpty() && t->type != SEMICOLON && t->type != AMPERSAND && t->type != PIPE)
    {
        switch (t->type)
        {
        case REALLYRIGHTARROW:
            /* appfile */
            tokensSkipElement();
            t = tokensGetNextElement();
            if (!t || t->type != WORD)
            {
                fprintf(stderr, "after >> should be filename\n");
                return 0;
            }
            *appfile = t->place;
            break;
        case ERRORRIGHTARROW:
            /* errfile */
            tokensSkipElement();
            t = tokensGetNextElement();
            if (!t || t->type != WORD)
            {
                fprintf(stderr, "after 2> should be filename\n");
                return 0;
            }
            *errfile = t->place;
            break;
        case RIGHTARROW:
            /* outfile */
            tokensSkipElement();
            t = tokensGetNextElement();
            if (!t || t->type != WORD)
            {
                fprintf(stderr, "after > should be filename\n");
                return 0;
            }
            *outfile = t->place;
            break;
        case LEFTARROW:
            /* infile */
            tokensSkipElement();
            t = tokensGetNextElement();
            if (!t || t->type != WORD)
            {
                fprintf(stderr, "after < should be filename\n");
                return 0;
            }
            *infile = t->place;
            break;
        default:
            p->argv[p->nargs++] = t->place;
            p->argv[p->nargs] = (char *)NULL;
            tokensSkipElement();
            break;
        }
        t = tokensCheckNextElement();
    }
    return p;
}
