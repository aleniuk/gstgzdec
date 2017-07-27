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

// Decode known stream.
int z_dec_decode
(z_dec *,
 const void * next_in, unsigned * avail_in,
 void * next_out, unsigned * avail_out);

#endif // __ZIPWRAP
