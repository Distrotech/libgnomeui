/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef GNOME_VFS_UTIL_H
#define GNOME_VFS_UTIL_H



#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomevfs/gnome-vfs-result.h>

G_BEGIN_DECLS

/* =======================================================================
 * gdk-pixbuf handing stuff.
 *
 * Shamelessly stolen from nautilus-gdk-pixbuf-extensions.h which is
 * Copyright (C) 2000 Eazel, Inc.
 * Authors: Darin Adler <darin@eazel.com>
 *
 * =======================================================================
 */

typedef struct GnomeGdkPixbufAsyncHandle    GnomeGdkPixbufAsyncHandle;
typedef void (*GnomeGdkPixbufLoadCallback) (GnomeGdkPixbufAsyncHandle *handle,
                                            GnomeVFSResult             error,
                                            GdkPixbuf                 *pixbuf,
                                            gpointer                   cb_data);
typedef void (*GnomeGdkPixbufDoneCallback) (GnomeGdkPixbufAsyncHandle *handle,
                                            gpointer                   cb_data);

/* Loading a GdkPixbuf with a URI. */
GdkPixbuf *
gnome_gdk_pixbuf_new_from_uri        (const char                 *uri);

/* Loading a GdkPixbuf with a URI. The image will be scaled to fit in the
 * requested size. */
GdkPixbuf *
gnome_gdk_pixbuf_new_from_uri_at_scale (const char                 *uri,
                                       gint                        width,
                                       gint                        height,
                                       gboolean                    preserve_aspect_ratio);

/* Same thing async. */
GnomeGdkPixbufAsyncHandle *
gnome_gdk_pixbuf_new_from_uri_async  (const char                 *uri,
                                      GnomeGdkPixbufLoadCallback  load_callback,
                                      GnomeGdkPixbufDoneCallback  done_callback,
                                      gpointer                    callback_data);

void
gnome_gdk_pixbuf_new_from_uri_cancel (GnomeGdkPixbufAsyncHandle  *handle);

G_END_DECLS

#endif
