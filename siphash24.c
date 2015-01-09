#include <stdint.h>

/* default: SipHash-2-4 */
#define ROTL(x,b) (uint64_t)( ((x) << (b)) | ((x) >> (64 - (b))) )

#define SIPROUND                                        \
  do {                                                  \
    v0 += v1; v1=ROTL(v1,13); v1 ^= v0; v0=ROTL(v0,32); \
    v2 += v3; v3=ROTL(v3,16); v3 ^= v2;                 \
    v0 += v3; v3=ROTL(v3,21); v3 ^= v0;                 \
    v2 += v1; v1=ROTL(v1,17); v1 ^= v2; v2=ROTL(v2,32); \
  } while(0)

int  siphash( uint8_t *out, const uint8_t *in, uint64_t inlen, const uint8_t *k ) {
  /* "somepseudorandomlygeneratedbytes" */
  uint64_t v0 = ((uint64_t)0x736f6d65UL<<32) | 0x70736575UL;
  uint64_t v1 = ((uint64_t)0x646f7261UL<<32) | 0x6e646f6dUL;
  uint64_t v2 = ((uint64_t)0x6c796765UL<<32) | 0x6e657261UL;
  uint64_t v3 = ((uint64_t)0x74656462UL<<32) | 0x79746573UL;
  uint64_t b;
  uint64_t k0 = ((uint64_t)0x07060504UL<<32) | 0x03020100UL; //U8TO64_LE( k );
  uint64_t k1 = ((uint64_t)0x0F0E0D0CUL<<32) | 0x0B0A0908UL; //U8TO64_LE( k + 8 );
  uint64_t m;
  int i;
  const int cROUNDS = 2, dROUNDS = 4;

  const uint8_t *end = in + inlen - ( inlen % sizeof( uint64_t ) );
  int left = inlen & 7;
  b = ( ( uint64_t )inlen ) << 56;
  v3 ^= k1; v2 ^= k0; v1 ^= k1; v0 ^= k0;
  for (; in != end; in += 8) {
    m = 0;
    for (i = 0; i < 8; i++)
      m |= ((uint64_t)in[i]) << (8*i);
    v3 ^= m;
    for (i = 0; i < cROUNDS; i++) SIPROUND;
    v0 ^= m;
  }
  for (; left; left--)
    b |= ((uint64_t)in[left-1]) << (8*left-8);
  v3 ^= b;
  for(i=0; i<cROUNDS; i++) SIPROUND;
  v0 ^= b; v2 ^= 0xff;
  for(i=0; i<dROUNDS; i++) SIPROUND;
  b = v0 ^ v1 ^ v2  ^ v3;
  for (i = 0; i < 8; i++) {
    out[i] = (uint8_t)b;
    b >>= 8;
  }
  return 0;
}
