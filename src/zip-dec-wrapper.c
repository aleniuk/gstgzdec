#include "zip-dec-wrapper.h"

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
	  BZ2_bzDecompressEnd((bz_stream *)&dec->stream_context);
	  break;
	case Z_GZIP:
	  inflateEnd((z_stream *)&dec->stream_context);
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
    ret = BZ2_bzDecompressInit (&dec->stream_context, 0, 0);
    ZCK(ret == BZ_OK);
    break;

  case Z_GZIP:
    dec->stream_context = calloc(1, sizeof(z_stream));
    ZCK(dec->stream_context);
    dec->dec_func = decode_gzip;
    ret = inflateInit2
      (&dec->stream_context, ZLIB_INFLATE_WINDOW_BITS);
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


