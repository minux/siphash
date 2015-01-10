/* default: SipHash-2-4 */

typedef struct { /* assume unsigned long is at least 32-bit */
  unsigned long hi;
  unsigned long lo;
} _uint64;

static inline void _rotl(_uint64 *v, int bits) { /* bits must <=32 */
  unsigned long tmp = v->hi;
  if (bits == 32) {
    v->hi = v->lo;
    v->lo = tmp;
  } else if (bits < 32) {
    v->hi = (tmp << bits) | ((0xfffffffful & v->lo) >> (32-bits));
    v->lo = (v->lo << bits) | ((0xfffffffful & tmp) >> (32-bits));
  }
}

static inline void _xor(_uint64 *dst, _uint64 *src) {
  dst->hi ^= src->hi;
  dst->lo ^= src->lo;
}

static inline void _add(_uint64 *dst, _uint64 *src) {
  dst->lo += src->lo;
  dst->hi += src->hi + ((dst->lo & 0xfffffffful) < (src->lo&0xfffffffful) ? 1:0);
}

#define SIPROUND                                        \
  do {                                                  \
    _add(&v0, &v1); _rotl(&v1, 13); _xor(&v1, &v0); _rotl(&v0, 32); \
    _add(&v2, &v3); _rotl(&v3, 16); _xor(&v3, &v2); \
    _add(&v0, &v3); _rotl(&v3, 21); _xor(&v3, &v0); \
    _add(&v2, &v1); _rotl(&v1, 17); _xor(&v1, &v2); _rotl(&v2, 32); \
  } while(0)

int siphash(unsigned char *out, const unsigned char *in, unsigned long inlen, const unsigned char *k /*unused*/) {
  /* "somepseudorandomlygeneratedbytes" */
  _uint64 v0 = {0x736f6d65UL, 0x70736575UL};
  _uint64 v1 = {0x646f7261UL, 0x6e646f6dUL};
  _uint64 v2 = {0x6c796765UL, 0x6e657261UL};
  _uint64 v3 = {0x74656462UL, 0x79746573UL};
  _uint64 b;
  /* hard-coded k. */
  _uint64 k0 = {0x07060504UL, 0x03020100UL}; /* U8TO64_LE(k); */
  _uint64 k1 = {0x0F0E0D0CUL, 0x0B0A0908UL}; /* U8TO64_LE(k + 8); */
  int i;
  const int cROUNDS = 2, dROUNDS = 4;
  const unsigned char *end = in + inlen - (inlen % 8);
  int left = inlen & 7;
  _xor(&v3, &k1); _xor(&v2, &k0); _xor(&v1, &k1); _xor(&v0, &k0);
  for (; in != end; in += 8) {
    b.hi = 0; b.lo = 0;
    for (i = 0; i < 4; i++) b.lo |= ((unsigned long)in[i]) << (8*i);
    for (i = 0; i < 4; i++) b.hi |= ((unsigned long)in[i+4]) << (8*i);
    _xor(&v3, &b);
    for (i = 0; i < cROUNDS; i++) SIPROUND;
    _xor(&v0, &b);
  }
  b.hi = (inlen & 0xff)<<24; b.lo = 0;
  for (; left; left--)
    if (left > 4)
      b.hi |= ((unsigned long)in[left-1]) << (8*left-8-32);
    else
      b.lo |= ((unsigned long)in[left-1]) << (8*left-8);
  _xor(&v3, &b);
  for(i=0; i<cROUNDS; i++) SIPROUND;
  _xor(&v0, &b); v2.lo ^= 0xff;
  for(i=0; i<dROUNDS; i++) SIPROUND;
  b.lo = 0; b.hi = 0;
  _xor(&b, &v0); _xor(&b, &v1); _xor(&b, &v2); _xor(&b, &v3);
  for (i = 0; i < 8; i++) {
    unsigned long *t = i > 3 ? &b.hi : &b.lo;
    out[i] = *t;
    *t >>= 8;
  }
  return 0;
}
