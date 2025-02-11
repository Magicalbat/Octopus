// Permuted congruential generator
// Based on https://www.pcg-random.org

typedef struct {
    u64 state;
    u64 increment;
} prng;

void prng_seed_r(prng* rng, u64 init_state, u64 init_seq);
void prng_seed(u64 init_state, u64 init_seq);

u32 prng_rand_r(prng* rng);
u32 prng_rand(void);

f32 prng_rand_f32_r(prng* rng);
f32 prng_rand_f32(void);

f32 prng_std_norm_r(prng* rng);
f32 prng_std_norm(void);

