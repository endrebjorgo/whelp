#include <stdio.h>

#define WHELP_IMPLEMENTATION
#include "../whelp.h"

int main(void) {
    /*
    max 6x + 9y = 2 
    subject to
        2x + 3y <= 12,
        x + y <= 5,
        x, y >= 0
    */

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Whelp_Arena arena = whelp_arena_new(1024);
    Whelp_Lp *lp = whelp_lp_new(&arena, (double*[]){&x, &y, &z}, 3);
    whelp_lp_set_objective(&arena, lp, (double[]){2.0, 3.0, 4.0}, 0.0);
    whelp_lp_add_constraint(&arena, lp, (double[]){3.0, 2.0, 1,0}, LE, 10.0);
    whelp_lp_add_constraint(&arena, lp, (double[]){2.0, 5.0, 3.0}, LE, 15.0);
    whelp_lp_solve(&arena, lp);
}
