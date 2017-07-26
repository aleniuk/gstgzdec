

#ifndef __ZIPWRAP
#define __ZIPWRAP

typedef enum {
  Z_UNNOWN = 0,
  Z_BZIP2,
  Z_GZIP
} z_type;

// probe data to get zip format
// WARNING: stream MUST have 2 bytes available,
// or we'll get SEGFAULT
z_type probe_stream(const unsigned short * stream);

typedef struct z_decoder_s z_dec; // class
z_dec * z_dec_alloc(z_type type); // constructor
void z_dec_free(z_dec **); // destructor

// decode known stream
int z_dec_decode
(z_dec *,
 const void * next_in, unsigned * avail_in,
 void * next_out, unsigned * avail_out);

#endif // __ZIPWRAP
