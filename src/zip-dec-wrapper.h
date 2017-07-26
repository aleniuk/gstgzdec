typedef enum {
  Z_UNNOWN = 0,
  Z_BZIP2,
  Z_GZIP
} z_type;

typedef struct z_decoder_s z_dec;

z_dec * z_dec_alloc(z_type type);
void z_dec_free(z_dec **);

int z_dec_decode
(z_dec *,
 void * next_in, unsigned * avail_in,
 void * next_out, unsigned * avail_out);

