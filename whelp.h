#ifndef WHELP_H
#define WHELP_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>

#define WHELP_MAX_CONSTRAINTS 16

typedef struct {
    void *memory;
    size_t count; 
    size_t capacity;
} Whelp_Arena;

Whelp_Arena whelp_arena_new(size_t capacity);
void *whelp_arena_alloc(Whelp_Arena *arena, size_t size);
void *whelp_arena_calloc(Whelp_Arena *arena, size_t size);
void whelp_arena_reset(Whelp_Arena *arena);
void whelp_arena_free(Whelp_Arena *arena);

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
    double *items;
    size_t rows;
    size_t cols;
} Whelp_Table;

Whelp_Table *whelp_table_new(Whelp_Arena *arena, size_t rows, size_t cols);
double whelp_table_get(Whelp_Table *table, size_t row, size_t col);
void whelp_table_set(Whelp_Table *table, size_t row, size_t col, double value);
int whelp_table_pivot_indices(Whelp_Table *table, size_t *row, size_t *col);
int whelp_table_pivot(Whelp_Table *table);
void whelp_table_display(Whelp_Table *table);

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
    if (capacity == 0) {
        fprintf(stderr, "ERROR: attempt to malloc 0.\n");
        exit(1);
    }
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

void *whelp_arena_calloc(Whelp_Arena *arena, size_t size) {
    uint8_t *memory = whelp_arena_alloc(arena, size);
    for (size_t i = 0; i < size; ++i) {
        memory[i] = 0;
    }
    return (void*)memory;
}

void whelp_arena_reset(Whelp_Arena *arena) {
    arena->count = 0;
}

void whelp_arena_free(Whelp_Arena *arena) {
    if (arena == NULL) {
        fprintf(stderr, "ERROR: attempt to free null.\n");
        exit(1);
    }
    if (arena->memory == NULL) {
        fprintf(stderr, "ERROR: attempt to free uninitialized arena.\n");
        exit(1);
    }
    free(arena->memory);
    arena->memory = NULL;
}

Whelp_Table *whelp_table_new(Whelp_Arena *arena, size_t rows, size_t cols) {
    Whelp_Table *table = whelp_arena_alloc(arena, sizeof(Whelp_Table));
    if (table == NULL) {
        fprintf(stderr, "ERROR: whelp_arena_alloc failed.\n");
        exit(1);
    }
    table->items = whelp_arena_alloc(arena, rows * cols * sizeof(double));
    if (table->items == NULL) {
        fprintf(stderr, "ERROR: whelp_arena_alloc failed.\n");
        exit(1);
    }
    table->rows = rows;
    table->cols = cols;
    return table;
}

double whelp_table_get(Whelp_Table *table, size_t row, size_t col) {
    if (row >= table->rows) {
        fprintf(stderr, "ERROR: row out of bounds.\n");
        exit(1);
    }
    if (col >= table->cols) {
        fprintf(stderr, "ERROR: col out of bounds.\n");
        exit(1);
    }
    return table->items[row * table->cols + col];
}

void whelp_table_set(Whelp_Table *table, size_t row, size_t col, double value) {
    if (row >= table->rows) {
        fprintf(stderr, "ERROR: row out of bounds.\n");
        exit(1);
    }
    if (col >= table->cols) {
        fprintf(stderr, "ERROR: col out of bounds.\n");
        exit(1);
    }
    table->items[row * table->cols + col] = value;
}

int whelp_table_pivot_indices(Whelp_Table *table, size_t *row, size_t *col) 
{
    size_t pivot_col = 0;
    for (size_t j = 0; j < table->cols - 1; ++j) {
        if (whelp_table_get(table, 0, j) < whelp_table_get(table, 0, pivot_col)) {
            pivot_col = j;
        }
    }
    if (whelp_table_get(table, 0, pivot_col) >= 0.0) return 0;
    
    size_t pivot_row = 1;
    double max_reciprocal_ratio = DBL_MIN;
    for (size_t i = 1; i < table->rows; ++i) {
        double row_intersection = whelp_table_get(table, i, pivot_col);
        double row_constant = whelp_table_get(table, i, table->cols - 1);
        double reciprocal_ratio = row_intersection / row_constant;
        if (reciprocal_ratio > max_reciprocal_ratio) {
            max_reciprocal_ratio = reciprocal_ratio;
            pivot_row = i;
        }
    }
    if (max_reciprocal_ratio <= 0.0) return 0;

    *row = pivot_row;
    *col = pivot_col;
    return 1;
}

int whelp_table_pivot(Whelp_Table *table) {
    size_t pivot_row;
    size_t pivot_col;
    if (!whelp_table_pivot_indices(table, &pivot_row, &pivot_col)) {
        return 0;
    }
    double divisor = whelp_table_get(table, pivot_row, pivot_col);
    whelp_table_set(table, pivot_row, pivot_col, 1.0); // potential floating-point errors...
    assert(divisor > 0.0 && "Unexpected.\n");
    
    for (size_t j = 0; j < table->cols; ++j) {
        if (j == pivot_col) continue; // ... so just set instead of dividing.
        double curr_value = whelp_table_get(table, pivot_row, j);
        whelp_table_set(table, pivot_row, j, curr_value / divisor);
    }
    for (size_t i = 0; i < table->rows; ++i) {
        if (i == pivot_row) continue;

        double scale = whelp_table_get(table, i, pivot_col);
        whelp_table_set(table, i, pivot_col, 0.0);
        for (size_t j = 0; j < table->cols; ++j) {
            if (j == pivot_col) continue; // same here
            double curr_value = whelp_table_get(table, i, j);
            double pivot_row_value = whelp_table_get(table, pivot_row, j);
            whelp_table_set(table, i, j, curr_value - scale * pivot_row_value);
        }
    }
    return 1;
}

void whelp_table_display(Whelp_Table *table) {
    for (size_t i = 0; i < table->rows; ++i) {
        printf("[ ");
        for (size_t j = 0; j < table->cols; ++j) {
            printf("%.2f ", whelp_table_get(table, i, j));
        }
        printf("]\n");
    }
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
    obj->constant = -1.0 * constant;
    lp->objective = obj;
}

void whelp_lp_add_constraint(Whelp_Arena *arena, Whelp_Lp *lp, double *coeffs, Whelp_Sense sense, double constant) {
    assert(sense == LE && "Not yet implemented for GE and EQ relations.\n");

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

Whelp_Table *whelp_lp_generate_tableau(Whelp_Arena *arena, Whelp_Lp *lp) {
    size_t slack_count = 0;
    for (size_t i = 0; i < lp->constraints_count; ++i) {
        if (lp->constraints[i].sense == EQ) continue;
        slack_count++;
    }
    size_t rows = lp->constraints_count + 1;
    size_t cols = lp->vars_count + slack_count + 1;

    Whelp_Table *table = whelp_table_new(arena, rows, cols);

    for (size_t j = 0; j < lp->vars_count; ++j) {
        whelp_table_set(table, 0, j, -1.0 * lp->objective->coeffs[j]);
    }
    size_t curr_slack = 0;
    for (size_t i = 1; i < rows; ++i) {
        Whelp_Relation *curr_constraint = &lp->constraints[i - 1];
        for (size_t j = 0; j < lp->vars_count; ++j) {
            whelp_table_set(table, i, j, curr_constraint->coeffs[j]);
        }
        whelp_table_set(table, i, table->cols - 1, curr_constraint->constant);
        
        if (curr_constraint->sense == EQ) continue;

        double value = curr_constraint->sense == LE ? 1.0 : -1.0;
        whelp_table_set(table, i, lp->vars_count + curr_slack, value);
        curr_slack++;
    }
    return table;
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
    Whelp_Table *table = whelp_lp_generate_tableau(arena, lp);
    
    size_t count = 0;
    while (count < 100) {
        whelp_table_display(table);
        printf("\n");
        if (!whelp_table_pivot(table)) break;
        count++;
    }
    for (size_t j = 0; j < lp->vars_count; ++j) {
        if (whelp_table_get(table, 0, j) != 0.0) continue;

        for (size_t i = 1; i < table->rows; ++i) {
            if (whelp_table_get(table, i, j) == 1.0) {
                *lp->vars[j] = whelp_table_get(table, i, table->cols - 1);
            }
        }
    }
}

#endif // WHELP_IMPLEMENTATION
