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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkobject.h>
#include <libgnome/gnome-i18n.h>

#include "gnome-vfs-util.h"

#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>

/* =======================================================================
 * gdk-pixbuf handing stuff.
 *
 * Shamelessly stolen from nautilus-gdk-pixbuf-extensions.c which is
 * Copyright (C) 2000 Eazel, Inc.
 * Authors: Darin Adler <darin@eazel.com>
            Ramiro Estrugo <ramiro@eazel.com>
 *
 * =======================================================================
 */

#define LOAD_BUFFER_SIZE 4096

struct GnomeGdkPixbufAsyncHandle {
    GnomeVFSAsyncHandle *vfs_handle;
    GnomeGdkPixbufLoadCallback load_callback;
    GnomeGdkPixbufDoneCallback done_callback;
    gpointer callback_data;
    GdkPixbufLoader *loader;
    char buffer[LOAD_BUFFER_SIZE];
};

static void file_opened_callback (GnomeVFSAsyncHandle      *vfs_handle,
                                  GnomeVFSResult            result,
                                  gpointer                  callback_data);
static void file_read_callback   (GnomeVFSAsyncHandle      *vfs_handle,
                                  GnomeVFSResult            result,
                                  gpointer                  buffer,
                                  GnomeVFSFileSize          bytes_requested,
                                  GnomeVFSFileSize          bytes_read,
                                  gpointer                  callback_data);
static void file_closed_callback (GnomeVFSAsyncHandle      *handle,
                                  GnomeVFSResult            result,
                                  gpointer                  callback_data);
static void load_done            (GnomeGdkPixbufAsyncHandle *handle,
                                  GnomeVFSResult            result,
                                  GdkPixbuf                *pixbuf);

/* Loading a GdkPixbuf with a URI. */
GdkPixbuf *
gnome_gdk_pixbuf_new_from_uri (const char *uri)
{
    GnomeVFSResult result;
    GnomeVFSHandle *handle;
    char buffer[LOAD_BUFFER_SIZE];
    char *local_path;
    GnomeVFSFileSize bytes_read;
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;	

    g_return_val_if_fail (uri != NULL, NULL);

    /* FIXME bugzilla.eazel.com 1964: unfortunately, there are
     * bugs in the gdk_pixbuf_loader stuff that make it not work
     * for various image types like .xpms. Since
     * gdk_pixbuf_new_from_file uses different code that does not
     * have the same bugs and is better-tested, we call that when
     * the file is local. This should be fixed (in gdk_pixbuf)
     * eventually, then this hack can be removed.
     */
    local_path = gnome_vfs_get_local_path_from_uri (uri);
    if (local_path != NULL) {
	pixbuf = gdk_pixbuf_new_from_file (local_path, NULL);
	g_free (local_path);
	return pixbuf;
    }
	
    result = gnome_vfs_open (&handle,
			     uri,
			     GNOME_VFS_OPEN_READ);
    if (result != GNOME_VFS_OK) {
	return NULL;
    }

    loader = gdk_pixbuf_loader_new ();
    while (1) {
	result = gnome_vfs_read (handle,
				 buffer,
				 sizeof (buffer),
				 &bytes_read);
	if (result != GNOME_VFS_OK) {
	    break;
	}
	if (bytes_read == 0) {
	    break;
	}
	if (!gdk_pixbuf_loader_write (loader,
				      buffer,
				      bytes_read,
				      NULL)) {
	    result = GNOME_VFS_ERROR_WRONG_FORMAT;
	    break;
	}
    }

    if (result != GNOME_VFS_OK) {
	gtk_object_unref (GTK_OBJECT (loader));
	gnome_vfs_close (handle);
	return NULL;
    }

    gnome_vfs_close (handle);

    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    if (pixbuf != NULL) {
	gdk_pixbuf_ref (pixbuf);
    }
    gtk_object_unref (GTK_OBJECT (loader));

    return pixbuf;
}

GnomeGdkPixbufAsyncHandle *
gnome_gdk_pixbuf_new_from_uri_async (const char *uri,
				     GnomeGdkPixbufLoadCallback load_callback,
				     GnomeGdkPixbufDoneCallback done_callback,
				     gpointer callback_data)
{
    GnomeGdkPixbufAsyncHandle *handle;

    handle = g_new0 (GnomeGdkPixbufAsyncHandle, 1);
    handle->load_callback = load_callback;
    handle->done_callback = done_callback;
    handle->callback_data = callback_data;

    gnome_vfs_async_open (&handle->vfs_handle,
			  uri,
			  GNOME_VFS_OPEN_READ,
			  file_opened_callback,
			  handle);

    return handle;
}

static void
file_opened_callback (GnomeVFSAsyncHandle *vfs_handle,
		      GnomeVFSResult result,
		      gpointer callback_data)
{
    GnomeGdkPixbufAsyncHandle *handle;

    handle = callback_data;
    g_assert (handle->vfs_handle == vfs_handle);

    if (result != GNOME_VFS_OK) {
	load_done (handle, result, NULL);
	return;
    }

    handle->loader = gdk_pixbuf_loader_new ();

    gnome_vfs_async_read (handle->vfs_handle,
			  handle->buffer,
			  sizeof (handle->buffer),
			  file_read_callback,
			  handle);
}

static void
file_read_callback (GnomeVFSAsyncHandle *vfs_handle,
		    GnomeVFSResult result,
		    gpointer buffer,
		    GnomeVFSFileSize bytes_requested,
		    GnomeVFSFileSize bytes_read,
		    gpointer callback_data)
{
    GnomeGdkPixbufAsyncHandle *handle;

    handle = callback_data;
    g_assert (handle->vfs_handle == vfs_handle);
    g_assert (handle->buffer == buffer);

    if (result == GNOME_VFS_OK && bytes_read != 0) {
	if (!gdk_pixbuf_loader_write (handle->loader,
				      buffer,
				      bytes_read,
				      NULL)) {
	    result = GNOME_VFS_ERROR_WRONG_FORMAT;
	}
	gnome_vfs_async_read (handle->vfs_handle,
			      handle->buffer,
			      sizeof (handle->buffer),
			      file_read_callback,
			      handle);
	return;
    }

    switch (result) {
    case GNOME_VFS_OK:
	if (bytes_read == 0) {
	    GdkPixbuf *pixbuf;

	    pixbuf = gdk_pixbuf_loader_get_pixbuf (handle->loader);
	    load_done (handle, result, pixbuf);
	}
	break;
    case GNOME_VFS_ERROR_EOF:
	{
	    GdkPixbuf *pixbuf;

	    pixbuf = gdk_pixbuf_loader_get_pixbuf (handle->loader);
	    load_done (handle, pixbuf ? GNOME_VFS_OK : result, pixbuf);
	}
	break;
    default:
	load_done (handle, result, NULL);
	break;
    }
}

static void
file_closed_callback (GnomeVFSAsyncHandle *handle,
		      GnomeVFSResult result,
		      gpointer callback_data)
{
    g_assert (callback_data == NULL);
}

static void
free_pixbuf_load_handle (GnomeGdkPixbufAsyncHandle *handle)
{
    if (handle->done_callback)
	(* handle->done_callback) (handle, handle->callback_data);
    if (handle->loader != NULL) {
	gdk_pixbuf_loader_close (handle->loader, NULL);
	g_object_unref (G_OBJECT (handle->loader));
    }
    g_free (handle);
}

static void
load_done (GnomeGdkPixbufAsyncHandle *handle,
	   GnomeVFSResult result,
	   GdkPixbuf *pixbuf)
{
    if (handle->vfs_handle != NULL) {
	gnome_vfs_async_close (handle->vfs_handle, file_closed_callback, NULL);
    }
    (* handle->load_callback) (handle, result, pixbuf, handle->callback_data);
    free_pixbuf_load_handle (handle);
}

void
gnome_gdk_pixbuf_new_from_uri_cancel (GnomeGdkPixbufAsyncHandle *handle)
{
    if (handle == NULL) {
	return;
    }
    if (handle->vfs_handle != NULL) {
	gnome_vfs_async_cancel (handle->vfs_handle);
    }
    free_pixbuf_load_handle (handle);
}
