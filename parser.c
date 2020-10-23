#include "shell.h"
int tokensNum;
token tokensTable[MAXARGS];
job *first_job = NULL;

static char *peakToken(char **pos);
static char *readToken(char **pos);
static job *parseJob(int *);
static process *parseProcess(int *, char **infile, char **outfile, char **appfile);
static char *blankskip(char *s);
int isControlSymbol(char c);

/**
 * returns num of tokens in line
*/
int initTokensTable(char *line)
{
    char *s = line;
    int tokens = 0;
    s = blankskip(s);
    while (*s)
    {
        switch (*s)
        {
        case ';':
            tokensTable[tokens++].type = SEMICOLON;
            *s++ = 0;
            break;
        case '&':
            tokensTable[tokens++].type = AMPERSAND;
            *s++ = 0;
            break;
        case '>':
            if (*(s + 1) == '>')
            {
                tokensTable[tokens++].type = REALLYRIGHTARROW;
                *s = *(s + 1) = 0;
                s += 2;
            }
            else
            {
                tokensTable[tokens++].type = RIGHTARROW;
                *s++ = 0;
            }
            break;
        case '<':
            tokensTable[tokens++].type = LEFTARROW;
            *s++ = 0;
            break;
        case '|':
            tokensTable[tokens++].type = PIPE;
            *s++ = 0;
            break;
        default:
            /* это случай WORD */
            tokensTable[tokens].place = s;
            tokensTable[tokens++].type = WORD;
            while (!isspace(*s) && !isControlSymbol(*s) && *s)
                s++;
            if (isspace(*s))
                *s++ = 0;
        }
        s = blankskip(s);
    }
    return tokens;
}



int isControlSymbol(char c)
{
    return (c == '|' || c == ';' || c == '&' || c == '>' || c == '<');
}

/**
 * хэндлит '&', ';', т.к. это основной элемент для разделения заданий
 * договоренность: каждая функция должна "прочитывать" только те токены, которые она хэндлит
*/
int parseline(char *line)
{
    tokensNum = initTokensTable(line);
    int tokenIdx = 0;

    first_job = parseJob(&tokenIdx);
    job *currentJob = first_job;

    while (tokenIdx < tokensNum && (tokensTable[tokenIdx].type == AMPERSAND || tokensTable[tokenIdx].type == SEMICOLON))
    {
        if (tokensTable[tokenIdx++].type == AMPERSAND)
        {
            currentJob->foreground = 0;
        }
        else
        {
            currentJob->foreground = 1;
        }
        currentJob->next = parseJob(&tokenIdx);
        currentJob = currentJob->next;
    }
    return tokensNum;
}

/**
 * ожидаемый вход: "proc1 | proc2 | ... | procn"
 * хэндлит '|', т.к. это основной строительный эл-т для заданий
 * обрабатывает <> для первого и последнего процесса в трубе??
*/
static job *parseJob(int *tokenIdx)
{
    char *infile = 0, *outfile = 0, *appfile = 0;
    job *j = (job *)malloc(sizeof(job));
    j->next = 0;
    j->first_process = parseProcess(tokenIdx, &infile, &outfile, &appfile);
    j->in = STDIN_FILENO;
    j->out = STDOUT_FILENO;
    process *currentProcess = j->first_process;

    while (tokensTable[*tokenIdx].type == PIPE)
    {
        outfile = appfile = 0;
        (*tokenIdx)++;
        currentProcess->next = parseProcess(tokenIdx, 0, &outfile, &appfile);
        /**
         * На самом деле в шелле можно так делать. 
         * Если "ls | sort < 1.txt", то вход команды sort будет состоять из объединения ls и 1.txt/
        */
        if (tokensTable[(*tokenIdx) + 1].type == PIPE)
        {
            /* получается что процесс не последний в пайпе, т.к. следующий токен - пайп */
            if (appfile || outfile)
            {
                fprintf(stderr, "Redirection stdout is allowed only for the last command in pipe\n");
            }
        }
        currentProcess = currentProcess->next;
    }
    /* открываем файлы для перенаправления */
    if (infile)
    {
        if ((j->in = open(infile, O_RDONLY)) == -1)
        {
            perror(infile);
            exit(1);
        }
    }
    if (outfile)
    {

        if ((j->out = open(outfile, O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1)
        {
            perror(outfile);
            exit(1);
        }
    }
    if (appfile)
    {
        if ((j->out = open(appfile, O_CREAT | O_APPEND | O_WRONLY, 0644)) == -1)
        {
            perror(appfile);
            exit(1);
        }
    }
    return j;
}

/**
 * ожидаемый вход: "arg0 arg1 ... argn [<,>,>>]"
 * если infile != 0, туда нужно положить <
 * аналогично для outfile, appfile.
 * если аргумент 0, но встречается перенправление его типа - syntax error
*/
static process *parseProcess(int *tokenIdx, char **infile, char **outfile, char **appfile)
{
    process *p = (process *)malloc(sizeof(process));
    p->nargs = 0;
    p->next = 0;
    p->argv[0] = (char *)NULL;

    p->argv[p->nargs++] = tokensTable[*tokenIdx].place;
    p->argv[p->nargs] = (char *)NULL;

    (*tokenIdx)++;
    tokenType t = tokensTable[*tokenIdx].type;
    while (t != SEMICOLON && t != AMPERSAND && t != PIPE && *tokenIdx < tokensNum)
    {
        switch (t)
        {
        case REALLYRIGHTARROW:
            /* appfile */
            (*tokenIdx)++;
            *appfile = tokensTable[*tokenIdx].place;
            break;
        case RIGHTARROW:
            /* outfile */
            (*tokenIdx)++;
            *outfile = tokensTable[*tokenIdx].place;
            break;
        case LEFTARROW:
            /* infile */
            (*tokenIdx)++;
            *infile = tokensTable[*tokenIdx].place;
            break;
        default:
            p->argv[p->nargs++] = tokensTable[*tokenIdx].place;
            p->argv[p->nargs] = (char *)NULL;
            break;
        }
        (*tokenIdx)++;
        t = tokensTable[*tokenIdx].type;
    }
    return p;
}

/* пропускает все значащие [\t, \n, _] символы */
static char *blankskip(char *s)
{
    while (isspace(*s) && *s)
        ++s;
    return (s);
}
