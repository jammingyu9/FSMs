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
    state = (nfa_state *)malloc(sizeof(nfa_state));
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

static nfa_state *nfa_compile_create_state(type_nfa *nfa, char symbol, int type)
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
            new_state = nfa_create_state(nfa->num_current_states, type);
            break;
            
        case STATE_FINAL:
        case STATE_INNER:
            new_state = nfa_create_state(nfa->num_current_states, type);
            break;
            
        default:
            fprintf(stderr, "error in nfa_compile_create_state: not supported state type\n");
            break;
    }
    if (new_state != NULL) nfa->num_current_states++;
    return new_state;
}

static nfa_state_group *nfa_create_state_group()
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
        st_gp = nfa_create_state_group();
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
    nfa->start_state = nfa_compile_create_state(nfa, 0, STATE_START);
    if (nfa->start_state == NULL)
    {
        fprintf(stderr, "error in nfa_compile(): error creating state\n");
        return 0;
    }
    nfa->current_state = nfa->start_state;
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
            new_state = nfa_compile_create_state(nfa, regex[i], STATE_INNER);
            
            if (new_state == NULL)
            {
                fprintf(stderr, "error in nfa_compile(): error creating state\n");
                return 0;
            }
            nfa->state_list.nfa_states[nfa->state_list.num_states++] = new_state;
            
            // make a transition
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
        }
        else if (regex[i] == 42) // closure (ex '*')
        {
            // TODO : check previous input
            // TODO : implement greediness?
            if (flag & FLAG_BRACE_CLOSE)
            {
                fprintf(stdout, "Making a circle for [%c]\n", symbol_stack[save_count - 1]);
                nfa_compile_create_transition(nfa, nfa->current_state, state_stack[save_count - 1], symbol_stack[save_count - 1]);
                nfa_compile_create_transition(nfa, state_stack[save_count - 1], nfa->current_state, ANY_SYMBOL);
                save_count--; brace_open_num--; brace_close_num--;
                flag &= ~FLAG_BRACE_CLOSE;
            }
            else 
            {
                fprintf(stdout, "Making a circle for [%c] (no brace)\n", current_symbol);
                nfa_compile_create_transition(nfa, nfa->current_state, before_state, current_symbol);
                nfa_compile_create_transition(nfa, before_state, nfa->current_state, ANY_SYMBOL);
            }
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

static int nfa_add_to_state_group(nfa_state_group *state_group, nfa_state *state)
{
    int i;
    int flag = 1;
    if (state != NULL)
    {
        for (i=0; i<state_group->num_states; i++)
        {
            if (state->id == state_group->nfa_states[i]->id)
                flag = 0;
        }
        if (flag == 1)
        {
            //add state!
            state_group->nfa_states[state_group->num_states++] = state;
        }
    }
    else
    {
        flag = 0;
        fprintf(stderr, "ERROR: Can't add an empty state.\n");
    }
    return flag;
}

/* 1) If empty transition occurs, the dst state must be added to next_states
 * 2) You must loop until theres no empty transition in next_states.
 * 
 */
static int nfa_process_empty_inputs(type_nfa *nfa, nfa_state_group *cur_states)
{
    int i, j;
    int added_num = 0;
    nfa_state_group *tmp_st_gp;
    
    for (i=0; i<cur_states->num_states; i++)
    {
        tmp_st_gp = cur_states->nfa_states[i]->transitions[ANY_SYMBOL];
        if (tmp_st_gp == NULL) continue;
        for (j=0; j<tmp_st_gp->num_states; j++)
        {
            added_num += nfa_add_to_state_group(cur_states, tmp_st_gp->nfa_states[j]);
        }
    }
    if (added_num != 0)
        nfa_process_empty_inputs(nfa, cur_states);
    return added_num;
}

/* Given an input, generate the next state group
 * from current state group
 */
static void nfa_process_one_input(type_nfa *nfa, nfa_state_group *cur_states, nfa_state_group *next_states, int ch)
{
    int i, j;
    nfa_state_group *tmp_st_gp;
    
    memset(next_states, 0, sizeof(nfa_state_group));
    // chew up all ANY_SYMBOL
    nfa_process_empty_inputs(nfa, cur_states);
    
    // chew up an normal symbol
    for (i=0; i<cur_states->num_states; i++)
    {
        tmp_st_gp = cur_states->nfa_states[i]->transitions[ch];
        if (tmp_st_gp == NULL) continue;
        for (j=0; j<tmp_st_gp->num_states; j++)
        {
            nfa_add_to_state_group(next_states, tmp_st_gp->nfa_states[j]);
        }
    }

    nfa_process_empty_inputs(nfa, next_states);
}

static int isFinalGroup(nfa_state_group *st_gp)
{
    int i;
    
    for (i=0; i<st_gp->num_states; i++)
    {
        if (st_gp->nfa_states[i]->type == STATE_FINAL)
            return 1;
    }
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
        //fprintf(stdout, "current num of states : %d\n", next->num_states);
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

static dfa_state *dfa_create_state(type_dfa *dfa, int type)
{
    dfa_state *d_state;
    
    d_state = (dfa_state *)malloc(sizeof(dfa_state));
    if (d_state == NULL)
    {
        fprintf(stderr, "ERROR: could not malloc dfa_state");
        return NULL;
    }
    memset(d_state, 0, sizeof(dfa_state));
    d_state->type = type;
    d_state->id = dfa->num_current_states;
    dfa->dfa_states[dfa->num_current_states++] = d_state;
    return d_state;
}

static nfa_dfa_mapped_state *ndmap_create_state(nfa_state_group *n_s, dfa_state *d_s)
{
    nfa_dfa_mapped_state *nd_state;
    
    nd_state = (nfa_dfa_mapped_state *)malloc(sizeof(nfa_dfa_mapped_state));
    if (nd_state == NULL)
    {
        fprintf(stderr, "ERROR: could not malloc nd_mapped_state");
        return NULL;
    }
    memset(nd_state, 0, sizeof(nfa_dfa_mapped_state));
    nd_state->d_state = d_s;
    nd_state->n_st_gp = n_s;
    return nd_state;
}

static int nfa_is_same_state_group(nfa_state_group *g1, nfa_state_group *g2)
{
    int i, j;
    int match_count = 0;
    if (g1->num_states == g2->num_states)
    {
        for (i=0; i<g2->num_states; i++)
        {
            for (j=0; j<g1->num_states; j++)
            {
                if (g2->nfa_states[i]->id == g1->nfa_states[j]->id)
                {
                    // state exists
                    match_count++;
                    break;
                }
            }
        }
        // same
        if (match_count == g1->num_states)
            return 1;
    }
    return 0;
}

type_dfa *nfa_to_dfa(type_nfa *nfa)
{
    type_dfa *dfa = NULL;
    dfa_state *d_state;
    nfa_state_group *n_states;
    nfa_state_group cur_states;
    nfa_state_group next_states;
    nfa_dfa_mapped_state *cur_state;
    nfa_dfa_mapped_state *new_state;
    nfa_dfa_mapped_state *queue[MAX_TRANSITIONS*MAX_TRANSITIONS] = {NULL, };
    int queue_count = 0;
    int cur_idx = 0;
    int i, j;

    dfa = (type_dfa *)malloc(sizeof(type_dfa));
    memset(dfa, 0, sizeof(type_dfa));
    memset(&cur_states, 0, sizeof(nfa_state_group));

    if (nfa->start_state != NULL)
    {
        // dfa start state
        dfa->start_state = dfa_create_state(dfa, STATE_START);
        
        // nfa start state_group
        n_states = nfa_create_state_group();
        n_states->nfa_states[n_states->num_states++] = nfa->start_state;
        nfa->num_current_states++;
        
        // link it
        cur_state = ndmap_create_state(n_states, dfa->start_state);
        queue[queue_count++] = cur_state;
    }
    else
    {
        fprintf(stderr, "Empty nfa!\n");
        return NULL;
    }
    fprintf(stderr, "start dfa states: %d\n", dfa->num_current_states);
    // process states in the queue
    while (cur_idx < queue_count)
    {
        // pop
        cur_state = queue[cur_idx];
        
        // process
        for (i=1; i<MAX_TRANSITIONS; i++)
        {
            memset(&next_states, 0, sizeof(nfa_state_group));
            
            /* next_states represents an next dfa state for input "i"
             * - we should add it to the queue if it's new
             * - we should make a transition if next_states is not empty
             */
            nfa_process_one_input(nfa, cur_state->n_st_gp, &next_states, i);
            if (next_states.num_states > 0)
            {
                fprintf(stderr, "%c has next state\n", i);
                int exist = 0;
                // does it already exist?
                for (j=0; j<queue_count; j++)
                {
                    if (nfa_is_same_state_group(queue[j]->n_st_gp, &next_states))
                    {
                        // is already in
                        fprintf(stderr, "State already in: %c\n", i);
                        cur_state->d_state->transitions[i] = queue[j]->d_state;
                        exist = 1;
                        break;
                    }
                }
                if (exist != 1)
                {
                    // make new state/transition and add it to the queue
                    fprintf(stderr, "Adding a new nd state\n");
                    int state_type = STATE_INNER;
                    for (j=0; j<next_states.num_states; j++)
                    {
                        if (next_states.nfa_states[j]->type == STATE_FINAL)
                        {
                            state_type = STATE_FINAL;
                        }
                    }
                    
                    d_state = dfa_create_state(dfa, state_type);
                    n_states = nfa_create_state_group();
                    memcpy(n_states, &next_states, sizeof(nfa_state_group));
                    
                    new_state = ndmap_create_state(n_states, d_state);
                    cur_state->d_state->transitions[i] = d_state;
                    queue[queue_count++] = new_state;
                    fprintf(stderr, "num states: %d\n", next_states.num_states);
                    for (j=0; j<next_states.num_states; j++)
                    {
                        fprintf(stderr, "state: %d\n", next_states.nfa_states[j]->id);
                    }
                }
            }
        }
        cur_idx++;
    }
    fprintf(stderr, "NFA to DFA conversion ended\n");
    fprintf(stderr, "total dfa states: %d\n", dfa->num_current_states);
    
    // free except dfa
    for (i=0; i<queue_count; i++)
    {
        if (queue[i] != NULL)
        {
            free(queue[i]);
        }
    }
    
    return dfa;
}

int dfa_scan(type_dfa *dfa, char *input_string, unsigned int input_len)
{
    int i;
    dfa_state *cur_state = NULL;
    
    if (dfa == NULL || dfa->start_state == NULL || input_len == 0)
    {
        fprintf(stderr, "ERROR: empty dfa or empty input.");
        return 0;
    }
    
    cur_state = dfa->start_state;
    for (i=0; i<input_len; i++)
    {
        cur_state = cur_state->transitions[(int)input_string[i]];
        if (cur_state == NULL)
            return 0;
    }
    if (cur_state->type == STATE_FINAL)
        return 1;
    return 0;
}

void dfa_clean(type_dfa *dfa)
{
    int i;
    
    if (dfa != NULL)
    {
        for (i=0; i<dfa->num_current_states; i++)
        {
            if (dfa->dfa_states[i] != NULL)
                free(dfa->dfa_states[i]);
        }
        free(dfa);
    }
}