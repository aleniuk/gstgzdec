/* GStreamer gz/bz2 decoder
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
 * The gstgzdec element decodes gz and bz compressed streams
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v filesrc location=file.gz ! gzdec ! filesink location=file2
 *
 * gst-launch -v filesrc location=file.bz ! gzdec ! filesink location=file2
 * ]|
 * </refsect2>
 */
//#define GST_10 1


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gstgzdec.h"

#include "zip-dec-wrapper.h"
#include "string.h"

GST_DEBUG_CATEGORY_STATIC (gzdec_debug);
#define GST_CAT_DEFAULT gzdec_debug

// GST_STATIC_CAPS("application/unknown")
// how about
// "application/x-bzip"
// "application/gzip"


// how about zip stream ?

static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

struct _GstGzdec
{
  GstElement parent;

  GstPad *sink;
  GstPad *src;

  guint buffer_size;

  guint64 offset; // it's ok for files, but what about endless stream?

  z_dec * dec;
};

struct _GstGzdecClass
{
  GstElementClass parent_class;
};

#ifdef GST_10
#define gst_gzdec_parent_class parent_class
G_DEFINE_TYPE (GstGzdec, GstGzdec, GST_TYPE_ELEMENT);
#else
GST_BOILERPLATE (GstGzdec, gst_gzdec, GstElement, GST_TYPE_ELEMENT);
#endif

const unsigned DEFAULT_BUFFER_SIZE = 4 * 1024;

enum
{
  PROP_0,
  PROP_BUFFER_SIZE
};


static GstFlowReturn
gst_gzdec_chain (GstPad * pad, GstBuffer * in)
{
  GstFlowReturn flow = GST_FLOW_OK;
  GstBuffer *out;
  GstGzdec *b;
  int r = 0;
  unsigned avail_in, avail_out;
  gpointer next_in, next_out;
  
#ifdef GST_10
  GstMapInfo map = GST_MAP_INFO_INIT, omap;
#endif
  
  b = GST_GZDEC (GST_PAD_PARENT (pad));

  next_in = (char *) GST_BUFFER_DATA (in);
  avail_in = GST_BUFFER_SIZE (in);

  while (avail_in) {
    guint allocated_out_buffer_size;
    guint bytes_to_write;

    // ret = gst_buffer_pool_acquire_buffer (dvdec->pool, &outbuf, NULL);

    
    /* Create the output buffer */
    flow = gst_pad_alloc_buffer (b->src, b->offset,
				 b->buffer_size,
				 GST_PAD_CAPS (b->src), &out);

    if (flow != GST_FLOW_OK) {
      GST_DEBUG_OBJECT (b, "pad alloc failed: %s", gst_flow_get_name (flow));
      z_dec_free(&b->dec);
      break;
    }

    /* Decode */
    next_out = (char *) GST_BUFFER_DATA (out);
    allocated_out_buffer_size =
      avail_out = GST_BUFFER_SIZE (out);

    // if we're feeding the first buffer,
    // probe stream type and initialize decoder
    if (!b->dec) {
      if (avail_out < 2)
	goto fail;
      z_type zip_stream_type = probe_stream(next_in);
      b->dec = z_dec_alloc(zip_stream_type);
      if (!b->dec)
	goto fail;
    }
    
    r = z_dec_decode
      (b->dec, next_in, &avail_in, next_out, &avail_out);
    if (r)
      goto fail;

    bytes_to_write = allocated_out_buffer_size - avail_out;

    
    if (bytes_to_write) {
    
      if (bytes_to_write >= GST_BUFFER_SIZE (out)) {
	gst_buffer_unref (out);
	break;
      }
#ifdef GST_10
      gst_buffer_resize (out, 0, bytes_to_write);
#else
      GST_BUFFER_SIZE (out) = bytes_to_write;
#endif
      GST_BUFFER_OFFSET (out) = b->offset;

      /* Push data */
      b->offset += GST_BUFFER_SIZE (out); // offset grows. that's BAD
      flow = gst_pad_push (b->src, out);
      if (flow != GST_FLOW_OK)
	break;
    }
  }

done:
#ifdef GST_10
  gst_buffer_unmap (in, &map);
#endif
  gst_buffer_unref (in);
  return flow;

fail:
  {
    GST_ELEMENT_ERROR (b, STREAM, DECODE, (NULL),
        ("Decompression failed"));
    z_dec_free(&b->dec);
    gst_buffer_unref (out);
    flow = GST_FLOW_ERROR;
    goto done;
  }
}

static void
gst_gzdec_init (GstGzdec * b, GstGzdecClass * klass)
{
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
