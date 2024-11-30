#include <stdio.h>

#include "base/base.h"

int main(void) {
    vec4f a = { 1, 2, 3, 4 };
    vec4f b = { 1, 2, 3, 4 };
    vec4f c = vec4f_comp_div(a, b);

    printf("%f %f %f %f\n", c.x, c.y, c.z, c.w);

    return 0;
}

