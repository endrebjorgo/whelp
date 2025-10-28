#include "../include/lp.h"
#include <stdlib.h>
#include <stdio.h>

Lp *lp_new(int vars) {
    Lp *lp = calloc(1, sizeof(Lp));
    if (lp == NULL)  {
        fprintf(stderr, "ERROR: calloc failed");
        exit(1);
    }
    lp->rows = 0;
    lp->cols = vars + 1;
    for (int i = 0; i < TABLE_BUF_SIZE; ++i) {
        lp->idxs[i] = i;
    }
    return lp; 
}

void lp_free(Lp **lp) {
    free(*lp);
    *lp = NULL;
}

void lp_display_table(Lp *lp) {
    for (int i = 0; i < lp->rows; ++i) {
        for (int j = 0; j < lp->cols; ++j) {
            printf("%.2f ", lp->table[i * lp->cols + j]);
        }
        printf("\n");
    }
}

void lp_add_row(Lp *lp, int size, double *values) {
    if (size != lp->cols) {
        fprintf(stderr, "ERROR: wrong number of values");
        exit(1);
    }
    int start = lp->rows * lp->cols;
    for (int j = 0; j < lp->cols; ++j) {
        lp->table[start + j] = values[j];
    }
    lp->rows++;
}

void lp_add_objective_function(Lp *lp, int size, double *coefficients) {
    if (lp->rows != 0) {
        fprintf(stderr, "ERROR: attempted to add objective to non-empty table");
        exit(1);
    }
    lp_add_row(lp, size, coefficients);
}

void lp_add_constraint_leq(Lp *lp, int size, double *coefficients, double constant) {
    if (lp->rows == 0) {
        fprintf(stderr, "ERROR: attempted to add constraint to empty table");
        exit(1);
    }
    double *values = calloc(size, sizeof(double));
    if (values == NULL) {
        fprintf(stderr, "ERROR: calloc failed");
        exit(1);
    }
    values[0] = constant;
    for (int i = 1; i < size; ++i) {
        values[i] = -coefficients[i - 1];
    }
    lp_add_row(lp, size, values);
}

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
