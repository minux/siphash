#include <assert.h>

/* default: SipHash-2-4 */
#define ROTL(x,b) (unsigned long long)( ((x) << (b)) | ((x) >> (64 - (b))) )
#define CHECK(v) assert(#v && ((v)->val >> 32) == ((v)->hi & 0xfffffffful) && ((v)->val & 0xfffffffful) == ((v)->lo & 0xfffffffful))

typedef struct _uint64_t {
  unsigned long hi;
  unsigned long lo;
  unsigned long long val;
} uint64_t;

// bits must <=32
static inline void rotl(uint64_t *v, int bits) {
  CHECK(v);
  unsigned long tmp = v->hi;
  if (bits == 32) {
    v->hi = v->lo;
    v->lo = tmp;
  } else if (bits < 32) {
    v->hi = (tmp << bits) | ((0xfffffffful & v->lo) >> (32-bits));
    v->lo = (v->lo << bits) | ((0xfffffffful & tmp) >> (32-bits));
  }
  v->val = ROTL(v->val, bits);
  CHECK(v);
}

static inline void xor(uint64_t *dst, uint64_t *src) {
  CHECK(dst); CHECK(src);
  dst->hi ^= src->hi;
  dst->lo ^= src->lo;
  dst->val ^= src->val;
  CHECK(dst);
}

static inline void add(uint64_t *dst, uint64_t *src) {
  CHECK(dst); CHECK(src);
  dst->lo += src->lo;
  dst->hi += src->hi + ((dst->lo & 0xfffffffful) < (src->lo&0xfffffffful) ? 1:0);
  dst->val += src->val;
  CHECK(dst);
}

static inline void init(uint64_t *v) {
  v->val = ((unsigned long long)v->hi << 32) | (0xfffffffful & v->lo);
}

#define SIPROUND                                        \
  do {                                                  \
    add(&v0, &v1); rotl(&v1, 13); xor(&v1, &v0); rotl(&v0, 32); \
    add(&v2, &v3); rotl(&v3, 16); xor(&v3, &v2); \
    add(&v0, &v3); rotl(&v3, 21); xor(&v3, &v0); \
    add(&v2, &v1); rotl(&v1, 17); xor(&v1, &v2); rotl(&v2, 32); \
  } while(0)

typedef unsigned char uint8_t;
typedef unsigned long uint; // must match type of uint64_t.lo/hi
int  siphash( uint8_t *out, const uint8_t *in, uint inlen, const uint8_t *k ) {
  /* "somepseudorandomlygeneratedbytes" */
  uint64_t v0 = {0x736f6d65UL, 0x70736575UL}; init(&v0);
  uint64_t v1 = {0x646f7261UL, 0x6e646f6dUL}; init(&v1);
  uint64_t v2 = {0x6c796765UL, 0x6e657261UL}; init(&v2);
  uint64_t v3 = {0x74656462UL, 0x79746573UL}; init(&v3);
  uint64_t b;
  // hard-coded k.
  uint64_t k0 = {0x07060504UL, 0x03020100UL}; init(&k0); //U8TO64_LE( k );
  uint64_t k1 = {0x0F0E0D0CUL, 0x0B0A0908UL}; init(&k1); //U8TO64_LE( k + 8 );
  uint64_t m;
  int i;
  const int cROUNDS = 2, dROUNDS = 4;

  const uint8_t *end = in + inlen - (inlen % 8);
  int left = inlen & 7;
  b.hi = (inlen & 0xff)<<24; b.lo = 0; init(&b);
  CHECK(&v0); CHECK(&v1); CHECK(&v2); CHECK(&v3); CHECK(&b);
  xor(&v3, &k1); xor(&v2, &k0); xor(&v1, &k1); xor(&v0, &k0);
  for (; in != end; in += 8) {
    m.hi = 0; m.lo = 0;
    for (i = 0; i < 4; i++) m.lo |= ((uint)in[i]) << (8 * i);
    for (i = 0; i < 4; i++) m.hi |= ((uint)in[i+4]) << (8 * i); init(&m);
    xor(&v3, &m);
    for (i = 0; i < cROUNDS; i++) SIPROUND;
    xor(&v0, &m);
  }
  CHECK(&v0); CHECK(&v1); CHECK(&v2); CHECK(&v3); CHECK(&b);
  for (; left; left--)
    if (left > 4)
      b.hi |= ((uint)in[left-1]) << (8*left-8-32);
    else
      b.lo |= ((uint)in[left-1]) << (8*left-8);
  init(&b);
  xor(&v3, &b);
  for(i=0; i<cROUNDS; i++) SIPROUND;
  CHECK(&v0); CHECK(&v1); CHECK(&v2); CHECK(&v3); CHECK(&b);
  xor(&v0, &b); v2.lo ^= 0xff; init(&v2);
  CHECK(&v0); CHECK(&v1); CHECK(&v2); CHECK(&v3); CHECK(&b);
  for(i=0; i<dROUNDS; i++) SIPROUND;
  CHECK(&v0); CHECK(&v1); CHECK(&v2); CHECK(&v3); CHECK(&b);
  b.lo = 0; b.hi = 0; init(&b);
  xor(&b, &v0); xor(&b, &v1); xor(&b, &v2); xor(&b, &v3);
  CHECK(&v0); CHECK(&v1); CHECK(&v2); CHECK(&v3); CHECK(&b);
  for (i = 0; i < 8; i++) {
    uint *t = i > 3 ? &b.hi : &b.lo;
    out[i] = *t;
    *t >>= 8;
  }
  return 0;
}
