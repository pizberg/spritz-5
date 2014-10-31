//
// Public domain. 2014. <cycad@greencams.net>
//
// An unoptimized reference implementation of Spritz as defined by Rivest
// and Schuldt in "Spritz - a spongy RC4-like stream cipher and hash function"
// found at http://people.csail.mit.edu/rivest/pubs/RS14.pdf
// 
// Example usage can be found in the test program in spritz.c.
//
// WARNING: Bad crypto is worse than no crypto; this implementation has
// has succeeded with the defined test vectors but beyond that is yet
// unproven. And even more importantly the algorithm itself is unproven.
//
// Use at your own risk.
//

#ifndef __SPRITZ_HPP
#define __SPRITZ_HPP 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPRITZ_N_DEFAULT 256

typedef struct spritz_context SPRITZ_CONTEXT;
struct spritz_context {
	size_t n;
	uint8_t i, j, k, z, a, w, d;
	uint8_t *state;
};

#define sc_alloc sc_alloc3
SPRITZ_CONTEXT* sc_alloc3(const void *key, size_t key_sz, size_t n);
SPRITZ_CONTEXT* sc_alloc5(const void *key, size_t key_sz, const void *iv, size_t iv_sz, size_t n);
void sc_free(SPRITZ_CONTEXT *ctx);
void sc_encrypt(SPRITZ_CONTEXT *ctx, void *data, size_t data_sz);
void sc_decrypt(SPRITZ_CONTEXT *ctx, void *data, size_t data_sz);

int sc_hash(const void *data, size_t data_sz, uint8_t *hash, uint8_t hash_sz, size_t n); // returns 1 or 0 on success or failure respectively

#ifdef __cplusplus
}
#endif

#endif
