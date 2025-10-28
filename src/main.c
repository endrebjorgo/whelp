#include "../include/lp.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    char* name;
    double value;
} Var;

Var *var_new(char *name, double value) {
    Var *var = calloc(1, sizeof(Var));
    if (var == NULL) {
        fprintf(stderr, "ERROR: calloc failed");
        exit(1);
    }
    var->name = name;
    var->value = value;
    return var;
}

void var_free(Var **var) {
    free(*var);
    *var = NULL;
}

typedef struct {
    int size;
    double constant;
    double coeffs[VAR_BUF_SIZE];
    Var *vars[VAR_BUF_SIZE];
} Expr;

Expr *expr_new(double constant, double *coeffs, Var **vars, int size) {
    if (size > VAR_BUF_SIZE) {
        fprintf(stderr, "ERROR: expression too large");
        exit(1);
    }
    Expr *expr = calloc(1, sizeof(Expr));
    if (expr == NULL) {
        fprintf(stderr, "ERROR: calloc failed");
        exit(1);
    }
    expr->constant = constant;
    expr->size = size;
    for (int i = 0; i < size; ++i) {
        expr->coeffs[i] = coeffs[i];
        expr->vars[i] = vars[i];
    }
    return expr;
}

void expr_free(Expr **expr) {
    free(*expr);
    *expr= NULL;
}

void expr_add_term(Expr *expr, double coeff, Var *var) {
    if (expr->size >= VAR_BUF_SIZE) {
        fprintf(stderr, "ERROR: expression too large");
        exit(1);
    }
    int i = expr->size;
    expr->coeffs[i] = coeff;
    expr->vars[i] = var;
    expr->size++;
}

double expr_evaluate(Expr *expr) {
    double sum = expr->constant;
    for (int i = 0; i < expr->size; ++i) {
        sum += expr->coeffs[i] * expr->vars[i]->value;
    }
    return sum;
}

typedef enum {
    EQ,
    LEQ,
    GEQ
} ConstraintOp;

typedef struct {
    Expr lhs;
    double rhs;
    ConstraintOp op;
} Constraint;

Constraint *constraint_new(Expr lhs, double rhs, ConstraintOp op) {
    Constraint *con = calloc(1, sizeof(Constraint));
    if (con == NULL) {
        fprintf(stderr, "ERROR: calloc failed");
        exit(1);
    }
    con->lhs = lhs;
    con->rhs = rhs;
    con->op = op;
    return con;
}

void contraint_free(Constraint **constraint) {
    free(*constraint);
    *constraint = NULL;
}

typedef struct {
    Expr objective;
    Constraint constraints[VAR_BUF_SIZE];
    double *table;
} Lip;

Lip *lip_new() {
    Lip *lip = calloc(1, sizeof(Lip));
    if (lip == NULL) {
        fprintf(stderr, "ERROR: calloc failed");
        exit(1);
    }
    return lip;
}

void lip_add_objective(Lip *lip, Expr expr) {
    lip->objective = expr;
}

int main(void) {
    Var *x1 = var_new("x1", 1.0);
    Var *x2 = var_new("x2", 1.0);

    double coeffs[] = {2.0, 3.0};
    Var *vars[] = {x1, x2};

    Expr *expr = expr_new(1.0, coeffs, vars, 2);

    double sum = expr_evaluate(expr);
    printf("%f\n", sum);

    var_free(&x1);
    var_free(&x2);
    expr_free(&expr);

    // TODO: add parser for .lp files
    
    /*
    Lp *lp = lp_new(3);

    double objective[] = {0.0, 40.0, 60.0, 50.0};
    lp_add_objective_function(lp, 4, objective);

    int constraints = 3;
    double coeffs[][3] = {
        {4.0, 6.0, 5.0},
        {3.0, 8.0, 6.0},
        {2.0, 3.0, 4.0},
    };
    double constants[] = {
        240.0, 
        200.0,
        120.0
    };

    for (int i = 0; i < constraints; ++i) {
        lp_add_constraint_leq(lp, 4, coeffs[i], constants[i]);
    }

    lp_solve(lp);
    lp_display_values(lp);

    lp_free(&lp);
    */

    return 0;
}
