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
#include <glib-object.h>

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

typedef struct {
    gint width;
    gint height;
    gint input_width;
    gint input_height;
    gboolean preserve_aspect_ratio;
} SizePrepareContext;


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

/**
 * gnome_gdk_pixbuf_new_from_uri:
 * @uri: the uri of an image
 * 
 * Loads a GdkPixbuf from the image file @uri points to
 * 
 * Return value: The pixbuf, or NULL on error
 **/
GdkPixbuf *
gnome_gdk_pixbuf_new_from_uri (const char *uri)
{
	return gnome_gdk_pixbuf_new_from_uri_at_scale(uri, -1, -1, TRUE);
}

static void
size_prepared_cb (GdkPixbufLoader *loader, 
		  int              width,
		  int              height,
		  gpointer         data)
{
	SizePrepareContext *info = data;

	g_return_if_fail (width > 0 && height > 0);

	info->input_width = width;
	info->input_height = height;
	
	if (width < info->width && height < info->height) return;
	
	if (info->preserve_aspect_ratio && 
	    (info->width > 0 || info->height > 0)) {
		if (info->width < 0)
		{
			width = width * (double)info->height/(double)height;
			height = info->height;
		}
		else if (info->height < 0)
		{
			height = height * (double)info->width/(double)width;
			width = info->width;
		}
		else if ((double)height * (double)info->width >
			 (double)width * (double)info->height) {
			width = 0.5 + (double)width * (double)info->height / (double)height;
			height = info->height;
		} else {
			height = 0.5 + (double)height * (double)info->width / (double)width;
			width = info->width;
		}
	} else {
		if (info->width > 0)
			width = info->width;
		if (info->height > 0)
			height = info->height;
	}
	
	gdk_pixbuf_loader_set_size (loader, width, height);
}

/**
 * gnome_gdk_pixbuf_new_from_uri:
 * @uri: the uri of an image
 * @width: The width the image should have or -1 to not constrain the width
 * @height: The height the image should have or -1 to not constrain the height
 * @preserve_aspect_ratio: %TRUE to preserve the image's aspect ratio
 * 
 * Loads a GdkPixbuf from the image file @uri points to, scaling it to the
 * desired size. If you pass -1 for @width or @height then the value
 * specified in the file will be used.
 *
 * When preserving aspect ratio, if both height and width are set the size
 * is picked such that the scaled image fits in a width * height rectangle.
 * 
 * Return value: The loaded pixbuf, or NULL on error
 *
 * Since: 2.14
 **/
GdkPixbuf *
gnome_gdk_pixbuf_new_from_uri_at_scale (const char *uri,
					gint        width,
					gint        height,
					gboolean    preserve_aspect_ratio)
{
    GnomeVFSResult result;
    GnomeVFSHandle *handle;
    char buffer[LOAD_BUFFER_SIZE];
    GnomeVFSFileSize bytes_read;
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;	
    GdkPixbufAnimation *animation;
    GdkPixbufAnimationIter *iter;
    gboolean has_frame;
    SizePrepareContext info;

    g_return_val_if_fail (uri != NULL, NULL);

    result = gnome_vfs_open (&handle,
			     uri,
			     GNOME_VFS_OPEN_READ);
    if (result != GNOME_VFS_OK) {
	return NULL;
    }

    loader = gdk_pixbuf_loader_new ();
    if (1 <= width || 1 <= height) {
        info.width = width;
        info.height = height;
	info.input_width = info.input_height = 0;
        info.preserve_aspect_ratio = preserve_aspect_ratio;        
        g_signal_connect (loader, "size-prepared", G_CALLBACK (size_prepared_cb), &info);
    }

    has_frame = FALSE;

    while (!has_frame) {
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
				      (unsigned char *)buffer,
				      bytes_read,
				      NULL)) {
	    result = GNOME_VFS_ERROR_WRONG_FORMAT;
	    break;
	}

	animation = gdk_pixbuf_loader_get_animation (loader);
	if (animation) {
		iter = gdk_pixbuf_animation_get_iter (animation, 0);
		if (!gdk_pixbuf_animation_iter_on_currently_loading_frame (iter)) {
			has_frame = TRUE;
		}
		g_object_unref (iter);
	}
    }

    gdk_pixbuf_loader_close (loader, NULL);
    
    if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF) {
	g_object_unref (G_OBJECT (loader));
	gnome_vfs_close (handle);
	return NULL;
    }

    gnome_vfs_close (handle);

    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    if (pixbuf != NULL) {
	g_object_ref (G_OBJECT (pixbuf));
    }
    g_object_unref (G_OBJECT (loader));

    g_object_set_data (G_OBJECT (pixbuf), "gnome-original-width",
		       GINT_TO_POINTER (info.input_width));
    g_object_set_data (G_OBJECT (pixbuf), "gnome-original-height",
		       GINT_TO_POINTER (info.input_height));
    
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
			  GNOME_VFS_PRIORITY_DEFAULT,
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
	if (result != GNOME_VFS_OK)
	    gnome_vfs_async_cancel (handle->vfs_handle);
	else
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
