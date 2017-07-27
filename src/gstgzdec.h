/* GStreamer gz/bz2 decoder
 * Copyright (C) 2017 Aleksandr Slobodeniuk
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
#ifndef __GST_GZDEC_H__
#define __GST_GZDEC_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_GZDEC \
  (gst_gzdec_get_type())
#define GST_GZDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GZDEC,GstGzdec))
#define GST_GZDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GZDEC,GstGzdecClass))
#define GST_IS_GZDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GZDEC))
#define GST_IS_GZDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GZDEC))

typedef struct _GstGzdec      GstGzdec;
typedef struct _GstGzdecClass GstGzdecClass;

GType gst_gzdec_get_type (void);

G_END_DECLS

#endif /* __GST_GZDEC_H__ */
