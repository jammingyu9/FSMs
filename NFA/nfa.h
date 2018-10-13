#ifndef NFA_H
#define NFA_H

#define MAX_STATES 128
#define MAX_TRANSITIONS 256

#define START_PRINTABLE_ASCII 32
#define NUM_PRINTABLE_ASCII 95

#define STATE_START 0x1
#define STATE_INNER 0x10
#define STATE_FINAL 0x100

/* NFA data structure */
typedef struct _nfa_transition {
    char input;
    int dst_id;
} nfa_transition;

// when it is scanned, the group of states of the same level are maintained
typedef struct nfa_state nfa_state;

typedef struct _nfa_state_group {
    nfa_state *nfa_states[MAX_STATES];
    unsigned int num_states;
} nfa_state_group;

struct nfa_state {
    int id;
    int type;
    // need allocation
    nfa_state_group *transitions[MAX_TRANSITIONS];
};
typedef struct _type_nfa {
    // symbols
    char *symbols;
    unsigned int num_symbols;
    
    // states
    nfa_state *start_state;
    nfa_state *current_state;
    nfa_state_group state_list;
    nfa_state *saved_state;
    char saved_symbol;
    unsigned int num_current_states;
} type_nfa;

/* DFA data structure */
/*
typedef struct _dfa_transition {
    char input;
    int dst_id;
} dfa_transition;

typedef struct dfa_state dfa_state;
struct dfa_state {
    int id;
    int type;
    
    dfa_state
};

typedef struct _type_dfa {
    // symbols
    char *symbols;
    unsigned int num_symbols;
    
    // states
    dfa_state *start_state;
    dfa_state *current_state;
    dfa_state *saved_state;
    char saved_symbol;
    unsigned int num_current_states;
} type_dfa;
*/


int nfa_init(type_nfa **);

nfa_state *nfa_create_state(int, int type);

int nfa_compile(type_nfa *,
                char *, unsigned int); //regular expression

int nfa_scan(type_nfa *,
             char *,
             unsigned int); // input string

void nfa_clean(type_nfa *);

//type_dfa *nfa_to_dfa(type_nfa *);

#endif