#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nfa.h"

int main(int argc, char **argv)
{
    type_nfa *nfa = NULL;
    //type_dfa *dfa = NULL;
    char regex_buffer[256] = {0, };
    char input_buffer[256] = {0, };
    int scan_result;
    
	printf("Init nfa\n");
    if (!nfa_init(&nfa))
    {
        fprintf(stderr, "Fail in nfa_init()\n");
        return 0;
    }
    
    printf("regex: ");
    scanf("%s", regex_buffer);
    
    if (!nfa_compile(nfa, regex_buffer, strlen(regex_buffer)))
    {
        fprintf(stderr, "Fail in nfa_compile()\n");
        return 0;
    }
    /*
    if ((dfa = nfa_to_dfa(nfa)) == NULL)
    {
        fprintf(stderr, "Fail in nfa_to_dfa()\n");
        return 0;
    }*/
    
    while (1)
    {
        printf("input string: ");
        memset(input_buffer, 0, 256);
        scanf("%s", input_buffer);
        if (strcmp("q", input_buffer) == 0) break;
        scan_result = nfa_scan(nfa, input_buffer, strlen(input_buffer));
        if (scan_result == 1)
        {
            // found
            fprintf(stdout, "NFA Accepted\n");
        }
        else
        {
            // not found
            fprintf(stdout, "NFA NOT Accepted\n");
        }
    }
    nfa_clean(nfa);
	return 1;
}
