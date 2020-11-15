#include "shell.h"
int isControlSymbol(char c);
static int tokensNum;
static token tokensTable[MAXARGS];
static int tokenIdx;
/* пропускает все значащие [\t, \n, _] символы */
static char *blankskip(char *s)
{
    while (isspace(*s) && *s){
        //*s = 0;
        ++s;
    }
    return (s);
}
/**
 * returns num of tokens in line
*/
int initTokensTable(char *line)
{
    char *s = line;
    tokensNum = 0;
    tokenIdx = 0;
    s = blankskip(s);
    while (*s)
    {
        switch (*s)
        {
        case ';':
            tokensTable[tokensNum++].type = SEMICOLON;
            *s++ = 0;
            break;
        case '&':
            tokensTable[tokensNum++].type = AMPERSAND;
            *s++ = 0;
            break;
        case '>':
            if (*(s + 1) == '>')
            {
                tokensTable[tokensNum++].type = REALLYRIGHTARROW;
                *s = *(s + 1) = 0;
                s += 2;
            }
            else
            {
                tokensTable[tokensNum++].type = RIGHTARROW;
                *s++ = 0;
            }
            break;
        case '<':
            tokensTable[tokensNum++].type = LEFTARROW;
            *s++ = 0;
            break;
        case '|':
            tokensTable[tokensNum++].type = PIPE;
            *s++ = 0;
            break;
        default:
            /* это случай WORD */
            tokensTable[tokensNum].place = s;
            tokensTable[tokensNum++].type = WORD;
            while (!isspace(*s) && !isControlSymbol(*s) && *s)
                s++;
            if (isspace(*s))
                *s++ = 0;
        }
        s = blankskip(s);
    }
    return tokensNum;
}

int tokensIsEmpty()
{
    return tokenIdx >= tokensNum;
}
token *tokensGetNextElement()
{
    return (tokensIsEmpty() ? 0 : &tokensTable[tokenIdx++]);
}
token *tokensCheckNextElement(token *out)
{
    return (tokensIsEmpty() ? 0 : &tokensTable[tokenIdx]);
}
void tokensSkipElement(){
    tokenIdx++;
}
