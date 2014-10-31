//
// Public domain. 2014. <cycad@greencams.net>
//

#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "spritz.h"


static void sc_swap(SPRITZ_CONTEXT *ctx, size_t i, size_t j);
static uint8_t sc_state(SPRITZ_CONTEXT *ctx, size_t i);
static size_t gcd(size_t a, size_t b);
static void sc_whip(SPRITZ_CONTEXT *ctx, size_t r);
static void sc_crush(SPRITZ_CONTEXT *ctx);
static void sc_squeeze(SPRITZ_CONTEXT *ctx, size_t r, uint8_t *output);
static uint8_t sc_squeeze_byte(SPRITZ_CONTEXT *ctx);
static void sc_update(SPRITZ_CONTEXT *ctx);
static uint8_t sc_output(SPRITZ_CONTEXT *ctx);
static uint8_t sc_drip(SPRITZ_CONTEXT *ctx);
static void sc_shuffle(SPRITZ_CONTEXT *ctx);
static void sc_absorb_byte(SPRITZ_CONTEXT *ctx, uint8_t b);
static void sc_absorb_buffer(SPRITZ_CONTEXT *ctx, const void *data, size_t data_sz);
static void sc_absorb_stop(SPRITZ_CONTEXT *ctx);
static void sc_absorb_nibble(SPRITZ_CONTEXT *ctx, uint8_t x);
static uint8_t sc_high(SPRITZ_CONTEXT *ctx, uint8_t b);
static uint8_t sc_low(SPRITZ_CONTEXT *ctx, uint8_t b);


static
uint8_t
sc_high(SPRITZ_CONTEXT *ctx, uint8_t b)
{
	return  b / ctx->d;
}


static
uint8_t sc_low(SPRITZ_CONTEXT *ctx, uint8_t b)
{
	return b % ctx->d;
}


static
void
sc_swap(SPRITZ_CONTEXT *ctx, size_t i, size_t j)
{
	uint8_t t;
	
	i %= ctx->n;
	j %= ctx->n;

	t = ctx->state[i];
	ctx->state[i] = ctx->state[j];
	ctx->state[j] = t;
}


static
uint8_t
sc_state(SPRITZ_CONTEXT *ctx, size_t i)
{
	return ctx->state[i % ctx->n];
}


static
size_t
gcd(size_t a, size_t b)
{
	size_t v, result;

	result = 0;
	for (v = 1; v <= a && v <= b; ++v) {
		if (a % v == 0 && b % v == 0) result = v;
	}

	return result;
}


static
void
sc_whip(SPRITZ_CONTEXT *ctx, size_t r)
{
	size_t v;

	if (r == 0) return;

	for (v = 0; v < r; ++v) {
		sc_update(ctx);
	}

	do {
		ctx->w = (uint8_t)((((size_t)ctx->w) + 1) % ctx->n);
	} while (gcd(ctx->w, ctx->n) != 1);
}


static
void
sc_crush(SPRITZ_CONTEXT *ctx)
{
	size_t v;
	uint8_t t;

	for (v = 0; v < ctx->n / 2; ++v) {
		t = (uint8_t)((ctx->n - 1 - v) % ctx->n);

		if (sc_state(ctx, v) > sc_state(ctx, t)) sc_swap(ctx, v, t);
	}
}


static
void
sc_squeeze(SPRITZ_CONTEXT *ctx, size_t r, uint8_t *output)
{
	size_t v;

	if (r == 0) return;

	if (ctx->a > 0) sc_shuffle(ctx);

	for (v = 0; v < r; ++v) {
		output[v] = sc_drip(ctx);
	}
}


static
uint8_t
sc_squeeze_byte(SPRITZ_CONTEXT *ctx)
{
	uint8_t b;
	sc_squeeze(ctx, 1, &b);
	return b;
}


static
void
sc_update(SPRITZ_CONTEXT *ctx)
{
	ctx->i = (uint8_t)((((size_t)ctx->i) + ctx->w) % ctx->n);

	ctx->j = (uint8_t)((((size_t)ctx->k) + sc_state(ctx, ((size_t)ctx->j) + sc_state(ctx, ctx->i))) % ctx->n);
	ctx->k = (uint8_t)((((size_t)ctx->i) + ctx->k + sc_state(ctx, ctx->j)) % ctx->n);
	
	sc_swap(ctx, ctx->i, ctx->j);
}


static
uint8_t
sc_output(SPRITZ_CONTEXT *ctx)
{
	size_t sum = (size_t)sc_state(ctx, ((size_t)ctx->j) +
				 (size_t)sc_state(ctx, ((size_t)ctx->i) +
				 (size_t)sc_state(ctx, ((size_t)ctx->z) + ctx->k)));

	ctx->z = (uint8_t)(sum % ctx->n);

	return ctx->z;
}


static
uint8_t
sc_drip(SPRITZ_CONTEXT *ctx)
{
	if (ctx->a > 0) sc_shuffle(ctx);
	sc_update(ctx);
	return sc_output(ctx);
}


static
void
sc_shuffle(SPRITZ_CONTEXT *ctx)
{
	sc_whip(ctx, ctx->n * 2);
	sc_crush(ctx);
	sc_whip(ctx, ctx->n * 2);
	sc_crush(ctx);
	sc_whip(ctx, ctx->n * 2);

	ctx->a = 0;
}


static
void
sc_absorb_byte(SPRITZ_CONTEXT *ctx, uint8_t b)
{
	b %= ctx->n;
	sc_absorb_nibble(ctx, sc_low(ctx, b));
	sc_absorb_nibble(ctx, sc_high(ctx, b));
}


static
void
sc_absorb_buffer(SPRITZ_CONTEXT *ctx, const void *data, size_t data_sz)
{
	size_t v;
	const uint8_t *p = (uint8_t*)data;

	for (v = 0; v < data_sz; ++v) {
		sc_absorb_byte(ctx, p[v]);
	}
}


static
void
sc_absorb_stop(SPRITZ_CONTEXT *ctx)
{
	if (ctx->a == ctx->n / 2) sc_shuffle(ctx);

	ctx->a = (uint8_t)((((size_t)ctx->a) + 1) % ctx->n);
}


static
void
sc_absorb_nibble(SPRITZ_CONTEXT *ctx, uint8_t x)
{
	if (ctx->a == ctx->n / 2) sc_shuffle(ctx);
	
	sc_swap(ctx, ctx->a, ctx->n / 2 + x);

	ctx->a = (uint8_t)((((size_t)ctx->a) + 1) % ctx->n);
}


SPRITZ_CONTEXT*
sc_alloc3(const void *key, size_t key_sz, size_t n)
{
	size_t v = 0;
	SPRITZ_CONTEXT *ctx = (SPRITZ_CONTEXT*)malloc(sizeof(SPRITZ_CONTEXT));
	if (!ctx) return NULL;
	
	memset(ctx, 0, sizeof(SPRITZ_CONTEXT));

	ctx->state = (uint8_t*)malloc(n);
	if (!ctx->state) { free(ctx); return NULL; }
	memset(ctx->state, 0, n);
	
	ctx->w = 1;
	ctx->n = n;
	ctx->d = (uint8_t)ceil(sqrt((double)n));
	
	for (v = 0; v < n; ++v) {
		ctx->state[v] = (uint8_t)v;
	}

	sc_absorb_buffer(ctx, key, key_sz);

	return ctx;
}


SPRITZ_CONTEXT*
sc_alloc5(const void *key, size_t key_sz, uint8_t *iv, size_t iv_sz, size_t n)
{
	SPRITZ_CONTEXT *ctx = sc_alloc3(key, key_sz, n);
	if (ctx) {
		sc_absorb_stop(ctx);
		sc_absorb_buffer(ctx, iv, iv_sz);
	}

	return ctx;
}


void
sc_free(SPRITZ_CONTEXT *ctx)
{
	if (ctx) free(ctx->state);
	free(ctx);
}


void
sc_encrypt(SPRITZ_CONTEXT *ctx, void *data, size_t data_sz)
{
	size_t v;
	uint8_t *p = (uint8_t*)data;

	for (v = 0; v < data_sz; ++v) {
		p[v] = (uint8_t)((((size_t)p[v]) + sc_squeeze_byte(ctx)) % ctx->n);
	}
}


void
sc_decrypt(SPRITZ_CONTEXT *ctx, void *data, size_t data_sz)
{
	size_t v;
	uint8_t *p = (uint8_t*)data;

	for (v = 0; v < data_sz; ++v) {
		p[v] = (uint8_t)((((size_t)p[v]) - sc_squeeze_byte(ctx)) % ctx->n);
	}
}


int
sc_hash(const void *data, size_t data_sz, uint8_t *hash, uint8_t hash_sz, size_t n)
{
	SPRITZ_CONTEXT *ctx;

	assert(hash_sz < n);
	if (hash_sz >= n) return 0;

	ctx = sc_alloc(NULL, 0, n);
	if (ctx) {
		sc_absorb_buffer(ctx, data, data_sz);
		sc_absorb_stop(ctx);
		sc_absorb_byte(ctx, hash_sz);
		sc_squeeze(ctx, hash_sz, hash);
	}

	return 1;
}


#if defined(SPRITZ_TEST)

#include <assert.h>
#include <stdio.h>

typedef struct test_vector TEST_VECTOR;
struct test_vector {
	const char *key;
	uint8_t *out;
};

TEST_VECTOR g_crypt_test_vectors[3] = {
	{ "ABC",     (uint8_t*)"\x77\x9a\x8e\x01\xf9\xe9\xcb\xc0" },
	{ "spam",    (uint8_t*)"\xf0\x60\x9a\x1d\xf1\x43\xce\xbf" },
	{ "arcfour", (uint8_t*)"\x1a\xfa\x8b\x5e\xe3\x37\xdb\xc7" },
};

TEST_VECTOR g_hash_test_vectors[3] = {
	{ "ABC",     (uint8_t*)"\x02\x8f\xa2\xb4\x8b\x93\x4a\x18" },
	{ "spam",    (uint8_t*)"\xac\xbb\xa0\x81\x3f\x30\x0d\x3a" },
	{ "arcfour", (uint8_t*)"\xff\x8c\xf2\x68\x09\x4c\x87\xb9" },
};

int
main(int argc, char *argv[])
{
	size_t i, j;
	SPRITZ_CONTEXT *ctx;
	const char *key;
	uint8_t *out;
	uint8_t hash[32];
	uint8_t b;
	int rv;

	// test the output of spritz through the sc_drip() function
	for (i = 0; i < 3; ++i) {
		key = g_crypt_test_vectors[i].key;
		out = g_crypt_test_vectors[i].out;
		
		ctx = sc_alloc(key, strlen(key), SPRITZ_N_DEFAULT);

		for (j = 0; j < 8; ++j) {
			b = sc_drip(ctx);
			if (b != out[j]) {
				printf("FAIL: %s[%d], %02X != %02X", key, j, b, out[j]);
				assert(0);
			}
		}

		sc_free(ctx);
	}

	// test the output of spritz hash function through the sc_drip() function
	for (i = 0; i < 3; ++i) {
		key = g_hash_test_vectors[i].key;
		out = g_hash_test_vectors[i].out;
		
		rv = sc_hash(key, strlen(key), hash, 32, SPRITZ_N_DEFAULT);
		if (!rv) { printf("FAIL: sc_hash(\"%s\");\n", key); assert(0); }

		for (j = 0; j < 8; ++j) {
			if (hash[j] != out[j]) {
				printf("FAIL: %s[%d], %02X != %02X", key, j, b, out[j]);
				assert(0);
			}
		}

		sc_free(ctx);
	}

	printf("Press ENTER to exit...\n");
	getchar();

	return 0;
}

#endif
