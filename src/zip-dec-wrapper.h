/* Copyright (C) 2017 Aleksandr Slobodeniuk
 *
 *   This file is part of gstgzdec.
 *
 *   Gstgzdec is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Gstgzdec is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with gstgzdec.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZIPWRAP
#define __ZIPWRAP

typedef enum {
  Z_UNKNOWN = 0,
  Z_BZIP2,
  Z_GZIP
} z_type;

// Probe data to get zip format.
// return value can be Z_BZIP2, Z_GZIP or Z_UNKNOWN.
// WARNING: stream MUST have 2 bytes available,
// or we'll get SEGFAULT
z_type probe_stream(const unsigned short * stream);

typedef struct z_decoder_s z_dec; // class

// Decoder's constructor.
// Type may be Z_BZIP2 or Z_GZIP,
// otherwize initialization will fail.
z_dec * z_dec_alloc(z_type type); // constructor

// Decoder's destructor.
// Totally cleanups all resources,
// and then sets pointer to NULL;
void z_dec_free(z_dec **); // destructor

// Decode known bz2 or gz stream.
// Next_in/next_out -- input/output buffers
// Avail_in and Avail_out must be set before call.
// They mean number of bytes in next_in
// and the size of free bytes in next_out.
// After call avail_in means "how many bytes with data
// in next_in still need to be decoded", and avail_out
// means "how many free bytes (with no data) avail_out
// has now".
// Function returns 0 if suceeded, or -1 on fail.
//
// Known bugs: may fail if avail_out is not enough for
// decompressed avail_in.
int z_dec_decode
(z_dec *,
 const void * next_in, unsigned * avail_in,
 void * next_out, unsigned * avail_out);

#endif // __ZIPWRAP
