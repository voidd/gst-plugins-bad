/* GStreamer
 * Copyright (C) 2011 Flumotion S.L. <devteam@flumotion.com>
 * Copyright (C) 2011 Andoni Morales Alastruey <ylatuya@gmail.com>
 *
 * gstadaptivefragment.c:
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

#include <glib.h>
#include <string.h>
#include "gstadaptivefragment.h"

static gboolean gst_adaptive_fragment_meta_init (GstAdaptiveFragmentMeta * meta,
    gpointer params, GstBuffer * buffer);
static void gst_adaptive_fragment_meta_free (GstAdaptiveFragmentMeta * meta,
    GstBuffer * buffer);

GType
gst_adaptive_fragment_meta_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { "memory", NULL };

  if (g_once_init_enter (&type)) {
    GType _type =
        gst_meta_api_type_register ("GstAdaptiveFragmentMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static const GstMetaInfo *
gst_adaptive_fragment_meta_get_info (void)
{
  static const GstMetaInfo *fragment_meta_info = NULL;

  if (g_once_init_enter (&fragment_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_ADAPTIVE_FRAGMENT_META_API_TYPE,
        "GstAdaptiveFragmentMeta", sizeof (GstAdaptiveFragmentMeta),
        (GstMetaInitFunction) gst_adaptive_fragment_meta_init,
        (GstMetaFreeFunction) gst_adaptive_fragment_meta_free,
        (GstMetaTransformFunction) NULL);
    g_once_init_leave (&fragment_meta_info, meta);
  }
  return fragment_meta_info;
}

static gboolean
gst_adaptive_fragment_meta_init (GstAdaptiveFragmentMeta * meta,
    gpointer params, GstBuffer * buffer)
{
  meta->download_start_time = g_get_real_time ();
  meta->index = 0;
  meta->name = g_strdup ("");
  meta->completed = FALSE;
  meta->discontinuous = FALSE;
  meta->file = NULL;

  return TRUE;
}

static void
gst_adaptive_fragment_meta_free (GstAdaptiveFragmentMeta * meta,
    GstBuffer * buffer)
{
  if (meta->name != NULL) {
    g_free (meta->name);
  }

  if (meta->file != NULL) {
    g_object_unref (meta->file);
    meta->file = NULL;
  }
}

GstBuffer *
gst_adaptive_fragment_new (void)
{
  GstBuffer *buf;

  buf = gst_buffer_new ();
  gst_buffer_add_meta (buf, gst_adaptive_fragment_meta_get_info (), NULL);
  return buf;
}

static GstAdaptiveFragmentMeta *
gst_adaptive_fragment_get_meta (GstBuffer * buffer)
{
  GstAdaptiveFragmentMeta *meta;

  meta = gst_buffer_get_fragment_meta (buffer);
  if (!meta) {
    g_error ("Buffer is not a fragment.");
    return NULL;
  }

  return meta;
}

void
gst_adaptive_fragment_set_name (GstBuffer * fragment, gchar * name)
{
  GstAdaptiveFragmentMeta *meta;

  meta = gst_adaptive_fragment_get_meta (fragment);
  if (!meta) {
    return;
  }

  if (meta->name != NULL)
    g_free (meta->name);
  meta->name = name;
}

gboolean
gst_adaptive_fragment_add_buffer (GstBuffer * fragment, GstBuffer * buffer)
{
  GstAdaptiveFragmentMeta *meta;

  meta = gst_adaptive_fragment_get_meta (fragment);
  if (!meta) {
    return FALSE;
  }

  if (meta->completed) {
    GST_DEBUG ("Fragment is completed, you can't add new buffers to it");
    return FALSE;
  }
  gst_buffer_append (fragment, buffer);
  return TRUE;
}

void
gst_adaptive_fragment_set_file (GstBuffer * fragment, GFile * file)
{
  GstAdaptiveFragmentMeta *meta;

  meta = gst_adaptive_fragment_get_meta (fragment);
  if (!meta) {
    return;
  }

  if (meta->file != NULL) {
    g_object_unref (meta->file);
  }

  meta->file = file;
}
