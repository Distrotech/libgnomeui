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

#include <libgnome/gnome-defs.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomevfs/gnome-vfs-types.h>

BEGIN_GNOME_DECLS

/* =======================================================================
 * gdk-pixbuf handing stuff.
 *
 * Shamelessly stolen from nautilus-gdk-pixbuf-extensions.h which is
 * Copyright (C) 2000 Eazel, Inc.
 * Authors: Darin Adler <darin@eazel.com>
 *
 * =======================================================================
 */

typedef struct GnomeGdkPixbufLoadHandle    GnomeGdkPixbufLoadHandle;
typedef void (*GnomeGdkPixbufLoadCallback) (GnomeVFSResult  error,
                                            GdkPixbuf      *pixbuf,
                                            gpointer        callback_data);

/* Loading a GdkPixbuf with a URI. */
GdkPixbuf *
gnome_gdk_pixbuf_new_from_uri        (const char                 *uri);

/* Same thing async. */
GnomeGdkPixbufLoadHandle *
gnome_gdk_pixbuf_new_from_uri_async  (const char                 *uri,
                                      GnomeGdkPixbufLoadCallback  callback,
                                      gpointer                    callback_data);

void
gnome_gdk_pixbuf_new_from_uri_cancel (GnomeGdkPixbufLoadHandle   *handle);

END_GNOME_DECLS

#endif
