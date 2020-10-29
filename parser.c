#include "shell.h"

int initTokensTable(char *line);
int tokensIsEmpty();
token *tokensGetNextElement();
token *tokensCheckNextElement();
void tokensSkipElement();

static job *parseJob();
static process *parseProcess(char **infile, char **outfile, char **appfile);



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

    job *currentJob = first_job = 0;
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
            if (currentJob)
            {
                currentJob->next = parseJob();
                currentJob = currentJob->next;
            }
            else
            {
                /* ��� ������ ������� � ������ */
                first_job = currentJob = parseJob();
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
    char *infile = 0, *outfile = 0, *appfile = 0;
    job *j = (job *)malloc(sizeof(job));
    j->next = 0;
    j->appfile = j->infile = j->outfile = 0;
    j->foreground = 1;
    j->pgid = 0;
    
    process *currentProcess = j->first_process = 0;

    token *t = tokensCheckNextElement();
    /* ������� ��������� ������� - ������ ;& ��� ���������� ������� */
    while (!tokensIsEmpty() && t->type != AMPERSAND && t->type != SEMICOLON)
    {
        switch (t->type)
        {
        case PIPE:
            if (currentProcess)
            {
                /* ��������� �� �����, ������� ����� ��� ��� */
                tokensSkipElement();
                /* ������ �� ����� ����������� ������ ���� ������ ������� */
                if (!(!tokensIsEmpty() && t->type != AMPERSAND && t->type != SEMICOLON))
                {
                    fprintf(stderr, "syntax error\n");
                    return j;
                }
                currentProcess->next = parseProcess(0, &outfile, &appfile);
                currentProcess = currentProcess->next;
            }
            else
            {
                fprintf(stderr, "syntax error\n");
                return j;
            }
            break;
        default:
            /* ��� ������ ������� */
            currentProcess = j->first_process = parseProcess(&infile, &outfile, &appfile);
            break;
        }
        t = tokensCheckNextElement();
    }
    if (infile)
        j->infile = infile;
    if (outfile)
        j->outfile = outfile;
    if (appfile)
        j->appfile = appfile;
    return j;
}

/**
 * ��������� ����: "arg0 arg1 ... argn [<,>,>>]"
 * ���� infile != 0, ���� ����� �������� <
 * ���������� ��� outfile, appfile.
 * ���� �������� 0, �� ����������� ��������������� ��� ���� - syntax error
*/
static process *parseProcess(char **infile, char **outfile, char **appfile)
{
    process *p = (process *)malloc(sizeof(process));
    p->nargs = 0;
    p->next = 0;
    p->argv[0] = (char *)NULL;

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
