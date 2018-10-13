#include "nfa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int init_nfa_symbols(type_nfa *nfa)
{
    int i;
    int start = START_PRINTABLE_ASCII;
    int end = START_PRINTABLE_ASCII + NUM_PRINTABLE_ASCII;
    
    if (nfa->symbols != NULL)
    {
        fprintf(stderr, "error in init_nfa_symbols: symbols already initiated\n");
        return 0;
    }
    
    // alocate symbols
    nfa->symbols = (char *)malloc(sizeof(char) * NUM_PRINTABLE_ASCII);
    if (nfa->symbols == NULL)
    {
        fprintf(stderr, "Fail in init_nfa_symbols: malloc()\n");
        return 0;
    }
    
    // init symbols
    for (i=start; i<end; i++)
    {
        nfa->symbols[i] = i;
    }
    nfa->num_symbols = NUM_PRINTABLE_ASCII;
    return 1;
}

int nfa_init(type_nfa **nfa)
{
    if (*nfa != NULL)
    {
        fprintf(stderr, "error in nfa_init(): nfa not null\n");
        return 0;
    }
    
    // allocate nfa
    *nfa = (type_nfa *)malloc(sizeof(type_nfa));
    if ((*nfa) == NULL)
    {
        fprintf(stderr, "error in nfa_init(): nfa not allocated\n");
        return 0;
    }
    memset(*nfa, 0, sizeof(type_nfa));
    
    // init symbols
    printf ("initiating symbols\n");
    if (!init_nfa_symbols(*nfa))
    {
        fprintf(stderr, "error in nfa_init(): error in init_nfa_symbols()\n");
        return 0;
    }
    fprintf(stdout, "symbols num: %d\n", (*nfa)->num_symbols);   
    
    return 1;
}

nfa_state *nfa_create_state(int id, int type)
{
    int i;
    nfa_state *state = NULL;

    // allocate state
    printf("before malloc\n");
    state = (nfa_state *)malloc(sizeof(nfa_state));
    printf("after malloc: %p\n", state);
    if (state == NULL)
    {
        fprintf(stderr, "Error creating state\n");
        return NULL;
    }
    state->id = id;
    state->type = type;
    // init transitions to null
    for (i=0; i<MAX_TRANSITIONS; i++)
    {
        state->transitions[i] = NULL;
    }
    return state;
}

static nfa_state *nfa_compile_create_state(type_nfa *nfa, char symbol, int id, int type)
{
    nfa_state *new_state = NULL;
    if (nfa == NULL)
    {
        fprintf(stderr, "error in nfa_compile_create_state: nfa not initialized\n");
        return NULL;
    }
    
    // create states
    switch (type)
    {
        case STATE_START:
            new_state = nfa_create_state(id, type);
            break;
            
        case STATE_FINAL:
        case STATE_INNER:
            new_state = nfa_create_state(id, type);
            break;
            
        default:
            fprintf(stderr, "error in nfa_compile_create_state: not supported state type\n");
            break;
    }
    return new_state;
}

static nfa_state_group *nfa_compile_create_state_group()
{
    nfa_state_group *nfa_st_gp;
    
    nfa_st_gp = (nfa_state_group *)malloc(sizeof(nfa_state_group));
    if (nfa_st_gp == NULL)
    {
        fprintf(stderr, "error creating state group of nfa\n");
        return NULL;
    }
    memset(nfa_st_gp, 0, sizeof(nfa_state_group));
    return nfa_st_gp;
}

static int nfa_compile_create_transition(type_nfa *nfa,
                                         nfa_state *src_st,
                                         nfa_state *dst_st,
                                         char ch)
{
    nfa_state_group *st_gp = NULL;
    if ((nfa == NULL) || (src_st == NULL) || (dst_st == NULL))
    {
        fprintf(stderr, "error creating state trasition\n");
        return 0;
    }
    
    st_gp = src_st->transitions[(int)ch];
    if (st_gp == NULL)
    {
        st_gp = nfa_compile_create_state_group();
        src_st->transitions[(int)ch] = st_gp;
    }
    
    if (st_gp->num_states == MAX_STATES)
    {
        fprintf(stderr, "error creating state trasition: MAX_STATES reached\n");
        return 0; 
    }
    
    // add the state
    st_gp->nfa_states[st_gp->num_states++] = dst_st;
    return 1;
}

int nfa_compile(type_nfa *nfa, char *regex, unsigned int regex_size)
{
    int i;
    int brace_flag, saved;
    nfa_state *new_state = NULL;
    
    if (nfa == NULL)
    {
        fprintf(stderr, "error in nfa_compile(): nfa not initiated\n");
        return 0;
    }
    
    brace_flag = 0;
    nfa->num_current_states = 0;
    /* make start state */
    nfa->start_state = nfa_compile_create_state(nfa, 0, nfa->num_current_states, STATE_START);
    if (nfa->start_state == NULL)
    {
        fprintf(stderr, "error in nfa_compile(): error creating state\n");
        return 0;
    }
    nfa->current_state = nfa->start_state;
    nfa->num_current_states++;
    
    // chew up regular expressions
    for (i=0; i<regex_size; i++)
    {
        /* Concatenation (e.g. met an operand)
         * 
         * 1) create a new state
         * 2) make a transition
         * 2-1) check and create a state_group if necessary
         * 2-2) add a new state to the state_group
         */
        if ((regex[i] >= 97) && (regex[i] <= 122)) 
        {
            // create a new state
            if (i == (regex_size-1))
                new_state = nfa_compile_create_state(nfa, regex[i], nfa->num_current_states, STATE_FINAL);
            else
                new_state = nfa_compile_create_state(nfa, regex[i], nfa->num_current_states, STATE_INNER);
            
            if (new_state == NULL)
            {
                fprintf(stderr, "error in nfa_compile(): error creating state\n");
                return 0;
            }
            new_state->id = nfa->num_current_states++;
            nfa->state_list.nfa_states[nfa->state_list.num_states++] = new_state;
            
            // make a transition
            nfa_compile_create_transition(nfa, nfa->current_state, new_state, regex[i]);
            nfa->current_state = new_state;
            
            // check for grouping (braces)
            if ((brace_flag == 1) && (saved == 0))
            {
                nfa->saved_state = new_state;
                nfa->saved_symbol = regex[i];
                saved = 1;
            }
            else if (brace_flag == 2)
                brace_flag = 0;
        }
        else if (regex[i] == 42) // closure (ex '*')
        {
            // TODO : check previous input
            // TODO : implement greediness?
            
            if (brace_flag == 0)
            {
                if (nfa_compile_create_transition(n
                fa, nfa->current_state, nfa->current_state, regex[i-1]) == 1)
                    fprintf(stdout, "Making a circle for [%c]\n", regex[i-1]);
            }
            else if (brace_flag == 2)
            {
                if (nfa_compile_create_transition(nfa, nfa->current_state, nfa->saved_state, nfa->saved_symbol) == 1)
                    fprintf(stdout, "Making a circle for [%c]\n", nfa->saved_symbol);
                brace_flag = 0;
            }
        }
        else if (regex[i] == 40) // group (ex '(')
        {
            brace_flag = 1;
        }
        else if (regex[i] == 41) // group (ex ')')
        {
            brace_flag = 2;
        }
        // TODO : union, OR
    }
    fprintf(stdout, "regex to nfa compile finished\n");
    return 1;
}

/* Given an input, generate the next state group
 * from current state group
 */
static void nfa_process_one_input(type_nfa *nfa, nfa_state_group *cur_states, nfa_state_group *next_states, char ch)
{
    int i, j;
    nfa_state_group *tmp_st_gp;
    
    memset(next_states, 0, sizeof(nfa_state_group));
    for (i=0; i<cur_states->num_states; i++)
    {
        tmp_st_gp = cur_states->nfa_states[i]->transitions[(int)ch];
        if (tmp_st_gp == NULL) continue;
        for (j=0; j<tmp_st_gp->num_states; j++)
        {
            next_states->nfa_states[next_states->num_states++] = tmp_st_gp->nfa_states[j];
        }
    }
}

static int isFinalGroup(nfa_state_group *st_gp)
{
    int i;
    
    for (i=0; i<st_gp->num_states; i++)
    {
        if (st_gp->nfa_states[i]->type == STATE_FINAL)
        {
            fprintf(stdout, "Final states\n");
            return 1;
        }
    }
    fprintf(stdout, "No final states\n");
    return 0;
}

int nfa_scan(type_nfa *nfa, char *input_string, unsigned int input_size) // input string
{
    int i;
    nfa_state *cur_state = nfa->start_state;
    nfa_state_group cur_states;
    nfa_state_group next_states;
    nfa_state_group *cur, *next, *tmp;
    
    memset(&cur_states, 0, sizeof(nfa_state_group));
    cur_states.nfa_states[cur_states.num_states++] = cur_state;
    memset(&next_states, 0, sizeof(nfa_state_group));
    
    cur = &cur_states;
    next = &next_states;
    for (i=0; i<input_size; i++)
    {
        // chew one input
        nfa_process_one_input(nfa, cur, next, input_string[i]);
        fprintf(stdout, "current num of states : %d\n", next->num_states);
        tmp = cur;
        cur = next;
        next = tmp;

        if (cur->num_states != 0)
        {
            // char match
            continue;
        }
        else
        {
            // no match
            break;
        }
    }
    if ((i == input_size) && isFinalGroup(cur))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void nfa_clean_states(nfa_state_group *st_gp)
{
    int i;
    
    for (i=0; i<st_gp->num_states; i++)
        free(st_gp->nfa_states[i]);
}

void nfa_clean(type_nfa *nfa)
{    
    if (nfa->symbols != NULL)
        free(nfa->symbols);
        
    if (nfa->start_state != NULL)
    {
        nfa_clean_states(&(nfa->state_list));
    }
}

/*
type_dfa *nfa_to_dfa(type_nfa *nfa)
{
    type_dfa *dfa = NULL;
    int i;

    // for each input symbols
    for (i=START_PRINTABLE_ASCII; i<NUM_PRINTABLE_ASCII; i++)
    {
        
    }
    
    return dfa;
}*/