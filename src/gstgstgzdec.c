/* GStreamer
 * Copyright (C) 2017 FIXME <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstgstgzdec
 *
 * The gstgzdec element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! gstgzdec ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gst.h>
#include "gstgstgzdec.h"

GST_DEBUG_CATEGORY_STATIC (gst_gstgzdec_debug_category);
#define GST_CAT_DEFAULT gst_gstgzdec_debug_category

/* prototypes */


static void gst_gstgzdec_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_gstgzdec_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_gstgzdec_dispose (GObject * object);
static void gst_gstgzdec_finalize (GObject * object);

static GstPad *gst_gstgzdec_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name);
static void gst_gstgzdec_release_pad (GstElement * element, GstPad * pad);
static GstStateChangeReturn
gst_gstgzdec_change_state (GstElement * element, GstStateChange transition);
static GstClock *gst_gstgzdec_provide_clock (GstElement * element);
static gboolean gst_gstgzdec_set_clock (GstElement * element, GstClock * clock);
static GstIndex *gst_gstgzdec_get_index (GstElement * element);
static void gst_gstgzdec_set_index (GstElement * element, GstIndex * index);
static gboolean gst_gstgzdec_send_event (GstElement * element, GstEvent * event);
static gboolean gst_gstgzdec_query (GstElement * element, GstQuery * query);

static GstCaps* gst_gstgzdec_sink_getcaps (GstPad *pad);
static gboolean gst_gstgzdec_sink_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_gstgzdec_sink_acceptcaps (GstPad *pad, GstCaps *caps);
static void gst_gstgzdec_sink_fixatecaps (GstPad *pad, GstCaps *caps);
static gboolean gst_gstgzdec_sink_activate (GstPad *pad);
static gboolean gst_gstgzdec_sink_activatepush (GstPad *pad, gboolean active);
static gboolean gst_gstgzdec_sink_activatepull (GstPad *pad, gboolean active);
static GstPadLinkReturn gst_gstgzdec_sink_link (GstPad *pad, GstPad *peer);
static void gst_gstgzdec_sink_unlink (GstPad *pad);
static GstFlowReturn gst_gstgzdec_sink_chain (GstPad *pad, GstBuffer *buffer);
static GstFlowReturn gst_gstgzdec_sink_chainlist (GstPad *pad, GstBufferList *bufferlist);
static gboolean gst_gstgzdec_sink_event (GstPad *pad, GstEvent *event);
static gboolean gst_gstgzdec_sink_query (GstPad *pad, GstQuery *query);
static GstFlowReturn gst_gstgzdec_sink_bufferalloc (GstPad *pad, guint64 offset, guint size,
    GstCaps *caps, GstBuffer **buf);
static GstIterator * gst_gstgzdec_sink_iterintlink (GstPad *pad);


static GstCaps* gst_gstgzdec_src_getcaps (GstPad *pad);
static gboolean gst_gstgzdec_src_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_gstgzdec_src_acceptcaps (GstPad *pad, GstCaps *caps);
static void gst_gstgzdec_src_fixatecaps (GstPad *pad, GstCaps *caps);
static gboolean gst_gstgzdec_src_activate (GstPad *pad);
static gboolean gst_gstgzdec_src_activatepush (GstPad *pad, gboolean active);
static gboolean gst_gstgzdec_src_activatepull (GstPad *pad, gboolean active);
static GstPadLinkReturn gst_gstgzdec_src_link (GstPad *pad, GstPad *peer);
static void gst_gstgzdec_src_unlink (GstPad *pad);
static GstFlowReturn gst_gstgzdec_src_getrange (GstPad *pad, guint64 offset, guint length,
    GstBuffer **buffer);
static gboolean gst_gstgzdec_src_event (GstPad *pad, GstEvent *event);
static gboolean gst_gstgzdec_src_query (GstPad *pad, GstQuery *query);
static GstIterator * gst_gstgzdec_src_iterintlink (GstPad *pad);


enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_gstgzdec_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );

static GstStaticPadTemplate gst_gstgzdec_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstGstgzdec, gst_gstgzdec, GST_TYPE_ELEMENT,
  GST_DEBUG_CATEGORY_INIT (gst_gstgzdec_debug_category, "gstgzdec", 0,
  "debug category for gstgzdec element"));

static void
gst_gstgzdec_class_init (GstGstgzdecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (element_class,
      &gst_gstgzdec_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_gstgzdec_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_gstgzdec_set_property;
  gobject_class->get_property = gst_gstgzdec_get_property;
  gobject_class->dispose = gst_gstgzdec_dispose;
  gobject_class->finalize = gst_gstgzdec_finalize;
  element_class->request_new_pad = GST_DEBUG_FUNCPTR (gst_gstgzdec_request_new_pad);
  element_class->release_pad = GST_DEBUG_FUNCPTR (gst_gstgzdec_release_pad);
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_gstgzdec_change_state);
  element_class->provide_clock = GST_DEBUG_FUNCPTR (gst_gstgzdec_provide_clock);
  element_class->set_clock = GST_DEBUG_FUNCPTR (gst_gstgzdec_set_clock);
  element_class->get_index = GST_DEBUG_FUNCPTR (gst_gstgzdec_get_index);
  element_class->set_index = GST_DEBUG_FUNCPTR (gst_gstgzdec_set_index);
  element_class->send_event = GST_DEBUG_FUNCPTR (gst_gstgzdec_send_event);
  element_class->query = GST_DEBUG_FUNCPTR (gst_gstgzdec_query);

}

static void
gst_gstgzdec_init (GstGstgzdec *gstgzdec)
{

  gstgzdec->sinkpad = gst_pad_new_from_static_template (&gst_gstgzdec_sink_template
      ,     
            "sink");
  gst_pad_set_getcaps_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_getcaps));
  gst_pad_set_setcaps_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_setcaps));
  gst_pad_set_acceptcaps_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_acceptcaps));
  gst_pad_set_fixatecaps_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_fixatecaps));
  gst_pad_set_activate_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_activate));
  gst_pad_set_activatepush_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_activatepush));
  gst_pad_set_activatepull_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_activatepull));
  gst_pad_set_link_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_link));
  gst_pad_set_unlink_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_unlink));
  gst_pad_set_chain_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_chain));
  gst_pad_set_chain_list_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_chainlist));
  gst_pad_set_event_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_event));
  gst_pad_set_query_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_query));
  gst_pad_set_bufferalloc_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_bufferalloc));
  gst_pad_set_iterate_internal_links_function (gstgzdec->sinkpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_sink_iterintlink));
  gst_element_add_pad (GST_ELEMENT(gstgzdec), gstgzdec->sinkpad);



  gstgzdec->srcpad = gst_pad_new_from_static_template (&gst_gstgzdec_src_template
      ,     
            "src");
  gst_pad_set_getcaps_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_getcaps));
  gst_pad_set_setcaps_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_setcaps));
  gst_pad_set_acceptcaps_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_acceptcaps));
  gst_pad_set_fixatecaps_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_fixatecaps));
  gst_pad_set_activate_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_activate));
  gst_pad_set_activatepush_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_activatepush));
  gst_pad_set_activatepull_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_activatepull));
  gst_pad_set_link_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_link));
  gst_pad_set_unlink_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_unlink));
  gst_pad_set_getrange_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_getrange));
  gst_pad_set_event_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_event));
  gst_pad_set_query_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_query));
  gst_pad_set_iterate_internal_links_function (gstgzdec->srcpad,
            GST_DEBUG_FUNCPTR(gst_gstgzdec_src_iterintlink));
  gst_element_add_pad (GST_ELEMENT(gstgzdec), gstgzdec->srcpad);


}

void
gst_gstgzdec_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGstgzdec *gstgzdec = GST_GSTGZDEC (object);

  GST_DEBUG_OBJECT (gstgzdec, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_gstgzdec_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstGstgzdec *gstgzdec = GST_GSTGZDEC (object);

  GST_DEBUG_OBJECT (gstgzdec, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_gstgzdec_dispose (GObject * object)
{
  GstGstgzdec *gstgzdec = GST_GSTGZDEC (object);

  GST_DEBUG_OBJECT (gstgzdec, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_gstgzdec_parent_class)->dispose (object);
}

void
gst_gstgzdec_finalize (GObject * object)
{
  GstGstgzdec *gstgzdec = GST_GSTGZDEC (object);

  GST_DEBUG_OBJECT (gstgzdec, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_gstgzdec_parent_class)->finalize (object);
}



static GstPad *
gst_gstgzdec_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name)
{

  return NULL;
}

static void
gst_gstgzdec_release_pad (GstElement * element, GstPad * pad)
{

}

static GstStateChangeReturn
gst_gstgzdec_change_state (GstElement * element, GstStateChange transition)
{
  GstGstgzdec *gstgzdec;
  GstStateChangeReturn ret;

  g_return_val_if_fail (GST_IS_GSTGZDEC (element), GST_STATE_CHANGE_FAILURE);
  gstgzdec = GST_GSTGZDEC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static GstClock *
gst_gstgzdec_provide_clock (GstElement * element)
{

  return NULL;
}

static gboolean
gst_gstgzdec_set_clock (GstElement * element, GstClock * clock)
{

  return GST_ELEMENT_CLASS (parent_class)->set_clock (element, clock);
}

static GstIndex *
gst_gstgzdec_get_index (GstElement * element)
{

  return NULL;
}

static void
gst_gstgzdec_set_index (GstElement * element, GstIndex * index)
{

}

static gboolean
gst_gstgzdec_send_event (GstElement * element, GstEvent * event)
{

  return TRUE;
}

static gboolean
gst_gstgzdec_query (GstElement * element, GstQuery * query)
{
  GstGstgzdec *gstgzdec = GST_GSTGZDEC (element);
  gboolean ret;

  GST_DEBUG_OBJECT (gstgzdec, "query");

  switch (GST_QUERY_TYPE (query)) {
    default:
      ret = GST_ELEMENT_CLASS (parent_class)->query (element, query);
      break;
  }

  return ret;
}

static GstCaps*
gst_gstgzdec_sink_getcaps (GstPad *pad)
{
  GstGstgzdec *gstgzdec;
  GstCaps *caps;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "getcaps");

  caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));

  gst_object_unref (gstgzdec);
  return caps;
}

static gboolean
gst_gstgzdec_sink_setcaps (GstPad *pad, GstCaps *caps)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "setcaps");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static gboolean
gst_gstgzdec_sink_acceptcaps (GstPad *pad, GstCaps *caps)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "acceptcaps");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static void
gst_gstgzdec_sink_fixatecaps (GstPad *pad, GstCaps *caps)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "fixatecaps");


  gst_object_unref (gstgzdec);
}

static gboolean
gst_gstgzdec_sink_activate (GstPad *pad)
{
  GstGstgzdec *gstgzdec;
  gboolean ret;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "activate");

  if (gst_pad_check_pull_range (pad)) {
    GST_DEBUG_OBJECT (pad, "activating pull");
    ret = gst_pad_activate_pull (pad, TRUE);
  } else {
    GST_DEBUG_OBJECT (pad, "activating push");
    ret = gst_pad_activate_push (pad, TRUE);
  }

  gst_object_unref (gstgzdec);
  return ret;
}

static gboolean
gst_gstgzdec_sink_activatepush (GstPad *pad, gboolean active)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "activatepush");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static gboolean
gst_gstgzdec_sink_activatepull (GstPad *pad, gboolean active)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "activatepull");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static GstPadLinkReturn
gst_gstgzdec_sink_link (GstPad *pad, GstPad *peer)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "link");


  gst_object_unref (gstgzdec);
  return GST_PAD_LINK_OK;
}

static void
gst_gstgzdec_sink_unlink (GstPad *pad)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "unlink");


  gst_object_unref (gstgzdec);
}

static GstFlowReturn
gst_gstgzdec_sink_chain (GstPad *pad, GstBuffer *buffer)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "chain");


  gst_object_unref (gstgzdec);
  return GST_FLOW_OK;
}

static GstFlowReturn
gst_gstgzdec_sink_chainlist (GstPad *pad, GstBufferList *bufferlist)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "chainlist");


  gst_object_unref (gstgzdec);
  return GST_FLOW_OK;
}

static gboolean
gst_gstgzdec_sink_event (GstPad *pad, GstEvent *event)
{
  gboolean res;
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "event");

  switch (GST_EVENT_TYPE (event)) {
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (gstgzdec);
  return res;
}

static gboolean
gst_gstgzdec_sink_query (GstPad *pad, GstQuery *query)
{
  gboolean res;
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "query");

  switch (GST_QUERY_TYPE(query)) {
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

  gst_object_unref (gstgzdec);
  return res;
}

static GstFlowReturn
gst_gstgzdec_sink_bufferalloc (GstPad *pad, guint64 offset, guint size,
    GstCaps *caps, GstBuffer **buf)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "bufferalloc");


  *buf = gst_buffer_new_and_alloc (size);
  gst_buffer_set_caps (*buf, caps);

  gst_object_unref (gstgzdec);
  return GST_FLOW_OK;
}

static GstIterator *
gst_gstgzdec_sink_iterintlink (GstPad *pad)
{
  GstGstgzdec *gstgzdec;
  GstIterator *iter;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "iterintlink");

  iter = gst_pad_iterate_internal_links_default (pad);

  gst_object_unref (gstgzdec);
  return iter;
}


static GstCaps*
gst_gstgzdec_src_getcaps (GstPad *pad)
{
  GstGstgzdec *gstgzdec;
  GstCaps *caps;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "getcaps");

  caps = gst_pad_get_pad_template_caps (pad);

  gst_object_unref (gstgzdec);
  return caps;
}

static gboolean
gst_gstgzdec_src_setcaps (GstPad *pad, GstCaps *caps)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "setcaps");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static gboolean
gst_gstgzdec_src_acceptcaps (GstPad *pad, GstCaps *caps)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "acceptcaps");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static void
gst_gstgzdec_src_fixatecaps (GstPad *pad, GstCaps *caps)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "fixatecaps");


  gst_object_unref (gstgzdec);
}

static gboolean
gst_gstgzdec_src_activate (GstPad *pad)
{
  GstGstgzdec *gstgzdec;
  gboolean ret;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "activate");

  if (gst_pad_check_pull_range (pad)) {
    GST_DEBUG_OBJECT (pad, "activating pull");
    ret = gst_pad_activate_pull (pad, TRUE);
  } else {
    GST_DEBUG_OBJECT (pad, "activating push");
    ret = gst_pad_activate_push (pad, TRUE);
  }

  gst_object_unref (gstgzdec);
  return ret;
}

static gboolean
gst_gstgzdec_src_activatepush (GstPad *pad, gboolean active)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "activatepush");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static gboolean
gst_gstgzdec_src_activatepull (GstPad *pad, gboolean active)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "activatepull");


  gst_object_unref (gstgzdec);
  return TRUE;
}

static GstPadLinkReturn
gst_gstgzdec_src_link (GstPad *pad, GstPad *peer)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "link");


  gst_object_unref (gstgzdec);
  return GST_PAD_LINK_OK;
}

static void
gst_gstgzdec_src_unlink (GstPad *pad)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "unlink");


  gst_object_unref (gstgzdec);
}

static GstFlowReturn
gst_gstgzdec_src_getrange (GstPad *pad, guint64 offset, guint length,
    GstBuffer **buffer)
{
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "getrange");


  gst_object_unref (gstgzdec);
  return GST_FLOW_OK;
}

static gboolean
gst_gstgzdec_src_event (GstPad *pad, GstEvent *event)
{
  gboolean res;
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "event");

  switch (GST_EVENT_TYPE (event)) {
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (gstgzdec);
  return res;
}

static gboolean
gst_gstgzdec_src_query (GstPad *pad, GstQuery *query)
{
  gboolean res;
  GstGstgzdec *gstgzdec;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "query");

  switch (GST_QUERY_TYPE(query)) {
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

  gst_object_unref (gstgzdec);
  return res;
}

static GstIterator *
gst_gstgzdec_src_iterintlink (GstPad *pad)
{
  GstGstgzdec *gstgzdec;
  GstIterator *iter;

  gstgzdec = GST_GSTGZDEC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(gstgzdec, "iterintlink");

  iter = gst_pad_iterate_internal_links_default (pad);

  gst_object_unref (gstgzdec);
  return iter;
}


static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "gstgzdec", GST_RANK_NONE,
      GST_TYPE_GSTGZDEC);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    gstgzdec,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

