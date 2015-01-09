#include <stdint.h>

/* default: SipHash-2-4 */
#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x,b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define SIPROUND                                        \
  do {                                                  \
    v0 += v1; v1=ROTL(v1,13); v1 ^= v0; v0=ROTL(v0,32); \
    v2 += v3; v3=ROTL(v3,16); v3 ^= v2;                 \
    v0 += v3; v3=ROTL(v3,21); v3 ^= v0;                 \
    v2 += v1; v1=ROTL(v1,17); v1 ^= v2; v2=ROTL(v2,32); \
  } while(0)

int  siphash( uint8_t *out, const uint8_t *in, uint64_t inlen, const uint8_t *k ) {
  /* "somepseudorandomlygeneratedbytes" */
  uint64_t v0 = 0x736f6d6570736575ULL;
  uint64_t v1 = 0x646f72616e646f6dULL;
  uint64_t v2 = 0x6c7967656e657261ULL;
  uint64_t v3 = 0x7465646279746573ULL;
  uint64_t b;
  uint64_t k0 = 0x0706050403020100ULL; //U8TO64_LE( k );
  uint64_t k1 = 0x0F0E0D0C0B0A0908ULL; //U8TO64_LE( k + 8 );
  uint64_t m;
  int i;
  const uint8_t *end = in + inlen - ( inlen % sizeof( uint64_t ) );
  int left = inlen & 7;
  b = ( ( uint64_t )inlen ) << 56;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for (; in != end; in += 8) {
    m = 0;
    for (i = 0; i < 8; i++)
      m |= ((uint64_t)in[i]) << (8*i);
    v3 ^= m;
    for(i=0; i<cROUNDS; i++) SIPROUND;
    v0 ^= m;
  }
  for (; left; left--)
    b |= ((uint64_t)in[left-1]) << (8*left-8);
  v3 ^= b;
  for(i=0; i<cROUNDS; i++) SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  for(i=0; i<dROUNDS; i++) SIPROUND;
  b = v0 ^ v1 ^ v2  ^ v3;
  for (i = 0; i < 8; i++) {
    out[i] = (uint8_t)b;
    b >>= 8;
  }
  return 0;
}
