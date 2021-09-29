#include "kernel/types.h"

struct xorshift32_state {
  uint a;
};

/* The state word must be initialized to non-zero */
uint xorshift32(struct xorshift32_state *state)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}

struct xorshift64_state {
  uint64 a;
};

uint64 xorshift64(struct xorshift64_state *state)
{
	uint64 x = state->a;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return state->a = x;
}

struct xorshift128_state {
  uint a, b, c, d;
};

/* The state array must be initialized to not be all zero */
uint xorshift128(struct xorshift128_state *state)
{
	/* Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs" */
	uint t = state->d;

	uint const s = state->a;
	state->d = state->c;
	state->c = state->b;
	state->b = s;

	t ^= t << 11;
	t ^= t >> 8;
	return state->a = t ^ s ^ (s >> 19);
}