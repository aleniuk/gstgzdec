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
 * The gstgzdec element decodes gz and bz2 compressed streams
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
#include "zip-dec-wrapper.h"
#include <gst/base/gsttypefindhelper.h>

GType gst_gzdec_get_type (void);
typedef struct _GstGzdec      GstGzdec;
typedef struct _GstGzdecClass GstGzdecClass;

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

// detect if headers are from 1.x
#if GST_VERSION_MAJOR >= 1
#define GST_1_0 1
#endif

GST_DEBUG_CATEGORY_STATIC (gzdec_debug);
#define GST_CAT_DEFAULT gzdec_debug

static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE
    ("sink", GST_PAD_SINK,
     GST_PAD_ALWAYS,
     GST_STATIC_CAPS
     ("application/x-bzip;application/x-gzip"));

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS_ANY);

typedef struct _GstGzdec
{
    GstElement parent;
    GstPad *sinkpad;
    GstPad *srcpad;
    guint buffer_size;
    z_dec *dec;
    gboolean typefind;
} _GstGzdec;

typedef struct _GstGzdecClass
{
    GstElementClass parent_class;
} GstGzdecClass;

#ifdef GST_1_0
#define gst_gzdec_parent_class parent_class
G_DEFINE_TYPE (GstGzdec, gst_gzdec, GST_TYPE_ELEMENT);
#else
GST_BOILERPLATE (GstGzdec, gst_gzdec, GstElement,
                 GST_TYPE_ELEMENT);
#endif

// bzip2 may fail if output buffer is not enough.
// Probably it can be fixed with some additional settings.
const unsigned DEFAULT_BUFFER_SIZE = 4 * 1024 * 1024;

enum {
    PROP_0,
    PROP_BUFFER_SIZE
};


static GstFlowReturn
gst_gzdec_chain (GstPad * pad,
#ifdef GST_1_0
		 GstObject * parent,
#endif
		 GstBuffer * in_buf)
{
    GstFlowReturn flow = GST_FLOW_OK;
    GstBuffer *out_buf = NULL;
    int r = 0;
    unsigned avail_in, avail_out;
    gchar *next_in, *next_out;
    gboolean buffer_pushed;

    // defining our 'error check lambda'
#define GZDEC_ERR(code, message) do {                                   \
        GST_ELEMENT_ERROR (gz, STREAM, code, (NULL), (message));	\
        flow = GST_FLOW_ERROR;                                          \
        goto done; } while(0)
	  
    GstGzdec *gz = GST_GZDEC (GST_PAD_PARENT (pad));

    // get sets from the input buffer
#ifdef GST_1_0
    GstMapInfo in_map = GST_MAP_INFO_INIT,
                        out_map = GST_MAP_INFO_INIT;
    gst_buffer_map (in_buf, &in_map, GST_MAP_READ);
    next_in = (gchar *)in_map.data;
    avail_in = in_map.size;
#else
    next_in = (gchar *) GST_BUFFER_DATA (in_buf);
    avail_in = GST_BUFFER_SIZE (in_buf);
#endif

    // avail out can still have data
    while (avail_in) {
        guint allocated_out_buffer_size;
        guint bytes_to_write;

        // Getting the output buffer.
        // No, I don't think it is a good solution,
        // to allocate buffer on each step.
        // I tried to figure out, how other plugins
        // manage their output buffers, but found only
        // 'stream-specific' instruments, like buffer_pool.
        // I would make ring of buffers, but Gstreamer's documentation
        // warns about gst_buffer_ref because it may cause unnecessary memcpys.
        // After all, I had to get get things work in a short time..
#ifdef GST_1_0
        out_buf = gst_buffer_new_and_alloc (gz->buffer_size);
        if (!out_buf)
            GZDEC_ERR(FAILED, "Buffer allocation failed");
#else
        flow = gst_pad_alloc_buffer(gz->srcpad, GST_BUFFER_OFFSET_NONE,
                                    gz->buffer_size, GST_PAD_CAPS (gz->srcpad), &out_buf);

        if (flow != GST_FLOW_OK) {
            z_dec_free(&gz->dec);
            GST_DEBUG_OBJECT(gz, "pad buffer allocation failed: %s",
                             gst_flow_get_name (flow));
            goto done;
        }
#endif
        buffer_pushed = FALSE;

        /* Decode */
#ifdef GST_1_0
        gst_buffer_map (out_buf, &out_map, GST_MAP_WRITE);
        next_out = (gchar *)out_map.data;
        avail_out = out_map.size;
#else
        next_out = (gchar *) GST_BUFFER_DATA (out_buf);
        avail_out = GST_BUFFER_SIZE (out_buf);
#endif
    
        allocated_out_buffer_size = avail_out;
      
        // if we're feeding the first buffer,
        // probe stream type and initialize decoder
        if (!gz->dec) {
            if (avail_out < 2)
                GZDEC_ERR(FAILED, "The first provided buffer's size "
                          "is > 0 and < 2 bytes at the same time."
                          "Decoder can't probe the stream and initialize"
                          "in this case.");

            const z_type zip_stream_type =
                probe_stream((const unsigned short *)next_in);
            if (zip_stream_type == Z_UNKNOWN)
                GZDEC_ERR(CODEC_NOT_FOUND, "This is neither bz2 or gz stream");
            
            gz->dec = z_dec_alloc(zip_stream_type);
            if (!gz->dec)
                GZDEC_ERR(FAILED, "Decoder's initialization returned an error");
        }
    
        r = z_dec_decode(gz->dec, next_in, &avail_in, next_out, &avail_out);
#ifdef GST_1_0
        gst_buffer_unmap (out_buf, &out_map);
#endif
        if (r) {
            z_dec_free(&gz->dec);
            GZDEC_ERR(DECODE, "decoding failed");
        }

        bytes_to_write = allocated_out_buffer_size - avail_out;

        if (bytes_to_write) {
            // setup size of the output buffer.
#ifdef GST_1_0
            gst_buffer_resize (out_buf, 0, bytes_to_write);
#else
            GST_BUFFER_SIZE (out_buf) = bytes_to_write;
#endif

            if (gz->typefind) {
                GstTypeFindProbability prob;
                GstCaps *caps;
                caps =
                    gst_type_find_helper_for_buffer
                    (GST_OBJECT (gz), out_buf, &prob);
              
              if (caps) {
                gst_pad_set_caps
                    (GST_PAD (gz->srcpad), caps);
                gst_caps_unref (caps);
              }
              gz->typefind = FALSE;
            }
            
            // Pushing data downstream.
            flow = gst_pad_push (gz->srcpad, out_buf);
            buffer_pushed = TRUE;
            if (flow != GST_FLOW_OK)
                break;
        }
    }

done:
#ifdef GST_1_0
    gst_buffer_unmap (in_buf, &in_map);
#endif
    // If we didn't push the output buffer downstream,
    // we're the one who should free it.
    // If we pushed it, it's unreffered by the pad
    if(out_buf && !buffer_pushed)
        gst_buffer_unref(out_buf);
    gst_buffer_unref (in_buf);
    return flow;
}

static void
gst_gzdec_init (GstGzdec * gz
#ifndef GST_1_0
		, GstGzdecClass * klass
#endif
    )
{
    gz->buffer_size = DEFAULT_BUFFER_SIZE;

    gz->sinkpad = gst_pad_new_from_static_template
                  (&sink_template, "sink");
    gst_pad_set_chain_function
        (gz->sinkpad, GST_DEBUG_FUNCPTR (gst_gzdec_chain));
    gst_element_add_pad (GST_ELEMENT (gz), gz->sinkpad);

    gz->srcpad = gst_pad_new_from_static_template
                 (&src_template, "src");
    gst_element_add_pad (GST_ELEMENT (gz), gz->srcpad);
    gst_pad_use_fixed_caps (gz->srcpad);

    gz->dec = NULL;
    gz->typefind = TRUE;
}

#ifndef GST_1_0
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
    GstGzdec *gz = GST_GZDEC (object);

    z_dec_free(&gz->dec);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_gzdec_get_property (GObject * object, guint prop_id,
			GValue * value, GParamSpec * pspec)
{
    GstGzdec *gz = GST_GZDEC (object);

    switch (prop_id) {
    case PROP_BUFFER_SIZE:
        g_value_set_uint (value, gz->buffer_size);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gst_gzdec_set_property (GObject * object, guint prop_id,
			const GValue * value, GParamSpec * pspec)
{
    GstGzdec *gz = GST_GZDEC (object);

    switch (prop_id) {
    case PROP_BUFFER_SIZE:
        gz->buffer_size = g_value_get_uint (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GstStateChangeReturn
gst_gzdec_change_state (GstElement * element, GstStateChange transition)
{
    GstGzdec *gz = GST_GZDEC (element);
    GstStateChangeReturn ret;

    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        // so we could start with clean context
        z_dec_free(&gz->dec);
        gz->typefind = TRUE;
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

#ifdef GST_1_0
    gst_element_class_add_static_pad_template
        (gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template
        (gstelement_class, &src_template);
    gst_element_class_set_static_metadata
        (gstelement_class, "GZ/BZ2 decoder",
         "Codec/Decoder/Depayloader", "Decodes compressed streams",
         "Aleksandr Slobodeniuk");
#endif

    GST_DEBUG_CATEGORY_INIT (gzdec_debug, "gzdec", 0, "GZ/BZ2 decompressor");
}


static gboolean
plugin_init (GstPlugin * p)
{
    if (!gst_element_register
        (p, "gzdec", GST_RANK_PRIMARY, GST_TYPE_GZDEC))
        return FALSE;
    return TRUE;
}

GST_PLUGIN_DEFINE
(GST_VERSION_MAJOR, GST_VERSION_MINOR,
#ifdef GST_1_0
 gzdec,
#else
 "gzdec",
#endif
 "Decompress bz2 and gz streams",
 plugin_init, "0.1", "GPL", "gzdec",
 "https://github.com/aleniuk/gstgzdec.git")
