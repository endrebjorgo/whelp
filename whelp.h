#ifndef WHELP_H
#define WHELP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define WHELP_MAX_CONSTRAINTS 16

typedef struct {
    void *memory;
    size_t count; 
    size_t capacity;
} Whelp_Arena;

Whelp_Arena whelp_arena_new(size_t capacity);
void *whelp_arena_alloc(Whelp_Arena *arena, size_t size);
void whelp_arena_reset(Whelp_Arena *arena);
void whelp_arena_free(Whelp_Arena **arena);

typedef enum {
    EQ,
    LE,
    GE
} Whelp_Sense;

typedef struct {
    double *coeffs;
    Whelp_Sense sense;
    double constant;
} Whelp_Relation;

typedef struct {
    double **vars;
    size_t vars_count;
    Whelp_Relation *objective;
    Whelp_Relation *constraints;
    size_t constraints_count;
} Whelp_Lp;

Whelp_Lp *whelp_lp_new(Whelp_Arena *arena, double **vars, size_t vars_count);
void whelp_lp_set_objective(Whelp_Arena *arena, Whelp_Lp *lp, double *coeffs, double constant);
void whelp_lp_add_constraint(Whelp_Arena *arena, Whelp_Lp *lp, double *coeffs, Whelp_Sense sense, double constant);
void whelp_lp_solve(Whelp_Arena *arena, Whelp_Lp *lp);

#endif // WHELP_H

#ifdef WHELP_IMPLEMENTATION

Whelp_Arena whelp_arena_new(size_t capacity) {
    Whelp_Arena arena = {0};
    arena.memory = malloc(capacity);
    arena.count = 0;
    arena.capacity = capacity;
    return arena; 
}

void *whelp_arena_alloc(Whelp_Arena *arena, size_t size) {
    if (arena->count + size >= arena->capacity) {
        return NULL;
    }
    arena->count += size;
    return arena->memory + arena->count - size;
}

void whelp_arena_reset(Whelp_Arena *arena) {
    arena->count = 0;
}

void whelp_arena_free(Whelp_Arena **arena) {
    free((*arena)->memory);
    free(*arena);
    *arena = NULL;
}

Whelp_Lp *whelp_lp_new(Whelp_Arena *arena, double **vars, size_t vars_count) {
    Whelp_Lp *lp = whelp_arena_alloc(arena, sizeof(Whelp_Lp));
    if (lp == NULL) {
        fprintf(stderr, "ERROR: whelp_arena_alloc failed.\n");
        exit(1);
    }
    lp->vars = vars;
    lp->vars_count = vars_count;
    lp->objective = NULL;
    lp->constraints = NULL;
    lp->constraints_count = 0;
    return lp;
}

void whelp_lp_set_objective(Whelp_Arena *arena, Whelp_Lp *lp, double *coeffs, double constant) {
    Whelp_Relation *obj = whelp_arena_alloc(arena, sizeof(Whelp_Relation));
    if (obj == NULL) {
        fprintf(stderr, "ERROR: whelp_arena_alloc failed.\n");
        exit(1);
    }
    obj->coeffs = whelp_arena_alloc(arena, lp->vars_count * sizeof(double));
    if (obj->coeffs == NULL) {
        fprintf(stderr, "ERROR: whelp_arena_alloc failed.\n");
        exit(1);
    }
    for (size_t i = 0; i < lp->vars_count; ++i) {
        obj->coeffs[i] = coeffs[i];
    }
    obj->sense = EQ;
    obj->constant = -constant;
    lp->objective = obj;
}

void whelp_lp_add_constraint(Whelp_Arena *arena, Whelp_Lp *lp, double *coeffs, Whelp_Sense sense, double constant) {
    if (sense != LE) {
        fprintf(stderr, "ERROR: currently does not support GE or EQ constraints.\n");
        exit(1);
    }

    if (lp->constraints_count == WHELP_MAX_CONSTRAINTS) {
        fprintf(stderr, "ERROR: reached max number of constraints.\n");
        exit(1);
    }
    if (lp->constraints == NULL) {
        lp->constraints = whelp_arena_alloc(arena, WHELP_MAX_CONSTRAINTS * sizeof(Whelp_Relation));
    }
    if (lp->constraints == NULL) {
        fprintf(stderr, "ERROR: whelp_arena_alloc failed.\n");
        exit(1);
    }
    Whelp_Relation *constraint = &lp->constraints[lp->constraints_count];
    constraint->coeffs = whelp_arena_alloc(arena, lp->vars_count * sizeof(double));
    if (constraint->coeffs == NULL) {
        fprintf(stderr, "ERROR: whelp_arena_alloc failed.\n");
        exit(1);
    }
    for (size_t i = 0; i < lp->vars_count; ++i) {
        constraint->coeffs[i] = coeffs[i];
    }
    constraint->sense = sense;
    constraint->constant = constant;
    lp->constraints_count++;
}

static double *_whelp_lp_generate_tableau(Whelp_Arena *arena, Whelp_Lp *lp) {
    size_t slack_count = 0;
    for (size_t i = 0; i < lp->constraints_count; ++i) {
        if (lp->constraints[i].sense == EQ) continue;
        if (lp->constraints[i].sense == GE) {
            for (size_t j = 0; j < lp->vars_count; ++j) {
                lp->constraints[i].coeffs[j] *= -1.0;
            }
            lp->constraints[i].sense = LE;
            lp->constraints[i].constant *= -1.0;
        }
        slack_count++;
    }
    // TODO: change implementation to work for GE and EQ
    size_t cols = lp->vars_count + slack_count + 1;
    size_t rows = lp->constraints_count + 1;
    double *tableau = whelp_arena_alloc(arena, cols * rows * sizeof(double));
    Whelp_Relation *curr;

    for (size_t i = 0; i < rows; ++i) {
        curr = (i == 0) ? lp->objective : &lp->constraints[i - 1];
        for (size_t j = 0; j < lp->vars_count; ++j) {
            tableau[i * cols + j] = curr->coeffs[j];
        }
        for (size_t j = lp->vars_count; j < cols - 1; ++j) {
            tableau[i * cols + j] =  0.0;
        }
        tableau[(i + 1) * cols - 1] = curr->constant;
    }
    // TODO: Enter 1's in the correct slack variable columns
    return tableau;
}

static void _whelp_tableau_display(double *tableau, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; ++i) {
        printf("[ ");
        for (size_t j = 0; j < cols; ++j) {
            printf("%.2f ", tableau[i * cols + j]);
        }
        printf("]\n");
    }
}

void whelp_lp_solve(Whelp_Arena *arena, Whelp_Lp *lp) {
    if (lp->objective == NULL) {
        fprintf(stderr, "ERROR: cannot solve lp without objective.\n");
        exit(1);
    }
    if (lp->constraints == NULL || lp->constraints_count == 0) {
        fprintf(stderr, "ERROR: cannot solve lp without constraints.\n");
        exit(1);
    }
    double *tableau = _whelp_lp_generate_tableau(arena, lp);
    size_t rows = lp->constraints_count + 1;
    size_t cols = lp->vars_count + lp->constraints_count + 1;
    _whelp_tableau_display(tableau, rows, cols);
}

/* ADDED FROM OLD IMPLEMENTATION FOR LATER REFERENCE
int lp_assign_pivot_idxs(Lp *lp, int *row, int *col) {
    int best_j = 1;
    for (int j = 1; j < lp->cols; ++j) {
        if (lp->table[j] > lp->table[best_j]) {
            best_j = j;
        }
    }
    if (lp->table[best_j] <= 0.0) {
        return 0;
    }
    double min_reciprocal_ratio = MAX_DOUBLE; 
    int best_i = 1;
    for (int i = 1; i < lp->rows; ++i) {
        double num = lp->table[i * lp->cols + best_j];
        double den = lp->table[i * lp->cols];
        double reciprocal_ratio = num / den;
        if (reciprocal_ratio < min_reciprocal_ratio) {
            min_reciprocal_ratio = reciprocal_ratio;
            best_i = i;
        }
    }
    if (min_reciprocal_ratio >= 0.0) {
        return 0;
    }
    *row = best_i;
    *col = best_j;
    return 1;
}

int lp_pivot(Lp *lp) {
    int row;
    int col;
    if (!lp_assign_pivot_idxs(lp, &row, &col)) {
        return 0;
    }
    double divisor = -1.0 * lp->table[row * lp->cols + col];
    lp->table[row * lp->cols + col] = -1.0;
    for (int j = 0; j < lp->cols; ++j) {
        lp->table[row*lp->cols + j] /= divisor;
    }
    for (int i = 0; i < lp->rows; ++i) {
        if (i == row) {
            continue;
        }
        double scale = lp->table[i * lp->cols + col];
        lp->table[i * lp->cols + col] = 0.0;
        for (int j = 0; j < lp->cols; ++j) {
            lp->table[i * lp->cols + j] += lp->table[row * lp->cols + j] * scale;
        }
    }
    int row_idx = row + lp->cols - 1;
    lp->idxs[col] ^= lp->idxs[row_idx];
    lp->idxs[row_idx] ^= lp->idxs[col];
    lp->idxs[col] ^= lp->idxs[row_idx];
    return 1;
}

void lp_display_values(Lp *lp) {
    printf("Z = %.2f\n", lp->table[0]);
    for (int j = 1; j < lp->cols; ++j) {
        double value = 0.0;
        if (lp->idxs[j] >= lp->cols) {
            int row = lp->idxs[j] - lp->cols + 1;
            value = lp->table[row * lp->cols];
        }
        printf("x%d = %.2f\n", j, value);
    }
}

void lp_solve(Lp *lp) {
    while (1) {
        if (!lp_pivot(lp)) {
            break;
        }
    }
}
*/

#endif // WHELP_IMPLEMENTATION
