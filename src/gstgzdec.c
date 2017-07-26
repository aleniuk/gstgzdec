/* GStreamer bz2 decoder
 * Copyright (C) 2006 Lutz Müller <lutz topfrose de>
 * Copyright (C) 2017 Aleksandr Slobodeniuk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:element-gstgzdec
 *
 * The gsty4mdec element decodes uncompressed video in YUV4MPEG format.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v filesrc location=file.y4m ! y4mdec ! xvimagesink
 * ]|
 * </refsect2>
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gstgzdec.h"

//#include "gst/base/gsttypefindhelper.h"

#include "zip-dec-wrapper.h"
#include "string.h"

GST_DEBUG_CATEGORY_STATIC (gzdec_debug);
#define GST_CAT_DEFAULT gzdec_debug

static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY"/*"application/x-bzip"*/));
static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

struct _GstGzdec
{
  GstElement parent;

  GstPad *sink;
  GstPad *src;

  /* Properties */
  guint first_buffer_size;
  guint buffer_size;

  gboolean ready;
  guint64 offset;

  z_dec * dec;
};

struct _GstGzdecClass
{
  GstElementClass parent_class;
};

#ifdef GST_10
#define gst_gzdec_parent_class parent_class
G_DEFINE_TYPE (GstGzdec, gst_gzdec, GST_TYPE_ELEMENT);
#else
GST_BOILERPLATE (GstGzdec, gst_gzdec, GstElement, GST_TYPE_ELEMENT);
#endif

#define DEFAULT_FIRST_BUFFER_SIZE 1024
#define DEFAULT_BUFFER_SIZE 1024

enum
{
  PROP_0,
  PROP_FIRST_BUFFER_SIZE,
  PROP_BUFFER_SIZE
};


static GstFlowReturn
gst_gzdec_chain (GstPad * pad, GstBuffer * in)
{
  GstFlowReturn flow;
  GstBuffer *out;
  GstGzdec *b;
  int r = 0;
  unsigned avail_in, avail_out;
  gpointer next_in, next_out;
  
#ifdef GST_10
  GstMapInfo map = GST_MAP_INFO_INIT, omap;
#endif
  
  b = GST_GZDEC (GST_PAD_PARENT (pad));

  if (!b->ready)
    goto not_ready;

  next_in = (char *) GST_BUFFER_DATA (in);
  avail_in = GST_BUFFER_SIZE (in);

  do {
    guint n;

    /* Create the output buffer */
    flow = gst_pad_alloc_buffer (b->src, b->offset,
        b->offset ? b->buffer_size : b->first_buffer_size,
        GST_PAD_CAPS (b->src), &out);

    if (flow != GST_FLOW_OK) {
      GST_DEBUG_OBJECT (b, "pad alloc failed: %s", gst_flow_get_name (flow));
      z_dec_free(&b->dec);
      break;
    }

    /* Decode */
    next_out = (char *) GST_BUFFER_DATA (out);
    avail_out = GST_BUFFER_SIZE (out);
    
    r = z_dec_decode
      (b->dec, next_in, &avail_in, next_out, &avail_out);
    if (r)
      goto decode_failed;

    if (avail_out >= GST_BUFFER_SIZE (out)) {
      gst_buffer_unref (out);
      break;
    }
    GST_BUFFER_SIZE (out) -= avail_out;
    // GST_BUFFER_OFFSET (out) = b->stream.total_out_lo32 - GST_BUFFER_SIZE (out);

    /* Configure source pad (if necessary) */
    if (!b->offset) {
      GstCaps *caps = NULL;

      //caps = gst_type_find_helper_for_buffer (GST_OBJECT (b), out, NULL);
      if (caps) {
#ifndef GST_10
        gst_buffer_set_caps (out, caps);
#endif
        gst_pad_set_caps (b->src, caps);
        gst_pad_use_fixed_caps (b->src);
        gst_caps_unref (caps);
      } else {
        /* FIXME: shouldn't we queue output buffers until we have a type? */
      }
    }

    /* Push data */
    n = GST_BUFFER_SIZE (out); // gst_buffer_get_size
    flow = gst_pad_push (b->src, out);
    if (flow != GST_FLOW_OK)
      break;
    b->offset += n;
  } while (avail_out);

done:
#ifdef GST_10
  gst_buffer_unmap (in, &map);
#endif
  gst_buffer_unref (in);
  return flow;

/* ERRORS */
decode_failed:
  {
    GST_ELEMENT_ERROR (b, STREAM, DECODE, (NULL),
        ("Failed to decompress data (error code %i).", r));
    z_dec_free(&b->dec);
    gst_buffer_unref (out);
    flow = GST_FLOW_ERROR;
    goto done;
  }
not_ready:
  {
    GST_ELEMENT_ERROR (b, LIBRARY, FAILED, (NULL), ("Decompressor not ready."));
    flow = GST_FLOW_WRONG_STATE;
    goto done;
  }
}

static void
gst_gzdec_init (GstGzdec * b, GstGzdecClass * klass)
{
  b->first_buffer_size = DEFAULT_FIRST_BUFFER_SIZE;
  b->buffer_size = DEFAULT_BUFFER_SIZE;

  b->sink = gst_pad_new_from_static_template
    (&sink_template, "sink");
  gst_pad_set_chain_function
    (b->sink, GST_DEBUG_FUNCPTR (gst_gzdec_chain));
  gst_element_add_pad (GST_ELEMENT (b), b->sink);

  b->src = gst_pad_new_from_static_template
    (&src_template, "src");
  gst_element_add_pad (GST_ELEMENT (b), b->src);
  gst_pad_use_fixed_caps (b->src);

  b->dec = NULL;
  b->offset = 0;  
}

static void
gst_gzdec_base_init (gpointer g_class)
{
  GstElementClass *ec = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (ec, &sink_template);
  gst_element_class_add_static_pad_template (ec, &src_template);
  gst_element_class_set_details_simple (ec, "GZ and GZ decoder",
      "Codec/Decoder", "Decodes compressed streams",
      "Aleksandr Slobodeniuk <alenuke at yandex dot ru>");
}

static void
gst_gzdec_finalize (GObject * object)
{
  GstGzdec *b = GST_GZDEC (object);

  z_dec_free(&b->dec);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_gzdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGzdec *b = GST_GZDEC (object);

  switch (prop_id) {
    case PROP_BUFFER_SIZE:
      g_value_set_uint (value, b->buffer_size);
      break;
    case PROP_FIRST_BUFFER_SIZE:
      g_value_set_uint (value, b->first_buffer_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
gst_gzdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGzdec *b = GST_GZDEC (object);

  switch (prop_id) {
    case PROP_BUFFER_SIZE:
      b->buffer_size = g_value_get_uint (value);
      break;
    case PROP_FIRST_BUFFER_SIZE:
      b->first_buffer_size = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static GstStateChangeReturn
gst_gzdec_change_state (GstElement * element, GstStateChange transition)
{
  GstGzdec *b = GST_GZDEC (element);
  GstStateChangeReturn ret;

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      //gst_gzdec_decompress_init (b);
      break;
    default:
      break;
  }
  return ret;
}

static void
gst_gzdec_class_init (GstGzdecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gstelement_class->change_state =
    GST_DEBUG_FUNCPTR (gst_gzdec_change_state);

  gobject_class->finalize = gst_gzdec_finalize;
  gobject_class->get_property = gst_gzdec_get_property;
  gobject_class->set_property = gst_gzdec_set_property;

  g_object_class_install_property
    (G_OBJECT_CLASS (klass),
      PROP_FIRST_BUFFER_SIZE,
     g_param_spec_uint
     ("first-buffer-size",
      "Size of first buffer",
      "Size of first buffer (used to determine the "
          "mime type of the uncompressed data)", 1, G_MAXUINT,
      DEFAULT_FIRST_BUFFER_SIZE,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property
    (G_OBJECT_CLASS (klass), PROP_BUFFER_SIZE,
     g_param_spec_uint
     ("buffer-size", "Buffer size", "Buffer size",
          1, G_MAXUINT, DEFAULT_BUFFER_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

#ifdef GST_10
  gst_element_class_add_static_pad_template
    (gstelement_class, &sink_template);
   gst_element_class_add_static_pad_template
     (gstelement_class, &src_template);
   gst_element_class_set_static_metadata
     (gstelement_class, "GZ decoder",
       "Codec/Decoder", "Decodes compressed streams",
       "Aleksandr Slobodeniuk");
#endif

  GST_DEBUG_CATEGORY_INIT (gzdec_debug, "gzdec", 0, "GZ decompressor");
}


static gboolean
plugin_init (GstPlugin * p)
{
  if (!gst_element_register
      (p, "gzdec", GST_RANK_NONE, GST_TYPE_GZDEC))
    return FALSE;
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, "gzdec",
    "Compress or decompress streams",
    plugin_init, "0.0", "LGPL", "gzdec", "gz")
