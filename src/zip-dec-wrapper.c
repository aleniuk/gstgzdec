#include "zip-dec-wrapper.h"

#include "stdlib.h"// calloc, free

#include "bzlib.h"
#include "zlib.h"

static const int ZLIB_INFLATE_WINDOW_BITS = 32;

struct z_decoder_s
{
  z_type type;
  void * stream_context;

  int (* dec_func)
  (void * stream_context,
   void * next_in, unsigned * avail_in,
   void * next_out, unsigned * avail_out);
};

// global CHECK macro
#define ZCK(statement)				\
  do {						\
    if (!(statement)) goto error;		\
  } while(0)

// just for sugar
#define NON_API static


NON_API int decode_gzip
(void * ctx,
 void * next_in, unsigned * avail_in,
 void * next_out, unsigned * avail_out)
{
  z_stream * z_ctx;
  int ret;
  
  ZCK(ctx &&
      next_in &&
      avail_in &&
      next_out &&
      avail_out);

  z_ctx = (z_stream *)ctx;

  z_ctx->next_in = next_in;
  z_ctx->avail_in = *avail_in;

  z_ctx->next_out = next_out;
  z_ctx->avail_out = *avail_out;

  ret = inflate(z_ctx, Z_NO_FLUSH);

  if (ret == Z_OK || ret == Z_STREAM_END)
  {
    // update in/out buffer size states
    *avail_in = z_ctx->avail_in;
    *avail_out = z_ctx->avail_out;

    if (ret == Z_STREAM_END)
      return 0;
    else
      return 0;
  }
  
 error:
  return -1;
}

NON_API int decode_bzip2
(void * ctx,
 void * next_in, unsigned * avail_in,
 void * next_out, unsigned * avail_out)
{
  bz_stream * bz_ctx;
  int ret;
  
  ZCK(ctx &&
      next_in &&
      avail_in &&
      next_out &&
      avail_out);

  bz_ctx = (bz_stream *)ctx;

  bz_ctx->next_in = next_in;
  bz_ctx->avail_in = *avail_in;

  bz_ctx->next_out = next_out;
  bz_ctx->avail_out = *avail_out;

  ret = BZ2_bzDecompress(bz_ctx);

  if (ret == BZ_OK || ret == BZ_STREAM_END)
  {
    // update in/out buffer size states
    *avail_in = bz_ctx->avail_in;
    *avail_out = bz_ctx->avail_out;

    if (ret == BZ_STREAM_END)
      return 0;
    else
      return 0;
  }
 error:
  return -1;
}



// totally clear everything,
// and uninitialize to NULL;
void z_dec_free(z_dec ** dec_p)
{
  if (dec_p) {
    if (*dec_p) {
      z_dec * dec = *dec_p;
      if (dec->stream_context) {

	// after calloc dec->type == Z_UNKNOWN == 0
	// so, we can be sure here
	switch (dec->type) {
	case Z_BZIP2:
	  BZ2_bzDecompressEnd((bz_stream *)dec->stream_context);
	  break;
	case Z_GZIP:
	  inflateEnd((z_stream *)dec->stream_context);
	  break;
	default:
	  // or clang compiler warns
	  break;
	}
	free(dec->stream_context);
	dec->stream_context = NULL;
      }
	
      free(dec);
    }
    *dec_p = NULL;
  }
}

// allocate the context or return NULL
z_dec * z_dec_alloc(z_type type)
{
  int ret;
  z_dec * dec = calloc(1, sizeof(z_dec));
  ZCK(dec);
  switch (type) {

  case Z_BZIP2:
    dec->stream_context = calloc(1, sizeof(bz_stream));
    ZCK(dec->stream_context);
    dec->dec_func = decode_bzip2;
    ret = BZ2_bzDecompressInit
      ((bz_stream *)dec->stream_context, 0, 0);
    ZCK(ret == BZ_OK);
    break;

  case Z_GZIP:
    dec->stream_context = calloc(1, sizeof(z_stream));
    ZCK(dec->stream_context);
    dec->dec_func = decode_gzip;
    ret = inflateInit2
      ((z_stream *)dec->stream_context,
       ZLIB_INFLATE_WINDOW_BITS);
    ZCK(ret == Z_OK);
    break;

  default:
    ZCK(0);
  }

  dec->type = type;

  // sucess
  return dec;
  // fail
 error:
  z_dec_free(&dec);
  return NULL;
}


// decode wrapper
int z_dec_decode(z_dec * dec,
 void * next_in, unsigned * avail_in,
 void * next_out, unsigned * avail_out)
{
  ZCK(dec &&
      next_in &&
      avail_in &&
      next_out &&
      avail_out &&
      dec->dec_func);

  return dec->dec_func(dec->stream_context,
		       next_in, avail_in,
		       next_out, avail_out);
  
 error:
  return -1;
}


#ifdef TEST

// compile:
// clang zip-dec-wrapper.c -DTEST $(pkg-config --libs zlib) -lbz2
//
// run:
// ./a.out infile.gz outfile type
// type : 1 == bz
//        2 == gz
//
// check:
// bzip2 -k zip-dec-wrapper.c
// ./a.out zip-dec-wrapper.c.bz2 test.c 1
// cmp test.c zip-dec-wrapper.c

#include "stdlib.h"
#include "stdio.h"

int main(int argc, char ** argv)
{
  int err = -1;
  if (argc > 3) {

    FILE
      * in = NULL,
      * out = NULL;
    char
      * inbuf = NULL,
      * outbuf = NULL;
    unsigned
      avail_in = 0,
      avail_out = 0;
    const unsigned
      segsize = 1024 * 1024;
    z_dec * dec = NULL;

    dec = z_dec_alloc(atoi(argv[3]));
    in = fopen(argv[1], "rb");
    out = fopen(argv[2], "wb");
    inbuf = malloc(segsize);
    outbuf = malloc(segsize);

    fseek(in, 0, SEEK_SET);

    if (dec && in && out && inbuf && outbuf) {
      for (;;) {
	int last_read_size =
	avail_in = fread(inbuf, 1, 1024, in);
	if (avail_in <= 0)
	  goto finish;
	
	// decode all we have
	while (avail_in) {

	  const unsigned offset = last_read_size - avail_in;

	  avail_out = segsize; // set output buffer size
	  err = z_dec_decode
	    (dec, inbuf + offset, &avail_in,
	     outbuf, &avail_out);
	  if (err)
	    goto finish;

	  // write all we have
	  if (avail_out) {
	    const unsigned bytes_to_write = segsize - avail_out; 
	    int written = fwrite(outbuf, 1, bytes_to_write, out);
	    if (written != (bytes_to_write)) {
	      err = -1;
	      goto finish;
	    }
	    avail_out = 0;
	  }
	}
	
      }
    }

  finish:
    if (inbuf)
      free(inbuf);
    if (outbuf)
      free(outbuf);
    if (in)
      fclose(in);
    if (out)
      fclose(out);
    if (dec)
      z_dec_free(&dec);
  }

  return err;
}

#endif
