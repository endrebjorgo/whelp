#include <stdlib.h>
#include <stdio.h>

#define TABLE_BUF_SIZE 256
#define VAR_BUF_SIZE 16
#define MAX_FLOAT 1000000.0f // For now...

typedef struct {
    int rows;
    int cols;
    float table[TABLE_BUF_SIZE];
    int idxs[TABLE_BUF_SIZE];
} Lp;

Lp *lp_new(int vars) {
    Lp *lp = malloc(sizeof(Lp));
    if (lp == NULL)  {
        fprintf(stderr, "ERROR: malloc failed");
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

void lp_add_row(Lp *lp, int size, float *values) {
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

void lp_add_objective_function(Lp *lp, int size, float *coefficients) {
    if (lp->rows != 0) {
        fprintf(stderr, "ERROR: attempted to add objective to non-empty table");
        exit(1);
    }
    lp_add_row(lp, size, coefficients);
}

void lp_add_constraint_leq(Lp *lp, int size, float *coefficients, float constant) {
    if (lp->rows == 0) {
        fprintf(stderr, "ERROR: attempted to add constraint to empty table");
        exit(1);
    }
    float *values = malloc(size * sizeof(float));
    if (values == NULL) {
        fprintf(stderr, "ERROR: malloc failed");
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
    if (lp->table[best_j] <= 0.0f) {
        return 0;
    }
    float min_reciprocal_ratio = MAX_FLOAT; 
    int best_i = 1;
    for (int i = 1; i < lp->rows; ++i) {
        float num = lp->table[i * lp->cols + best_j];
        float den = lp->table[i * lp->cols];
        float reciprocal_ratio = num / den;
        if (reciprocal_ratio < min_reciprocal_ratio) {
            min_reciprocal_ratio = reciprocal_ratio;
            best_i = i;
        }
    }
    if (min_reciprocal_ratio >= 0.0f) {
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
    float divisor = -1.0 * lp->table[row * lp->cols + col];
    lp->table[row * lp->cols + col] = -1.0f;
    for (int j = 0; j < lp->cols; ++j) {
        lp->table[row*lp->cols + j] /= divisor;
    }
    for (int i = 0; i < lp->rows; ++i) {
        if (i == row) {
            continue;
        }
        float scale = lp->table[i * lp->cols + col];
        lp->table[i * lp->cols + col] = 0.0f;
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
        float value = 0.0f;
        if (lp->idxs[j] >= lp->cols) {
            int row = lp->idxs[j] - lp->cols + 1;
            value = lp->table[row * lp->cols];
        }
        printf("x%d = %.2f\n", j, value);
    }
}

void lp_solve(Lp *lp) {
    while (1) {
        lp_display_table(lp);
        printf("\n");
        if (!lp_pivot(lp)) {
            break;
        }
    }
}

#define VARS 3
int main(void) {
    Lp *lp = lp_new(VARS);

    float objective[] = {0.0, 40.0, 60.0, 50.0};
    lp_add_objective_function(lp, VARS + 1, objective);

    int constraints = 3;
    float coeffs[][VARS] = {
        {4.0, 6.0, 5.0},
        {3.0, 8.0, 6.0},
        {2.0, 3.0, 4.0},
    };
    float constants[] = {
        240.0, 
        200.0,
        120.0
    };

    for (int i = 0; i < constraints; ++i) {
        lp_add_constraint_leq(lp, VARS + 1, coeffs[i], constants[i]);
    }

    lp_solve(lp);
    lp_display_table(lp);
    lp_display_values(lp);

    lp_free(&lp);

    return 0;
}
