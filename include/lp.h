#ifndef LP_H
#define LP_H

#define TABLE_BUF_SIZE 256
#define VAR_BUF_SIZE 16
#define MAX_DOUBLE 1000000.0 // For now...

typedef struct {
    int rows;
    int cols;
    double table[TABLE_BUF_SIZE];
    int idxs[TABLE_BUF_SIZE];
} Lp;

Lp *lp_new(int vars);
void lp_free(Lp **lp);
void lp_add_row(Lp *lp, int size, double *values);
void lp_add_objective_function(Lp *lp, int size, double *coefficients);
void lp_add_constraint_leq(Lp *lp, int size, double *coefficients, double constant);
int lp_assign_pivot_idxs(Lp *lp, int *row, int *col);
int lp_pivot(Lp *lp);
void lp_display_values(Lp *lp);
void lp_solve(Lp *lp);

#endif
