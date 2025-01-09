#include "base_prng.h"

// Permuted congruential generator
// Based on https://www.pcg-random.org

#include <math.h>

static THREAD_LOCAL prng s_rng = { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL };

void prng_seed_r(prng* rng, u64 init_state, u64 init_seq) {
    if (rng == NULL) {
        return;
    }

    rng->state = 0;
    rng->increment = (init_seq << 1) | 1;

    prng_rand_r(rng);

    rng->state += init_state;

    prng_rand_r(rng);
}
void prng_seed(u64 init_state, u64 init_seq) {
    prng_seed_r(&s_rng, init_state, init_seq);
}

u32 prng_rand_r(prng* rng) {
    if (rng == NULL) {
        return 0;
    }

    u64 old_state = rng->state;

    rng->state = old_state * 6364136223846793005ULL + rng->increment;

    u32 xorshifted = ((old_state >> 18u) ^ old_state) >> 27u;
    u32 rot = old_state >> 59u;

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
u32 prng_rand(void) {
    return prng_rand_r(&s_rng);
}

f32 prng_rand_f32_r(prng* rng) {
    return ldexpf(prng_rand_r(rng), -32);
}
f32 prng_rand_f32(void) {
    return prng_rand_f32_r(&s_rng);
}

// Box-Muller Transform
// https://en.wikipedia.org/wiki/Box–Muller_transform
f32 prng_std_norm_r(prng* rng){
    if (rng == NULL) {
        return 0;
    }

    static const f32 epsilon = 1e-6;

    f32 u1 = epsilon;
    f32 u2 = 0.0f;

    do {
        u1 = (prng_rand_f32_r(rng)) * 2.0f - 1.0f;
    } while (u1 <= epsilon);
    u2 = (prng_rand_f32_r(rng)) * 2.0f - 1.0f;

    f32 mag = sqrtf(-2.0f * logf(u1));
    f32 z0 = mag * cos(2.0f * 3.141592653f * u2);

    // I am ignoring the second value here
    // It might be worth trying to use it
    //f32 z1 = mag * sin(two_pi * u2);

    return z0;
}

f32 prng_std_norm(void) {
    return prng_std_norm_r(&s_rng);
}

