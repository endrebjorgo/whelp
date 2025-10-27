#include "../include/lp.h"

#include <stdlib.h>
#include <stdio.h>

int main(void) {
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

    return 0;
}
