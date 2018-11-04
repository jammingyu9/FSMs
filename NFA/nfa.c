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
        if (st_gp == NULL)
        {
            fprintf(stderr, "error creating state trasition\n");
            return 0;
        }
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
    int flag, save_count;
    int brace_open_num = 0, brace_close_num = 0;
    nfa_state *new_state = NULL;
    nfa_state *before_state = NULL;
    nfa_state *ast_before_state = NULL;
    char current_symbol;
    nfa_state *state_stack[STATE_STACK_SIZE] = {NULL,};
    char symbol_stack[STATE_STACK_SIZE] = {0, };
    
    
    if (nfa == NULL)
    {
        fprintf(stderr, "error in nfa_compile(): nfa not initiated\n");
        return 0;
    }
    save_count = 0;
    flag = 0;
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
    current_symbol = 0;
    
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
            new_state = nfa_compile_create_state(nfa, regex[i], nfa->num_current_states, STATE_INNER);
            
            if (new_state == NULL)
            {
                fprintf(stderr, "error in nfa_compile(): error creating state\n");
                return 0;
            }
            new_state->id = nfa->num_current_states++;
            nfa->state_list.nfa_states[nfa->state_list.num_states++] = new_state;
            
            // make a transition
            if (flag & FLAG_ASTERISK)
            {
                nfa_compile_create_transition(nfa, nfa->current_state, new_state, regex[i]);
                // also can be skipped
                nfa_compile_create_transition(nfa, ast_before_state, new_state, regex[i]);
                flag &= ~FLAG_ASTERISK;
            }
            else
                nfa_compile_create_transition(nfa, nfa->current_state, new_state, regex[i]);
            
            before_state = nfa->current_state;
            nfa->current_state = new_state;
            current_symbol = regex[i];
            
            // save symbols for braces
            if (flag & FLAG_BRACE_OPEN)
            {
                int i;
                for (i=0; i<brace_open_num; i++)
                {
                    state_stack[save_count] = nfa->current_state;
                    symbol_stack[save_count] = current_symbol;
                    save_count++;
                }
                flag &= ~FLAG_BRACE_OPEN;
            }
            if (flag & FLAG_BRACE_CLOSE)
            {
                save_count--; brace_open_num--; brace_close_num--;
                flag &= ~FLAG_BRACE_CLOSE;
            }

            if (flag & FLAG_ASTERISK)
            {
                flag &= ~FLAG_ASTERISK;
            }
        }
        else if (regex[i] == 42) // closure (ex '*')
        {
            // TODO : check previous input
            // TODO : implement greediness?
            if (flag & FLAG_BRACE_CLOSE)
            {
                if (nfa_compile_create_transition(nfa, nfa->current_state, state_stack[save_count - 1], symbol_stack[save_count - 1]) == 1)
                    fprintf(stdout, "Making a circle for [%c]\n", symbol_stack[save_count - 1]);
                ast_before_state = state_stack[save_count - 1];
                save_count--; brace_open_num--; brace_close_num--;
                flag &= ~FLAG_BRACE_CLOSE;
            }
            else 
            {
                if (nfa_compile_create_transition(nfa, nfa->current_state, before_state, current_symbol) == 1)
                    fprintf(stdout, "Making a circle for [%c] (no brace)\n", current_symbol);
                ast_before_state = before_state;
            }
            flag |= FLAG_ASTERISK;
        }
        else if (regex[i] == 40) // group (ex '(')
        {
            brace_open_num++;
            flag |= FLAG_BRACE_OPEN;
        }
        else if (regex[i] == 41) // group (ex ')')
        {
            if (flag & FLAG_BRACE_CLOSE)
            {
                save_count--; brace_open_num--; brace_close_num--;
            }
            brace_close_num++;
            flag |= FLAG_BRACE_CLOSE;
            if (flag & FLAG_ASTERISK)
                flag &= ~FLAG_ASTERISK;
        }
        // TODO : union, OR
        if (i == (regex_size-1))
        {
            nfa->current_state->type = STATE_FINAL;
        }
    }
    if (brace_open_num != brace_close_num)
    {
        fprintf(stderr, "brace numbers do not match!\n");
        return 0;
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
    for (i=0; i<cur_states->num_states; i++)
    {
        tmp_st_gp = cur_states->nfa_states[i]->transitions[ANY_SYMBOL];
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