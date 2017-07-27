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
 * gst-launch -v filesrc location=file.bz2 ! gzdec ! filesink location=file2
 * ]|
 * </refsect2>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gstgzdec.h"
#include "zip-dec-wrapper.h"
#include "string.h"

#if GST_VERSION_MAJOR >= 1
#define GST_10 1
#endif

GST_DEBUG_CATEGORY_STATIC (gzdec_debug);
#define GST_CAT_DEFAULT gzdec_debug

// how about
// "application/x-bzip"
// "application/gzip"

static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE
  ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
   GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE
  ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
   GST_STATIC_CAPS_ANY);

struct _GstGzdec
{
  GstElement parent;
  GstPad *sinkpad;
  GstPad *srcpad;
  guint buffer_size;
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

const unsigned DEFAULT_BUFFER_SIZE = 4 * 1024 * 1024;

enum
{
  PROP_0,
  PROP_BUFFER_SIZE
};


static GstFlowReturn
gst_gzdec_chain (GstPad * pad,
#ifdef GST_10
		 GstObject * parent,
#endif
		 GstBuffer * in)
{
  GstFlowReturn flow = GST_FLOW_OK;
  GstBuffer *out = NULL;
  GstGzdec *b;
  int r = 0;
  unsigned avail_in, avail_out;
  gchar * next_in, *next_out;

  // defining our 'error check lambda'
#define GZDEC_ERR(code, message) do {				\
    GST_ELEMENT_ERROR (b, STREAM, code, (NULL), (message));	\
    flow = GST_FLOW_ERROR;					\
    goto done; } while(0)
	

  
#ifdef GST_10
  GstMapInfo map = GST_MAP_INFO_INIT, omap;
#endif
  
  b = GST_GZDEC (GST_PAD_PARENT (pad));

#ifdef GST_10
  gst_buffer_map (in, &map, GST_MAP_READ);
  next_in = (gchar *)map.data;
  avail_in = map.size;
#else
  next_in = (gchar *) GST_BUFFER_DATA (in);
  avail_in = GST_BUFFER_SIZE (in);
#endif
  
  while (avail_in) {
    guint allocated_out_buffer_size;
    guint bytes_to_write;
#ifdef GST_10
    out = gst_buffer_new_and_alloc (b->buffer_size);
    if (!out)
      GZDEC_ERR(FAILED, "Buffer allocation failed");
#else
    flow = gst_pad_alloc_buffer
      (b->srcpad, GST_BUFFER_OFFSET_NONE,
       b->buffer_size, GST_PAD_CAPS (b->srcpad), &out);

    if (flow != GST_FLOW_OK) {
      z_dec_free(&b->dec);
      GST_DEBUG_OBJECT
	(b, "pad buffer allocation failed: %s",
	 gst_flow_get_name (flow));
      goto done;
    }
#endif

    /* Decode */
#ifdef GST_10
    gst_buffer_map (out, &omap, GST_MAP_WRITE);
    next_out = (gchar *)omap.data;
    avail_out = omap.size;
#else
    next_out = (gchar *) GST_BUFFER_DATA (out);
    avail_out = GST_BUFFER_SIZE (out);
#endif
    allocated_out_buffer_size = avail_out;
      
    // if we're feeding the first buffer,
    // probe stream type and initialize decoder
    if (!b->dec) {
      if (avail_out < 2)
	GZDEC_ERR
	  (FAILED, "The first provided buffer's size "
	   "is > 0 and < 2 bytes at the same time."
	   "Decoder can't probe the stream and initialize"
	   "in this case.");

      const z_type zip_stream_type =
	probe_stream((const unsigned short *)next_in);
      b->dec = z_dec_alloc(zip_stream_type);
      if (!b->dec) {
	switch (zip_stream_type) {
	case Z_UNNOWN:
	  GZDEC_ERR(CODEC_NOT_FOUND, "This is neither bz2 or gz stream");
	default:
	  GZDEC_ERR(FAILED, "Decoder's initialization returned an error");
	}
      }
    }
    
    r = z_dec_decode
      (b->dec, next_in, &avail_in, next_out, &avail_out);
#ifdef GST_10
    gst_buffer_unmap (out, &omap);
#endif
    if (r) {
      z_dec_free(&b->dec);	    
      GZDEC_ERR(DECODE, "decoding failed");
    }

    bytes_to_write = allocated_out_buffer_size - avail_out;

    if (bytes_to_write) {

#ifdef GST_10
      // out map's size changing?
      gst_buffer_resize (out, 0, bytes_to_write);
#else
      GST_BUFFER_SIZE (out) = bytes_to_write;
#endif

      // Pushing data downstream.
      // 1. Increasing it's refcount, so downstream element could safely unref it.
      // 2. Pushing buffer to the pad.
      out = gst_buffer_ref(out); // 1.
      flow = gst_pad_push (b->srcpad, out); // 2.
      if (flow != GST_FLOW_OK)
	break;
    }
  }

done:
#ifdef GST_10
  gst_buffer_unmap (in, &map);
#endif
  if(out)
    gst_buffer_unref(out);
  gst_buffer_unref (in);
  return flow;
}

static void
gst_gzdec_init (GstGzdec * b
#ifndef GST_10
		, GstGzdecClass * klass
#endif
		)
{
  b->buffer_size = DEFAULT_BUFFER_SIZE;

  b->sinkpad = gst_pad_new_from_static_template
    (&sink_template, "sink");
  gst_pad_set_chain_function
    (b->sinkpad, GST_DEBUG_FUNCPTR (gst_gzdec_chain));
  gst_element_add_pad (GST_ELEMENT (b), b->sinkpad);

  b->srcpad = gst_pad_new_from_static_template
    (&src_template, "src");
  gst_element_add_pad (GST_ELEMENT (b), b->srcpad);
  gst_pad_use_fixed_caps (b->srcpad);

  b->dec = NULL;
}

#ifndef GST_10
static void
gst_gzdec_base_init (gpointer g_class)
{
  GstElementClass *ec = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (ec, &sink_template);
  gst_element_class_add_static_pad_template (ec, &src_template);
  gst_element_class_set_details_simple (ec, "GZ/BZ2 decoder",
      "Codec/Decoder", "Decodes compressed streams",
      "Aleksandr Slobodeniuk <alenuke@yandex.ru>");
}
#endif

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

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      // so we could start again
      z_dec_free(&b->dec);
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
     (gstelement_class, "GZ/BZ2 decoder",
       "Codec/Decoder", "Decodes compressed streams",
       "Aleksandr Slobodeniuk");
#endif

  GST_DEBUG_CATEGORY_INIT (gzdec_debug, "gzdec", 0, "GZ/BZ2 decompressor");
}


static gboolean
plugin_init (GstPlugin * p)
{
  if (!gst_element_register
      (p, "gzdec", GST_RANK_NONE, GST_TYPE_GZDEC))
    return FALSE;
  return TRUE;
}

GST_PLUGIN_DEFINE
(GST_VERSION_MAJOR, GST_VERSION_MINOR,
#ifdef GST_10
 gzdec,
#else
 "gzdec",
#endif
    "Decompress bz2 and gz streams",
    plugin_init, "0.0", "GPL", "gzdec",
    "https://github.com/aleniuk/gstgzdec.git")
